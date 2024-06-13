/* Copyright (c) Queen's Printer for Ontario, 2020. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "Location.h"
#include "MergeIterator.h"
#include "Cell.h"
namespace tbd
{
const merged_map_type apply_offsets_spreadkey(
  const double duration,
  const OffsetSet& offsets,
  const vector<CellPts>& cell_pts)
{
  // NOTE: really tried to do this in parallel, but not enough points
  // in a cell for it to work well
  merged_map_type result{};
  map<pair<Location, Location>, CellIndex> derived_keys{};
  // apply offsets to point
  for (const auto& out : offsets)
  {
    const double x_o = duration * out.x();
    const double y_o = duration * out.y();
    for (const auto& pts_for_cell : cell_pts)
    {
      const Location& src = std::get<0>(pts_for_cell);
      const OffsetSet& pts = std::get<1>(pts_for_cell);
      for (const auto& p : pts)
      {
        // putting results in copy of offsets and returning that
        // at the end of everything, we're just adding something to every double in the set by duration?
        const double x = x_o + p.x();
        const double y = y_o + p.y();
        // don't need cell attributes, just location
        //   Location dst = Location(
        //     static_cast<Idx>(y),
        //     static_cast<Idx>(x));
        // try to insert a pair with no direction and no points
        auto e = result.try_emplace(
          Location{
            static_cast<Idx>(y),
            static_cast<Idx>(x)},
          tbd::topo::DIRECTION_NONE,
          NULL);
        auto& pair1 = e.first->second;
        // always add point since we're calling try_emplace with empty list
        pair1.second.emplace_back(x, y);
        const Location& dst = e.first->first;
        if (src != dst)
        {
          // no point in doing this if we didn't leave the cell
          // // if (e.second)
          // // {
          // //   // was inserted so calculate source
          // //   pair1.first = relativeIndex(
          // //     src,
          // //     dst);
          // // }
          auto e_s = derived_keys.try_emplace(
            pair<Location, Location>(src, dst),
            tbd::topo::DIRECTION_NONE);
          if (e_s.second)
          {
            // we inserted a pair of (src, dst), which means we've never
            // calculated the relativeIndex for this so add it to main map
            pair1.first |= relativeIndex(
              src,
              dst);
            // I guess you could put it in the derived_keys too, but that
            // doesn't get used
          }
        }
      }
    }
  }
  return static_cast<const merged_map_type>(result);
}
}