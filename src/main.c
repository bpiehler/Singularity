#include <pebble.h>
#include <math.h>
#include "game_state.h"
#include "math_utils.h"
#include "shop_menu.h"

static Window *s_main_window;
static TextLayer *s_mass_layer, *s_gravity_layer;
static Layer *s_canvas_layer;
static GameState s_state __attribute__((aligned(8))); 

static void update_display();

#if defined(PBL_HEALTH)
static int s_last_step_count = 0;
static void health_handler(HealthEventType event, void *context) {
  if (event == HealthEventSignificantUpdate) {
    s_last_step_count = 0;
  }

  const time_t start = time_start_of_today();
  const time_t end = time(NULL);
  HealthServiceAccessibilityMask mask = health_service_metric_accessible(HealthMetricStepCount, start, end);

  if (mask & HealthServiceAccessibilityMaskAvailable) {
    int total_steps = (int)health_service_sum_today(HealthMetricStepCount);
    if (s_last_step_count == 0) {
      s_last_step_count = total_steps;
    } else {
      int delta = total_steps - s_last_step_count;
      if (delta > 0) {
        game_state_add_steps(&s_state, delta);
      }
      s_last_step_count = total_steps;
    }
  }
}
#endif

static AppTimer *s_tap_timer = NULL;
static int s_hold_time_ms = 0;
static bool s_is_app_exiting = false;
static int s_taps_since_last_tick = 0;
static bool s_app_has_focus = true;

static void focus_handler(bool in_focus) {
  s_app_has_focus = in_focus;
  if (s_app_has_focus) {
    // Catch-up for time spent in background (notifications, etc)
    game_state_apply_offline_gains(&s_state);
    update_display();
  }
}

static void tap_timer_callback(void *data) {
  if (s_is_app_exiting || !s_tap_timer || !s_app_has_focus) return;

  s_hold_time_ms += 100;
  
  // Repeating taps (fires every 200ms AFTER the initial 500ms delay)
  // We check % 200 to achieve 5 taps per second
  if (s_hold_time_ms >= 500 && (s_hold_time_ms % 200 == 0)) {
    s_state.mass += game_state_calculate_tap_strength(&s_state);
    s_taps_since_last_tick++;
  }

  s_tap_timer = app_timer_register(100, tap_timer_callback, NULL);
}

static void select_down_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_is_app_exiting) return;
  s_hold_time_ms = 0;
  if (s_tap_timer) app_timer_cancel(s_tap_timer);
  
  // Initial Tap (Immediate)
  s_state.mass += game_state_calculate_tap_strength(&s_state);
  s_taps_since_last_tick++;

  // Start timer for repeats and prestige hold
  s_tap_timer = app_timer_register(100, tap_timer_callback, NULL);
}

static void select_up_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_tap_timer) {
    app_timer_cancel(s_tap_timer);
    s_tap_timer = NULL;
  }
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  
  // The "Well" where the circle lives: 35 to 143
  int well_y_center = 35 + ((bounds.size.h - 35 - 25) / 2);
  GPoint center = GPoint(bounds.size.w / 2, well_y_center);

  // radius mapped to 10-45px to fit perfectly within the 100px well
  double log_mass = log10(s_state.mass > 1.0 ? s_state.mass : 1.0);
  if (log_mass > 36.0) log_mass = 36.0;
  int radius = 10 + (int)((log_mass / 36.0) * 35.0);

  // Singularity Instability
  bool is_unstable = (s_state.mass >= PRESTIGE_THRESHOLD * 0.9);
  if (is_unstable) {
    center.x += (rand() % 3) - 1;
    center.y += (rand() % 3) - 1;
  }

  int era = game_state_get_era(&s_state);
  GColor era_color;
  
  if (is_unstable) {
    era_color = GColorRed;
  } else {
    static const GColor colors[] = {
      GColorWhite, GColorIslamicGreen, GColorCyan, GColorYellow, GColorVividViolet
    };
    era_color = PBL_IF_COLOR_ELSE(colors[era], GColorWhite);
  }

  graphics_context_set_fill_color(ctx, era_color);
  graphics_fill_circle(ctx, center, radius);

  // Era-specific B&W patterns
  #if defined(PBL_BW)
  if (!is_unstable) {
    if (era == 1) { // Stripes
      for (int i = -radius; i < radius; i += 4) {
        graphics_context_set_stroke_color(ctx, GColorBlack);
        graphics_draw_line(ctx, GPoint(center.x - radius, center.y + i), GPoint(center.x + radius, center.y + i));
      }
    } else if (era == 2) { // Ring
      graphics_context_set_stroke_color(ctx, GColorBlack);
      graphics_draw_circle(ctx, center, radius / 2);
    } else if (era == 3) { // Core
      graphics_context_set_fill_color(ctx, GColorBlack);
      graphics_fill_circle(ctx, center, radius / 2);
    }
  }
  #endif

  graphics_context_set_stroke_width(ctx, 2);
  graphics_context_set_stroke_color(ctx, is_unstable ? GColorWhite : era_color);
  graphics_draw_circle(ctx, center, radius + 2);

  // Purchase Ready Indicator (>)
  // Logic: Can we afford another unit of our highest owned tier?
  int highest_owned = -1;
  for (int i = NUM_TIERS - 1; i >= 0; i--) {
    if (s_state.counts[i] > 0) {
      highest_owned = i;
      break;
    }
  }
  if (highest_owned >= 0) {
    double cost = calculate_cost(TIERS[highest_owned].base_cost, s_state.counts[highest_owned]);
    if (s_state.mass >= cost) {
      graphics_context_set_text_color(ctx, era_color);
      graphics_draw_text(ctx, ">", fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), 
                         GRect(bounds.size.w - 15, 12, 10, 20), 
                         GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    }
  }
}

