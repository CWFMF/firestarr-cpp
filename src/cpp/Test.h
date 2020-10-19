/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_TEST_H
#define FS_TEST_H
#include "stdafx.h"
#include "FWI.h"
namespace fs
{
class FwiWeather;
};
namespace fs
{
#include "stdafx.h"
static const double TEST_GRID_SIZE = 100.0;
static const char TEST_PROJ4[] =
  "+proj=tmerc +lat_0=0.000000000 +lon_0=-90.000000000"
  " +k=0.999600 +x_0=500000.000 +y_0=0.000 +a=6378137.000 +b=6356752.314 +units=m";
static const double TEST_XLLCORNER = 221803.990666;
static const double TEST_YLLCORNER = 12543955.311160;
/**
 * \brief Runs test cases for constant weather and fuel based on given arguments.
 * \param output_directory root directory for outputs
 * \param constant_wx FwiWeather to use for constant indices, where anything Invalid uses default
 * \param constant_fuel_name FBP fuel to use or empty string for default
 * \param constant_slope Value to use for slope, where Invalid uses default
 * \param constant_aspect Value to use for aspect, where Invalid uses default
 * \param test_all whether to run all combinations of test outputs filtered by criteria (true), or
 * to just run the default single value modified by whatever has been specified (false)
 * \return
 */
int test(
  const string_view output_directory,
  const DurationSize num_hours,
  const ptr<const FwiWeather> wx,
  const string_view constant_fuel_name,
  const SlopeSize constant_slope,
  const AspectSize constant_aspect,
  const bool test_all
);
}
#endif
