#pragma once

#include <pebble.h>

#define NUM_TIERS 9
#define OFFLINE_CAP_SECONDS 172800 // 48 hours
#define PRESTIGE_THRESHOLD 1e36

typedef struct {
  char *name;
  double base_cost;
  double yield;
} TierInfo;

extern const TierInfo TIERS[NUM_TIERS];

#define STORAGE_KEY_GAME_STATE 100
#define STORAGE_VERSION 1

// CRITICAL: Doubles must be at the top for 8-byte alignment on ARM hardware.
typedef struct __attribute__((aligned(8))) {
  double mass;
  double dust;
  double cached_gravity;
  double cached_tap_strength;
  int counts[NUM_TIERS];
  uint32_t version;
  time_t last_update;
} GameState;

typedef void (*ShopPurchaseCallback)(void);

// Initialize a new game state
void game_state_init(GameState *state);

// Calculate and cache gravity/tap strength
void game_state_update_cache(GameState *state);

// Get current gravity (cached)
double game_state_calculate_gravity(GameState *state);

// Get current tap strength (cached)
double game_state_calculate_tap_strength(GameState *state);

// Save state to persistent storage
void game_state_save(GameState *state);

// Load state from persistent storage, returns true if successful
bool game_state_load(GameState *state);

// Calculate and apply offline gains, returns mass gained
double game_state_apply_offline_gains(GameState *state);

// Buy max possible units of a tier
void game_state_buy_max(GameState *state, int tier_index);

// Trigger a prestige reset, returns amount of dust earned
double game_state_prestige(GameState *state);

// Add mass based on steps taken
void game_state_add_steps(GameState *state, int steps);

// Get the current era (0-4) based on highest tier owned
int game_state_get_era(GameState *state);
