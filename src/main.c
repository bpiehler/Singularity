#include <pebble.h>
#include "game_state.h"
#include "math_utils.h"
#include "shop_menu.h"

static Window *s_main_window;
static TextLayer *s_mass_layer, *s_gravity_layer;
static Layer *s_canvas_layer;
static GameState *s_state; // Moved to HEAP for alignment safety
static double s_next_tier_cost = PRESTIGE_THRESHOLD;

static void update_display();
static void update_next_tier_cost();

#if defined(PBL_HEALTH)
static int s_last_step_count = 0;
static void health_handler(HealthEventType event, void *context) {
  // Temporarily disabled for stability testing
}
#endif

static AppTimer *s_tap_timer = NULL;
static int s_hold_time_ms = 0;

static void tap_timer_callback(void *data) {
  if (!s_state) return;
  s_state->mass += game_state_calculate_tap_strength(s_state);
  s_hold_time_ms += 200;
  
  if (s_hold_time_ms >= 5000 && s_state->mass >= PRESTIGE_THRESHOLD) {
    game_state_prestige(s_state);
    update_next_tier_cost();
    update_display();
    vibes_double_pulse();
    s_tap_timer = NULL;
    return; 
  }
  s_tap_timer = app_timer_register(200, tap_timer_callback, NULL);
}

static void select_down_handler(ClickRecognizerRef recognizer, void *context) {
  if (!s_state) return;
  s_hold_time_ms = 0;
  if (s_tap_timer) app_timer_cancel(s_tap_timer);
  s_tap_timer = app_timer_register(200, tap_timer_callback, NULL);
  s_state->mass += game_state_calculate_tap_strength(s_state);
}

static void select_up_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_tap_timer) {
    app_timer_cancel(s_tap_timer);
    s_tap_timer = NULL;
  }
}

static void update_next_tier_cost() {
  if (!s_state) return;
  s_next_tier_cost = PRESTIGE_THRESHOLD;
  for (int i = 0; i < NUM_TIERS; i++) {
    double cost = calculate_cost(TIERS[i].base_cost, s_state->counts[i]);
    if (s_state->mass < cost) {
      s_next_tier_cost = cost;
      break;
    }
  }
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  if (!s_state) return;
  GRect bounds = layer_get_bounds(layer);
  GPoint center = grect_center_point(&bounds);

  double goal = (s_next_tier_cost > 0) ? s_next_tier_cost : PRESTIGE_THRESHOLD;
  double ratio = s_state->mass / goal;
  if (!(ratio >= 0.0 && ratio <= 1.0)) ratio = 0.0;

  int radius = 10 + (int)(ratio * 50);

  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorElectricBlue, GColorWhite));
  graphics_fill_circle(ctx, center, radius);
  graphics_context_set_stroke_width(ctx, 3);
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorVividViolet, GColorWhite));
  graphics_draw_circle(ctx, center, radius + 2);
}

static void update_display() {
  if (!s_state || !s_mass_layer || !s_gravity_layer) return;

  static char s_mass_buffer[64];
  static char s_gravity_buffer[64];
  char val_buffer[32];
  
  format_mass(s_state->mass, val_buffer);
  snprintf(s_mass_buffer, sizeof(s_mass_buffer), "%s mg", val_buffer);
  text_layer_set_text(s_mass_layer, s_mass_buffer);
  
  double current_gravity = game_state_calculate_gravity(s_state);
  format_mass(current_gravity, val_buffer);
  snprintf(s_gravity_buffer, sizeof(s_gravity_buffer), "G: %s/s", val_buffer);
  text_layer_set_text(s_gravity_layer, s_gravity_buffer);

  if (s_state->mass >= s_next_tier_cost) update_next_tier_cost();
  if (s_canvas_layer) layer_mark_dirty(s_canvas_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  if (!s_state) return;
  if (tick_time->tm_sec % 10 == 0) {
    APP_LOG(APP_LOG_LEVEL_INFO, "App Heartbeat (Mass=%d)", (int)s_state->mass);
  }

  s_state->mass += game_state_calculate_gravity(s_state);
  update_display();
  
  if (tick_time->tm_sec == 0) {
    game_state_save(s_state);
  }
}

static void open_shop_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_state) shop_menu_show(s_state, update_display);
}

static void click_config_provider(void *context) {
  window_raw_click_subscribe(BUTTON_ID_SELECT, select_down_handler, select_up_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_UP, open_shop_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, open_shop_handler);
}

static void main_window_appear(Window *window) {
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
}

static void main_window_disappear(Window *window) {
  tick_timer_service_unsubscribe();
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  window_set_background_color(window, GColorBlack);

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  s_mass_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(40, 30), bounds.size.w, 30));
  text_layer_set_background_color(s_mass_layer, GColorClear);
  text_layer_set_text_color(s_mass_layer, PBL_IF_COLOR_ELSE(GColorCeleste, GColorWhite));
  text_layer_set_text_alignment(s_mass_layer, GTextAlignmentCenter);
  text_layer_set_font(s_mass_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_mass_layer));

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
  s_mass_layer = NULL; s_gravity_layer = NULL; s_canvas_layer = NULL;
}

static void init() {
  s_state = malloc(sizeof(GameState));
  if (!s_state) return;

  if (!game_state_load(s_state)) {
    game_state_init(s_state);
  }

  update_next_tier_cost();

  s_main_window = window_create();
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load, 
    .unload = main_window_unload,
    .appear = main_window_appear,
    .disappear = main_window_disappear
  });
  window_stack_push(s_main_window, true);
}

static void deinit() {
  if (s_state) {
    game_state_save(s_state);
    free(s_state);
    s_state = NULL;
  }
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
