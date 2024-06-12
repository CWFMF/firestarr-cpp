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

#include <iostream>
#include <iterator>

template <class _InputLeft, class _InputRight>
struct MergeIterator
{
  using difference_type = std::ptrdiff_t;
  using key_type = merged_map_type::key_type;
  using mapped_type = merged_map_type::mapped_type;
  using value_type = merged_map_type::value_type;
  using key_compare = merged_map_type::key_compare;
  using _InputIteratorLeft = _InputLeft::iterator;
  using _InputIteratorRight = _InputRight::iterator;
private:
  _InputIteratorLeft it_lhs_;
  _InputIteratorLeft end_lhs_;
  _InputIteratorRight it_rhs_;
  _InputIteratorRight end_rhs_;
  // static_assert(std::sentinel_for<decltype(sentinel), decltype(it_lhs_)>);
  shared_ptr<value_type> start_;
  shared_ptr<value_type> current_;
  shared_ptr<value_type> end_;
  shared_ptr<value_type> find_next()
  {
    // shared_ptr<source_pair> pair0 = make_shared<source_pair>(&(*it_lhs_));
    // shared_ptr<source_pair> pair1 = make_shared<source_pair>(&(*it_rhs_));
    // pointer to current item
    auto pair0 = shared_ptr<value_type>(&(*it_lhs_));
    auto pair1 = shared_ptr<value_type>(&(*it_rhs_));
    if ((end_lhs_ == it_lhs_))
    {
      ++it_rhs_;
      // use end_rhs_ as the 'end', but type is different if we just return that
      if (end_rhs_ == it_rhs_)
      {
        return end_;
      }
      return pair1;
    }
    // know we're not at end of lhs, so return from there
    if (end_rhs_ == it_rhs_)
    {
      ++it_lhs_;
      return pair0;
    }
    auto& k0 = (*pair0).first;
    auto& k1 = (*pair1).first;
    // neither is done so add lower value
    if (k0 < k1)
    {
      ++it_lhs_;
      return pair0;
    }
    else if (k0 == k1)
    {
      // this is a bit weird because we need to combine them
      auto pair_out = make_shared<value_type>(k0, pair0);
      mapped_type& value_out = pair_out->second;
      CellIndex& s_out = value_out.first;
      vector<InnerPos>& pts_out = value_out.second;
      const mapped_type& value_from = pair1->second;
      const CellIndex& s_from = value_from.first;
      const vector<InnerPos>& pts_from = value_from.second;
      s_out |= s_from;
      pts_out.insert(pts_out.end(), pts_from.begin(), pts_from.end());
      ++it_lhs_;
      ++it_rhs_;
      return pair_out;
    }
    else
    {
      assert(k0 > k1);
      // know it_rhs_ != end_rhs_ so no need to check if we need to use end_
      ++it_rhs_;
      return pair1;
    }
  }
public:
  ~MergeIterator()
  {
    // do we even need to do anything?
  }
  // Default constructor is required to pass forward_MergeIterator assertion
  MergeIterator()
  {
    throw std::runtime_error("Not implemented");
  }
  MergeIterator(
    _InputIteratorLeft it_lhs,
    _InputIteratorLeft end_lhs,
    _InputIteratorRight it_rhs,
    _InputIteratorRight end_rhs)
    : it_lhs_(it_lhs),
      end_lhs_(end_lhs),
      it_rhs_(it_rhs),
      end_rhs_(end_rhs)
  {
    __glibcxx_function_requires(_InputIteratorConcept<_InputIteratorLeft>);
    __glibcxx_function_requires(_InputIteratorConcept<_InputIteratorRight>);
    __glibcxx_function_requires(_EqualOpConcept<
                                merged_map_type::value_type,
                                typename iterator_traits<_InputIteratorLeft>::value_type>);
    __glibcxx_function_requires(_EqualOpConcept<
                                merged_map_type::value_type,
                                typename iterator_traits<_InputIteratorRight>::value_type>);
    __glibcxx_function_requires(_EqualOpConcept<
                                typename iterator_traits<_InputIterator>::value_type,
                                typename iterator_traits<_InputIteratorRight>::value_type>);
    current_ = find_next();
    start_ = current_;
    end_ = shared_ptr<value_type>(&(*end_rhs));
  }
  MergeIterator(_InputLeft lhs, _InputRight rhs)
    : MergeIterator(
        lhs.begin(),
        lhs.end(),
        rhs.begin(),
        rhs.end())
  {
  }
  const value_type& operator*() const
  {
    return *current_;
  }
  const value_type* operator->() const
  {
    return current_.get();
  }
  MergeIterator& operator++()
  {
    current_ = find_next();
    return *this;
  }
  MergeIterator operator++(int)
  {
    MergeIterator tmp = *this;
    ++(*this);
    return tmp;
  }
  auto operator<=>(const MergeIterator&) const = default;   // three-way comparison C++20
  auto begin()
  {
    return start_;
  }
  auto end()
  {
    return end_;
  }
  static_assert(std::input_iterator<MergeIterator>);
};

template <class _InputLeft, class _InputRight>
const merged_map_type merge_to_map(_InputLeft lhs, _InputRight rhs)
{
  merged_map_type r{};
  auto it = MergeIterator<_InputLeft, _InputRight>(lhs, rhs);
  for (const merged_map_type::value_type& kv : it)
  {
    r.emplace(kv);
  }
  return static_cast<const merged_map_type>(r);
}
const merged_map_type merge_list_of_maps(
  //   const vector<merged_map_type>& points_and_sources)
  const auto& points_and_sources)
{
  merged_map_type r{};
  merged_map_type r0{};
  for (const merged_map_type& m0 : points_and_sources)
  {
    auto m1 = MergeIterator<
      merged_map_type,
      merged_map_type>(r, m0);
    auto it_m = m1.begin();
    while (m1.end() != it_m)
    // for (const auto& kv : m_it)
    {
      const auto& kv = *it_m;
      r0.emplace(kv);
      ++it_m;
    }
    std::swap(r, r0);
    r0 = {};
  }
  return static_cast<const merged_map_type>(r);
  //   return merge_list_of_maps(
  //     points_and_sources.begin(),
  //     points_and_sources.end());
}
}
