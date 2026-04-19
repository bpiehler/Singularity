#include "math_utils.h"
#include <math.h>
#include <stdio.h>

void format_mass(double mass, char *buffer) {
  // 1. Extreme Safety Guards
  if (isnan(mass)) {
    snprintf(buffer, 32, "NaN");
    return;
  }
  if (isinf(mass)) {
    snprintf(buffer, 32, "Infinity");
    return;
  }
  if (mass <= 0.0) {
    snprintf(buffer, 32, "0 mg");
    return;
  }
  if (mass > 1e300) {
    snprintf(buffer, 32, "Infinite Mass");
    return;
  }

  // 2. Unit Logic
  if (mass < 1000.0) {
    snprintf(buffer, 32, "%d mg", (int)mass);
  } else if (mass < 1e6) {
    double g = mass / 1000.0;
    int whole = (int)g;
    int frac = (int)((g - (double)whole) * 10.0 + 0.5) % 10;
    snprintf(buffer, 32, "%d.%d g", whole, frac);
  } else if (mass < 1e9) {
    double kg = mass / 1e6;
    int whole = (int)kg;
    int frac = (int)((kg - (double)whole) * 10.0 + 0.5) % 10;
    snprintf(buffer, 32, "%d.%d kg", whole, frac);
  } else {
    // Tonnes Anchor
    double tonnes = mass / 1e9;
    
    if (tonnes < 1000.0) {
      int whole = (int)tonnes;
      int frac = (int)((tonnes - (double)whole) * 10.0 + 0.5) % 10;
      snprintf(buffer, 32, "%d.%d t", whole, frac);
    } else {
      // Scientific Notation on Tonnes
      int exponent = 0;
      double mantissa = tonnes;
      
      // Safe normalization loop
      if (mantissa > 0) {
        while (mantissa >= 10.0 && exponent < 308) {
          mantissa /= 10.0;
          exponent++;
        }
      }
      
      int m_int = (int)mantissa;
      int m_frac = (int)((mantissa - (double)m_int) * 1000.0 + 0.5);
      if (m_frac >= 1000) {
        m_int++;
        m_frac = 0;
      }
      snprintf(buffer, 32, "%d.%03de%d t", m_int, m_frac, exponent);
    }
  }
}

double calculate_cost(double base_cost, int count) {
  if (count < 0) count = 0;
  if (count > 10000) return 1e300; // Cap cost
  return base_cost * pow(1.15, (double)count);
}

double calculate_milestone_multiplier(int count) {
  int milestones = count / 25;
  if (milestones > 100) milestones = 100; // Cap bonus
  return pow(2.0, (double)milestones);
}

double calculate_prestige_dust(double total_mass, double threshold) {
  if (total_mass < threshold || threshold <= 0) return 0;
  double ratio = total_mass / threshold;
  if (isnan(ratio) || isinf(ratio)) return 1e6; // Cap dust
  return sqrt(ratio);
}
