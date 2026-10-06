/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include "CellPoints.h"
#include "FireSpread.h"
#include "SpreadCache.h"
namespace fs
{
using namespace fuel;
void spread_points(
  CellPointsMap& result,
  const CellPoints& cell_pts,
  const OffsetSet& offsets_after_duration,
  const DurationSize arrival_time
) noexcept
{
  // done with list so don't need mutex
  auto pt_dirs = cell_pts.point_directions();
  std::sort(pt_dirs.begin(), pt_dirs.end());
  const auto it_pt_dirs_last = std::unique(pt_dirs.begin(), pt_dirs.end());
  auto it_pt_dirs = pt_dirs.cbegin();
  while (it_pt_dirs != it_pt_dirs_last)
  {
    const auto& [pt, dir] = *it_pt_dirs;
    for (const ROSOffset& r : offsets_after_duration)
    {
      const auto& x_o = r.offset.x;
      const auto& y_o = r.offset.y;
      const XYPos pt_new{XPos{x_o + pt.x.value}, YPos{y_o + pt.y.value}};
      std::ignore = insert(
        result,
        pt,
        SpreadData{arrival_time, r.intensity, r.ros, r.raz, Direction{Degrees{dir}}},
        pt_new
      );
    }
    ++it_pt_dirs;
  }
}
CellPointsMap spread_map(
  const BurnedData& unburnable,
  const SpreadCache& spread_info,
  const spreading_points& to_spread,
  const DurationSize new_time,
  const DurationSize duration
) noexcept
{
  CellPointsMap cell_pts{};
  auto spread =
    std::views::transform(to_spread, [&](const spreading_points::value_type& kv0) -> CellPointsMap {
      const auto& key = kv0.first;
      const auto& offsets = spread_info.offsets(key);
      const auto& cell_pts = kv0.second;
      OffsetSet offsets_after_duration{};
      offsets_after_duration.resize(offsets.size());
      std::transform(
        offsets.cbegin(),
        offsets.cend(),
        offsets_after_duration.begin(),
        [&](const ROSOffset& r) {
          return ROSOffset{
            r.intensity, r.ros, r.raz, Offset{r.offset.x * duration, r.offset.y * duration}
          };
        }
      );
      auto r = [&]() {
        CellPointsMap result{};
        for (auto& [location, cell_pts] : cell_pts)
        {
          if (cell_pts.empty())
          {
            continue;
          }
          spread_points(result, cell_pts, offsets_after_duration, new_time);
          // result.merge(unburnable, r1);
        }
        return result;
      }();
      return r;
    });
  auto it = spread.begin();
  while (spread.end() != it)
  {
    const CellPointsMap& cell_pts_cur = *it;
    // // HACK: keep old behaviour until we can figure out whey removing isn't the same as not
    // adding const auto h = cell_pts.location().hash(); if (!unburnable[h])
    // {
    cell_pts.merge(unburnable, cell_pts_cur);
    ++it;
  }
#ifdef DEBUG_CELLPOINTS
  const auto n_c = cell_pts.size();
#endif
  cell_pts.remove_if([&](const CellPointsMap::map_value& kv) {
    auto& [location, pts] = kv;
    // clear out if unburnable
    const auto do_clear = unburnable.at(location);
    return do_clear;
  });
#ifdef DEBUG_CELLPOINTS
  logging::note("{:d} cell_pts before remove_if() and {:d} after", n_c, cell_pts.size());
#endif
  return cell_pts;
}
}
