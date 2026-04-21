#include "math_utils.h"
#include <math.h>
#include <stdio.h>

void format_mass(double mass, char *buffer) {
  if (isnan(mass)) { snprintf(buffer, 32, "NaN"); return; }
  if (isinf(mass)) { snprintf(buffer, 32, "Infinity"); return; }
  if (mass <= 0.0) { snprintf(buffer, 32, "0 mg"); return; }

  if (mass < 1000.0) {
    snprintf(buffer, 32, "%d mg", (int)mass);
  } else if (mass < 1e6) {
    double g = mass / 1000.0;
    int whole = (int)g;
    int frac = (int)((g - (double)whole) * 10.0 + 0.5);
    if (frac >= 10) { whole++; frac = 0; }
    snprintf(buffer, 32, "%d.%d g", whole, frac);
  } else if (mass < 1e9) {
    double kg = mass / 1e6;
    int whole = (int)kg;
    int frac = (int)((kg - (double)whole) * 10.0 + 0.5);
    if (frac >= 10) { whole++; frac = 0; }
    snprintf(buffer, 32, "%d.%d kg", whole, frac);
  } else {
    double tonnes = mass / 1e9;
    if (tonnes < 1000.0) {
      int whole = (int)tonnes;
      int frac = (int)((tonnes - (double)whole) * 10.0 + 0.5);
      if (frac >= 10) { whole++; frac = 0; }
      snprintf(buffer, 32, "%d.%d t", whole, frac);
    } else {
      int exp = 0;
      double mantissa = tonnes;
      while (mantissa >= 10.0 && exp < 308) { mantissa /= 10.0; exp++; }
      int m_int = (int)mantissa;
      int m_frac = (int)((mantissa - (double)m_int) * 100.0 + 0.5);
      if (m_frac >= 100) { m_int++; m_frac = 0; }
      snprintf(buffer, 32, "%d.%02de%d t", m_int, m_frac, exp);
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
  if (ratio < 0) return 0;
  
  // Custom Newton-Raphson Sqrt for total stability
  double x = ratio;
  double y = 1.0;
  for (int i = 0; i < 10; i++) {
    y = (y + x / y) / 2.0;
  }
  return y;
}
