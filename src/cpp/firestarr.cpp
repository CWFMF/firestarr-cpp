/* SPDX-License-Identifier: AGPL-3.0-or-later */
/*! \mainpage FireSTARR Documentation
 *
 * \section intro_sec Introduction
 *
 * FireSTARR is a probabilistic fire growth model.
 */
#include "fs/ArgumentParser.h"
#include "fs/FWI.h"
#include "fs/Log.h"
#include "fs/Model.h"
#include "fs/Settings.h"
#include "fs/StartPoint.h"
#include "fs/stdafx.h"
#include "fs/Test.h"
#include "fs/TimeUtil.h"
#include "fs/Util.h"
#include "fs/Weather.h"
#include "version.h"
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
  using namespace fs::settings;
  cout << std::format("FireSTARR {:s}\n\n", SPECIFIC_REVISION);
  MainArgumentParser parser{argc, argv};
  // HACK: resolve once and fail if not set already
  static auto& settings = fs::settings::instance();
  parser.parse_args();
  auto result = -1;
#ifdef NDEBUG
  try
  {
#endif
    // HACK: check here so verbosity can affect showing compile info
    if (parser.help_requested())
    {
      fs::logging::note("Specific revision is %s", SPECIFIC_REVISION);
      fs::logging::debug("Full hash is: %s", FULL_HASH);
      fs::logging::debug("Compiled on: %s", COMPILED_ON);
      parser.show_help_and_exit();
    }
    // HACK: know saving settings made output_directory already
    static const auto dir_out = settings.output_directory;
    static const auto dir_log = settings.log_directory();
    if (dir_log != dir_out)
    {
      make_directory_recursive(dir_log.c_str());
      logging::warning(
        "Log file (%s) is being written outside the output directory (%s)",
        settings.log_file.c_str(),
        dir_out.c_str()
      );
    }
    const auto opened_log = Log::openLogFile(settings.log_file.c_str());
    if (!opened_log)
    {
      logging::fatal("Can't open log file %s", settings.log_file.c_str());
    }
    fs::logging::note("Specific revision is %s", SPECIFIC_REVISION);
    fs::logging::debug("Full hash is: %s", FULL_HASH);
    fs::logging::debug("Compiled on: %s", COMPILED_ON);
    fs::logging::note("Output directory is %s", settings.output_directory.c_str());
    fs::logging::note("Output log is %s", settings.log_file.c_str());
    // at this point we've parsed positional args and know we're not in test mode
    if (!parser.was_parsed("--apcp_prev"))
    {
      fs::logging::warning(
        "Assuming 0 precipitation between noon yesterday and weather start for startup indices"
      );
    }
    if (!settings.is_test())
    {
      const StartPoint start_point(settings.latitude.value(), settings.longitude.value());
      // HACK: ISI for yesterday really doesn't matter so just use any wind
      // HACK: it's basically wrong to assign this precip to yesterday's object,
      // but don't want to add another argument right now
      const FwiWeather yesterday{settings.get_weather()};
      auto& start_date = settings.start_date.value();
      fs::fix_tm(&start_date);
      fs::logging::debug(
        "Simulation start time after fix_tm() again is %s", format_datetime(start_date).c_str()
      );
      // we were given a time, so number of days is until end of year
      tm start = start_date;
      const auto start_t = mktime(&start);
      auto year_end = start;
      year_end.tm_mon = 12 - TM_MONTH_OFFSET;
      year_end.tm_mday = 31;
      const auto seconds = difftime(mktime(&year_end), start_t);
      // start day counts too, so +1
      // HACK: but we don't want to go to Jan 1 so don't add 1
      size_t num_days = static_cast<size_t>(seconds / fs::DAY_SECONDS);
      fs::logging::debug("Calculated number of days until end of year: %d", num_days);
      // +1 because day 1 counts too
      // +2 so that results don't change when we change number of days
      num_days = min(num_days, static_cast<size_t>(settings.output_date_offsets.max()) + 2);
      parser.log_args();
      result = Model::runScenarios(
        settings.output_directory,
        settings.wx_file_name,
        yesterday,
        settings.raster_root.canonical(),
        start_point,
        start_date,
        settings.perimeter,
        settings.initial_size.value_or(0)
      );
      Log::closeLogFile();
    }
    else
    {
      // test mode
      parser.show_args();
      result = fs::test(settings);
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
  // HACK: want to be able to do tests without changing source files but can't find a nice way
  constexpr auto fct_main = fs::main;
  exit(fct_main(argc, argv));
}
