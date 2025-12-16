/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_WEATHER_H
#define FS_WEATHER_H
#include "StrictType.h"
#include "unstable.h"
#include "Util.h"
namespace fs
{
/**
 * \brief Temperature in degrees Celsius.
 */
/**
 * \brief Temperature in degrees Celsius.
 */
struct Temperature : public StrictType<Temperature>
{
  using StrictType<Temperature>::StrictType;
};
/**
 * \brief Relative humidity as a percentage.
 */
struct RelativeHumidity : public StrictType<RelativeHumidity>
{
  using StrictType<RelativeHumidity>::StrictType;
};
/**
 * \brief Speed in kilometers per hour.
 */
struct Speed : public StrictType<Speed>
{
  using StrictType<Speed>::StrictType;
};
/**
 * \brief Direction with access to degrees or radians.
 */
struct Direction : public StrictType<Direction>
{
  using StrictType<Direction>::StrictType;
  static consteval Direction Zero() { return Direction{0.0}; };
  static consteval Direction Invalid() { return Direction{-1.0}; };
  constexpr Direction(const MathSize value = 0, const bool is_radians = false)
    : StrictType{is_radians ? to_degrees(value) : value}
  { }
  /**
   * \brief Direction as radians, where 0 is North and values increase clockwise
   * \return Direction as radians, where 0 is North and values increase clockwise
   */
  [[nodiscard]] constexpr MathSize asRadians() const { return to_radians(asDegrees()); }
  /**
   * \brief Direction as degrees, where 0 is North and values increase clockwise
   * \return Direction as degrees, where 0 is North and values increase clockwise
   */
  [[nodiscard]] constexpr MathSize asDegrees() const { return value; }
  /**
   * \brief Heading (opposite of this direction)
   * \return Heading (opposite of this direction)
   */
  [[nodiscard]] constexpr MathSize heading() const { return to_heading(asRadians()); }
};
/**
 * \brief Wind with a Speed and Direction.
 */
struct Wind
{
  Direction direction{};
  Speed speed{};
  static consteval Wind Zero() { return {Direction::Zero(), Speed::Zero()}; };
  static consteval Wind Invalid() { return {Direction::Invalid(), Speed::Invalid()}; };
  auto operator<=>(const Wind& rhs) const = default;
  /**
   * \brief Direction wind is going towards
   * \return Direction wind is going towards
   */
  [[nodiscard]] constexpr MathSize heading() const noexcept { return direction.heading(); }
  /**
   * \brief X component of wind vector (km/h)
   * \return X component of wind vector (km/h)
   */
  [[nodiscard]] constexpr MathSize wsvX() const noexcept
  {
    // HACK: rounding error due to assignment before adding in old tests so keep for now
    volatile auto v = speed.value * sin(direction.heading());
    return v;
  }
  /**
   * \brief Y component of wind vector (km/h)
   * \return Y component of wind vector (km/h)
   */
  [[nodiscard]] constexpr MathSize wsvY() const noexcept
  {
    // HACK: rounding error due to assignment before adding in old tests so keep for now
    volatile auto v = speed.value * cos(direction.heading());
    return v;
  }
};
/**
 * \brief Precipitation (1hr accumulation) (mm)
 */
struct Precipitation : public StrictType<Precipitation>
{
  using StrictType<Precipitation>::StrictType;
};
/**
 * \brief Collection of weather indices used for calculating FwiWeather.
 */
struct Weather
{
  static consteval Weather Zero() { return {}; }
  static consteval Weather Invalid()
  {
    return {
      .temperature = Temperature::Invalid(),
      .rh = RelativeHumidity::Invalid(),
      .wind = Wind::Invalid(),
      .prec = Precipitation::Invalid()
    };
  }
  /**
   * \brief Temperature (Celsius)
   */
  Temperature temperature{};
  /**
   * \brief Relative Humidity (%)
   */
  RelativeHumidity rh{};
  /**
   * \brief Wind (km/h)
   */
  Wind wind{};
  /**
   * \brief Precipitation (1hr accumulation) (mm)
   */
  Precipitation prec{};
  auto operator<=>(const Weather& rhs) const = default;
};
}
#endif
