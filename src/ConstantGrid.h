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

#pragma once
#include <algorithm>
#include <string>
#include <utility>
#include <vector>
#include "Grid.h"
#include "Util.h"
namespace firestarr
{
namespace data
{
/**
 * \brief A GridData<T, V, const vector<T>> that cannot change once initialized.
 * \tparam T The initialization value type.
 * \tparam V The initialized value type.
 */
template <class T, class V = T>
class ConstantGrid
  : public GridData<T, V, const vector<T>>
{
public:
  /**
   * \brief Value for grid at given Location.
   * \param location Location to get value for.
   * \return Value at grid Location.
   */
  [[nodiscard]] constexpr T at(const Location& location) const noexcept override
  {
    return at(location.hash());
  }
  /**
   * \brief Value for grid at given Location.
   * \param hash HashSize hash for Location to get value for.
   * \return Value at grid Location.
   */
  [[nodiscard]] constexpr T at(const HashSize hash) const noexcept
  {
    return this->data.at(hash);
  }
  /**
   * \brief Throw an error because ConstantGrid can't change values.
   */
  // ! @cond Doxygen_Suppress
  void set(const Location&, const T) override
  // ! @endcond
  {
    throw runtime_error("Cannot change ConstantGrid");
  }
  ~ConstantGrid() = default;
  ConstantGrid(const ConstantGrid& rhs) noexcept = delete;
  ConstantGrid(ConstantGrid&& rhs) noexcept = delete;
  ConstantGrid& operator=(const ConstantGrid& rhs) noexcept = delete;
  ConstantGrid& operator=(ConstantGrid&& rhs) noexcept = delete;
  /**
   * \brief Constructor
   * \param cell_size Cell width and height (m)
   * \param rows Number of rows
   * \param columns Number of columns
   * \param no_data Value to use for no data
   * \param nodata Integer value that represents no data
   * \param xllcorner Lower left corner X coordinate (m)
   * \param yllcorner Lower left corner Y coordinate (m)
   * \param xurcorner Upper right corner X coordinate (m)
   * \param yurcorner Upper right corner Y coordinate (m)
   * \param proj4 Proj4 projection definition
   * \param data Data to set as grid data
   */
  ConstantGrid(const double cell_size,
               const Idx rows,
               const Idx columns,
               const T no_data,
               const int nodata,
               const double xllcorner,
               const double yllcorner,
               const double xurcorner,
               const double yurcorner,
               string&& proj4,
               vector<T>&& data)
    : GridData<T, V, const vector<T>>(cell_size,
                                      rows,
                                      columns,
                                      no_data,
                                      nodata,
                                      xllcorner,
                                      yllcorner,
                                      xurcorner,
                                      yurcorner,
                                      std::forward<string>(proj4),
                                      std::move(data))
  {
  }
  /**
   * \brief Constructor
   * \param cell_size Cell width and height (m)
   * \param rows Number of rows
   * \param columns Number of columns
   * \param no_data Value to use for no data
   * \param nodata Integer value that represents no data
   * \param xllcorner Lower left corner X coordinate (m)
   * \param yllcorner Lower left corner Y coordinate (m)
   * \param proj4 Proj4 projection definition
   * \param initialization_value Value to initialize entire grid with
   */
  ConstantGrid(const double cell_size,
               const Idx rows,
               const Idx columns,
               const T& no_data,
               const int nodata,
               const double xllcorner,
               const double yllcorner,
               const string& proj4,
               const T& initialization_value) noexcept
    : ConstantGrid(cell_size,
                   rows,
                   columns,
                   no_data,
                   nodata,
                   xllcorner,
                   yllcorner,
                   proj4,
                   std::move(vector<T>(static_cast<size_t>(rows) * MAX_COLUMNS,
                                       initialization_value)))
  {
  }
  /**
   * \brief Read a section of a TIFF into a ConstantGrid
   * \param filename File name to read from
   * \param tif Pointer to open TIFF denoted by filename
   * \param gtif Pointer to open geotiff denoted by filename
   * \param point Point to center ConstantGrid on
   * \param convert Function taking int and nodata int value that returns T
   * \return ConstantGrid containing clipped data for TIFF
   */
  [[nodiscard]] static ConstantGrid<T, V>* readTiff(const string& filename,
                                                    TIFF* tif,
                                                    GTIF* gtif,
                                                    const topo::Point& point,
                                                    std::function<T(int, int)> convert)
  {
    logging::info("Reading file %s", filename.c_str());
#ifndef NDEBUG
    auto min_value = std::numeric_limits<int16>::max();
    auto max_value = std::numeric_limits<int16>::min();
#endif
    const GridBase grid_info = read_header<T>(tif, gtif);
    const auto coordinates = grid_info.findFullCoordinates(point, false);
    auto min_column = max(static_cast<FullIdx>(0),
                          static_cast<FullIdx>(std::get<1>(*coordinates) - static_cast<FullIdx>(MAX_COLUMNS) / static_cast<FullIdx>(2)));
    if (min_column + MAX_COLUMNS >= grid_info.calculateColumns())
    {
      min_column = grid_info.calculateColumns() - MAX_COLUMNS;
    }
    const auto max_column = static_cast<FullIdx>(min_column + MAX_COLUMNS);
    auto min_row = max(static_cast<FullIdx>(0),
                       static_cast<FullIdx>(std::get<0>(*coordinates) - static_cast<FullIdx>(MAX_ROWS) / static_cast<FullIdx>(2)));
    if (min_row + MAX_COLUMNS >= grid_info.calculateRows())
    {
      min_row = grid_info.calculateRows() - MAX_COLUMNS;
    }
    const auto max_row = static_cast<FullIdx>(min_row + MAX_COLUMNS);
    const FullIdx offset_x = -min_column;
    const FullIdx offset_y = -min_row;
    T no_data = convert(grid_info.nodata(), grid_info.nodata());
    vector<T> values(MAX_COLUMNS * MAX_COLUMNS, no_data);
    int tile_width;
    int tile_length;
    TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tile_width);
    TIFFGetField(tif, TIFFTAG_TILELENGTH, &tile_length);
    logging::warning("%s: malloc start", filename.c_str());
    const auto buf = _TIFFmalloc(TIFFTileSize(tif));
    logging::warning("%s: read start", filename.c_str());
    const tsample_t smp{};
    for (auto h = 0; h < grid_info.calculateRows(); h += tile_length)
    {
      const auto y_min = static_cast<FullIdx>(max(static_cast<FullIdx>(0), min_row - h));
      const auto y_limit = min(static_cast<FullIdx>(tile_length),
                               min(static_cast<FullIdx>(grid_info.calculateRows()), max_row) - h);
      for (auto w = 0; w < grid_info.calculateColumns(); w += tile_width)
      {
        TIFFReadTile(tif, buf, static_cast<uint32>(w), static_cast<uint32>(h), 0, smp);
        const auto x_min = static_cast<FullIdx>(max(static_cast<FullIdx>(0), min_column - w));
        const auto x_limit = min(static_cast<FullIdx>(tile_width),
                                 min(grid_info.calculateColumns(),
                                     max_column)
                                   - w);
        for (auto y = y_min; y < y_limit; ++y)
        {
          // read in so that (0, 0) has a hash of 0
          const FullIdx i = static_cast<FullIdx>(MAX_COLUMNS) - (static_cast<FullIdx>(h) + y + offset_y + 1);
          for (auto x = x_min; x < x_limit; ++x)
          {
            const auto cur_hash = static_cast<HashSize>(i) * MAX_COLUMNS + w + x + offset_x;
            const auto offset = y * tile_length + x;
            auto cur = *(static_cast<int16*>(buf) + offset);
#ifndef NDEBUG
            min_value = min(cur, min_value);
            max_value = max(cur, max_value);
#endif
            values.at(cur_hash) = convert(cur, grid_info.nodata());
          }
        }
      }
    }
    logging::warning("%s: read end", filename.c_str());
    _TIFFfree(buf);
    logging::warning("%s: free end", filename.c_str());
    const auto new_xll = grid_info.xllcorner() - offset_x * grid_info.cellSize();
    const auto new_yll = grid_info.yllcorner()
                       + (static_cast<double>(grid_info.calculateRows()) - max_row)
                           * grid_info.cellSize();
    logging::check_fatal(new_yll < grid_info.yllcorner(),
                         "New yllcorner is outside original grid");
    logging::note("Translated lower left is (%f, %f) from (%f, %f)",
                  new_xll,
                  new_yll,
                  grid_info.xllcorner(),
                  grid_info.yllcorner());
    auto result = new ConstantGrid<T, V>(grid_info.cellSize(),
                                         MAX_ROWS,
                                         MAX_COLUMNS,
                                         no_data,
                                         grid_info.nodata(),
                                         new_xll,
                                         new_yll,
                                         new_xll + MAX_COLUMNS * grid_info.cellSize(),
                                         new_yll + MAX_ROWS * grid_info.cellSize(),
                                         string(grid_info.proj4()),
                                         std::move(values));
    auto new_location = result->findCoordinates(point, true);
    logging::check_fatal(nullptr == new_location, "Invalid location after reading");
    logging::note("Coordinates are (%d, %d => %f, %f)",
                  std::get<0>(*new_location),
                  std::get<1>(*new_location),
                  std::get<0>(*new_location) + std::get<3>(*new_location) / 1000.0,
                  std::get<1>(*new_location) + std::get<2>(*new_location) / 1000.0);
#ifndef NDEBUG
    logging::note("Values for %s range from %d to %d",
                  filename.c_str(),
                  min_value,
                  max_value);
#endif
    return result;
  }
  /**
   * \brief Read a section of a TIFF into a ConstantGrid
   * \param filename File name to read from
   * \param point Point to center ConstantGrid on
   * \param convert Function taking int and nodata int value that returns T
   * \return ConstantGrid containing clipped data for TIFF
   */
  [[nodiscard]] static ConstantGrid<T, V>* readTiff(const string& filename,
                                                    const topo::Point& point,
                                                    std::function<T(int, int)> convert)
  {
    return with_tiff<ConstantGrid<T, V>*>(
      filename,
      [&filename, &convert, &point](TIFF* tif, GTIF* gtif)
      {
        return readTiff(filename, tif, gtif, point, convert);
      });
  }
  /**
   * \brief Read a section of a TIFF into a ConstantGrid
   * \param filename File name to read from
   * \param point Point to center ConstantGrid on
   * \return ConstantGrid containing clipped data for TIFF
   */
  [[nodiscard]] static ConstantGrid<T, T>* readTiff(const string& filename,
                                                    const topo::Point& point)
  {
    return readTiff(filename, point, util::no_convert<T>);
  }
  /**
   * \brief Save contents to .asc file
   * \param dir Directory to save into
   * \param base_name File base name to use
   */
  void saveToAsciiFile(const string& dir,
                       const string& base_name) const
  {
    saveToAsciiFile<V>(dir, base_name, [](V value)
                       {
                         return value;
                       });
  }
  /**
   * \brief Save contents to .asc file
   * \tparam R Type to be written to .asc file
   * \param dir Directory to save into
   * \param base_name File base name to use
   * \param convert Function to convert from V to R
   */
  template <class R>
  void saveToAsciiFile(const string& dir,
                       const string& base_name,
                       std::function<R(T value)> convert) const
  {
    Idx min_row = 0;
    Idx num_rows = MAX_ROWS;
    Idx min_column = 0;
    Idx num_columns = MAX_COLUMNS;
    const double xll = this->xllcorner() + min_column * this->cellSize();
    // offset is different for y since it's flipped
    const double yll = this->yllcorner() + (min_row) * this->cellSize();
    logging::verbose("Lower left corner is (%f, %f)", xll, yll);
    ofstream out;
    out.open(dir + base_name + ".asc");
    write_ascii_header(out,
                       num_columns,
                       num_rows,
                       xll,
                       yll,
                       this->cellSize(),
                       static_cast<double>(this->noDataInt()));
    for (Idx ro = 0; ro < num_rows; ++ro)
    {
      // HACK: do this so that we always get at least one pixel in output
      // need to output in reverse order since (0,0) is bottom left
      const Idx r = num_rows - 1 - ro;
      for (Idx co = 0; co < num_columns; ++co)
      {
        const Location idx(static_cast<Idx>(r), static_cast<Idx>(min_column + co));
        // HACK: use + here so that it gets promoted to a printable number
        //       prevents char type being output as characters
        out << +(convert(this->at(idx)))
            << " ";
      }
      out << "\n";
    }
    out.close();
    this->createPrj(dir, base_name);
  }
private:
  /**
   * \brief Constructor
   * \param grid_info GridBase defining Grid area
   * \param no_data Value that represents no data
   * \param values Values to initialize grid with
   */
  ConstantGrid(const GridBase& grid_info, const T& no_data, vector<T>&& values)
    : ConstantGrid(grid_info.cellSize(),
                   no_data,
                   grid_info.nodata(),
                   grid_info.xllcorner(),
                   grid_info.yllcorner(),
                   grid_info.xurcorner(),
                   grid_info.yurcorner(),
                   string(grid_info.proj4()),
                   std::move(values))
  {
    logging::check_fatal(
      this->data.size() != static_cast<size_t>(grid_info.calculateRows()) * MAX_COLUMNS,
      "Invalid grid size");
  }
};
}
}
