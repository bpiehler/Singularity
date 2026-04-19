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
  double cached_prestige_dust;
  int counts[NUM_TIERS];
  uint32_t version;
  time_t last_update;
} GameState;

typedef void (*ShopPurchaseCallback)(void);

void game_state_init(GameState *state);
void game_state_update_cache(GameState *state);
double game_state_calculate_gravity(GameState *state);
double game_state_calculate_tap_strength(GameState *state);
void game_state_save(GameState *state);
bool game_state_load(GameState *state);
double game_state_apply_offline_gains(GameState *state);
void game_state_buy_max(GameState *state, int tier_index);
double game_state_prestige(GameState *state);
void game_state_add_steps(GameState *state, int steps);
int game_state_get_era(GameState *state);
void main_trigger_big_bang();
