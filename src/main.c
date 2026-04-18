#include <pebble.h>
#include "game_state.h"
#include "math_utils.h"
#include "shop_menu.h"

static Window *s_main_window;
static TextLayer *s_mass_layer, *s_gravity_layer;
static Layer *s_canvas_layer;
static GameState s_state;
static double s_next_tier_cost __attribute__((aligned(8))) = PRESTIGE_THRESHOLD;
static AppTimer *s_tap_timer = NULL;
static int s_hold_time_ms = 0;

static void update_display();
static void update_next_tier_cost();

#if defined(PBL_HEALTH)
static int s_last_step_count = 0;

static void health_handler(HealthEventType event, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "health A: event=%d", (int)event);
  if (event == HealthEventSignificantUpdate) {
    s_last_step_count = 0;
  }

  APP_LOG(APP_LOG_LEVEL_DEBUG, "health B: checking accessible");
  const time_t start = time_start_of_today();
  const time_t end = time(NULL);
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(HealthMetricStepCount, start, end);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "health C: mask=%d", (int)mask);

  if (mask & HealthServiceAccessibilityMaskAvailable) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "health D: sum_today");
    int total_steps = (int)health_service_sum_today(HealthMetricStepCount);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "health E: steps=%d last=%d", total_steps, s_last_step_count);

    if (s_last_step_count == 0) {
      s_last_step_count = total_steps;
    } else {
      int delta = total_steps - s_last_step_count;
      if (delta > 0) {
        game_state_add_steps(&s_state, delta);
        update_display();
      }
      s_last_step_count = total_steps;
    }
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "health F: done");
}
#endif

static void tap_timer_callback(void *data) {
  // Fire a tap
  s_state.mass += game_state_calculate_tap_strength(&s_state);
  update_display();
  
  s_hold_time_ms += 200;
  
  // If held for 5 seconds and threshold met, trigger Big Bang
  if (s_hold_time_ms >= 5000 && s_state.mass >= PRESTIGE_THRESHOLD) {
    APP_LOG(APP_LOG_LEVEL_INFO, "BIG BANG TRIGGERED");
    game_state_prestige(&s_state);
    update_next_tier_cost();
    update_display();
    vibes_double_pulse();
    s_tap_timer = NULL;
    return; 
  }

  s_tap_timer = app_timer_register(200, tap_timer_callback, NULL);
}

static void select_down_handler(ClickRecognizerRef recognizer, void *context) {
  s_hold_time_ms = 0;
  if (s_tap_timer) app_timer_cancel(s_tap_timer);
  s_tap_timer = app_timer_register(200, tap_timer_callback, NULL);
  
  // Fire immediate first tap
  s_state.mass += game_state_calculate_tap_strength(&s_state);
  update_display();
}

static void select_up_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_tap_timer) {
    app_timer_cancel(s_tap_timer);
    s_tap_timer = NULL;
  }
}

static void update_next_tier_cost() {
  s_next_tier_cost = PRESTIGE_THRESHOLD;
  for (int i = 0; i < NUM_TIERS; i++) {
    double cost = calculate_cost(TIERS[i].base_cost, s_state.counts[i]);
    if (s_state.mass < cost) {
      s_next_tier_cost = cost;
      break;
    }
  }
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "canvas A");
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);

  double goal = (s_next_tier_cost > 0) ? s_next_tier_cost : PRESTIGE_THRESHOLD;
  double ratio = s_state.mass / goal;
  // Clamp covers NaN, Inf, negatives — NaN comparisons always return false so
  // a plain "> 1.0" clamp lets NaN through and causes UB in the (int) cast.
  if (!(ratio >= 0.0 && ratio <= 1.0)) ratio = 0.0;

  int radius = 10 + (int)(ratio * 50);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "canvas B: radius=%d", radius);

  GColor fill_color = PBL_IF_COLOR_ELSE(GColorElectricBlue, GColorWhite);
  if (s_state.mass >= PRESTIGE_THRESHOLD * 0.9) fill_color = GColorRed;

  graphics_context_set_fill_color(ctx, fill_color);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "canvas C: fill");
  graphics_fill_circle(ctx, center, radius);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "canvas D: stroke");
  graphics_context_set_stroke_width(ctx, 3);
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorVividViolet, GColorWhite));
  graphics_draw_circle(ctx, center, radius + 2);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "canvas E: done");
}

static void update_display() {
  if (!s_mass_layer || !s_gravity_layer) return;

  static char s_mass_buffer[64];
  static char s_gravity_buffer[64];
  char val_buffer[32];
  
  format_mass(s_state.mass, val_buffer);
  snprintf(s_mass_buffer, sizeof(s_mass_buffer), "%s mg", val_buffer);
  text_layer_set_text(s_mass_layer, s_mass_buffer);
  
  double current_gravity = game_state_calculate_gravity(&s_state);
  format_mass(current_gravity, val_buffer);
  snprintf(s_gravity_buffer, sizeof(s_gravity_buffer), "G: %s/s", val_buffer);
  text_layer_set_text(s_gravity_layer, s_gravity_buffer);

  if (s_state.mass >= s_next_tier_cost) update_next_tier_cost();
  if (s_canvas_layer) layer_mark_dirty(s_canvas_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (tick_time->tm_sec % 10 == 0) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Heartbeat: App is alive");
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "tick A: sec=%d", tick_time->tm_sec);

  double gravity = game_state_calculate_gravity(&s_state);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "tick B: gravity ok");

  s_state.mass += gravity;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "tick C: mass ok");

  update_display();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "tick D: display ok");

  if (tick_time->tm_sec == 0) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "tick E: saving");
    game_state_save(&s_state);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "tick F: saved");
  }
}

