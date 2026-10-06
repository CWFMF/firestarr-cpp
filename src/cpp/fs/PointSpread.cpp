/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include "PointSpread.h"
#include "FireSpread.h"
#include "Scenario.h"
namespace fs
{
DurationSize do_spread(
  MathSize& max_ros,
  CellPointsMap& points,
  SpreadCache& spread_info,
  ptr<const Scenario> scenario,
  const BurnedData& unburnable,
  const DurationSize time,
  const DurationSize max_duration
) noexcept
{
  // get once and keep
  static const auto& settings = fs::settings::instance();
  static const MathSize ros_min = settings.minimum_ros;
  spreading_points to_spread{};
  // if we use an iterator this way we don't need to copy keys to erase things
  auto& lhs = points.cells_;
  auto it_cells = lhs.begin();
  while (it_cells != lhs.end())
  {
    auto& [loc, pts] = *it_cells;
    const Cell for_cell = scenario->cell(loc);
    const auto key = for_cell.key();
    // any cell that has the same fuel, slope, and aspect has the same spread
    auto& origin = *spread_info.add_spread(key, scenario, time);
    // filter out things not spreading fast enough here so they get copied if they aren't
    // isNotSpreading() had better be true if ros is lower than minimum
    const auto ros = origin.headRos();
    if (ros >= ros_min)
    {
      max_ros = max(max_ros, ros);
      // NOTE: shouldn't be Cell if we're looking up by just Location later
      to_spread[key].emplace_back(std::move(*it_cells));
      it_cells = lhs.erase(it_cells);
#ifdef DEBUG_CELLPOINTS
      auto& v = to_spread[key];
      const auto n = v.size();
      const auto& p = v[n - 1].second;
      logging::note(
        "added {:d} items to to_spread[{:d}][({:d}, {:d})]", p.size(), key, loc.x(), loc.y()
      );
#endif
    }
    else
    {
      ++it_cells;
    }
  }
  // if nothing in to_spread then nothing is spreading
  if (to_spread.empty())
  {
    return -1;
  }
  const auto duration =
    ((max_ros > 0)
       ? min(max_duration, settings.maximum_spread_distance * scenario->cellSize() / max_ros)
       : max_duration);
  const auto new_time = time + duration / DAY_MINUTES;
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
      CellPointsMap cell_pts_cur{};
      for (auto& [location, cell_pts] : cell_pts)
      {
        if (cell_pts.empty())
        {
          continue;
        }
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
              cell_pts_cur,
              pt,
              SpreadData{new_time, r.intensity, r.ros, r.raz, Direction{Degrees{dir}}},
              pt_new
            );
          }
          ++it_pt_dirs;
        }
        // result.merge(unburnable, r1);
      }
      return cell_pts_cur;
    });
  auto it_spread = spread.begin();
  while (spread.end() != it_spread)
  {
    const CellPointsMap& cell_pts_cur = *it_spread;
    // // HACK: keep old behaviour until we can figure out whey removing isn't the same as not
    // adding const auto h = cell_pts.location().hash(); if (!unburnable[h])
    // {
    cell_pts.merge(unburnable, cell_pts_cur);
    ++it_spread;
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
  points.merge(unburnable, cell_pts);
  return new_time;
}
}
