#include <pebble.h>
#include "game_state.h"
#include "math_utils.h"
#include "shop_menu.h"

static Window *s_main_window;
static TextLayer *s_mass_layer, *s_gravity_layer;
static Layer *s_canvas_layer;
static GameState s_state;
static double s_next_tier_cost = PRESTIGE_THRESHOLD;
static bool s_is_prestiging = false;
static int s_last_step_count = 0;
static AppTimer *s_tap_timer = NULL;
static int s_hold_time_ms = 0;

static void update_display();
static void update_next_tier_cost();

static void tap_timer_callback(void *data) {
  if (s_tap_timer == NULL) return; 

  // Fire a tap
  s_state.mass += game_state_calculate_tap_strength(&s_state);
  update_display();
  
  s_hold_time_ms += 100;
  
  // If held for 5 seconds and threshold met, trigger Big Bang
  if (s_hold_time_ms >= 5000 && s_state.mass >= PRESTIGE_THRESHOLD) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Big Bang triggered!");
    double earned = game_state_prestige(&s_state);
    if (earned > 0) {
      vibes_double_pulse();
      update_next_tier_cost();
      update_display();
      game_state_save(&s_state);
    }
    s_tap_timer = NULL;
    return; 
  }

  // Continue timer while button is held
  s_tap_timer = app_timer_register(100, tap_timer_callback, NULL);
}

static void health_handler(HealthEventType event, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Health Event: %d", (int)event);
  if (event != HealthEventSleepUpdate) {
    int total_steps = (int)health_service_sum_today(HealthMetricStepCount);
    int delta = total_steps - s_last_step_count;
    
    if (delta > 0) {
      APP_LOG(APP_LOG_LEVEL_INFO, "Steps delta: %d", delta);
      game_state_add_steps(&s_state, delta);
      update_display();
    }
    s_last_step_count = total_steps;
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
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);

  // Use cached cost for scaling to save CPU
  int radius = 10 + (int)((s_state.mass / s_next_tier_cost) * 50);
  if (radius > 70) radius = 70;

  // Cosmic Colors
  GColor fill_color = PBL_IF_COLOR_ELSE(GColorElectricBlue, GColorWhite);
  
  // If close to prestige, core starts to turn Red
  if (s_state.mass >= PRESTIGE_THRESHOLD * 0.9) {
    fill_color = GColorRed;
  }

  graphics_context_set_fill_color(ctx, fill_color);
  graphics_fill_circle(ctx, center, radius);
  
  graphics_context_set_stroke_width(ctx, 3);
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorVividViolet, GColorWhite));
  graphics_draw_circle(ctx, center, radius + 2);
}

static void update_display() {
  static char s_mass_buffer[32];
  static char s_gravity_buffer[32];
  char val_buffer[16];
  
  format_mass(s_state.mass, val_buffer);
  snprintf(s_mass_buffer, sizeof(s_mass_buffer), "%s mg", val_buffer);
  text_layer_set_text(s_mass_layer, s_mass_buffer);
  
  format_mass(game_state_calculate_gravity(&s_state), val_buffer);
  snprintf(s_gravity_buffer, sizeof(s_gravity_buffer), "G: %s/s", val_buffer);
  text_layer_set_text(s_gravity_layer, s_gravity_buffer);

  // If we exceeded the current goal, find the next one for the visual scale
  if (s_state.mass >= s_next_tier_cost) {
    update_next_tier_cost();
  }

  if (s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  s_state.mass += game_state_calculate_gravity(&s_state);
  update_display();
  
  // Save every minute as a backup
  if (tick_time->tm_sec == 0) {
    game_state_save(&s_state);
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Up/Down clicked, opening shop...");
  shop_menu_show(&s_state, update_display);
}

static void select_down_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Select down");
  s_hold_time_ms = 0;
  if (s_tap_timer) app_timer_cancel(s_tap_timer);
  s_tap_timer = app_timer_register(100, tap_timer_callback, NULL);
  
  // Also fire immediate first tap
  s_state.mass += game_state_calculate_tap_strength(&s_state);
  update_display();
}

static void select_up_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Select up");
  if (s_tap_timer) {
    app_timer_cancel(s_tap_timer);
    s_tap_timer = NULL;
  }
}

static void click_config_provider(void *context) {
  window_raw_click_subscribe(BUTTON_ID_SELECT, select_down_handler, select_up_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, up_click_handler);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  window_set_background_color(window, GColorBlack);

  // Canvas for the Center of Mass (bottom layer)
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  // Mass at top
  s_mass_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(40, 30), bounds.size.w, 30));
  text_layer_set_background_color(s_mass_layer, GColorClear);
  text_layer_set_text_color(s_mass_layer, PBL_IF_COLOR_ELSE(GColorCeleste, GColorWhite));
  text_layer_set_text_alignment(s_mass_layer, GTextAlignmentCenter);
  text_layer_set_font(s_mass_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_mass_layer));

  // Gravity at bottom
  s_gravity_layer = text_layer_create(GRect(0, bounds.size.h - PBL_IF_ROUND_ELSE(50, 40), bounds.size.w, 20));
  text_layer_set_background_color(s_gravity_layer, GColorClear);
  text_layer_set_text_color(s_gravity_layer, PBL_IF_COLOR_ELSE(GColorCeleste, GColorWhite));
  text_layer_set_text_alignment(s_gravity_layer, GTextAlignmentCenter);
  text_layer_set_font(s_gravity_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_gravity_layer));

  update_display();
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_mass_layer);
  text_layer_destroy(s_gravity_layer);
  layer_destroy(s_canvas_layer);
  s_canvas_layer = NULL;
}

static void init() {
  APP_LOG(APP_LOG_LEVEL_INFO, "App init start");
  if (!game_state_load(&s_state)) {
    game_state_init(&s_state);
  }
  
  update_next_tier_cost();
  
  // Health subscription
  if (health_service_metric_accessible(HealthMetricStepCount, time(NULL), time(NULL))) {
    s_last_step_count = (int)health_service_sum_today(HealthMetricStepCount);
    health_service_events_subscribe(health_handler, NULL);
  }
  
  double gained = game_state_apply_offline_gains(&s_state);
  if (gained > 0) {
    char buf[64];
    char val_buf[16];
    format_mass(gained, val_buf);
    snprintf(buf, sizeof(buf), "While you were away, you gained %s mg of mass!", val_buf);
    APP_LOG(APP_LOG_LEVEL_INFO, "%s", buf);
  }

  s_main_window = window_create();
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);
  
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void deinit() {
  game_state_save(&s_state);
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
