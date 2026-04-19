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
  printf("Testing Tonne-Anchored Scientific Formatting...\n");
  char buf[32];
  
  // Early game
  format_mass(500, buf);
  assert(strcmp(buf, "500 mg") == 0);
  format_mass(1500, buf);
  assert(strcmp(buf, "1.5 g") == 0);
  format_mass(2500000, buf);
  assert(strcmp(buf, "2.5 kg") == 0);
  
  // Tonne anchor
  format_mass(5.2e9, buf);
  assert(strcmp(buf, "5.2 t") == 0);
  
  // Tonne scientific
  // 1e12 mg = 1000 t = 1.000e3 t
  format_mass(1e12, buf);
  assert(strcmp(buf, "1.000e3 t") == 0);
  
  // 1e36 mg = 1e27 t = 1.000e27 t
  format_mass(1e36, buf);
  assert(strcmp(buf, "1.000e27 t") == 0);

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
  
  // Tap should be (1*10) + (Gravity*0.25) = 10.25
  double tap = game_state_calculate_tap_strength(&state);
  assert(tap == 10.25);
  
  printf("✓ Tap Empowerment passed\n");
}

int main() {
  printf("=== STARTING PHASE 1 TESTS ===\n");
  test_formatting();
  test_empowered_tap();
  printf("=== ALL TESTS PASSED ===\n");
  return 0;
}
