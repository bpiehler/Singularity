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
  printf("Testing Tonnes-Anchored Formatting...\n");
  char buf[32];
  
  // mg
  format_mass(500, buf);
  assert(strcmp(buf, "500 mg") == 0);
  
  // g
  format_mass(1500, buf);
  assert(strcmp(buf, "1.5 g") == 0);
  
  // kg
  format_mass(2500000, buf);
  assert(strcmp(buf, "2.5 kg") == 0);
  
  // t (below 1000)
  format_mass(5.2e9, buf);
  assert(strcmp(buf, "5.2 t") == 0);
  
  // t (scientific)
  // 1e12 mg = 1000 t = 1.000e3 t
  format_mass(1e12, buf);
  assert(strcmp(buf, "1.000e3 t") == 0);
  
  // 1e30 mg = 1e21 t = 1.000e21 t
  format_mass(1e30, buf);
  assert(strcmp(buf, "1.000e21 t") == 0);

  printf("✓ Formatting tests passed\n");
}

int main() {
  printf("=== STARTING TONNES-ANCHOR TESTS ===\n");
  test_formatting();
  printf("=== ALL TESTS PASSED ===\n");
  return 0;
}
