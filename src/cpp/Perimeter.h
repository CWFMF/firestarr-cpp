/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_PERIMETER_H
#define FS_PERIMETER_H
#include "stdafx.h"
#include "Location.h"
#include "Point.h"
namespace fs
{
class FwiWeather;
class Environment;
/**
 * \brief Perimeter for an existing fire to initialize a simulation with.
 */
class Perimeter
{
public:
  /**
   * \brief Initialize perimeter from a file
   * \param perim File to read from
   * \param point Origin of fire
   * \param env Environment to apply Perimeter to
   */
  Perimeter(const string_view perim, const Point& point, const Environment& env);
  /**
   * \brief Create a Perimeter of the given size at the given Location
   * \param location Location to center Perimeter on
   * \param size Size of Perimeter to create
   * \param env Environment to apply Perimeter to
   */
  Perimeter(const Location& location, size_t size, const Environment& env);
  /**
   * \brief List of all burned Locations
   * \return All Locations burned by this Perimeter
   */
  [[nodiscard]] const list<Location>& burned() const noexcept;
  /**
   * \brief List of all Locations along the edge of this Perimeter
   * \return All Locations along the edge of this Perimeter
   */
  [[nodiscard]] const list<Location>& edge() const noexcept;

private:
  /**
   * \brief List of all burned Locations
   */
  list<Location> burned_;
  /**
   * \brief List of all Locations along the edge of this Perimeter
   */
  list<Location> edge_;
};
}
#endif
