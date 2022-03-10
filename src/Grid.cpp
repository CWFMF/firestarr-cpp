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
#include "Grid.h"
#include "UTM.h"
using tbd::Idx;
namespace tbd::data
{
string find_value(const string& key, const string& within)
{
  const auto c = within.find(key);
  if (c != string::npos)
  {
    const string str = &within.c_str()[c + string(key).length()];
    return str.substr(0, str.find(' '));
  }
  return "";
}
int str_to_int(const string& str)
{
  return stoi(str);
}
double str_to_double(const string& str)
{
  return stod(str);
}
template <class T>
bool find_value(const string& key,
                const string& within,
                T* result,
                T (*convert)(const string& str))
{
  const auto str = find_value(key, within);
  if (!str.empty())
  {
    *result = convert(str);
    logging::extensive("%s '%s'\n", key.c_str(), str.c_str());
    return true;
  }
  return false;
}
double find_meridian(const string& proj4) noexcept
{
  try
  {
    int zone{};
    auto meridian = 0.0;
    if (find_value("+zone=", proj4, &zone, &str_to_int))
    {
      meridian = topo::utm_central_meridian_deg(zone);
    }
    else if (!find_value("+lon_0=", proj4, &meridian, &str_to_double))
    {
      logging::fatal("Can't find meridian for grid");
    }
    return meridian;
  }
  catch (...)
  {
    return logging::fatal<double>("Unable to parse meridian in string '%s'",
                                  proj4.c_str());
  }
}
GridBase::GridBase(const double cell_size,
                   const int nodata,
                   const double xllcorner,
                   const double yllcorner,
                   const double xurcorner,
                   const double yurcorner,
                   string&& proj4) noexcept
  : proj4_(std::forward<string>(proj4)),
    cell_size_(cell_size),
    xllcorner_(xllcorner),
    yllcorner_(yllcorner),
    xurcorner_(xurcorner),
    yurcorner_(yurcorner),
    nodata_(nodata)
{
  // HACK: don't know if meridian is initialized yet, so repeat calculation
  meridian_ = find_meridian(this->proj4_);
  zone_ = topo::meridian_to_zone(find_meridian(this->proj4_));
}
GridBase::GridBase() noexcept
  : cell_size_(-1),
    xllcorner_(-1),
    yllcorner_(-1),
    xurcorner_(-1),
    yurcorner_(-1),
    meridian_(-1),
    zone_(-1),
    nodata_(-1)
{
}
void GridBase::createPrj(const string& dir, const string& base_name) const
{
  ofstream out;
  out.open(dir + base_name + ".prj");
  logging::extensive(proj4_.c_str());
  // HACK: use what we know is true for the grids that were generated and
  // hope it's correct otherwise
  const auto proj = find_value("+proj=", proj4_);
  if (!proj.empty() && proj != "tmerc" && proj != "utm")
  {
    logging::fatal("Cannot create projection for non-transverse grid");
  }
  out << "Projection    TRANSVERSE\n";
  out << "Datum         AI_CSRS\n";
  out << "Spheroid      GRS80\n";
  out << "Units         METERS\n";
  out << "Zunits        NO\n";
  out << "Xshift        0.0\n";
  out << "Yshift        0.0\n";
  out << "Parameters    \n";
  out << "0.9996 /* scale factor at central meridian\n";
  // HACK: assume we're always on an exact degree
  out << static_cast<int>(meridian_) << "  0  0.0 /* longitude of central meridian\n";
  out << "   0  0  0.0 /* latitude of origin\n";
  out << "500000.0 /* false easting (meters)\n";
  out << "0.0 /* false northing (meters)\n";
  out.close();
}
unique_ptr<Coordinates> GridBase::findCoordinates(const topo::Point& point,
                                                  const bool flipped) const
{
  auto full = findFullCoordinates(point, flipped);
  return make_unique<Coordinates>(static_cast<Idx>(std::get<0>(*full)),
                                  static_cast<Idx>(std::get<1>(*full)),
                                  std::get<2>(*full),
                                  std::get<3>(*full));
}
// Use pair instead of Location, so we can go above max columns & rows
unique_ptr<FullCoordinates> GridBase::findFullCoordinates(const topo::Point& point,
                                                          const bool flipped) const
{
  auto x = 0.0;
  auto y = 0.0;
  lat_lon_to_utm(point, this->zone(), &x, &y);
  logging::note("Coordinates (%f, %f) converted to (%0.1f, %f, %f)",
                point.latitude(),
                point.longitude(),
                this->zone(),
                x,
                y);
  logging::debug("Lower left is (%f, %f)", this->xllcorner_, this->yllcorner_);
  // convert coordinates into cell position
  const auto actual_x = (x - this->xllcorner_) / this->cell_size_;
  // these are already flipped across the y-axis on reading, so it's the same as for x now
  auto actual_y = (!flipped)
                  ? (y - this->yllcorner_) / this->cell_size_
                  : (yurcorner_ - y) / cell_size_;
  const auto column = static_cast<FullIdx>(actual_x);
  const auto row = static_cast<FullIdx>(round(actual_y - 0.5));
  if (0 > column || column >= calculateColumns() || 0 > row || row >= calculateRows())
  {
    logging::debug("Returning nullptr from findFullCoordinates() for (%f, %f) => (%d, %d)",
                   actual_x,
                   actual_y,
                   column,
                   row);
    return nullptr;
  }
  const auto sub_x = static_cast<SubSize>((actual_x - column) * 1000);
  const auto sub_y = static_cast<SubSize>((actual_y - row) * 1000);
  return make_unique<FullCoordinates>(static_cast<FullIdx>(row),
                                      static_cast<FullIdx>(column),
                                      sub_x,
                                      sub_y);
}
void write_ascii_header(ofstream& out,
                        const double num_columns,
                        const double num_rows,
                        const double xll,
                        const double yll,
                        const double cell_size,
                        const double no_data)
{
  out << "ncols         " << num_columns << "\n";
  out << "nrows         " << num_rows << "\n";
  out << "xllcorner     " << fixed << setprecision(6) << xll << "\n";
  out << "yllcorner     " << fixed << setprecision(6) << yll << "\n";
  out << "cellsize      " << cell_size << "\n";
  out << "NODATA_value  " << no_data << "\n";
}
}
