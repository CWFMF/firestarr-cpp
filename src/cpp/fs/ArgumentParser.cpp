/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "ArgumentParser.h"
#include <utility>
#include "Log.h"
#include "Settings.h"
namespace fs::settings
{
static map<std::string, std::function<void()>> PARSE_FCT{};
static vector<std::pair<std::string, std::string>> PARSE_HELP{};
static map<std::string, bool> PARSE_REQUIRED{};
static map<std::string, bool> PARSE_HAVE{};
ArgumentParser* PARSER{nullptr};
void ArgumentParser::mark_parsed(const string arg) { PARSE_HAVE.emplace(arg, true); }
bool ArgumentParser::was_parsed(const string arg) { return PARSE_HAVE.contains(arg); }
template <class T>
T parse(auto fct)
{
  auto& parser = *PARSER;
  parser.mark_parsed(parser.cur_arg());
  // HACK: use auto instead of std::function<T()> so call is easier
  return static_cast<T>(fct());
}
template <class T>
T parse_once(auto fct)
{
  auto& parser = *PARSER;
  if (parser.was_parsed(parser.cur_arg()))
  {
    printf("\nArgument %s already specified\n\n", parser.cur_arg().c_str());
    parser.show_usage_and_exit();
  }
  // HACK: use auto instead of std::function<T()> so call is easier
  return parse<T>(fct);
}
bool parse_flag(bool not_inverse);
template <class T>
T parse_value()
{
  auto& parser = *PARSER;
  return parse_once<T>([&] { return stod(parser.get_arg()); });
}
size_t parse_size_t();
string parse_string();
template <class T>
T parse_index()
{
  auto& parser = *PARSER;
  return parse_once<T>([&] { return T(stod(parser.get_arg())); });
}
void register_argument(string v, string help, bool required, std::function<void()> fct);
template <class T>
void register_setter(
  std::function<void(T)> fct_set,
  string v,
  string help,
  bool required,
  std::function<T()> fct
)
{
  register_argument(v, help, required, [=] { fct_set(fct()); });
}
template <class T>
void register_setter(T& variable, string v, string help, bool required, std::function<T()> fct)
{
  register_argument(v, help, required, [&variable, fct] { variable = fct(); });
}
template <class T>
void register_setter(
  atomic<T>& variable,
  string v,
  string help,
  bool required,
  std::function<T()> fct
)
{
  register_argument(v, help, required, [&variable, fct] { variable = fct(); });
}
void register_flag(std::function<void(bool)> fct, bool not_inverse, string v, string help);
void register_flag(bool& variable, bool not_inverse, string v, string help);
template <class T>
void register_index(T& index, string v, string help, bool required)
{
  register_argument(v, help, required, [&] { index = parse_index<T>(); });
}
string ArgumentParser::get_args()
{
  std::string args{arguments_.at(0)};
  for (size_t i = 1; i < arguments_.size(); ++i)
  {
    args.append(" ");
    args.append(arguments_.at(i));
  }
  return args;
}
constexpr auto FMT_ARGS{"Arguments are:\n  %s\n\n"};
void ArgumentParser::show_args()
{
  auto args = get_args();
  printf(FMT_ARGS, args.c_str());
}
void ArgumentParser::log_args()
{
  auto args = get_args();
  fs::logging::note(FMT_ARGS, args.c_str());
}
static vector<Usage> USAGES{};
void add_usage(const Usage usage) { USAGES.emplace_back(usage); }
void add_usages(const vector<Usage> usages)
{
  for (const auto& u : usages)
  {
    add_usage(u);
  }
}
void ArgumentParser::show_usage_and_exit(int exit_code)
{
  // NOTE: this assumes there are always optional args
  //        (but -h, -v, -q should always be there)
  for (const auto& usage : USAGES)
  {
    // FIX: extra space if no positional args
    printf(
      "Usage: %s %s [OPTION]...\n\n%s\n\n",
      binary_name_.c_str(),
      usage.positional_arg_summary.c_str(),
      usage.description.c_str()
    );
  }
  printf(" Input Options\n");
  // FIX: this should show arguments specific to mode, but it doesn't indicate that on the outputs
  for (auto& kv : PARSE_HELP)
  {
    printf("   %-25s %s\n", kv.first.c_str(), kv.second.c_str());
  }
  exit(exit_code);
}
void ArgumentParser::show_usage_and_exit()
{
  show_args();
  show_usage_and_exit(-1);
}
void ArgumentParser::show_help_and_exit()
{
  // showing help isn't an error
  show_usage_and_exit(0);
}
string ArgumentParser::get_arg() noexcept
{
  // check if we don't have any more arguments
  fs::logging::check_fatal(
    cur_arg_ + 1 >= args_expanded().size(),
    "Missing argument to --%s",
    args_expanded().at(cur_arg_).c_str()
  );
  return args_expanded().at(++cur_arg_);
}
size_t parse_size_t()
{
  auto& parser = *PARSER;
  return parse_once<size_t>([&] { return static_cast<size_t>(stoi(parser.get_arg())); });
}
string parse_string()
{
  auto& parser = *PARSER;
  return parse_once<string>([&]() { return parser.get_arg(); });
}
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
void register_flag(atomic<bool>& variable, bool not_inverse, string v, string help)
{
  register_argument(v, help, false, [=, &variable] { variable = parse_flag(not_inverse); });
}
void register_flag(bool& variable, bool not_inverse, string v, string help)
{
  register_argument(v, help, false, [=, &variable] { variable = parse_flag(not_inverse); });
}
ArgumentParser::ArgumentParser(
  const Usage usage,
  const int argc,
  const char* const argv[],
  const PositionalArgumentsRequired require_positional
)
  : ArgumentParser(vector<Usage>{usage}, argc, argv, require_positional)
{ }
ArgumentParser::ArgumentParser(
  const vector<Usage> usages,
  const int argc,
  const char* const argv[],
  const PositionalArgumentsRequired require_positional
)
  : ArgumentParser(
      usages,
      [&]() {
        vector<std::string> args{};
        for (auto i = 0; i < argc; ++i)
        {
          args.emplace_back(argv[i]);
        }
        return args;
      }(),
      require_positional
    )
{ }
ArgumentParser::ArgumentParser(
  const vector<Usage> usages,
  const vector<string> arguments,
  const PositionalArgumentsRequired require_positional
)
  : ArgumentParser(
      usages,
      arguments,
      [&]() {
        auto bin = arguments.at(0);
        replace(bin.begin(), bin.end(), '\\', '/');
        const auto end = max(static_cast<size_t>(0), bin.rfind('/') + 1);
        auto directory = bin.substr(0, end);
        auto name = bin.substr(end, bin.size() - end);
        return std::make_pair(directory, name);
      }(),
      require_positional
    )
{ }
// HACK: already parsed binary from arg 0
ArgumentParser::ArgumentParser(
  const vector<Usage> usages,
  const vector<string> arguments,
  const pair<string, string> binary,
  const PositionalArgumentsRequired require_positional
)
  : require_positional_{require_positional}, cur_arg_{1}, arguments_{arguments},
    // HACK: already parsed binary from arg 0
    binary_directory_{std::get<0>(binary)}, binary_name_{std::get<1>(binary)}
{
  // HACK: count -v and -q before anything to get right log level
  constexpr auto log_default = fs::logging::LOG_NOTE;
  logging::Log::setLogLevel(log_default);
  for (const auto& arg : arguments)
  {
    if (arg.starts_with("-") && !arg.starts_with(("--")))
    {
      // HACK: not quite right if somehow a positional arg could start with '-' and have letters
      // increment for each -v and decrement for each -q
      for (const auto c : arg)
      {
        if ('q' == c)
        {
          logging::Log::decreaseLogLevel();
        }
        else if ('v' == c)
        {
          logging::Log::increaseLogLevel();
        }
      }
    }
  }
  // FIX: doing this here means we always see the settings if we haven't adjusted log level
  Settings::setRoot(binary_directory_);
  logging::check_fatal(nullptr != PARSER, "Parser initialized multiple times");
  PARSER = this;
  add_usages(usages);
  fs::show_debug_settings();
  assert(1 == cur_arg_);
  // HACK: revert log level so -v and -q set it
  logging::Log::setLogLevel(log_default);
  register_flag(help_requested_, true, "-h", "Show help");
  // can be used multiple times
  register_argument("-v", "Increase output level", false, &Log::increaseLogLevel);
  // if they want to specify -v and -q then that's fine
  register_argument("-q", "Decrease output level", false, &Log::decreaseLogLevel);
}
Settings& ArgumentParser::parse_args()
{
  // HACK: resolve once and fail if not set already
  static auto& settings = fs::settings::instance();
  auto& args = args_expanded();
  while (cur_arg_ < args.size())
  {
    const string arg = args.at(cur_arg_);
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
          // cur_arg_ would be incremented while trying to parse at this point, so -1 is 'arg'
          printf(
            "\n'%s' is not a valid value for argument %s\n\n",
            args.at(cur_arg_).c_str(),
            arg.c_str()
          );
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
      positional_args_.emplace_back(arg);
      fs::logging::debug("Found positional argument '%s'", arg.c_str());
    }
    ++cur_arg_;
  }
  if (help_requested_)
  {
    return settings;
  }
  for (auto& kv : PARSE_REQUIRED)
  {
    if (kv.second && PARSE_HAVE.end() == PARSE_HAVE.find(kv.first))
    {
      fs::logging::fatal("%s must be specified", kv.first.c_str());
    }
  }
  if ((PositionalArgumentsRequired::Required == require_positional_)
      == (0 == positional_args_.size()))
  {
    show_usage_and_exit();
  }
  // HACK: should never happen
  return settings;
}
bool ArgumentParser::has_positional() const { return (cur_positional_ < positional_args_.size()); };
string ArgumentParser::get_positional()
{
  if (!has_positional())
  {
    fs::logging::error("Not enough positional arguments");
    show_usage_and_exit();
  }
  // return from front and advance to next
  return positional_args_[cur_positional_++];
}
void ArgumentParser::done_positional()
{
  // should be exactly at size since increments after getting argument
  if (positional_args_.size() != cur_positional_)
  {
    fs::logging::error("Too many positional arguments");
    show_usage_and_exit();
  }
  // HACK: resolve once and fail if not set already
  static const auto& settings = settings::instance();
  // HACK: save settings here since should be parsed
  settings.saveTo(settings.output_directory);
}
static const Usage USAGE_MAIN{
  "Run simulations and save output in the specified directory",
  "<output_dir> <yyyy-mm-dd> <lat> <lon> <HH:MM>"
};
static const Usage USAGE_SURFACE{
  "Calculate probability surface and save output in the specified directory",
  "surface <output_dir> <yyyy-mm-dd> <lat> <lon> <HH:MM>"
};
static const Usage USAGE_TEST{
  "Run test cases and save output in the specified directory",
  "test <output_dir>"
};
static const vector<Usage> DEFAULT_USAGES{USAGE_MAIN, USAGE_SURFACE, USAGE_TEST};
Settings& SettingsArgumentParser::parse_args() { return ArgumentParser::parse_args(); }
MainArgumentParser::MainArgumentParser(const int argc, const char* const argv[])
  : SettingsArgumentParser(DEFAULT_USAGES, argc, argv)
{
  // HACK: resolve once and fail if not set already
  static auto& settings = fs::settings::instance();
  register_flag(settings.save_as_ascii, true, "--ascii", "Save grids as .asc");
  register_flag(settings.save_as_tiff, false, "--no-tiff", "Do not save grids as .tif");
  if (arguments_.size() > 1 && 0 == strcmp(arguments_.at(1).c_str(), "test"))
  {
    fs::logging::note("Running in test mode");
    settings.mode = Mode::Test;
    cur_arg_ += 1;
    skipped_args_ = 1;
    // if we have a directory and nothing else then use defaults for single run
    // if we have 'all' then overrride specified indices, but then filter down to the subset that
    // matches what was specified
    register_setter<
      MathSize>(settings.hours, "--hours", "Duration in hours", false, &parse_value<MathSize>);
    register_setter<string>(settings.fuel_name, "--fuel", "FBP fuel type", false, &parse_string);
    register_index<Ffmc>(settings.ffmc, "--ffmc", "Constant Fine Fuel Moisture Code", false);
    register_index<Dmc>(settings.dmc, "--dmc", "Constant Duff Moisture Code", false);
    register_index<Dc>(settings.dc, "--dc", "Constant Drought Code", false);
    register_setter<
      MathSize>(settings.wind_direction, "--wd", "Constant wind direction", false, &parse_value<MathSize>);
    register_setter<
      MathSize>(settings.wind_speed, "--ws", "Constant wind speed", false, &parse_value<MathSize>);
    register_setter<
      SlopeSize>(settings.slope, "--slope", "Constant slope", false, &parse_value<SlopeSize>);
    register_setter<
      AspectSize>(settings.aspect, "--aspect", "Constant slope aspect/azimuth", false, &parse_value<AspectSize>);
    register_setter<size_t>(
      [&](const auto v) { settings.static_curing = v; },
      "--curing",
      "Specify static grass curing",
      false,
      &parse_size_t
    );
    register_flag(settings.force_greenup, true, "--force-greenup", "Force green up for all fires");
    register_flag(
      settings.force_no_greenup, true, "--force-no-greenup", "Force no green up for all fires"
    );
  }
  else
  {
    register_flag(settings.save_individual, true, "-i", "Save individual maps for simulations");
    register_flag(settings.run_async, false, "-s", "Run in synchronous mode");
    register_flag(settings.save_points, true, "--points", "Save simulation points to file");
    register_flag(
      settings.save_intensity, false, "--no-intensity", "Do not output intensity grids"
    );
    register_flag(
      settings.save_probability, false, "--no-probability", "Do not output probability grids"
    );
    register_flag(settings.save_occurrence, true, "--occurrence", "Output occurrence grids");
    register_flag(
      settings.save_simulation_area, true, "--sim-area", "Output simulation area grids"
    );
    register_setter<string>(
      [&](const auto v) { settings.raster_root = v; },
      "--raster-root",
      "Use specified directory as raster root",
      false,
      &parse_string
    );
    register_setter<string>(
      [&](const auto v) { settings.fuel_lookup = v; },
      "--fuel-lut",
      "Use specified fuel lookup table",
      false,
      &parse_string
    );
    register_setter<
      DurationSize>(settings.utc_offset, "--tz", "UTC offset (hours)", true, &parse_value<DurationSize>);
    register_setter<size_t>(
      [&](const auto v) { settings.static_curing = v; },
      "--curing",
      "Specify static grass curing",
      false,
      &parse_size_t
    );
    register_flag(settings.force_greenup, true, "--force-greenup", "Force green up for all fires");
    register_flag(
      settings.force_no_greenup, true, "--force-no-greenup", "Force no green up for all fires"
    );
    register_setter<string>(
      settings.log_file_name, "--log", "Output log file", false, &parse_string
    );
    register_setter<size_t>(
      settings.salt,
      "--salt",
      "Specify salt to use for random seeds (default 0)",
      false,
      &parse_size_t
    );
    if (arguments_.size() > 1 && 0 == strcmp(arguments_.at(1).c_str(), "surface"))
    {
      fs::logging::note("Running in probability surface mode");
      settings.mode = Mode::Surface;
      // skip 'surface' argument if present
      cur_arg_ += 1;
      skipped_args_ = 1;
      register_index<Ffmc>(settings.ffmc, "--ffmc", "Constant Fine Fuel Moisture Code", true);
      register_index<Dmc>(settings.dmc, "--dmc", "Constant Duff Moisture Code", true);
      register_index<Dc>(settings.dc, "--dc", "Constant Drought Code", true);
      register_setter<
        MathSize>(settings.wind_direction, "--wd", "Constant wind direction", true, &parse_value<MathSize>);
      register_setter<
        MathSize>(settings.wind_speed, "--ws", "Constant wind speed", true, &parse_value<MathSize>);
    }
    else
    {
      register_setter<string>(
        settings.wx_file_name, "--wx", "Input weather file", true, &parse_string
      );
      register_flag(
        settings.deterministic,
        true,
        "--deterministic",
        "Run deterministically (100% chance of spread & survival)"
      );
      register_setter<
        ThresholdSize>(settings.confidence_level, "--confidence", "Use specified confidence level", false, &parse_value<ThresholdSize>);
      register_setter<string>(
        settings.perimeter, "--perim", "Start from perimeter", false, &parse_string
      );
      register_setter<size_t>(
        settings.initial_size, "--size", "Start from size", false, &parse_size_t
      );
      // HACK: want different text for same flag so define here too
      register_index<Ffmc>(settings.ffmc, "--ffmc", "Startup Fine Fuel Moisture Code", true);
      register_index<Dmc>(settings.dmc, "--dmc", "Startup Duff Moisture Code", true);
      register_index<Dc>(settings.dc, "--dc", "Startup Drought Code", true);
      register_index<Precipitation>(
        settings.apcp_prev,
        "--apcp_prev",
        "Startup precipitation between 1200 yesterday and start of hourly weather",
        false
      );
    }
    register_setter<string>(
      [&](const auto v) { settings.output_date_offsets = OutputDateOffsets{v}; },
      "--output_date_offsets",
      "Override output date offsets",
      false,
      &parse_string
    );
  }
}
Settings& MainArgumentParser::parse_args()
{
  auto& settings = SettingsArgumentParser::parse_args();
  if (help_requested())
  {
    return settings;
  }
  // fs::show_debug_settings();
  // parse positional arguments
  // output directory is always the first thing
  // positional arguments all start with <output_dir> after mode (if applicable)
  // "./firestarr [surface] <output_dir> <yyyy-mm-dd> <lat> <lon> <HH:MM> [options] [-v | -q]"
  settings.output_directory = [&]() {
    auto d = get_positional();
    replace(d.begin(), d.end(), '\\', '/');
    if ('/' != d[d.length() - 1])
    {
      d += '/';
    }
    return d;
  }();
  // if name starts with "/" then it's an absolute path, otherwise append to working directory
  settings.log_file = (settings.log_file_name.starts_with("/") ? "" : settings.output_directory)
                    + settings.log_file_name;
  // HACK: ensure settings initialized before doing this
  // probabalistic surface is computationally impossible at this point
  if (settings.is_surface())
  {
    settings.deterministic = true;
  }
  return settings;
}
string ArgumentParser::cur_arg() { return args_expanded().at(cur_arg_); };
bool parse_flag(bool not_inverse)
{
  return parse_once<bool>([not_inverse] { return not_inverse; });
}
vector<string>& ArgumentParser::args_expanded()
{
  // if empty then not parsed, or parsing is fast since no arguments
  if (arguments_expanded_.empty())
  {
    arguments_expanded_ = [&]() {
      vector<std::string> args{};
      for (const auto& s : arguments_)
      {
        if (s.starts_with("-") && !s.starts_with("--"))
        {
          // break anything starting with just one - into individual letters
          for (size_t i = 1; i < s.length(); ++i)
          {
            const string arg = string("-") + s.at(i);
            // if this isn't a flag then don't expand it
            if (PARSE_FCT.find(arg) != PARSE_FCT.end())
            {
              args.emplace_back(arg);
            }
            else if (1 == i)
            {
              // if this is just the start of a non-flag then leave it alone
              args.emplace_back(s);
              break;
            }
            else
            {
              return logging::fatal<vector<string>>(
                "Invalid argument %s found as part of combined flag argument %s",
                arg.c_str(),
                s.c_str()
              );
            }
          }
        }
        else
        {
          args.emplace_back(s);
        }
      }
      return args;
    }();
  }
  return arguments_expanded_;
}
}
