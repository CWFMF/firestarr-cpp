/* SPDX-License-Identifier: AGPL-3.0-or-later */
/*! \mainpage FireSTARR Documentation
 *
 * \section intro_sec Introduction
 *
 * FireSTARR is a probabilistic fire growth model.
 */
#include "stdafx.h"
#include "ArgumentParser.h"
#include "FWI.h"
#include "Log.h"
#include "Model.h"
#include "Settings.h"
#include "StartPoint.h"
#include "Test.h"
#include "TestFwi.h"
#include "TimeUtil.h"
#include "Util.h"
#include "version.h"
#include "Weather.h"
namespace fs
{
using fs::AspectSize;
using fs::Dc;
using fs::Direction;
using fs::Dmc;
using fs::Ffmc;
using fs::FwiWeather;
using fs::INVALID_ASPECT;
using fs::INVALID_SLOPE;
using fs::INVALID_TIME;
using fs::MathSize;
using fs::Model;
using fs::Precipitation;
using fs::RelativeHumidity;
using fs::SlopeSize;
using fs::Speed;
using fs::StartPoint;
using fs::Temperature;
using fs::ThresholdSize;
using fs::Wind;
using fs::logging::Log;
int main(const int argc, const char* const argv[])
{
#ifdef _WIN32
  printf("FireSTARR windows-testing\n\n");
#else
  printf("FireSTARR %s\n\n", SPECIFIC_REVISION);
#endif
  MainArgumentParser parser{argc, argv};
  parser.parse_args();
  auto result = -1;
#ifdef NDEBUG
  try
  {
#endif
    fs::logging::check_fatal(
      !Log::openLogFile(parser.log_file.c_str()), "Can't open log file %s", parser.log_file.c_str()
    );
    fs::logging::note("Output directory is %s", parser.output_directory.c_str());
    fs::logging::note("Output log is %s", parser.log_file.c_str());
    // at this point we've parsed positional args and know we're not in test mode
    if (!was_parsed("--apcp_prev"))
    {
      fs::logging::warning(
        "Assuming 0 precipitation between noon yesterday and weather start for startup indices"
      );
    }
    if (parser.mode != TEST)
    {
      // handle surface/simulation positional arguments
      // positional arguments should be:
      // "./firestarr [surface] <output_dir> <yyyy-mm-dd> <lat> <lon> <HH:MM> [options] [-v | -q]"
      string date(parser.get_positional());
      tm start_date{};
      start_date.tm_year = stoi(date.substr(0, 4)) - TM_YEAR_OFFSET;
      start_date.tm_mon = stoi(date.substr(5, 2)) - TM_MONTH_OFFSET;
      start_date.tm_mday = stoi(date.substr(8, 2));
      const auto latitude = stod(parser.get_positional());
      const auto longitude = stod(parser.get_positional());
      const StartPoint start_point(latitude, longitude);
      size_t num_days = 0;
      string arg(parser.get_positional());
      tm start{};
      if (5 == arg.size() && ':' == arg[2])
      {
        try
        {
          // if this is a time then we aren't just running the weather
          start_date.tm_hour = stoi(arg.substr(0, 2));
          fs::logging::check_fatal(
            start_date.tm_hour < 0 || start_date.tm_hour > 23,
            "Simulation start time has an invalid hour (%d)",
            start_date.tm_hour
          );
          start_date.tm_min = stoi(arg.substr(3, 2));
          fs::logging::check_fatal(
            start_date.tm_min < 0 || start_date.tm_min > 59,
            "Simulation start time has an invalid minute (%d)",
            start_date.tm_min
          );
          fs::logging::note(
            "Simulation start time before fix_tm() is %d-%02d-%02d %02d:%02d",
            start_date.tm_year + TM_YEAR_OFFSET,
            start_date.tm_mon + TM_MONTH_OFFSET,
            start_date.tm_mday,
            start_date.tm_hour,
            start_date.tm_min
          );
          fs::fix_tm(&start_date);
          fs::logging::note(
            "Simulation start time after fix_tm() is %d-%02d-%02d %02d:%02d",
            start_date.tm_year + TM_YEAR_OFFSET,
            start_date.tm_mon + TM_MONTH_OFFSET,
            start_date.tm_mday,
            start_date.tm_hour,
            start_date.tm_min
          );
          // we were given a time, so number of days is until end of year
          start = start_date;
          const auto start_t = mktime(&start);
          auto year_end = start;
          year_end.tm_mon = 12 - TM_MONTH_OFFSET;
          year_end.tm_mday = 31;
          const auto seconds = difftime(mktime(&year_end), start_t);
          // start day counts too, so +1
          // HACK: but we don't want to go to Jan 1 so don't add 1
          num_days = static_cast<size_t>(seconds / fs::DAY_SECONDS);
          fs::logging::debug("Calculated number of days until end of year: %d", num_days);
          // +1 because day 1 counts too
          // +2 so that results don't change when we change number of days
          num_days = min(num_days, static_cast<size_t>(Settings::maxDateOffset()) + 2);
        }
        catch (std::exception&)
        {
          show_usage_and_exit();
        }
      }
      parser.done_positional();
      // HACK: ISI for yesterday really doesn't matter so just use any wind
      // HACK: it's basically wrong to assign this precip to yesterday's object,
      // but don't want to add another argument right now
      const FwiWeather yesterday{parser.get_yesterday_weather()};
      fs::fix_tm(&start_date);
      fs::logging::note(
        "Simulation start time after fix_tm() again is %d-%02d-%02d %02d:%02d",
        start_date.tm_year + TM_YEAR_OFFSET,
        start_date.tm_mon + TM_MONTH_OFFSET,
        start_date.tm_mday,
        start_date.tm_hour,
        start_date.tm_min
      );
      start = start_date;
      log_args();
      result = Model::runScenarios(
        parser.output_directory,
        parser.wx_file_name.c_str(),
        yesterday,
        Settings::rasterRoot(),
        start_point,
        start,
        parser.perim,
        parser.size
      );
      Log::closeLogFile();
    }
    else
    {
      // test mode
      if (parser.has_positional())
      {
        const auto arg = parser.get_positional();
        if (0 != strcmp(arg.c_str(), "all"))
        {
          fs::logging::error(
            "Only positional argument allowed for test mode aside from output directory is 'all' but got '%s'",
            arg.c_str()
          );
          show_usage_and_exit();
        }
        parser.test_all = true;
      }
      parser.done_positional();
      const FwiWeather wx{parser.get_test_weather()};
      show_args();
      result = fs::test(
        parser.output_directory,
        parser.hours,
        &wx,
        parser.fuel_name,
        parser.slope,
        parser.aspect,
        parser.test_all
      );
    }
#ifdef NDEBUG
  }
  catch (const std::exception& ex)
  {
    fs::logging::fatal(ex);
    std::terminate();
  }
#endif
  return result;
}
}
// HACK: keep everything else out of std namepace
int main(const int argc, const char* const argv[])
{
#ifdef TEST_DUFF
  // FIX: this was used to compare to the old template version, but doesn't work now
  //      left for reference for now so idea could be used for more tests
  constexpr auto fct_main = fs::duff::test_duff;
#elif TEST_FWI
  constexpr auto fct_main = fs::testing::test_fwi;
#else
  constexpr auto fct_main = fs::main;
#endif
  exit(fct_main(argc, argv));
}
