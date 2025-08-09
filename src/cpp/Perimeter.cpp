/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2025. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "Perimeter.h"

namespace fs::topo
{
BurnedMap::BurnedMap(
  const Grid<unsigned char, unsigned char>& perim_grid,
  const Environment& env)
  : GridMap<unsigned char, unsigned char>(
    *env.makeMap<unsigned char>(static_cast<unsigned char>(0)))
{
  // HACK: fix offset if the perimeter raster is different from this one
  logging::check_fatal(0 != strcmp(perim_grid.proj4().c_str(), this->proj4().c_str()),
                       "Invalid projection for input perimeter raster - %s instead of %s",
                       perim_grid.proj4().c_str(),
                       this->proj4().c_str());
  logging::check_fatal(perim_grid.cellSize() != this->cellSize(),
                       "Invalid cell size for input perimeter raster - %f instead of %f",
                       perim_grid.cellSize(),
                       this->cellSize());
  const auto offset_x = static_cast<Idx>((this->xllcorner() - perim_grid.xllcorner()) / this->cellSize());
  const auto perim_origin = static_cast<Idx>(perim_grid.rows() + perim_grid.yllcorner() / this->cellSize());
  const auto this_origin = static_cast<Idx>(this->rows() + this->yllcorner() / this->cellSize());
  const auto offset_y = static_cast<Idx>((perim_origin - this_origin));
  // make sure we don't go out of bounds on grid
  const auto min_column = static_cast<Idx>(offset_x < 0 ? abs(offset_x) : 0);
  const auto max_columns = min(this->columns(), perim_grid.columns());
  const auto min_row = static_cast<Idx>(offset_y < 0 ? abs(offset_y) : 0);
  const auto max_rows = min(this->rows(),
                            static_cast<Idx>(perim_grid.rows() - abs(offset_y)));
  logging::note("Correcting perimeter raster offset by %dx%d cells",
                offset_x,
                offset_y);
  size_t count = 0;
  // since it was read in as a vector we need to check all the cells
  for (auto r = min_row; r < max_rows; ++r)
  {
    for (auto c = min_column; c < max_columns; ++c)
    {
      const auto loc = env.cell(r, c);
      const Location fixed_loc(static_cast<Idx>(r + offset_y),
                               static_cast<Idx>(c + offset_x));
      const auto value = perim_grid.at(fixed_loc.hash());
      if (value != perim_grid.nodataValue() && !fuel::is_null_fuel(loc))
      {
        this->GridMap<unsigned char, unsigned char>::set(loc.hash(), value);
        // logging::debug("(%d, %d) = (%d)", fixed_loc.column(), fixed_loc.row(), value);
        ++count;
      }
    }
  }
#ifdef DEBUG_GRIDS
  for (auto& kv : data)
  {
    logging::check_fatal(fuel::is_null_fuel(env.cell(kv.first)),
                         "Null fuel in BurnedData");
  }
#endif
  logging::info("Loaded burned area of size %lu ha",
                static_cast<size_t>(this->cellSize() / 100.0 * count));
}
Perimeter::Perimeter(const BurnedMap& burned_map)
  : burned(burned_map.makeList()),
    edge(burned_map.makeEdge())
{
}
BurnedMap make_burned_map(
  const string& perim,
  const Point& point,
  const Environment& env)
{
  auto perim_grid = data::ConstantGrid<unsigned char>::readTiff(perim, point);
  return BurnedMap(*perim_grid, env);
}
Perimeter::Perimeter(const string& perim, const Point& point, const Environment& env)
  : Perimeter(make_burned_map(perim, point, env))
{
}
BurnedMap make_burned_map(
  const HashSize hash_value,
  const size_t size,
  const Environment& env)
{
  const Location location{hash_value};
  // NOTE: FwiWeather is unused but could change this to try doing length to breadth ratio
  auto perim_grid = env.makeMap<unsigned char>(0);
  // want to find cells in the area that fill up the size we're looking for
  size_t count = 0;
  // convert into number of cells
  const auto num_cells = size / (100.0 * 100.0 / (perim_grid->cellSize() * perim_grid->cellSize()));
  auto max_distance = sqrt(num_cells / M_PI);
  perim_grid->set(hash_value, 1);
  ++count;
  // HACK: assume fuel for origin matches the rest of the fire
  while (num_cells > count)
  {
    const auto range = static_cast<Idx>(ceil(max_distance));
    for (auto x = -range; x <= range && num_cells > count; ++x)
    {
      for (auto y = -range; y <= range && num_cells > count; ++y)
      {
        // look at any cell that's within the range
        if (sqrt(util::pow_int<2>(x) + util::pow_int<2>(y)) < max_distance)
        {
          const auto r = static_cast<Idx>(location.row() + x);
          const auto c = static_cast<Idx>(location.column() + y);
          const auto loc = env.cell(r, c);
          const auto hash_value = loc.hash();
          if (1 != perim_grid->at(hash_value) && !fuel::is_null_fuel(loc))
          {
            perim_grid->set(hash_value, 1);
            ++count;
          }
        }
      }
    }
    max_distance += 0.1;
  }
  return BurnedMap(*perim_grid, env);
}
Perimeter::Perimeter(
  const HashSize hash_value,
  const size_t size,
  const Environment& env)
  : Perimeter(make_burned_map(hash_value, size, env))
{
}
}