static void open_shop_handler(ClickRecognizerRef recognizer, void *context) {
  shop_menu_show(&s_state, update_display);
}

static void click_config_provider(void *context) {
  // Fix SDK conflict by using RAW clicks for SELECT
  window_raw_click_subscribe(BUTTON_ID_SELECT, select_down_handler, select_up_handler, NULL);
  
  window_single_click_subscribe(BUTTON_ID_UP, open_shop_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, open_shop_handler);
}

static void main_window_appear(Window *window) {
  APP_LOG(APP_LOG_LEVEL_INFO, "main_window_appear: subscribing services");
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  #if defined(PBL_HEALTH)
  if (!health_service_events_subscribe(health_handler, NULL)) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "main_window_appear: health subscribe FAILED");
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "main_window_appear: health subscribed OK");
  }
  #endif
}

static void main_window_disappear(Window *window) {
  APP_LOG(APP_LOG_LEVEL_INFO, "main_window_disappear: unsubscribing services");
  tick_timer_service_unsubscribe();
  #if defined(PBL_HEALTH)
  health_service_events_unsubscribe();
  #endif
}

static void main_window_load(Window *window) {
  APP_LOG(APP_LOG_LEVEL_INFO, "main_window_load: start");
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  APP_LOG(APP_LOG_LEVEL_INFO, "main_window_load: bounds %dx%d", bounds.size.w, bounds.size.h);
  window_set_background_color(window, GColorBlack);

  s_canvas_layer = layer_create(bounds);
  if (!s_canvas_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "main_window_load: canvas layer alloc FAILED");
    return;
  }
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);
  APP_LOG(APP_LOG_LEVEL_INFO, "main_window_load: canvas layer OK");

  s_mass_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(40, 30), bounds.size.w, 30));
  if (!s_mass_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "main_window_load: mass layer alloc FAILED");
    return;
  }
  text_layer_set_background_color(s_mass_layer, GColorClear);
  text_layer_set_text_color(s_mass_layer, PBL_IF_COLOR_ELSE(GColorCeleste, GColorWhite));
  text_layer_set_text_alignment(s_mass_layer, GTextAlignmentCenter);
  text_layer_set_font(s_mass_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_mass_layer));
  APP_LOG(APP_LOG_LEVEL_INFO, "main_window_load: mass layer OK");

  s_gravity_layer = text_layer_create(GRect(0, bounds.size.h - PBL_IF_ROUND_ELSE(50, 40), bounds.size.w, 20));
  if (!s_gravity_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "main_window_load: gravity layer alloc FAILED");
    return;
  }
  text_layer_set_background_color(s_gravity_layer, GColorClear);
  text_layer_set_text_color(s_gravity_layer, PBL_IF_COLOR_ELSE(GColorCeleste, GColorWhite));
  text_layer_set_text_alignment(s_gravity_layer, GTextAlignmentCenter);
  text_layer_set_font(s_gravity_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_gravity_layer));
  APP_LOG(APP_LOG_LEVEL_INFO, "main_window_load: gravity layer OK");

  update_display();
  APP_LOG(APP_LOG_LEVEL_INFO, "main_window_load: complete");
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_mass_layer);
  text_layer_destroy(s_gravity_layer);
  layer_destroy(s_canvas_layer);
  s_mass_layer = NULL; s_gravity_layer = NULL; s_canvas_layer = NULL;
}

static void init() {
  APP_LOG(APP_LOG_LEVEL_INFO, "init: start");

  if (game_state_load(&s_state)) {
    APP_LOG(APP_LOG_LEVEL_INFO, "init: save loaded (version=%lu)", (unsigned long)s_state.version);
    double gained = game_state_apply_offline_gains(&s_state);
    if (gained > 0) {
      APP_LOG(APP_LOG_LEVEL_INFO, "init: offline gains applied");
    }
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "init: no valid save, starting fresh");
    game_state_init(&s_state);
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "init: mass=%d dust=%d",
          (int)s_state.mass, (int)s_state.dust);

  update_next_tier_cost();

  s_main_window = window_create();
  if (!s_main_window) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "init: window_create FAILED");
    return;
  }
  APP_LOG(APP_LOG_LEVEL_INFO, "init: window created, pushing to stack");

  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
    .appear = main_window_appear,
    .disappear = main_window_disappear
  });
  window_stack_push(s_main_window, true);
  APP_LOG(APP_LOG_LEVEL_INFO, "init: complete");
}

static void deinit() {
  game_state_save(&s_state);
  window_destroy(s_main_window);
}

int main(void) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "=== MAIN ENTRY ===");
  init();
  app_event_loop();
  deinit();
}
