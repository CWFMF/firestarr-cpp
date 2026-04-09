/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "debug_settings.h"
#ifdef DEBUG_ANY
#include <cstdio>
#include <format>
#include <iostream>
#include <string>
#endif
namespace fs
{
void show_debug_settings()
{
#ifdef DEBUG_ANY
  constexpr auto width = 60;
  constexpr auto side_width = 1;
  // FIX: duplicate code from settings file output
  std::string hr = std::format("{:#^{}}\n", "#", width);
  std::string side = std::format("{:#^{}}", "#", side_width);
  auto print_centered = [&](const std::string& text) {
    std::cout << std::format("{}{: ^{}}{}\n", side, text, width - (2 * side_width), side);
  };
  std::cout << hr;
  print_centered("DEBUG OPTIONS");
  std::cout << hr;
#ifndef NDEBUG
  print_centered("DEBUG");
#endif
#ifdef DEBUG_DIRECTIONS
  print_centered("DEBUG_DIRECTIONS");
#endif
#ifdef DEBUG_FUEL_VARIABLE
  print_centered("DEBUG_FUEL_VARIABLE");
#endif
#ifdef DEBUG_FWI_WEATHER
  print_centered("DEBUG_FWI_WEATHER");
#endif
#ifdef DEBUG_GRIDS
  print_centered("DEBUG_GRIDS");
#endif
#ifdef DEBUG_POINTS
  print_centered("DEBUG_POINTS");
#endif
#ifdef DEBUG_PROBABILITY
  print_centered("DEBUG_PROBABILITY");
#endif
#ifdef DEBUG_SIMULATION
  print_centered("DEBUG_SIMULATION");
#endif
#ifdef DEBUG_STATISTICS
  print_centered("DEBUG_STATISTICS");
#endif
#ifdef DEBUG_WEATHER
  print_centered("DEBUG_WEATHER");
#endif
  std::cout << hr;
#endif
}
}
