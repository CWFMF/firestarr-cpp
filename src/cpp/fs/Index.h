/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_INDEX_H
#define FS_INDEX_H
#include "stdafx.h"
namespace fs
{
/**
 * \brief A wrapper around a MathSize to ensure correct types are used.
 * \tparam T The derived class that this Index represents.
 */
template <class T>
class Index
{
public:
  /**
   * \brief Value represented by this
   */
  MathSize value{0};
  ~Index() = default;
  /**
   * \brief Construct with a value of 0
   */
  constexpr Index() noexcept = default;
  /**
   * \brief Construct with given value
   * \param value Value to assign
   */
  constexpr explicit Index(const MathSize value) noexcept : value(value) { }
  constexpr Index(Index<T>&& rhs) noexcept = default;
  constexpr Index(const Index<T>& rhs) noexcept = default;
  Index<T>& operator=(Index<T>&& rhs) noexcept = default;
  Index<T>& operator=(const Index<T>& rhs) noexcept = default;
  auto operator<=>(const Index<T>& rhs) const = default;
  /**
   * \brief Equality operator
   * \param rhs Index to compare to
   * \return Whether or not these are equivalent
   */
  [[nodiscard]] constexpr bool operator==(const Index<T>& rhs) const noexcept
  {
    return value == rhs.value;
  }
  /**
   * \brief Not equals operator
   * \param rhs Index to compare to
   * \return Whether or not these are not equivalent
   */
  [[nodiscard]] constexpr bool operator!=(const Index<T>& rhs) const noexcept
  {
    return !(*this == rhs);
  }
  /**
   * \brief Less than operator
   * \param rhs Index to compare to
   * \return Whether or not this is less than the provided Index
   */
  constexpr bool operator<(const Index<T> rhs) const noexcept { return value < rhs.value; }
  /**
   * \brief Greater than operator
   * \param rhs Index to compare to
   * \return Whether or not this is greater than the provided Index
   */
  [[nodiscard]] constexpr bool operator>(const Index<T> rhs) const noexcept
  {
    return value > rhs.value;
  }
  /**
   * \brief Less than or equal to operator
   * \param rhs Index to compare to
   * \return Whether or not this is less than or equal to the provided Index
   */
  [[nodiscard]] constexpr bool operator<=(const Index<T> rhs) const noexcept
  {
    return value <= rhs.value;
  }
  /**
   * \brief Greater than or equal to operator
   * \param rhs Index to compare to
   * \return Whether or not this is greater than or equal to the provided Index
   */
  [[nodiscard]] constexpr bool operator>=(const Index<T> rhs) const noexcept
  {
    return value >= rhs.value;
  }
  /**
   * \brief Addition operator
   * \param rhs Index to add value from
   * \return The value of this plus the value of the provided index
   */
  [[nodiscard]] constexpr Index<T> operator+(const Index<T> rhs) const noexcept
  {
    return Index<T>(value + rhs.value);
  }
  /**
   * \brief Subtraction operator
   * \param rhs Index to add value from
   * \return The value of this minus the value of the provided index
   */
  [[nodiscard]] constexpr Index<T> operator-(const Index<T> rhs) const noexcept
  {
    return Index<T>(value - rhs.value);
  }
  /**
   * \brief Addition assignment operator
   * \param rhs Index to add value from
   * \return This, plus the value of the provided Index
   */
  constexpr Index<T>& operator+=(const Index<T> rhs) noexcept
  {
    value += rhs.value;
    return *this;
  }
  /**
   * \brief Subtraction assignment operator
   * \param rhs Index to add value from
   * \return This, minus the value of the provided Index
   */
  constexpr Index<T>& operator-=(const Index<T> rhs) noexcept
  {
    value -= rhs.value;
    return *this;
  }
};
}
#endif
