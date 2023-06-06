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
#include "Environment.h"
#include "EnvironmentInfo.h"
#include "FuelLookup.h"
#include "ProbabilityMap.h"
#include "Scenario.h"
#include "Settings.h"

namespace tbd
{
namespace topo
{
Environment::~Environment()
{
  delete cells_;
}
Environment Environment::load(const Point& point,
                              const string& in_fuel,
                              const string& in_elevation)
{
  logging::note("Fuel raster is %s", in_fuel.c_str());
  if (sim::Settings::runAsync())
  {
    logging::debug("Loading grids async");
    auto fuel = async(launch::async, [&in_fuel, &point]() { return FuelGrid::readTiff(in_fuel, point, sim::Settings::fuelLookup()); });
    auto elevation = async(launch::async, [&in_elevation, &point]() { return ElevationGrid::readTiff(in_elevation, point); });
    logging::debug("Waiting for grids");
    return Environment(*unique_ptr<FuelGrid>(fuel.get()),
                       *unique_ptr<ElevationGrid>(elevation.get()),
                       point);
  }
  logging::warning("Loading grids synchronously");
  // HACK: need to copy strings since closures do that above
  return Environment(*unique_ptr<FuelGrid>(
                       FuelGrid::readTiff(string(in_fuel), point, sim::Settings::fuelLookup())),
                     *unique_ptr<ElevationGrid>(
                       ElevationGrid::readTiff(string(in_elevation), point)),
                     point);
}
sim::ProbabilityMap* Environment::makeProbabilityMap(const double time,
                                                     const double start_time,
                                                     const int min_value,
                                                     const int low_max,
                                                     const int med_max,
                                                     const int max_value) const
{
  return new sim::ProbabilityMap(time,
                                 start_time,
                                 min_value,
                                 low_max,
                                 med_max,
                                 max_value,
                                 *cells_);
}
Environment Environment::loadEnvironment(const string& path,
                                         const Point& point,
                                         const string& perimeter,
                                         const int year)
{
  logging::note("Using ignition point (%f, %f)", point.latitude(), point.longitude());
  logging::info("Running using inputs directory '%s'", path.c_str());
  auto rasters = util::find_rasters(path, year);
  auto best_x = numeric_limits<double>::max();
  auto best_y = numeric_limits<FullIdx>::max();
  unique_ptr<const EnvironmentInfo> env_info = nullptr;
  unique_ptr<data::GridBase> for_info = nullptr;
  if (!perimeter.empty())
  {
    for_info = make_unique<data::GridBase>(data::read_header(perimeter));
    logging::info("Perimeter projection is %s", for_info->proj4().c_str());
  }
  for (const auto& raster : rasters)
  {
    auto fuel = raster;
    // make sure we're using a consistent directory separator
    std::replace(fuel.begin(), fuel.end(), '\\', '/');
    // HACK: assume there's only one instance of 'fuel' in the file name we want to change
    const auto find_what = string("fuel");
    const auto find_len = find_what.length();
    const auto find_start = fuel.find(find_what, fuel.find_last_of('/'));
    const auto elevation = string(fuel).replace(find_start, find_len, "dem");
    unique_ptr<const EnvironmentInfo> cur_info = EnvironmentInfo::loadInfo(
      fuel,
      elevation);
    const auto cur_x = abs(point.longitude() - cur_info->meridian());
    logging::verbose("Zone %0.1f meridian is %0.2f degrees from point",
                     cur_info->zone(),
                     cur_x);
    // HACK: assume floating point is going to always be exactly the same result
    if ((nullptr == for_info || cur_info->meridian() == for_info->meridian())
        && cur_x <= best_x)
    {
      logging::verbose("SWITCH X");
      if (cur_x != best_x)
      {
        // if we're switching zones then we need to reset this
        best_y = numeric_limits<FullIdx>::max();
      }
      best_x = cur_x;
      // overwrite should delete current pointer
      const auto coordinates = cur_info->findFullCoordinates(point, false);
      if (nullptr != coordinates)
      {
        logging::verbose("CHECK Y");
        const auto cur_y = static_cast<FullIdx>(abs(
          std::get<0>(*coordinates) - cur_info->calculateRows() / static_cast<FullIdx>(2)));
        logging::verbose(("Current y value is " + std::to_string(cur_y)).c_str());
        if (cur_y < best_y)
        {
          logging::verbose("SWITCH Y");
          env_info = std::move(cur_info);
          best_y = cur_y;
        }
      }
      else
      {
        logging::verbose("NULLPTR");
      }
    }
  }
  logging::check_fatal(nullptr == env_info,
                       "Could not find an environment to use for (%f, %f)",
                       point.latitude(),
                       point.longitude());
  logging::debug("Best match for (%f, %f) is zone %0.2f with a meridian of %0.2f",
                 point.latitude(),
                 point.longitude(),
                 env_info->zone(),
                 env_info->meridian());
  logging::note("Projection is %s", env_info->proj4().c_str());
  // envInfo should get deleted automatically because it uses unique_ptr
  return env_info->load(point);
}
unique_ptr<Coordinates> Environment::findCoordinates(const Point& point,
                                                     const bool flipped) const
{
  return cells_->findCoordinates(point, flipped);
}
}
}
