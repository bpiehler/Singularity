#pragma once

#include <pebble.h>
#include "game_state.h"

// Show the stats ledger
void stats_menu_show(GameState *state);

// Hide the stats ledger
void stats_menu_hide();

// Final cleanup of stats resources
void stats_menu_deinit();
