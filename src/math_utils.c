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
  if (mass <= 0.0) {
    snprintf(buffer, 32, "0 mg");
    return;
  }

  if (mass < 1000.0) {
    // Milligrams: 0 - 999 mg
    snprintf(buffer, 32, "%d mg", (int)mass);
  } else if (mass < 1e6) {
    // Grams: 1.0 g - 999.9 g
    double g = mass / 1000.0;
    int whole = (int)g;
    int frac = (int)((g - (double)whole) * 10.0 + 0.5) % 10;
    snprintf(buffer, 32, "%d.%d g", whole, frac);
  } else if (mass < 1e9) {
    // Kilograms: 1.0 kg - 999.9 kg
    double kg = mass / 1e6;
    int whole = (int)kg;
    int frac = (int)((kg - (double)whole) * 10.0 + 0.5) % 10;
    snprintf(buffer, 32, "%d.%d kg", whole, frac);
  } else {
    // Tonnes anchor for everything 1e9 mg and above
    double tonnes = mass / 1e9;
    
    // Safety: only cast to int if tonnes is actually small
    if (tonnes < 1000000.0) {
      int whole = (int)tonnes;
      int frac = (int)((tonnes - (double)whole) * 10.0 + 0.5) % 10;
      snprintf(buffer, 32, "%d.%d t", whole, frac);
    } else {
      // Scientific notation on tonnes: >= 1000 t
      int exponent = 0;
      double mantissa = tonnes;
      while (mantissa >= 10.0 && exponent < 308) {
        mantissa /= 10.0;
        exponent++;
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
  return base_cost * pow(1.15, (double)count);
}

double calculate_milestone_multiplier(int count) {
  return pow(2.0, (double)(count / 25));
}

double calculate_prestige_dust(double total_mass, double threshold) {
  if (total_mass < threshold || threshold <= 0) return 0;
  
  double ratio = total_mass / threshold;
  if (isnan(ratio) || isinf(ratio)) return 1.0;
  
  // Use pow(x, 0.5) as a stack-safe sqrt alternative
  return pow(ratio, 0.5);
}
