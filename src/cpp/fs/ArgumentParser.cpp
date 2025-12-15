/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "ArgumentParser.h"
#include "Settings.h"
namespace fs
{
static const char* BIN_NAME = nullptr;
static int ARGC = 0;
static size_t SKIPPED_ARGS = 0;
static const char* const* ARGV = nullptr;
static int CUR_ARG = 0;
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
size_t parse_size_t()
{
  return parse_once<size_t>([] { return static_cast<size_t>(stoi(get_arg())); });
}
const char* parse_raw() { return parse_once<const char*>(&get_arg); }
string parse_string() { return string(parse_raw()); }
void register_argument(string v, string help, bool required, std::function<void()> fct)
{
  PARSE_FCT.emplace(v, fct);
  PARSE_HELP.emplace_back(v, help);
  PARSE_REQUIRED.emplace(v, required);
}
void register_flag(std::function<void(bool)> fct, bool not_inverse, string v, string help)
{
  register_argument(v, help, false, [=] { fct(parse_flag(not_inverse)); });
}
void register_flag(bool& variable, bool not_inverse, string v, string help)
{
  register_argument(v, help, false, [=, &variable] { variable = parse_flag(not_inverse); });
}
ArgumentParser::ArgumentParser(const int argc, const char* const argv[])
{
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
  logging::Log::setLogLevel(fs::logging::LOG_NOTE);
  register_argument("-h", "Show help", false, &show_help_and_exit);
  // can be used multiple times
  register_argument("-v", "Increase output level", false, &Log::increaseLogLevel);
  // if they want to specify -v and -q then that's fine
  register_argument("-q", "Decrease output level", false, &Log::decreaseLogLevel);
}
void ArgumentParser::parse_args()
{
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
}
bool ArgumentParser::has_positional() const { return (cur_arg < positional_args.size()); };
string ArgumentParser::get_positional()
{
  if (!has_positional())
  {
    fs::logging::error("Not enough positional arguments");
    show_usage_and_exit();
  }
  // return from front and advance to next
  return positional_args[cur_arg++];
}
void ArgumentParser::done_positional()
{
  // should be exactly at size since increments after getting argument
  if (positional_args.size() != cur_arg)
  {
    fs::logging::error("Too many positional arguments");
    show_usage_and_exit();
  }
}
MainArgumentParser::MainArgumentParser(const int argc, const char* const argv[])
  : ArgumentParser(argc, argv)
{
  register_flag(&Settings::setSaveAsAscii, true, "--ascii", "Save grids as .asc");
  register_flag(&Settings::setSaveAsTiff, false, "--no-tiff", "Do not save grids as .tif");
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
}
void MainArgumentParser::parse_args()
{
  ArgumentParser::parse_args();
  // parse positional arguments
  // output directory is always the first thing
  // positional arguments all start with <output_dir> after mode (if applicable)
  // "./firestarr [surface] <output_dir> <yyyy-mm-dd> <lat> <lon> <HH:MM> [options] [-v | -q]"
  output_directory = get_positional();
  replace(output_directory.begin(), output_directory.end(), '\\', '/');
  if ('/' != output_directory[output_directory.length() - 1])
  {
    output_directory += '/';
  }
  // if name starts with "/" then it's an absolute path, otherwise append to working directory
  log_file = log_file_name.starts_with("/") ? log_file_name : (output_directory + log_file_name);
  // at this point we've parsed positional args and know we're not in test mode
  if (!PARSE_HAVE.contains("--apcp_prev"))
  {
    fs::logging::warning(
      "Assuming 0 precipitation between noon yesterday and weather start for startup indices"
    );
  }
}
FwiWeather MainArgumentParser::get_test_weather() const
{
  return {
    Weather{
      Temperature::Zero(),
      RelativeHumidity::Zero(),
      Wind{Direction{wind_direction, false}, Speed{wind_speed}},
      Precipitation::Zero()
    },
    ffmc,
    dmc,
    dc
  };
}
FwiWeather MainArgumentParser::get_yesterday_weather() const
{
  return {
    Weather{
      Temperature::Zero(),
      RelativeHumidity::Zero(),
      Wind{Direction{wind_direction, false}, Speed{wind_speed}},
      {apcp_prev}
    },
    ffmc,
    dmc,
    dc
  };
}
const char* cur_arg() { return ARGV[CUR_ARG]; };
bool parse_flag(bool not_inverse)
{
  return parse_once<bool>([not_inverse] { return not_inverse; });
}
}
