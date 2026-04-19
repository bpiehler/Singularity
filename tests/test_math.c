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
  printf("Testing Expanded Formatting thresholds...\n");
  char buf[32];
  
  // mg
  format_mass(500, buf);
  assert(strcmp(buf, "500 mg") == 0);
  
  // kt (kilotonnes)
  format_mass(1.5e12, buf);
  assert(strcmp(buf, "1.5 kt") == 0);
  
  // Gt (gigatonnes)
  format_mass(2.5e18, buf);
  assert(strcmp(buf, "2.5 Gt") == 0);
  
  // Tt (teratonnes)
  format_mass(8.2e21, buf);
  assert(strcmp(buf, "8.2 Tt") == 0);
  
  // scientific threshold (now 1e24)
  format_mass(1e24, buf);
  assert(strcmp(buf, "1.000e24 mg") == 0);
  format_mass(1e30, buf);
  assert(strcmp(buf, "1.000e30 mg") == 0);

  printf("✓ Formatting tests passed\n");
}

int main() {
  printf("=== STARTING REALISM LOGIC TESTS ===\n");
  test_formatting();
  printf("=== ALL TESTS PASSED ===\n");
  return 0;
}
