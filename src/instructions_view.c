#include "instructions_view.h"

static Window *s_window;
static ScrollLayer *s_scroll_layer;
static Layer *s_content_layer;

typedef struct {
  char *title;
  char *body;
  GColor color;
} InstructionSection;

static InstructionSection s_sections[] = {
  {
    "THE MISSION", 
    "From a lone milligram, coalesce the scattered dust of the void. Amass enough matter to reach a point of infinite density. Once the Singularity is achieved, you may trigger the Big Bang to seed the next universe with permanent Cosmic Dust.",
    GColorIslamicGreen
  },
  {
    "PROGRESSION",
    "• PASSIVE: Gravity draws in matter while you wait.\n• ACTIVE: Tap DOWN or walk in the physical world to accelerate accumulation. Kinetic energy and movement are far more potent than passive attraction.",
    GColorCyan
  },
  {
    "COMMANDS",
    "• SELECT: Visit the Cosmic Forge (Shop).\n• UP: View the Cosmic Ledger (Stats & Help).\n• DOWN: Tap to add mass; Hold for continuous rapid accumulation.\n• LONG-SELECT: In the Forge, hold to manifest the maximum number of bodies you can afford.",
    GColorYellow
  }
};

#define NUM_SECTIONS 3

static void content_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GFont font_title = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  GFont font_body = fonts_get_system_font(FONT_KEY_GOTHIC_24);
  
  int padding_h = PBL_IF_ROUND_ELSE(28, 5);
  int padding_v = PBL_IF_ROUND_ELSE(40, 10);
  int cur_y = padding_v;
  int width = bounds.size.w - (padding_h * 2);

  for (int i = 0; i < NUM_SECTIONS; i++) {
    // Draw Title
    graphics_context_set_text_color(ctx, PBL_IF_COLOR_ELSE(s_sections[i].color, GColorWhite));
    GRect title_rect = GRect(padding_h, cur_y, width, 30);
    graphics_draw_text(ctx, s_sections[i].title, font_title, title_rect, 
                       GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    cur_y += 28;

    // Draw Body
    graphics_context_set_text_color(ctx, GColorWhite);
    GSize body_size = graphics_text_layout_get_content_size(
      s_sections[i].body, font_body, GRect(padding_h, 0, width, 1000),
      GTextOverflowModeWordWrap, GTextAlignmentLeft
    );
    GRect body_rect = GRect(padding_h, cur_y, width, body_size.h);
    graphics_draw_text(ctx, s_sections[i].body, font_body, body_rect,
                       GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
    
    cur_y += body_size.h + 15;
  }
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_scroll_layer = scroll_layer_create(bounds);
  scroll_layer_set_click_config_onto_window(s_scroll_layer, window);

  // Calculate total height
  GFont font_body = fonts_get_system_font(FONT_KEY_GOTHIC_24);
  int padding_h = PBL_IF_ROUND_ELSE(28, 5);
  int padding_v = PBL_IF_ROUND_ELSE(40, 10);
  int total_h = padding_v;
  int width = bounds.size.w - (padding_h * 2);

  for (int i = 0; i < NUM_SECTIONS; i++) {
    total_h += 28; // Title
    GSize body_size = graphics_text_layout_get_content_size(
      s_sections[i].body, font_body, GRect(padding_h, 0, width, 1000),
      GTextOverflowModeWordWrap, GTextAlignmentLeft
    );
    total_h += body_size.h + 15;
  }
  total_h += padding_v; // Bottom padding

  s_content_layer = layer_create(GRect(0, 0, bounds.size.w, total_h));
  layer_set_update_proc(s_content_layer, content_update_proc);

  scroll_layer_add_child(s_scroll_layer, s_content_layer);
  scroll_layer_set_content_size(s_scroll_layer, GSize(bounds.size.w, total_h));

  layer_add_child(window_layer, scroll_layer_get_layer(s_scroll_layer));
}

static void window_unload(Window *window) {
  layer_destroy(s_content_layer);
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
