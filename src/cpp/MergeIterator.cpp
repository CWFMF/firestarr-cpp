/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "MergeIterator.h"

namespace tbd::sim
{
const merged_map_type::mapped_type merge_cell_data(
  const merged_map_type::mapped_type& lhs,
  const merged_map_type::mapped_type& rhs)
{
  using list_type = merged_map_type::mapped_type::second_type;
  merged_map_type::mapped_type pair_out{lhs};
  CellIndex& s_out = pair_out.first;
  list_type& pts_out = pair_out.second;
  const CellIndex& s_from = rhs.first;
  const list_type& pts_from = rhs.second;
  s_out |= s_from;
  pts_out.insert(pts_out.end(), pts_from.begin(), pts_from.end());
  // pts_out.insert(pts_from);
  return pair_out;
}

const merged_map_type merge_maps(
  const merged_map_type& lhs,
  const merged_map_type& rhs)
{
  return merge_maps_generic<merged_map_type>(
    lhs,
    rhs,
    merge_cell_data);
}
}
