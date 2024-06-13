/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "Cell.h"
#include "Location.h"
#include "MergeIterator.h"

namespace fs
{
const merged_map_type
apply_offsets_location(
  const Location& location,
  const double duration,
  const OffsetSet& pts,
  const OffsetSet& offsets
) noexcept
{
  merged_map_type result{};
  // apply offsets to point
  for (const auto& out : offsets)
  {
    const double x_o = duration * out.x();
    const double y_o = duration * out.y();
    for (const auto& p : pts)
    {
      // putting results in copy of offsets and returning that
      // at the end of everything, we're just adding something to every double in the set by
      // duration?
      const double x = x_o + p.x();
      const double y = y_o + p.y();
      // try to insert a pair with no direction and no points
      auto e = result.try_emplace(
        Location{static_cast<Idx>(y), static_cast<Idx>(x)},
        fs::DIRECTION_NONE,
        NULL
      );
      auto& pair = e.first->second;
      // always add point since we're calling try_emplace with empty list
      pair.second.emplace_back(x, y);
      if (e.second)
      {
        // was inserted so calculate source
        pair.first = relativeIndex(location, e.first->first);
      }
    }
  }
  return static_cast<const merged_map_type>(result);
}

const merged_map_type
apply_offsets_spreadkey(
  const double duration,
  const OffsetSet& offsets,
  const vector<CellPts>& cell_pts
)
{
  return merge_reduce_maps(
    cell_pts,
    [&duration, &offsets](const tuple<Cell, OffsetSet>& pts_for_cell) -> const merged_map_type {
      const Location& location = std::get<0>(pts_for_cell);
      const OffsetSet& pts = std::get<1>(pts_for_cell);
      return apply_offsets_location(location, duration, pts, offsets);
    }
  );
}
}
