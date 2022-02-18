/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2021-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "Util.h"
#include "Log.h"
#include <regex>
#include <filesystem>
namespace fs::util
{
void
read_directory(
  const string& name,
  vector<string>* v,
  const string& match
)
{
  string full_match = ".*/" + match;
  logging::verbose(("Matching '" + full_match + "'").c_str());
  static const std::regex re(full_match, std::regex_constants::icase);
  for (const auto& entry : std::filesystem::directory_iterator(name))
  {
    logging::verbose(("Checking if file: " + entry.path().string()).c_str());
    if (std::filesystem::is_regular_file(entry))
    {
      logging::extensive(("Checking regex match: " + entry.path().string()).c_str());
      if (std::regex_match(entry.path().string(), re))
      {
        v->push_back(entry.path());
      }
    }
  }
}
void
read_directory(
  const string& name,
  vector<string>* v
)
{
  read_directory(name, v, "/*");
}
vector<string>
find_rasters(
  const string& dir,
  const int year
)
{
  const auto by_year = dir + "/" + to_string(year) + "/";
  const auto raster_root = directory_exists(by_year.c_str()) ? by_year : dir + "/default/";
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
bool
directory_exists(
  const char* dir
) noexcept
{
  struct stat dir_info{};
  return stat(dir, &dir_info) == 0 && dir_info.st_mode & S_IFDIR;
}
void
make_directory(
  const char* dir
) noexcept
{
  if (-1 == mkdir(dir, 0777) && errno != EEXIST)
  {
    struct stat dir_info{};
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
void
make_directory_recursive(
  const char* dir
) noexcept
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
void
read_date(
  istringstream* iss,
  string* str,
  tm* t
)
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
UsageCount::UsageCount(
  string for_what
) noexcept
  : count_(0),
    for_what_(std::move(for_what))
{
}
UsageCount&
UsageCount::operator++() noexcept
{
  ++count_;
  return *this;
}
}
