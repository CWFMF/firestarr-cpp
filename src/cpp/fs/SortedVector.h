/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_SORTEDVECTOR_H
#define FS_SORTEDVECTOR_H
#include "stdafx.h"
namespace fs
{
template <class T>
vector<T> merge(const vector<T>& lhs, const vector<T>& rhs) noexcept
{
  vector<T> out{};
  // maximum size is if no overlap
  out.reserve(lhs.size() + rhs.size());
  auto it_lhs = lhs.begin();
  auto it_rhs = rhs.begin();
  auto end_lhs = lhs.end();
  auto end_rhs = rhs.end();
  while (true)
  {
    if (end_lhs == it_lhs)
    {
      while (end_rhs != it_rhs)
      {
        out.emplace_back(*it_rhs);
        ++it_rhs;
      }
      break;
    }
    if (end_rhs == it_rhs)
    {
      while (end_lhs != it_lhs)
      {
        out.emplace_back(*it_lhs);
        ++it_lhs;
      }
      break;
    }
    // at end of neither so pick lower value
    const T& v0 = *it_lhs;
    const T& v1 = *it_rhs;
    if (v0 < v1)
    {
      out.emplace_back(v0);
      ++it_lhs;
    }
    else if (v0 > v1)
    {
      out.emplace_back(v1);
      ++it_rhs;
    }
    else
    {
      out.emplace_back(v0);
      ++it_lhs;
      ++it_rhs;
    }
  }
}
template <class T>
class SortedVector
{
  std::vector<T> values_{};

public:
  ~SortedVector() = default;
  /**
   * \brief Construct empty SortedVector
   */
  SortedVector() = default;
  SortedVector(const SortedVector& rhs);
  SortedVector(SortedVector&& rhs) noexcept;
  SortedVector& operator=(const SortedVector& rhs) noexcept;
  SortedVector& operator=(SortedVector&& rhs) noexcept;
  auto insert(const T& v) noexcept
  {
    auto it = std::lower_bound(values_.begin(), values_.end(), v);
    return values_.insert(it, v);
  }
  auto contains(const T& v) const noexcept
  {
    auto it = std::lower_bound(values_.begin(), values_.end(), v);
    return it != values_.end();
  }
  [[nodiscard]] size_t size() const noexcept { return values_.size(); }
  void merge(const SortedVector<T>& rhs) noexcept { values_ = merge(values_, rhs.values_); }
};
template <class K, class V>
class LinearMap
{
  using value_type = std::pair<K, V>;
  // FIX: duplicate SortedVector for now
  std::vector<value_type> values_{};

public:
  static auto compare_function(const value_type& kv0, const value_type& kv1)
  {
    return kv0.first <=> kv1.first;
  };
  ~LinearMap() = default;
  /**
   * \brief Construct empty LinearMap
   */
  LinearMap() = default;
  LinearMap(const LinearMap& rhs);
  LinearMap(LinearMap&& rhs) noexcept;
  LinearMap& operator=(const LinearMap& rhs) noexcept;
  LinearMap& operator=(LinearMap&& rhs) noexcept;
  template <class... Args>
  auto try_emplace(
    const K& k1,
    std::function<void(const K& k0, V& v0, V& v1)> fct_merge,
    Args... args
  ) noexcept
  {
    // find lower bound based on key only
    auto it = std::lower_bound(values_.begin(), values_.end(), compare_function);
    if (values_.end() != it)
    {
      auto& [k0, v0] = *it;
      if (k0 == k1)
      {
        // if keys match then merge values and keep key
        fct_merge(k0, v0, V{args...});
        // key existed
        return std::pair{it, true};
      }
      // keys don't match
    }
    // insert at key position
    it = values_.insert(it, V{args...});
    // key did not exist
    return std::pair{it, false};
  }
  auto insert(
    const value_type& kv,
    std::function<void(const K& k0, V& v0, V& v1)> fct_merge
  ) noexcept
  {
    // find lower bound based on key only
    auto it = std::lower_bound(values_.begin(), values_.end(), compare_function);
    if (values_.end() != it)
    {
      auto& [k0, v0] = *it;
      auto& [k1, v1] = kv;
      if (k0 == k1)
      {
        // if keys match then merge values and keep key
        fct_merge(k0, v0, v1);
        return it;
      }
      // keys don't match
    }
    // insert at key position
    return values_.insert(it, kv);
  }
  auto contains(const value_type& v) const noexcept
  {
    auto it = std::lower_bound(values_.begin(), values_.end(), compare_function);
    return it != values_.end();
  }
  [[nodiscard]] size_t size() const noexcept { return values_.size(); }
  // void merge(const LinearMap<K, V>& rhs) noexcept { values_ = merge(values_, rhs.values_); }
};
}
#endif
