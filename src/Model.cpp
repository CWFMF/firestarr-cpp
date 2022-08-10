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
#include "Model.h"
#include "Scenario.h"
#include "FBP45.h"
#include "Observer.h"
#include "Perimeter.h"
#include "ProbabilityMap.h"
#include "UTM.h"
namespace tbd::sim
{
Semaphore Model::task_limiter{static_cast<int>(std::thread::hardware_concurrency())};
BurnedData* Model::getBurnedVector() const noexcept
{
  try
  {
    lock_guard<mutex> lock(vector_mutex_);
    if (!vectors_.empty())
    {
      // check again once we have the mutex
      if (!vectors_.empty())
      {
        const auto v = std::move(vectors_.back()).release();
        vectors_.pop_back();
        // this is already reset before it was given back
        return v;
      }
    }
    auto result = environment().makeBurnedData().release();
    //    environment().resetBurnedData(result);
    return result;
  }
  catch (...)
  {
    std::terminate();
  }
}
void Model::releaseBurnedVector(BurnedData* has_burned) const noexcept
{
  if (nullptr == has_burned)
  {
    return;
  }
  try
  {
    environment().resetBurnedData(has_burned);
    lock_guard<mutex> lock(vector_mutex_);
    vectors_.push_back(unique_ptr<BurnedData>(has_burned));
  }
  catch (...)
  {
    std::terminate();
  }
}
Model::Model(const topo::StartPoint& start_point,
             topo::Environment* env)
  : start_time_(Clock::now()),
    time_limit_(std::chrono::seconds(Settings::maximumTimeSeconds())),
    env_(env)
{
  logging::debug("Calculating for (%f, %f)", start_point.latitude(), start_point.longitude());
  const auto nd_for_point =
    calculate_nd_for_point(env->elevation(), start_point);
  for (auto day = 0; day < MAX_DAYS; ++day)
  {
    nd_.at(static_cast<size_t>(day)) = static_cast<int>(day - nd_for_point);
    logging::verbose("Day %d has nd %d, is%s green, %d%% curing",
                     day,
                     nd_.at(static_cast<size_t>(day)),
                     fuel::calculate_is_green(nd_.at(static_cast<size_t>(day)))
                       ? ""
                       : " not",
                     fuel::calculate_grass_curing(nd_.at(static_cast<size_t>(day))));
  }
}
void Model::readWeather(const string& filename,
                        const wx::FwiWeather& yesterday,
                        const double latitude)
{
  map<size_t, map<Day, wx::FwiWeather>> wx{};
  map<Day, struct tm> dates{};
  auto min_date = numeric_limits<Day>::max();
  ifstream in;
  in.open(filename);
  logging::check_fatal(!in.is_open(),
                       "Could not open input weather file %s",
                       filename.c_str());
  if (in.is_open())
  {
    string str;
    logging::info("Reading scenarios from '%s'", filename.c_str());
    // read header line
    getline(in, str);
    // get rid of whitespace
    str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
    str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
    str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
    constexpr auto expected_header =
      "Scenario,Date,APCP,TMP,RH,WS,WD";
    logging::check_fatal(expected_header != str,
                         "Input CSV must have columns in this order:\n'%s'\n but got:\n'%s'",
                         expected_header,
                         str.c_str());
    auto prev = yesterday;
    while (getline(in, str))
    {
      istringstream iss(str);
      if (getline(iss, str, ',') && !str.empty())
      {
        // HACK: ignore date and just worry about relative order??
        // Scenario
        logging::verbose("Scenario is %s", str.c_str());
        auto cur = 0;
        try
        {
          cur = static_cast<size_t>(-stoi(str));
        }
        catch (std::exception&)
        {
          // HACK: somehow stoi() is still getting empty strings
          logging::fatal("Error reading weather file %s: %s is not a valid integer", filename.c_str(), str.c_str());
        }
        if (wx.find(cur) == wx.end())
        {
          logging::debug("Loading scenario %d...", cur);
          wx.emplace(cur, map<Day, wx::FwiWeather>());
          prev = yesterday;
        }
        auto& s = wx.at(cur);
        struct tm t
        {
        };
        util::read_date(&iss, &str, &t);
        year_ = t.tm_year + 1900;
        const auto ticks = mktime(&t);
        if (1 == cur)
        {
          logging::debug("Date '%s' is %ld and calculated jd is %d",
                         str.c_str(),
                         ticks,
                         t.tm_yday);
          if (!s.empty() && t.tm_yday < min_date)
          {
            logging::fatal(
              "Weather input file crosses year boundary or dates are not sequential");
          }
        }
        min_date = min(min_date, static_cast<Day>(t.tm_yday));
        logging::check_fatal(s.find(static_cast<Day>(t.tm_yday)) != s.end(),
                             "Day already exists");
        const auto month = t.tm_mon + 1;
        s.emplace(static_cast<Day>(t.tm_yday),
                  wx::FwiWeather(&iss,
                                 &str,
                                 prev,
                                 month,
                                 latitude));
        prev = s.at(static_cast<Day>(t.tm_yday));
        if (s.find(static_cast<Day>(t.tm_yday)) == s.end())
        {
          dates.emplace(static_cast<Day>(t.tm_yday), t);
        }
      }
    }
    in.close();
  }
  for (auto& kv : wx)
  {
    kv.second.emplace(static_cast<Day>(min_date - 1), yesterday);
  }
  const auto file_out = string(Settings::outputDirectory()) + "/wx_out.csv";
  FILE* out = fopen(file_out.c_str(), "w");
  logging::check_fatal(nullptr == out, "Cannot open file %s for output", file_out.c_str());
  fprintf(out, "Scenario,Day,APCP,TMP,RH,WS,WD,FFMC,DMC,DC,ISI,BUI,FWI\n");
  size_t i = 1;
  for (auto& kv : wx)
  {
    auto& s = kv.second;
    for (auto& kv2 : s)
    {
      auto& day = kv2.first;
      auto& w = kv2.second;
      fprintf(out,
              "%ld,%d,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g\n",
              i,
              day,
              w.apcp().asDouble(),
              w.tmp().asDouble(),
              w.rh().asDouble(),
              w.wind().speed().asDouble(),
              w.wind().direction().asDouble(),
              w.ffmc().asDouble(),
              w.dmc().asDouble(),
              w.dc().asDouble(),
              w.isi().asDouble(),
              w.bui().asDouble(),
              w.fwi().asDouble());
    }
    ++i;
  }
  logging::check_fatal(0 != fclose(out), "Could not close file %s", file_out.c_str());
  const auto fuel_lookup = sim::Settings::fuelLookup();
  // loop through and try to find duplicates
  for (const auto& kv : wx)
  {
    const auto k = kv.first;
    const auto s = kv.second;
    if (wx_.find(k) == wx_.end())
    {
      const auto w = make_shared<wx::FireWeather>(fuel_lookup.usedFuels(), s);
      wx_.emplace(k, w);
    }
  }
}
void Model::findStarts(const Location location)
{
  logging::error("Trying to start a fire in non-fuel");
  Idx range = 1;
  // HACK: should always be centered in the grid
  while (starts_.empty() && (range < (MAX_COLUMNS / 2)))
  {
    for (Idx x = -range; x <= range; ++x)
    {
      for (Idx y = -range; y <= range; ++y)
      {
        // make sure we only look at the outside of the box
        if (1 == range || abs(x) == range || abs(y) == range)
        {
          //          const auto loc = env_->cell(location.hash() + (y * MAX_COLUMNS) + x);
          const auto loc = env_->cell(Location(location.row() + y, location.column() + x));
          if (!fuel::is_null_fuel(loc))
          {
            starts_.push_back(make_shared<topo::Cell>(cell(loc)));
          }
        }
      }
    }
    ++range;
  }
  logging::check_fatal(starts_.empty(), "Fuel grid is empty");
  logging::info("Using %d start locations:", starts_.size());
  for (const auto& s : starts_)
  {
    logging::info("\t%d, %d", s->row(), s->column());
  }
}
void Model::makeStarts(Coordinates coordinates,
                       const topo::Point& point,
                       const wx::FwiWeather& yesterday,
                       const string& perim,
                       const size_t size)
{
  const Location location(std::get<0>(coordinates), std::get<1>(coordinates));
  if (!perim.empty())
  {
    logging::note("Initializing from perimeter %s", perim.c_str());
    perimeter_ = make_shared<topo::Perimeter>(perim, point, *env_);
  }
  else if (size > 0)
  {
    logging::note("Initializing from size %d ha", size);
    perimeter_ = make_shared<topo::Perimeter>(
      cell(location),
      size,
      yesterday,
      *env_);
  }
  // figure out where the fire can exist
  if (nullptr != perimeter_ && !perimeter_->burned().empty())
  {
    logging::check_fatal(size != 0 && !perim.empty(), "Can't specify size and perimeter");
    // we have a perimeter to start from
    // HACK: make sure this isn't empty
    starts_.push_back(make_shared<topo::Cell>(cell(location)));
    logging::note("Fire starting with size %0.1f ha",
                  perimeter_->burned().size() * env_->cellSize() / 100.0);
  }
  else
  {
    if (nullptr != perimeter_)
    {
      logging::check_fatal(!perimeter_->burned().empty(),
                           "Not using perimeter so it should be empty");
      logging::note("Using fire perimeter results in empty fire - changing to use point");
      perimeter_ = nullptr;
    }
    logging::note("Fire starting with size %0.1f ha", env_->cellSize() / 100.0);
    //    if (0 == size && fuel::is_null_fuel(cell(location.hash())))
    if (0 == size && fuel::is_null_fuel(cell(location)))
    {
      findStarts(location);
    }
    else
    {
      starts_.push_back(make_shared<topo::Cell>(cell(location)));
    }
  }
  logging::note("Creating %ld streams x %ld location%s = %ld scenarios",
                wx_.size(),
                starts_.size(),
                starts_.size() > 1 ? "s" : "",
                wx_.size() * starts_.size());
}
Iteration Model::readScenarios(const topo::StartPoint& start_point,
                               const double start,
                               const bool save_intensity,
                               const Day start_day,
                               const Day last_date)
{
  vector<Scenario*> result{};
  auto saves = Settings::outputDateOffsets();
  const auto setup_scenario = [&result, save_intensity, &saves](Scenario* scenario) {
    if (save_intensity)
    {
      scenario->registerObserver(new IntensityObserver(*scenario, ""));
      scenario->registerObserver(new ArrivalObserver(*scenario));
      scenario->registerObserver(new SourceObserver(*scenario));
    }
    // FIX: this should be relative to the start date, not the weather start date
    for (const auto& i : saves)
    {
      scenario->addSaveByOffset(i);
    }
    result.push_back(scenario);
  };
  for (const auto& kv : wx_)
  {
    const auto id = kv.first;
    const auto cur_wx = kv.second;
    if (nullptr != perimeter_)
    {
      setup_scenario(new Scenario(this,
                                  id,
                                  cur_wx.get(),
                                  start,
                                  perimeter_,
                                  start_point,
                                  start_day,
                                  last_date));
    }
    else
    {
      for (const auto& cur_start : starts_)
      {
        // should always have at least the day before the fire in the weather stream
        setup_scenario(new Scenario(this,
                                    id,
                                    cur_wx.get(),
                                    start,
                                    cur_start,
                                    start_point,
                                    start_day,
                                    last_date));
      }
    }
  }
  return Iteration(result);
}
bool Model::isOutOfTime() const noexcept
{
  return (Clock::now() - startTime()) > timeLimit();
}
ProbabilityMap* Model::makeProbabilityMap(const double time,
                                          const double start_time,
                                          const int min_value,
                                          const int low_max,
                                          const int med_max,
                                          const int max_value) const
{
  return env_->makeProbabilityMap(time,
                                  start_time,
                                  min_value,
                                  low_max,
                                  med_max,
                                  max_value);
}
static void show_probabilities(const map<double, ProbabilityMap*>& probabilities)
{
  for (const auto& kv : probabilities)
  {
    kv.second->show();
  }
}
map<double, ProbabilityMap*> make_prob_map(const Model& model,
                                           const vector<double>& saves,
                                           const double started,
                                           const int min_value,
                                           const int low_max,
                                           const int med_max,
                                           const int max_value)
{
  map<double, ProbabilityMap*> result{};
  for (const auto& time : saves)
  {
    result.emplace(
      time,
      model.makeProbabilityMap(time,
                               started,
                               min_value,
                               low_max,
                               med_max,
                               max_value));
  }
  return result;
}
map<double, util::SafeVector*> make_size_map(const vector<double>& saves)
{
  map<double, util::SafeVector*> result{};
  for (const auto& time : saves)
  {
    result.emplace(time, new util::SafeVector());
  }
  return result;
}
bool add_statistics(const size_t i,
                    vector<double>* means,
                    vector<double>* pct,
                    const Model& model,
                    const util::SafeVector& v)
{
  const auto sizes = v.getValues();
  logging::check_fatal(sizes.empty(), "No sizes at end of simulation");
  const util::Statistics s{sizes};
  static_cast<void>(util::insert_sorted(pct, s.percentile(95)));
  static_cast<void>(util::insert_sorted(means, s.mean()));
  if (model.isOutOfTime())
  {
    logging::note(
      "Stopping after %d iterations. Time limit of %d seconds has been reached.",
      i,
      Settings::maximumTimeSeconds());
    return false;
  }
  return true;
}
/*!
 * \page ending Simulation stop conditions
 *
 * Simulations will continue to run until a stop condition is reached.
 *
 * 1) the program has reached the time defined in the settings file as the maximum
 * run duration.
 *
 * 2) the amount of variability in the output statistics has decreased to a point
 * that is less than the confidence level defined in the settings file
 */
size_t runs_required(const size_t i,
                     const vector<double>* means,
                     const vector<double>* pct,
                     const Model& model)
{
  if (model.isOutOfTime())
  {
    logging::note(
      "Stopping after %d iterations. Time limit of %d seconds has been reached.",
      i,
      Settings::maximumTimeSeconds());
    return 0;
  }
  const auto for_means = util::Statistics{*means};
  const auto for_pct = util::Statistics{*pct};
  if (!(!for_means.isConfident(Settings::confidenceLevel())
        || !for_pct.isConfident(Settings::confidenceLevel())))
  {
    return 0;
  }
  const auto left = max(for_means.runsRequired(i, Settings::confidenceLevel()),
                        for_pct.runsRequired(i, Settings::confidenceLevel()));
  return left;
}
map<double, ProbabilityMap*> Model::runIterations(const topo::StartPoint& start_point,
                                                  const double start,
                                                  const Day start_day,
                                                  const bool save_intensity)
{
  auto last_date = start_day;
  for (const auto i : Settings::outputDateOffsets())
  {
    last_date = max(static_cast<Day>(start_day + i), last_date);
  }
  // use independent seeds so that if we remove one threshold it doesn't affect the other
  std::seed_seq seed_spread{0.0, start, start_point.latitude(), start_point.longitude()};
  std::seed_seq seed_extinction{1.0, start, start_point.latitude(), start_point.longitude()};
  mt19937 mt_spread(seed_spread);
  mt19937 mt_extinction(seed_extinction);
  vector<double> means{};
  vector<double> pct{};
  size_t i = 0;
  auto iterations = readScenarios(start_point,
                                  start,
                                  save_intensity,
                                  start_day,
                                  last_date);
  // put probability maps into map
  const auto saves = iterations.savePoints();
  const auto started = iterations.startTime();
  auto probabilities = make_prob_map(*this,
                                     saves,
                                     started,
                                     0,
                                     Settings::intensityMaxLow(),
                                     Settings::intensityMaxModerate(),
                                     numeric_limits<int>::max());
  vector<map<double, ProbabilityMap*>> all_probabilities{};
  all_probabilities.push_back(make_prob_map(*this,
                                            saves,
                                            started,
                                            0,
                                            Settings::intensityMaxLow(),
                                            Settings::intensityMaxModerate(),
                                            numeric_limits<int>::max()));
  auto runs_left = 1;
  // HACK: just do this here so that we know it happened
  //iterations.reset(&mt_extinction, &mt_spread);
  if (Settings::runAsync())
  {
    vector<Iteration> all_iterations{};
    all_iterations.push_back(std::move(iterations));
    auto threads = list<std::thread>{};
    for (size_t x = 1; x < std::thread::hardware_concurrency() / 4; ++x)
    {
      all_iterations.push_back(readScenarios(start_point,
                                             start,
                                             save_intensity,
                                             start_day,
                                             last_date));
      all_probabilities.push_back(make_prob_map(*this,
                                                saves,
                                                started,
                                                0,
                                                Settings::intensityMaxLow(),
                                                Settings::intensityMaxModerate(),
                                                numeric_limits<int>::max()));
    }
    size_t cur_iter = 0;
    for (auto& iter : all_iterations)
    {
      iter.reset(&mt_extinction, &mt_spread);
      auto& scenarios = iter.getScenarios();
      for (auto s : scenarios)
      {
        threads.emplace_back(&Scenario::run,
                             s,
                             &all_probabilities[cur_iter]);
      }
      ++cur_iter;
    }
    cur_iter = 0;
    while (runs_left > 0)
    {
      // should have completed one iteration, so add it
      auto& iteration = all_iterations[cur_iter];
      // so now try to loop through and add iterations as they finish
      size_t k = 0;
      while (k < iteration.size())
      {
        threads.front().join();
        threads.pop_front();
        ++k;
      }
      auto final_sizes = iteration.finalSizes();
      ++i;
      for (auto& kv : all_probabilities[cur_iter])
      {
        probabilities[kv.first]->addProbabilities(*kv.second);
        // clear so we don't double count
        kv.second->reset();
      }
      if (!add_statistics(i, &means, &pct, *this, final_sizes))
      {
        // ran out of time
        for (auto& iter : all_iterations)
        {
          iter.cancel();
        }
        for (auto& t : threads)
        {
          if (t.joinable())
          {
            t.join();
          }
        }
        return probabilities;
      }
      runs_left = runs_required(i, &means, &pct, *this);
      logging::note("Need another %d iterations", runs_left);
      if (runs_left > 0)
      {
        iteration.reset(&mt_extinction, &mt_spread);
        auto& scenarios = iteration.getScenarios();
        for (auto s : scenarios)
        {
          threads.emplace_back(&Scenario::run,
                               s,
                               &all_probabilities[cur_iter]);
        }
        ++cur_iter;
        // loop around to start if required
        cur_iter %= all_iterations.size();
      }
      else
      {
        for (auto& iter : all_iterations)
        {
          iter.cancel();
        }
        for (auto& t : threads)
        {
          if (t.joinable())
          {
            t.join();
          }
        }
      }
    }
    // everything should be done when this section ends
  }
  else
  {
    logging::note("Running in synchronous mode");
    while (runs_left > 0)
    {
      logging::note("Running iteration %d", i + 1);
      iterations.reset(&mt_extinction, &mt_spread);
      for (auto s : iterations.getScenarios())
      {
        s->run(&probabilities);
      }
      ++i;
      if (!add_statistics(i, &means, &pct, *this, iterations.finalSizes()))
      {
        // ran out of time
        return probabilities;
      }
      runs_left = runs_required(i, &means, &pct, *this);
      logging::note("Need another %d iterations", runs_left);
    }
  }
  return probabilities;
}
int Model::runScenarios(const char* const weather_input,
                        const char* const raster_root,
                        const wx::FwiWeather& yesterday,
                        const topo::StartPoint& start_point,
                        const tm& start_time,
                        const bool save_intensity,
                        const string& perimeter,
                        const size_t size)
{
  auto env = topo::Environment::loadEnvironment(raster_root,
                                                start_point,
                                                perimeter,
                                                start_time.tm_year);
  logging::debug("Environment loaded");
  const auto position = env.findCoordinates(start_point, true);
#ifndef NDEBUG
  logging::check_fatal(
    std::get<0>(*position) > MAX_ROWS || std::get<1>(*position) > MAX_COLUMNS,
    "Location loaded outside of grid at position (%d, %d)",
    std::get<0>(*position),
    std::get<1>(*position));
#endif
  logging::info("Position is (%d, %d)", std::get<0>(*position), std::get<1>(*position));
  const Location location{std::get<0>(*position), std::get<1>(*position)};
  Model model(start_point, &env);
  auto x = 0.0;
  auto y = 0.0;
  const auto zone = lat_lon_to_utm(start_point, &x, &y);
  logging::note("UTM coordinates are: %d %d %d",
                zone,
                static_cast<int>(x),
                static_cast<int>(y));
  logging::note("Grid has size (%d, %d)", env.rows(), env.columns());
  logging::note("Fire start position is cell (%d, %d)",
                location.row(),
                location.column());
  model.readWeather(weather_input, yesterday, start_point.latitude());
  if (model.wx_.empty())
  {
    logging::fatal("No weather provided");
  }
  const auto w = model.wx_.begin()->second;
  logging::debug("Have weather from day %d to %d", w->minDate(), w->maxDate());
  const auto numDays = (w->maxDate() - w->minDate() + 1);
  const auto needDays = Settings::maxDateOffset();
  if (numDays < needDays)
  {
    logging::fatal("Not enough weather to proceed - have %d days but looking for %d", numDays, needDays);
  }
  // want to output internal representation of weather to file
  model.outputWeather();
  model.makeStarts(*position, start_point, yesterday, perimeter, size);
  auto start_hour = ((start_time.tm_hour + (static_cast<double>(start_time.tm_min) / 60))
                     / DAY_HOURS);
  // HACK: round to 2 digits so that we can keep test output the same
  start_hour = static_cast<double>(static_cast<int>(start_hour * 100)) / 100;
  const auto start = start_time.tm_yday + start_hour;
  const auto start_day = static_cast<Day>(start);
  auto probabilities =
    model.runIterations(start_point, start, start_day, save_intensity);
  logging::note("Ran %d simulations", Scenario::completed());
  show_probabilities(probabilities);
  auto final_time = numeric_limits<double>::min();
  for (const auto by_time : probabilities)
  {
    const auto time = by_time.first;
    final_time = max(final_time, time);
    const auto prob = by_time.second;
    prob->saveAll(model, start_time, time, start_day);
  }
  for (const auto& kv : probabilities)
  {
    delete kv.second;
  }
  return 0;
}
void Model::outputWeather()
{
  const auto file_out = string(Settings::outputDirectory()) + "/wx_hourly_out.csv";
  FILE* out = fopen(file_out.c_str(), "w");
  logging::check_fatal(nullptr == out, "Cannot open file %s for output", file_out.c_str());
  fprintf(out, "Scenario,Date,APCP,TMP,RH,WS,WD,FFMC,DMC,DC,ISI,BUI,FWI\n");
  size_t i = 1;
  for (auto& kv : wx_)
  {
    auto& s = kv.second;
    // do we need to index this by hour and day?
    // was assuming it started at 0 for first hour and day
    auto wx = s->getWeather();
    size_t min_hour = s->minDate() * DAY_HOURS;
    size_t wx_size = wx->size();
    size_t hour = min_hour;
    for (size_t j = 0; j < wx_size; ++j)
    {
      size_t day = hour / 24;
      auto w = wx->at(hour - min_hour);
      size_t month;
      size_t day_of_month;
      month_and_day(year_, day, &month, &day_of_month);
      if (nullptr != w)
      {
        fprintf(out,
                "%ld,%d-%02ld-%02ld %02ld:00,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g\n",
                i,
                year_,
                month,
                day_of_month,
                hour - day * DAY_HOURS,
                w->apcp().asDouble(),
                w->tmp().asDouble(),
                w->rh().asDouble(),
                w->wind().speed().asDouble(),
                w->wind().direction().asDouble(),
                w->ffmc().asDouble(),
                w->dmc().asDouble(),
                w->dc().asDouble(),
                w->isi().asDouble(),
                w->bui().asDouble(),
                w->fwi().asDouble());
      }
      else
      {
        fprintf(out,
                "%ld,%d-%02ld-%02ld %02ld:00,,,,,,,,,,,\n",
                i,
                year_,
                month,
                day_of_month,
                hour - day * DAY_HOURS);
      }
      ++hour;
    }
    ++i;
  }
  logging::check_fatal(0 != fclose(out), "Could not close file %s", file_out.c_str());
}
}
