/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "MergeIterator.h"

namespace tbd::sim
{
const merged_map_type::mapped_type merge_cell_data(
  const merged_map_type::mapped_type& lhs,
  const merged_map_type::mapped_type& rhs)
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

template <typename M, class F>
const M merge_maps_generic(
  const M& lhs,
  const M& rhs,
  F f)
{
  using const_iterator = typename M::const_iterator;
  using value_type = typename M::value_type;
  using key_type = typename M::key_type;
  using mapped_type = typename M::mapped_type;
  std::function<mapped_type(const mapped_type&, const mapped_type&)> fct_merge = f;
  M out{};
  const_iterator it_lhs = lhs.begin();
  const_iterator it_rhs = rhs.begin();
  const_iterator end_lhs = lhs.end();
  const_iterator end_rhs = rhs.end();
  while (true)
  {
    if (end_lhs == it_lhs)
    {
      while (end_rhs != it_rhs)
      {
        out.emplace(*it_rhs);
        ++it_rhs;
      }
      break;
    }
    if (end_rhs == it_rhs)
    {
      while (end_lhs != it_lhs)
      {
        out.emplace(*it_lhs);
        ++it_lhs;
      }
      break;
    }
    // at end of neither so pick lower value
    const value_type& pair0 = *it_lhs;
    const value_type& pair1 = *it_rhs;
    const key_type& k0 = pair0.first;
    const key_type& k1 = pair1.first;
    const mapped_type& m0 = pair0.second;
    const mapped_type& m1 = pair1.second;
    if (k0 < k1)
    {
      out.emplace(pair0);
      ++it_lhs;
    }
    else if (k0 > k1)
    {
      out.emplace(pair1);
      ++it_rhs;
    }
    else
    {
      assert(k0 == k1);
      const mapped_type merged = fct_merge(m0, m1);
      out.emplace(pair<const key_type, const mapped_type>(k0, merged));
      ++it_lhs;
      ++it_rhs;
    }
  }
  return out;
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
