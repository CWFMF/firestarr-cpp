/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include "debug_settings.h"

// #define VLD_FORCE_ENABLE
// #include "vld.h"
#define _USE_MATH_DEFINES
#define NOMINMAX
#include <ctime>
#include <chrono>
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cerrno>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <execution>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <random>
#include <ranges>
#include <set>
#include <sstream>
#include <cstdint>
#include <string>
#include <cstring>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#ifdef _WIN32
#include <geo_normalize.h>
#include <geotiff.h>
#include <geovalues.h>
#include <xtiffio.h>
#else
#include <geotiff/geo_normalize.h>
#include <geotiff/geotiff.h>
#include <geotiff/geovalues.h>
#include <geotiff/xtiffio.h>
#endif
#include "tiff.h"
#include <tiffio.h>
#include <sys/stat.h>
// unreferenced inline function has been removed
// Informational: catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught
// function 'X' selected for automatic inline expansion
// function not inlined
// selected for automatic inline expansion
// Do not assign the result of an allocation or a function call with an owner<T> return value to a raw pointer, use owner<T> instead
// Do not delete a raw pointer that is not an owner<T>
// Return a scoped object instead of a heap-allocated if it has a move constructor
// Reset or explicitly delete an owner<T> pointer
// Do not assign to an owner<T> which may be in valid state
// Do not assign a raw pointer to an owner<T>
// Prefer scoped objects, don't heap-allocate unnecessarily
// Avoid calling new and delete explicitly, use std::make_unique<T> instead
// Global initializer calls a non-constexpr function
// Symbol is never tested for nullness, it can be marked as not_null
// Function hides a non-virtual function
// prefer to use gsl::at()
// Don't use a static_cast for arithmetic conversions. Use brace initialization, gsl::narrow_cast or gsl::narrow
// Don't use pointer arithmetic. Use span instead
// Only index into arrays using constant expressions
// No array to pointer decay
using std::abs;
using std::array;
using std::async;
using std::atomic;
using std::endl;
using std::fixed;
using std::function;
using std::future;
using std::get;
using std::getline;
using std::hash;
using std::ifstream;
using std::istringstream;
using std::launch;
using std::list;
using std::lock_guard;
using std::make_shared;
using std::make_tuple;
using std::make_unique;
using std::map;
using std::max;
using std::min;
using std::mt19937;
using std::mutex;
using std::numeric_limits;
using std::ofstream;
using std::ostream;
using std::ostringstream;
using std::pair;
using std::put_time;
using std::runtime_error;
using std::set;
using std::setprecision;
using std::shared_ptr;
using std::stod;
using std::stoi;
using std::stol;
using std::string;
using std::string_view;
using std::stringstream;
using std::to_string;
using std::to_wstring;
using std::tuple;
using std::uniform_real_distribution;
using std::unique_ptr;
using std::unordered_map;
using std::vector;
using std::wstring;
namespace tbd
{
/**
 * \brief Size of the hash of a Cell
 */
using HashSize = uint32_t;
/**
 * \brief Size of the index for a Cell
 */
using CellIndex = uint8_t;
/**
 * \brief A row or column index for a grid
 */
using Idx = int16_t;
/**
 * \brief A row or column index for a grid not in memory yet
 */
using FullIdx = int64_t;
/**
 * \brief Type used for perimeter raster values (uses [0, 1])
 */
// FIX: could try to get really fancy and use number of bits per pixel options
using PerimSize = uint8_t;
// using PerimSize = uint16_t;
/**
 * \brief Type used for fuel values (uses [0 - 999]?)
 */
// FIX: seriously does not like uint for some reason
using FuelSize = uint16_t;
// using FuelSize = int16_t;
/**
 * \brief Type used for aspect values (uses [0 - 359])
 */
using AspectSize = uint16_t;
/**
 * \brief Type used for elevation values (uses [? - 9800?])
 */
using ElevationSize = int16_t;
/**
 * \brief Type used for slope values (uses [0 - MAX_SLOPE_FOR_DISTANCE])
 */
using SlopeSize = uint8_t;
/**
 * \brief Type used for storing intensities
 */
using IntensitySize = uint16_t;
/**
 * \brief A day (0 - 366)
 */
using Day = uint16_t;
static constexpr Day MAX_DAYS = 366;
/**
 * \brief Maximum number of columns for an Environment
 */
static constexpr Idx MAX_COLUMNS = 4096;
/**
 * \brief Maximum number of rows for an Environment
 */
static constexpr Idx MAX_ROWS = MAX_COLUMNS;
// static_assert(static_cast<size_t>(MAX_ROWS) * (MAX_COLUMNS - 1) <= std::numeric_limits<Idx>::max());
static constexpr Idx PREFERRED_TILE_WIDTH = 256;
static constexpr Idx TILE_WIDTH = min(MAX_COLUMNS, static_cast<Idx>(PREFERRED_TILE_WIDTH));
/**
 * \brief Maximum aspect value (360 == 0)
 */
static constexpr auto MAX_ASPECT = 359;
/**
 * \brief Maximum slope that affects ISI - everything after this is the same factor
 */
static constexpr auto MAX_SLOPE_FOR_FACTOR = 60;
/**
 * \brief Maximum slope that can be stored - this is used in the horizontal distance calculation
 */
static constexpr auto MAX_SLOPE_FOR_DISTANCE = 127;
/**
 * \brief Number of all possible fuels in simulation
 */
static constexpr auto NUMBER_OF_FUELS = 56;
/**
 * \brief 2*pi
 */
static constexpr auto M_2_X_PI = 2.0 * M_PI;
/**
 * \brief 3/2*pi
 */
static constexpr auto M_3_X_PI_2 = 3.0 * M_PI_2;
/**
 * \brief Ratio of degrees to radians
 */
static constexpr auto M_RADIANS_TO_DEGREES = 180.0 / M_PI;
/**
 * \brief Number of hours in a day
 */
static constexpr int DAY_HOURS = 24;
/**
 * \brief Number of minutes in an hour
 */
static constexpr int HOUR_MINUTES = 60;
/**
 * \brief Number of seconds in a minute
 */
static constexpr int MINUTE_SECONDS = 60;
/**
 * \brief Number of minutes in a day
 */
static constexpr int DAY_MINUTES = DAY_HOURS * HOUR_MINUTES;
/**
 * \brief Number of seconds in a day
 */
static constexpr int DAY_SECONDS = DAY_MINUTES * MINUTE_SECONDS;
/**
 * \brief Number of hours in a year
 */
static constexpr int YEAR_HOURS = MAX_DAYS * DAY_HOURS;
/**
 * \brief Array of results of a function for all possible integer percent slopes
 */
using SlopeTableArray = array<double, MAX_SLOPE_FOR_DISTANCE + 1>;
/**
 * \brief Size of an angle in degrees
 */
using DegreesSize = uint16_t;
/**
 * \brief Array of results of a function for all possible integer angles in degrees
 */
using AngleTableArray = array<double, 361>;
/**
 * \brief Size to use for representing fuel types
 */
using FuelCodeSize = uint8_t;
/**
 * \brief Size to use for representing the data in a Cell
 */
using Topo = uint64_t;
/**
 * \brief Size to use for representing sub-coordinates for location within a Cell
 */
using SubSize = uint16_t;
/**
 * \brief Coordinates (row, column, sub-row, sub-column)
 */
using Coordinates = tuple<Idx, Idx, SubSize, SubSize>;
/**
 * \brief FullCoordinates (row, column, sub-row, sub-column)
 */
using FullCoordinates = tuple<FullIdx, FullIdx, SubSize, SubSize>;
/**
 * \brief Type of clock to use for times
 */
using Clock = std::chrono::steady_clock;
/**
 * \brief Offset from a position
 */
struct Offset;
/**
 * \brief Collection of Offsets
 */
using OffsetSet = vector<Offset>;

struct Offset
{
public:
  /**
   * \brief Offset in the x direction (column)
   */
  inline constexpr double x() const noexcept
  {
    return coords_[0];
  }
  /**
   * \brief Offset in the y direction (row)
   */
  inline constexpr double y() const noexcept
  {
    return coords_[1];
  }
  constexpr Offset(const double a, const double b) noexcept
    : coords_()
  {
    coords_[0] = a;
    coords_[1] = b;
  }
  constexpr Offset() noexcept
    : Offset(-1, -1)
  {
  }
  constexpr Offset(Offset&& rhs) noexcept = default;
  constexpr Offset(const Offset& rhs) noexcept = default;
  Offset& operator=(const Offset& rhs) noexcept = default;
  Offset& operator=(Offset&& rhs) noexcept = default;
  /**
   * \brief Multiply by duration to get total offset over time
   * \param duration time to multiply by
   */
  constexpr Offset after(const double duration) const noexcept
  {
    return Offset(x() * duration, y() * duration);
  }
  /**
   * \brief Less than operator
   * \param rhs Offset to compare to
   * \return Whether or not this is less than the other
   */
  bool operator<(const Offset& rhs) const noexcept
  {
    if (x() == rhs.x())
    {
      if (y() == rhs.y())
      {
        // they are "identical" so this is false
        return false;
      }
      return y() < rhs.y();
    }
    return x() < rhs.x();
  }
  /**
   * \brief Equality operator
   * \param rhs Offset to compare to
   * \return Whether or not this is equivalent to the other
   */
  bool operator==(const Offset& rhs) const noexcept
  {
    return (x() == rhs.x())
        && (y() == rhs.y());
  }
  /**
   * \brief Add offset to position and return result
   */
  [[nodiscard]] constexpr Offset add(const Offset o) const noexcept
  {
    return Offset(x() + o.x(), y() + o.y());
  }
  friend constexpr OffsetSet apply_duration(
    const double duration,
    // copy when passed in
    OffsetSet offsets);
  // constexpr inline OffsetSet apply_offsets(
  //   // copy when passed in
  //   OffsetSet offsets) const noexcept
  // {
  //   const double& x0 = coords_[0];
  //   const double& y0 = coords_[1];
  //   // putting results in copy of offsets and returning that
  //   Offset* o = &(offsets[0]);
  //   // this is an invalid point to after array we can use as a guard
  //   Offset* e = &(offsets[offsets.size()]);
  //   while (o != e)
  //   {
  //     double* x1 = &(o->coords_[0]);
  //     double* y1 = &(o->coords_[1]);
  //     (*x1) += x0;
  //     (*y1) += y0;
  //     ++o;
  //   }
  //   return offsets;
  // }
  constexpr inline OffsetSet apply_offsets(
    // copy when passed in
    OffsetSet offsets) const noexcept
  {
    const double& x0 = coords_[0];
    const double& y0 = coords_[1];
    // putting results in copy of offsets and returning that
    // at the end of everything, we're just adding something to every double in the set by duration?
    double* out = &(offsets[0].coords_[0]);
    // this is an invalid point to after array we can use as a guard
    double* e = &(offsets[offsets.size()].coords_[0]);
    while (out != e)
    {
      (*out) += x0;
      ++out;
      (*out) += y0;
      ++out;
    }
    return offsets;
  }
private:
  // coordinates as an array so we can treat an array of these as an array of doubles
  double coords_[2];
};
// define multiplication in other order since equivalent
constexpr Offset after(const double duration, const Offset& o)
{
  return o.after(duration);
}
constexpr inline OffsetSet apply_duration(
  const double duration,
  // copy when passed in
  OffsetSet offsets)
{
  // OffsetSet r{};
  // r.resize(offsets.size());
  // std::transform(
  //   offsets.cbegin(),
  //   offsets.cend(),
  //   r.begin(),
  //   [&duration](const Offset& o) -> Offset {
  //     return o.after(duration);
  //   });
  // at the end of everything, we're just mutliplying every double in the set by duration?
  double* d = &(offsets[0].coords_[0]);
  // this is an invalid point to after array we can use as a guard
  double* e = &(offsets[offsets.size()].coords_[0]);
  // std::for_each_n(
  //   d,
  //   (e - d),
  //   [&duration](const double x) {
  //     return x * duration;
  //   });
  // std::for_each(
  //   std::execution::par_unseq,
  //   d,
  //   e,
  //   [&duration](double& x) {
  //     x *= duration;
  //   });
  while (d != e)
  {
    *d *= duration;
    ++d;
  }
  return offsets;
}
}
