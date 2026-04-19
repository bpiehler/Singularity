#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stddef.h>
#include "../src/math_utils.h"
#include "../src/game_state.h"

// Mock implementation files for full logic test
#include "../src/math_utils.c"
#include "../src/game_state.c"

void test_formatting() {
  printf("Testing Tonnes-Anchored Formatting...\n");
  char buf[32];
  
  format_mass(500, buf);
  assert(strcmp(buf, "500 mg") == 0);
  
  format_mass(1e30, buf);
  assert(strcmp(buf, "1.000e21 t") == 0);

  printf("✓ Formatting tests passed\n");
}

void test_empowered_tap() {
  printf("Testing Empowered Tap Logic...\n");
  GameState state;
  game_state_init(&state);
  
  // Starting out: 10mg tap (Fast-Start)
  assert(game_state_calculate_tap_strength(&state) == 10.0);
  
  // Buy 1 Pebble (Base yield 1.0)
  state.counts[0] = 1;
  game_state_update_cache(&state);
  
  // Tap should be (1*10) + (Gravity*0.25)
  // Gravity is exactly 1.0. Tap = 10 + 0.25 = 10.25
  double tap = game_state_calculate_tap_strength(&state);
  assert(tap == 10.25);
  
  // Buy 1 Asteroid (Base yield 1.0e6)
  state.counts[3] = 1;
  game_state_update_cache(&state);
  
  // Highest yield owned is now 1.0e6 (Asteroid)
  // Total Gravity = Asteroid(1.0e6) + Pebble(1.0) = 1000001.0
  // Tap = (1.0e6 * 10) + (1000001.0 * 0.25) = 10,000,000 + 250,000.25 = 10,250,000.25
  tap = game_state_calculate_tap_strength(&state);
  assert(tap == 10250000.25);
  
  printf("✓ Tap Empowerment passed\n");
}

void test_empowered_steps() {
  printf("Testing Empowered Step Logic...\n");
  GameState state;
  game_state_init(&state);
  
  // No gravity: 1 step = 100mg (as per floor)
  game_state_add_steps(&state, 100);
  assert(state.mass == 1.0 + (100 * 100.0));
  
  // Set gravity to 1,000,000 mg/s
  state.counts[3] = 1; // 1 Asteroid
  game_state_update_cache(&state);
  double start_mass = state.mass;
  
  // 100 steps = 100 seconds of gravity
  game_state_add_steps(&state, 100);
  double expected_gain = 1000000.0 * 100.0;
  assert(state.mass == start_mass + expected_gain);
  
  printf("✓ Step Empowerment passed\n");
}

int main() {
  printf("=== STARTING EMPOWERMENT LOGIC TESTS ===\n");
  test_formatting();
  test_empowered_tap();
  test_empowered_steps();
  printf("=== ALL TESTS PASSED ===\n");
  return 0;
}
