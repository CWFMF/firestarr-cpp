/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include "Grid.h"
#include <geo_normalize.h>
#include <tiffio.h>
#include <xtiffio.h>
#include "Log.h"
#include "tiff.h"
#include "UTM.h"
using fs::Idx;
namespace fs
{
using fs::find_value;
using fs::from_lat_long;
using fs::to_lat_long;
using fs::try_fix_meridian;
template <class R>
string saveToTiffFile(
  const GridBase& grid,
  const Idx columns,
  const Idx rows,
  const tuple<Idx, Idx, Idx, Idx> bounds,
  const string_view dir,
  const string_view base_name,
  const uint16_t bits_per_sample,
  const uint16_t sample_format,
  std::function<R(Location)> value_at,
  const int nodata_as_int
)
{
  uint32_t tileWidth = min(static_cast<int>(columns), 256);
  uint32_t tileHeight = min(static_cast<int>(rows), 256);
  auto min_column = std::get<0>(bounds);
  auto min_row = std::get<1>(bounds);
  auto max_column = std::get<2>(bounds);
  auto max_row = std::get<3>(bounds);
  logging::check_fatal(
    min_column > max_column, "Invalid bounds for columns with %d => %d", min_column, max_column
  );
  logging::check_fatal(
    min_row > max_row, "Invalid bounds for rows with %d => %d", min_row, max_row
  );
#ifdef DEBUG_GRIDS
  logging::debug(
    "Bounds are (%d, %d), (%d, %d) initially", min_column, min_row, max_column, max_row
  );
#endif
  Idx c_min = 0;
  while (c_min + static_cast<Idx>(tileWidth) <= min_column)
  {
    c_min += static_cast<Idx>(tileWidth);
  }
  Idx c_max = c_min + static_cast<Idx>(tileWidth);
  while (c_max < max_column)
  {
    c_max += static_cast<Idx>(tileWidth);
  }
  min_column = c_min;
  max_column = c_max;
  Idx r_min = 0;
  while (r_min + static_cast<Idx>(tileHeight) <= min_row)
  {
    r_min += static_cast<Idx>(tileHeight);
  }
  Idx r_max = r_min + static_cast<Idx>(tileHeight);
  while (r_max < max_row)
  {
    r_max += static_cast<Idx>(tileHeight);
  }
  min_row = r_min;
  max_row = r_max;
  logging::check_fatal(
    min_column >= max_column, "Invalid bounds for columns with %d => %d", min_column, max_column
  );
  logging::check_fatal(
    min_row >= max_row, "Invalid bounds for rows with %d => %d", min_row, max_row
  );
#ifdef DEBUG_GRIDS
  logging::debug(
    "Bounds are (%d, %d), (%d, %d) after correction", min_column, min_row, max_column, max_row
  );
#endif
  logging::extensive("(%d, %d) => (%d, %d)", min_column, min_row, max_column, max_row);
  logging::check_fatal((max_row - min_row) % tileHeight != 0, "Invalid start and end rows");
  logging::check_fatal(
    (max_column - min_column) % tileHeight != 0, "Invalid start and end columns"
  );
  logging::extensive("Lower left corner is (%d, %d)", min_column, min_row);
  logging::extensive("Upper right corner is (%d, %d)", max_column, max_row);
  const MathSize xll = grid.xllcorner() + min_column * grid.cellSize();
  // offset is different for y since it's flipped
  const MathSize yll = grid.yllcorner() + (min_row)*grid.cellSize();
  logging::extensive("Lower left corner is (%f, %f)", xll, yll);
  const auto num_rows = static_cast<size_t>(max_row - min_row);
  const auto num_columns = static_cast<size_t>(max_column - min_column);
  // ensure this is always divisible by tile size
  logging::check_fatal(0 != (num_rows % tileWidth), "%d rows not divisible by tiles", num_rows);
  logging::check_fatal(
    0 != (num_columns % tileHeight), "%d columns not divisible by tiles", num_columns
  );
  const auto filename = create_file_name(dir, base_name, "tif");
  GeoTiff geotiff{filename, "w"};
  auto tif = geotiff.tiff();
  auto gtif = geotiff.gtif();
  logging::check_fatal(!gtif, "Cannot open file %s as a GEOTIFF", filename.c_str());
  const double xul = xll;
  const double yul = grid.yllcorner() + (grid.cellSize() * max_row);
  double tiePoints[6] = {0.0, 0.0, 0.0, xul, yul, 0.0};
  double pixelScale[3] = {grid.cellSize(), grid.cellSize(), 0.0};
  // make sure to use floating point if values are
  TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT, sample_format);
  // FIX: was using double, and that usually doesn't make sense, but sometime it might?
  // use buffer big enought to fit any (R  + '.000\0') + 1
  constexpr auto n = std::numeric_limits<R>::digits10;
  static_assert(n > 0);
  char str[n + 6]{0};
  sxprintf(str, "%d.000", nodata_as_int);
  // logging::extensive(
  //   "%s using nodata string '%s' for nodata value of (%d, %f)",
  //   typeid(&grid).name(),
  //   str,
  //   nodata_as_int,
  //   static_cast<MathSize>(no_data)
  // );
  TIFFSetField(tif, TIFFTAG_GDAL_NODATA, str);
  logging::extensive("%s takes %d bits", string(base_name).c_str(), bits_per_sample);
  TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, num_columns);
  TIFFSetField(tif, TIFFTAG_IMAGELENGTH, num_rows);
  TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 1);
  TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bits_per_sample);
  TIFFSetField(tif, TIFFTAG_TILEWIDTH, tileWidth);
  TIFFSetField(tif, TIFFTAG_TILELENGTH, tileHeight);
  TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
  GTIFSetFromProj4(gtif, grid.proj4().c_str());
  TIFFSetField(tif, TIFFTAG_GEOTIEPOINTS, 6, tiePoints);
  TIFFSetField(tif, TIFFTAG_GEOPIXELSCALE, 3, pixelScale);
  size_t tileSize = tileWidth * tileHeight;
  const auto buf_size = tileSize * sizeof(R);
  logging::extensive("%s has buffer size %d", string(base_name).c_str(), buf_size);
  auto buf = static_cast<R*>(_TIFFmalloc(buf_size));
  for (uint32_t co = 0; co < num_columns; co += tileWidth)
  {
    for (uint32_t ro = 0; ro < num_rows; ro += tileHeight)
    {
      std::fill_n(&buf[0], tileWidth * tileHeight, static_cast<R>(nodata_as_int));
      // NOTE: shouldn't need to check if writing outside of tile because we made bounds on tile
      // edges above need to put data from grid into buffer, but flipped vertically
      for (uint32_t x = 0; x < tileWidth; ++x)
      {
        for (uint32_t y = 0; y < tileHeight; ++y)
        {
          const Idx r = static_cast<Idx>(max_row) - (ro + y + 1);
          const Idx c = static_cast<Idx>(min_column) + co + x;
          const Location idx(r, c);
          // might be out of bounds if not divisible by number of tiles
          if (!(rows <= r || 0 > r || columns <= c || 0 > c))
          {
            // HACK: was getting invalid rasters if assigning directly into buf
            const R value = value_at(idx);
            buf[x + y * tileWidth] = value;
          }
        }
      }
      logging::check_fatal(
        TIFFWriteTile(tif, buf, co, ro, 0, 0) < 0, "Cannot write tile to %s", filename.c_str()
      );
    }
  }
  GTIFWriteKeys(gtif);
  _TIFFfree(buf);
  return filename;
}
string GridBase::saveToTiffFileInt(
  const Idx columns,
  const Idx rows,
  const tuple<Idx, Idx, Idx, Idx> bounds,
  const string_view dir,
  const string_view base_name,
  const uint16_t bits_per_sample,
  // const uint16_t sample_format,
  const bool is_unsigned,
  std::function<int(Location)> value_at,
  const int nodata_as_int
) const
{
  // logging::check_fatal(
  //   SAMPLEFORMAT_INT != sample_format && SAMPLEFORMAT_UINT != sample_format,
  //   "Expected int for SAMPLEFORMAT but got value %d",
  //   bits_per_sample
  // );
  logging::check_fatal(
    8 != bits_per_sample && 16 != bits_per_sample && 32 != bits_per_sample,
    "Expected integer to have 8, 16, or 32 bits but got %d",
    bits_per_sample
  );
  const auto sample_format = is_unsigned ? SAMPLEFORMAT_UINT : SAMPLEFORMAT_INT;
  if (is_unsigned)
  {
    // unsigned int
    if (8 == bits_per_sample)
    {
      return saveToTiffFile<uint8_t>(
        *this,
        columns,
        rows,
        bounds,
        dir,
        base_name,
        bits_per_sample,
        sample_format,
        [&](Location idx) { return static_cast<uint8_t>(value_at(idx)); },
        nodata_as_int
      );
    }
    if (16 == bits_per_sample)
    {
      return saveToTiffFile<uint16_t>(
        *this,
        columns,
        rows,
        bounds,
        dir,
        base_name,
        bits_per_sample,
        sample_format,
        [&](Location idx) { return static_cast<uint16_t>(value_at(idx)); },
        nodata_as_int
      );
    }
    if (32 == bits_per_sample)
    {
      return saveToTiffFile<uint32_t>(
        *this,
        columns,
        rows,
        bounds,
        dir,
        base_name,
        bits_per_sample,
        sample_format,
        [&](Location idx) { return static_cast<uint32_t>(value_at(idx)); },
        nodata_as_int
      );
    }
  }
  else
  {
    // signed int
    if (8 == bits_per_sample)
    {
      return saveToTiffFile<int8_t>(
        *this,
        columns,
        rows,
        bounds,
        dir,
        base_name,
        bits_per_sample,
        sample_format,
        [&](Location idx) { return static_cast<int8_t>(value_at(idx)); },
        nodata_as_int
      );
    }
    if (16 == bits_per_sample)
    {
      return saveToTiffFile<int16_t>(
        *this,
        columns,
        rows,
        bounds,
        dir,
        base_name,
        bits_per_sample,
        sample_format,
        [&](Location idx) { return static_cast<int16_t>(value_at(idx)); },
        nodata_as_int
      );
    }
    if (32 == bits_per_sample)
    {
      return saveToTiffFile<int32_t>(
        *this,
        columns,
        rows,
        bounds,
        dir,
        base_name,
        bits_per_sample,
        sample_format,
        [&](Location idx) { return static_cast<int32_t>(value_at(idx)); },
        nodata_as_int
      );
    }
  }
  return logging::fatal<string>(
    "Invalid combination of BITSPERSAMPLE (%d) and SAMPLEFORMAT (%d)",
    bits_per_sample,
    sample_format
  );
}
string GridBase::saveToTiffFileFloat(
  const Idx columns,
  const Idx rows,
  const tuple<Idx, Idx, Idx, Idx> bounds,
  const string_view dir,
  const string_view base_name,
  // const uint16_t bits_per_sample,
  // const uint16_t sample_format,
  std::function<double(Location)> value_at,
  const int nodata_as_int
) const
{
  constexpr auto bits_per_sample = 16;
  constexpr auto sample_format = SAMPLEFORMAT_IEEEFP;
  if (16 == bits_per_sample)
  {
    return saveToTiffFile<float>(
      *this,
      columns,
      rows,
      bounds,
      dir,
      base_name,
      bits_per_sample,
      sample_format,
      [&](Location idx) { return static_cast<float>(value_at(idx)); },
      nodata_as_int
    );
  }
  return logging::fatal<string>(
    "Invalid combination of BITSPERSAMPLE (%d) and SAMPLEFORMAT (%d)",
    bits_per_sample,
    sample_format
  );
}
GridBase::GridBase(
  const MathSize cell_size,
  const MathSize xllcorner,
  const MathSize yllcorner,
  const MathSize xurcorner,
  const MathSize yurcorner,
  const string_view proj4
) noexcept
  : proj4_(try_fix_meridian(proj4)), cell_size_(cell_size), xllcorner_(xllcorner),
    yllcorner_(yllcorner), xurcorner_(xurcorner), yurcorner_(yurcorner)
{ }
void GridBase::createPrj(const string_view dir, const string_view base_name) const
{
  const auto filename = string(dir) + string(base_name) + ".prj";
  FILE* out = fopen(filename.c_str(), "w");
  logging::extensive(proj4_.c_str());
  // HACK: use what we know is true for the grids that were generated and
  // hope it's correct otherwise
  const auto proj = find_value("+proj=", proj4_);
  // HACK: expect zone to already be converted to meridian in proj4 definition
  const auto lon_0 = find_value("+lon_0=", proj4_);
  const auto lat_0 = find_value("+lat_0=", proj4_);
  const auto k = find_value("+k=", proj4_);
  const auto x_0 = find_value("+x_0=", proj4_);
  const auto y_0 = find_value("+y_0=", proj4_);
  if (proj.empty() || k.empty() || lat_0.empty() || lon_0.empty() || x_0.empty() || y_0.empty())
  {
    logging::fatal("Cannot convert proj4 '%s' into .prj file", proj4_.c_str());
  }
  fprintf(out, "Projection    TRANSVERSE\n");
  fprintf(out, "Datum         AI_CSRS\n");
  fprintf(out, "Spheroid      GRS80\n");
  // NOTE: uses American spelling
  fprintf(out, "Units         METERS\n");
  fprintf(out, "Zunits        NO\n");
  fprintf(out, "Xshift        0.0\n");
  fprintf(out, "Yshift        0.0\n");
  fprintf(out, "Parameters    \n");
  // NOTE: it seems like comments in .prj files is no longer supported?
  // these are all just strings since they're out of the proj4 string
  //   - but convert to number so precision matches old output
  // HACK: this is the order they were previously, so not just outputting in order they occur
  fprintf(out, "%g /* scale factor at central meridian\n", stod(k));
  fprintf(out, "%g  0  0.0 /* longitude of central meridian\n", stod(lon_0));
  fprintf(out, "%4g  0  0.0 /* latitude of origin\n", stod(lat_0));
  // NOTE: uses American spelling
  fprintf(out, "%6.1f /* false easting (meters)\n", stod(x_0));
  fprintf(out, "%0.1f /* false northing (meters)\n", stod(y_0));
  fclose(out);
}
unique_ptr<Coordinates> GridBase::findCoordinates(const Point& point, const bool flipped) const
{
  auto full = findFullCoordinates(point, flipped);
  return make_unique<Coordinates>(
    static_cast<Idx>(std::get<0>(*full)),
    static_cast<Idx>(std::get<1>(*full)),
    std::get<2>(*full),
    std::get<3>(*full)
  );
}
// Use pair instead of Location, so we can go above max columns & rows
unique_ptr<FullCoordinates> GridBase::findFullCoordinates(const Point& point, const bool flipped)
  const
{
  MathSize x;
  MathSize y;
  from_lat_long(this->proj4_, point, &x, &y);
  logging::debug(
    "Coordinates (%f, %f) converted to (%f, %f)", point.latitude(), point.longitude(), x, y
  );
  // check that north is the top of the raster at least along center
  const auto x_mid = (xllcorner_ + xurcorner_) / 2.0;
  Point south = to_lat_long(proj4_, x_mid, yllcorner_);
  Point north = to_lat_long(proj4_, x_mid, yurcorner_);
  auto x_s = static_cast<MathSize>(0.0);
  auto y_s = static_cast<MathSize>(0.0);
  from_lat_long(this->proj4_, south, &x_s, &y_s);
  auto x_n = static_cast<MathSize>(0.0);
  auto y_n = static_cast<MathSize>(0.0);
  from_lat_long(this->proj4_, north, &x_n, &y_n);
  // FIX: how different is too much?
  constexpr MathSize MAX_DEVIATION = 0.001;
  const auto deviation = x_n - x_s;
  if (abs(deviation) > MAX_DEVIATION)
  {
    logging::note(
      "Due north is not the top of the raster for (%f, %f) with proj4 '%s' - %f vs %f gives deviation of %f degrees which exceeds maximum of %f degrees",
      point.latitude(),
      point.longitude(),
      this->proj4_.c_str(),
      x_n,
      x_s,
      deviation,
      MAX_DEVIATION
    );
    return nullptr;
  }
  else if (abs(deviation * 10) > MAX_DEVIATION)
  {
    // if we're within an order of magnitude of an unacceptable deviation then warn about it
    logging::warning(
      "Due north deviates by %f degrees from South to North along the middle of the raster",
      deviation
    );
  }
  logging::verbose("Lower left is (%f, %f)", this->xllcorner_, this->yllcorner_);
  // convert coordinates into cell position
  const auto actual_x = (x - this->xllcorner_) / this->cell_size_;
  // these are already flipped across the y-axis on reading, so it's the same as for x now
  auto actual_y =
    (!flipped) ? (y - this->yllcorner_) / this->cell_size_ : (yurcorner_ - y) / cell_size_;
  const auto column = static_cast<FullIdx>(actual_x);
  const auto row = static_cast<FullIdx>(round(actual_y - 0.5));
  if (0 > column || column >= calculateColumns() || 0 > row || row >= calculateRows())
  {
    logging::verbose(
      "Returning nullptr from findFullCoordinates() for (%f, %f) => (%d, %d)",
      actual_x,
      actual_y,
      column,
      row
    );
    return nullptr;
  }
  const auto sub_x = static_cast<SubSize>((actual_x - column) * 1000);
  const auto sub_y = static_cast<SubSize>((actual_y - row) * 1000);
  return make_unique<FullCoordinates>(
    static_cast<FullIdx>(row), static_cast<FullIdx>(column), sub_x, sub_y
  );
}
void write_ascii_header(
  ofstream& out,
  const MathSize num_columns,
  const MathSize num_rows,
  const MathSize xll,
  const MathSize yll,
  const MathSize cell_size,
  const MathSize no_data
)
{
  out << "ncols         " << num_columns << "\n";
  out << "nrows         " << num_rows << "\n";
  out << "xllcorner     " << fixed << setprecision(6) << xll << "\n";
  out << "yllcorner     " << fixed << setprecision(6) << yll << "\n";
  out << "cellsize      " << cell_size << "\n";
  out << "NODATA_value  " << no_data << "\n";
}
[[nodiscard]] GridBase read_header(GeoTiff& geotiff)
{
  auto tif = geotiff.tiff();
  auto gtif = geotiff.gtif();
  GTIFDefn definition;
  if (GTIFGetDefn(geotiff.gtif(), &definition))
  {
    uint32_t columns;
    uint32_t rows;
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &columns);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &rows);
    double x = 0.0;
    double y = rows;
    logging::check_fatal(
      !GTIFImageToPCS(gtif, &x, &y), "Unable to translate image to PCS coordinates."
    );
    const auto yllcorner = y;
    const auto xllcorner = x;
    logging::debug("Lower left for header is (%f, %f)", xllcorner, yllcorner);
    double adf_coefficient[6] = {0};
    x = 0.5;
    y = 0.5;
    logging::check_fatal(
      !GTIFImageToPCS(gtif, &x, &y), "Unable to translate image to PCS coordinates."
    );
    adf_coefficient[4] = x;
    adf_coefficient[5] = y;
    x = 1.5;
    y = 0.5;
    logging::check_fatal(
      !GTIFImageToPCS(gtif, &x, &y), "Unable to translate image to PCS coordinates."
    );
    const auto cell_width = x - adf_coefficient[4];
    x = 0.5;
    y = 1.5;
    logging::check_fatal(
      !GTIFImageToPCS(gtif, &x, &y), "Unable to translate image to PCS coordinates."
    );
    const auto cell_height = y - adf_coefficient[5];
    logging::check_fatal(cell_width != -cell_height, "Can only use grids with square pixels");
    logging::debug("Cell size is %f", cell_width);
    // HACK: GTIFGetProj4Defn uses malloc
    std::unique_ptr<char, decltype(std::free)*> proj4_char{
      GTIFGetProj4Defn(&definition), std::free
    };
    auto proj4 = string(proj4_char.get());
    const auto xurcorner = xllcorner + cell_width * columns;
    const auto yurcorner = yllcorner + cell_width * rows;
    return {cell_width, xllcorner, yllcorner, xurcorner, yurcorner, string(proj4)};
  }
  throw runtime_error("Cannot read TIFF header");
}
[[nodiscard]] GridBase read_header(const string_view filename)
{
  GeoTiff geotiff{filename, "r"};
  return read_header(geotiff);
}
string create_file_name(
  const string_view dir,
  const string_view base_name,
  const string_view extension
)
{
  return string(dir) + string(base_name) + "." + string(extension);
}
}
