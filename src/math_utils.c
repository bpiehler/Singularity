#include "math_utils.h"
#include <math.h>
#include <stdio.h>

void format_mass(double mass, char *buffer) {
  if (isnan(mass)) {
    snprintf(buffer, 32, "NaN");
    return;
  }
  if (isinf(mass)) {
    snprintf(buffer, 32, "Infinity");
    return;
  }
  if (mass <= 0) {
    snprintf(buffer, 32, "0 mg");
    return;
  }

  // Thresholds for units
  const char* units[] = {"mg", "g", "kg", "t", "kt", "Mt", "Gt", "Tt"};
  double current_threshold = 1000.0;
  
  // mg is handled as pure integer
  if (mass < 1000.0) {
    snprintf(buffer, 32, "%d mg", (int)mass);
    return;
  }

  // Find the appropriate metric unit
  int unit_idx = 0;
  double val = mass;
  while (val >= 1000.0 && unit_idx < 7) {
    val /= 1000.0;
    unit_idx++;
  }

  if (unit_idx < 8 && val < 1000.0) {
    // Add 0.05 for rounding to 1 decimal place before casting to int
    int whole = (int)val;
    int frac = (int)((val - whole) * 10.0 + 0.5);
    if (frac >= 10) {
      whole++;
      frac = 0;
    }
    snprintf(buffer, 32, "%d.%d %s", whole, frac, units[unit_idx]);
  } else {
    // Scientific notation for larger numbers: >= 1.0e24 mg
    int exponent = 0;
    double mantissa = mass;
    while (mantissa >= 10.0 && exponent < 308) {
      mantissa /= 10.0;
      exponent++;
    }
    while (mantissa < 1.0 && mantissa > 0.0 && exponent > -308) {
      mantissa *= 10.0;
      exponent--;
    }
    
    int mantissa_int = (int)mantissa;
    int mantissa_frac = (int)((mantissa - mantissa_int) * 1000.0 + 0.5);
    if (mantissa_frac >= 1000) {
      mantissa_int++;
      mantissa_frac = 0;
    }
    snprintf(buffer, 32, "%d.%03de%d mg", mantissa_int, mantissa_frac, exponent);
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
