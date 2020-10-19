/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "EnvironmentInfo.h"
#include <utility>
#include "Environment.h"
#include "Grid.h"
#include "Log.h"
#include "Point.h"
#include "Settings.h"
namespace fs
{
EnvironmentInfo::~EnvironmentInfo() = default;
EnvironmentInfo::EnvironmentInfo(
  const string_view in_fuel,
  const string_view in_elevation,
  GridBase&& fuel,
  GridBase&& elevation
) noexcept
  : fuel_(fuel), elevation_(elevation), in_fuel_(std::move(in_fuel)),
    in_elevation_(std::move(in_elevation))
{
  logging::debug(
    "fuel: %dx%d => (%f, %f)",
    fuel.calculateColumns(),
    fuel.calculateRows(),
    fuel.xllcorner(),
    fuel.yllcorner()
  );
  logging::debug(
    "elevation: %dx%d => (%f, %f)",
    elevation.calculateColumns(),
    elevation.calculateRows(),
    elevation.xllcorner(),
    elevation.yllcorner()
  );
  logging::check_fatal(
    !(fuel.calculateRows() == elevation.calculateRows()
      && fuel.calculateColumns() == elevation.calculateColumns()
      && fuel.cellSize() == elevation.cellSize() && fuel.xllcorner() == elevation.xllcorner()
      && fuel.yllcorner() == elevation.yllcorner()),
    "Grids are not aligned"
  );
}
EnvironmentInfo::EnvironmentInfo(const string_view in_fuel, const string_view in_elevation)
  : EnvironmentInfo(in_fuel, in_elevation, read_header(in_fuel), read_header(in_elevation))
{ }
unique_ptr<EnvironmentInfo> EnvironmentInfo::loadInfo(
  const string_view in_fuel,
  const string_view in_elevation
)
{
  if (Settings::runAsync())
  {
    auto fuel_async = async(launch::async, [in_fuel]() { return read_header(in_fuel); });
    auto elevation_async =
      async(launch::async, [in_elevation]() { return read_header(in_elevation); });
    const auto e =
      new EnvironmentInfo(in_fuel, in_elevation, fuel_async.get(), elevation_async.get());
    return unique_ptr<EnvironmentInfo>(e);
  }
  const auto e =
    new EnvironmentInfo(in_fuel, in_elevation, read_header(in_fuel), read_header(in_elevation));
  return unique_ptr<EnvironmentInfo>(e);
}
Environment EnvironmentInfo::load(const Point& point) const
{
  return Environment::load(point, in_fuel_, in_elevation_);
}
unique_ptr<Coordinates> EnvironmentInfo::findCoordinates(const Point& point, const bool flipped)
  const
{
  return fuel_.findCoordinates(point, flipped);
}
unique_ptr<FullCoordinates> EnvironmentInfo::findFullCoordinates(
  const Point& point,
  const bool flipped
) const
{
  return fuel_.findFullCoordinates(point, flipped);
}
}
