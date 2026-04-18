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

// Initialize state
void game_state_init(GameState *state);

// Calculate total gravity per second
double game_state_calculate_gravity(GameState *state);

// Calculate tap strength
double game_state_calculate_tap_strength(GameState *state);

// Apply offline gains
void game_state_apply_offline_gains(GameState *state);
