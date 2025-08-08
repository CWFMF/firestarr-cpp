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

struct CellPoints
{
  CellPoints() = default;
  CellPoints(const CellPoints& rhs) = default;
  CellPoints(CellPoints&& rhs) = default;
  CellPoints&
  operator=(const CellPoints& rhs) = default;
  CellPoints&
  operator=(CellPoints&& rhs) = default;
  CellPoints(const bool is_unburnable, const XYSize x, const XYSize y);
  void
  insert(const XYSize x, const XYSize y);
  bool
  isUnburnable() const;
  set<XYPos>
  unique(const HashSize hash_value) const;
  unique_ptr<array_dists> distances = nullptr;
  unique_ptr<array_pts> points = nullptr;
};

class Points
{
public:
  Points() = default;
  Points(const Points& rhs) noexcept = default;
  Points(Points&& rhs) noexcept = default;
  Points&
  operator=(const Points& rhs) noexcept = default;
  Points&
  operator=(Points&& rhs) noexcept = default;
  void
  insert(const bool is_unburnable, const XYSize x, const XYSize y);
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

  /**
   * \brief Iterator for underlying GridMap
   * \return Iterator for underlying GridMap
   */
  [[nodiscard]] map<HashSize, CellPoints>::iterator
  begin() noexcept
  {
    return map_.begin();
  }

  /**
   * \brief Iterator for underlying GridMap
   * \return Iterator for underlying GridMap
   */
  [[nodiscard]] map<HashSize, CellPoints>::iterator
  end() noexcept
  {
    return map_.end();
  }

  auto
  erase(
    auto& it
  )
  {
    return map_.erase(it);
  }

private:
  map<HashSize, CellPoints> map_{};
};
}
#endif
