/* SPDX-FileCopyrightText: 2021-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef FS_DEBUG_SETTINGS_H
#define FS_DEBUG_SETTINGS_H

// if in debug mode then set everything, otherwise uncomment turning things off if trying to debug
// specific things
#define DEBUG_DIRECTIONS
#define DEBUG_FUEL_VARIABLE
#define DEBUG_FWI_WEATHER
#define DEBUG_GRIDS
#define DEBUG_POINTS
#define DEBUG_PROBABILITY
#define DEBUG_SIMULATION
#define DEBUG_STATISTICS
#define DEBUG_CELLPOINTS
#define DEBUG_NEW_SPREAD
#define DEBUG_NEW_SPREAD_CHECK
#define DEBUG_NEW_SPREAD_VERBOSE
// #define DEBUG_WEATHER
// // use this for trying to troubleshoot specific problems short-term
// #define DEBUG_TEMPORARY

#ifdef NDEBUG

#undef DEBUG_DIRECTIONS
#undef DEBUG_FUEL_VARIABLE
#undef DEBUG_FWI_WEATHER
#undef DEBUG_GRIDS

#undef DEBUG_POINTS
#undef DEBUG_PROBABILITY
#undef DEBUG_SIMULATION
#undef DEBUG_STATISTICS
#undef DEBUG_WEATHER
#endif
#undef DEBUG_CELLPOINTS
#undef DEBUG_NEW_SPREAD
#undef DEBUG_NEW_SPREAD_CHECK
#undef DEBUG_NEW_SPREAD_VERBOSE

#define USE_OLD_SPREAD
#define USE_NEW_SPREAD

#ifndef USE_OLD_SPREAD
#undef DEBUG_CELLPOINTS
#endif
#ifndef USE_NEW_SPREAD
#undef DEBUG_NEW_SPREAD
#undef DEBUG_NEW_SPREAD_CHECK
#undef DEBUG_NEW_SPREAD_VERBOSE
#endif

#if not(defined(NDEBUG)) || defined(DEBUG_DIRECTIONS) || defined(DEBUG_FUEL_VARIABLE)              \
  || defined(DEBUG_FWI_WEATHER) || defined(DEBUG_GRIDS) || defined(DEBUG_POINTS)                   \
  || defined(DEBUG_PROBABILITY) || defined(DEBUG_SIMULATION) || defined(DEBUG_STATISTICS)          \
  || defined(DEBUG_WEATHER)
#define DEBUG_ANY
#endif

namespace fs::debug
{
void
show_debug_settings();
}
#endif
