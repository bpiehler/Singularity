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

  if (mass < 1000.0) {
    // Milligrams: 0 - 999 mg
    snprintf(buffer, 32, "%d mg", (int)mass);
  } else if (mass < 1e6) {
    // Grams: 1.0 g - 999.9 g
    double val = mass / 1e3;
    snprintf(buffer, 32, "%d.%d g", (int)val, (int)((val - (int)val) * 10.0 + 0.5) % 10);
  } else if (mass < 1e9) {
    // Kilograms: 1.0 kg - 999.9 kg
    double val = mass / 1e6;
    snprintf(buffer, 32, "%d.%d kg", (int)val, (int)((val - (int)val) * 10.0 + 0.5) % 10);
  } else {
    // Tonnes anchor for everything 1e9 mg and above
    double tonnes = mass / 1e9;
    
    if (tonnes < 1000.0) {
      // Tonnes: 1.0 t - 999.9 t
      snprintf(buffer, 32, "%d.%d t", (int)tonnes, (int)((tonnes - (int)tonnes) * 10.0 + 0.5) % 10);
    } else {
      // Scientific notation on tonnes: >= 1000 t
      int exponent = 0;
      double mantissa = tonnes;
      while (mantissa >= 10.0 && exponent < 308) {
        mantissa /= 10.0;
        exponent++;
      }
      
      int m_int = (int)mantissa;
      int m_frac = (int)((mantissa - m_int) * 1000.0 + 0.5);
      if (m_frac >= 1000) {
        m_int++;
        m_frac = 0;
      }
      snprintf(buffer, 32, "%d.%03de%d t", m_int, m_frac, exponent);
    }
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
