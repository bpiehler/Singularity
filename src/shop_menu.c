#include "shop_menu.h"
#include "math_utils.h"
#include <math.h>

static Window *s_shop_window;
static MenuLayer *s_menu_layer;
static GameState *s_game_state;
static ShopPurchaseCallback s_callback;

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return NUM_TIERS + 1;
}

static int16_t menu_get_cell_height_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  return 52;
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  int i = cell_index->row;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Shop: Draw Row Index");
  APP_LOG(APP_LOG_LEVEL_DEBUG, "%d", i);

  if (!s_game_state) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Shop: GameState is NULL");
    return;
  }

  static char s_name_buf[64];
  static char s_cost_buf[64];
  static char s_val_buf[32];
  bool affordable = false;

  if (i < NUM_TIERS) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Shop: Accessing Tier");
    APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", TIERS[i].name);

    double cost = calculate_cost(TIERS[i].base_cost, s_game_state->counts[i]);
    affordable = s_game_state->mass >= cost;
    
    snprintf(s_name_buf, sizeof(s_name_buf), "%s (x%d)", TIERS[i].name, s_game_state->counts[i]);
    format_mass(cost, s_val_buf);
    snprintf(s_cost_buf, sizeof(s_cost_buf), "Cost: %s", s_val_buf);
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Shop: Drawing Big Bang");
    affordable = s_game_state->mass >= PRESTIGE_THRESHOLD;
    snprintf(s_name_buf, sizeof(s_name_buf), "%s", affordable ? "THE BIG BANG" : "SINGULARITY");
    
    if (affordable) {
      double dust = calculate_prestige_dust(s_game_state->mass, PRESTIGE_THRESHOLD);
      int d_int = (int)dust;
      snprintf(s_cost_buf, sizeof(s_cost_buf), "Reward: %d Dust", d_int);
    } else {
      format_mass(PRESTIGE_THRESHOLD, s_val_buf);
      snprintf(s_cost_buf, sizeof(s_cost_buf), "Goal: %s", s_val_buf);
    }
  }

  GRect bounds = layer_get_bounds(cell_layer);
  bool is_highlighted = menu_cell_layer_is_highlighted(cell_layer);
  
  GColor text_color;
  if (is_highlighted) {
    text_color = affordable ? GColorWhite : PBL_IF_COLOR_ELSE(GColorLightGray, GColorWhite);
  } else {
    if (i == NUM_TIERS) {
      text_color = affordable ? GColorVividViolet : GColorDarkGray;
    } else {
      text_color = affordable ? GColorCeleste : GColorDarkGray;
    }
  }
  graphics_context_set_text_color(ctx, text_color);

  #if !defined(PBL_COLOR)
  if (is_highlighted && !affordable) {
    graphics_context_set_compositing_mode(ctx, GCompOpClear);
  }
  #endif

  int left_padding = PBL_IF_ROUND_ELSE(20, 5);
  graphics_draw_text(ctx, s_name_buf, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), 
                     GRect(left_padding, 3, bounds.size.w - (left_padding + 5), 26), 
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  graphics_draw_text(ctx, s_cost_buf, fonts_get_system_font(FONT_KEY_GOTHIC_18), 
                     GRect(left_padding, 27, bounds.size.w - (left_padding + 5), 20), 
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Shop: Done Row");
  APP_LOG(APP_LOG_LEVEL_DEBUG, "%d", i);
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
    } else {
      vibes_short_pulse();
    }
  } else {
    if (s_game_state->mass >= PRESTIGE_THRESHOLD) {
      vibes_long_pulse();
      shop_menu_hide(); 
      app_timer_register(100, big_bang_timer_callback, NULL);
    } else {
      vibes_short_pulse();
    }
  }
}

static void menu_select_long_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  int i = cell_index->row;
  if (i >= NUM_TIERS) return;
  double start_mass = s_game_state->mass;
  game_state_buy_max(s_game_state, i);
  if (s_game_state->mass < start_mass) {
    vibes_long_pulse();
    menu_layer_reload_data(s_menu_layer);
    if (s_callback) s_callback();
  }
}

static void shop_window_load(Window *window) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Shop: Load Start");
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  window_set_background_color(window, GColorBlack);

  s_menu_layer = menu_layer_create(bounds);
  if (!s_menu_layer) return;

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
  
  // Smart Selection Loop
  APP_LOG(APP_LOG_LEVEL_INFO, "Shop: Smart Select Start");
  int initial_row = 0;
  for (int i = NUM_TIERS - 1; i >= 0; i--) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Shop: Checking Tier");
    APP_LOG(APP_LOG_LEVEL_DEBUG, "%d", i);
    
    double cost = calculate_cost(TIERS[i].base_cost, s_game_state->counts[i]);
    if (s_game_state->mass >= cost) {
      initial_row = i;
      break;
    }
  }
  APP_LOG(APP_LOG_LEVEL_INFO, "Shop: Initial Row Chosen");
  APP_LOG(APP_LOG_LEVEL_INFO, "%d", initial_row);

  menu_layer_set_selected_index(s_menu_layer, MenuIndex(0, initial_row), MenuRowAlignCenter, false);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
  APP_LOG(APP_LOG_LEVEL_INFO, "Shop: Load Complete");
}

static void shop_window_unload(Window *window) {
  if (s_menu_layer) {
    menu_layer_destroy(s_menu_layer);
    s_menu_layer = NULL;
  }
  s_shop_window = NULL;
}

void shop_menu_show(GameState *state, ShopPurchaseCallback callback) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Shop: Show Called");
  s_game_state = state;
  s_callback = callback;
  if (s_shop_window) {
    window_stack_push(s_shop_window, true);
    return;
  }
  s_shop_window = window_create();
  window_set_window_handlers(s_shop_window, (WindowHandlers) {
    .load = shop_window_load,
    .unload = shop_window_unload,
  });
  window_stack_push(s_shop_window, true);
}

void shop_menu_hide() {
  if (s_shop_window) window_stack_pop(true);
}

void shop_menu_deinit() {
  if (s_shop_window) {
    window_destroy(s_shop_window);
    s_shop_window = NULL;
  }
}
