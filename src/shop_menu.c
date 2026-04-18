#include "shop_menu.h"
#include "math_utils.h"

static Window *s_shop_window;
static MenuLayer *s_menu_layer;
static GameState *s_game_state;
static ShopPurchaseCallback s_callback;

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return NUM_TIERS;
}

static int16_t menu_get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  return 52;
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  int i = cell_index->row;
  if (!s_game_state || i >= NUM_TIERS) return;

  double cost = calculate_cost(TIERS[i].base_cost, s_game_state->counts[i]);
  bool affordable = s_game_state->mass >= cost;
  GRect bounds = layer_get_bounds(cell_layer);
  
  char name_buf[32];
  snprintf(name_buf, sizeof(name_buf), "%s (x%d)", TIERS[i].name, s_game_state->counts[i]);
  
  char cost_buf[32];
  char val_buf[16];
  format_mass(cost, val_buf);
  snprintf(cost_buf, sizeof(cost_buf), "Cost: %s", val_buf);

  bool is_highlighted = menu_cell_layer_is_highlighted(cell_layer);
  
  // Dark Nebula Colors
  GColor text_color;
  if (is_highlighted) {
    text_color = affordable ? GColorWhite : GColorLightGray;
  } else {
    text_color = affordable ? GColorCeleste : GColorDarkGray;
  }
  graphics_context_set_text_color(ctx, text_color);

  int left_padding = PBL_IF_ROUND_ELSE(20, 5);
  // Vertically centered within 52px: (52 - (24 + 18)) / 2 = 5px approx
  graphics_draw_text(ctx, name_buf, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), 
                     GRect(left_padding, 3, bounds.size.w - (left_padding + 5), 26), 
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  graphics_draw_text(ctx, cost_buf, fonts_get_system_font(FONT_KEY_GOTHIC_18), 
                     GRect(left_padding, 27, bounds.size.w - (left_padding + 5), 20), 
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  }

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  int i = cell_index->row;
  double cost = calculate_cost(TIERS[i].base_cost, s_game_state->counts[i]);
  
  if (s_game_state->mass >= cost) {
    s_game_state->mass -= cost;
    s_game_state->counts[i]++;
    vibes_double_pulse();
    menu_layer_reload_data(s_menu_layer);
    if (s_callback) s_callback();
  } else {
    vibes_short_pulse();
  }
}

static void menu_select_long_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  int i = cell_index->row;
  double start_mass = s_game_state->mass;
  game_state_buy_max(s_game_state, i);
  
  if (s_game_state->mass < start_mass) {
    vibes_long_pulse();
    menu_layer_reload_data(s_menu_layer);
    if (s_callback) s_callback();
  }
}

static void shop_window_load(Window *window) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Shop: window_load start");
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  window_set_background_color(window, GColorBlack);

  s_menu_layer = menu_layer_create(bounds);
  if (!s_menu_layer) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Shop: MenuLayer NULL");
    return;
  }

  #if defined(PBL_COLOR)
  menu_layer_set_normal_colors(s_menu_layer, GColorBlack, GColorCeleste);
  menu_layer_set_highlight_colors(s_menu_layer, GColorImperialPurple, GColorWhite);
  #endif

  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_rows = menu_get_num_rows_callback,
    .get_cell_height = menu_get_cell_height_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
    .select_long_click = menu_select_long_callback,
  });

  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
  APP_LOG(APP_LOG_LEVEL_INFO, "Shop: window_load end");
}

static void shop_window_unload(Window *window) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Shop: window_unload start");
  if (s_menu_layer) {
    menu_layer_destroy(s_menu_layer);
    s_menu_layer = NULL;
  }
  APP_LOG(APP_LOG_LEVEL_INFO, "Shop: window_unload end");
}

void shop_menu_show(GameState *state, ShopPurchaseCallback callback) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Shop: show() called");
  s_game_state = state;
  s_callback = callback;
  
  if (s_shop_window) {
    window_destroy(s_shop_window);
  }
  
  s_shop_window = window_create();
  window_set_window_handlers(s_shop_window, (WindowHandlers) {
    .load = shop_window_load,
    .unload = shop_window_unload,
  });
  
  window_stack_push(s_shop_window, true);
}

void shop_menu_hide() {
  if (s_shop_window) {
    window_stack_remove(s_shop_window, true);
    window_destroy(s_shop_window);
    s_shop_window = NULL;
  }
}
