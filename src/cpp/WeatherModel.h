/* SPDX-FileCopyrightText: 2020-2021 Queen's Printer for Ontario */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef FS_WEATHERMODEL_H
#define FS_WEATHERMODEL_H

#include "Point.h"
namespace fs
{
namespace wx
{
/**
 * \brief A model within the WxSHIELD ensemble set.
 */
class WeatherModel
{
public:
  /**
   * \brief Time that model data was generated
   * \return Time that model data was generated
   */
  [[nodiscard]] constexpr const TIMESTAMP_STRUCT&
  generated() const
  {
    return generated_;
  }
  /**
   * \brief Name of this model
   * \return Name of this model
   */
  [[nodiscard]] constexpr const string&
  name() const
  {
    return name_;
  }
  /**
   * \brief Point this model represents
   * \return Point this model represents
   */
  [[nodiscard]] constexpr const topo::Point&
  point() const
  {
    return point_;
  }
  /**
   * \brief Distance that model data is from the Point being represented (m)
   * \return Distance that model data is from the Point being represented (m)
   */
  [[nodiscard]] constexpr double
  distanceFrom() const
  {
    return distance_from_;
  }
  /**
   * \brief Constructor
   * \param generated Time model was generated for
   * \param name Name of the model
   * \param point Point model was generated for
   * \param distance_from Distance model data was from Point
   */
  WeatherModel(
    const TIMESTAMP_STRUCT& generated,
    string&& name,
    const topo::Point& point,
    double distance_from
  ) noexcept;
  /**
   * \brief Destructor
   */
  ~WeatherModel() = default;
  /**
   * \brief Move constructor
   * \param rhs WeatherModel to move from
   */
  WeatherModel(WeatherModel&& rhs) noexcept = default;
  /**
   * \brief Copy constructor
   * \param rhs WeatherModel to copy from
   */
  WeatherModel(const WeatherModel& rhs) = default;
  /**
   * \brief Move assignment
   * \param rhs WeatherModel to move from
   * \return This, after assignment
   */
  WeatherModel&
  operator=(WeatherModel&& rhs) noexcept = default;
  /**
   * \brief Copy assignment
   * \param rhs WeatherModel to copy from
   * \return This, after assignment
   */
  WeatherModel&
  operator=(const WeatherModel& rhs);
private:
  /**
   * \brief When this was generated
   */
  TIMESTAMP_STRUCT generated_;
  /**
   * \brief Name of this
   */
  string name_;
  /**
   * \brief Point this represents
   */
  topo::Point point_;
  /**
   * \brief Distance actual point for this is from represented Point (m)
   */
  double distance_from_;
};
/**
 * \brief Provides ordering for WeatherModel comparison.
 */
struct ModelCompare
{
  /**
   * \brief Provides ordering for WeatherModel comparison
   * \param x First WeatherModel to compare
   * \param y Second WeatherModel to compare
   * \return Whether or not x <= y
   */
  [[nodiscard]] bool
  operator()(const WeatherModel& x, const WeatherModel& y) const noexcept;
};
}
}
#endif
