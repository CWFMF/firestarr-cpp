/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "debug_settings.h"
#include <cstdio>
#include <format>
#include <iostream>
#include <string>
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
  auto printf_centered = [&](const std::string& text) {
    std::cout << std::format("{}{: ^{}}{}\n", side, text, width - (2 * side_width), side);
  };
  std::cout << hr;
  printf_centered("DEBUG OPTIONS");
  std::cout << hr;
#endif
#ifndef NDEBUG
  printf_centered("DEBUG");
#endif
#ifdef DEBUG_DIRECTIONS
  printf_centered("DEBUG_DIRECTIONS");
#endif
#ifdef DEBUG_FUEL_VARIABLE
  printf_centered("DEBUG_FUEL_VARIABLE");
#endif
#ifdef DEBUG_FWI_WEATHER
  printf_centered("DEBUG_FWI_WEATHER");
#endif
#ifdef DEBUG_GRIDS
  printf_centered("DEBUG_GRIDS");
#endif
#ifdef DEBUG_POINTS
  printf_centered("DEBUG_POINTS");
#endif
#ifdef DEBUG_PROBABILITY
  printf_centered("DEBUG_PROBABILITY");
#endif
#ifdef DEBUG_SIMULATION
  printf_centered("DEBUG_SIMULATION");
#endif
#ifdef DEBUG_STATISTICS
  printf_centered("DEBUG_STATISTICS");
#endif
#ifdef DEBUG_WEATHER
  printf_centered("DEBUG_WEATHER");
#endif
#ifdef DEBUG_ANY
  std::cout << hr;
#endif
}
}
