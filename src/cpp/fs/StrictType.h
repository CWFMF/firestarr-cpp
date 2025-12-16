/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_INDEX_H
#define FS_INDEX_H
#include "stdafx.h"
#include <type_traits>
namespace fs
{
/**
 * \brief A wrapper around a ValueType to ensure correct types are used.
 */
template <class ConcreteType, class ValueType = MathSize>
struct StrictType
{
  using T = StrictType<ConcreteType, ValueType>;
  ValueType value{0.0};
  constexpr StrictType() = default;
  constexpr explicit StrictType(const ValueType value) noexcept : value(value)
  {
    // HACK: ensure we're always using
    //       struct X : public StrictType<X>
    static_assert(
      std::is_same_v<decltype(*this), StrictType<ConcreteType, ValueType>&>, "Different types"
    );
    static_assert(std::is_same_v<decltype(*this), T&>, "Different types");
    static_assert(std::is_same_v<T, StrictType<ConcreteType, ValueType>>, "Different types");
    static_assert(std::is_convertible_v<ConcreteType, T>, "Different types");
    // // doesn't work in this direction
    // static_assert(std::is_convertible_v<T, ConcreteType>, "Different types");
  }
  constexpr StrictType(T&& rhs) noexcept = default;
  constexpr StrictType(const T& rhs) noexcept = default;
  T& operator=(T&& rhs) noexcept = default;
  T& operator=(const T& rhs) noexcept = default;
  auto operator<=>(const T& rhs) const = default;
  [[nodiscard]] constexpr T operator+(const T& rhs) const noexcept { return {value + rhs.value}; }
  [[nodiscard]] constexpr T operator-(const T& rhs) const noexcept { return {value - rhs.value}; }
  constexpr T& operator+=(const T& rhs) noexcept
  {
    value += rhs.value;
    return *this;
  }
  constexpr T& operator-=(const T& rhs) noexcept
  {
    value -= rhs.value;
    return *this;
  }
};
}
#endif
