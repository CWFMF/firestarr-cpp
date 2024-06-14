/* SPDX-FileCopyrightText: 2005, 2021 Jordan Evens */
/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2021-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "InnerPos.h"

namespace fs::sim
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

/**
 * Points in a cell furthest in each direction
 */
class CellPoints
{
public:
  using array_pts = std::array<InnerPos, NUM_DIRECTIONS>;
  using array_dists = std::array<double, NUM_DIRECTIONS>;
  CellPoints() noexcept;
  CellPoints(const vector<InnerPos>& pts) noexcept;
  CellPoints(const double x, const double y) noexcept;
  CellPoints(const InnerPos& p) noexcept;
  void
  insert(const InnerPos& p) noexcept;
  set<InnerPos>
  unique() const noexcept
  {
    return set<InnerPos>{pts_.begin(), pts_.end()};
  }
  const array_pts
  points() const
  {
    return pts_;
  }
private:
  void
  insert(const double cell_x, const double cell_y, const InnerPos& p) noexcept;
  array_pts pts_;
  array_dists dists_;
};
}
