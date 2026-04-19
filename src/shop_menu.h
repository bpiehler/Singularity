#pragma once

#include <pebble.h>
#include "game_state.h"

// Callback type for when a purchase is made
typedef void (*ShopPurchaseCallback)(void);

// Show the shop menu
void shop_menu_show(GameState *state, ShopPurchaseCallback callback);

// Hide the shop menu
void shop_menu_hide();

// Trigger the Big Bang animation on the main screen (implemented in main.c)
void main_trigger_big_bang();

// Final cleanup of shop resources
void shop_menu_deinit();
