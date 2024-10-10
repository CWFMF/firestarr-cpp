/* SPDX-License-Identifier: AGPL-3.0-or-later */
/*! \mainpage FireSTARR Documentation
 *
 * \section intro_sec Introduction
 *
 * FireSTARR is a probabilistic fire growth model.
 */
#include "stdafx.h"
#include "debug_settings.h"
#include "Log.h"
#include "Model.h"
#include "Settings.h"
#include "StartPoint.h"
#include "Test.h"
#include "TimeUtil.h"
#include "Util.h"
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
static const char* BIN_NAME = nullptr;
static map<std::string, std::function<void()>> PARSE_FCT{};
static vector<std::pair<std::string, std::string>> PARSE_HELP{};
static map<std::string, bool> PARSE_REQUIRED{};
static map<std::string, bool> PARSE_HAVE{};
static int ARGC = 0;
static const char* const* ARGV = nullptr;
static int CUR_ARG = 0;
enum MODE
{
  SIMULATION,
  TEST,
  SURFACE
};
string get_args()
{
  std::string args(ARGV[0]);
  for (auto i = 1; i < ARGC; ++i)
  {
    args.append(" ");
    args.append(ARGV[i]);
  }
  return args;
}
void show_args()
{
  auto args = get_args();
  printf("Arguments are:\n%s\n", args.c_str());
}
void log_args()
{
  auto args = get_args();
  fs::logging::note("Arguments are:\n%s\n", args.c_str());
}
void show_usage_and_exit(int exit_code)
{
  printf("Usage: %s <output_dir> <yyyy-mm-dd> <lat> <lon> <HH:MM> [options]\n\n", BIN_NAME);
  printf("Run simulations and save output in the specified directory\n\n\n");
  printf("Usage: %s surface <output_dir> <yyyy-mm-dd> <lat> <lon> <HH:MM> [options]\n\n", BIN_NAME);
  printf("Calculate probability surface and save output in the specified directory\n\n\n");
  printf("Usage: %s test <output_dir> [options]\n\n", BIN_NAME);
  printf(" Run test cases and save output in the specified directory\n\n");
  printf(" Input Options\n");
  // FIX: this should show arguments specific to mode, but it doesn't indicate that on the outputs
  for (auto& kv : PARSE_HELP)
  {
    printf("   %-25s %s\n", kv.first.c_str(), kv.second.c_str());
  }
  exit(exit_code);
}
void show_usage_and_exit()
{
  show_args();
  show_usage_and_exit(-1);
}
void show_help_and_exit()
{
  // showing help isn't an error
  show_usage_and_exit(0);
}
const char* get_arg() noexcept
{
  // check if we don't have any more arguments
  fs::logging::check_fatal(CUR_ARG + 1 >= ARGC, "Missing argument to --%s", ARGV[CUR_ARG]);
  return ARGV[++CUR_ARG];
}
template <class T>
T parse(std::function<T()> fct)
{
  PARSE_HAVE.emplace(ARGV[CUR_ARG], true);
  return fct();
}
template <class T>
T parse_once(std::function<T()> fct)
{
  if (PARSE_HAVE.contains(ARGV[CUR_ARG]))
  {
    printf("\nArgument %s already specified\n\n", ARGV[CUR_ARG]);
    show_usage_and_exit();
  }
  return parse(fct);
}
bool parse_flag(bool not_inverse)
{
  return parse_once<bool>([not_inverse] { return not_inverse; });
}
template <class T>
T parse_value()
{
  return parse_once<T>([] { return stod(get_arg()); });
}
size_t parse_size_t()
{
  return parse_once<size_t>([] { return static_cast<size_t>(stoi(get_arg())); });
}
const char* parse_raw() { return parse_once<const char*>(&get_arg); }
string parse_string() { return string(parse_raw()); }
template <class T>
T parse_index()
{
  return parse_once<T>([] { return T(stod(get_arg())); });
}
void register_argument(string v, string help, bool required, std::function<void()> fct)
{
  PARSE_FCT.emplace(v, fct);
  PARSE_HELP.emplace_back(v, help);
  PARSE_REQUIRED.emplace(v, required);
}
template <class T>
void register_setter(
  std::function<void(T)> fct_set,
  string v,
  string help,
  bool required,
  std::function<T()> fct
)
{
  register_argument(v, help, required, [fct_set, fct] { fct_set(fct()); });
}
template <class T>
void register_setter(T& variable, string v, string help, bool required, std::function<T()> fct)
{
  register_argument(v, help, required, [&variable, fct] { variable = fct(); });
}
void register_flag(std::function<void(bool)> fct, bool not_inverse, string v, string help)
{
  register_argument(v, help, false, [not_inverse, fct] { fct(parse_flag(not_inverse)); });
}
void register_flag(bool& variable, bool not_inverse, string v, string help)
{
  register_argument(v, help, false, [not_inverse, &variable] {
    variable = parse_flag(not_inverse);
  });
}
template <class T>
void register_index(T& index, string v, string help, bool required)
{
  register_argument(v, help, required, [&index] { index = parse_index<T>(); });
}
int main(const int argc, const char* const argv[])
{
#ifdef _WIN32
  printf("FireSTARR windows-testing\n\n");
#else
  printf("FireSTARR %s\n\n", SPECIFIC_REVISION);
#endif
  fs::show_debug_settings();
  ARGC = argc;
  ARGV = argv;
  auto bin = string(ARGV[CUR_ARG++]);
  replace(bin.begin(), bin.end(), '\\', '/');
  const auto end = max(static_cast<size_t>(0), bin.rfind('/') + 1);
  const auto bin_dir = bin.substr(0, end);
  const auto bin_name = bin.substr(end, bin.size() - end);
  BIN_NAME = bin.c_str();
  Settings::setRoot(bin_dir.c_str());
  Log::setLogLevel(fs::logging::LOG_NOTE);
  register_argument("-h", "Show help", false, &show_help_and_exit);
  string wx_file_name;
  string log_file_name = "firestarr.log";
  string fuel_name;
  string perim;
  bool test_all = false;
  MathSize hours = INVALID_TIME;
  size_t size = 0;
  // ffmc, dmc, dc are required for simulation & surface mode, so no indication of it not being
  // provided
  Ffmc ffmc = Ffmc::Invalid;
  Dmc dmc = Dmc::Invalid;
  Dc dc = Dc::Invalid;
  auto wind_direction = Direction::Invalid.asValue();
  auto wind_speed = Speed::Invalid.asValue();
  auto slope = static_cast<SlopeSize>(INVALID_SLOPE);
  auto aspect = static_cast<AspectSize>(INVALID_ASPECT);
  size_t SKIPPED_ARGS = 0;
  // FIX: need to get rain since noon yesterday to start of this hourly weather
  Precipitation apcp_prev;
  // can be used multiple times
  register_argument("-v", "Increase output level", false, &Log::increaseLogLevel);
  // if they want to specify -v and -q then that's fine
  register_argument("-q", "Decrease output level", false, &Log::decreaseLogLevel);
  register_flag(&Settings::setSaveAsAscii, true, "--ascii", "Save grids as .asc");
  register_flag(&Settings::setSaveAsTiff, false, "--no-tiff", "Do not save grids as .tif");
  auto result = -1;
  MODE mode = SIMULATION;
  if (ARGC > 1 && 0 == strcmp(ARGV[1], "test"))
  {
    fs::logging::note("Running in test mode");
    mode = TEST;
    CUR_ARG += 1;
    SKIPPED_ARGS = 1;
    // if we have a directory and nothing else then use defaults for single run
    // if we have 'all' then overrride specified indices, but then filter down to the subset that
    // matches what was specified
    register_setter<MathSize>(hours, "--hours", "Duration in hours", false, &parse_value<MathSize>);
    register_setter<string>(fuel_name, "--fuel", "FBP fuel type", false, &parse_string);
    register_index<Ffmc>(ffmc, "--ffmc", "Constant Fine Fuel Moisture Code", false);
    register_index<Dmc>(dmc, "--dmc", "Constant Duff Moisture Code", false);
    register_index<Dc>(dc, "--dc", "Constant Drought Code", false);
    register_setter<
      MathSize>(wind_direction, "--wd", "Constant wind direction", false, &parse_value<MathSize>);
    register_setter<
      MathSize>(wind_speed, "--ws", "Constant wind speed", false, &parse_value<MathSize>);
    register_setter<SlopeSize>(slope, "--slope", "Constant slope", false, &parse_value<SlopeSize>);
    register_setter<
      AspectSize>(aspect, "--aspect", "Constant slope aspect/azimuth", false, &parse_value<AspectSize>);
    register_setter<size_t>(
      &Settings::setStaticCuring, "--curing", "Specify static grass curing", false, &parse_size_t
    );
    register_flag(
      &Settings::setForceGreenup, true, "--force-greenup", "Force green up for all fires"
    );
    register_flag(
      &Settings::setForceNoGreenup, true, "--force-no-greenup", "Force no green up for all fires"
    );
  }
  else
  {
    register_flag(&Settings::setSaveIndividual, true, "-i", "Save individual maps for simulations");
    register_flag(&Settings::setRunAsync, false, "-s", "Run in synchronous mode");
    register_flag(&Settings::setSavePoints, true, "--points", "Save simulation points to file");
    register_flag(
      &Settings::setSaveIntensity, false, "--no-intensity", "Do not output intensity grids"
    );
    register_flag(
      &Settings::setSaveProbability, false, "--no-probability", "Do not output probability grids"
    );
    register_flag(&Settings::setSaveOccurrence, true, "--occurrence", "Output occurrence grids");
    register_flag(
      &Settings::setSaveSimulationArea, true, "--sim-area", "Output simulation area grids"
    );
    register_setter<const char*>(
      &Settings::setRasterRoot,
      "--raster-root",
      "Use specified directory as raster root",
      false,
      &parse_raw
    );
    register_setter<const char*>(
      &Settings::setFuelLookupTable,
      "--fuel-lut",
      "Use specified fuel lookup table",
      false,
      &parse_raw
    );
    register_setter<
      DurationSize>(&Settings::setUtcOffset, "--tz", "UTC offset (hours)", true, &parse_value<DurationSize>);
    register_setter<size_t>(
      &Settings::setStaticCuring, "--curing", "Specify static grass curing", false, &parse_size_t
    );
    register_flag(
      &Settings::setForceGreenup, true, "--force-greenup", "Force green up for all fires"
    );
    register_flag(
      &Settings::setForceNoGreenup, true, "--force-no-greenup", "Force no green up for all fires"
    );
    register_setter<string>(log_file_name, "--log", "Output log file", false, &parse_string);
    if (ARGC > 1 && 0 == strcmp(ARGV[1], "surface"))
    {
      fs::logging::note("Running in probability surface mode");
      mode = SURFACE;
      // skip 'surface' argument if present
      CUR_ARG += 1;
      SKIPPED_ARGS = 1;
      // probabalistic surface is computationally impossible at this point
      Settings::setDeterministic(true);
      Settings::setSurface(true);
      register_index<Ffmc>(ffmc, "--ffmc", "Constant Fine Fuel Moisture Code", true);
      register_index<Dmc>(dmc, "--dmc", "Constant Duff Moisture Code", true);
      register_index<Dc>(dc, "--dc", "Constant Drought Code", true);
      register_setter<
        MathSize>(wind_direction, "--wd", "Constant wind direction", true, &parse_value<MathSize>);
      register_setter<
        MathSize>(wind_speed, "--ws", "Constant wind speed", true, &parse_value<MathSize>);
    }
    else
    {
      register_setter<string>(wx_file_name, "--wx", "Input weather file", true, &parse_string);
      register_flag(
        &Settings::setDeterministic,
        true,
        "--deterministic",
        "Run deterministically (100% chance of spread & survival)"
      );
      register_setter<
        ThresholdSize>(&Settings::setConfidenceLevel, "--confidence", "Use specified confidence level", false, &parse_value<ThresholdSize>);
      register_setter<string>(perim, "--perim", "Start from perimeter", false, &parse_string);
      register_setter<size_t>(size, "--size", "Start from size", false, &parse_size_t);
      // HACK: want different text for same flag so define here too
      register_index<Ffmc>(ffmc, "--ffmc", "Startup Fine Fuel Moisture Code", true);
      register_index<Dmc>(dmc, "--dmc", "Startup Duff Moisture Code", true);
      register_index<Dc>(dc, "--dc", "Startup Drought Code", true);
      register_index<Precipitation>(
        apcp_prev,
        "--apcp_prev",
        "Startup precipitation between 1200 yesterday and start of hourly weather",
        false
      );
    }
    register_setter<const char*>(
      &Settings::setOutputDateOffsets,
      "--output_date_offsets",
      "Override output date offsets",
      false,
      &parse_raw
    );
    if (2 == ARGC && 0 == strcmp(ARGV[CUR_ARG], "-h"))
    {
      // HACK: just do this for now
      show_help_and_exit();
    }
    else if (3 > (ARGC - SKIPPED_ARGS))
    {
      show_usage_and_exit();
    }
  }
#ifdef NDEBUG
  try
  {
#endif
    vector<string> positional_args{};
    while (CUR_ARG < ARGC)
    {
      const string arg = ARGV[CUR_ARG];
      bool is_positional = !arg.starts_with("-");
      if (!is_positional)
      {
        // check for single letter flags or '--'
        if (PARSE_FCT.find(arg) != PARSE_FCT.end())
        {
          fs::logging::debug("Found option for argument '%s'", arg.c_str());
          try
          {
            PARSE_FCT[arg]();
          }
          catch (std::exception&)
          {
            // CUR_ARG would be incremented while trying to parse at this point, so -1 is 'arg'
            printf("\n'%s' is not a valid value for argument %s\n\n", ARGV[CUR_ARG], arg.c_str());
            show_usage_and_exit();
          }
        }
        else
        {
          if (arg.starts_with("--"))
          {
            // anything starting with '--' should be a flag, but it's not a valid one so complain
            printf("\n'%s' is not a valid option\n\n", arg.c_str());
            show_usage_and_exit();
          }
          is_positional = true;
        }
      }
      if (is_positional)
      {
        // this is a positional argument so add to that list
        positional_args.emplace_back(arg);
        fs::logging::debug("Found positional argument '%s'", arg.c_str());
      }
      ++CUR_ARG;
    }
    for (auto& kv : PARSE_REQUIRED)
    {
      if (kv.second && PARSE_HAVE.end() == PARSE_HAVE.find(kv.first))
      {
        fs::logging::fatal("%s must be specified", kv.first.c_str());
      }
    }
    if (0 == positional_args.size())
    {
      // always require at least some positional argument
      show_usage_and_exit();
    }
    // parse positional arguments
    // output directory is always the first thing
    size_t cur_arg = 0;
    auto has_positional = [&cur_arg, &positional_args]() {
      return (cur_arg < positional_args.size());
    };
    auto get_positional = [&cur_arg, &positional_args, &has_positional]() {
      if (!has_positional())
      {
        fs::logging::error("Not enough positional arguments");
        show_usage_and_exit();
      }
      // return from front and advance to next
      return positional_args[cur_arg++];
    };
    auto done_positional = [&cur_arg, &positional_args]() {
      // should be exactly at size since increments after getting argument
      if (positional_args.size() != cur_arg)
      {
        fs::logging::error("Too many positional arguments");
        show_usage_and_exit();
      }
    };
    // positional arguments all start with <output_dir> after mode (if applicable)
    // "./firestarr [surface] <output_dir> <yyyy-mm-dd> <lat> <lon> <HH:MM> [options] [-v | -q]"
    string output_directory(get_positional());
    replace(output_directory.begin(), output_directory.end(), '\\', '/');
    if ('/' != output_directory[output_directory.length() - 1])
    {
      output_directory += '/';
    }
    struct stat info{};
    if (stat(output_directory.c_str(), &info) != 0 || !(info.st_mode & S_IFDIR))
    {
      fs::make_directory_recursive(output_directory.c_str());
    }
    // if name starts with "/" then it's an absolute path, otherwise append to working directory
    const string log_file =
      log_file_name.starts_with("/") ? log_file_name : (output_directory + log_file_name);
    fs::logging::check_fatal(
      !Log::openLogFile(log_file.c_str()), "Can't open log file %s", log_file.c_str()
    );
    fs::logging::note("Output directory is %s", output_directory.c_str());
    fs::logging::note("Output log is %s", log_file.c_str());
    if (mode != TEST)
    {
      // handle surface/simulation positional arguments
      // positional arguments should be:
      // "./firestarr [surface] <output_dir> <yyyy-mm-dd> <lat> <lon> <HH:MM> [options] [-v | -q]"
      string date(get_positional());
      tm start_date{};
      start_date.tm_year = stoi(date.substr(0, 4)) - TM_YEAR_OFFSET;
      start_date.tm_mon = stoi(date.substr(5, 2)) - TM_MONTH_OFFSET;
      start_date.tm_mday = stoi(date.substr(8, 2));
      const auto latitude = stod(get_positional());
      const auto longitude = stod(get_positional());
      const StartPoint start_point(latitude, longitude);
      size_t num_days = 0;
      string arg(get_positional());
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
      done_positional();
      // at this point we've parsed positional args and know we're not in test mode
      if (!PARSE_HAVE.contains("--apcp_prev"))
      {
        fs::logging::warning(
          "Assuming 0 precipitation between noon yesterday and weather start for startup indices"
        );
        apcp_prev = Precipitation::Zero;
      }
      // HACK: ISI for yesterday really doesn't matter so just use any wind
      // HACK: it's basically wrong to assign this precip to yesterday's object,
      // but don't want to add another argument right now
      const auto yesterday = FwiWeather(
        Temperature::Zero,
        RelativeHumidity::Zero,
        Wind(Direction(wind_direction, false), Speed(wind_speed)),
        Precipitation(apcp_prev),
        ffmc,
        dmc,
        dc
      );
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
        output_directory,
        wx_file_name.c_str(),
        yesterday,
        Settings::rasterRoot(),
        start_point,
        start,
        perim,
        size
      );
      Log::closeLogFile();
    }
    else
    {
      // test mode
      if (has_positional())
      {
        const auto arg = get_positional();
        if (0 != strcmp(arg.c_str(), "all"))
        {
          fs::logging::error(
            "Only positional argument allowed for test mode aside from output directory is 'all' but got '%s'",
            arg.c_str()
          );
          show_usage_and_exit();
        }
        test_all = true;
      }
      done_positional();
      const auto wx = FwiWeather(
        Temperature::Zero,
        RelativeHumidity::Zero,
        Wind(Direction(wind_direction, false), Speed(wind_speed)),
        Precipitation::Zero,
        ffmc,
        dmc,
        dc
      );
      show_args();
      result = fs::test(output_directory, hours, &wx, fuel_name, slope, aspect, test_all);
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
int main(const int argc, const char* const argv[]) { exit(fs::main(argc, argv)); }
