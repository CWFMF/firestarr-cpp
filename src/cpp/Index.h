/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2021-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_INDEX_H
#define FS_INDEX_H
#include "stdafx.h"
#include <limits>
#include "unstable.h"
namespace fs
{
struct IndexValue
{
  MathSize value{0.0};
  auto operator<=>(const IndexValue& rhs) const = default;
};
/**
 * \brief A wrapper around a MathSize to ensure correct types are used.
 * \tparam T The derived class that this Index represents.
 */
template <class T>
struct Index : public IndexValue
{
  static consteval T Zero() { return {0.0}; }
  static consteval T Invalid() { return {std::numeric_limits<decltype(value)>::min()}; }
  using IndexValue::IndexValue;
  constexpr Index(const MathSize value) noexcept : IndexValue(value) { };
  auto operator<=>(const Index<T>& rhs) const = default;
  auto operator<=>(const T& rhs) const { return value <=> rhs.value; }
};
}
#endif
