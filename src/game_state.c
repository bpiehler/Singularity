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
    raw_g += tier_yield * state->counts[i];
    if (state->counts[i] > 0) {
      highest_base_yield = TIERS[i].yield;
    }
  }
  
  // Apply multiplicative dust bonus (+10% per dust)
  state->cached_gravity = raw_g * (1.0 + (state->dust * 0.1));
  
  // Tap Strength = (Highest Tier Base Yield * 10) + (Gravity * 0.25)
  // This ensures taps always feel like a significant boost over passive income.
  state->cached_tap_strength = (highest_base_yield * 10.0) + (state->cached_gravity * 0.25);
  
  // Global floor to ensure fresh start isn't broken
  if (state->cached_tap_strength < 1.0) state->cached_tap_strength = 1.0;
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
  APP_LOG(APP_LOG_LEVEL_INFO, "Persist: Saving...");
  state->version = STORAGE_VERSION;
  state->last_update = time(NULL);
  persist_write_data(STORAGE_KEY_GAME_STATE, state, sizeof(GameState));
  APP_LOG(APP_LOG_LEVEL_INFO, "Persist: Save Complete");
}

bool game_state_load(GameState *state) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Persist: Loading...");
  if (persist_exists(STORAGE_KEY_GAME_STATE)) {
    persist_read_data(STORAGE_KEY_GAME_STATE, state, sizeof(GameState));
    if (state->version == STORAGE_VERSION) {
      game_state_update_cache(state);
      APP_LOG(APP_LOG_LEVEL_INFO, "Persist: Load Success");
      return true;
    }
    APP_LOG(APP_LOG_LEVEL_WARNING, "Persist: Outdated Version");
  }
  return false;
}

double game_state_apply_offline_gains(GameState *state) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Math: Offline Check");
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
    APP_LOG(APP_LOG_LEVEL_INFO, "Math: Offline Gain Applied");
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
  // 1 Step = 1.0 Seconds of total passive gravity
  double gravity = game_state_calculate_gravity(state);
  
  // Floor for starting out: if gravity is zero, 1 step = 100mg
  double gain = (gravity > 0) ? (gravity * (double)steps) : (100.0 * (double)steps);
  state->mass += gain;
}

int game_state_get_era(GameState *state) {
  int highest = -1;
  for (int i = NUM_TIERS - 1; i >= 0; i--) {
    if (state->counts[i] > 0) {
      highest = i;
      break;
    }
  }
  if (highest <= 2) return 0; // Terrestrial (Pebble, Rock, Boulder)
  if (highest <= 4) return 1; // Lunar/Asteroid (Mountain, Asteroid)
  if (highest <= 6) return 2; // Planetary (Moon, Planet)
  if (highest == 7) return 3; // Stellar (Gas Giant)
  return 4;                   // Singularity (Star)
}
