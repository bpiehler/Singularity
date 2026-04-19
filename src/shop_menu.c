#include "shop_menu.h"
#include "math_utils.h"
#include <math.h>

static Window *s_shop_window;
static MenuLayer *s_menu_layer;
static GameState *s_game_state;
static ShopPurchaseCallback s_callback;
static char s_prestige_reward_buf[64];

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return NUM_TIERS + 1;
}

static int16_t menu_get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  return 52;
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  int i = cell_index->row;
  if (!s_game_state || i > NUM_TIERS) return;

  static char s_name_buf[64];
  static char s_cost_buf[64];
  static char s_val_buf[32];
  bool affordable = false;

  if (i < NUM_TIERS) {
    double cost = calculate_cost(TIERS[i].base_cost, s_game_state->counts[i]);
    affordable = s_game_state->mass >= cost;
    snprintf(s_name_buf, sizeof(s_name_buf), "%s (x%d)", TIERS[i].name, s_game_state->counts[i]);
    format_mass(cost, s_val_buf);
    snprintf(s_cost_buf, sizeof(s_cost_buf), "Cost: %s", s_val_buf);
  } else {
    affordable = s_game_state->mass >= PRESTIGE_THRESHOLD;
    snprintf(s_name_buf, sizeof(s_name_buf), "%s", affordable ? "THE BIG BANG" : "SINGULARITY");
    if (affordable) {
      snprintf(s_cost_buf, sizeof(s_cost_buf), "%s", s_prestige_reward_buf);
    } else {
      format_mass(PRESTIGE_THRESHOLD, s_val_buf);
      snprintf(s_cost_buf, sizeof(s_cost_buf), "Goal: %s", s_val_buf);
    }
  }

  GRect bounds = layer_get_bounds(cell_layer);
  bool is_highlighted = menu_cell_layer_is_highlighted(cell_layer);
  GColor text_color = affordable ? GColorCeleste : GColorDarkGray;
  if (is_highlighted) text_color = GColorWhite;
  graphics_context_set_text_color(ctx, text_color);
  int lp = PBL_IF_ROUND_ELSE(20, 5);
  graphics_draw_text(ctx, s_name_buf, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), 
                     GRect(lp, 3, bounds.size.w - (lp + 5), 26), 
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, s_cost_buf, fonts_get_system_font(FONT_KEY_GOTHIC_18), 
                     GRect(lp, 27, bounds.size.w - (lp + 5), 20), 
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

static void big_bang_timer_callback(void *data) {
  main_trigger_big_bang();
}

static void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  int i = cell_index->row;
  if (i < NUM_TIERS) {
    double cost = calculate_cost(TIERS[i].base_cost, s_game_state->counts[i]);
    if (s_game_state->mass >= cost) {
      s_game_state->mass -= cost;
      s_game_state->counts[i]++;
      game_state_update_cache(s_game_state);
      vibes_double_pulse();
      menu_layer_reload_data(s_menu_layer);
      if (s_callback) s_callback();
    }
  } else if (s_game_state->mass >= PRESTIGE_THRESHOLD) {
    vibes_long_pulse();
    shop_menu_hide(); 
    app_timer_register(100, big_bang_timer_callback, NULL);
  }
}

static void menu_select_long_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  int i = cell_index->row;
  if (i >= NUM_TIERS) return;
  double m = s_game_state->mass;
  game_state_buy_max(s_game_state, i);
  if (s_game_state->mass < m) {
    vibes_long_pulse();
    menu_layer_reload_data(s_menu_layer);
    if (s_callback) s_callback();
  }
}

static void shop_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  window_set_background_color(window, GColorBlack);
  s_menu_layer = menu_layer_create(bounds);
  if (!s_menu_layer) return;
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_rows = menu_get_num_rows_callback,
    .get_cell_height = menu_get_cell_height_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
    .select_long_click = menu_select_long_callback,
  });
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  int r = 0;
  for (int i = NUM_TIERS - 1; i >= 0; i--) {
    if (s_game_state->mass >= calculate_cost(TIERS[i].base_cost, s_game_state->counts[i])) {
      r = i; break;
    }
  }
  menu_layer_set_selected_index(s_menu_layer, MenuIndex(0, r), MenuRowAlignCenter, false);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void shop_window_unload(Window *window) {
  if (s_menu_layer) { menu_layer_destroy(s_menu_layer); s_menu_layer = NULL; }
  s_shop_window = NULL;
}

void shop_menu_show(GameState *state, ShopPurchaseCallback callback) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Loc: Shop Show Start");
  s_game_state = state;
  s_callback = callback;
  
  if (s_game_state->mass >= PRESTIGE_THRESHOLD) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Loc: Reward Prep");
    static char vbuf[32];
    format_mass(s_game_state->cached_prestige_dust, vbuf);
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Val: Reward Dust");
    APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", vbuf);
    
    snprintf(s_prestige_reward_buf, 64, "Reward: %s Dust", vbuf);
  } else {
    s_prestige_reward_buf[0] = '\0';
  }
  
  if (s_shop_window) {
    window_stack_push(s_shop_window, true);
    return;
  }
  s_shop_window = window_create();
  window_set_window_handlers(s_shop_window, (WindowHandlers) { .load = shop_window_load, .unload = shop_window_unload });
  window_stack_push(s_shop_window, true);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Loc: Shop Show End");
}

void shop_menu_hide() { if (s_shop_window) window_stack_pop(true); }

void shop_menu_deinit() {
  if (s_shop_window) { window_destroy(s_shop_window); s_shop_window = NULL; }
}
