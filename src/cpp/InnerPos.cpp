/* Copyright (c) Queen's Printer for Ontario, 2020. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "Location.h"
#include "MergeIterator.h"
namespace tbd
{
const merged_map_type apply_offsets_location(
  const Location& location,
  const double duration,
  const OffsetSet& pts,
  const OffsetSet& offsets) noexcept
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
      auto& pair = e.first->second;
      // always add point since we're calling try_emplace with empty list
      pair.second.emplace_back(x, y);
      if (e.second)
      {
        // was inserted so calculate source
        pair.first = relativeIndex(
          location,
          e.first->first);
      }
    }
  }
  return static_cast<const merged_map_type>(result);
}
}