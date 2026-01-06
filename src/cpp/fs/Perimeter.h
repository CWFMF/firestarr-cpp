/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_PERIMETER_H
#define FS_PERIMETER_H
#include "stdafx.h"
#include "GridMap.h"
#include "Point.h"
namespace fs
{
class Environment;
/**
 * \brief A map of locations which have burned in a Scenario.
 * Use this class so that we can filter by fuel cells but not expose the members
 */
class BurnedMap final : public GridMap<unsigned char>
{
public:
  /**
   * \brief Constructor
   * \param perim_grid Grid representing Perimeter to initialize from
   * \param env Environment to use as base
   */
  BurnedMap(const Grid<unsigned char, unsigned char>& perim_grid, const Environment& env);
};
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
  Perimeter(const Location& location, const size_t size, const Environment& env);
  /**
   * \brief List of all burned Locations
   */
  const list<Location> burned;
  /**
   * \brief List of all Locations along the edge of this Perimeter
   */
  const list<Location> edge;

private:
  Perimeter(const BurnedMap& burned_map);
};
}
#endif
