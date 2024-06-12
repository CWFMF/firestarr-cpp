/* SPDX-FileCopyrightText: 2021-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef FS_MERGEITERATOR_H
#define FS_MERGEITERATOR_H

#include "stdafx.h"

#include "Cell.h"
#include "InnerPos.h"
#include "Location.h"

namespace fs
{

using source_pair = pair<CellIndex, vector<InnerPos>>;
using merged_map_type = map<Location, source_pair>;
using merged_map_pair = pair<Location, source_pair>;
using map_type = map<Location, vector<InnerPos>>;

const merged_map_type::mapped_type
merge_cell_data(const merged_map_type::mapped_type& lhs, const merged_map_type::mapped_type& rhs);

const merged_map_type
merge_maps(const merged_map_type& lhs, const merged_map_type& rhs);

template <class F>
const merged_map_type
merge_reduce_maps(
  const auto& points_and_sources,
  F fct_transform
)
{
  return std::transform_reduce(
    std::execution::par_unseq,
    points_and_sources.begin(),
    points_and_sources.end(),
    merged_map_type{},
    merge_maps,
    fct_transform
  );
}
}
#endif
