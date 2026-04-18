#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "../src/math_utils.h"
#include "../src/game_state.h"

// Mock the missing implementation files by including them 
// (or we can compile them together, but for a simple script this works)
#include "../src/math_utils.c"
#include "../src/game_state.c"

void test_formatting() {
  char buf[32];
  
  format_mass(0, buf);
  assert(strcmp(buf, "0.000e0") == 0);
  
  format_mass(123.456, buf);
  assert(strcmp(buf, "123.456") == 0);
  
  format_mass(1000.0, buf);
  assert(strcmp(buf, "1.000e3") == 0);
  
  format_mass(1e16, buf);
  assert(strcmp(buf, "1.000e16") == 0);
  
  printf("✓ Formatting tests passed\n");
}

void test_costs() {
  // Base cost 100, 0 owned -> 100
  assert(fabs(calculate_cost(100, 0) - 100.0) < 0.001);
  
  // Base 100, 1 owned -> 115
  assert(fabs(calculate_cost(100, 1) - 115.0) < 0.001);
  
  printf("✓ Cost calculation tests passed\n");
}

void test_milestones() {
  // 0-24 units -> x1
  assert(calculate_milestone_multiplier(0) == 1.0);
  assert(calculate_milestone_multiplier(24) == 1.0);
  
  // 25 units -> x2
  assert(calculate_milestone_multiplier(25) == 2.0);
  
  // 50 units -> x4
  assert(calculate_milestone_multiplier(50) == 4.0);
  
  printf("✓ Milestone tests passed\n");
}

int main() {
  printf("Running Math Tests...\n");
  test_formatting();
  test_costs();
  test_milestones();
  printf("All tests passed!\n");
  return 0;
}
