#include "game_state.h"
#include "math_utils.h"
#include <math.h>

const TierInfo TIERS[NUM_TIERS] = {
  {"Pebble", 100.0, 1.0},
  {"Rock", 2500.0, 20.0},
  {"Boulder", 50000.0, 500.0},
  {"Hill", 1000000.0, 12000.0},
  {"Mountain", 75000000.0, 500000.0},
  {"Planet", 5000000000.0, 20000000.0},
  {"Solar System", 500000000000.0, 1000000000.0},
  {"Galaxy", 100000000000000.0, 100000000000.0},
  {"Universe", 5000000000000000.0, 5000000000000.0}
};

void game_state_update_cache(GameState *state) {
  double g = 0;
  for (int i = 0; i < NUM_TIERS; i++) {
    double tier_yield = TIERS[i].yield * calculate_milestone_multiplier(state->counts[i]);
    g += tier_yield * state->counts[i];
  }
  
  // Base starting gravity from Dust: 0.1/s per dust
  g += (state->dust * 0.1);
  
  // Apply Cosmic Dust bonus multiplier (10% per dust)
  state->cached_gravity = g * (1.0 + (state->dust * 0.1));
  
  // Tap is 1mg + 5% of gravity
  state->cached_tap_strength = 1.0 + (state->cached_gravity * 0.05);
}

void game_state_init(GameState *state) {
  state->version = STORAGE_VERSION;
  state->mass = 1.0;
  state->dust = 0.0;
  for (int i = 0; i < NUM_TIERS; i++) {
    state->counts[i] = 0;
  }
  state->last_update = time(NULL);
  game_state_update_cache(state);
}

double game_state_calculate_gravity(GameState *state) {
  return state->cached_gravity;
}

double game_state_calculate_tap_strength(GameState *state) {
  return state->cached_tap_strength;
}

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
    APP_LOG(APP_LOG_LEVEL_WARNING, "Outdated save version");
  }
  return false;
}

double game_state_apply_offline_gains(GameState *state) {
  if (state->last_update == 0) {
    state->last_update = time(NULL);
    return 0;
  }
  
  time_t now = time(NULL);
  double seconds_diff = (double)(now - state->last_update);
  double gained = 0;

  if (seconds_diff > 10.0) {
    if (seconds_diff > OFFLINE_CAP_SECONDS) {
      seconds_diff = OFFLINE_CAP_SECONDS;
    }
    
    double gravity = game_state_calculate_gravity(state);
    gained = gravity * seconds_diff;
    state->mass += gained;
  }
  
  state->last_update = now;
  return gained;
}

void game_state_buy_max(GameState *state, int i) {
  double base = TIERS[i].base_cost;
  double current_unit_cost = calculate_cost(base, state->counts[i]);
  if (state->mass < current_unit_cost) return;

  // Closed-form geometric series solve for k units:
  // k = floor(log(1 + mass * 0.15 / current_unit_cost) / log(1.15))
  // We use 0.15 as the rate (r-1) for our 1.15x scaling
  int k = (int)(log(1.0 + state->mass * 0.15 / current_unit_cost) / log(1.15));
  if (k < 1) k = 1;

  // Total cost for k units: Cost = C * (1.15^k - 1) / 0.15
  double total_cost = current_unit_cost * (pow(1.15, k) - 1.0) / 0.15;

  // Adjust for floating point rounding errors to ensure we don't overspend
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
  double earned_dust = calculate_prestige_dust(state->mass, PRESTIGE_THRESHOLD);
  if (earned_dust < 1.0) return 0;
  
  state->dust += earned_dust;
  state->mass = 1.0;
  for (int i = 0; i < NUM_TIERS; i++) {
    state->counts[i] = 0;
  }
  state->last_update = time(NULL);
  game_state_update_cache(state);
  
  return earned_dust;
}

void game_state_add_steps(GameState *state, int steps) {
  if (steps <= 0) return;
  double tap_strength = game_state_calculate_tap_strength(state);
  state->mass += (tap_strength * steps);
}
