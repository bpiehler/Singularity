#include "shop_menu.h"
#include "math_utils.h"

static Window *s_shop_window;
static MenuLayer *s_menu_layer;
static GameState *s_game_state;
static ShopPurchaseCallback s_callback;

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return NUM_TIERS;
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  int i = cell_index->row;
  double cost = calculate_cost(TIERS[i].base_cost, s_game_state->counts[i]);
  
  char name_buf[32];
  snprintf(name_buf, sizeof(name_buf), "%s (x%d)", TIERS[i].name, s_game_state->counts[i]);
  
  char cost_buf[32];
  char val_buf[16];
  format_mass(cost, val_buf);
  snprintf(cost_buf, sizeof(cost_buf), "Cost: %s", val_buf);
  
  menu_cell_basic_draw(ctx, cell_layer, name_buf, cost_buf, NULL);
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  int i = cell_index->row;
  double cost = calculate_cost(TIERS[i].base_cost, s_game_state->counts[i]);
  
  if (s_game_state->mass >= cost) {
    s_game_state->mass -= cost;
    s_game_state->counts[i]++;
    
    // Haptic feedback for purchase
    vibes_double_pulse();
    
    menu_layer_reload_data(s_menu_layer);
    if (s_callback) s_callback();
  }
}

static void shop_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_menu_layer = menu_layer_create(bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_rows = menu_get_num_rows_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });

  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void shop_window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  s_shop_window = NULL;
}

void shop_menu_show(GameState *state, ShopPurchaseCallback callback) {
  s_game_state = state;
  s_callback = callback;
  
  if (!s_shop_window) {
    s_shop_window = window_create();
    window_set_window_handlers(s_shop_window, (WindowHandlers) {
      .load = shop_window_load,
      .unload = shop_window_unload,
    });
  }
  window_stack_push(s_shop_window, true);
}

void shop_menu_hide() {
  if (s_shop_window) {
    window_stack_remove(s_shop_window, true);
  }
}
