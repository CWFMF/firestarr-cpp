/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_INDEX_H
#define FS_INDEX_H
#include "stdafx.h"
namespace fs
{
namespace units
{
template <size_t N>
struct UnitType
{
  consteval explicit UnitType(const char (&str)[N]) { std::copy_n(str, N, value); }
  char value[N];
  auto operator<=>(const UnitType&) const = default;
  bool operator==(const UnitType&) const = default;
};
static constexpr UnitType Unitless{"unitless"};
static constexpr UnitType Celsius{"degrees Celsius"};
static constexpr UnitType Percent{"percent"};
static constexpr UnitType KilometresPerHour{"km/h"};
static constexpr UnitType CompassDegrees{"degrees"};
static constexpr UnitType CompassRadians{"radians"};
static constexpr UnitType MillimetresAccumulated{"mm accumulated"};
}
/**
 * \brief A wrapper around a ValueType to ensure correct types are used.
 */
template <
  class ConcreteType,
  units::UnitType U = units::Unitless,
  class ValueType = MathSize,
  int InvalidValue = -1>
struct StrictType
{
  using T = StrictType<ConcreteType, U, ValueType, InvalidValue>;
  static constexpr auto Units = U;
  static consteval ConcreteType Zero() { return ConcreteType{0.0}; };
  static consteval ConcreteType Invalid()
  {
    return ConcreteType{static_cast<ValueType>(InvalidValue)};
  };
  ValueType value{0.0};
  constexpr StrictType() = default;
  constexpr explicit StrictType(const ValueType value) noexcept : value(value)
  {
    // HACK: ensure we're always using
    //       struct X : public StrictType<X>
    static_assert(
      std::is_same_v<decltype(*this), StrictType<ConcreteType, U, ValueType, InvalidValue>&>,
      "Different types"
    );
    static_assert(std::is_same_v<decltype(*this), T&>, "Different types");
    static_assert(
      std::is_same_v<T, StrictType<ConcreteType, U, ValueType, InvalidValue>>, "Different types"
    );
    static_assert(std::is_convertible_v<ConcreteType, T>, "Different types");
    // // doesn't work in this direction
    // static_assert(std::is_convertible_v<T, ConcreteType>, "Different types");
  }
  constexpr StrictType(T&& rhs) noexcept = default;
  constexpr StrictType(const T& rhs) noexcept = default;
  T& operator=(T&& rhs) noexcept = default;
  T& operator=(const T& rhs) noexcept = default;
  auto operator<=>(const T& rhs) const = default;
  [[nodiscard]] constexpr ConcreteType operator+(const T& rhs) const noexcept
  {
    return ConcreteType{value + rhs.value};
  }
  [[nodiscard]] constexpr ConcreteType operator-(const T& rhs) const noexcept
  {
    return ConcreteType{value - rhs.value};
  }
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
