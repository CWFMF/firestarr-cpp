/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include "Util.h"
#include <regex>
#include "Log.h"
#include "TimeUtil.h"
namespace fs
{
FileList read_directory(const string_view name, const string_view match, const bool for_files)
{
  FileList files{};
  string full_match = ".*/" + string(match);
  logging::verbose("Matching '{:s}'", full_match);
  const std::regex re(full_match, std::regex_constants::icase);
  for (const auto& entry : std::filesystem::directory_iterator(name))
  {
    logging::verbose("Checking if file: {:s}", entry.path().string());
    if ((for_files && std::filesystem::is_regular_file(entry))
        || (!for_files && std::filesystem::is_directory(entry)))
    {
      logging::extensive("Checking regex match: {:s}", entry.path().string());
      if (std::regex_match(entry.path().string(), re))
      {
        files.emplace_back(entry
                             .path()
#ifdef _WIN32
                             .generic_string()
#endif
        );
      }
    }
  }
  return files;
}
FileList find_rasters(const string_view dir, const YearSize year)
{
  // HACK: read_directory() doesn't work if path doesn't end in '/'
  const string path = string(dir) + (dir.ends_with("/") ? "" : "/");
  const string for_year = path + to_string(year) + "/";
  const string for_default = path + "default/";
  // use first existing folder of dir/year, dir/default, or dir in that order
  const string raster_root = directory_exists(for_year.c_str())
                             ? for_year
                             : (directory_exists(for_default.c_str()) ? for_default : path);
  FileList files{};
  logging::info("Raster root is {:s}", raster_root);
  try
  {
    files.append_range(read_directory(raster_root, "fuel.*\\.tif"));
  }
  catch (const std::exception& ex)
  {
    logging::error("Unable to read directory {:s}", raster_root);
    logging::error("{:s}", ex.what());
  }
  return files;
}
bool directory_exists(const char* dir) noexcept
{
  struct stat dir_info{};
  return stat(dir, &dir_info) == 0 && dir_info.st_mode & S_IFDIR;
}
bool file_exists(const char* path) noexcept
{
  struct stat path_info{};
  // FIX: check that this works on symlinks
  return stat(path, &path_info) == 0 && path_info.st_mode & S_IFREG;
}
void make_directory(const char* dir) noexcept
{
  std::error_code ec{};
  if (std::filesystem::create_directory(dir, ec))
  {
    if (ec)
    {
      exit(logging::fatal("Error {:d} creating directory {:s}", ec.value(), dir));
    }
    struct stat dir_info{};
    if (stat(dir, &dir_info) != 0)
    {
      exit(logging::fatal("Cannot create directory {:s}", dir));
    }
    else if (dir_info.st_mode & S_IFDIR)
    {
      // everything is fine
    }
    else
    {
      exit(logging::fatal("{:s} is not a directory\n", dir));
    }
  }
}
void make_directory_recursive(string dir) noexcept
{
  const auto len = dir.length();
  if (dir[len - 1] == '/')
    dir[len - 1] = 0;
  for (auto p = &dir[0] + 1; *p; ++p)
    if (*p == '/')
    {
      *p = 0;
      make_directory(dir.c_str());
      *p = '/';
    }
  make_directory(dir.c_str());
}
tm to_tm(const YearSize year, const int month, const int day, const int hour, const int minute)
{
  // do this to calculate yday
  tm t{};
  t.tm_year = year - TM_YEAR_OFFSET;
  t.tm_mon = month - TM_MONTH_OFFSET;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = minute;
  mktime(&t);
  return t;
}
string format_date(const tm& date)
{
  return std::format(
    "{:4d}-{:02d}-{:02d}",
    date.tm_year + TM_YEAR_OFFSET,
    date.tm_mon + TM_MONTH_OFFSET,
    date.tm_mday
  );
}
string format_time(const tm& date)
{
  return std::format("{:02d}:{:02d}", date.tm_hour, date.tm_min);
}
string format_datetime(const tm& date)
{
  // make sure parts match whole
  return std::format("{} {}", format_date(date), format_time(date));
}
tm parse_date(const string value)
{
  tm date{};
  date.tm_year = stoi(value.substr(0, 4)) - TM_YEAR_OFFSET;
  date.tm_mon = stoi(value.substr(5, 2)) - TM_MONTH_OFFSET;
  date.tm_mday = stoi(value.substr(8, 2));
  return date;
}
void add_time(tm& date, const string value)
{
  // if this is a time then we aren't just running the weather
  date.tm_hour = stoi(value.substr(0, 2));
  fs::logging::check_fatal(
    date.tm_hour < 0 || date.tm_hour > 23,
    "Simulation start time has an invalid hour ({:d})",
    date.tm_hour
  );
  date.tm_min = stoi(value.substr(3, 2));
  fs::logging::check_fatal(
    date.tm_min < 0 || date.tm_min > 59,
    "Simulation start time has an invalid minute ({:d})",
    date.tm_min
  );
  fs::logging::verbose("Simulation start time before fix_tm() is {:s}", format_datetime(date));
  fs::fix_tm(&date);
  fs::logging::verbose("Simulation start time after fix_tm() is {:s}", format_datetime(date));
}
DurationSize to_time(const tm& t)
{
  return t.tm_yday
       + ((t.tm_hour + (static_cast<DurationSize>(t.tm_min) / HOUR_MINUTES)) / DAY_HOURS);
}
DurationSize to_time(
  const YearSize year,
  const int month,
  const int day,
  const int hour,
  const int minute
)
{
  return to_time(to_tm(year, month, day, hour, minute));
}
void read_date(istringstream* iss, string* str, tm* t)
{
  *t = {};
  // Date
  getline(iss, str, ',');
  string ds;
  istringstream dss(*str);
  getline(dss, ds, '-');
  t->tm_year = stoi(ds) - TM_YEAR_OFFSET;
  getline(dss, ds, '-');
  t->tm_mon = stoi(ds) - TM_MONTH_OFFSET;
  getline(dss, ds, ' ');
  t->tm_mday = stoi(ds);
  getline(dss, ds, ':');
  t->tm_hour = stoi(ds);
  logging::verbose("Date is {:s}", format_datetime(*t));
}
UsageCount::~UsageCount() { logging::note("{:s} called {:d} times", for_what_, count_.load()); }
UsageCount::UsageCount(string for_what) noexcept : count_(0), for_what_(std::move(for_what)) { }
UsageCount& UsageCount::operator++() noexcept
{
  ++count_;
  return *this;
}
constexpr int DAYS_IN_MONTH[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
constexpr int DAYS_IN_MONTH_LEAP[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
void month_and_day(
  const YearSize year,
  const size_t day_of_year,
  size_t* month,
  size_t* day_of_month
)
{
  auto days = (is_leap_year(year) ? DAYS_IN_MONTH_LEAP : DAYS_IN_MONTH);
  *month = 1;
  int days_left = day_of_year;
  while (days[*month - 1] <= days_left)
  {
    days_left -= days[*month - 1];
    ++(*month);
  }
  *day_of_month = days_left + 1;
}
bool is_leap_year(const YearSize year)
{
  if (year % 400 == 0)
  {
    return true;
  }
  if (year % 100 == 0)
  {
    return false;
  }
  return (year % 4 == 0);
}
string make_timestamp(const YearSize year, const DurationSize time)
{
  size_t day = floor(time);
  size_t hour = (time - day) * static_cast<DurationSize>(DAY_HOURS);
  size_t minute =
    round(((time - day) * static_cast<DurationSize>(DAY_HOURS) - hour) * HOUR_MINUTES);
  if (60 == minute)
  {
    minute = 0;
    hour += 1;
  }
  if (24 == hour)
  {
    day += 1;
    hour = 0;
  }
  size_t month;
  size_t day_of_month;
  month_and_day(year, day, &month, &day_of_month);
  return std::format("{:4d}-{:02d}-{:02d} {:02d}:{:02d}", year, month, day_of_month, hour, minute);
}
string get_canonical_path(const char* const dir_root, string path)
{
#ifdef _WIN32
  std::replace(path.begin(), path.end(), '/', '\\');
#else
  std::replace(path.begin(), path.end(), '\\', '/');
#endif
#ifdef _WIN32
  if (':' == path.at(1))
  {
    logging::note("Absolute path use on windows: {:s}", path.c_str());
  }
  else
#endif
    if (!path.starts_with("/"))
  {
    try
    {
      // not an absolute path so should be relative to root
      const pushd dir{dir_root};
      logging::note("dir_root = {:s}", dir_root);
      auto p_rel = std::filesystem::relative(path);
      logging::verbose("p_rel = {:s}", p_rel.generic_string());
      auto p_abs =
        p_rel.empty() ? dir.current_directory : std::filesystem::absolute(p_rel).generic_string();
      logging::verbose("p_abs = {:s}", p_abs);
      // auto p_can = std::filesystem::canonical(p_rel);
      // logging::note("p_can = {:s}", p_can.c_str());
      logging::debug(
        "Relative to {:s} path {:s} becomes {:s}", dir_root, p_rel.generic_string(), p_abs
        // ,
        // p_can.c_str()
      );
      path = p_abs;
      logging::info("Converted relative path to absolute path {:s}", path);
    }
    catch (std::exception&)
    {
      exit(logging::fatal("Unable to convert relative path to canonical for {:s}", path));
      path = "";
    }
  }
  return path;
}
string tolower(string value)
{
  std::transform(value.begin(), value.end(), value.begin(), [](const auto c) {
    return std::tolower(c);
  });
  return value;
}
string toupper(string value)
{
  std::transform(value.begin(), value.end(), value.begin(), [](const auto c) {
    return std::toupper(c);
  });
  return value;
}
}
