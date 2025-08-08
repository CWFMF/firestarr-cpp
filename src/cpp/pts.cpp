/* Copyright (c) Jordan Evens, 2005, 2021 */
/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "pts.h"
#include "Log.h"
#include "Location.h"
#include "Scenario.h"

namespace fs::sim
{
constexpr InnerSize DIST_22_5 = static_cast<InnerSize>(0.2071067811865475244008443621048490392848359376884740365883398689);
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
  D_PTS(M_0_5, 1.0)};

constexpr inline auto dist_line(const auto& a, const auto& b)
{
  return ((a - b) * (a - b));
}
static constexpr InnerPos to_inner(const XYSize x, const XYSize y)
{
  XYSize integral;
  const auto x0 = static_cast<DistanceSize>(modf(x, &integral));
  const auto y0 = static_cast<DistanceSize>(modf(y, &integral));
  return {x0, y0};
}
CellPoints::CellPoints(const bool is_unburnable, const XYSize x, const XYSize y)
{
  if (!is_unburnable)
  {
    points = make_unique<array_pts>();
    distances = make_unique<array_dists>();
    auto p1 = to_inner(x, y);
    std::fill(points->begin(), points->end(), p1);
    for (size_t i = 0; i < NUM_DIRECTIONS; ++i)
    {
      const auto& p2 = POINTS_OUTER[i];
      const auto& x2 = p2.first;
      const auto& y2 = p2.second;
      (*distances)[i] = dist_line(p1.x(), x2) + dist_line(p1.y(), y2);
    }
  }
}
void CellPoints::insert(const XYSize x, const XYSize y)
{
  if (isUnburnable())
  {
    return;
  }
  const InnerPos p1 = to_inner(x, y);
  for (size_t i = 0; i < NUM_DIRECTIONS; ++i)
  {
    const auto& p2 = POINTS_OUTER[i];
    const auto& x2 = p2.first;
    const auto& y2 = p2.second;
    const auto d = dist_line(p1.x(), x2) + dist_line(p1.y(), y2);
    auto& p_d = (*distances)[i];
    auto& p_p = (*points)[i];
    p_p = (d < p_d) ? p1 : p_p;
    p_d = (d < p_d) ? d : p_d;
  }
}
bool CellPoints::isUnburnable() const
{
  return nullptr == distances;
}
set<XYPos> CellPoints::unique(
  const HashSize hash_value) const
{
  if (isUnburnable())
  {
    return {};
  }
  const auto [x1, y1] = Location::unhashXY(hash_value);
  auto it = std::views::transform(
    *points,
    [x1, y1](const auto& p0) {
      return XYPos(p0.x() + x1, p0.y() + y1);
    });
  return {it.begin(), it.end()};
}
void Points::insert(
  const bool is_unburnable,
  const XYSize x,
  const XYSize y)
{
  // HACK: try to insert nullptr and if that works modify
  auto p = map_.try_emplace(
    Location::hashXY(x, y),
    is_unburnable,
    x,
    y);
  p.first->second.insert(x, y);
}
set<XYPos> Points::unique(const HashSize hash_value) const noexcept
{
  set<XYPos> r{};
  for (auto& kv : map_)
  {
    if (kv.first == hash_value)
    {
      if (!kv.second.isUnburnable())
      {
        auto u = kv.second.unique(kv.first);
        r.insert(u.begin(), u.end());
      }
    }
  }
  return r;
}
set<XYPos> Points::unique() const noexcept
{
  set<XYPos> r{};
  for (auto& kv : map_)
  {
    if (!kv.second.isUnburnable())
    {
      auto u = kv.second.unique(kv.first);
      r.insert(u.begin(), u.end());
    }
  }
  return r;
}
set<HashSize> Points::keys() const noexcept
{
  auto k = std::views::keys(map_);
  return {k.begin(), k.end()};
}
size_t Points::size() const noexcept
{
  return unique().size();
}
size_t Points::erase(const HashSize hash_value) noexcept
{
  return map_.erase(hash_value);
}
}
