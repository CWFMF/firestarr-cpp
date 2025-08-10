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
static constexpr std::pair<DistanceSize, DistanceSize> to_inner(const XYSize x, const XYSize y)
{
  XYSize integral;
  const auto x0 = static_cast<DistanceSize>(modf(x, &integral));
  const auto y0 = static_cast<DistanceSize>(modf(y, &integral));
  return {x0, y0};
}
CellPoints::CellPoints(const IntensityMap& intensity_map, const XYPos& p0)
  : cellpts_(
    intensity_map.cannotSpread(p0.x(), p0.y())
      ? nullptr
      : make_unique<array_cellpts>())
{
  if (nullptr != cellpts_)
  {
    auto& pts = points();
    auto& dists = distances();
    auto [x1, y1] = to_inner(p0.x(), p0.y());
    std::fill(pts.begin(), pts.end(), p0);
    for (size_t i = 0; i < NUM_DIRECTIONS; ++i)
    {
      const auto& p2 = POINTS_OUTER[i];
      const auto& x2 = p2.first;
      const auto& y2 = p2.second;
      dists[i] = dist_line(x1, x2) + dist_line(y1, y2);
    }
  }
}
array_dists& CellPoints::distances()
{
  // NOTE: protected, since unsafe if we don't know this isn't unburnable
  return std::get<0>(*cellpts_);
}
array_pts& CellPoints::points()
{
  // NOTE: protected, since unsafe if we don't know this isn't unburnable
  return std::get<1>(*cellpts_);
}
const array_dists& CellPoints::distances() const
{
  return std::get<0>(*cellpts_);
}
const array_pts& CellPoints::points() const
{
  return std::get<1>(*cellpts_);
}
void CellPoints::insert(const XYPos& p0)
{
  if (isUnburnable())
  {
    return;
  }
  auto& pts = points();
  auto& dists = distances();
  const auto [x1, y1] = to_inner(p0.x(), p0.y());
  for (size_t i = 0; i < NUM_DIRECTIONS; ++i)
  {
    const auto& p2 = POINTS_OUTER[i];
    const auto& x2 = p2.first;
    const auto& y2 = p2.second;
    const auto d = dist_line(x1, x2) + dist_line(y1, y2);
    auto& p_d = dists[i];
    auto& p_p = pts[i];
    p_p = (d < p_d) ? p0 : p_p;
    p_d = (d < p_d) ? d : p_d;
  }
}
bool CellPoints::isUnburnable() const
{
  return nullptr == cellpts_;
}
set<XYPos> CellPoints::unique() const
{
  if (isUnburnable())
  {
    return {};
  }
  auto it = std::views::transform(
    points(),
    [](const auto& p0) {
      return XYPos{p0.x(), p0.y()};
    });
  return {it.begin(), it.end()};
  // array_pts pts{points().cbegin(), points().cend()};
  // std::sort(pts.begin(), pts.end());
  // auto end = std::unique(pts.begin(), pts.end());
  // std::set<XYPos> r = {pts.begin(), end};
  // return r;
}
void Points::insert(
  const IntensityMap& intensity_map,
  const XYPos& p0)
{
  // No need to look up value in intensity_map if emplace doesn't happen
  auto p = map_.try_emplace(
    Location::hashXY(p0.x(), p0.y()),
    intensity_map,
    p0);
  p.first->second.insert(p0);
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
        auto u = kv.second.unique();
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
      auto u = kv.second.unique();
      r.insert(u.begin(), u.end());
    }
  }
  return r;
}
size_t Points::size() const noexcept
{
  return unique().size();
}
}
