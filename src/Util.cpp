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
#include "Util.h"
#include "Log.h"
#include <regex>
#include <filesystem>
namespace fs = std::filesystem;

TIFF* GeoTiffOpen(const char* const filename, const char* const mode)
{
  TIFF* tif = XTIFFOpen(filename, mode);
  static const TIFFFieldInfo xtiffFieldInfo[] = {
    {TIFFTAG_GDAL_NODATA, -1, -1, TIFF_ASCII, FIELD_CUSTOM, true, false, (char*)"GDALNoDataValue"},
    {TIFFTAG_GEOPIXELSCALE, -1, -1, TIFF_DOUBLE, FIELD_CUSTOM, true, true, (char*)"GeoPixelScale"},
    {TIFFTAG_GEOTRANSMATRIX, -1, -1, TIFF_DOUBLE, FIELD_CUSTOM, true, true, (char*)"GeoTransformationMatrix"},
    {TIFFTAG_GEOTIEPOINTS, -1, -1, TIFF_DOUBLE, FIELD_CUSTOM, true, true, (char*)"GeoTiePoints"},
    {TIFFTAG_GEOKEYDIRECTORY, -1, -1, TIFF_SHORT, FIELD_CUSTOM, true, true, (char*)"GeoKeyDirectory"},
    {TIFFTAG_GEODOUBLEPARAMS, -1, -1, TIFF_DOUBLE, FIELD_CUSTOM, true, true, (char*)"GeoDoubleParams"},
    {TIFFTAG_GEOASCIIPARAMS, -1, -1, TIFF_ASCII, FIELD_CUSTOM, true, true, (char*)"GeoASCIIParams"}};
  TIFFMergeFieldInfo(tif, xtiffFieldInfo, sizeof(xtiffFieldInfo) / sizeof(xtiffFieldInfo[0]));
  return tif;
}

namespace tbd::util
{
void read_directory(const string& name, vector<string>* v, const string& match)
{
  string full_match = ".*/" + match;
  logging::verbose(("Matching '" + full_match + "'").c_str());
  static const std::regex re(full_match, std::regex_constants::icase);
  for (const auto& entry : fs::directory_iterator(name))
  {
    logging::verbose(("Checking if file: " + entry.path().string()).c_str());
    if (fs::is_regular_file(entry))
    {
      logging::extensive(("Checking regex match: " + entry.path().string()).c_str());
      if (std::regex_match(entry.path().string(), re))
      {
        v->push_back(entry.path());
      }
    }
  }
}
void read_directory(const string& name, vector<string>* v)
{
  read_directory(name, v, "/*");
}
vector<string> find_rasters(const string& dir, const int year)
{
  const auto by_year = dir + "/" + to_string(year) + "/";
  const auto raster_root = directory_exists(by_year.c_str())
                           ? by_year
                           : dir + "/default/";
  vector<string> results{};
  try
  {
    read_directory(raster_root, &results, "fuel.*\\.tif");
  }
  catch (const std::exception& e)
  {
    logging::error("Unable to read directory %s", raster_root.c_str());
  }
  return results;
}
bool directory_exists(const char* dir) noexcept
{
  struct stat dir_info
  {
  };
  return stat(dir, &dir_info) == 0 && dir_info.st_mode & S_IFDIR;
}
void make_directory(const char* dir) noexcept
{
  if (-1 == mkdir(dir, 0777) && errno != EEXIST)
  {
    struct stat dir_info
    {
    };
    if (stat(dir, &dir_info) != 0)
    {
      logging::fatal("Cannot create directory %s", dir);
    }
    else if (dir_info.st_mode & S_IFDIR)
    {
      // everything is fine
    }
    else
    {
      logging::fatal("%s is not a directory\n", dir);
    }
  }
}
void make_directory_recursive(const char* dir) noexcept
{
  char tmp[256];
  snprintf(tmp, sizeof tmp, "%s", dir);
  const auto len = strlen(tmp);
  if (tmp[len - 1] == '/')
    tmp[len - 1] = 0;
  for (auto p = tmp + 1; *p; ++p)
    if (*p == '/')
    {
      *p = 0;
      make_directory(tmp);
      *p = '/';
    }
  make_directory(tmp);
}
void read_date(istringstream* iss, string* str, tm* t)
{
  *t = {};
  // Date
  getline(iss, str, ',');
  string ds;
  istringstream dss(*str);
  getline(dss, ds, '-');
  t->tm_year = stoi(ds) - 1900;
  getline(dss, ds, '-');
  t->tm_mon = stoi(ds) - 1;
  getline(dss, ds, ' ');
  t->tm_mday = stoi(ds);
}
UsageCount::~UsageCount()
{
  logging::note("%s called %d times", for_what_.c_str(), count_.load());
}
UsageCount::UsageCount(string for_what) noexcept
  : count_(0), for_what_(std::move(for_what))
{
}
UsageCount& UsageCount::operator++() noexcept
{
  ++count_;
  return *this;
}
}
