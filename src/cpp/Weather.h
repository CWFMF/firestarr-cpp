/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_WEATHER_H
#define FS_WEATHER_H
#include "stdafx.h"
#include "Log.h"
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
struct Temperature
{
  MathSize value{0};
  static constexpr Temperature Zero() { return Temperature{0}; };
  static constexpr Temperature Invalid() { return Temperature{-1}; };
  auto operator<=>(const Temperature& rhs) const = default;
};
/**
 * \brief Relative humidity as a percentage.
 */
struct RelativeHumidity
{
  MathSize value{0};
  static constexpr RelativeHumidity Zero() { return RelativeHumidity{0}; };
  static constexpr RelativeHumidity Invalid() { return RelativeHumidity{-1}; };
  auto operator<=>(const RelativeHumidity& rhs) const = default;
};
/**
 * \brief Speed in kilometers per hour.
 */
struct Speed
{
  MathSize value{0};
  static constexpr Speed Zero() { return Speed{0}; };
  static constexpr Speed Invalid() { return Speed{-1}; };
  auto operator<=>(const Speed& rhs) const = default;
};
/**
 * \brief Direction with access to degrees or radians.
 */
struct Direction
{
  MathSize value{0};
  static constexpr Direction Zero() { return Direction{0, false}; };
  static constexpr Direction Invalid() { return Direction{-1, false}; };
  auto operator<=>(const Direction& rhs) const = default;
  constexpr Direction(const MathSize value = 0, const bool is_radians = false)
    : value{is_radians ? to_degrees(value) : value}
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
  static constexpr Wind Zero() { return {Direction::Zero(), Speed::Zero()}; };
  static constexpr Wind Invalid() { return {Direction::Invalid(), Speed::Invalid()}; };
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
struct Precipitation
{
  MathSize value{0};
  static constexpr Precipitation Zero() { return Precipitation{0}; };
  static constexpr Precipitation Invalid() { return Precipitation{-1}; };
  auto operator<=>(const Precipitation& rhs) const = default;
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
