/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include "CellPoints.h"
#include "FireSpread.h"
#ifdef DEBUG_THREADS
#include "Log.h"
#include "Statistics.h"
#endif
namespace fs
{
static CellPointsMap spread_points(
  const CellPoints& cell_pts,
  const OffsetSet& offsets_after_duration,
  const DurationSize arrival_time
) noexcept
{
  CellPointsMap r1{};
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
        r1, pt, SpreadData{arrival_time, r.intensity, r.ros, r.raz, Direction{Degrees{dir}}}, pt_new
      );
    }
    ++it_pt_dirs;
  }
  return r1;
}
}
