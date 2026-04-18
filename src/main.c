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

static void update_display();
static void update_next_tier_cost();

static void health_handler(HealthEventType event, void *context) {
  if (event != HealthEventSleepUpdate) {
    int total_steps = (int)health_service_sum_today(HealthMetricStepCount);
    int delta = total_steps - s_last_step_count;
    
    if (delta > 0) {
      game_state_add_steps(&s_state, delta);
      update_display();
    }
    s_last_step_count = total_steps;
  }
}

static void prestige_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_state.mass < PRESTIGE_THRESHOLD) return;
  
  s_is_prestiging = true;
  double earned = game_state_prestige(&s_state);
  if (earned > 0) {
    // Big Bang Effect: Success haptics
    vibes_double_pulse();
    update_next_tier_cost();
    update_display();
    game_state_save(&s_state);
  }
  s_is_prestiging = false;
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

  graphics_context_set_fill_color(ctx, GColorLightGray);
  graphics_fill_circle(ctx, center, radius);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_circle(ctx, center, radius);
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

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  s_state.mass += game_state_calculate_tap_strength(&s_state);
  update_display();
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Up/Down clicked, opening shop...");
  shop_menu_show(&s_state, update_display);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, up_click_handler);
  
  // Big Bang Prestige (5 second hold)
  window_long_click_subscribe(BUTTON_ID_SELECT, 5000, prestige_long_click_handler, NULL);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Canvas for the Center of Mass (bottom layer)
  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  // Mass at top
  s_mass_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(40, 30), bounds.size.w, 30));
  text_layer_set_background_color(s_mass_layer, GColorClear);
  text_layer_set_text_color(s_mass_layer, GColorBlack);
  text_layer_set_text_alignment(s_mass_layer, GTextAlignmentCenter);
  text_layer_set_font(s_mass_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_mass_layer));

  // Gravity at bottom
  s_gravity_layer = text_layer_create(GRect(0, bounds.size.h - PBL_IF_ROUND_ELSE(50, 40), bounds.size.w, 20));
  text_layer_set_background_color(s_gravity_layer, GColorClear);
  text_layer_set_text_color(s_gravity_layer, GColorBlack);
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
