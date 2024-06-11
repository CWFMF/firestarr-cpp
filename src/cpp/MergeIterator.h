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

const merged_map_type merge_iterators(
  const auto& lhs,
  const auto& rhs)
{
  merged_map_type result{};
  auto it_lhs = lhs.cbegin();
  auto it_rhs = rhs.cbegin();
  // any benefit to storing instead of calling every loop?
  const auto& end_lhs = lhs.end();
  const auto& end_rhs = rhs.end();
  // use defines so we don't need to keep assigning variables
#define k0 (it_lhs->first)
#define k1 (it_rhs->first)
#define v0 (it_lhs->second)
#define v1 (it_rhs->second)
#define is_done_0 (end_lhs == it_lhs)
#define is_done_1 (end_rhs == it_rhs)
  auto add_pair_direct = [&result](source_pair& pair_out, const source_pair& from_pair) {
    CellIndex& s_out = pair_out.first;
    vector<InnerPos>& pts_out = pair_out.second;
    const CellIndex& s_from = from_pair.first;
    const vector<InnerPos>& pts_from = from_pair.second;
    s_out |= s_from;
    pts_out.insert(pts_out.end(), pts_from.begin(), pts_from.end());
  };

  auto add_pair = [&add_pair_direct, &result](const Location key, const source_pair& from_pair) {
    add_pair_direct(result[key], from_pair);
  };
  auto add_lhs = [&add_pair, &result, &it_lhs, &it_rhs]() {
    assert(k0 < k1);
    add_pair(k0, v0);
    ++it_lhs;
  };
  auto add_rhs = [&add_pair, &result, &it_lhs, &it_rhs]() {
    assert(k0 > k1);
    add_pair(k1, v1);
    ++it_rhs;
  };
  auto add_both = [&add_pair_direct, &result, &it_lhs, &it_rhs]() {
    assert(k1 == k0);
    source_pair& pair_out = result[k1];
    // just duplicate for now
    add_pair_direct(pair_out, v0);
    add_pair_direct(pair_out, v1);
  };
  while (true)
  {
    if (is_done_0)
    {
      while (!is_done_1)
      {
        add_rhs();
      }
      break;
    }
    if (is_done_1)
    {
      while (!is_done_0)
      {
        add_lhs();
      }
      break;
    }
    // neither is done so add lower value
    if (k0 < k1)
    {
      add_lhs();
    }
    else if (k0 == k1)
    {
      add_both();
    }
    else
    {
      assert(k0 > k1);
      add_rhs();
    }
  }
#undef k0
#undef k1
#undef v0
#undef v1
#undef is_done_0
#undef is_done_1
  return static_cast<const merged_map_type>(result);
}
}
