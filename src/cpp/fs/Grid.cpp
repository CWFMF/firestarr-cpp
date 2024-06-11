/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "Grid.h"
#include "UTM.h"
using fs::Idx;
namespace fs
{
using fs::from_lat_long;
using fs::to_lat_long;
using fs::try_fix_meridian;
string find_value(const string& key, const string& within)
{
  const auto c = within.find(key);
  if (c != string::npos)
  {
    const string str = &within.at(c + string(key).length());
    return str.substr(0, str.find(' '));
  }
  return "";
}
int str_to_int(const string& str) { return stoi(str); }
MathSize str_to_value(const string& str) { return static_cast<MathSize>(stod(str)); }
template <class T>
bool find_value(const string& key, const string& within, T* result, T (*convert)(const string& str))
{
  const auto str = find_value(key, within);
  if (!str.empty())
  {
    *result = convert(str);
    logging::extensive("%s '%s'\n", string(key).c_str(), str.c_str());
    return true;
  }
  return false;
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
[[nodiscard]] GridBase read_header(TIFF* tif, GTIF* gtif)
{
  GTIFDefn definition;
  if (GTIFGetDefn(gtif, &definition))
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
    auto proj4_char = unique_ptr<char>(GTIFGetProj4Defn(&definition));
    auto proj4 = string(proj4_char.get());
    const auto xurcorner = xllcorner + cell_width * columns;
    const auto yurcorner = yllcorner + cell_width * rows;
    return {cell_width, xllcorner, yllcorner, xurcorner, yurcorner, string(proj4)};
  }
  throw runtime_error("Cannot read TIFF header");
}
[[nodiscard]] GridBase read_header(const string_view filename)
{
  return with_tiff<GridBase>(filename, [](TIFF* tif, GTIF* gtif) {
    return read_header(tif, gtif);
  });
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
