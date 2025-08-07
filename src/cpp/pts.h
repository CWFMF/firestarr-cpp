/* SPDX-FileCopyrightText: 2005, 2021 Jordan Evens */
/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2021-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef FS_PTS_H
#define FS_PTS_H

#include "stdafx.h"

#include "InnerPos.h"
#include "IntensityMap.h"

namespace fs
{
static constexpr size_t FURTHEST_N = 0;
static constexpr size_t FURTHEST_NNE = 1;
static constexpr size_t FURTHEST_NE = 2;
static constexpr size_t FURTHEST_ENE = 3;
static constexpr size_t FURTHEST_E = 4;
static constexpr size_t FURTHEST_ESE = 5;
static constexpr size_t FURTHEST_SE = 6;
static constexpr size_t FURTHEST_SSE = 7;
static constexpr size_t FURTHEST_S = 8;
static constexpr size_t FURTHEST_SSW = 9;
static constexpr size_t FURTHEST_SW = 10;
static constexpr size_t FURTHEST_WSW = 11;
static constexpr size_t FURTHEST_W = 12;
static constexpr size_t FURTHEST_WNW = 13;
static constexpr size_t FURTHEST_NW = 14;
static constexpr size_t FURTHEST_NNW = 15;
static constexpr size_t NUM_DIRECTIONS = 16;

static constexpr auto MASK_NE = DIRECTION_N & DIRECTION_NE & DIRECTION_E;
static constexpr auto MASK_SE = DIRECTION_S & DIRECTION_SE & DIRECTION_E;
static constexpr auto MASK_SW = DIRECTION_S & DIRECTION_SW & DIRECTION_W;
static constexpr auto MASK_NW = DIRECTION_N & DIRECTION_NW & DIRECTION_W;
// mask of sides that would need to be burned for direction to not matter
static constexpr std::array<CellIndex, NUM_DIRECTIONS> DIRECTION_MASKS{
  DIRECTION_N,
  MASK_NE,
  MASK_NE,
  MASK_NE,
  DIRECTION_E,
  MASK_SE,
  MASK_SE,
  MASK_SE,
  DIRECTION_S,
  MASK_SW,
  MASK_SW,
  MASK_SW,
  DIRECTION_W,
  MASK_NW,
  MASK_NW,
  MASK_NW
};
static constexpr std::array<const char*, NUM_DIRECTIONS> DIRECTION_NAMES{
  "N",
  "NNE",
  "NE",
  "ENE",
  "E",
  "ESE",
  "SE",
  "SSE",
  "S",
  "SSW",
  "SW",
  "WSW",
  "W",
  "WNW",
  "NW",
  "NNW"
};
using array_dists = std::array<DistanceSize, NUM_DIRECTIONS>;
using array_pts = std::array<InnerPos, NUM_DIRECTIONS>;
using spreading_points_new = map<SpreadKey, vector<pair<HashSize, set<XYPos>>>>;
static constexpr const DistanceSize INVALID_DISTANCE = static_cast<DistanceSize>(
  MAX_ROWS * MAX_ROWS
);
// not sure what's going on with this and wondering if it doesn't keep number exactly
// shouldn't be any way to be further than twice the entire width of the area
static const XYPos INVALID_XY_POSITION{};
static const pair<DistanceSize, XYPos> INVALID_XY_PAIR{INVALID_DISTANCE, INVALID_XY_POSITION};
static const XYSize INVALID_XY_LOCATION = INVALID_XY_PAIR.second.x();
static constexpr const InnerPos INVALID_INNER_POSITION{};
static const pair<DistanceSize, InnerPos> INVALID_INNER_PAIR{INVALID_DISTANCE, {}};
static const InnerSize INVALID_INNER_LOCATION = INVALID_INNER_PAIR.second.x();

// HACK: fill with default values
template <class T>
constexpr std::array<T, NUM_DIRECTIONS>
make_filled(
  const T& value
)
{
  std::array<T, NUM_DIRECTIONS> r{};
  r.fill(value);
  return r;
}

static constexpr array_dists INVALID_DISTANCES = make_filled(INVALID_DISTANCE);
static_assert(INVALID_DISTANCE == INVALID_DISTANCES[0]);
static_assert(INVALID_DISTANCE == INVALID_DISTANCES[1]);
static_assert(INVALID_DISTANCE == INVALID_DISTANCES[2]);
static_assert(INVALID_DISTANCE == INVALID_DISTANCES[3]);
static_assert(INVALID_DISTANCE == INVALID_DISTANCES[4]);
static_assert(INVALID_DISTANCE == INVALID_DISTANCES[5]);
static_assert(INVALID_DISTANCE == INVALID_DISTANCES[6]);
static_assert(INVALID_DISTANCE == INVALID_DISTANCES[7]);
static_assert(INVALID_DISTANCE == INVALID_DISTANCES[8]);
static_assert(INVALID_DISTANCE == INVALID_DISTANCES[9]);
static_assert(INVALID_DISTANCE == INVALID_DISTANCES[10]);
static_assert(INVALID_DISTANCE == INVALID_DISTANCES[11]);
static_assert(INVALID_DISTANCE == INVALID_DISTANCES[12]);
static_assert(INVALID_DISTANCE == INVALID_DISTANCES[13]);
static_assert(INVALID_DISTANCE == INVALID_DISTANCES[14]);
static_assert(INVALID_DISTANCE == INVALID_DISTANCES[15]);

class Pts
{
public:
  Pts(const bool is_unburnable, const XYPos p);

  Pts(
    const bool is_unburnable,
    const XYSize x,
    const XYSize y
  )
    : Pts(is_unburnable, XYPos{x, y})
  {
  }

  Pts&
  insert(
    const XYPos p0
  )
  {
    return insert(p0.x(), p0.y());
  }

  Pts&
  insert(const XYSize x, const XYSize y);
  set<XYPos>
  unique() const noexcept;
  bool
  empty() const;

  inline const array_dists&
  distances() const
  {
    return distances_;
  }

  inline const array_pts&
  points() const
  {
    return points_;
  }

  inline array_dists&
  distances()
  {
    return distances_;
  }

  inline array_pts&
  points()
  {
    return points_;
  }

  bool
  canBurn() const;

private:
  array_dists distances_ = INVALID_DISTANCES;
  array_pts points_{};
  CellPos cell_x_y_{INVALID_INDEX, INVALID_INDEX};
};

class PtMap
{
public:
  PtMap() = default;
  PtMap(const PtMap& rhs) noexcept = default;
  PtMap(PtMap&& rhs) noexcept = default;
  PtMap&
  operator=(const PtMap& rhs) noexcept = default;
  PtMap&
  operator=(PtMap&& rhs) noexcept = default;
  void
  insert(const bool is_unburnable, const XYPos p0);

  void
  insert(
    const bool is_unburnable,
    const XYSize x,
    const XYSize y
  )
  {
    insert(is_unburnable, XYPos{x, y});
  }

  set<XYPos>
  unique(const HashSize hash_value) const noexcept;
  set<XYPos>
  unique() const noexcept;
  set<HashSize>
  keys() const noexcept;
  size_t
  size() const noexcept;
  size_t
  erase(const HashSize hash_value) noexcept;
  // private:
  map<HashSize, Pts> map_;
};
}
#endif
