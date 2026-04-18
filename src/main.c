#include <pebble.h>
#include "game_state.h"
#include "math_utils.h"
#include "shop_menu.h"

static Window *s_main_window;
static TextLayer *s_mass_layer, *s_gravity_layer;
static GameState s_state;

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
  shop_menu_show(&s_state, update_display);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, up_click_handler);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

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
}

static void init() {
  if (!game_state_load(&s_state)) {
    game_state_init(&s_state);
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
