/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include "ConstantGrid.h"
#include <geo_normalize.h>
#include <tiffio.h>
#include <xtiffio.h>
#include "tiff.h"
namespace fs
{
template <class T>
int value_at_int(void* const buf, const FullIdx offset)
{
  auto cur = *(static_cast<T*>(buf) + offset);
  return static_cast<int>(cur);
};
// ConstantGrids are all int values of some kind
/**
 * \brief Read a section of a TIFF into a ConstantGrid
 * \param filename File name to read from
 * \param point Point to center ConstantGrid on
 * \return ConstantGrid containing clipped data for TIFF
 */
[[nodiscard]] ConstantGrid<int, int> readTiffInt(const string_view filename, const Point& point)
{
  GeoTiff geotiff{filename, "r"};
  auto tif = geotiff.tiff();
  auto gtif = geotiff.gtif();
#ifdef DEBUG_GRIDS
  auto min_value = std::numeric_limits<int>::max();
  auto max_value = std::numeric_limits<int>::min();
#endif
  logging::debug(
    "Raster type int has limits %ld, %ld",
    std::numeric_limits<int>::min(),
    std::numeric_limits<int>::max()
  );
  const GridBase grid_info = read_header(geotiff);
  uint32_t tile_width;
  uint32_t tile_length;
  TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tile_width);
  TIFFGetField(tif, TIFFTAG_TILELENGTH, &tile_length);
  void* data;
  uint32_t count;
  TIFFGetField(tif, TIFFTAG_GDAL_NODATA, &count, &data);
  logging::check_fatal(0 == count, "NODATA value is not set in input");
  logging::debug("NODATA value is '%s'", static_cast<char*>(data));
  const auto nodata_orig = stoi(string(static_cast<char*>(data)));
  logging::debug("NODATA value is originally parsed as %d", nodata_orig);
  const auto nodata_input = static_cast<int>(stoi(string(static_cast<char*>(data))));
  logging::debug("NODATA value is parsed as %d", nodata_input);
  auto actual_rows = grid_info.calculateRows();
  auto actual_columns = grid_info.calculateColumns();
  const auto coordinates = grid_info.findFullCoordinates(point, true);
  logging::note(
    "Coordinates before reading are (%d, %d => %f, %f)",
    std::get<0>(*coordinates),
    std::get<1>(*coordinates),
    std::get<0>(*coordinates) + std::get<2>(*coordinates) / 1000.0,
    std::get<1>(*coordinates) + std::get<3>(*coordinates) / 1000.0
  );
  auto min_column = max(
    static_cast<FullIdx>(0),
    static_cast<FullIdx>(
      std::get<1>(*coordinates) - static_cast<FullIdx>(MAX_COLUMNS) / static_cast<FullIdx>(2)
    )
  );
  if (min_column + MAX_COLUMNS >= actual_columns)
  {
    min_column = max(static_cast<FullIdx>(0), actual_columns - MAX_COLUMNS);
  }
  // make sure we're at the start of a tile
  const auto tile_column = tile_width * static_cast<FullIdx>(min_column / tile_width);
  const auto max_column = static_cast<FullIdx>(min(min_column + MAX_COLUMNS - 1, actual_columns));
#ifdef DEBUG_GRIDS
  logging::check_fatal(min_column < 0, "Column can't be less than 0");
  logging::check_fatal(
    max_column - min_column > MAX_COLUMNS, "Can't have more than %d columns", MAX_COLUMNS
  );
  logging::check_fatal(
    max_column > actual_columns, "Can't have more than actual %d columns", actual_columns
  );
#endif
  auto min_row = max(
    static_cast<FullIdx>(0),
    static_cast<FullIdx>(
      std::get<0>(*coordinates) - static_cast<FullIdx>(MAX_ROWS) / static_cast<FullIdx>(2)
    )
  );
  if (min_row + MAX_COLUMNS >= actual_rows)
  {
    min_row = max(static_cast<FullIdx>(0), actual_rows - MAX_ROWS);
  }
  const auto tile_row = tile_width * static_cast<FullIdx>(min_row / tile_width);
  const auto max_row = static_cast<FullIdx>(min(min_row + MAX_ROWS - 1, actual_rows));
#ifdef DEBUG_GRIDS
  logging::check_fatal(min_row < 0, "Row can't be less than 0 but is %d", min_row);
  logging::check_fatal(
    max_row - min_row > MAX_ROWS,
    "Can't have more than %d rows but have %d",
    MAX_ROWS,
    max_row - min_row
  );
  logging::check_fatal(max_row > actual_rows, "Can't have more than actual %d rows", actual_rows);
#endif
  vector<int> values(static_cast<size_t>(MAX_ROWS) * MAX_COLUMNS, nodata_input);
  logging::verbose("%s: malloc start", geotiff.filename());
  int bps = std::numeric_limits<int>::digits + (1 * std::numeric_limits<int>::is_signed);
  uint16_t sample_format;
  logging::check_fatal(
    !TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT, &sample_format),
    "Cannot determine TIFFTAG_SAMPLEFORMAT"
  );
  uint16_t bps_file;
  logging::check_fatal(
    !TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &bps_file), "Cannot determine TIFFTAG_BITSPERSAMPLE"
  );
  if (bps != bps_file)
  {
    logging::warning(
      "Raster %s type is not expected type (%d bits instead of %d)",
      geotiff.filename(),
      bps_file,
      bps
    );
  }
