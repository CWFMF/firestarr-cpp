/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_TYPES_H
#define FS_TYPES_H
#include "stdafx.h"
#include "StrictType.h"
namespace fs
{
/**
 * \brief Ratio of degrees to radians
 */
static constexpr auto M_RADIANS_TO_DEGREES = 180.0 / std::numbers::pi;
struct Radians;
struct Degrees : public StrictType<Degrees, units::CompassDegrees>
{
  using StrictType::StrictType;
  explicit constexpr Degrees(const DirectionSize degrees) noexcept
    : Degrees{static_cast<MathSize>(degrees)}
  { }
};
struct Radians : public StrictType<Radians, units::CompassRadians>
{
  using StrictType::StrictType;
  static consteval Radians Pi() { return Radians{std::numbers::pi}; };
  static consteval Radians PiX2() { return Radians{2 * std::numbers::pi}; };
  static consteval Radians D_360() { return Radians{Degrees{static_cast<AspectSize>(360)}}; };
  static consteval Radians D_270() { return Radians{Degrees{static_cast<AspectSize>(270)}}; };
  static consteval Radians D_180() { return Radians{Degrees{static_cast<AspectSize>(180)}}; };
  static consteval Radians D_090() { return Radians{Degrees{static_cast<AspectSize>(90)}}; };
  explicit constexpr Radians(const Degrees& degrees) noexcept
    : Radians{degrees.value / M_RADIANS_TO_DEGREES}
  { }
  explicit constexpr Radians(const AspectSize aspect) noexcept : Radians{Degrees{aspect}} { }
  [[nodiscard]] constexpr Degrees asDegrees() const
  {
    return Degrees{value * M_RADIANS_TO_DEGREES};
  }
  [[nodiscard]] static constexpr Radians fix(const Radians& radians)
  {
    if (radians > PiX2())
    {
      return radians - PiX2();
    }
    if (radians < Radians::Zero())
    {
      return radians + PiX2();
    }
    return radians;
  }
  /**
   * \brief Ensure that value lies between 0 and 2 * PI
   * \param theta value to ensure is within bounds
   * \return value within range of (0, 2 * PI]
   */
  [[nodiscard]] constexpr Radians fix() const { return Radians::fix(*this); }
  /**
   * \brief Convert Bearing to Heading (opposite angle)
   * \param azimuth Bearing
   * \return Heading
   */
  [[nodiscard]] constexpr Radians to_heading() const { return (*this + D_180()).fix(); }
  [[nodiscard]] static constexpr Radians from_degrees(const MathSize degrees)
  {
    return Radians{Degrees{degrees}};
  }
  [[nodiscard]] static constexpr Radians from_aspect(const MathSize aspect)
  {
    return Radians{static_cast<AspectSize>(aspect)};
  }
};
static constexpr Degrees INVALID_DIRECTION{std::numeric_limits<DirectionSize>::max()};
static constexpr Radians abs(const Radians& radians) { return Radians{radians.value}; };
static inline MathSize tan(const Radians& radians) { return std::tan(radians.value); };
static inline MathSize sin(const Radians& radians) { return fs::sin(radians.value); };
static inline MathSize cos(const Radians& radians) { return fs::cos(radians.value); };
static constexpr Degrees abs(const Degrees& degrees) { return Degrees{degrees.value}; };
static inline MathSize tan(const Degrees& degrees) { return tan(Radians{degrees}); };
static inline MathSize sin(const Degrees& degrees) { return sin(Radians{degrees}); };
static inline MathSize cos(const Degrees& degrees) { return cos(Radians{degrees}); };
}
#endif
