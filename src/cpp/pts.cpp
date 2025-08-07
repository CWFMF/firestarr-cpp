/* SPDX-FileCopyrightText: 2005, 2021 Jordan Evens */
/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2021-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "pts.h"

#include "Location.h"
#include "Log.h"
#include "Scenario.h"

namespace fs
{
constexpr InnerSize DIST_22_5 = static_cast<InnerSize>(
  0.2071067811865475244008443621048490392848359376884740365883398689
);
constexpr InnerSize P_0_5 = static_cast<InnerSize>(0.5) + DIST_22_5;
constexpr InnerSize M_0_5 = static_cast<InnerSize>(0.5) - DIST_22_5;
using DISTANCE_PAIR = pair<DistanceSize, DistanceSize>;
#define D_PTS(x, y) (DISTANCE_PAIR{static_cast<DistanceSize>(x), static_cast<DistanceSize>(y)})
static constexpr std::array<DISTANCE_PAIR, NUM_DIRECTIONS> POINTS_OUTER{
  D_PTS(0.5, 1.0),
  // north-northeast is closest to point (0.5 + 0.207, 1.0)
  D_PTS(P_0_5, 1.0),
  // northeast is closest to point (1.0, 1.0)
  D_PTS(1.0, 1.0),
  // east-northeast is closest to point (1.0, 0.5 + 0.207)
  D_PTS(1.0, P_0_5),
  // east is closest to point (1.0, 0.5)
  D_PTS(1.0, 0.5),
  // east-southeast is closest to point (1.0, 0.5 - 0.207)
  D_PTS(1.0, M_0_5),
  // southeast is closest to point (1.0, 0.0)
  D_PTS(1.0, 0.0),
  // south-southeast is closest to point (0.5 + 0.207, 0.0)
  D_PTS(P_0_5, 0.0),
  // south is closest to point (0.5, 0.0)
  D_PTS(0.5, 0.0),
  // south-southwest is closest to point (0.5 - 0.207, 0.0)
  D_PTS(M_0_5, 0.0),
  // southwest is closest to point (0.0, 0.0)
  D_PTS(0.0, 0.0),
  // west-southwest is closest to point (0.0, 0.5 - 0.207)
  D_PTS(0.0, M_0_5),
  // west is closest to point (0.0, 0.5)
  D_PTS(0.0, 0.5),
  // west-northwest is closest to point (0.0, 0.5 + 0.207)
  D_PTS(0.0, P_0_5),
  // northwest is closest to point (0.0, 1.0)
  D_PTS(0.0, 1.0),
  // north-northwest is closest to point (0.5 - 0.207, 1.0)
  D_PTS(M_0_5, 1.0)
};

constexpr inline auto
dist_line(
  const auto& a,
  const auto& b
)
{
  return ((a - b) * (a - b));
}

static constexpr InnerPos
to_inner(
  const XYSize x,
  const XYSize y
)
{
  XYSize integral;
  const auto x0 = static_cast<DistanceSize>(modf(x, &integral));
  const auto y0 = static_cast<DistanceSize>(modf(y, &integral));
  return {x0, y0};
}

Pts::Pts(
  const bool is_unburnable,
  const XYPos p
)
{
  cell_x_y_ = {static_cast<Idx>(p.x()), static_cast<Idx>(p.y())};
  // HACK: assign value of first item if burnable
  if (!is_unburnable)
  {
    const auto p0 = to_inner(p.x(), p.y());
    const auto& x0 = p0.x();
    const auto& y0 = p0.y();
    for (size_t i = 0; i < NUM_DIRECTIONS; ++i)
    {
      const auto& p1 = POINTS_OUTER[i];
      const auto& x1 = p1.first;
      const auto& y1 = p1.second;
      const auto d = dist_line(x0, x1) + dist_line(y0, y1);
      auto& p_d = distances_[i];
      auto& p_p = points_[i];
      p_p = p0;
      p_d = (d < p_d) ? d : p_d;
    }
  }
}

Pts&
Pts::insert(
  const XYSize x,
  const XYSize y
)
{
  if (canBurn())
  {
    // need to calculate distances, but we know everything is the same point
    const InnerPos p0 = to_inner(x, y);
    for (size_t i = 0; i < NUM_DIRECTIONS; ++i)
    {
      const auto& p1 = POINTS_OUTER[i];
      const auto& x1 = p1.first;
      const auto& y1 = p1.second;
      const auto d = dist_line(p0.x(), x1) + dist_line(p0.y(), y1);
      auto& p_d = distances_[i];
      auto& p_p = points_[i];
      p_p = (d < p_d) ? p0 : p_p;
      p_d = (d < p_d) ? d : p_d;
    }
  }
  return *this;
}

inline bool
Pts::canBurn() const
{
  return (INVALID_DISTANCE != distances()[0]);
}

bool
Pts::empty() const
{
  return !canBurn();
}

void
PtMap::insert(
  const bool is_unburnable,
  const XYPos p0
)
{
  auto p = map_.try_emplace(p0.hash(), is_unburnable, p0);
  auto& pts = p.first->second;
  if (!p.second)
  {
    pts.insert(p0);
  }
}

set<XYPos>
Pts::unique() const noexcept
{
  set<XYPos> r{};
  Location loc{cell_x_y_.hash()};
  // if any point is invalid then they all have to be
  if (canBurn())
  {
    const auto& pts = points();
    for (size_t i = 0; i < NUM_DIRECTIONS; ++i)
    {
      const auto& p = pts[i];
      r.insert({cell_x_y_.x(), cell_x_y_.y(), p.x(), p.y()});
    }
  }
  return r;
}

set<XYPos>
PtMap::unique(
  const HashSize hash_value
) const noexcept
{
  set<XYPos> r{};
  for (auto& kv : map_)
  {
    if (kv.first == hash_value)
    {
      auto& pts = kv.second;
      auto u = pts.unique();
      r.insert(u.begin(), u.end());
    }
  }
  return r;
}

set<XYPos>
PtMap::unique() const noexcept
{
  set<XYPos> r{};
  for (auto& kv : map_)
  {
    auto& pts = kv.second;
    auto u = pts.unique();
    r.insert(u.begin(), u.end());
  }
  return r;
}

set<HashSize>
PtMap::keys() const noexcept
{
  auto k = std::views::keys(map_);
  return {k.begin(), k.end()};
}

size_t
PtMap::size() const noexcept
{
  return unique().size();
}

size_t
PtMap::erase(
  const HashSize hash_value
) noexcept
{
  return map_.erase(hash_value);
}
}
