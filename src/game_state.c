#include "game_state.h"
#include "math_utils.h"
#include <math.h>

const TierInfo TIERS[NUM_TIERS] = {
  {"Pebble", 100.0, 1.0},
  {"Rock", 1000000.0, 10000.0},
  {"Boulder", 1.0e10, 1.0e8},
  {"Mountain", 1.0e14, 1.0e12},
  {"Asteroid", 1.0e18, 1.0e16},
  {"Moon", 1.0e22, 1.0e20},
  {"Planet", 1.0e26, 1.0e24},
  {"Gas Giant", 1.0e30, 1.0e28},
  {"Star", 1.0e34, 1.0e32}
};

void game_state_update_cache(GameState *state) {
  double raw_g = 0;
  double highest_base_yield = 1.0;
  for (int i = 0; i < NUM_TIERS; i++) {
    double tier_yield = TIERS[i].yield * calculate_milestone_multiplier(state->counts[i]);
    raw_g += tier_yield * (double)state->counts[i];
    if (state->counts[i] > 0) highest_base_yield = TIERS[i].yield;
  }
  state->cached_gravity = raw_g * (1.0 + (state->dust * 0.1));
  state->cached_tap_strength = (highest_base_yield * 10.0) + (state->cached_gravity * 0.25);
  if (state->cached_tap_strength < 1.0) state->cached_tap_strength = 1.0;
  
  // Cache prestige reward using custom safe math
  state->cached_prestige_dust = calculate_prestige_dust(state->mass, PRESTIGE_THRESHOLD);
}

void game_state_init(GameState *state) {
  state->version = STORAGE_VERSION;
  state->mass = 1.0;
  state->dust = 0.0;
  for (int i = 0; i < NUM_TIERS; i++) state->counts[i] = 0;
  state->last_update = time(NULL);
  game_state_update_cache(state);
}

double game_state_calculate_gravity(GameState *state) { return state->cached_gravity; }
double game_state_calculate_tap_strength(GameState *state) { return state->cached_tap_strength; }

void game_state_save(GameState *state) {
  state->version = STORAGE_VERSION;
  state->last_update = time(NULL);
  persist_write_data(STORAGE_KEY_GAME_STATE, state, sizeof(GameState));
}

bool game_state_load(GameState *state) {
  if (persist_exists(STORAGE_KEY_GAME_STATE)) {
    persist_read_data(STORAGE_KEY_GAME_STATE, state, sizeof(GameState));
    if (state->version == STORAGE_VERSION) {
      game_state_update_cache(state);
      return true;
    }
  }
  return false;
}

double game_state_apply_offline_gains(GameState *state) {
  if (state->last_update == 0) { state->last_update = time(NULL); return 0; }
  time_t now = time(NULL);
  double seconds_diff = (double)(now - state->last_update);
  double gained = 0;
  if (seconds_diff > 10.0) {
    if (seconds_diff > OFFLINE_CAP_SECONDS) seconds_diff = OFFLINE_CAP_SECONDS;
    gained = state->cached_gravity * seconds_diff;
    state->mass += gained;
    game_state_update_cache(state);
  }
  state->last_update = now;
  return gained;
}

void game_state_buy_max(GameState *state, int i) {
  double base = TIERS[i].base_cost;
  double current_unit_cost = calculate_cost(base, state->counts[i]);
  if (state->mass < current_unit_cost) return;
  int k = (int)(log(1.0 + state->mass * 0.15 / current_unit_cost) / log(1.15));
  if (k < 1) k = 1;
  double total_cost = current_unit_cost * (pow(1.15, k) - 1.0) / 0.15;
  while (total_cost > state->mass && k > 0) {
    k--;
    total_cost = current_unit_cost * (pow(1.15, k) - 1.0) / 0.15;
  }
  if (k > 0 && state->mass >= total_cost) {
    state->mass -= total_cost;
    state->counts[i] += k;
    game_state_update_cache(state);
  }
}

double game_state_prestige(GameState *state) {
  double earned = calculate_prestige_dust(state->mass, PRESTIGE_THRESHOLD);
  if (earned < 1.0) return 0;
  state->dust += earned;
  state->mass = 1.0;
  for (int i = 0; i < NUM_TIERS; i++) state->counts[i] = 0;
  state->last_update = time(NULL);
  game_state_update_cache(state);
  return earned;
}

void game_state_add_steps(GameState *state, int steps) {
  if (steps <= 0) return;
  double gain = (state->cached_gravity > 0) ? (state->cached_gravity * (double)steps) : (100.0 * (double)steps);
  state->mass += gain;
  game_state_update_cache(state);
}

int game_state_get_era(GameState *state) {
  int highest = -1;
  for (int i = NUM_TIERS - 1; i >= 0; i--) if (state->counts[i] > 0) { highest = i; break; }
  if (highest <= 2) return 0;
  if (highest <= 4) return 1;
  if (highest <= 6) return 2;
  if (highest == 7) return 3;
  return 4;
}
