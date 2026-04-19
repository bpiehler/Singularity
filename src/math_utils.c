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
    int whole = (int)g;
    int frac = (int)((g - (double)whole) * 10.0 + 0.5) % 10;
    snprintf(buffer, 32, "%d.%d g", whole, frac);
  } else if (mass < 1e9) {
    double kg = mass / 1e6;
    int whole = (int)kg;
    int frac = (int)((kg - (double)whole) * 10.0 + 0.5) % 10;
    snprintf(buffer, 32, "%d.%d kg", whole, frac);
  } else {
    double tonnes = mass / 1e9;
    if (tonnes < 1000.0) {
      int whole = (int)tonnes;
      int frac = (int)((tonnes - (double)whole) * 10.0 + 0.5) % 10;
      snprintf(buffer, 32, "%d.%d t", whole, frac);
    } else {
      int exponent = 0;
      double mantissa = tonnes;
      int loop_count = 0;
      while (mantissa >= 10.0 && exponent < 308 && loop_count < 1000) {
        mantissa /= 10.0;
        exponent++;
        loop_count++;
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
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Loc: Calc Dust Start");
  if (total_mass < threshold || threshold <= 0) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Loc: Threshold not met");
    return 0;
  }
  
  static char lbuf[32];
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Val: Raw Mass");
  format_mass(total_mass, lbuf);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", lbuf);

  double ratio = total_mass / threshold;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Val: Ratio");
  format_mass(ratio, lbuf);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", lbuf);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Loc: Sqrt Trigger");
  double res = sqrt(ratio);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Loc: Sqrt Done");
  
  return res;
}
