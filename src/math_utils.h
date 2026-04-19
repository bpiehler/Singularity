#pragma once

#include <pebble.h>

// Format a double into a scientific notation string (e.g., "1.234e56")
// Buffer must be at least 32 bytes
void format_mass(double mass, char *buffer);

// Calculate the cost of the next unit in a tier
// C = Base * 1.15^Count
double calculate_cost(double base_cost, int count);

// Calculate the bonus multiplier for a tier based on milestones (25 units)
// Mult = 2 ^ (Count / 25)
double calculate_milestone_multiplier(int count);

// Calculate Cosmic Dust earned upon prestige
// Dust = sqrt(TotalMass / Threshold)
double calculate_prestige_dust(double total_mass, double threshold);
