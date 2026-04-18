#include "math_utils.h"
#include <math.h>
#include <stdio.h>

void format_mass(double mass, char *buffer) {
  if (mass == 0) {
    snprintf(buffer, 16, "0.000e0");
    return;
  }

  if (mass < 1000.0) {
    // Standard notation for small numbers
    int integer_part = (int)mass;
    int fractional_part = (int)((mass - integer_part) * 1000.0);
    snprintf(buffer, 16, "%d.%03d", integer_part, fractional_part);
  } else {
    // Scientific notation for larger numbers
    int exponent = 0;
    double mantissa = mass;
    while (mantissa >= 10.0) {
      mantissa /= 10.0;
      exponent++;
    }
    while (mantissa < 1.0 && mantissa > 0.0) {
      mantissa *= 10.0;
      exponent--;
    }
    
    // We manually extract parts of the double since snprintf with %e 
    // might not be supported on all Pebble SDK platforms/firmwares.
    int mantissa_int = (int)mantissa;
    int mantissa_frac = (int)((mantissa - mantissa_int) * 1000.0);
    snprintf(buffer, 16, "%d.%03de%d", mantissa_int, mantissa_frac, exponent);
  }
}

double calculate_cost(double base_cost, int count) {
  return base_cost * pow(1.15, count);
}

double calculate_milestone_multiplier(int count) {
  int milestones = count / 25;
  return pow(2.0, (double)milestones);
}

double calculate_prestige_dust(double total_mass, double threshold) {
  if (total_mass < threshold) return 0;
  return sqrt(total_mass / threshold);
}
