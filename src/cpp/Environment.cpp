/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2021-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "Environment.h"
#include "EnvironmentInfo.h"
#include "FuelLookup.h"
#include "Grid.h"
#include "Log.h"
#include "Point.h"
#include "ProbabilityMap.h"
#include "Settings.h"
#include "Util.h"
namespace fs
{
Environment Environment::load(
  const Point& point,
  const string_view in_fuel,
  const string_view in_elevation
)
{
  logging::note("Fuel raster is %s", string(in_fuel).c_str());
  const auto& lookup = Settings::fuelLookup();
  if (Settings::runAsync())
  {
    logging::debug("Loading grids async");
    auto fuel = async(launch::async, [&in_fuel, &point]() {
      return FuelGrid::readTiff(in_fuel, point, Settings::fuelLookup());
    });
    auto elevation = async(launch::async, [&in_elevation, &point]() {
      return ElevationGrid::readTiff(in_elevation, point);
    });
    logging::debug("Waiting for grids");
    return Environment(fuel.get(), elevation.get(), point);
  }
  logging::warning("Loading grids synchronously");
  // HACK: need to copy strings since closures do that above
  return Environment(
    FuelGrid::readTiff(string(in_fuel), point, lookup),
    ElevationGrid::readTiff(string(in_elevation), point),
    point
  );
}
ProbabilityMap* Environment::makeProbabilityMap(
  const DurationSize time,
  const DurationSize start_time,
  const int min_value,
  const int low_max,
  const int med_max,
  const int max_value
) const
{
  return new ProbabilityMap(time, start_time, min_value, low_max, med_max, max_value, cells_);
}
Environment Environment::loadEnvironment(
  const string_view path,
  const Point& point,
  const string_view perimeter,
  const YearSize year
)
{
  logging::note("Using ignition point (%f, %f)", point.latitude(), point.longitude());
  logging::info("Running using inputs directory '%s'", string(path).c_str());
  auto rasters = find_rasters(path, year);
  auto best_score = numeric_limits<MathSize>::min();
  unique_ptr<const EnvironmentInfo> env_info = nullptr;
  unique_ptr<GridBase> for_info = nullptr;
  string best_fuel;
  string best_elevation;
  auto found_best = false;
  if (!perimeter.empty())
  {
    for_info = make_unique<GridBase>(read_header(perimeter));
    logging::info("Perimeter projection is %s", for_info->proj4().c_str());
  }
  for (const auto& raster : rasters)
  {
    auto fuel = raster;
    logging::verbose("Replacing directory separators in path for: %s\n", fuel.c_str());
    // make sure we're using a consistent directory separator
    std::replace(fuel.begin(), fuel.end(), '\\', '/');
    // HACK: assume there's only one instance of 'fuel' in the file name we want to change
    const auto find_what = string("fuel");
    const auto find_len = find_what.length();
    const auto find_start = fuel.find(find_what, fuel.find_last_of('/'));
    const auto elevation = string(fuel).replace(find_start, find_len, "dem");
    unique_ptr<const EnvironmentInfo> cur_info = EnvironmentInfo::loadInfo(fuel, elevation);
    // want the raster that's going to give us the most room to spread, so pick the one with the
    // most
    //   cells between the ignition and the edge on the side where it's closest to the edge
    // FIX: need to pick raster that aligns with perimeter if we have one
    //      -  for now at least ensure the same projection
    if (nullptr != for_info && 0 != strcmp(for_info->proj4().c_str(), cur_info->proj4().c_str()))
    {
      continue;
    }
    // FIX: just worrying about distance from specified lat/long for now, but should pick based on
    // bounds of perimeter flipped because we're reading from a raster so change (left, top) to
    // (left, bottom)
    const auto coordinates = cur_info->findFullCoordinates(point, true);
    if (nullptr != coordinates)
    {
      auto actual_rows = cur_info->calculateRows();
      auto actual_columns = cur_info->calculateColumns();
      const auto x = std::get<0>(*coordinates);
      const auto y = std::get<1>(*coordinates);
      logging::note(
        "Coordinates before reading are (%d, %d => %f, %f)",
        x,
        y,
        x + std::get<2>(*coordinates) / 1000.0,
        y + std::get<3>(*coordinates) / 1000.0
      );
      // if it's not in the raster then this is not an option
      // FIX: are these +/-1 because of counting the cell itself and starting from 0?
      const auto dist_W = x;
      const auto dist_E = actual_columns - x;
      const auto dist_N = actual_rows - y;
      const auto dist_S = y;
      // FIX: should take size of cells into account too? But is largest areas or highest resolution
      // the priority?
      logging::note(
        "Coordinates distance to bottom left is: (%d, %d) and top right is (%d, %d)",
        dist_W,
        dist_S,
        dist_E,
        dist_N
      );
      // shortest hypoteneuse is the closest corner to the origin, so want highest value for this
      const auto cur_score = sq(min(dist_W, dist_E)) + sq(min(dist_N, dist_S));
      if (cur_score > best_score)
      {
        best_score = cur_score;
        best_fuel = fuel;
        best_elevation = elevation;
        found_best = true;
      }
    }
  }
  if (nullptr == env_info && found_best)
  {
    env_info = EnvironmentInfo::loadInfo(best_fuel, best_elevation);
  }
  logging::check_fatal(
    nullptr == env_info,
    "Could not find an environment to use for (%f, %f)",
    point.latitude(),
    point.longitude()
  );
  logging::debug(
    "Best match for (%f, %f) has projection '%s'",
    point.latitude(),
    point.longitude(),
    env_info->proj4().c_str()
  );
  logging::note("Projection is %s", string(env_info->proj4()).c_str());
  // envInfo should get deleted automatically because it uses unique_ptr
  return env_info->load(point);
}
unique_ptr<Coordinates> Environment::findCoordinates(const Point& point, const bool flipped) const
{
  return cells_.findCoordinates(point, flipped);
}
const BurnedData& Environment::unburnable() const { return not_burnable_; }
CellGrid Environment::makeCells(const FuelGrid& fuel, const ElevationGrid& elevation)
{
  logging::check_equal(fuel.yllcorner(), elevation.yllcorner(), "yllcorner");
  static Cell nodata{};
  auto values = vector<Cell>{fuel.data.size()};
  vector<HashSize> hashes{};
  for (Idx r = 0; r < fuel.rows(); ++r)
  {
    for (Idx c = 0; c < fuel.columns(); ++c)
    {
      const auto h = hashes.emplace_back(Location(r, c).hash());
      const Location loc{r, c, h};
      if (r >= 0 && r < fuel.rows() && c >= 0 && c < fuel.columns())
      {
        // NOTE: this needs to translate to internal codes?
        const auto f = FuelType::safeCode(fuel.at(loc));
        auto s = static_cast<SlopeSize>(INVALID_SLOPE);
        auto a = static_cast<AspectSize>(INVALID_ASPECT);
        // HACK: don't calculate for outside box of cells
        if (r > 0 && r < fuel.rows() - 1 && c > 0 && c < fuel.columns() - 1)
        {
          MathSize dem[9];
          bool valid = true;
          for (int i = -1; i < 2; ++i)
          {
            for (int j = -1; j < 2; ++j)
            {
              // grid is (0, 0) at bottom left, but want [0] in array to be NW corner
              auto actual_row = static_cast<Idx>(r - i);
              auto actual_column = static_cast<Idx>(c + j);
              auto cur_loc = Location{actual_row, actual_column};
              const auto v = elevation.at(cur_loc);
              // can't calculate slope & aspect if any surrounding cell is nodata
              if (elevation.nodataValue() == v)
              {
                valid = false;
                break;
              }
              dem[3 * (i + 1) + (j + 1)] = 1.0 * v;
            }
            if (!valid)
            {
              break;
            }
          }
          if (valid)
          {
            // Horn's algorithm
            const MathSize dx =
              ((dem[2] + dem[5] + dem[5] + dem[8]) - (dem[0] + dem[3] + dem[3] + dem[6]))
              / elevation.cellSize();
            const MathSize dy =
              ((dem[6] + dem[7] + dem[7] + dem[8]) - (dem[0] + dem[1] + dem[1] + dem[2]))
              / elevation.cellSize();
            const MathSize key = (dx * dx + dy * dy);
            auto slope_pct = static_cast<float>(100 * (sqrt(key) / 8.0));
            s = min(
              static_cast<SlopeSize>(MAX_SLOPE_FOR_DISTANCE),
              static_cast<SlopeSize>(round(slope_pct))
            );
            static_assert(std::numeric_limits<SlopeSize>::max() >= MAX_SLOPE_FOR_DISTANCE);
            MathSize aspect_azimuth = 0.0;
            if (s > 0 && (dx != 0 || dy != 0))
            {
              aspect_azimuth = atan2(dy, -dx) * M_RADIANS_TO_DEGREES;
              // NOTE: need to change this out of 'math' direction into 'real' direction (i.e. N
              // is 0, not E)
              aspect_azimuth =
                (aspect_azimuth > 90.0) ? (450.0 - aspect_azimuth) : (90.0 - aspect_azimuth);
              if (aspect_azimuth == 360.0)
              {
                aspect_azimuth = 0.0;
              }
            }
            a = static_cast<AspectSize>(round(aspect_azimuth));
          }
        }
        const auto cell = Cell{h, s, a, f};
        values.at(h) = cell;
#ifdef DEBUG_GRIDS
#ifndef VLD_RPTHOOK_INSTALL
        logging::check_equal(cell.row(), r, "Cell row");
        logging::check_equal(cell.column(), c, "Cell column");
        const auto v = values.at(h);
        logging::check_equal(v.row(), r, "Row");
        logging::check_equal(v.column(), c, "Column");
        if (!(INVALID_SLOPE == cell.slope() || INVALID_ASPECT == cell.aspect()
              || INVALID_FUEL_CODE == cell.fuelCode()))
        {
          logging::check_equal(cell.slope(), s, "Cell slope");
          logging::check_equal(v.slope(), s, "Slope");
          if (0 != s)
          {
            logging::check_equal(cell.aspect(), a, "Cell aspect");
            logging::check_equal(v.aspect(), a, "Aspect");
          }
          else
          {
            logging::check_equal(
              cell.aspect(), static_cast<AspectSize>(a), "Cell aspect when slope is 0"
            );
            logging::check_equal(v.aspect(), static_cast<AspectSize>(0), "Aspect when slope is 0");
          }
          logging::check_equal(v.fuelCode(), f, "Fuel");
          logging::check_equal(cell.fuelCode(), f, "Cell fuel");
        }
        else
        {
          logging::check_equal(cell.slope(), INVALID_SLOPE, "Invalid Cell slope");
          logging::check_equal(cell.aspect(), INVALID_ASPECT, "Invalid Cell aspect");
          logging::check_equal(cell.fuelCode(), INVALID_FUEL_CODE, "Invalid Cell fuel");
          logging::check_equal(v.slope(), INVALID_SLOPE, "Invalid slope");
          logging::check_equal(v.aspect(), INVALID_ASPECT, "Invalid aspect");
          logging::check_equal(v.fuelCode(), INVALID_FUEL_CODE, "Invalid fuel");
        }
#endif
#endif
      }
    }
  }
  return CellGrid(
    fuel.cellSize(),
    fuel.rows(),
    fuel.columns(),
    nodata.fullHash(),
    nodata,
    fuel.xllcorner(),
    fuel.yllcorner(),
    fuel.xurcorner(),
    fuel.yurcorner(),
    string(fuel.proj4()),
    std::move(values)
  );
}
Environment::Environment(const FuelGrid& fuel, const ElevationGrid& elevation, const Point& point)
  : Environment(
      makeCells(fuel, elevation),
      elevation.at(Location(*elevation.findCoordinates(point, false).get()))
    )
{
  // take elevation at point so that if max grid size changes elevation doesn't
  logging::note("Start elevation is %d", elevation_);
}
string_view Environment::proj4() const { return cells_.proj4(); }
Idx Environment::rows() const { return cells_.rows(); }
Idx Environment::columns() const { return cells_.columns(); }
MathSize Environment::cellSize() const { return cells_.cellSize(); }
ElevationSize Environment::elevation() const { return elevation_; }
Cell Environment::cell(const Idx row, const Idx column) const
{
  return cells_.at(Location(row, column));
}
Cell Environment::cell(const Location& location) const { return cells_.at(location); }
Cell Environment::offset(const Event& event, const Idx row, const Idx column) const
{
  const auto& p = event.cell();
  return cell(Location(p.row() + row, p.column() + column));
}
#ifdef FIX_THIS_LATER
void Environment::saveToFile(const string& output_directory) const
{
#ifndef NDEBUG
  if (Settings::saveSimulationArea())
  {
    logging::debug("Saving simulation area");
    const auto lookup = Settings::fuelLookup();
    auto convert_to_slope = [&elevation](const Cell& v) -> SlopeSize { return v.slope(); };
    auto convert_to_aspect = [&elevation](const Cell& v) -> AspectSize { return v.aspect(); };
    auto convert_to_area = [&elevation](const ElevationSize v) -> ElevationSize {
      // need to still be nodata if it was
      return (v == elevation.nodataValue()) ? v : 3;
    };
    auto convert_to_fuelcode = [&lookup](const FuelType* const value) -> FuelSize {
      return lookup.fuelToCode(value);
    };
    fuel.saveToFile<FuelSize>(output_directory, "fuel", convert_to_fuelcode);
    elevation.saveToFile(output_directory, "dem");
    // save slope & aspect grids
    cells_->saveToFile<SlopeSize>(
      output_directory, "slope", convert_to_slope, static_cast<SlopeSize>(INVALID_SLOPE)
    );
    cells_->saveToFile<AspectSize>(
      output_directory, "aspect", convert_to_aspect, static_cast<AspectSize>(INVALID_ASPECT)
    );
    // HACK: make a grid with "3" as the value so if we merge max with it it'll cover up anything
    // else
    elevation.saveToFile<ElevationSize>(output_directory, "simulation_area", convert_to_area);
    logging::debug("Done saving fuel grid");
  }
  logging::debug("Done saving fuel grid");
#endif
  const auto lookup = Settings::fuelLookup();
  cells_.saveToTiffFile<FuelSize>(output_directory, "fuel", [&](const auto& value) {
    return lookup.fuelToCode(fuel_by_code(value.fuelCode()));
  });
  // FIX: missing elevation
  cells_.saveToTiffFile<AspectSize>(output_directory, "aspect", [&](const auto& value) {
    return value.aspect();
  });
  cells_.saveToTiffFile<SlopeSize>(output_directory, "slope", [&](const auto& value) {
    return value.slope();
  });
}
#endif
Environment::Environment(CellGrid&& cells, const ElevationSize elevation) noexcept
  : cells_(cells), not_burnable_{cells_}, elevation_(elevation)
{ }
}