#ifdef DEBUG_GRIDS
  int bps_int16_t =
    std::numeric_limits<int16_t>::digits + (1 * std::numeric_limits<int16_t>::is_signed);
  logging::debug("Size of pointer to int is %ld vs %ld", sizeof(int16_t*), sizeof(int*));
  logging::debug(
    "Raster %s calculated bps for type int is %ld; tif says bps is %ld; int16_t is %ld",
    geotiff.filename(),
    bps,
    bps_file,
    bps_int16_t
  );
#endif
  const auto tile_size = TIFFTileSize(tif);
  logging::debug("Tile size for reading %s is %ld", geotiff.filename(), tile_size);
  const auto buf = _TIFFmalloc(tile_size);
  logging::verbose("%s: read start", geotiff.filename());
  const tsample_t smp{};
  logging::debug(
    "Want to clip grid to (%d, %d) => (%d, %d) for a %dx%d raster",
    min_row,
    min_column,
    max_row,
    max_column,
    actual_rows,
    actual_columns
  );
  // HACK: it really feels like there should be a better way to do this
  switch (sample_format)
  {
    case SAMPLEFORMAT_VOID:
      return logging::fatal<ConstantGrid<int, int>>("SAMPLEFORMAT_VOID unsupported");
    case SAMPLEFORMAT_UINT:
      break;
    case SAMPLEFORMAT_INT:
      break;
    case SAMPLEFORMAT_IEEEFP:
      return logging::fatal<ConstantGrid<int, int>>(
        "Expected integer raster but got SAMPLEFORMAT_IEEEFP"
      );
    case SAMPLEFORMAT_COMPLEXINT:
      return logging::fatal<ConstantGrid<int, int>>(
        "Expected integer raster but got SAMPLEFORMAT_COMPLEXINT"
      );
    case SAMPLEFORMAT_COMPLEXIEEEFP:
      return logging::fatal<ConstantGrid<int, int>>(
        "Expected integer raster but got SAMPLEFORMAT_COMPLEXIEEEFP"
      );
    default:
      return logging::fatal<ConstantGrid<int, int>>(
        "Unknown TIFFTAG_SAMPLEFORMAT value %d", sample_format
      );
  }
  auto value_at = [&]() {
    if (SAMPLEFORMAT_UINT == sample_format)
    {
      // unsigned int
      if (8 == bps_file)
      {
        return &value_at_int<uint8_t>;
      }
      if (16 == bps_file)
      {
        return &value_at_int<uint16_t>;
      }
      if (32 == bps_file)
      {
        return &value_at_int<uint32_t>;
      }
    }
    else if (SAMPLEFORMAT_INT == sample_format)
    {
      // signed int
      if (8 == bps_file)
      {
        return &value_at_int<int8_t>;
      }
      if (16 == bps_file)
      {
        return &value_at_int<int16_t>;
      }
      if (32 == bps_file)
      {
        return &value_at_int<int32_t>;
      }
    }
    // return &value_at_int<int32_t>;
    return logging::fatal<int (*)(void*, const FullIdx)>(
      "SAMPLEFORMAT %d has invalid TIFFTAG_BITSPERSAMPLE %d", sample_format, bps_file
    );
  }();
  for (auto h = tile_row; h <= max_row; h += tile_length)
  {
    for (auto w = tile_column; w <= max_column; w += tile_width)
    {
      tmsize_t t_size =
        TIFFReadTile(tif, buf, static_cast<uint32_t>(w), static_cast<uint32_t>(h), 0, smp);
      for (FullIdx y = 0; (y < static_cast<FullIdx>(tile_length)) && (y + h <= max_row); ++y)
      {
        // read in so that (0, 0) has a hash of 0
        const auto y_row = static_cast<HashSize>((h - min_row) + y);
        const auto actual_row = (max_row - min_row) - y_row;
        if (actual_row >= 0 && actual_row < MAX_ROWS)
        {
          for (auto x = 0; (x < static_cast<FullIdx>(tile_width)) && (x + w <= max_column); ++x)
          {
            const auto offset = y * tile_width + x;
            const auto actual_column = ((w - min_column) + x);
            if (actual_column >= 0 && actual_column < MAX_ROWS)
            {
              const auto cur_hash = actual_row * MAX_COLUMNS + actual_column;
              // auto cur = *(static_cast<int*>(buf) + offset);
              auto cur = value_at(buf, offset);
#ifdef DEBUG_GRIDS
              min_value = min(cur, min_value);
              max_value = max(cur, max_value);
#endif
              values.at(cur_hash) = cur;
            }
          }
        }
      }
    }
  }
  logging::verbose("%s: read end", geotiff.filename());
  _TIFFfree(buf);
  logging::verbose("%s: free end", geotiff.filename());
  const auto new_xll =
    grid_info.xllcorner() + (static_cast<MathSize>(min_column) * grid_info.cellSize());
  const auto new_yll =
    grid_info.yllcorner()
    + (static_cast<MathSize>(actual_rows) - static_cast<MathSize>(max_row)) * grid_info.cellSize();
