/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_INNERPOS_H
#define FS_INNERPOS_H
#include "stdafx.h"
#include "Log.h"
namespace fs
{
/**
 * \brief Offset from a position
 */
struct Offset
{
public:
  /**
   * \brief Offset in the x direction (column)
   */
  inline constexpr double x() const noexcept { return x_; }
  /**
   * \brief Offset in the y direction (row)
   */
  inline constexpr double y() const noexcept { return y_; }
  constexpr Offset(const double a, const double b) noexcept : x_(a), y_(b) { }
  constexpr Offset() noexcept : Offset(-1, -1) { }
  constexpr Offset(Offset&& rhs) noexcept = default;
  constexpr Offset(const Offset& rhs) noexcept = default;
  Offset& operator=(const Offset& rhs) noexcept = default;
  Offset& operator=(Offset&& rhs) noexcept = default;
  /**
   * \brief Multiply by duration to get total offset over time
   * \param duration time to multiply by
   */
  constexpr Offset after(const double duration) const noexcept
  {
    return Offset(x() * duration, y() * duration);
  }
  /**
   * \brief Less than operator
   * \param rhs Offset to compare to
   * \return Whether or not this is less than the other
   */
  bool operator<(const Offset& rhs) const noexcept
  {
    if (x() == rhs.x())
    {
      if (y() == rhs.y())
      {
        // they are "identical" so this is false
        return false;
      }
      return y() < rhs.y();
    }
    return x() < rhs.x();
  }
  /**
   * \brief Equality operator
   * \param rhs Offset to compare to
   * \return Whether or not this is equivalent to the other
   */
  bool operator==(const Offset& rhs) const noexcept { return (x() == rhs.x()) && (y() == rhs.y()); }
  /**
   * \brief Add offset to position and return result
   */
  [[nodiscard]] constexpr Offset add(const Offset o) const noexcept
  {
    return Offset(x() + o.x(), y() + o.y());
  }

private:
  /**
   * \brief Offset in the x direction (column)
   */
  double x_;
  /**
   * \brief Offset in the y direction (row)
   */
  double y_;
};
// define multiplication in other order since equivalent
constexpr Offset after(const double duration, const Offset& o) { return o.after(duration); }
static constexpr MathSize x(const auto& p) { return p.x(); }
static constexpr MathSize y(const auto& p) { return p.y(); }
/**
 * \brief Collection of Offsets
 */
using OffsetSet = vector<Offset>;
/**
 * \brief The position within a Cell that a spreading point has.
 */
using InnerPos = Offset;
}
#endif
