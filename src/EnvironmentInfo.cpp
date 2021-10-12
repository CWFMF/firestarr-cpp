// Copyright (c) 2020-2021, Queen's Printer for Ontario.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "stdafx.h"
#include "EnvironmentInfo.h"
#include "Environment.h"
#include "Settings.h"
namespace firestarr
{
namespace topo
{
EnvironmentInfo::~EnvironmentInfo() = default;
EnvironmentInfo::EnvironmentInfo(string in_fuel,
                                 string in_elevation,
                                 data::GridBase&& fuel,
                                 data::GridBase&& elevation) noexcept
  : fuel_(std::move(fuel)),
    elevation_(std::move(elevation)),
    in_fuel_(std::move(in_fuel)),
    in_elevation_(std::move(in_elevation))
{
  logging::debug("fuel: %dx%d => (%f, %f)", fuel.calculateColumns(), fuel.calculateRows(), fuel.xllcorner(), fuel.yllcorner());
  logging::debug("elevation: %dx%d => (%f, %f)", elevation.calculateColumns(), elevation.calculateRows(), elevation.xllcorner(), elevation.yllcorner());
  logging::check_fatal(!(fuel.calculateRows() == elevation.calculateRows()
                         && fuel.calculateColumns() == elevation.calculateColumns()
                         && fuel.cellSize() == elevation.cellSize()
                         && fuel.xllcorner() == elevation.xllcorner()
                         && fuel.yllcorner() == elevation.yllcorner()),
                       "Grids are not aligned");
}
EnvironmentInfo::EnvironmentInfo(const string& in_fuel,
                                 const string& in_elevation)
  : EnvironmentInfo(in_fuel,
                    in_elevation,
                    data::read_header<const fuel::FuelType*>(in_fuel),
                    data::read_header<ElevationSize>(in_elevation))
{
}
unique_ptr<EnvironmentInfo> EnvironmentInfo::loadInfo(const string& in_fuel,
                                                      const string& in_elevation)
{
  if (sim::Settings::runAsync())
  {
    auto fuel_async = async(launch::async,
                            [in_fuel]()
                            {
                              return data::read_header<const fuel::FuelType*>(in_fuel);
                            });
    auto elevation_async = async(launch::async,
                                 [in_elevation]()
                                 {
                                   return data::read_header<ElevationSize>(in_elevation);
                                 });
    const auto e = new EnvironmentInfo(in_fuel,
                                       in_elevation,
                                       fuel_async.get(),
                                       elevation_async.get());
    return unique_ptr<EnvironmentInfo>(e);
  }
  const auto e = new EnvironmentInfo(in_fuel,
                                     in_elevation,
                                     data::read_header<const fuel::FuelType*>(in_fuel),
                                     data::read_header<ElevationSize>(in_elevation));
  return unique_ptr<EnvironmentInfo>(e);
}
Environment EnvironmentInfo::load(const Point& point) const
{
  return Environment::load(point, in_fuel_, in_elevation_);
}
unique_ptr<Coordinates> EnvironmentInfo::findCoordinates(
  const Point& point,
  const bool flipped) const
{
  return fuel_.findCoordinates(point, flipped);
}
unique_ptr<FullCoordinates> EnvironmentInfo::findFullCoordinates(
  const Point& point,
  const bool flipped) const
{
  return fuel_.findFullCoordinates(point, flipped);
}
}
}
