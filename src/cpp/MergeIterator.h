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

// mangled version of std::transform_reduce() that calls .begin() and .end()
template <typename _ForwardIteratorSource, class _Tp, class _BinaryOperation, class _UnaryOperation>
_Tp do_transform_reduce(
  _ForwardIteratorSource&& container,
  _Tp __init,
  _BinaryOperation __binary_op,
  _UnaryOperation __unary_op)
{
  // to help compiler determine type
  using _ForwardIterator = decltype(container.begin());
  return std::transform_reduce(
    std::execution::par_unseq,
    static_cast<_ForwardIterator>(container.begin()),
    static_cast<_ForwardIterator>(container.end()),
    __init,
    __binary_op,
    __unary_op);
}

const merged_map_type::mapped_type merge_cell_data(
  const merged_map_type::mapped_type& lhs,
  const merged_map_type::mapped_type& rhs);

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
  const merged_map_type& rhs);

template <class F>
const merged_map_type merge_reduce_maps(
  const auto& points_and_sources,
  F fct_transform)
{
  return do_transform_reduce(
    points_and_sources,
    merged_map_type{},
    merge_maps,
    fct_transform);
}
}
