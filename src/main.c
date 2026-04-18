#include <pebble.h>
#include "game_state.h"
#include "math_utils.h"

static Window *s_main_window;
static TextLayer *s_mass_layer;
static GameState s_state;

static void update_mass_display() {
  static char s_mass_buffer[32];
  char mass_val_buffer[16];
  format_mass(s_state.mass, mass_val_buffer);
  snprintf(s_mass_buffer, sizeof(s_mass_buffer), "%s mg", mass_val_buffer);
  text_layer_set_text(s_mass_layer, s_mass_buffer);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  s_state.mass += game_state_calculate_tap_strength(&s_state);
  update_mass_display();
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_mass_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));
  text_layer_set_background_color(s_mass_layer, GColorClear);
  text_layer_set_text_color(s_mass_layer, GColorBlack);
  text_layer_set_text_alignment(s_mass_layer, GTextAlignmentCenter);
  text_layer_set_font(s_mass_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  layer_add_child(window_layer, text_layer_get_layer(s_mass_layer));

  update_mass_display();
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_mass_layer);
}

static void init() {
  game_state_init(&s_state);
  
  s_main_window = window_create();
  window_set_click_config_provider(s_main_window, click_config_provider);
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
