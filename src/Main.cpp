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
  cout << "Usage: " << BIN_NAME << " <output_dir> <yyyy-mm-dd> <lat> <lon> <HH:MM> [options] [-v | -q]" << endl
       << endl
       << " Run simulations and save output in the specified directory" << endl
       << endl
       << endl
       << "Usage: " << BIN_NAME << " test <output_dir> <numHours>"
       << "[slope [aspect [wind_speed [wind_direction]]]]" << endl
       << endl
       << " Run test cases and save output in the specified directory" << endl
       << endl
       << " Input Options" << endl;
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
    cout << endl
         << "Argument " << ARGV[CUR_ARG] << " already specified" << endl
         << endl;
    show_usage_and_exit();
  }
  return parse(fct);
}
bool parse_flag(bool not_inverse)
{
  return parse_once<bool>([not_inverse]
                          {
                            return not_inverse;
                          });
}
template <class T>
T parse_index()
{
  return parse_once<T>([]
                       {
                         return T(stod(get_arg()));
                       });
}
void register_argument(string v, string help, bool required, std::function<void()> fct)
{
  PARSE_FCT.emplace(v, fct);
  PARSE_HELP.emplace_back(v, help);
  PARSE_REQUIRED.emplace(v, required);
}
void register_flag(bool not_inverse, string v, string help, bool required, std::function<void(bool)> fct)
{
  register_argument(v,
                    help,
                    required,
                    [not_inverse, fct]
                    {
                      fct(parse_flag(not_inverse));
                    });
}
template <class T>
void register_index(T& index, string v, string help, bool required)
{
  register_argument(v,
                    help,
                    required,
                    [&index]
                    {
                      index = parse_index<T>();
                    });
}
int main(const int argc, const char* const argv[])
{
  ARGC = argc;
  ARGV = argv;
#ifndef NDEBUG
  cout << "**************************************************\n";
  cout << "******************* DEBUG MODE *******************\n";
  cout << "**************************************************\n";
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
  tbd::wx::Ffmc ffmc;
  tbd::wx::Dmc dmc;
  tbd::wx::Dc dc;
  tbd::wx::AccumulatedPrecipitation apcp_0800;
  // can be used multiple times
  register_argument("-v", "Increase output level", false, &Log::increaseLogLevel);
  // if they want to specify -v and -q then that's fine
  register_argument("-q", "Decrease output level", false, &Log::decreaseLogLevel);
  if (ARGC > 1 && 0 == strcmp(ARGV[1], "test"))
  {
    if (ARGC <= 3)
    {
      show_usage_and_exit();
    }
    return tbd::sim::test(ARGC, ARGV);
  }
  register_argument("-i",
                    "Save intensity maps for simulations",
                    false,
                    [&save_intensity]
                    {
                      save_intensity = parse_flag(true);
                    });
  register_flag(false, "-s", "Run in synchronous mode", false, &Settings::setRunAsync);
  register_flag(true, "--ascii", "Save grids as .asc", false, &Settings::setSaveAsAscii);
  register_flag(false, "--no-intensity", "Do not output intensity grids", false, &Settings::setSaveIntensity);
  register_flag(false, "--no-probability", "Do not output probability grids", false, &Settings::setSaveProbability);
  register_flag(true, "--occurrence", "Output occurrence grids", false, &Settings::setSaveOccurrence);
  register_argument("--wx",
                    "Input weather file",
                    true,
                    [&wx_file_name]
                    {
                      wx_file_name = parse_once<const char*>(&get_arg);
                    });

  register_argument("--confidence",
                    "Use specified confidence level",
                    false,
                    []
                    {
                      Settings::setConfidenceLevel(parse_once<double>([]
                                                                      {
                                                                        return stod(get_arg());
                                                                      }));
                    });
  register_argument("--perim",
                    "Start from perimeter",
                    false,
                    [&perim]
                    {
                      perim = parse_once<const char*>(&get_arg);
                    });
  register_argument("--size",
                    "Start from size",
                    false,
                    [&size]
                    {
                      size = parse_once<size_t>([]
                                                {
                                                  return static_cast<size_t>(stoi(get_arg()));
                                                });
                    });
  register_index<tbd::wx::Ffmc>(ffmc, "--ffmc", "Startup Fine Fuel Moisture Code", true);
  register_index<tbd::wx::Dmc>(dmc, "--dmc", "Startup Duff Moisture Code", true);
  register_index<tbd::wx::Dc>(dc, "--dc", "Startup Drought Code", true);
  register_index<tbd::wx::AccumulatedPrecipitation>(apcp_0800, "--apcp_0800", "Startup 0800 precipitation", false);
  register_argument("--output_date_offsets",
                    "Override output date offsets",
                    false,
                    []
                    {
                      Settings::setOutputDateOffsets(parse_once<const char*>([]
                                                                             {
                                                                               auto offsets = get_arg();
                                                                               tbd::logging::warning("Overriding output offsets with %s", offsets);
                                                                               return offsets;
                                                                             }));
                    });
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
          start_date.tm_min = stoi(arg.substr(3, 2));
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
            PARSE_FCT[ARGV[CUR_ARG]]();
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
      if (!PARSE_HAVE.contains("--apcp_0800"))
      {
        tbd::logging::warning("Assuming 0 precipitation for startup indices");
        apcp_0800 = tbd::wx::AccumulatedPrecipitation::Zero;
      }
      // HACK: ISI for yesterday really doesn't matter so just use any wind
      const auto isi_fixed = tbd::wx::Isi(tbd::wx::Speed(0), ffmc);
      const auto bui_fixed = tbd::wx::Bui(dmc, dc);
      const auto fwi_fixed = tbd::wx::Fwi(isi_fixed, bui_fixed);
      const auto yesterday = tbd::wx::FwiWeather(tbd::wx::Temperature(0),
                                                 tbd::wx::RelativeHumidity(0),
                                                 tbd::wx::Wind(tbd::wx::Direction(0, false), tbd::wx::Speed(0)),
                                                 tbd::wx::AccumulatedPrecipitation(0),
                                                 ffmc,
                                                 dmc,
                                                 dc,
                                                 isi_fixed,
                                                 bui_fixed,
                                                 fwi_fixed);
      tbd::util::fix_tm(&start_date);
      start = start_date;
      cout << "Arguments are:\n";
      for (auto j = 0; j < ARGC; ++j)
      {
        cout << " " << ARGV[j];
      }
      cout << "\n";
      return tbd::sim::Model::runScenarios(wx_file_name.c_str(),
                                           Settings::rasterRoot(),
                                           yesterday,
                                           start_point,
                                           start,
                                           save_intensity,
                                           perim,
                                           size);
    }
    show_usage_and_exit();
  }
  catch (const runtime_error& err)
  {
    tbd::logging::fatal(err.what());
  }
  Log::closeLogFile();
}
