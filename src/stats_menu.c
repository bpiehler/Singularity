#include "stats_menu.h"
#include "math_utils.h"
#include "instructions_view.h"

static Window *s_stats_window;
static MenuLayer *s_menu_layer;
static GameState *s_game_state;

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return 5; // Dust, Big Bangs, Playtime, Peak Mass, Instructions
}

static int16_t menu_get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  return 52;
}

static void format_playtime(uint32_t total_seconds, char *buffer, size_t size) {
  uint32_t days = total_seconds / 86400;
  uint32_t hours = (total_seconds % 86400) / 3600;
  uint32_t minutes = (total_seconds % 3600) / 60;
  
  if (days > 0) {
    snprintf(buffer, size, "%dd %dh %dm", (int)days, (int)hours, (int)minutes);
  } else if (hours > 0) {
    snprintf(buffer, size, "%dh %dm", (int)hours, (int)minutes);
  } else {
    snprintf(buffer, size, "%dm", (int)minutes);
  }
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  if (!s_game_state) return;
  int i = cell_index->row;

  static char s_title_buf[64];
  static char s_subtitle_buf[64];
  static char s_val_buf[32];

  switch (i) {
    case 0: // Cosmic Dust
      format_mass(s_game_state->dust, s_val_buf);
      snprintf(s_title_buf, sizeof(s_title_buf), "%s Dust", s_val_buf);
      snprintf(s_subtitle_buf, sizeof(s_subtitle_buf), "Bonus: +%d%% Gravity", (int)(s_game_state->dust * 10.0));
      break;
    case 1: // Singularities
      snprintf(s_title_buf, sizeof(s_title_buf), "Big Bangs: %d", s_game_state->total_singularities);
      snprintf(s_subtitle_buf, sizeof(s_subtitle_buf), "Total Universes Created");
      break;
    case 2: // Playtime
      format_playtime(s_game_state->total_playtime_seconds, s_title_buf, sizeof(s_title_buf));
      snprintf(s_subtitle_buf, sizeof(s_subtitle_buf), "Total Cosmic Playtime");
      break;
    case 3: // Peak Mass
      format_mass(s_game_state->highest_mass_ever, s_val_buf);
      snprintf(s_title_buf, sizeof(s_title_buf), "Peak Mass");
      snprintf(s_subtitle_buf, sizeof(s_subtitle_buf), "%s", s_val_buf);
      break;
    case 4: // Instructions
      snprintf(s_title_buf, sizeof(s_title_buf), "How to Play");
      snprintf(s_subtitle_buf, sizeof(s_subtitle_buf), "Help & Instructions");
      break;
  }

  GRect bounds = layer_get_bounds(cell_layer);
  graphics_context_set_text_color(ctx, GColorCeleste);
  
  int lp = PBL_IF_ROUND_ELSE(20, 5);
  graphics_draw_text(ctx, s_title_buf, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), 
                     GRect(lp, 3, bounds.size.w - (lp + 5), 26), 
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  graphics_context_set_text_color(ctx, GColorLightGray);
  graphics_draw_text(ctx, s_subtitle_buf, fonts_get_system_font(FONT_KEY_GOTHIC_18), 
                     GRect(lp, 27, bounds.size.w - (lp + 5), 20), 
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  if (cell_index->row == 4) {
    instructions_view_show();
  }
}

static void stats_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  window_set_background_color(window, GColorBlack);

  s_menu_layer = menu_layer_create(bounds);
  if (!s_menu_layer) return;

  #if defined(PBL_COLOR)
  menu_layer_set_normal_colors(s_menu_layer, GColorBlack, GColorCeleste);
  menu_layer_set_highlight_colors(s_menu_layer, GColorDarkGray, GColorWhite);
  #endif

  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_rows = menu_get_num_rows_callback,
    .get_cell_height = menu_get_cell_height_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });

  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void stats_window_unload(Window *window) {
  if (s_menu_layer) {
    menu_layer_destroy(s_menu_layer);
    s_menu_layer = NULL;
  }
  
  // High Priority Memory Refactor: Destroy window on unload to free heap
  window_destroy(s_stats_window);
  s_stats_window = NULL;
}

void stats_menu_show(GameState *state) {
  s_game_state = state;
  if (!s_stats_window) {
    s_stats_window = window_create();
    window_set_window_handlers(s_stats_window, (WindowHandlers) {
      .load = stats_window_load,
      .unload = stats_window_unload,
    });
  }
  window_stack_push(s_stats_window, true);
}

void stats_menu_hide() {
  if (s_stats_window) {
    window_stack_pop(true);
  }
}

void stats_menu_deinit() {
  instructions_view_deinit();
  if (s_stats_window) {
    window_stack_remove(s_stats_window, false);
    window_destroy(s_stats_window);
    s_stats_window = NULL;
  }
}
