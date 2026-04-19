#include "math_utils.h"
#include <math.h>
#include <stdio.h>

void format_mass(double mass, char *buffer) {
  if (isnan(mass)) {
    snprintf(buffer, 16, "NaN");
    return;
  }
  if (isinf(mass)) {
    snprintf(buffer, 16, "Infinity");
    return;
  }
  if (mass <= 0) {
    snprintf(buffer, 16, "0 mg");
    return;
  }

  if (mass < 1000.0) {
    // Milligrams: 0 - 999 mg
    snprintf(buffer, 16, "%d mg", (int)mass);
  } else if (mass < 1000000.0) {
    // Grams: 1.0 g - 999.9 g
    double g = mass / 1000.0;
    int whole = (int)g;
    int frac = (int)((g - whole) * 10.0);
    snprintf(buffer, 16, "%d.%d g", whole, frac);
  } else if (mass < 1000000000.0) {
    // Kilograms: 1.0 kg - 999.9 kg
    double kg = mass / 1000000.0;
    int whole = (int)kg;
    int frac = (int)((kg - whole) * 10.0);
    snprintf(buffer, 16, "%d.%d kg", whole, frac);
  } else if (mass < 1000000000000.0) {
    // Tonnes: 1.0 t - 999.9 t
    double t = mass / 1000000000.0;
    int whole = (int)t;
    int frac = (int)((t - whole) * 10.0);
    snprintf(buffer, 16, "%d.%d t", whole, frac);
  } else {
    // Scientific notation for larger numbers: >= 1.000e12 mg
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
    int mantissa_frac = (int)((mantissa - mantissa_int) * 1000.0);
    snprintf(buffer, 16, "%d.%03de%d mg", mantissa_int, mantissa_frac, exponent);
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
