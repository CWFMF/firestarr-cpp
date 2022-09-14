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

/*! \mainpage TBD Documentation
 *
 * \section intro_sec Introduction
 *
 * TBD is a probabilistic fire growth model.
 */
#include "stdafx.h"
#include "Model.h"
#include "Scenario.h"
#include "Test.h"
#include "TimeUtil.h"
#include "Log.h"
using tbd::logging::Log;
using tbd::sim::Settings;
static const char* BIN_NAME = nullptr;
static map<std::string, std::function<void()>> PARSE_FCT{};
static vector<std::pair<std::string, std::string>> PARSE_HELP{};
static map<std::string, bool> PARSE_REQUIRED{};
static map<std::string, bool> PARSE_HAVE{};
static int ARGC = 0;
static const char* const* ARGV = nullptr;
static int CUR_ARG = 0;
void show_usage_and_exit()
{
  printf("Usage: %s <output_dir> <yyyy-mm-dd> <lat> <lon> <HH:MM> [options] [-v | -q]\n\n", BIN_NAME);
  printf(" Run simulations and save output in the specified directory\n\n\n");
  printf("Usage: %s test <output_dir> <numHours> [slope [aspect [wind_speed [wind_direction]]]]\n\n", BIN_NAME);
  printf(" Run test cases and save output in the specified directory\n\n");
  printf(" Input Options\n");
  for (auto& kv : PARSE_HELP)
  {
    printf("   %-25s %s\n", kv.first.c_str(), kv.second.c_str());
  }
  exit(-1);
}
const char* get_arg() noexcept
{
  // check if we don't have any more arguments
  tbd::logging::check_fatal(CUR_ARG + 1 >= ARGC, "Missing argument to --%s", ARGV[CUR_ARG]);
  // check if we have another flag right after
  tbd::logging::check_fatal('-' == ARGV[CUR_ARG + 1][0],
                            "Missing argument to --%s",
                            ARGV[CUR_ARG]);
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
double parse_double()
{
  return parse_once<double>([] { return stod(get_arg()); });
}
size_t parse_size_t()
{
  return parse_once<size_t>([] { return static_cast<size_t>(stoi(get_arg())); });
}
const char* parse_raw()
{
  return parse_once<const char*>(&get_arg);
}
string parse_string()
{
  return string(parse_raw());
}
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
void register_setter(std::function<void(T)> fct_set, string v, string help, bool required, std::function<T()> fct)
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
  register_argument(v, help, false, [not_inverse, &variable] { variable = parse_flag(not_inverse); });
}
template <class T>
void register_index(T& index, string v, string help, bool required)
{
  register_argument(v, help, required, [&index] { index = parse_index<T>(); });
}
int main(const int argc, const char* const argv[])
{
  ARGC = argc;
  ARGV = argv;
#ifndef NDEBUG
  printf("**************************************************\n");
  printf("******************* DEBUG MODE *******************\n");
  printf("**************************************************\n");
#endif
  // _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  Log::setLogLevel(tbd::logging::LOG_NOTE);
  auto bin = string(ARGV[CUR_ARG++]);
  replace(bin.begin(), bin.end(), '\\', '/');
  const auto end = max(static_cast<size_t>(0), bin.rfind('/') + 1);
  bin = bin.substr(end, bin.size() - end);
  BIN_NAME = bin.c_str();
  register_argument("-h", "Show help", false, &show_usage_and_exit);
  auto save_intensity = false;
  string wx_file_name;
  string perim;
  size_t size = 0;
  // can be used multiple times
  register_argument("-v", "Increase output level", false, &Log::increaseLogLevel);
  // if they want to specify -v and -q then that's fine
  register_argument("-q", "Decrease output level", false, &Log::decreaseLogLevel);
  auto result = -1;
  if (ARGC > 1 && 0 == strcmp(ARGV[1], "test"))
  {
    if (ARGC <= 3)
    {
      show_usage_and_exit();
    }
    result = tbd::sim::test(ARGC, ARGV);
  }
  else
  {
    register_flag(save_intensity, true, "-i", "Save intensity maps for simulations");
    register_flag(&Settings::setRunAsync, false, "-s", "Run in synchronous mode");
    register_flag(&Settings::setSaveAsAscii, true, "--ascii", "Save grids as .asc");
    register_flag(&Settings::setSaveIntensity, false, "--no-intensity", "Do not output intensity grids");
    register_flag(&Settings::setSaveProbability, false, "--no-probability", "Do not output probability grids");
    register_flag(&Settings::setSaveOccurrence, true, "--occurrence", "Output occurrence grids");
    register_setter<string>(wx_file_name, "--wx", "Input weather file", true, &parse_string);
    register_setter<double>(&Settings::setConfidenceLevel, "--confidence", "Use specified confidence level", false, &parse_double);
    register_setter<string>(perim, "--perim", "Start from perimeter", false, &parse_string);
    register_setter<size_t>(size, "--size", "Start from size", false, &parse_size_t);
    register_setter<const char*>(&Settings::setOutputDateOffsets, "--output_date_offsets", "Override output date offsets", false, &parse_raw);
    if (3 > ARGC)
    {
      show_usage_and_exit();
    }
    try
    {
      if (6 <= ARGC)
      {
        string output_directory(ARGV[CUR_ARG++]);
        replace(output_directory.begin(), output_directory.end(), '\\', '/');
        if ('/' != output_directory[output_directory.length() - 1])
        {
          output_directory += '/';
        }
        Settings::setOutputDirectory(output_directory);
        struct stat info
        {
        };
        if (stat(Settings::outputDirectory(), &info) != 0 || !(info.st_mode & S_IFDIR))
        {
          tbd::util::make_directory_recursive(Settings::outputDirectory());
        }
        const string log_file = (string(Settings::outputDirectory()) + "log.txt");
        tbd::logging::check_fatal(!Log::openLogFile(log_file.c_str()),
                                  "Can't open log file");
        tbd::logging::note("Output directory is %s", Settings::outputDirectory());
        tbd::logging::note("Output log is %s", log_file.c_str());
        string date(ARGV[CUR_ARG++]);
        tm start_date{};
        start_date.tm_year = stoi(date.substr(0, 4)) - 1900;
        start_date.tm_mon = stoi(date.substr(5, 2)) - 1;
        start_date.tm_mday = stoi(date.substr(8, 2));
        const auto latitude = stod(ARGV[CUR_ARG++]);
        const auto longitude = stod(ARGV[CUR_ARG++]);
        const tbd::topo::StartPoint start_point(latitude, longitude);
        size_t num_days = 0;
        string arg(ARGV[CUR_ARG++]);
        tm start{};
        if (5 == arg.size() && ':' == arg[2])
        {
          try
          {
            // if this is a time then we aren't just running the weather
            start_date.tm_hour = stoi(arg.substr(0, 2));
            tbd::logging::check_fatal(start_date.tm_hour < 0 || start_date.tm_hour > 23,
              "Simulation start time has an invalid hour (%d)",
                                      start_date.tm_hour);
            start_date.tm_min = stoi(arg.substr(3, 2));
            tbd::logging::check_fatal(start_date.tm_min < 0 || start_date.tm_min > 59,
                                      "Simulation start time has an invalid minute (%d)",
                                      start_date.tm_min);
            tbd::util::fix_tm(&start_date);
            // we were given a time, so number of days is until end of year
            start = start_date;
            const auto start_t = mktime(&start);
            auto year_end = start;
            year_end.tm_mon = 11;
            year_end.tm_mday = 31;
            const auto seconds = difftime(mktime(&year_end), start_t);
            // start day counts too, so +1
            // HACK: but we don't want to go to Jan 1 so don't add 1
            num_days = static_cast<size_t>(seconds / tbd::DAY_SECONDS);
            tbd::logging::debug("Calculated number of days until end of year: %d",
                                num_days);
            // +1 because day 1 counts too
            // +2 so that results don't change when we change number of days
            num_days = min(num_days, static_cast<size_t>(Settings::maxDateOffset()) + 2);
          }
          catch (std::exception&)
          {
            show_usage_and_exit();
          }
          while (CUR_ARG < ARGC)
          {
            if (PARSE_FCT.find(ARGV[CUR_ARG]) != PARSE_FCT.end())
            {
              try
              {
                PARSE_FCT[ARGV[CUR_ARG]]();
              }
              catch (std::exception&)
              {
                printf("\n'%s' is not a valid value for argument %s\n\n", ARGV[CUR_ARG], ARGV[CUR_ARG - 1]);
                show_usage_and_exit();
              }
            }
            else
            {
              show_usage_and_exit();
            }
            ++CUR_ARG;
          }
        }
        else
        {
          show_usage_and_exit();
        }
        for (auto& kv : PARSE_REQUIRED)
        {
          if (kv.second && PARSE_HAVE.end() == PARSE_HAVE.find(kv.first))
          {
            tbd::logging::fatal("%s must be specified", kv.first.c_str());
          }
        }
        tbd::util::fix_tm(&start_date);
        start = start_date;
        printf("Arguments are:\n");
        for (auto j = 0; j < ARGC; ++j)
        {
          printf(" %s", ARGV[j]);
        }
        printf("\n");
        result = tbd::sim::Model::runScenarios(wx_file_name.c_str(),
                                               Settings::rasterRoot(),
                                               start_point,
                                               start,
                                               save_intensity,
                                               perim,
                                               size);
        Log::closeLogFile();
      }
      else
      {
        show_usage_and_exit();
      }
    }
    catch (const runtime_error& err)
    {
      tbd::logging::fatal(err.what());
    }
  }
  return result;
}
