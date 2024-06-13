/* Copyright (c) Queen's Printer for Ontario, 2020. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "Location.h"
#include "MergeIterator.h"
namespace tbd
{
map<topo::Location, OffsetSet> apply_offsets(
  const double duration,
  const OffsetSet& pts,
  const OffsetSet& offsets) noexcept
{
  // apply offsets to point
  std::map<Location, OffsetSet> r{};
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
      r[Location(
          static_cast<Idx>(y),
          static_cast<Idx>(x))]
        .emplace_back(x, y);
    }
  }
  return r;
}
}
namespace tbd::sim
{
const merged_map_type apply_offsets_location(
  const Location& location,
  const double duration,
  const OffsetSet& pts,
  const OffsetSet& offsets) noexcept
{
  return merge_reduce_maps(
    apply_offsets(duration, pts, offsets),
    [&location](const map_type::value_type& kv) -> const merged_map_type {
      const Location k = kv.first;
      return {
        merged_map_type::value_type(
          k,
          merged_map_type::mapped_type(
            relativeIndex(location, k),
            kv.second))};
    });
}
}