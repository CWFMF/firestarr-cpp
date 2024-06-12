/* SPDX-FileCopyrightText: 2021-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "MergeIterator.h"

namespace fs::sim
{
const merged_map_type::mapped_type
merge_cell_data(
  const merged_map_type::mapped_type& lhs,
  const merged_map_type::mapped_type& rhs
)
{
  merged_map_type::mapped_type pair_out{lhs};
  CellIndex& s_out = pair_out.first;
  vector<InnerPos>& pts_out = pair_out.second;
  const CellIndex& s_from = rhs.first;
  const vector<InnerPos>& pts_from = rhs.second;
  s_out |= s_from;
  pts_out.insert(pts_out.end(), pts_from.begin(), pts_from.end());
  return pair_out;
}

const merged_map_type
merge_maps(
  const merged_map_type& lhs,
  const merged_map_type& rhs
)
{
  return merge_maps_generic<merged_map_type>(lhs, rhs, merge_cell_data);
}
}
