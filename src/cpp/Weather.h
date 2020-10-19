/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_WEATHER_H
#define FS_WEATHER_H
#include "stdafx.h"
#include "Index.h"
#include "Util.h"
namespace fs
{
/**
 * \brief Temperature in degrees Celsius.
 */
class Temperature : public Index<Temperature>
{
public:
  //! @cond Doxygen_Suppress
  using Index::Index;
  //! @endcond
  /**
   * \brief 0 degrees Celsius
   */
  static const Temperature Zero;
  static const Temperature Invalid;
};
/**
 * \brief Relative humidity as a percentage.
 */
class RelativeHumidity : public Index<RelativeHumidity>
{
public:
  //! @cond Doxygen_Suppress
  using Index::Index;
  //! @endcond
  /**
   * \brief 0% Relative Humidity
   */
  static const RelativeHumidity Zero;
  static const RelativeHumidity Invalid;
};
/**
 * \brief Speed in kilometers per hour.
 */
class Speed : public Index<Speed>
{
public:
  //! @cond Doxygen_Suppress
  using Index::Index;
  //! @endcond
  /**
   * \brief 0 km/h
   */
  static const Speed Zero;
  static const Speed Invalid;
};
/**
 * \brief Direction with access to degrees or radians.
 */
class Direction : public Index<Direction>
{
public:
  ~Direction() = default;
  /**
   * \brief Construct with Direction of 0 (North)
   */
  constexpr Direction() noexcept = default;
  /**
   * \brief Constructor
   * \param value Direction to use
   * \param is_radians Whether the given direction is in radians (as opposed to degrees)
   */
  constexpr Direction(const MathSize value, const bool is_radians)
    : Index(is_radians ? to_degrees(value) : value)
  { }
  constexpr Direction(const Direction& rhs) noexcept = default;
  constexpr Direction(Direction&& rhs) noexcept = default;
  Direction& operator=(const Direction& rhs) noexcept = default;
  Direction& operator=(Direction&& rhs) noexcept = default;
  /**
   * \brief Direction as radians, where 0 is North and values increase clockwise
   * \return Direction as radians, where 0 is North and values increase clockwise
   */
  [[nodiscard]] constexpr MathSize asRadians() const { return to_radians(asDegrees()); }
  /**
   * \brief Direction as degrees, where 0 is North and values increase clockwise
   * \return Direction as degrees, where 0 is North and values increase clockwise
   */
  [[nodiscard]] constexpr MathSize asDegrees() const { return asValue(); }
  /**
   * \brief Heading (opposite of this direction)
   * \return Heading (opposite of this direction)
   */
  [[nodiscard]] constexpr MathSize heading() const { return to_heading(asRadians()); }
  /**
   * \brief Direction of 0 (North)
   */
  static const Direction Zero;
  static const Direction Invalid;
};
/**
 * \brief Wind with a Speed and Direction.
 */
class Wind
{
public:
  ~Wind() = default;
  /**
   * \brief Construct with 0 values
   */
  constexpr Wind() noexcept : wsv_x_(0), wsv_y_(0), direction_(0.0, false), speed_(0) { }
  /**
   * \brief Constructor
   * \param direction Direction wind is coming from
   * \param speed Speed of wind
   */
  Wind(const Direction& direction, const Speed speed) noexcept
    : wsv_x_(speed.asValue() * sin(direction.heading())),
      wsv_y_(speed.asValue() * cos(direction.heading())), direction_(direction), speed_(speed)
  { }
  constexpr Wind(const Wind& rhs) noexcept = default;
  constexpr Wind(Wind&& rhs) noexcept = default;
  Wind& operator=(const Wind& rhs) noexcept = default;
  Wind& operator=(Wind&& rhs) noexcept = default;
  /**
   * \brief Speed of wind (km/h)
   * \return Speed of wind (km/h)
   */
  [[nodiscard]] constexpr const Speed& speed() const noexcept { return speed_; }
  /**
   * \brief Direction wind is coming from.
   * \return Direction wind is coming from.
   */
  [[nodiscard]] constexpr const Direction& direction() const noexcept { return direction_; }
  /**
   * \brief Direction wind is going towards
   * \return Direction wind is going towards
   */
  [[nodiscard]] constexpr MathSize heading() const noexcept { return direction_.heading(); }
  /**
   * \brief X component of wind vector (km/h)
   * \return X component of wind vector (km/h)
   */
  [[nodiscard]] constexpr MathSize wsvX() const noexcept { return wsv_x_; }
  /**
   * \brief Y component of wind vector (km/h)
   * \return Y component of wind vector (km/h)
   */
  [[nodiscard]] constexpr MathSize wsvY() const noexcept { return wsv_y_; }
  /**
   * \brief Not equal to operator
   * \param rhs Wind to compare to
   * \return Whether or not these are not equivalent
   */
  [[nodiscard]] constexpr bool operator!=(const Wind& rhs) const noexcept
  {
    return direction_ != rhs.direction_ || speed_ != rhs.speed_;
  }
  /**
   * \brief Equals to operator
   * \param rhs Wind to compare to
   * \return Whether or not these are equivalent
   */
  [[nodiscard]] constexpr bool operator==(const Wind& rhs) const noexcept
  {
    return direction_ == rhs.direction_ && speed_ == rhs.speed_;
  }
  /**
   * \brief Less than operator
   * \param rhs Wind to compare to
   * \return Whether or not this is less than the compared to Wind
   */
  [[nodiscard]] constexpr bool operator<(const Wind& rhs) const noexcept
  {
    if (direction_ == rhs.direction_)
    {
      return speed_ < rhs.speed_;
    }
    return direction_ < rhs.direction_;
  }
  /**
   * \brief Wind with 0 Speed from Direction 0
   */
  [[maybe_unused]] static const Wind Zero;
  static const Wind Invalid;

private:
  /**
   * \brief Wind speed vector in X direction (East is positive)
   */
  MathSize wsv_x_;
  /**
   * \brief Wind speed vector in Y direction (North is positive)
   */
  MathSize wsv_y_;
  /**
   * \brief Direction (degrees or radians, N is 0)
   */
  Direction direction_;
  /**
   * \brief Speed (km/h)
   */
  Speed speed_;
};
/**
 * \brief Precipitation (1hr accumulation) (mm)
 */
class Precipitation : public Index<Precipitation>
{
public:
  //! @cond Doxygen_Suppress
  using Index::Index;
  //! @endcond
  /**
   * \brief Accumulated Precipitation of 0 mm
   */
  static const Precipitation Zero;
  static const Precipitation Invalid;
};
/**
 * \brief Collection of weather indices used for calculating FwiWeather.
 */
class Weather
{
public:
  virtual ~Weather() = default;
  /**
   * \brief Constructor with no initialization
   */
  constexpr Weather() noexcept = default;
  /**
   * \brief Construct with given indices
   * \param temp Temperature (Celsius)
   * \param rh Relative Humidity (%)
   * \param wind Wind (km/h)
   * \param prec Precipitation (1hr accumulation) (mm)
   */
  constexpr Weather(
    const Temperature& temp,
    const RelativeHumidity& rh,
    const Wind& wind,
    const Precipitation& prec
  ) noexcept
    : temp_(temp), rh_(rh), wind_(wind), prec_(prec)
  { }
  constexpr Weather(Weather&& rhs) noexcept = default;
  constexpr Weather(const Weather& rhs) noexcept = default;
  Weather& operator=(Weather&& rhs) noexcept = default;
  Weather& operator=(const Weather& rhs) = default;
  /**
   * \brief Temperature (Celsius)
   * \return Temperature (Celsius)
   */
  [[nodiscard]] constexpr const Temperature& temp() const noexcept { return temp_; }
  /**
   * \brief Relative Humidity (%)
   * \return Relative Humidity (%)
   */
  [[nodiscard]] constexpr const RelativeHumidity& rh() const noexcept { return rh_; }
  /**
   * \brief Wind (km/h)
   * \return Wind (km/h)
   */
  [[nodiscard]] constexpr const Wind& wind() const noexcept { return wind_; }
  /**
   * \brief Precipitation (1hr accumulation) (mm)
   * \return Precipitation (1hr accumulation) (mm)
   */
  [[nodiscard]] constexpr const Precipitation& prec() const noexcept { return prec_; }

private:
  /**
   * \brief Temperature (Celsius)
   */
  Temperature temp_;
  /**
   * \brief Relative Humidity (%)
   */
  RelativeHumidity rh_;
  /**
   * \brief Wind (km/h)
   */
  Wind wind_;
  /**
   * \brief Precipitation (1hr accumulation) (mm)
   */
  Precipitation prec_;
};
}
#endif
