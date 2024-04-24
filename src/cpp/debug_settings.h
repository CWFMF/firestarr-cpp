/* Copyright (c) Queen's Printer for Ontario, 2020. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once

// if in debug mode then set everything, otherwise uncomment turning things off if trying to debug specific things
#define DEBUG_FUEL_VARIABLE
#define DEBUG_FWI_WEATHER
#define DEBUG_GRIDS
#define DEBUG_PROBABILITY
#define DEBUG_SIMULATION
#define DEBUG_STATISTICS
#define DEBUG_WEATHER

#ifdef NDEBUG

#undef DEBUG_FUEL_VARIABLE
#undef DEBUG_FWI_WEATHER
#undef DEBUG_GRIDS
#undef DEBUG_PROBABILITY
#undef DEBUG_SIMULATION
#undef DEBUG_STATISTICS
#undef DEBUG_WEATHER

#endif

#if not(defined(NDEBUG)) || defined(DEBUG_FUEL_VARIABLE) || defined(DEBUG_FWI_WEATHER) || defined(DEBUG_GRIDS) || defined(DEBUG_PROBABILITY) || defined(DEBUG_SIMULATION) || defined(DEBUG_STATISTICS) || defined(DEBUG_WEATHER)
#define DEBUG_ANY
#endif

namespace tbd::debug
{
void show_debug_settings();
}
