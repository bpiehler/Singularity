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
    snprintf(buffer, 32, "%d mg", (int)mass);
  } else if (mass < 1e6) {
    double g = mass / 1000.0;
    snprintf(buffer, 32, "%d.%d g", (int)g, (int)((g - (double)((int)g)) * 10.0 + 0.5) % 10);
  } else if (mass < 1e9) {
    double kg = mass / 1e6;
    snprintf(buffer, 32, "%d.%d kg", (int)kg, (int)((kg - (double)((int)kg)) * 10.0 + 0.5) % 10);
  } else {
    double tonnes = mass / 1e9;
    if (tonnes < 1000.0) {
      snprintf(buffer, 32, "%d.%d t", (int)tonnes, (int)((tonnes - (double)((int)tonnes)) * 10.0 + 0.5) % 10);
    } else {
      int exponent = 0;
      double mantissa = tonnes;
      while (mantissa >= 10.0 && exponent < 308) {
        mantissa /= 10.0;
        exponent++;
      }
      int m_int = (int)mantissa;
      int m_frac = (int)((mantissa - (double)m_int) * 1000.0 + 0.5);
      if (m_frac >= 1000) { m_int++; m_frac = 0; }
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
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Math: Prestige Calc Start");
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Mass: %d", (int)(total_mass / 1e20)); // Log scaled mass
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Threshold: %d", (int)(threshold / 1e20));

  if (total_mass < threshold || threshold <= 0) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Math: Threshold Not Met");
    return 0;
  }
  
  double ratio = total_mass / threshold;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Ratio: %d", (int)ratio);
  
  double dust = sqrt(ratio);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Result: %d", (int)dust);
  
  return dust;
}
