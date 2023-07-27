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
#include <chrono>
#include "Model.h"
#include "Scenario.h"
#include "FBP45.h"
#include "Observer.h"
#include "Perimeter.h"
#include "ProbabilityMap.h"
#include "UTM.h"
#include "FireWeatherDaily.h"
namespace tbd::sim
{
// constexpr double PCT_CPU = 0.8;
// HACK: assume using half the CPUs probably means that faster cores are being used?
constexpr double PCT_CPU = 0.5;
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
  catch (const std::exception& ex)
  {
    logging::fatal(ex);
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
  catch (const std::exception& ex)
  {
    logging::fatal(ex);
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
void Model::readWeather(const wx::FwiWeather& yesterday,
                        const double latitude,
                        const string& filename)
{
  map<size_t, vector<const wx::FwiWeather*>*> wx{};
  map<size_t, map<Day, wx::FwiWeather>> wx_daily{};
  map<Day, struct tm> dates{};
  Day min_date = numeric_limits<Day>::max();
  Day max_date = numeric_limits<Day>::min();
  time_t prev_time = numeric_limits<time_t>::min();
  ifstream in;
  in.open(filename);
  logging::check_fatal(!in.is_open(),
                       "Could not open input weather file %s",
                       filename.c_str());
  if (in.is_open())
  {
#ifndef NDEBUG
    const auto file_out = string(Settings::outputDirectory()) + "/wx_hourly_out_read.csv";
    FILE* out = fopen(file_out.c_str(), "w");
    logging::check_fatal(nullptr == out, "Cannot open file %s for output", file_out.c_str());
    fprintf(out, "Scenario,Date,PREC,TEMP,RH,WS,WD,FFMC,DMC,DC,ISI,BUI,FWI\n");
#endif
    string str;
    logging::info("Reading scenarios from '%s'", filename.c_str());
    // read header line
    getline(in, str);
    // get rid of whitespace
    str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
    str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
    str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
    constexpr auto expected_header =
      "Scenario,Date,PREC,TEMP,RH,WS,WD,FFMC,DMC,DC,ISI,BUI,FWI";
    logging::check_fatal(expected_header != str,
                         "Input CSV must have columns in this order:\n'%s'\n but got:\n'%s'",
                         expected_header,
                         str.c_str());
    auto prev = &yesterday;
    // HACK: adding to original object if we don't do this?
    auto apcp_24h = yesterday.prec().asDouble();
    while (getline(in, str))
    {
      istringstream iss(str);
      if (getline(iss, str, ',') && !str.empty())
      {
        // HACK: ignore date and just worry about relative order??
        // Scenario
        logging::verbose("Scenario is %s", str.c_str());
        size_t cur = 0;
        try
        {
          cur = static_cast<size_t>(stoi(str));
        }
        catch (const std::exception& ex)
        {
          // HACK: somehow stoi() is still getting empty strings
          logging::fatal(ex, "Error reading weather file %s: %s is not a valid integer", filename.c_str(), str.c_str());
        }
        if (wx.find(cur) == wx.end())
        {
          logging::debug("Loading scenario %d...", cur);
          wx.emplace(cur, new vector<const wx::FwiWeather*>());
          prev_time = std::numeric_limits<time_t>::min();
          logging::check_fatal(wx_daily.find(cur) != wx_daily.end(),
                               "Somehow have daily weather for scenario %ld before hourly weather",
                               cur);
          wx_daily.emplace(cur, map<Day, wx::FwiWeather>());
          prev = &yesterday;
          logging::extensive("Resetting new scenario precip to %f from %f",
                             yesterday.prec().asDouble(),
                             apcp_24h);
          apcp_24h = yesterday.prec().asDouble();
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
          if (!s->empty() && t.tm_yday < min_date)
          {
            logging::fatal(
              "Weather input file crosses year boundary or dates are not sequential");
          }
        }
        min_date = min(min_date, static_cast<Day>(t.tm_yday));
        max_date = max(max_date, static_cast<Day>(t.tm_yday));
        time_t cur_time = mktime(&t);
        if (prev_time != std::numeric_limits<time_t>::min())
        {
          logging::check_fatal((cur_time - prev_time) != (60 * 60),
                               "Expected sequential hours in weather input");
        }
        prev_time = cur_time;
        const auto for_time = (t.tm_yday - min_date) * DAY_HOURS + t.tm_hour;
        // HACK: can be up until rest of year since start date
        const size_t new_size = (max_date - min_date + 1) * DAY_HOURS;
        const auto old_size = s->size();
        if (old_size != new_size)
        {
          s->resize(new_size);
          for (auto i = old_size; i < new_size; ++i)
          {
            s->at(i) = nullptr;
          }
        }
        logging::verbose("for_time == %d", for_time);
        const wx::FwiWeather* w = new wx::FwiWeather(&iss,
                                                     &str);
        s->at(for_time) = w;
        logging::check_fatal(0 > w->prec().asDouble(),
                             "Hourly weather precip %f is negative",
                             w->prec().asDouble());
        apcp_24h += w->prec().asDouble();
        logging::extensive("Adding %f to precip results in accumulation of %f",
                           w->prec().asDouble(),
                           apcp_24h);
        if (12 == t.tm_hour)
        {
          // we just hit noon on a new day, so add the daily value
          auto& s_daily = wx_daily.at(cur);
          const auto day = static_cast<Day>(t.tm_yday);
          logging::check_fatal(s_daily.find(day) != s_daily.end(),
                               "Day already exists");
          const auto month = t.tm_mon + 1;
          s_daily.emplace(day,
                          wx::FwiWeather(*prev,
                                         month,
                                         latitude,
                                         w->temp(),
                                         w->rh(),
                                         w->wind(),
                                         wx::Precipitation(apcp_24h)));
          // new 24 hour period
          logging::extensive("Resetting daily precip to %f from %f", 0.0, apcp_24h);
          apcp_24h = 0;
          prev = &s_daily.at(static_cast<Day>(t.tm_yday));
        }
#ifndef NDEBUG
        const auto month = t.tm_mon + 1;
        logging::debug("%ld,%d-%02d-%02d %02d:00,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g",
                       cur,
                       year_,
                       month,
                       t.tm_mday,
                       t.tm_hour,
                       w->prec().asDouble(),
                       w->temp().asDouble(),
                       w->rh().asDouble(),
                       w->wind().speed().asDouble(),
                       w->wind().direction().asDouble(),
                       w->ffmc().asDouble(),
                       w->dmc().asDouble(),
                       w->dc().asDouble(),
                       w->isi().asDouble(),
                       w->bui().asDouble(),
                       w->fwi().asDouble());
        fprintf(out,
                "%ld,%d-%02d-%02d %02d:00,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g\n",
                cur,
                year_,
                month,
                t.tm_mday,
                t.tm_hour,
                w->prec().asDouble(),
                w->temp().asDouble(),
                w->rh().asDouble(),
                w->wind().speed().asDouble(),
                w->wind().direction().asDouble(),
                w->ffmc().asDouble(),
                w->dmc().asDouble(),
                w->dc().asDouble(),
                w->isi().asDouble(),
                w->bui().asDouble(),
                w->fwi().asDouble());
#endif
      }
    }
#ifndef NDEBUG
    logging::check_fatal(0 != fclose(out), "Could not close file %s", file_out.c_str());
#endif
    in.close();
  }
  //  for (auto& kv : wx)
  //  {
  //    kv.second.emplace(static_cast<Day>(min_date - 1), yesterday);
  //  }
  //  const auto file_out = string(Settings::outputDirectory()) + "/wx_out.csv";
  //  FILE* out = fopen(file_out.c_str(), "w");
  //  logging::check_fatal(nullptr == out, "Cannot open file %s for output", file_out.c_str());
  //  fprintf(out, "Scenario,Day,PREC,TEMP,RH,WS,WD,FFMC,DMC,DC,ISI,BUI,FWI\n");
  //  size_t i = 1;
  //  for (auto& kv : wx)
  //  {
  //    auto& s = kv.second;
  //    for (auto& kv2 : s)
  //    {
  //      auto& day = kv2.first;
  //      auto& w = kv2.second;
  //      fprintf(out,
  //              "%ld,%d,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g\n",
  //              i,
  //              day,
  //              w.prec().asDouble(),
  //              w.temp().asDouble(),
  //              w.rh().asDouble(),
  //              w.wind().speed().asDouble(),
  //              w.wind().direction().asDouble(),
  //              w.ffmc().asDouble(),
  //              w.dmc().asDouble(),
  //              w.dc().asDouble(),
  //              w.isi().asDouble(),
  //              w.bui().asDouble(),
  //              w.fwi().asDouble());
  //    }
  //    ++i;
  //  }
  //  logging::check_fatal(0 != fclose(out), "Could not close file %s", file_out.c_str());
  const auto fuel_lookup = sim::Settings::fuelLookup();
  const auto& f = fuel_lookup.usedFuels();
  // loop through and try to find duplicates
  for (const auto& kv : wx)
  {
    const auto k = kv.first;
    const auto s = kv.second;
    // FIX: this is just looking for duplicate scenario ids, not weather?
    if (wx_.find(k) == wx_.end())
    {
      const auto w = make_shared<wx::FireWeather>(f, min_date, max_date, s);
      wx_.emplace(k, w);
      // calculate daily indices
      auto& s_daily = wx_daily.at(k);
      // HACK: set yesterday to match today
      s_daily.emplace(min_date - 1, s_daily.at(min_date));
      const auto w_daily = make_shared<wx::FireWeatherDaily>(f, s_daily);
      wx_daily_.emplace(k, w_daily);
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
  // if (nullptr != perimeter_)
  // {
  //   initial_intensity_ = make_shared<IntensityMap>(*this, &(*perimeter_));
  // }
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
    const auto cur_wx = kv.second.get();
    const auto cur_daily = wx_daily_.at(id).get();
    if (nullptr != perimeter_)
    {
      setup_scenario(new Scenario(this,
                                  id,
                                  cur_wx,
                                  cur_daily,
                                  start,
                                  // initial_intensity_,
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
                                    cur_wx,
                                    cur_daily,
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
[[nodiscard]] std::chrono::seconds Model::runTime() const
{
  const auto run_time = last_checked_ - startTime();
  const auto run_time_seconds = std::chrono::duration_cast<std::chrono::seconds>(run_time);
  return run_time_seconds;
}
bool Model::shouldStop() const noexcept
{
  return isOutOfTime() || isOverSimulationCountLimit();
}
bool Model::isOutOfTime() const noexcept
{
  // return is_out_of_time_ || runTime() > timeLimit();
  // return runTime() > timeLimit();
  // return ((last_checked_ - startTime()) > timeLimit());
  // return (is_out_of_time_ || ((last_checked_ - startTime()) > timeLimit()));
  // return (Clock::now() - startTime()) > timeLimit();
  return is_out_of_time_;
}
bool Model::isOverSimulationCountLimit() const noexcept
{
  return is_over_simulation_count_;
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
bool Model::add_statistics(vector<double>* all_sizes,
                           vector<double>* means,
                           vector<double>* pct,
                           const util::SafeVector& sizes)
{
  const auto cur_sizes = sizes.getValues();
  logging::check_fatal(cur_sizes.empty(), "No sizes at end of simulation");
  const util::Statistics s{cur_sizes};
  static_cast<void>(util::insert_sorted(pct, s.percentile(95)));
  static_cast<void>(util::insert_sorted(means, s.mean()));
  // NOTE: Used to just look at mean and percentile of each iteration, but should probably look at all the sizes together?
  for (const auto size : cur_sizes)
  {
    static_cast<void>(util::insert_sorted(all_sizes, size));
  }
  is_over_simulation_count_ = all_sizes->size() >= Settings::maximumCountSimulations();
  if (isOverSimulationCountLimit())
  {
    logging::note(
      "Stopping after %d iterations. Simulation limit of %d simulations has been reached.",
      all_sizes->size(),
      Settings::maximumCountSimulations());
    return false;
  }
  if (isOutOfTime())
  {
    logging::note(
      "Stopping after %d iterations. Time limit of %d seconds has been reached.",
      pct->size(),
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
                     const vector<double>* all_sizes,
                     const vector<double>* means,
                     const vector<double>* pct,
                     const Model& model)
{
  if (Settings::deterministic())
  {
    logging::note("Stopping after %i iteration because running in deterministic mode");
    return 0;
  }
  if (model.isOverSimulationCountLimit())
  {
    logging::note(
      "Stopping after %d iterations. Simulation limit of %d simulations has been reached.",
      all_sizes->size(),
      Settings::maximumCountSimulations());
    return 0;
  }
  if (model.isOutOfTime())
  {
    logging::note(
      "Stopping after %d iterations. Time limit of %d seconds has been reached.",
      i,
      Settings::maximumTimeSeconds());
    return 0;
  }
  const auto for_sizes = util::Statistics{*all_sizes};
  const auto for_means = util::Statistics{*means};
  const auto for_pct = util::Statistics{*pct};
  if (!(!for_means.isConfident(Settings::confidenceLevel())
        || !for_pct.isConfident(Settings::confidenceLevel())
        || !for_sizes.isConfident(Settings::confidenceLevel())))
  {
    return 0;
  }
  // const auto left = max(
  //   max(max(for_means.runsRequired(i, Settings::confidenceLevel()),
  //           for_pct.runsRequired(i, Settings::confidenceLevel())),
  //       for_sizes.runsRequired();
  const auto runs_for_means = for_means.runsRequired(Settings::confidenceLevel());
  const auto runs_for_pct = for_pct.runsRequired(Settings::confidenceLevel());
  const auto runs_for_sizes = for_sizes.runsRequired(Settings::confidenceLevel());
  logging::debug("Runs required based on criteria: { means: %ld, pct: %ld, sizes: %ld}",
                 runs_for_means,
                 runs_for_pct,
                 runs_for_sizes);
  logging::debug("Number of values based on criteria: { means: %ld, pct: %ld, sizes: %ld}",
                 for_means.n(),
                 for_pct.n(),
                 for_sizes.n());
  const auto left = max(
    max(
      runs_for_means,
      runs_for_pct),
    runs_for_sizes);
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
  // HACK: seed_seq takes a list of integers now, so multiply and convert to get more digits
  const auto lat = static_cast<size_t>(start_point.latitude() * pow(10, std::numeric_limits<size_t>::digits10 - 4));
  const auto lon = static_cast<size_t>(start_point.longitude() * pow(10, std::numeric_limits<size_t>::digits10 - 4));
  logging::debug("lat/long (%f, %f) converted to (%ld, %ld)", start_point.latitude(), start_point.longitude(), lat, lon);
  std::seed_seq seed_spread{static_cast<size_t>(0), static_cast<size_t>(start_day), lat, lon};
  std::seed_seq seed_extinction{static_cast<size_t>(1), static_cast<size_t>(start_day), lat, lon};
  mt19937 mt_spread(seed_spread);
  mt19937 mt_extinction(seed_extinction);
  vector<double> all_sizes{};
  vector<double> means{};
  vector<double> pct{};
  size_t runs_done = 0;
  vector<Iteration> all_iterations{};
  all_iterations.push_back(readScenarios(start_point,
                                         start,
                                         save_intensity,
                                         start_day,
                                         last_date));
  // HACK: reference from vector so timer can cancel everything in vector
  auto& iteration = all_iterations[0];
  // put probability maps into map
  const auto saves = iteration.savePoints();
  const auto started = iteration.startTime();
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
  logging::verbose("Setting up initial intensity map with perimeter");
  auto runs_left = 1;
  // // set up a timer to mark when simulation is out of time
  // auto t = Settings::maximumTimeSeconds();
  // // set up a timer to check the clock to see when simulation is out of time
  // logging::verbose("Starting timer for %ld seconds", t);
  // auto timer = std::thread([t, this] (){
  //   printf("Starting timer for %ld seconds", t);
  //   std::this_thread::sleep_for(std::chrono::seconds(t));
  //   printf("out of time after %ld seconds", t);
  //   is_out_of_time_ = true;
  // });
  // typedef std::chrono::duration<float> s;
  auto timer = std::thread([this, &runs_done, &runs_left, &all_iterations]() {
    constexpr auto CHECK_INTERVAL = std::chrono::seconds(1);
    // const auto SLEEP_INTERVAL = std::chrono::seconds(Settings::maximumTimeSeconds());
    do
    {
      this->last_checked_ = Clock::now();
      // think we need to check regularly instead of just sleeping so that we can see
      // if we've done enough runs and need to stop for that reason
      std::this_thread::sleep_for(CHECK_INTERVAL);
      // set bool so other things don't need to check clock
      is_out_of_time_ = runTime() >= timeLimit();
      // logging::debug("Checking clock");
    }
    while (runs_left > 0 && !shouldStop());
    if (isOutOfTime())
    {
      logging::warning("Ran out of time - cancelling simulations");
    }
    if (0 == runs_done)
    {
      logging::warning("Ran out of time, but haven't finished any iterations, so cancelling all but first");
    }
    size_t i = 0;
    for (auto& iter : all_iterations)
    {
      // don't cancel first iteration if no iterations are done
      if (0 != runs_done || 0 != i)
      {
        // if not over limit then just did all the runs so no warning
        iter.cancel(shouldStop());
      }
      ++i;
    }
    const auto run_time_seconds = runTime().count();
    // const auto run_time = last_checked_ - startTime();
    // const auto run_time_seconds = std::chrono::duration_cast<std::chrono::seconds>(run_time);
    // const auto time_left = Settings::maximumTimeSeconds() - run_time_seconds.count();
    const auto time_left = Settings::maximumTimeSeconds() - run_time_seconds;
    logging::debug("Ending timer after %ld seconds with %ld seconds left",
                   run_time_seconds,
                   time_left);
  });
  auto threads = list<std::thread>{};
  // const auto finalize_probabilities = [&threads, &timer, &probabilities](bool do_cancel) {
  const auto finalize_probabilities = [&threads, &timer, &probabilities]() {
    // assume timer is cancelling everything
    for (auto& t : threads)
    {
      if (t.joinable())
      {
        t.join();
      }
    }
    if (timer.joinable())
    {
      timer.join();
    }
    return probabilities;
  };
  // HACK: just do this here so that we know it happened
  //iterations.reset(&mt_extinction, &mt_spread);
  if (Settings::runAsync())
  {
    // FIX: I think we can just have 2 Iteration objects and roll through starting
    // threads in the second one as the first one finishes?
    // const auto MAX_THREADS = static_cast<size_t>(std::thread::hardware_concurrency() * PCT_CPU);
    // const auto MAX_THREADS = static_cast<size_t>(std::thread::hardware_concurrency() / 4);
    // const auto MAX_THREADS = std::thread::hardware_concurrency() - 1;
    const auto MAX_THREADS = std::thread::hardware_concurrency();
    // const auto MAX_CONCURRENT = std::max<size_t>(MAX_THREADS, 1);
    // const auto concurrent_iterations = std::max<size_t>(
    //   MAX_CONCURRENT / all_iterations[0].getScenarios().size(),
    //   1);
    // HACK: just set max of 4 for now
    // constexpr auto MIN_ITERATIONS_BEFORE_CHECK = 4;
    // const auto concurrent_iterations = std::min(
    //   static_cast<size_t>(MIN_ITERATIONS_BEFORE_CHECK),
    //   MAX_THREADS);
    // no point in running multiple iterations if deterministic
    const auto concurrent_iterations = Settings::deterministic()
                                       ? 1
                                       : std::max<size_t>(
                                         ceil(MAX_THREADS / iteration.size()),
                                         2);
    // const auto concurrent_iterations = MAX_THREADS;
    for (size_t x = 1; x < concurrent_iterations; ++x)
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
      // FIX: look at converting so that new threads get started as others complete
      // - would have to have multiple Iterations so we keep the data from them separate?
      size_t k = 0;
      while (k < iteration.size())
      {
        threads.front().join();
        threads.pop_front();
        ++k;
      }
      auto final_sizes = iteration.finalSizes();
      ++runs_done;
      for (auto& kv : all_probabilities[cur_iter])
      {
        probabilities[kv.first]->addProbabilities(*kv.second);
        // clear so we don't double count
        kv.second->reset();
      }
      if (!add_statistics(&all_sizes, &means, &pct, final_sizes))
      {
        // ran out of time but timer should cancel everything
        return finalize_probabilities();
      }
      // if (runs_done >= MIN_ITERATIONS_BEFORE_CHECK)
      {
        runs_left = runs_required(runs_done, &all_sizes, &means, &pct, *this);
        // runs_left = runs_required(runs_done, &means, &pct, *this);
        logging::note("Need another %d iterations", runs_left);
      }
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
        // no runs required, so stop
        return finalize_probabilities();
      }
    }
    // everything should be done when this section ends
  }
  else
  {
    logging::note("Running in synchronous mode");
    while (runs_left > 0)
    {
      logging::note("Running iteration %d", runs_done + 1);
      iteration.reset(&mt_extinction, &mt_spread);
      for (auto s : iteration.getScenarios())
      {
        s->run(&probabilities);
      }
      ++runs_done;
      if (!add_statistics(&all_sizes, &means, &pct, iteration.finalSizes()))
      {
        // ran out of time but timer should cance everything
        return finalize_probabilities();
      }
      runs_left = runs_required(runs_done, &all_sizes, &means, &pct, *this);
      // runs_left = runs_required(runs_done, &means, &pct, *this);
      logging::note("Need another %d iterations", runs_left);
    }
  }
  return finalize_probabilities();
}
int Model::runScenarios(const char* const weather_input,
                        const wx::FwiWeather& yesterday,
                        const char* const raster_root,
                        const topo::StartPoint& start_point,
                        const tm& start_time,
                        const bool save_intensity,
                        const string& perimeter,
                        const size_t size)
{
  tbd::logging::note("Simulation start time at start of runScenarios() is %d-%02d-%02d %02d:%02d",
                     start_time.tm_year + 1900,
                     start_time.tm_mon + 1,
                     start_time.tm_mday,
                     start_time.tm_hour,
                     start_time.tm_min);
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
  model.readWeather(yesterday, start_point.latitude(), weather_input);
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
#ifndef NDEBUG
  model.outputWeather();
#endif
  model.makeStarts(*position, start_point, perimeter, size);
  auto start_hour = ((start_time.tm_hour + (static_cast<double>(start_time.tm_min) / 60))
                     / DAY_HOURS);
  logging::note("Simulation start time is %d-%02d-%02d %02d:%02d",
                start_time.tm_year + 1900,
                start_time.tm_mon + 1,
                start_time.tm_mday,
                start_time.tm_hour,
                start_time.tm_min);
  const auto start = start_time.tm_yday + start_hour;
  logging::note("Simulation start time of %f is %s",
                start,
                make_timestamp(model.year(), start).c_str());
  const auto start_day = static_cast<Day>(start);
  // want to check that start time is in the range of the weather data we have
  logging::check_fatal(start < w->minDate(), "Start time is before weather streams start");
  logging::check_fatal(start > w->maxDate(), "Start time is after weather streams end");
  auto probabilities =
    model.runIterations(start_point, start, start_day, save_intensity);
  logging::note("Ran %d simulations", Scenario::completed());
  const auto run_time_seconds = model.runTime();
  const auto time_left = Settings::maximumTimeSeconds() - run_time_seconds.count();
  logging::debug("Finished successfully after %ld seconds with %ld seconds left",
                 run_time_seconds.count(),
                 time_left);
  logging::debug("Processed %ld spread events between all scenarios", Scenario::total_steps());
  show_probabilities(probabilities);
  auto final_time = numeric_limits<double>::min();
  for (const auto by_time : probabilities)
  {
    const auto time = by_time.first;
    final_time = max(final_time, time);
    const auto prob = by_time.second;
    logging::info("Setting perimeter");
    prob->setPerimeter(model.perimeter_.get());
    prob->saveAll(model, start_time, time, start_day);
  }
  // HACK: update last checked time to use in calculation
  model.last_checked_ = Clock::now();
  logging::note("Total simulation time was %ld seconds", model.runTime());
  for (const auto& kv : probabilities)
  {
    delete kv.second;
  }
  return 0;
}
#ifndef NDEBUG
void Model::outputWeather()
{
  outputWeather(wx_, "wx_hourly_out.csv");
  outputWeather(wx_daily_, "wx_daily_out.csv");
}
void Model::outputWeather(
  map<size_t, shared_ptr<wx::FireWeather>>& weather,
  const char* file_name)
{
  const auto file_out = string(Settings::outputDirectory()) + file_name;
  FILE* out = fopen(file_out.c_str(), "w");
  logging::check_fatal(nullptr == out, "Cannot open file %s for output", file_out.c_str());
  fprintf(out, "Scenario,Date,PREC,TEMP,RH,WS,WD,FFMC,DMC,DC,ISI,BUI,FWI\n");
  size_t i = 1;
  for (auto& kv : weather)
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
                w->prec().asDouble(),
                w->temp().asDouble(),
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
#endif
}
