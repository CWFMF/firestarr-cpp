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
namespace fs
{
#ifdef DEBUG_WEATHER
constexpr auto FMT_OUT =
  "%ld,%d-%02d-%02d %02d:%02d:%02d,%1.6f,%1.6f,%1.6f,%1.6f,%1.6f,%1.6f,%1.6f,%1.6f,%1.6f,%1.6f,%1.6f%s";
#endif
// HACK: assume using half the CPUs probably means that faster cores are being used?
constexpr MathSize PCT_CPU = 0.5;
Semaphore Model::task_limiter{static_cast<int>(std::thread::hardware_concurrency())};
Model::Model(
  const tm& start_time,
  const string_view output_directory,
  const StartPoint& start_point,
  Environment* env
)
  : output_directory_(output_directory), start_time_(start_time), running_since_(Clock::now()),
    time_limit_(Settings::maximumTimeSeconds()), env_(env), latitude_(start_point.latitude()),
    longitude_(start_point.longitude())
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
void Model::setWeather(const FwiWeather& weather, const Day start_day)
{
  yesterday_ = weather;
  const auto fuel_lookup = Settings::fuelLookup();
  const auto& f = fuel_lookup.usedFuels();
  wx_.emplace(
    0,
    FireWeather{
      f,
      static_cast<Day>(start_day - 1),
      weather.dc(),
      weather.dmc(),
      weather.ffmc(),
      weather.wind()
    }
  );
  wx_daily_.emplace(
    0,
    FireWeather{
      f,
      static_cast<Day>(start_day - 1),
      weather.dc(),
      weather.dmc(),
      weather.ffmc(),
      weather.wind()
    }
  );
}
void Model::readWeather(
  const FwiWeather& yesterday,
  const MathSize latitude,
  const string& filename
)
{
  map<size_t, vector<FwiWeather>> wx{};
  map<size_t, map<Day, FwiWeather>> wx_daily{};
  map<Day, struct tm> dates{};
  Day min_date = numeric_limits<Day>::max();
  Day max_date = numeric_limits<Day>::min();
  time_t prev_time = numeric_limits<time_t>::min();
  ifstream in{filename};
  logging::check_fatal(!in.is_open(), "Could not open input weather file %s", filename.c_str());
  if (in.is_open())
  {
#ifndef NDEBUG
    const auto file_out = string(output_directory_) + "/wx_hourly_out_read.csv";
    FILE* out = fopen(file_out.c_str(), "w");
    logging::check_fatal(nullptr == out, "Cannot open file %s for output", file_out.c_str());
    fprintf(out, "Scenario,Date,PREC,TEMP,RH,WS,WD,FFMC,DMC,DC,ISI,BUI,FWI\r\n");
#endif
    string str;
    logging::info("Reading scenarios from '%s'", filename.c_str());
    // read header line
    getline(in, str);
    // get rid of whitespace
    str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
    str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
    str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
    constexpr auto expected_header = "Scenario,Date,PREC,TEMP,RH,WS,WD,FFMC,DMC,DC,ISI,BUI,FWI";
    logging::check_fatal(
      expected_header != str,
      "Input CSV must have columns in this order:\n'%s'\n but got:\n'%s'",
      expected_header,
      str.c_str()
    );
    auto prev = &yesterday;
    // HACK: adding to original object if we don't do this?
    auto apcp_24h = yesterday.prec().asValue();
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
          wx.emplace(cur, vector<FwiWeather>{});
          prev_time = std::numeric_limits<time_t>::min();
          logging::check_fatal(
            wx_daily.find(cur) != wx_daily.end(),
            "Somehow have daily weather for scenario %ld before hourly weather",
            cur
          );
          wx_daily.emplace(cur, map<Day, FwiWeather>());
          prev = &yesterday;
          logging::extensive(
            "Resetting new scenario precip to %f from %f", yesterday.prec().asValue(), apcp_24h
          );
          apcp_24h = yesterday.prec().asValue();
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
        max_date = max(max_date, static_cast<Day>(t.tm_yday));
        time_t cur_time = mktime(&t);
        if (prev_time != std::numeric_limits<time_t>::min())
        {
          auto seconds_diff = (cur_time - prev_time);
          logging::check_fatal(
            seconds_diff != HOUR_SECONDS,
            "Expected sequential hours in weather input but rows are %f hours away from each other",
            seconds_diff / static_cast<MathSize>(HOUR_SECONDS)
          );
        }
        prev_time = cur_time;
        const auto for_time = (t.tm_yday - min_date) * DAY_HOURS + t.tm_hour;
        // HACK: can be up until rest of year since start date
        const size_t new_size = (max_date - min_date + 1) * DAY_HOURS;
        const auto old_size = s.size();
        if (old_size != new_size)
        {
          s.resize(new_size);
        }
        logging::verbose("for_time == %d", for_time);
        FwiWeather w{read_fwi_weather(&iss, &str)};
        s.at(for_time) = w;
        logging::check_fatal(
          0 > w.prec().asValue(), "Hourly weather precip %f is negative", w.prec().asValue()
        );
        apcp_24h += w.prec().asValue();
        logging::extensive(
          "Adding %f to precip results in accumulation of %f", w.prec().asValue(), apcp_24h
        );
        if (12 == t.tm_hour)
        {
          // we just hit noon on a new day, so add the daily value
          auto& s_daily = wx_daily.at(cur);
          const auto day = static_cast<Day>(t.tm_yday);
          logging::check_fatal(s_daily.find(day) != s_daily.end(), "Day already exists");
          const auto month = t.tm_mon + 1;
          s_daily.emplace(
            day,
            FwiWeather(*prev, month, latitude, w.temp(), w.rh(), w.wind(), Precipitation(apcp_24h))
          );
          // new 24 hour period
          logging::extensive("Resetting daily precip to %f from %f", 0.0, apcp_24h);
          apcp_24h = 0;
          prev = &s_daily.at(static_cast<Day>(t.tm_yday));
        }
#ifdef DEBUG_WEATHER
        const auto month = t.tm_mon + 1;
        logging::debug(
          FMT_OUT,
          cur,
          year_,
          month,
          t.tm_mday,
          t.tm_hour,
          t.tm_min,
          t.tm_sec,
          w.prec().asValue(),
          w.temp().asValue(),
          w.rh().asValue(),
          w.wind().speed().asValue(),
          w.wind().direction().asValue(),
          w.ffmc().asValue(),
          w.dmc().asValue(),
          w.dc().asValue(),
          w.isi().asValue(),
          w.bui().asValue(),
          w.fwi().asValue() ""
        );
        fprintf(
          out,
          FMT_OUT,
          cur,
          year_,
          month,
          t.tm_mday,
          t.tm_hour,
          t.tm_min,
          t.tm_sec,
          w.prec().asValue(),
          w.temp().asValue(),
          w.rh().asValue(),
          w.wind().speed().asValue(),
          w.wind().direction().asValue(),
          w.ffmc().asValue(),
          w.dmc().asValue(),
          w.dc().asValue(),
          w.isi().asValue(),
          w.bui().asValue(),
          w.fwi().asValue() "\r\n"
        );
#endif
      }
    }
#ifndef NDEBUG
    logging::check_fatal(0 != fclose(out), "Could not close file %s", file_out.c_str());
#endif
    in.close();
  }
  const auto fuel_lookup = Settings::fuelLookup();
  const auto& f = fuel_lookup.usedFuels();
  // loop through and try to find duplicates
  for (const auto& kv : wx)
  {
    const auto k = kv.first;
    const auto& s = kv.second;
    // FIX: this is just looking for duplicate scenario ids, not weather?
    if (wx_.find(k) == wx_.end())
    {
      wx_.emplace(k, FireWeather{f, min_date, max_date, s});
      // calculate daily indices
      auto& s_daily = wx_daily.at(k);
      // HACK: set yesterday to match today
      s_daily.emplace(min_date - 1, s_daily.at(min_date));
      wx_daily_.emplace(k, FireWeather{f, s_daily});
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
  logging::info("Using %d start locations:", ignitionScenarios());
  for (const auto& s : starts_)
  {
    logging::info("\t%d, %d", s->row(), s->column());
  }
}
void Model::findAllStarts()
{
  logging::note("Running scenarios for every possible start location");
  for (Idx x = 0; x < env_->columns(); ++x)
  {
    for (Idx y = 0; y < env_->rows(); ++y)
    {
      const auto loc = env_->cell(Location(y, x));
      if (!is_null_fuel(loc))
      {
        starts_.push_back(make_shared<Cell>(cell(loc)));
      }
    }
  }
  logging::info("Using %d start locations:", ignitionScenarios());
}
void Model::makeStarts(
  Coordinates coordinates,
  const Point& point,
  const string_view perim,
  size_t size
)
{
  Location location(std::get<0>(coordinates), std::get<1>(coordinates));
  if (!perim.empty())
  {
    logging::note("Initializing from perimeter %s", string(perim).c_str());
    perimeter_ = make_shared<Perimeter>(perim, point, *env_);
    // HACK: if perimeter is only one cell then use position not perimeter so it can bounce if
    // non-fuel
    const auto burned = perimeter_->burned();
    const auto s = burned.size();
    if (1 >= s)
    {
      logging::note("Converting perimeter into point since size is %ld", s);
      // use whatever the one cell is instead of the lat/long
      if (1 == s)
      {
        location = *(burned.begin());
      }
      // HACK: use 0 for 0 or 1 so it'll assign by point
      size = 0;
      perimeter_ = nullptr;
    }
  }
  // use if instead of else if in case perimeter was a single point and got switched
  if (size > 0)
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
    if (Settings::surface())
    {
      findAllStarts();
    }
    else
    {
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
  }
  logging::note(
    "Creating %ld streams x %ld location%s = %ld scenarios",
    wx_.size(),
    ignitionScenarios(),
    ignitionScenarios() > 1 ? "s" : "",
    wx_.size() * ignitionScenarios()
  );
}
Iteration Model::readScenarios(
  const StartPoint& start_point,
  const DurationSize start,
  const Day start_day,
  const Day last_date
)
{
  // FIX: this is going to do a lot of work to set up each scenario if we're making a surface
  vector<Scenario*> result{};
  auto saves = Settings::outputDateOffsets();
  auto save_individual = Settings::saveIndividual();
  const auto setup_scenario = [&result, save_individual, &saves](Scenario* scenario) {
    if (save_individual)
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
  if (Settings::surface())
  {
    setup_scenario(new Scenario(
      this, 0, &wx_.at(0), &wx_daily_.at(0), start, starts_.at(0), start_point, start_day, last_date
    ));
  }
  else
  {
    for (const auto& kv : wx_)
    {
      const auto id = kv.first;
      const auto* cur_wx = &kv.second;
      const auto* cur_daily = &wx_daily_.at(id);
      if (nullptr != perimeter_)
      {
        setup_scenario(new Scenario(
          this, id, cur_wx, cur_daily, start, perimeter_, start_point, start_day, last_date
        ));
      }
      else
      {
        for (const auto& cur_start : starts_)
        {
          // should always have at least the day before the fire in the weather stream
          setup_scenario(new Scenario(
            this, id, cur_wx, cur_daily, start, cur_start, start_point, start_day, last_date
          ));
        }
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
bool Model::shouldStop() const noexcept
{
  return !Settings::surface() && (isOutOfTime() || isOverSimulationCountLimit());
}
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
static void show_probabilities(const map<ThresholdSize, ProbabilityMap*>& probabilities)
{
  for (const auto& kv : probabilities)
  {
    kv.second->show();
  }
}
map<DurationSize, ProbabilityMap*> make_prob_map(
  const Model& model,
  const vector<DurationSize>& saves,
  const DurationSize started,
  const int min_value,
  const int low_max,
  const int med_max,
  const int max_value
)
{
  map<DurationSize, ProbabilityMap*> result{};
  for (const auto& time : saves)
  {
    result.emplace(
      time, model.makeProbabilityMap(time, started, min_value, low_max, med_max, max_value)
    );
  }
  return result;
}
map<DurationSize, SafeVector*> make_size_map(const vector<DurationSize>& saves)
{
  map<DurationSize, SafeVector*> result{};
  for (const auto& time : saves)
  {
    result.emplace(time, new SafeVector());
  }
  return result;
}
bool Model::add_statistics(
  vector<MathSize>* all_sizes,
  vector<MathSize>* means,
  vector<MathSize>* pct,
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
  for (const auto& size : cur_sizes)
  {
    static_cast<void>(insert_sorted(all_sizes, size));
  }
  if (Settings::surface())
  {
    return true;
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
  const vector<MathSize>* all_sizes,
  const vector<MathSize>* means,
  const vector<MathSize>* pct,
  const Model& model
)
{
  if (Settings::deterministic())
  {
    logging::note("Stopping after iteration %ld because running in deterministic mode", i);
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
DurationSize Model::saveProbabilities(
  map<DurationSize, ProbabilityMap*>& probabilities,
  const Day start_day,
  const bool is_interim
)
{
  auto final_time = numeric_limits<DurationSize>::min();
  for (const auto& by_time : probabilities)
  {
    const auto time = by_time.first;
    final_time = max(final_time, time);
    const auto prob = by_time.second;
    logging::debug("Setting perimeter");
    prob->setPerimeter(this->perimeter_.get());
    std::ignore = prob->saveAll(outputDirectory(), this->start_time_, time, is_interim);
    const auto day = static_cast<int>(round(time));
    const auto n = nd(day);
    logging::note(
      "Fuels for day %d are %s green-up and grass has %d%% curing",
      day - static_cast<int>(start_day),
      calculate_is_green(n) ? "after" : "before",
      calculate_grass_curing(n)
    );
  }
  return final_time;
}
map<DurationSize, ProbabilityMap*> Model::runIterations(
  const StartPoint& start_point,
  const DurationSize start,
  const Day start_day
)
{
  auto last_date = start_day;
  for (const auto& i : Settings::outputDateOffsets())
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
  vector<MathSize> all_sizes{};
  vector<MathSize> means{};
  vector<MathSize> pct{};
  size_t iterations_done = 0;
  size_t scenarios_done = 0;
  size_t scenarios_required_done = 0;
  vector<Iteration> all_iterations{};
  logging::verbose("Reading scenarios");
  all_iterations.push_back(readScenarios(start_point, start, start_day, last_date));
  // HACK: reference from vector so timer can cancel everything in vector
  auto& iteration = all_iterations[0];
  const auto scenarios_per_iteration = iteration.size();
  // put probability maps into map
  logging::verbose("Setting save points");
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
  vector<map<DurationSize, ProbabilityMap*>> all_probabilities{};
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
                            &scenarios_done,
                            &all_probabilities,
                            &iterations_done,
                            &runs_left,
                            &all_sizes,
                            &all_iterations,
                            &is_being_cancelled,
                            &probabilities,
                            &start_day]() {
    constexpr auto CHECK_INTERVAL = std::chrono::seconds(1);
    do
    {
      this->last_checked_ = Clock::now();
      // think we need to check regularly instead of just sleeping so that we can see
      // if we've done enough runs and need to stop for that reason
      std::this_thread::sleep_for(CHECK_INTERVAL);
      // set bool so other things don't need to check clock
      is_out_of_time_ = runTime().count() >= timeLimit().count();
      logging::verbose("Checking clock [%ld of %ld]", runTime(), timeLimit());
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
        saveProbabilities(all_probabilities[0], start_day, true);
      }
    }
    const auto run_time_seconds = runTime().count();
    const auto time_left = Settings::maximumTimeSeconds() - run_time_seconds;
    logging::debug(
      "Ending timer after %ld seconds with %ld seconds left", run_time_seconds, time_left
    );
  });
  auto threads = list<std::thread>{};
  const auto finalize_probabilities =
    [this, &start_day, &all_sizes, &is_being_cancelled, &threads, &timer, &probabilities]() {
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
  // if using surface just run each start through in a loop here
  size_t cur_start = 0;
  auto reset_iter = [&cur_start, this, &mt_extinction, &mt_spread](Iteration& iter) {
    if (Settings::surface())
    {
      if (cur_start >= starts_.size())
      {
        return false;
      }
      auto start_cell = starts_[cur_start];
      iter.reset_with_new_start(start_cell);
      ++cur_start;
    }
    else
    {
      iter.reset(&mt_extinction, &mt_spread);
    }
    return true;
  };
  if (Settings::runAsync())
  {
    const auto HARDWARE_THREADS = static_cast<size_t>(std::thread::hardware_concurrency());
    // maybe a bit slower but prefer to run all scenarios at the same time
    const auto MAX_THREADS = max(HARDWARE_THREADS, scenarios_per_iteration);
    if (MAX_THREADS > HARDWARE_THREADS)
    {
      logging::note(
        "Increasing to use at least one thread for each of %ld scenarios", scenarios_per_iteration
      );
      Model::task_limiter.set_limit(MAX_THREADS);
    }
    // no point in running multiple iterations if deterministic
    const auto concurrent_iterations = 1;
    for (size_t x = 1; x < concurrent_iterations; ++x)
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
                         &all_iterations,
                         &start_day](Scenario* s, size_t i, bool is_required) {
      auto result = s->run(&all_probabilities[i]);
      ++scenarios_done;
      logging::extensive(
        "Done %ld scenarios in iteration %ld which %s required",
        scenarios_done,
        i,
        (is_required ? "is" : "is not")
      );
      if (is_required)
      {
        logging::verbose(
          "Done %ld scenarios in iteration %ld which %s required",
          scenarios_done,
          i,
          (is_required ? "is" : "is not")
        );
        ++scenarios_required_done;
        logging::debug(
          "Have (%ld of %ld) scenarios and %s being cancelled",
          scenarios_required_done,
          scenarios_per_iteration,
          (is_being_cancelled ? "is" : "not")
        );
        if (is_being_cancelled)
        {
          // no point in saving interim if final is done
          if (scenarios_per_iteration != scenarios_required_done)
          {
            logging::info(
              "Saving interim results for (%ld of %ld) scenarios",
              scenarios_required_done,
              scenarios_per_iteration
            );
            saveProbabilities(all_probabilities[0], start_day, true);
          }
        }
      }
      return result;
    };
    logging::debug("Created %d iterations to run concurrently", all_iterations.size());
    size_t cur_iter = 0;
    for (auto& iter : all_iterations)
    {
      if (reset_iter(iter))
      {
        auto& scenarios = iter.getScenarios();
        for (auto s : scenarios)
        {
          threads.emplace_back(run_scenario, s, cur_iter, 0 == cur_iter);
        }
        ++cur_iter;
      }
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
        if (Settings::surface())
        {
          runs_left = ignitionScenarios() - iterations_done;
        }
        else
        {
          runs_left = runs_required(iterations_done, &all_sizes, &means, &pct, *this);
          logging::note("Need another %d iterations", runs_left);
        }
      }
      if (runs_left > 0)
      {
        if (reset_iter(iteration))
        {
          auto& scenarios = iteration.getScenarios();
          for (auto s : scenarios)
          {
            threads.emplace_back(run_scenario, s, cur_iter, false);
          }
          ++cur_iter;
          // loop around to start if required
          cur_iter %= all_iterations.size();
        }
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
      if (reset_iter(iteration))
      {
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
        if (Settings::surface())
        {
          runs_left = ignitionScenarios() - iterations_done;
        }
        else
        {
          runs_left = runs_required(iterations_done, &all_sizes, &means, &pct, *this);
          logging::note("Need another %d iterations", runs_left);
        }
      }
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
  // don't flip for Environment because that already happened
  const auto position = env.findCoordinates(start_point, false);
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
  auto start_hour =
    ((start_time.tm_hour + (static_cast<DurationSize>(start_time.tm_min) / 60)) / DAY_HOURS);
  logging::note(
    "Simulation start time is %d-%02d-%02d %02d:%02d",
    start_time.tm_year + TM_YEAR_OFFSET,
    start_time.tm_mon + TM_MONTH_OFFSET,
    start_time.tm_mday,
    start_time.tm_hour,
    start_time.tm_min
  );
  const auto start = start_time.tm_yday + start_hour;
  const auto start_day = static_cast<Day>(start);
  if (Settings::surface())
  {
    // yesterday should have constants to use
    model.setWeather(yesterday, start_day);
    model.year_ = start_time.tm_year + 1900;
  }
  else
  {
    model.readWeather(yesterday, start_point.latitude(), weather_input);
    if (model.wx_.empty())
    {
      logging::fatal("No weather provided");
    }
    const auto& w = model.wx_.begin()->second;
    logging::debug("Have weather from day %d to %d", w.minDate(), w.maxDate());
    const auto numDays = (w.maxDate() - w.minDate() + 1);
    const auto needDays = Settings::maxDateOffset();
    if (numDays < needDays)
    {
      logging::fatal(
        "Not enough weather to proceed - have %d days but looking for %d", numDays, needDays
      );
    }
    // want to output internal representation of weather to file
#ifdef DEBUG_WEATHER
    if (!Settings::surface())
    {
      model.outputWeather();
    }
#endif
    // want to check that start time is in the range of the weather data we have
    logging::check_fatal(start < w.minDate(), "Start time is before weather streams start");
    logging::check_fatal(start > w.maxDate(), "Start time is after weather streams end");
  }
  logging::note(
    "Simulation start time of %f is %s", start, make_timestamp(model.year(), start).c_str()
  );
  model.makeStarts(*position, start_point, perimeter, size);
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
  model.saveProbabilities(probabilities, start_day, false);
  ProbabilityMap::deleteInterim();
  // HACK: update last checked time to use in calculation
  model.last_checked_ = Clock::now();
  logging::note("Total simulation time was %ld seconds", model.runTime());
  for (const auto& kv : probabilities)
  {
    delete kv.second;
  }
  return 0;
}
#ifdef DEBUG_WEATHER
void Model::outputWeather()
{
  outputWeather(wx_, "wx_hourly_out.csv");
  outputWeather(wx_daily_, "wx_daily_out.csv");
}
void Model::outputWeather(map<size_t, shared_ptr<FireWeather>>& weather, const char* file_name)
{
  const auto file_out = string(output_directory) + file_name;
  const auto file_out_fbp = string(output_directory) + string("fbp_") + file_name;
  FILE* out = fopen(file_out.c_str(), "w");
  FILE* out_fbp = fopen(file_out_fbp.c_str(), "w");
  logging::check_fatal(nullptr == out, "Cannot open file %s for output", file_out.c_str());
  constexpr auto HEADER_FWI = "Scenario,Date,PREC,TEMP,RH,WS,WD,FFMC,DMC,DC,ISI,BUI,FWI";
  constexpr auto HEADER_FBP_PRIMARY = "CFB,CFC,FD,HFI,RAZ,ROS,SFC,TFC";
  fprintf(out, "%s\r\n", HEADER_FWI);
  fprintf(out_fbp, "%s,%s\r\n", HEADER_FWI, HEADER_FBP_PRIMARY);
  size_t i = 0;
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
        fprintf(
          out,
          FMT_OUT,
          i,
          year_,
          static_cast<uint8_t>(month),
          static_cast<uint8_t>(day_of_month),
          static_cast<uint8_t>(hour - day * DAY_HOURS),
          0,
          0,
          8prec().asValue(),
          w->temp().asValue(),
          w->rh().asValue(),
          w->wind().speed().asValue(),
          w->wind().direction().asValue(),
          w->ffmc().asValue(),
          w->dmc().asValue(),
          w->dc().asValue(),
          w->isi().asValue(),
          w->bui().asValue(),
          w->fwi().asValue(),
          "\r\n"
        );
        SlopeSize SLOPE_MAX = MAX_SLOPE_FOR_DISTANCE;
        SlopeSize SLOPE_INCREMENT = 200;
        AspectSize ASPECT_MAX = 360;
        AspectSize ASPECT_INCREMENT = 450;
        const auto lookup = Settings::fuelLookup();
        const auto fuel = lookup.byName("C-2");
        for (SlopeSize slope = 0; slope < SLOPE_MAX; slope += SLOPE_INCREMENT)
        {
          for (AspectSize aspect = 0; aspect < ASPECT_MAX; aspect += ASPECT_INCREMENT)
          {
            // for (auto fuel_name : FUELS)
            {
              const auto fuel_name = fuel->name();
              // calculate and output fbp
              // const auto spread = SpreadInfo(year_,
              const SpreadInfo spread(
                year_,
                month,
                day_of_month,
                hour,
                0,
                latitude_,
                longitude_,
                env_->elevation(),
                slope,
                aspect,
                fuel_name,
                w
              );
              constexpr auto FMT_FBP_OUT =
                "%ld,%d-%02d-%02d %02d:%02d:%02d,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g,%c,%1.6g,%1.6g,%1.6g,%1.6g,%1.6g%s";
              printf(
                FMT_FBP_OUT,
                i,
                year_,
                static_cast<uint8_t>(month),
                static_cast<uint8_t>(day_of_month),
                static_cast<uint8_t>(hour - day * DAY_HOURS),
                0,
                0,
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
                w->fwi().asValue(),
                spread.crownFractionBurned(),
                spread.crownFuelConsumption(),
                spread.fireDescription(),
                spread.maxIntensity(),
                spread.headDirection().asDegrees(),
                spread.headRos(),
                spread.surfaceFuelConsumption(),
                spread.totalFuelConsumption(),
                "\r\n"
              );
              fprintf(
                out_fbp,
                FMT_FBP_OUT,
                i,
                year_,
                static_cast<uint8_t>(month),
                static_cast<uint8_t>(day_of_month),
                static_cast<uint8_t>(hour - day * DAY_HOURS),
                0,
                0,
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
                w->fwi().asValue(),
                spread.crownFractionBurned(),
                spread.crownFuelConsumption(),
                spread.fireDescription(),
                spread.maxIntensity(),
                spread.headDirection().asDegrees(),
                spread.headRos(),
                spread.surfaceFuelConsumption(),
                spread.totalFuelConsumption(),
                "\r\n"
              );
            }
          }
        }
      }
      ++hour;
    }
    ++i;
  }
  logging::check_fatal(0 != fclose(out), "Could not close file %s", file_out.c_str());
  logging::check_fatal(0 != fclose(out_fbp), "Could not close file %s", file_out_fbp.c_str());
}
#endif
}
