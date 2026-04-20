#include "instructions_view.h"

static Window *s_window;
static ScrollLayer *s_scroll_layer;
static TextLayer *s_text_layer;

static const char *s_instructions_text = 
  "THE MISSION\n"
  "From a lone milligram, coalesce the scattered dust of the void. Amass enough matter to reach a point of infinite density. Once the Singularity is achieved, you may trigger the Big Bang to seed the next universe with permanent Cosmic Dust.\n\n"
  "PROGRESSION\n"
  "• PASSIVE: Gravity draws in matter while you wait.\n"
  "• ACTIVE: Tap DOWN or walk in the physical world to accelerate accumulation. Kinetic energy and movement are far more potent than passive attraction.\n\n"
  "COMMANDS\n"
  "• SELECT: Visit the Cosmic Forge (Shop).\n"
  "• UP: View the Cosmic Ledger (Stats & Help).\n"
  "• DOWN: Tap to add mass; Hold for continuous rapid accumulation.\n"
  "• LONG-SELECT: In the Forge, hold to manifest the maximum number of bodies you can afford.";

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_scroll_layer = scroll_layer_create(bounds);
  scroll_layer_set_click_config_onto_window(s_scroll_layer, window);

  // Calculate maximum height needed for text
  GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_18);
  int padding = 5;
  GSize max_size = graphics_text_layout_get_content_size(
    s_instructions_text, font, GRect(padding, 0, bounds.size.w - (padding * 2), 2000),
    GTextOverflowModeWordWrap, GTextAlignmentLeft
  );

  s_text_layer = text_layer_create(GRect(padding, 0, bounds.size.w - (padding * 2), max_size.h + 20));
  text_layer_set_text(s_text_layer, s_instructions_text);
  text_layer_set_font(s_text_layer, font);
  text_layer_set_background_color(s_text_layer, GColorClear);
  text_layer_set_text_color(s_text_layer, GColorWhite);

  scroll_layer_add_child(s_scroll_layer, text_layer_get_layer(s_text_layer));
  scroll_layer_set_content_size(s_scroll_layer, GSize(bounds.size.w, max_size.h + 20));

  layer_add_child(window_layer, scroll_layer_get_layer(s_scroll_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
  scroll_layer_destroy(s_scroll_layer);
}

void instructions_view_show() {
  if (!s_window) {
    s_window = window_create();
    window_set_background_color(s_window, GColorBlack);
    window_set_window_handlers(s_window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload,
    });
  }
  window_stack_push(s_window, true);
}

void instructions_view_deinit() {
  if (s_window) {
    window_stack_remove(s_window, false);
    window_destroy(s_window);
    s_window = NULL;
  }
}