static void update_display() {
  if (!s_mass_layer || !s_gravity_layer) return;

  static char s_mass_buffer[64];
  static char s_gravity_buffer[64];
  char val_buffer[32];
  
  format_mass(s_state.mass, val_buffer);
  snprintf(s_mass_buffer, sizeof(s_mass_buffer), "%s", val_buffer);
  text_layer_set_text(s_mass_layer, s_mass_buffer);
  
  double current_gravity = game_state_calculate_gravity(&s_state);
  format_mass(current_gravity, val_buffer);
  snprintf(s_gravity_buffer, sizeof(s_gravity_buffer), "G: %s/s", val_buffer);
  text_layer_set_text(s_gravity_layer, s_gravity_buffer);

  if (s_canvas_layer) layer_mark_dirty(s_canvas_layer);
}

static void save_timer_handler(void *data) {
  if (s_is_app_exiting) return;
  game_state_save(&s_state);
  app_timer_register(300000, save_timer_handler, NULL);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (s_taps_since_last_tick > 0) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Taps this second: %d", s_taps_since_last_tick);
    s_taps_since_last_tick = 0;
  }
  s_state.mass += game_state_calculate_gravity(&s_state);
  update_display();

  // Save progress every minute
  if (tick_time->tm_sec == 0) {
    game_state_save(&s_state);
  }
}

static void open_shop_handler(ClickRecognizerRef recognizer, void *context) {
  shop_menu_show(&s_state, update_display);
}

static void up_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  // Debug: Warp forward 1 hour (3600 seconds)
  double gravity = game_state_calculate_gravity(&s_state);
  double gain = gravity * 3600.0;
  
  // Minimal floor for early warp testing
  if (gain < 1000000.0) gain = 1000000.0; 
  
  s_state.mass += gain;
  update_display();
  vibes_short_pulse();
  APP_LOG(APP_LOG_LEVEL_INFO, "Debug: Warped 1 hour forward (+1e6 floor)");
}

static void click_config_provider(void *context) {
  // DOWN (Bottom) handles all tapping
  window_raw_click_subscribe(BUTTON_ID_DOWN, select_down_handler, select_up_handler, NULL);
  
  // SELECT (Center) and UP (Top) open the shop
  window_single_click_subscribe(BUTTON_ID_SELECT, open_shop_handler);
  window_single_click_subscribe(BUTTON_ID_UP, open_shop_handler);
  
  // UP (Top) Long-press for Time Warp
  window_long_click_subscribe(BUTTON_ID_UP, 500, up_long_click_handler, NULL);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  window_set_background_color(window, GColorBlack);

  // partitioning: 
  // Header: 0-35 (Black)
  // Well: 35-143 (Background for circle)
  // Footer: 143-bottom (Black)

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  // Header Bar (Mass)
  s_mass_layer = text_layer_create(GRect(0, 0, bounds.size.w, 35));
  text_layer_set_background_color(s_mass_layer, GColorBlack);
  text_layer_set_text_color(s_mass_layer, GColorWhite);
  text_layer_set_text_alignment(s_mass_layer, GTextAlignmentCenter);
  text_layer_set_font(s_mass_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_mass_layer));

  // Footer Bar (Gravity)
  s_gravity_layer = text_layer_create(GRect(0, bounds.size.h - 25, bounds.size.w, 25));
  text_layer_set_background_color(s_gravity_layer, GColorBlack);
  text_layer_set_text_color(s_gravity_layer, GColorCeleste);
  text_layer_set_text_alignment(s_gravity_layer, GTextAlignmentCenter);
  text_layer_set_font(s_gravity_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_gravity_layer));

  update_display();
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_mass_layer);
  text_layer_destroy(s_gravity_layer);
  layer_destroy(s_canvas_layer);
  s_mass_layer = NULL; s_gravity_layer = NULL; s_canvas_layer = NULL;
}

static void init() {
  s_is_app_exiting = false;
  if (!game_state_load(&s_state)) {
    game_state_init(&s_state);
  } else {
    // Force check for gains immediately after load
    game_state_apply_offline_gains(&s_state);
  }

  s_main_window = window_create();
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load, 
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);
  
  // Global subscriptions
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  app_focus_service_subscribe(focus_handler);
  #if defined(PBL_HEALTH)
  health_service_events_subscribe(health_handler, NULL);
  #endif
  
  app_timer_register(300000, save_timer_handler, NULL);
}

static void deinit() {
  s_is_app_exiting = true;
  tick_timer_service_unsubscribe();
  app_focus_service_unsubscribe();
  #if defined(PBL_HEALTH)
  health_service_events_unsubscribe();
  #endif
  
  game_state_save(&s_state);
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
