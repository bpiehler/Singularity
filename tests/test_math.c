#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stddef.h>
#include "../src/math_utils.h"
#include "../src/game_state.h"

// Mock the missing implementation files
#include "../src/math_utils.c"
#include "../src/game_state.c"

void test_formatting() {
  printf("Testing Formatting...\n");
  char buf[32];
  
  format_mass(0, buf);
  assert(strcmp(buf, "0.000e0") == 0);
  
  format_mass(123.456, buf);
  assert(strcmp(buf, "123.456") == 0);
  
  format_mass(1e16, buf);
  assert(strcmp(buf, "1.000e16") == 0);

  format_mass(NAN, buf);
  assert(strcmp(buf, "NaN") == 0);

  printf("✓ Formatting tests passed\n");
}

void test_alignment() {
  printf("Testing Memory Alignment...\n");
  // Mass and Dust must be at the very start for 8-byte alignment on ARM
  assert(offsetof(GameState, mass) == 0);
  assert(offsetof(GameState, dust) == 8);
  printf("✓ Alignment tests passed\n");
}

void test_buy_max_geometric() {
  printf("Testing Buy Max (Geometric Series)...\n");
  GameState state;
  game_state_init(&state);
  
  // Give enough mass to buy exactly 10 Pebbles
  // Cost for 10 units = 100 * (1.15^10 - 1) / 0.15 = 2030.37
  state.mass = 2031;
  game_state_buy_max(&state, 0);
  
  assert(state.counts[0] == 10);
  assert(state.mass < 1.0); // Should have ~0.63 mg left
  
  printf("✓ Buy Max math passed\n");
}

int main() {
  printf("=== STARTING AUTOMATED LOGIC TESTS ===\n");
  test_alignment();
  test_formatting();
  test_buy_max_geometric();
  printf("=== ALL TESTS PASSED ===\n");
  return 0;
}
