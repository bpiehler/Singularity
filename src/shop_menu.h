#pragma once

#include <pebble.h>
#include "game_state.h"

// Callback type for when a purchase is made
typedef void (*ShopPurchaseCallback)(void);

// Show the shop menu
void shop_menu_show(GameState *state, ShopPurchaseCallback callback);

// Hide the shop menu
void shop_menu_hide();

// Refresh the visual core's target cost (implemented in main.c)
void update_next_tier_cost();
