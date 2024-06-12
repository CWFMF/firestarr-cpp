/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "Location.h"
#include "Cell.h"
#include "InnerPos.h"

namespace tbd::sim
{
using topo::Location;
using topo::SpreadKey;
using source_pair = pair<CellIndex, vector<InnerPos>>;
using merged_map_type = map<Location, source_pair>;
using merged_map_pair = pair<Location, source_pair>;
using map_type = map<Location, vector<InnerPos>>;

const merged_map_type::mapped_type merge_cell_data(
  const merged_map_type::mapped_type& lhs,
  const merged_map_type::mapped_type& rhs);

const merged_map_type merge_maps(
  const merged_map_type& lhs,
  const merged_map_type& rhs);

const merged_map_type merge_list_of_maps(
  //   const vector<merged_map_type>& points_and_sources)
  const auto& points_and_sources)
{
  return std::reduce(
    std::execution::par_unseq,
    points_and_sources.begin(),
    points_and_sources.end(),
    merged_map_type{},
    merge_maps);
}
}
