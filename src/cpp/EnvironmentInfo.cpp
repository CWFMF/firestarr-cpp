/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2021-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "EnvironmentInfo.h"
#include "Environment.h"
#include "Settings.h"

namespace fs::topo
{
EnvironmentInfo::~EnvironmentInfo() = default;
EnvironmentInfo::EnvironmentInfo(
  string in_fuel,
  string in_slope,
  string in_aspect,
  string in_elevation,
  data::GridBase&& fuel,
  data::GridBase&& slope,
  data::GridBase&& aspect,
  data::GridBase&& elevation
) noexcept
  : fuel_(std::move(fuel)),
    slope_(std::move(slope)),
    aspect_(std::move(aspect)),
    elevation_(std::move(elevation)),
    in_fuel_(std::move(in_fuel)),
    in_slope_(std::move(in_slope)),
    in_aspect_(std::move(in_aspect)),
    in_elevation_(std::move(in_elevation))
{
  logging::debug(
    "fuel: %dx%d => (%f, %f)",
    fuel.columns(),
    fuel.rows(),
    fuel.xllcorner(),
    fuel.yllcorner()
  );
  logging::debug(
    "slope: %dx%d => (%f, %f)",
    slope.columns(),
    slope.rows(),
    slope.xllcorner(),
    slope.yllcorner()
  );
  logging::debug(
    "aspect: %dx%d => (%f, %f)",
    aspect.columns(),
    aspect.rows(),
    aspect.xllcorner(),
    aspect.yllcorner()
  );
  logging::check_fatal(
    !(fuel.rows() == slope.rows() && fuel.columns() == slope.columns()
      && fuel.cellSize() == slope.cellSize() && fuel.xllcorner() == slope.xllcorner()
      && fuel.yllcorner() == slope.yllcorner() && fuel.rows() == aspect.rows()
      && fuel.columns() == aspect.columns() && fuel.cellSize() == aspect.cellSize()
      && fuel.xllcorner() == aspect.xllcorner() && fuel.yllcorner() == aspect.yllcorner()
      && fuel.rows() == elevation.rows() && fuel.columns() == elevation.columns()
      && fuel.cellSize() == elevation.cellSize() && fuel.xllcorner() == elevation.xllcorner()
      && fuel.yllcorner() == elevation.yllcorner()),
    "Grids are not aligned"
  );
}
EnvironmentInfo::EnvironmentInfo(
  const string& in_fuel,
  const string& in_slope,
  const string& in_aspect,
  const string& in_elevation
)
  : EnvironmentInfo(
      in_fuel,
      in_slope,
      in_aspect,
      in_elevation,
      data::read_header<const fuel::FuelType*>(in_fuel),
      data::read_header<SlopeSize>(in_slope),
      data::read_header<AspectSize>(in_aspect),
      data::read_header<ElevationSize>(in_elevation)
    )
{
}
unique_ptr<EnvironmentInfo>
EnvironmentInfo::loadInfo(
  const string& in_fuel,
  const string& in_slope,
  const string& in_aspect,
  const string& in_elevation
)
{
  if (sim::Settings::runAsync())
  {
    auto fuel_async = async(launch::async, [in_fuel]() {
      return data::read_header<const fuel::FuelType*>(in_fuel);
    });
    auto slope_async = async(launch::async, [in_slope]() {
      return data::read_header<SlopeSize>(in_slope);
    });
    auto aspect_async = async(launch::async, [in_aspect]() {
      return data::read_header<AspectSize>(in_aspect);
    });
    auto elevation_async = async(launch::async, [in_elevation]() {
      return data::read_header<ElevationSize>(in_elevation);
    });
    const auto e = new EnvironmentInfo(
      in_fuel,
      in_slope,
      in_aspect,
      in_elevation,
      fuel_async.get(),
      slope_async.get(),
      aspect_async.get(),
      elevation_async.get()
    );
    return unique_ptr<EnvironmentInfo>(e);
  }
  const auto e = new EnvironmentInfo(
    in_fuel,
    in_slope,
    in_aspect,
    in_elevation,
    data::read_header<const fuel::FuelType*>(in_fuel),
    data::read_header<SlopeSize>(in_slope),
    data::read_header<AspectSize>(in_aspect),
    data::read_header<ElevationSize>(in_elevation)
  );
  return unique_ptr<EnvironmentInfo>(e);
}
Environment
EnvironmentInfo::load(
  const fuel::FuelLookup& lookup,
  const Point& point
) const
{
  return Environment::load(lookup, point, in_fuel_, in_slope_, in_aspect_, in_elevation_);
}
unique_ptr<Coordinates>
EnvironmentInfo::findCoordinates(
  const Point& point,
  const bool flipped
) const
{
  return fuel_.findCoordinates(point, flipped);
}
}
