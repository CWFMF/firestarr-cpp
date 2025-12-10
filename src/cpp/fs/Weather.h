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
  static constexpr Temperature Zero() { return Temperature{0}; };
  static constexpr Temperature Invalid() { return Temperature{-1}; };
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
  static constexpr RelativeHumidity Zero() { return RelativeHumidity{0}; };
  static constexpr RelativeHumidity Invalid() { return RelativeHumidity{-1}; };
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
  static constexpr Speed Zero() { return Speed{0}; };
  static constexpr Speed Invalid() { return Speed{-1}; };
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
  [[nodiscard]] constexpr MathSize asDegrees() const { return value; }
  /**
   * \brief Heading (opposite of this direction)
   * \return Heading (opposite of this direction)
   */
  [[nodiscard]] constexpr MathSize heading() const { return to_heading(asRadians()); }
  /**
   * \brief Direction of 0 (North)
   */
  static constexpr Direction Zero() { return Direction{0, false}; };
  static constexpr Direction Invalid() { return Direction{-1, false}; };
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
  constexpr Wind() noexcept : wsv_x_(0), wsv_y_(0), direction(0.0, false), speed(0) { }
  /**
   * \brief Constructor
   * \param direction Direction wind is coming from
   * \param speed Speed of wind
   */
  Wind(const Direction& direction, const Speed speed) noexcept
    : wsv_x_(speed.value * sin(direction.heading())),
      wsv_y_(speed.value * cos(direction.heading())), direction(direction), speed(speed)
  { }
  constexpr Wind(const Wind& rhs) noexcept = default;
  constexpr Wind(Wind&& rhs) noexcept = default;
  Wind& operator=(const Wind& rhs) noexcept = default;
  Wind& operator=(Wind&& rhs) noexcept = default;
  /**
   * \brief Direction wind is going towards
   * \return Direction wind is going towards
   */
  [[nodiscard]] constexpr MathSize heading() const noexcept { return direction.heading(); }
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
    return direction != rhs.direction || speed != rhs.speed;
  }
  /**
   * \brief Equals to operator
   * \param rhs Wind to compare to
   * \return Whether or not these are equivalent
   */
  [[nodiscard]] constexpr bool operator==(const Wind& rhs) const noexcept
  {
    return direction == rhs.direction && speed == rhs.speed;
  }
  /**
   * \brief Less than operator
   * \param rhs Wind to compare to
   * \return Whether or not this is less than the compared to Wind
   */
  [[nodiscard]] constexpr bool operator<(const Wind& rhs) const noexcept
  {
    if (direction == rhs.direction)
    {
      return speed < rhs.speed;
    }
    return direction < rhs.direction;
  }
  /**
   * \brief Wind with 0 Speed from Direction 0
   */
  static constexpr Wind Zero() { return Wind{Direction::Zero(), Speed::Zero()}; };
  static constexpr Wind Invalid() { return Wind{Direction::Invalid(), Speed::Invalid()}; };

private:
  /**
   * \brief Wind speed vector in X direction (East is positive)
   */
  MathSize wsv_x_;
  /**
   * \brief Wind speed vector in Y direction (North is positive)
   */
  MathSize wsv_y_;

public:
  /**
   * \brief Direction (degrees or radians, N is 0)
   */
  Direction direction;
  /**
   * \brief Speed (km/h)
   */
  Speed speed;
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
  static constexpr Precipitation Zero() { return Precipitation{0}; };
  static constexpr Precipitation Invalid() { return Precipitation{-1}; };
  ;
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
   * \param temperature Temperature (Celsius)
   * \param rh Relative Humidity (%)
   * \param wind Wind (km/h)
   * \param prec Precipitation (1hr accumulation) (mm)
   */
  constexpr Weather(
    const Temperature& temperature,
    const RelativeHumidity& rh,
    const Wind& wind,
    const Precipitation& prec
  ) noexcept
    : temperature(temperature), rh(rh), wind(wind), prec(prec)
  { }
  constexpr Weather(Weather&& rhs) noexcept = default;
  constexpr Weather(const Weather& rhs) noexcept = default;
  Weather& operator=(Weather&& rhs) noexcept = default;
  Weather& operator=(const Weather& rhs) = default;
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
};
}
#endif