#ifdef DEBUG_GRIDS
  logging::check_fatal(new_yll < grid_info.yllcorner(), "New yllcorner is outside original grid");
#endif
  logging::verbose(
    "Translated lower left is (%f, %f) from (%f, %f)",
    new_xll,
    new_yll,
    grid_info.xllcorner(),
    grid_info.yllcorner()
  );
  const auto num_rows = max_row - min_row + 1;
  const auto num_columns = max_column - min_column + 1;
  ConstantGrid<int, int> result{
    grid_info.cellSize(),
    static_cast<Idx>(num_rows),
    static_cast<Idx>(num_columns),
    nodata_input,
    nodata_input,
    new_xll,
    new_yll,
    new_xll + (static_cast<MathSize>(num_columns) + 1) * grid_info.cellSize(),
    new_yll + (static_cast<MathSize>(num_rows) + 1) * grid_info.cellSize(),
    string(grid_info.proj4()),
    std::move(values)
  };
  auto new_location = result.findCoordinates(point, false);
#ifdef DEBUG_GRIDS
  logging::check_fatal(nullptr == new_location, "Invalid location after reading");
#endif
  logging::note(
    "Coordinates are (%d, %d => %f, %f)",
    std::get<0>(*new_location),
    std::get<1>(*new_location),
    std::get<0>(*new_location) + std::get<2>(*new_location) / 1000.0,
    std::get<1>(*new_location) + std::get<3>(*new_location) / 1000.0
  );
#ifdef DEBUG_GRIDS
  logging::note("Values for %s range from %d to %d", filename_.c_str(), min_value, max_value);
#endif
  return result;
}
}
