#pragma once

#include <pebble.h>

#define NUM_TIERS 9
#define OFFLINE_CAP_SECONDS 172800 // 48 hours
#define PRESTIGE_THRESHOLD 1e16

typedef struct {
  char *name;
  double base_cost;
  double yield;
} TierInfo;

typedef struct {
  double mass;
  double dust;
  int counts[NUM_TIERS];
  time_t last_update;
} GameState;

// Tier static data
extern const TierInfo TIERS[NUM_TIERS];

#define STORAGE_KEY_GAME_STATE 100

// Initialize state
void game_state_init(GameState *state);

// Save state to persistent storage
void game_state_save(GameState *state);

// Load state from persistent storage, returns true if data was found
bool game_state_load(GameState *state);

// Calculate total gravity per second
double game_state_calculate_gravity(GameState *state);

// Calculate tap strength
double game_state_calculate_tap_strength(GameState *state);

// Apply offline gains, returns mass gained
double game_state_apply_offline_gains(GameState *state);

// Buy as many units of a tier as possible
void game_state_buy_max(GameState *state, int tier_index);
