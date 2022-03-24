/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2021-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "Model.h"
#include <chrono>
#include "FBP45.h"
#include "FireWeather.h"
#include "Input.h"
#include "Log.h"
#include "Observer.h"
#include "Perimeter.h"
#include "ProbabilityMap.h"
#include "Scenario.h"
#include "Util.h"
#include "UTM.h"
namespace fs
{
// HACK: assume using half the CPUs probably means that faster cores are being used?
constexpr double PCT_CPU = 0.5;
Semaphore Model::task_limiter{static_cast<int>(std::thread::hardware_concurrency())};
// FIX: just do one iteration at a time for now
// const auto MAX_THREADS = std::thread::hardware_concurrency();
// // no point in running multiple iterations if deterministic
// const auto concurrent_iterations = Settings::deterministic()
//                                    ? 1
//                                    : std::max<size_t>(
//                                        ceil(MAX_THREADS / scenarios_per_iteration),
//                                        2
//                                      );
constexpr auto MAX_CONCURRENT = 1;
Model::Model(
  const tm& start_time,
  const string_view output_directory,
  const StartPoint& start_point,
  Environment* env
)
  : start_time_(start_time), running_since_(Clock::now()),
    time_limit_(std::chrono::seconds(Settings::maximumTimeSeconds())), env_(env),
    output_directory_(output_directory)
{
  logging::debug("Calculating for (%f, %f)", start_point.latitude(), start_point.longitude());
  const auto nd_for_point = calculate_nd_ref_for_point(env->elevation(), start_point);
  for (auto day = 0; day < MAX_DAYS; ++day)
  {
    nd_.at(static_cast<size_t>(day)) = static_cast<int>(day - nd_for_point);
    logging::verbose(
      "Day %d has nd %d, is%s green, %d%% curing",
      day,
      nd_.at(static_cast<size_t>(day)),
      calculate_is_green(nd_.at(static_cast<size_t>(day))) ? "" : " not",
      calculate_grass_curing(nd_.at(static_cast<size_t>(day)))
    );
  }
}
void Model::readWeather(string filename, const FwiWeather& yesterday, const double latitude)
{
  map<size_t, map<Day, FwiWeather>> wx{};
  map<Day, struct tm> dates{};
  auto min_date = numeric_limits<Day>::max();
  ifstream in{filename};
  logging::check_fatal(!in.is_open(), "Could not open input weather file %s", filename.c_str());
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
    constexpr auto expected_header = "Scenario,Date,PREC,TEMP,RH,WS,WD";
    logging::check_fatal(
      expected_header != str,
      "Input CSV must have columns in this order:\n'%s'\n but got:\n'%s'",
      expected_header,
      str.c_str()
    );
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
          logging::fatal(
            ex,
            "Error reading weather file %s: %s is not a valid integer",
            filename.c_str(),
            str.c_str()
          );
        }
        if (wx.find(cur) == wx.end())
        {
          logging::debug("Loading scenario %d...", cur);
          wx.emplace(cur, map<Day, FwiWeather>());
        }
        auto& s = wx.at(cur);
        struct tm t{};
        read_date(&iss, &str, &t);
        year_ = t.tm_year + TM_YEAR_OFFSET;
        const auto ticks = mktime(&t);
        if (1 == cur)
        {
          logging::debug("Date '%s' is %ld and calculated jd is %d", str.c_str(), ticks, t.tm_yday);
          if (!s.empty() && t.tm_yday < min_date)
          {
            logging::fatal("Weather input file crosses year boundary or dates are not sequential");
          }
        }
        min_date = min(min_date, static_cast<Day>(t.tm_yday));
        logging::check_fatal(s.find(static_cast<Day>(t.tm_yday)) != s.end(), "Day already exists");
        s.emplace(static_cast<Day>(t.tm_yday), read_weather(&iss, &str));
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
  const auto file_out = output_directory_ + "/wx_out.csv";
  FILE* out = fopen(file_out.c_str(), "w");
  logging::check_fatal(nullptr == out, "Cannot open file %s for output", file_out.c_str());
  fprintf(out, "Scenario,Day,PREC,TEMP,RH,WS,WD,FFMC,DMC,DC,ISI,BUI,FWI\n");
  size_t i = 1;
  for (auto& kv : wx)
  {
    auto& s = kv.second;
    logging::check_fatal(s.find(static_cast<Day>(min_date - 1)) != s.end(), "Day already exists");
    // recalculate everything with startup indices
    map<Day, FwiWeather> new_wx{};
    auto last_ffmc = yesterday.ffmc();
    auto last_dmc = yesterday.dmc();
    auto last_dc = yesterday.dc();
    auto prec = yesterday.prec();
    for (auto& kv2 : s)
    {
      auto& day = kv2.first;
      auto& w = kv2.second;
      const fs::Ffmc ffmc(w.temp(), w.rh(), w.wind().speed(), prec, last_ffmc);
      const auto t = dates[day];
      const auto month = t.tm_mon + TM_MONTH_OFFSET;
      const fs::Dmc dmc(w.temp(), w.rh(), prec, last_dmc, month, latitude);
      const fs::Dc dc(w.temp(), prec, last_dc, month, latitude);
      const fs::Isi isi(w.wind().speed(), ffmc);
      const fs::Bui bui(dmc, dc);
      const fs::Fwi fwi(isi, bui);
      const fs::FwiWeather n(w.temp(), w.rh(), w.wind(), prec, ffmc, dmc, dc, isi, bui, fwi);
      new_wx.emplace(static_cast<Day>(day), n);
    }
    new_wx.emplace(static_cast<Day>(min_date - 1), yesterday);
    kv.second = new_wx;
  }
  const auto& fuel_lookup = Settings::fuelLookup();
  // loop through and try to find duplicates
  for (const auto& kv : wx)
  {
    const auto k = kv.first;
    const auto s = kv.second;
    if (wx_.find(k) == wx_.end())
    {
      const auto w = make_shared<FireWeather>(fuel_lookup.usedFuels(), s);
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
          const auto loc = env_->cell(Location(location.row() + y, location.column() + x));
          if (!is_null_fuel(loc))
          {
            starts_.push_back(make_shared<Cell>(cell(loc)));
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
void Model::makeStarts(
  Coordinates coordinates,
  const Point& point,
  const string_view perim,
  const size_t size
)
{
  const Location location(std::get<0>(coordinates), std::get<1>(coordinates));
  if (!perim.empty())
  {
    logging::note("Initializing from perimeter %s", string(perim).c_str());
    perimeter_ = make_shared<Perimeter>(perim, point, *env_);
  }
  else if (size > 0)
  {
    logging::note("Initializing from size %d ha", size);
    perimeter_ = make_shared<Perimeter>(cell(location), size, *env_);
  }
  // figure out where the fire can exist
  if (nullptr != perimeter_ && !perimeter_->burned().empty())
  {
    logging::check_fatal(size != 0 && !perim.empty(), "Can't specify size and perimeter");
    // we have a perimeter to start from
    // HACK: make sure this isn't empty
    starts_.push_back(make_shared<Cell>(cell(location)));
    logging::note(
      "Fire starting with size %0.1f ha", perimeter_->burned().size() * env_->cellSize() / 100.0
    );
  }
  else
  {
    if (nullptr != perimeter_)
    {
      logging::check_fatal(
        !perimeter_->burned().empty(), "Not using perimeter so it should be empty"
      );
      logging::note("Using fire perimeter results in empty fire - changing to use point");
      perimeter_ = nullptr;
    }
    logging::note("Fire starting with size %0.1f ha", env_->cellSize() / 100.0);
    if (0 == size && is_null_fuel(cell(location)))
    {
      findStarts(location);
    }
    else
    {
      starts_.push_back(make_shared<Cell>(cell(location)));
    }
  }
  logging::note(
    "Creating %ld streams x %ld location%s = %ld scenarios",
    wx_.size(),
    starts_.size(),
    starts_.size() > 1 ? "s" : "",
    wx_.size() * starts_.size()
  );
}
Iteration Model::readScenarios(
  const StartPoint& start_point,
  const double start,
  const Day start_day,
  const Day last_date
)
{
  vector<Scenario*> result{};
  auto saves = Settings::outputDateOffsets();
  const auto setup_scenario = [&result, &saves](Scenario* scenario) {
    if (Settings::saveIntensity())
    {
      scenario->registerObserver(new IntensityObserver(*scenario));
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
      setup_scenario(
        new Scenario(this, id, cur_wx.get(), start, perimeter_, start_point, start_day, last_date)
      );
    }
    else
    {
      for (const auto& cur_start : starts_)
      {
        // should always have at least the day before the fire in the weather stream
        setup_scenario(
          new Scenario(this, id, cur_wx.get(), start, cur_start, start_point, start_day, last_date)
        );
      }
    }
  }
  return Iteration(result);
}
[[nodiscard]] std::chrono::seconds Model::runTime() const
{
  const auto run_time = last_checked_ - runningSince();
  const auto run_time_seconds = std::chrono::duration_cast<std::chrono::seconds>(run_time);
  return run_time_seconds;
}
bool Model::shouldStop() const noexcept { return isOutOfTime() || isOverSimulationCountLimit(); }
bool Model::isOutOfTime() const noexcept { return is_out_of_time_; }
bool Model::isOverSimulationCountLimit() const noexcept { return is_over_simulation_count_; }
ProbabilityMap* Model::makeProbabilityMap(
  const DurationSize time,
  const DurationSize start_time,
  const int min_value,
  const int low_max,
  const int med_max,
  const int max_value
) const
{
  return env_->makeProbabilityMap(time, start_time, min_value, low_max, med_max, max_value);
}
static void show_probabilities(const map<double, ProbabilityMap*>& probabilities)
{
  for (const auto& kv : probabilities)
  {
    kv.second->show();
  }
}
template <class T>
ostream& operator<<(ostream& os, const vector<T>& v)
{
  for (auto& m : v)
  {
    os << m << " ";
  }
  os << endl;
  return os;
}
map<double, ProbabilityMap*> make_prob_map(
  const Model& model,
  const vector<double>& saves,
  const double started,
  const int min_value,
  const int low_max,
  const int med_max,
  const int max_value
)
{
  map<double, ProbabilityMap*> result{};
  for (const auto& time : saves)
  {
    result.emplace(
      time, model.makeProbabilityMap(time, started, min_value, low_max, med_max, max_value)
    );
  }
  return result;
}
bool Model::add_statistics(
  vector<double>* all_sizes,
  vector<double>* means,
  vector<double>* pct,
  const SafeVector& sizes
)
{
  const auto cur_sizes = sizes.getValues();
  logging::check_fatal(cur_sizes.empty(), "No sizes at end of simulation");
  const Statistics s{cur_sizes};
  static_cast<void>(insert_sorted(pct, s.percentile(95)));
  static_cast<void>(insert_sorted(means, s.mean()));
  // NOTE: Used to just look at mean and percentile of each iteration, but should probably look at
  // all the sizes together?
  for (const auto size : cur_sizes)
  {
    static_cast<void>(insert_sorted(all_sizes, size));
  }
  is_over_simulation_count_ = all_sizes->size() >= Settings::maximumCountSimulations();
  if (isOverSimulationCountLimit())
  {
    logging::note(
      "Stopping after %d iterations. Simulation limit of %d simulations has been reached.",
      all_sizes->size(),
      Settings::maximumCountSimulations()
    );
    return false;
  }
  if (isOutOfTime())
  {
    logging::note(
      "Stopping after %d iterations. Time limit of %d seconds has been reached.",
      pct->size(),
      Settings::maximumTimeSeconds()
    );
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
size_t runs_required(
  const size_t i,
  const vector<double>* all_sizes,
  const vector<double>* means,
  const vector<double>* pct,
  const Model& model
)
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
      Settings::maximumCountSimulations()
    );
    return 0;
  }
  if (model.isOutOfTime())
  {
    logging::note(
      "Stopping after %d iterations. Time limit of %d seconds has been reached.",
      i,
      Settings::maximumTimeSeconds()
    );
    return 0;
  }
  const auto for_sizes = Statistics{*all_sizes};
  const auto for_means = Statistics{*means};
  const auto for_pct = Statistics{*pct};
  if (!(!for_means.isConfident(Settings::confidenceLevel())
        || !for_pct.isConfident(Settings::confidenceLevel())
        || !for_sizes.isConfident(Settings::confidenceLevel())))
  {
    return 0;
  }
  const auto runs_for_means = for_means.runsRequired(Settings::confidenceLevel());
  const auto runs_for_pct = for_pct.runsRequired(Settings::confidenceLevel());
  const auto runs_for_sizes = for_sizes.runsRequired(Settings::confidenceLevel());
  logging::debug(
    "Runs required based on criteria: { means: %ld, pct: %ld, sizes: %ld}",
    runs_for_means,
    runs_for_pct,
    runs_for_sizes
  );
  logging::debug(
    "Number of values based on criteria: { means: %ld, pct: %ld, sizes: %ld}",
    for_means.n(),
    for_pct.n(),
    for_sizes.n()
  );
  const auto left = max(max(runs_for_means, runs_for_pct), runs_for_sizes);
  return left;
}
double Model::saveProbabilities(map<double, ProbabilityMap*>& probabilities, const bool is_interim)
{
  auto final_time = numeric_limits<double>::min();
  for (const auto by_time : probabilities)
  {
    const auto time = by_time.first;
    final_time = max(final_time, time);
    const auto prob = by_time.second;
    logging::debug("Setting perimeter");
    prob->setPerimeter(this->perimeter_.get());
    std::ignore = prob->saveAll(outputDirectory(), this->start_time_, time, is_interim);
  }
  return final_time;
}
map<double, ProbabilityMap*> Model::runIterations(
  const StartPoint& start_point,
  const double start,
  const Day start_day
)
{
  auto last_date = start_day;
  for (const auto i : Settings::outputDateOffsets())
  {
    last_date = max(static_cast<Day>(start_day + i), last_date);
  }
  // use independent seeds so that if we remove one threshold it doesn't affect the other
  // HACK: seed_seq takes a list of integers now, so multiply and convert to get more digits
  const auto lat = static_cast<size_t>(
    start_point.latitude() * pow(10, std::numeric_limits<size_t>::digits10 - 4)
  );
  const auto lon = static_cast<size_t>(
    start_point.longitude() * pow(10, std::numeric_limits<size_t>::digits10 - 4)
  );
  logging::debug(
    "lat/long (%f, %f) converted to (%ld, %ld)",
    start_point.latitude(),
    start_point.longitude(),
    lat,
    lon
  );
  std::seed_seq seed_spread{static_cast<size_t>(0), static_cast<size_t>(start_day), lat, lon};
  std::seed_seq seed_extinction{static_cast<size_t>(1), static_cast<size_t>(start_day), lat, lon};
  mt19937 mt_spread(seed_spread);
  mt19937 mt_extinction(seed_extinction);
  vector<double> all_sizes{};
  vector<double> means{};
  vector<double> pct{};
  size_t iterations_done = 0;
  size_t scenarios_done = 0;
  size_t scenarios_required_done = 0;
  vector<Iteration> all_iterations{};
  all_iterations.push_back(readScenarios(start_point, start, start_day, last_date));
  // HACK: reference from vector so timer can cancel everything in vector
  auto& iteration = all_iterations[0];
  const auto scenarios_per_iteration = iteration.size();
  // put probability maps into map
  const auto saves = iteration.savePoints();
  const auto started = iteration.startTime();
  auto probabilities = make_prob_map(
    *this,
    saves,
    started,
    0,
    Settings::intensityMaxLow(),
    Settings::intensityMaxModerate(),
    numeric_limits<int>::max()
  );
  vector<map<double, ProbabilityMap*>> all_probabilities{};
  all_probabilities.push_back(make_prob_map(
    *this,
    saves,
    started,
    0,
    Settings::intensityMaxLow(),
    Settings::intensityMaxModerate(),
    numeric_limits<int>::max()
  ));
  logging::verbose("Setting up initial intensity map with perimeter");
  auto runs_left = 1;
  bool is_being_cancelled = false;
  // HACK: use initial value for type
  auto timer = std::thread([this,
                            &scenarios_per_iteration,
                            &scenarios_required_done,
                            &all_probabilities,
                            &iterations_done,
                            &runs_left,
                            &all_iterations,
                            &is_being_cancelled,
                            &start_day]() {
    constexpr auto CHECK_INTERVAL = std::chrono::seconds(1);
    do
    {
      this->last_checked_ = Clock::now();
      // think we need to check regularly instead of just sleeping so that we can see
      // if we've done enough runs and need to stop for that reason
      std::this_thread::sleep_for(CHECK_INTERVAL);
      // set bool so other things don't need to check clock
      is_out_of_time_ = runTime() >= timeLimit();
    }
    while (runs_left > 0 && !shouldStop());
    if (isOutOfTime())
    {
      logging::warning("Ran out of time - cancelling simulations");
    }
    if (0 == iterations_done)
    {
      logging::warning(
        "Ran out of time, but haven't finished any iterations, so cancelling all but first"
      );
    }
    size_t i = 0;
    for (auto& iter : all_iterations)
    {
      // don't cancel first iteration if no iterations are done
      if (0 != iterations_done || 0 != i)
      {
        // if not over limit then just did all the runs so no warning
        iter.cancel(shouldStop());
      }
      ++i;
    }
    if (0 == iterations_done)
    {
      is_being_cancelled = true;
      if (scenarios_required_done > 0)
      {
        logging::info(
          "Saving interim results for (%ld of %ld) scenarios in timer thread",
          scenarios_required_done,
          scenarios_per_iteration
        );
        saveProbabilities(all_probabilities[0], true);
      }
    }
    const auto run_time_seconds = runTime().count();
    const auto time_left = Settings::maximumTimeSeconds() - run_time_seconds;
    logging::debug(
      "Ending timer after %ld seconds with %ld seconds left", run_time_seconds, time_left
    );
  });
  auto threads = list<std::thread>{};
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
  if (Settings::runAsync())
  {
    // FIX: I think we can just have 2 Iteration objects and roll through starting
    // threads in the second one as the first one finishes?
    for (size_t x = 1; x < MAX_CONCURRENT; ++x)
    {
      all_iterations.push_back(readScenarios(start_point, start, start_day, last_date));
      all_probabilities.push_back(make_prob_map(
        *this,
        saves,
        started,
        0,
        Settings::intensityMaxLow(),
        Settings::intensityMaxModerate(),
        numeric_limits<int>::max()
      ));
    }
    auto run_scenario = [this,
                         &is_being_cancelled,
                         &scenarios_per_iteration,
                         &scenarios_required_done,
                         &scenarios_done,
                         &all_probabilities,
                         &start_day](Scenario* s, size_t i) {
      auto result = s->run(&all_probabilities[i]);
      ++scenarios_done;
      if (i == 0)
      {
        ++scenarios_required_done;
        if (is_being_cancelled)
        {
          // no point in saving interim if final is done
          if (scenarios_per_iteration != scenarios_required_done)
          {
            logging::info(
              "Saving interim results for (%ld of %ld) scenarios in timer thread",
              scenarios_required_done,
              scenarios_per_iteration
            );
            saveProbabilities(all_probabilities[0], true);
          }
        }
      }
      return result;
    };
    size_t cur_iter = 0;
    for (auto& iter : all_iterations)
    {
      iter.reset(&mt_extinction, &mt_spread);
      auto& scenarios = iter.getScenarios();
      for (auto s : scenarios)
      {
        threads.emplace_back(run_scenario, s, cur_iter);
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
      while (k < scenarios_per_iteration)
      {
        threads.front().join();
        threads.pop_front();
        ++k;
      }
      auto final_sizes = iteration.finalSizes();
      ++iterations_done;
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
      {
        runs_left = runs_required(iterations_done, &all_sizes, &means, &pct, *this);
        logging::note("Need another %d iterations", runs_left);
      }
      if (runs_left > 0)
      {
        iteration.reset(&mt_extinction, &mt_spread);
        auto& scenarios = iteration.getScenarios();
        for (auto s : scenarios)
        {
          threads.emplace_back(run_scenario, s, cur_iter);
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
      logging::note("Running iteration %d", iterations_done + 1);
      iteration.reset(&mt_extinction, &mt_spread);
      for (auto s : iteration.getScenarios())
      {
        s->run(&probabilities);
      }
      ++iterations_done;
      if (!add_statistics(&all_sizes, &means, &pct, iteration.finalSizes()))
      {
        // ran out of time but timer should cance everything
        return finalize_probabilities();
      }
      runs_left = runs_required(iterations_done, &all_sizes, &means, &pct, *this);
      logging::note("Need another %d iterations", runs_left);
    }
  }
  return finalize_probabilities();
}
int Model::runScenarios(
  const string_view output_directory,
  const char* const weather_input,
  const FwiWeather& yesterday,
  const char* const raster_root,
  const StartPoint& start_point,
  const tm& start_time,
  const string_view perimeter,
  const size_t size
)
{
  fs::logging::note(
    "Simulation start time at start of runScenarios() is %d-%02d-%02d %02d:%02d",
    start_time.tm_year + TM_YEAR_OFFSET,
    start_time.tm_mon + TM_MONTH_OFFSET,
    start_time.tm_mday,
    start_time.tm_hour,
    start_time.tm_min
  );
  auto env = Environment::loadEnvironment(
    raster_root, start_point, perimeter, start_time.tm_year + TM_YEAR_OFFSET
  );
  logging::debug("Environment loaded");
  const auto position = env.findCoordinates(start_point, true);
#ifndef NDEBUG
  logging::check_fatal(
    std::get<0>(*position) > MAX_ROWS || std::get<1>(*position) > MAX_COLUMNS,
    "Location loaded outside of grid at position (%d, %d)",
    std::get<0>(*position),
    std::get<1>(*position)
  );
#endif
  logging::info("Position is (%d, %d)", std::get<0>(*position), std::get<1>(*position));
  const Location location{std::get<0>(*position), std::get<1>(*position)};
  Model model(start_time, output_directory, start_point, &env);
  logging::note("Grid has size (%d, %d)", env.rows(), env.columns());
  logging::note("Fire start position is cell (%d, %d)", location.row(), location.column());
  model.readWeather(weather_input, yesterday, start_point.latitude());
  if (model.wx_.empty())
  {
    logging::fatal("No weather provided");
  }
  const auto& w = model.wx_.begin()->second;
  logging::debug("Have weather from day %d to %d", w->minDate(), w->maxDate());
  const auto numDays = (w->maxDate() - w->minDate() + 1);
  const auto needDays = Settings::maxDateOffset();
  if (numDays < needDays)
  {
    logging::fatal(
      "Not enough weather to proceed - have %d days but looking for %d", numDays, needDays
    );
  }
  // want to output internal representation of weather to file
#ifndef NDEBUG
  model.outputWeather();
#endif
  model.makeStarts(*position, start_point, perimeter, size);
  auto start_hour =
    ((start_time.tm_hour + (static_cast<double>(start_time.tm_min) / 60)) / DAY_HOURS);
  logging::note(
    "Simulation start time is %d-%02d-%02d %02d:%02d",
    start_time.tm_year + TM_YEAR_OFFSET,
    start_time.tm_mon + TM_MONTH_OFFSET,
    start_time.tm_mday,
    start_time.tm_hour,
    start_time.tm_min
  );
  const auto start = start_time.tm_yday + start_hour;
  logging::note(
    "Simulation start time of %f is %s", start, make_timestamp(model.year(), start).c_str()
  );
  const auto start_day = static_cast<Day>(start);
  // want to check that start time is in the range of the weather data we have
  logging::check_fatal(start < w->minDate(), "Start time is before weather streams start");
  logging::check_fatal(start > w->maxDate(), "Start time is after weather streams end");
  auto probabilities = model.runIterations(start_point, start, start_day);
  logging::note("Ran %d simulations", Scenario::completed());
  const auto run_time_seconds = model.runTime();
  const auto time_left = Settings::maximumTimeSeconds() - run_time_seconds.count();
  logging::debug(
    "Finished successfully after %ld seconds with %ld seconds left",
    run_time_seconds.count(),
    time_left
  );
  logging::debug("Processed %ld spread events between all scenarios", Scenario::total_steps());
  show_probabilities(probabilities);
  // auto final_time =
  model.saveProbabilities(probabilities, false);
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
  const auto file_out = output_directory_ + "/wx_hourly_out.csv";
  FILE* out = fopen(file_out.c_str(), "w");
  logging::check_fatal(nullptr == out, "Cannot open file %s for output", file_out.c_str());
  fprintf(out, "Scenario,Date,PREC,TEMP,RH,WS,WD,FFMC,DMC,DC,ISI,BUI,FWI\n");
  size_t i = 1;
  for (auto& kv : wx_)
  {
    auto& s = kv.second;
    // do we need to index this by hour and day?
    // was assuming it started at 0 for first hour and day
    auto wx = s->getWeather();
    size_t min_hour = s->minDate() * DAY_HOURS;
    size_t wx_size = wx.size();
    size_t hour = min_hour;
    for (size_t j = 0; j < wx_size; ++j)
    {
      size_t day = hour / 24;
      auto w = wx.at(hour - min_hour);
      size_t month;
      size_t day_of_month;
      month_and_day(year_, day, &month, &day_of_month);
      if (nullptr != w)
      {
        fprintf(
          out,
          "%ld,%d-%02ld-%02ld %02ld:00,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g\n",
          i,
          year_,
          month,
          day_of_month,
          hour - day * DAY_HOURS,
          w->prec().asValue(),
          w->temp().asValue(),
          w->rh().asValue(),
          w->wind().speed().asValue(),
          w->wind().direction().asValue(),
          w->ffmc().asValue(),
          w->dmc().asValue(),
          w->dc().asValue(),
          w->isi().asValue(),
          w->bui().asValue(),
          w->fwi().asValue()
        );
      }
      else
      {
        fprintf(
          out,
          "%ld,%d-%02ld-%02ld %02ld:00,,,,,,,,,,,\n",
          i,
          year_,
          month,
          day_of_month,
          hour - day * DAY_HOURS
        );
      }
      ++hour;
    }
    ++i;
  }
  logging::check_fatal(0 != fclose(out), "Could not close file %s", file_out.c_str());
}
#endif
}
