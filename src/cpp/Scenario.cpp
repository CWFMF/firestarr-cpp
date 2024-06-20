/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#define LOG_POINTS_RELATIVE
// #undef LOG_POINTS_RELATIVE
#define LOG_POINTS_CELL
#undef LOG_POINTS_CELL

#include "Scenario.h"
#include "Observer.h"
#include "Perimeter.h"
#include "ProbabilityMap.h"
#include "IntensityMap.h"
#include "FuelType.h"
#include "MergeIterator.h"
#include "CellPoints.h"
#include "FuelLookup.h"
#include "Location.h"
#include "Cell.h"

namespace tbd::sim
{
using topo::Position;
using topo::Cell;
using topo::Perimeter;
using topo::SpreadKey;
using topo::StartPoint;

constexpr auto CELL_CENTER = 0.5;
constexpr auto PRECISION = 0.001;
static atomic<size_t> COUNT = 0;
static atomic<size_t> COMPLETED = 0;
static atomic<size_t> TOTAL_STEPS = 0;
static std::mutex MUTEX_SIM_COUNTS;
static map<size_t, size_t> SIM_COUNTS{};

constexpr auto STAGE_CONDENSE = 'C';
constexpr auto STAGE_NEW = 'N';
constexpr auto STAGE_SPREAD = 'S';
constexpr auto STAGE_INVALID = 'X';

template <typename T, typename F>
void do_each(T& for_list, F fct)
{
  std::for_each(
    std::execution::seq,
    for_list.begin(),
    for_list.end(),
    fct);
}

template <typename T, typename F>
void do_par(T& for_list, F fct)
{
  std::for_each(
    std::execution::par_unseq,
    for_list.begin(),
    for_list.end(),
    fct);
}

CellPointsMap merge_list(
  const BurnedData& unburnable,
  map<SpreadKey, SpreadInfo>& spread_info,
  const double duration,
  const spreading_points& to_spread)
{
  auto spread = std::views::transform(
    to_spread,
    [&duration, &spread_info](
      const spreading_points::value_type& kv0) -> CellPointsMap {
      auto& key = kv0.first;
      const auto& offsets = spread_info[key].offsets();
#ifdef DEBUG_POINTS
      logging::check_fatal(
        offsets.empty(),
        "offsets.empty()");
#endif
      const spreading_points::mapped_type& cell_pts = kv0.second;
#ifdef DEBUG_POINTS
      logging::check_fatal(
        cell_pts.empty(),
        "cell_pts.empty()");
#endif
      auto r = apply_offsets_spreadkey(duration, offsets, cell_pts);
#ifdef DEBUG_POINTS
      logging::check_fatal(
        r.unique().empty(),
        "r.unique().empty()");
#endif
      return r;
    });
  auto it = spread.begin();
  CellPointsMap out{};
  while (spread.end() != it)
  {
    const CellPointsMap& cell_pts = *it;
#ifdef DEBUG_POINTS
    logging::check_fatal(
      cell_pts.unique().empty(),
      "Merging empty points");
#endif
    // // HACK: keep old behaviour until we can figure out whey removing isn't the same as not adding
    // const auto h = cell_pts.location().hash();
    // if (!unburnable[h])
    // {
    out.merge(unburnable, cell_pts);
#ifdef DEBUG_POINTS
    logging::check_fatal(
      out.unique().empty(),
      "Empty points after merge");
#endif
    // }
    ++it;
  }
  return out;
}
CellPointsMap calculate_spread(
  Scenario& scenario,
  map<SpreadKey, SpreadInfo>& spread_info,
  const double duration,
  const spreading_points& to_spread,
  const BurnedData& unburnable)
{
  CellPointsMap cell_pts = merge_list(
    unburnable,
    spread_info,
    duration,
    to_spread);
#ifdef DEBUG_POINTS
  const auto u = cell_pts.unique();
  logging::check_fatal(
    u.empty(),
    "No points after spread");
  size_t total = 0;
  set<InnerPos> u0{};
  for (const auto& kv : cell_pts.map_)
  {
    const auto loc = kv.first;
    const auto u1 = kv.second.unique();
    logging::check_fatal(
      u1.empty(),
      "No points in CellPoints for (%d, %d) after spread",
      loc.column(),
      loc.row());
    // make sure all points are actually in the right location
    const auto s0_cur = u0.size();
    logging::check_equal(
      s0_cur,
      total,
      "total");
    for (const auto& p1 : u1)
    {
      const Location loc1{static_cast<Idx>(p1.y()), static_cast<Idx>(p1.x())};
      logging::check_equal(
        loc1.column(),
        loc.column(),
        "column");
      logging::check_equal(
        loc1.row(),
        loc.row(),
        "row");
      u0.emplace(p1);
    }
    total += u1.size();
    logging::check_equal(
      s0_cur + u1.size(),
      total,
      "total");
  }
#endif
  cell_pts.remove_if(
    [&scenario, &unburnable](
      const pair<Location, CellPoints>& kv) {
      const auto& location = kv.first;
#ifdef DEBUG_POINTS
      // look up Cell from scenario here since we don't need attributes until now
      const Cell k = scenario.cell(location);
      const auto& cell_pts = kv.second;
      const auto u = cell_pts.unique();
      logging::check_fatal(
        u.empty(),
        "Empty points when checking to remove");
      logging::check_equal(
        k,
        scenario.cell(cell_pts.location()),
        "CellPoints location");
      logging::check_equal(
        k.column(),
        location.column(),
        "Cell column");
      logging::check_equal(
        k.row(),
        location.row(),
        "Cell row");
#endif
      const auto h = location.hash();
      // clear out if unburnable
      const auto do_clear = unburnable[h];
#ifdef DEBUG_POINTS
      logging::check_equal(h, k.hash(), "Cell vs Location hash()");
      if (fuel::is_null_fuel(k))
      {
        logging::check_fatal(
          !do_clear,
          "Not clearing when not fuel");
      }
#endif
      return do_clear;
    });
  return cell_pts;
}
class LogPoints
{
public:
  ~LogPoints();
  LogPoints(const string dir_out,
            bool do_log,
            size_t id,
            double start_time);
  void log_point(size_t step,
                 const char stage,
                 double time,
                 double x,
                 double y);
  bool isLogging() const;
  template <class V>
  void log_points(size_t step,
                  const char stage,
                  double time,
                  V points)
  {
    // don't loop if not logging
    if (isLogging())
    {
      const auto u = points.unique();
#ifdef DEBUG_POINTS
      logging::check_fatal(
        u.empty(),
        "Logging empty points");
#endif
      for (const auto& p : u)
      {
        log_point(step, stage, time, p.x(), p.y());
      }
    }
#ifdef DEBUG_POINTS
    else
    {
      const auto u = points.unique();
      logging::check_fatal(
        u.empty(),
        "Logging empty points");
    }
#endif
  };
private:
  size_t id_;
  double start_time_;
  char last_stage_;
  size_t last_step_;
#ifdef DEBUG_POINTS
  double last_time_;
#endif
  char stage_id_[1024];
  /**
   * \brief FILE to write logging information about points to
   */
  FILE* log_points_;
  FILE* log_stages_;
};
LogPoints::~LogPoints()
{
  if (NULL != log_points_)
  {
    fclose(log_points_);
    fclose(log_stages_);
  }
}
LogPoints::LogPoints(
  const string dir_out,
  bool do_log,
  size_t id,
  double start_time)
  : id_(id),
    start_time_(start_time),
    last_stage_(STAGE_INVALID),
    last_step_(std::numeric_limits<size_t>::max()),
    stage_id_()
{
  if (do_log)
  {
    constexpr auto HEADER_STAGES = "step_id,scenario,stage,step,time";
#ifdef LOG_POINTS_CELL
    constexpr auto HEADER_POINTS = "step_id,column,row,x,y\n";
#else
    constexpr auto HEADER_POINTS = "step_id,x,y\n";
#endif
    char log_name[2048];
    sxprintf(log_name, "%s/scenario_%05ld_points.txt", dir_out.c_str(), id);
    log_points_ = fopen(log_name, "w");
    sxprintf(log_name, "%s/scenario_%05ld_stages.txt", dir_out.c_str(), id);
    log_stages_ = fopen(log_name, "w");
    fprintf(log_points_, HEADER_POINTS);
    fprintf(log_stages_, HEADER_STAGES);
  }
  else
  {
    static_assert(NULL == nullptr);
    log_points_ = nullptr;
    log_stages_ = nullptr;
  }
}
void LogPoints::log_point(size_t step,
                          const char stage,
                          double time,
                          double x,
                          double y)
{
  if (!isLogging())
  {
    return;
  }
  static const auto FMT_LOG_STAGE = "%s,%ld,%c,%ld,%f\n";
#ifdef LOG_POINTS_CELL
  static const auto FMT_LOG_POINT = "%s,%d,%d,%f,%f\n";
  const auto column = static_cast<Idx>(x);
  const auto row = static_cast<Idx>(y);
#else
  static const auto FMT_LOG_POINT = "%s,%f,%f\n";
#endif
#ifdef LOG_POINTS_RELATIVE
  constexpr auto MID = MAX_COLUMNS / 2;
  const auto p_x = x - MID;
  const auto p_y = y - MID;
  const auto t = time - start_time_;
#else
  const auto p_x = x;
  const auto p_y = y;
  const auto t = time;
#endif
  // time should always be the same for each step, regardless of stage
  if (last_step_ != step || last_stage_ != stage)
  {
    sxprintf(stage_id_, "%ld%c%ld", id_, stage, step);
    last_stage_ = stage;
    last_step_ = step;
#ifdef DEBUG_POINTS
    last_time_ = t;
#endif
    fprintf(log_stages_,
            FMT_LOG_STAGE,
            stage_id_,
            id_,
            stage,
            step,
            t);
#ifdef DEBUG_POINTS
  }
  else
  {
    logging::check_fatal(t != last_time_,
                         "Expected %s to have time %f but got %f",
                         stage_id_,
                         last_time_,
                         t);
#endif
  }
  fprintf(log_points_,
          FMT_LOG_POINT,
          stage_id_,
#ifdef LOG_POINTS_CELL
          column,
          row,
#endif
          p_x,
          p_y);
}

bool LogPoints::isLogging() const
{
  return nullptr != log_points_;
}

void IObserver_deleter::operator()(IObserver* ptr) const
{
  delete ptr;
}
void Scenario::clear() noexcept
{
  //  scheduler_.clear();
  scheduler_ = set<Event, EventCompare>();
  //  arrival_.clear();
  arrival_ = {};
  //  points_.clear();
  points_ = {};
  if (!Settings::surface())
  {
    spread_info_ = {};
  }
  extinction_thresholds_.clear();
  spread_thresholds_by_ros_.clear();
  max_ros_ = 0;
#ifdef DEBUG_SIMULATION
  log_check_fatal(!scheduler_.empty(), "Scheduler isn't empty after clear()");
#endif
  model_->releaseBurnedVector(unburnable_);
  unburnable_ = nullptr;
  step_ = 0;
  oob_spread_ = 0;
}
size_t Scenario::completed() noexcept
{
  return COMPLETED;
}
size_t Scenario::count() noexcept
{
  return COUNT;
}
size_t Scenario::total_steps() noexcept
{
  return TOTAL_STEPS;
}
Scenario::~Scenario()
{
  clear();
}
/*!
 * \page probability Probability of events
 *
 * Probability throughout the simulations is handled using pre-rolled random numbers
 * based on a fixed seed, so that simulation results are reproducible.
 *
 * Probability is stored as 'thresholds' for a certain event on a day-by-day and hour-by-hour
 * basis. If the calculated probability of that type of event matches or exceeds the threshold
 * then the event will occur.
 *
 * Each iteration of a scenario will have its own thresholds, and thus different behaviour
 * can occur with the same input indices.
 *
 * Thresholds are used to determine:
 * - extinction
 * - spread events
 */
static void make_threshold(vector<double>* thresholds,
                           mt19937* mt,
                           const Day start_day,
                           const Day last_date,
                           double (*convert)(double value))
{
  const auto total_weight = Settings::thresholdScenarioWeight() + Settings::thresholdDailyWeight() + Settings::thresholdHourlyWeight();
  uniform_real_distribution<double> rand(0.0, 1.0);
  const auto general = rand(*mt);
  for (size_t i = start_day; i < MAX_DAYS; ++i)
  {
    const auto daily = rand(*mt);
    for (auto h = 0; h < DAY_HOURS; ++h)
    {
      // generate no matter what so if we extend the time period the results
      // for the first days don't change
      const auto hourly = rand(*mt);
      // only save if we're going to use it
      // HACK: +1 so if it's exactly at the end time there's something there
      if (i <= static_cast<size_t>(last_date + 1))
      {
        // subtract from 1.0 because we want weight to make things more likely not less
        // ensure we stay between 0 and 1
        thresholds->at((i - start_day) * DAY_HOURS + h) =
          convert(
            max(0.0,
                min(1.0,
                    1.0 - (Settings::thresholdScenarioWeight() * general + Settings::thresholdDailyWeight() * daily + Settings::thresholdHourlyWeight() * hourly) / total_weight)));
        //        thresholds->at((i - start_day) * DAY_HOURS + h) = 0.0;
      }
    }
  }
}
constexpr double same(const double value) noexcept
{
  return value;
}
static void make_threshold(vector<double>* thresholds,
                           mt19937* mt,
                           const Day start_day,
                           const Day last_date)
{
  make_threshold(thresholds, mt, start_day, last_date, &same);
}
Scenario::Scenario(Model* model,
                   const size_t id,
                   wx::FireWeather* weather,
                   wx::FireWeather* weather_daily,
                   const double start_time,
                   //  const shared_ptr<IntensityMap>& initial_intensity,
                   const shared_ptr<Perimeter>& perimeter,
                   const StartPoint& start_point,
                   const Day start_day,
                   const Day last_date)
  : Scenario(model,
             id,
             weather,
             weather_daily,
             start_time,
             //  initial_intensity,
             perimeter,
             nullptr,
             start_point,
             start_day,
             last_date)
{
}
Scenario::Scenario(Model* model,
                   const size_t id,
                   wx::FireWeather* weather,
                   wx::FireWeather* weather_daily,
                   const double start_time,
                   const shared_ptr<Cell>& start_cell,
                   const StartPoint& start_point,
                   const Day start_day,
                   const Day last_date)
  : Scenario(model,
             id,
             weather,
             weather_daily,
             start_time,
             // make_unique<IntensityMap>(*model, nullptr),
             nullptr,
             start_cell,
             start_point,
             start_day,
             last_date)
{
}
// HACK: just set next start point here for surface right now
Scenario* Scenario::reset_with_new_start(const shared_ptr<Cell>& start_cell,
                                         util::SafeVector* final_sizes)
{
  start_cell_ = start_cell;
  // FIX: remove duplicated code
  // logging::extensive("Set cell; resetting");
  // return reset(nullptr, nullptr, final_sizes);
  cancelled_ = false;
  model_->releaseBurnedVector(unburnable_);
  unburnable_ = nullptr;
  current_time_ = start_time_;
  intensity_ = nullptr;
  probabilities_ = nullptr;
  final_sizes_ = final_sizes;
  ran_ = false;
  clear();
  for (const auto& o : observers_)
  {
    o->reset();
  }
  current_time_ = start_time_ - 1;
  points_ = {};
  intensity_ = make_unique<IntensityMap>(model());
  // HACK: never reset these if using a surface
  // if (!Settings::surface())
  {
    // these are reset in clear()
    // spread_info_ = {};
    // max_ros_ = 0;
    // surrounded_ = POOL_BURNED_DATA.acquire();
    current_time_index_ = numeric_limits<size_t>::max();
  }
  ++COUNT;
  {
    // want a global count of how many times this scenario ran
    std::lock_guard<std::mutex> lk(MUTEX_SIM_COUNTS);
    simulation_ = ++SIM_COUNTS[id_];
  }
  return this;
}
Scenario* Scenario::reset(mt19937* mt_extinction,
                          mt19937* mt_spread,
                          util::SafeVector* final_sizes)
{
  cancelled_ = false;
  model_->releaseBurnedVector(unburnable_);
  unburnable_ = nullptr;
  current_time_ = start_time_;
  intensity_ = nullptr;
  //  weather_(weather);
  //  model_(model);
  probabilities_ = nullptr;
  final_sizes_ = final_sizes;
  //  start_point_(std::move(start_point));
  //  id_(id);
  //  start_time_(start_time);
  //  simulation_(-1);
  //  start_day_(start_day);
  //  last_date_(last_date);
  ran_ = false;
  // track this here because reset is always called before use
  // HACK: +2 so there's something there if we land exactly on the end date
  const auto num = (static_cast<size_t>(last_date_) - start_day_ + 2) * DAY_HOURS;
  clear();
  extinction_thresholds_.resize(num);
  spread_thresholds_by_ros_.resize(num);
  // if these are null then all probability thresholds remain 0
  if (nullptr != mt_extinction)
  {
    make_threshold(&extinction_thresholds_, mt_extinction, start_day_, last_date_);
  }
  if (nullptr != mt_spread)
  {
    make_threshold(&spread_thresholds_by_ros_,
                   mt_spread,
                   start_day_,
                   last_date_,
                   &SpreadInfo::calculateRosFromThreshold);
  }
  //  std::fill(extinction_thresholds_.begin(), extinction_thresholds_.end(), 1.0 - abs(1.0 / (10 * id_)));
  //  std::fill(spread_thresholds_by_ros_.begin(), spread_thresholds_by_ros_.end(), 1.0 - abs(1.0 / (10 * id_)));
  // std::fill(extinction_thresholds_.begin(), extinction_thresholds_.end(), 0.5);
  //  std::fill(spread_thresholds_by_ros_.begin(), spread_thresholds_by_ros_.end(), SpreadInfo::calculateRosFromThreshold(0.5));
  for (const auto& o : observers_)
  {
    o->reset();
  }
  current_time_ = start_time_ - 1;
  points_ = {};
  // don't do this until we run so that we don't allocate memory too soon
  // log_verbose("Applying initial intensity map");
  // // HACK: if initial_intensity is null then perimeter must be too?
  // intensity_ = (nullptr == initial_intensity_)
  //   ? make_unique<IntensityMap>(model(), nullptr)
  //   : make_unique<IntensityMap>(*initial_intensity_);
  intensity_ = make_unique<IntensityMap>(model());
  spread_info_ = {};
  arrival_ = {};
  max_ros_ = 0;
  // surrounded_ = POOL_BURNED_DATA.acquire();
  current_time_index_ = numeric_limits<size_t>::max();
  ++COUNT;
  {
    // want a global count of how many times this scenario ran
    std::lock_guard<std::mutex> lk(MUTEX_SIM_COUNTS);
    simulation_ = ++SIM_COUNTS[id_];
  }
  return this;
}
void Scenario::evaluate(const Event& event)
{
#ifdef DEBUG_SIMULATION
  log_check_fatal(event.time() < current_time_,
                  "Expected time to be > %f but got %f",
                  current_time_,
                  event.time());
#endif
  const auto& p = event.cell();
  switch (event.type())
  {
    case Event::FIRE_SPREAD:
      ++step_;
#ifdef DEBUG_POINTS
      // if (tbd::logging::Log::getLogLevel() >= tbd::logging::LOG_VERBOSE)
      {
        const auto ymd = tbd::make_timestamp(model().year(), event.time());
        // log_note("Handling spread event for time %f representing %s with %ld points", event.time(), ymd.c_str(), points_.size());
      }
#endif
      scheduleFireSpread(event);
      break;
    case Event::SAVE:
      saveObservers(event.time());
      saveStats(event.time());
      break;
    case Event::NEW_FIRE:
      log_points_->log_point(step_, STAGE_NEW, event.time(), p.column() + CELL_CENTER, p.row() + CELL_CENTER);
      // HACK: don't do this in constructor because scenario creates this in its constructor
      points_.insert(p.column() + CELL_CENTER, p.row() + CELL_CENTER);
      if (fuel::is_null_fuel(event.cell()))
      {
        log_fatal("Trying to start a fire in non-fuel");
      }
      log_verbose("Starting fire at point (%f, %f) in fuel type %s at time %f",
                  p.column() + CELL_CENTER,
                  p.row() + CELL_CENTER,
                  fuel::FuelType::safeName(fuel::check_fuel(event.cell())),
                  event.time());
      if (!survives(event.time(), event.cell(), event.timeAtLocation()))
      {
        // const auto wx = weather(event.time());
        // HACK: show daily values since that's what survival uses
        const auto wx = weather_daily(event.time());
        log_info("Didn't survive ignition in %s with weather %f, %f",
                 fuel::FuelType::safeName(fuel::check_fuel(event.cell())),
                 wx->ffmc(),
                 wx->dmc());
        // HACK: we still want the fire to have existed, so set the intensity of the origin
      }
      // fires start with intensity of 1
      burn(event, 1);
      scheduleFireSpread(event);
      break;
    case Event::END_SIMULATION:
      log_verbose("End simulation event reached at %f", event.time());
      endSimulation();
      break;
    default:
      throw runtime_error("Invalid event type");
  }
}
Scenario::Scenario(Model* model,
                   const size_t id,
                   wx::FireWeather* weather,
                   wx::FireWeather* weather_daily,
                   const double start_time,
                   //  const shared_ptr<IntensityMap>& initial_intensity,
                   const shared_ptr<Perimeter>& perimeter,
                   const shared_ptr<Cell>& start_cell,
                   StartPoint start_point,
                   const Day start_day,
                   const Day last_date)
  : current_time_(start_time),
    unburnable_(nullptr),
    intensity_(nullptr),
    // initial_intensity_(initial_intensity),
    perimeter_(perimeter),
    // surrounded_(nullptr),
    max_ros_(0),
    start_cell_(start_cell),
    weather_(weather),
    weather_daily_(weather_daily),
    model_(model),
    probabilities_(nullptr),
    final_sizes_(nullptr),
    start_point_(std::move(start_point)),
    id_(id),
    start_time_(start_time),
    simulation_(-1),
    start_day_(start_day),
    last_date_(last_date),
    ran_(false),
    step_(0),
    oob_spread_(0)
{
  last_save_ = weather_->minDate();
  const auto wx = weather_->at(start_time_);
  logging::check_fatal(nullptr == wx,
                       "No weather for start time %s",
                       make_timestamp(model->year(), start_time_).c_str());
  const auto saves = Settings::outputDateOffsets();
  const auto last_save = start_day_ + saves[saves.size() - 1];
  logging::check_fatal(last_save > weather_->maxDate(),
                       "No weather for last save time %s",
                       make_timestamp(model->year(), last_save).c_str());
  log_points_ = make_shared<LogPoints>(model_->outputDirectory(),
                                       Settings::savePoints(),
                                       id_,
                                       start_time_);
}
void Scenario::saveStats(const double time) const
{
  probabilities_->at(time)->addProbability(*intensity_);
  if (time == last_save_)
  {
    final_sizes_->addValue(intensity_->fireSize());
  }
}
void Scenario::registerObserver(IObserver* observer)
{
  observers_.push_back(unique_ptr<IObserver, IObserver_deleter>(observer));
}
void Scenario::notify(const Event& event) const
{
  for (const auto& o : observers_)
  {
    o->handleEvent(event);
  }
}
void Scenario::saveObservers(const string& base_name) const
{
  for (const auto& o : observers_)
  {
    o->save(model_->outputDirectory(), base_name);
  }
}
void Scenario::saveObservers(const double time) const
{
  char buffer[64]{0};
  sxprintf(buffer,
           "%03zu_%06ld_%03d",
           id(),
           simulation(),
           static_cast<int>(time));
  saveObservers(string(buffer));
}
void Scenario::saveIntensity(const string& dir, const string& base_name) const
{
  intensity_->save(dir, base_name);
}
bool Scenario::ran() const noexcept
{
  return ran_;
}
Scenario::Scenario(Scenario&& rhs) noexcept
  : observers_(std::move(rhs.observers_)),
    save_points_(std::move(rhs.save_points_)),
    extinction_thresholds_(std::move(rhs.extinction_thresholds_)),
    spread_thresholds_by_ros_(std::move(rhs.spread_thresholds_by_ros_)),
    current_time_(rhs.current_time_),
    points_(std::move(rhs.points_)),
    unburnable_(std::move(rhs.unburnable_)),
    scheduler_(std::move(rhs.scheduler_)),
    intensity_(std::move(rhs.intensity_)),
    // initial_intensity_(std::move(rhs.initial_intensity_)),
    perimeter_(std::move(rhs.perimeter_)),
    spread_info_(std::move(rhs.spread_info_)),
    arrival_(std::move(rhs.arrival_)),
    max_ros_(rhs.max_ros_),
    start_cell_(std::move(rhs.start_cell_)),
    weather_(rhs.weather_),
    weather_daily_(rhs.weather_daily_),
    model_(rhs.model_),
    probabilities_(rhs.probabilities_),
    final_sizes_(rhs.final_sizes_),
    start_point_(std::move(rhs.start_point_)),
    id_(rhs.id_),
    start_time_(rhs.start_time_),
    last_save_(rhs.last_save_),
    simulation_(rhs.simulation_),
    start_day_(rhs.start_day_),
    last_date_(rhs.last_date_),
    ran_(rhs.ran_)
{
}
Scenario& Scenario::operator=(Scenario&& rhs) noexcept
{
  if (this != &rhs)
  {
    observers_ = std::move(rhs.observers_);
    save_points_ = std::move(rhs.save_points_);
    extinction_thresholds_ = std::move(rhs.extinction_thresholds_);
    spread_thresholds_by_ros_ = std::move(rhs.spread_thresholds_by_ros_);
    points_ = std::move(rhs.points_);
    current_time_ = rhs.current_time_;
    scheduler_ = std::move(rhs.scheduler_);
    intensity_ = std::move(rhs.intensity_);
    // initial_intensity_ = std::move(rhs.initial_intensity_);
    perimeter_ = std::move(rhs.perimeter_);
    // surrounded_ = rhs.surrounded_;
    start_cell_ = std::move(rhs.start_cell_);
    weather_ = rhs.weather_;
    weather_daily_ = rhs.weather_daily_;
    model_ = rhs.model_;
    probabilities_ = rhs.probabilities_;
    final_sizes_ = rhs.final_sizes_;
    start_point_ = std::move(rhs.start_point_);
    id_ = rhs.id_;
    start_time_ = rhs.start_time_;
    last_save_ = rhs.last_save_;
    simulation_ = rhs.simulation_;
    start_day_ = rhs.start_day_;
    last_date_ = rhs.last_date_;
    ran_ = rhs.ran_;
  }
  return *this;
}
void Scenario::burn(const Event& event, const IntensitySize)
{
#ifdef DEBUG_SIMULATION
  log_check_fatal(
    intensity_->hasBurned(event.cell()),
    "Re-burning cell (%d, %d)",
    event.cell().column(),
    event.cell().row());
#endif
#ifdef DEBUG_POINTS
  log_check_fatal(
    (*unburnable_)[event.cell().hash()],
    "Burning unburnable cell (%d, %d)",
    event.cell().column(),
    event.cell().row());
#endif
  // Observers only care about cells burning so do it here
  notify(event);
  // WIP: call burn without proper information for now so we can commit IntensityMap changes
  intensity_->burn(event.cell(), event.intensity(), 0, tbd::wx::Direction::Zero);
#ifdef DEBUG_GRIDS
  log_check_fatal(
    !intensity_->hasBurned(event.cell()),
    "Wasn't marked as burned after burn");
#endif
  arrival_[event.cell()] = event.time();
  // scheduleFireSpread(event);
}
bool Scenario::isSurrounded(const Location& location) const
{
  return intensity_->isSurrounded(location);
}
Cell Scenario::cell(const InnerPos& p) const noexcept
{
  return cell(p.y(), p.x());
}
string Scenario::add_log(const char* format) const noexcept
{
  const string tmp;
  stringstream iss(tmp);
  static char buffer[1024]{0};
  sxprintf(buffer, "Scenario %4ld.%04ld (%3f): ", id(), simulation(), current_time_);
  iss << buffer << format;
  return iss.str();
}
#ifdef DEBUG_PROBABILITY
void saveProbabilities(const string& dir,
                       const string& base_name,
                       vector<double>& thresholds)
{
  ofstream out;
  out.open(dir + base_name + ".csv");
  for (auto v : thresholds)
  {
    out << v << '\n';
  }
  out.close();
}
#endif
Scenario* Scenario::run(map<double, ProbabilityMap*>* probabilities)
{
#ifdef DEBUG_SIMULATION
  log_check_fatal(ran(), "Scenario has already run");
#endif
  log_verbose("Starting");
  CriticalSection _(Model::task_limiter);
  unburnable_ = model_->getBurnedVector();
  probabilities_ = probabilities;
  log_verbose("Setting save points");
  for (auto time : save_points_)
  {
    // NOTE: these happen in this order because of the way they sort based on type
    addEvent(Event::makeSave(static_cast<double>(time)));
  }
  if (nullptr == perimeter_)
  {
    addEvent(Event::makeNewFire(start_time_, cell(*start_cell_)));
  }
  else
  {
    log_verbose("Applying perimeter");
    intensity_->applyPerimeter(*perimeter_);
    log_verbose("Perimeter applied");
    const auto& env = model().environment();
    log_verbose("Igniting points");
    for (const auto& location : perimeter_->edge())
    {
      //      const auto cell = env.cell(location.hash());
      const auto cell = env.cell(location);
#ifdef DEBUG_SIMULATION
      log_check_fatal(fuel::is_null_fuel(cell), "Null fuel in perimeter");
#endif
      // log_verbose("Adding point (%d, %d)",
      log_verbose("Adding point (%f, %f)",
                  cell.column() + CELL_CENTER,
                  cell.row() + CELL_CENTER);
      points_.insert(cell.column() + CELL_CENTER, cell.row() + CELL_CENTER);
      // auto e = points_.try_emplace(cell, cell.column() + CELL_CENTER, cell.row() + CELL_CENTER);
      // log_check_fatal(!e.second,
      //                 "Excepted to add point to new cell but (%ld, %ld) is already in map",
      //                 cell.column(),
      //                 cell.row());
    }
    addEvent(Event::makeFireSpread(start_time_));
  }
  // HACK: make a copy of the event so that it still exists after it gets processed
  // NOTE: sorted so that EventSaveASCII is always just before this
  // Only run until last time we asked for a save for
  log_verbose("Creating simulation end event for %f", last_save_);
  addEvent(Event::makeEnd(last_save_));
  // mark all original points as burned at start
  for (auto& kv : points_.map_)
  {
    const auto& location = cell(kv.first);
    // const auto& location = kv.first;
    // would be burned already if perimeter applied
    if (canBurn(location))
    {
      const auto fake_event = Event::makeFireSpread(
        start_time_,
        static_cast<IntensitySize>(1),
        location);
      burn(fake_event, static_cast<IntensitySize>(1));
    }
  }
  while (!cancelled_ && !scheduler_.empty())
  {
    evaluateNextEvent();
    // // FIX: the timer thread can cancel these instead of having this check
    // if (!evaluateNextEvent())
    // {
    //   cancel(true);
    // }
  }
  ++TOTAL_STEPS;
  model_->releaseBurnedVector(unburnable_);
  unburnable_ = nullptr;
  if (cancelled_)
  {
    return nullptr;
  }
  const auto completed = ++COMPLETED;
  // const auto count = Settings::surface() ? model_->ignitionScenarios() : (+COUNT);
  // HACK: use + to pull value out of atomic
  const auto count = Settings::surface() ? model_->scenarioCount() : (+COUNT);
  const auto log_level = (0 == (completed % 1000)) ? logging::LOG_NOTE : logging::LOG_INFO;
  if (Settings::surface())
  {
    const auto ratio_done = static_cast<double>(completed) / count;
    const auto s = model_->runTime().count();
    const auto r = static_cast<size_t>(s / ratio_done) - s;
    log_output(log_level,
               "[% d of % d] (%0.2f%%) <%lds : %lds remaining> Completed with final size % 0.1f ha",
               completed,
               count,
               100 * ratio_done,
               s,
               r,
               currentFireSize());
  }
  else
  {
#ifdef NDEBUG
    log_output(log_level,
               "[% d of % d] Completed with final size % 0.1f ha",
               completed,
               count,
               currentFireSize());
#else
    // try to make output consistent if in debug mode
    log_output(log_level, "Completed with final size %0.1f ha", currentFireSize());
#endif
  }
  ran_ = true;
#ifdef DEBUG_PROBABILITY
  // nice to have this get output when debugging, but only need it in extreme cases
  if (logging::Log::getLogLevel() <= logging::LOG_EXTENSIVE)
  {
    char buffer[64]{0};
    sxprintf(buffer,
             "%03zu_%06ld_extinction",
             id(),
             simulation());
    saveProbabilities(model().outputDirectory(), string(buffer), extinction_thresholds_);
    sxprintf(buffer,
             "%03zu_%06ld_spread",
             id(),
             simulation());
    saveProbabilities(model().outputDirectory(), string(buffer), spread_thresholds_by_ros_);
  }
#endif
  if (oob_spread_ > 0)
  {
    log_warning("Tried to spread out of bounds %ld times", oob_spread_);
  }
  return this;
}
void Scenario::scheduleFireSpread(const Event& event)
{
  const auto time = event.time();
  // HACK: if a surface then always use 1600 weather
  // keeps a bunch of things we don't need in it if we don't reset?
  // const auto this_time = Settings::surface() ? util::time_index(static_cast<Day>(time), 16) : util::time_index(time);
  const auto this_time = util::time_index(time);
  // const auto wx_time = Settings::surface() ? util::to_time(util::time_index(static_cast<Day>(time), 16)) : util::to_time(this_time);
  // const auto wx_time = util::to_time(this_time);
  const auto wx = Settings::surface() ? model_->yesterday() : weather(time);
  const auto wx_daily = Settings::surface() ? model_->yesterday() : weather_daily(time);
  // note("time is %f", time);
  current_time_ = time;
  logging::check_fatal(nullptr == wx, "No weather available for time %f", time);
  //  log_note("%d points", points_->size());
  const auto next_time = static_cast<double>(this_time + 1) / DAY_HOURS;
  // should be in minutes?
  const auto max_duration = (next_time - time) * DAY_MINUTES;
  // log_verbose("time is %f, next_time is %f, max_duration is %f",
  //      time,
  //      next_time,
  //      max_duration);
  const auto max_time = time + max_duration / DAY_MINUTES;
  // if (wx->ffmc().asDouble() < minimumFfmcForSpread(time))
  // HACK: use the old ffmc for this check to be consistent with previous version
  if (wx_daily->ffmc().asDouble() < minimumFfmcForSpread(time))
  {
    addEvent(Event::makeFireSpread(max_time));
    log_extensive("Waiting until %f because of FFMC", max_time);
    return;
  }
  // log_note("There are %ld spread offsets calculated", spread_info_.size());
  if (current_time_index_ != this_time)
  {
    // logging::check_fatal(Settings::surface() && current_time_index_ != numeric_limits<size_t>::max(),
    //                      "Expected to only pick weather time once");
    current_time_index_ = this_time;
    // seemed like it would be good to keep offsets but max_ros_ needs to reset or things slow to a crawl?
    if (!Settings::surface())
    {
      spread_info_ = {};
    }
    max_ros_ = 0.0;
  }
  // get once and keep
  const auto ros_min = Settings::minimumRos();
  spreading_points to_spread{};
  // make block to prevent it being visible beyond use
  {
    // if we use an iterator this way we don't need to copy keys to erase things
    auto it = points_.map_.begin();
    while (it != points_.map_.end())
    {
      const Location& loc = it->first;
      const Cell for_cell = cell(loc);
#ifdef DEBUG_POINTS
      const auto loc1 = it->first;
      logging::check_equal(
        cell(loc1).hash(),
        for_cell.hash(),
        "hash");
      logging::check_equal(
        loc1.column(),
        loc.column(),
        "column");
      logging::check_equal(
        loc1.row(),
        loc.row(),
        "row");
      const auto f = fuel::fuel_by_code(for_cell.fuelCode());
      logging::check_fatal(
        nullptr == f,
        "Null fuel");
      logging::check_fatal(
        fuel::is_null_fuel(for_cell),
        "Spreading in non-fuel");
#endif
      const auto key = for_cell.key();
      // HACK: need to lookup before emplace since might try to create Cell without fuel
      // if (!fuel::is_null_fuel(loc))
      const auto h = for_cell.hash();
      if (!(*unburnable_)[h])
      // if (canBurn(for_cell))
      {
        const auto& origin_inserted = spread_info_.try_emplace(key, *this, time, key, nd(time), wx);
        // any cell that has the same fuel, slope, and aspect has the same spread
        const auto& origin = origin_inserted.first->second;
        // filter out things not spreading fast enough here so they get copied if they aren't
        // isNotSpreading() had better be true if ros is lower than minimum
        const auto ros = origin.headRos();
#ifdef DEBUG_POINTS
        SpreadInfo spread{*this, time, key, nd(time), wx};
        logging::check_equal(
          spread.headRos(),
          ros,
          "ros");
        if (fuel::is_null_fuel(for_cell))
        {
          logging::check_equal(
            -1,
            ros,
            "ros in non-fuel");
        }
#endif
        if (ros >= ros_min)
        {
          max_ros_ = max(max_ros_, ros);
          // NOTE: shouldn't be Cell if we're looking up by just Location later
          to_spread[key].emplace_back(loc, std::move(it->second));
          it = points_.map_.erase(it);
        }
        else
        {
          ++it;
        }
      }
    }
  }
  // if nothing in to_spread then nothing is spreading
  if (to_spread.empty())
  {
    // if no spread then we left everything back in points_ still
    log_verbose("Waiting until %f", max_time);
    addEvent(Event::makeFireSpread(max_time));
    return;
  }
  // note("Max spread is %f, max_ros is %f",
  //      Settings::maximumSpreadDistance() * cellSize(),
  //      max_ros_);
  const auto duration = ((max_ros_ > 0)
                           ? min(max_duration,
                                 Settings::maximumSpreadDistance() * cellSize() / max_ros_)
                           : max_duration);
  // note("Spreading for %f minutes", duration);
  const auto new_time = time + duration / DAY_MINUTES;
  if ((0 == id_) && (abs(new_time - 154.9987423154746) < 0.001))
  {
    printf("here\n");
  }
  CellPointsMap points_cur = calculate_spread(
    *this,
    spread_info_,
    duration,
    to_spread,
    *unburnable_);
#ifdef DEBUG_POINTS
  logging::check_fatal(
    points_.unique().empty(),
    "points_.unique().empty()");
  map<Location, set<InnerPos>> m0{};
  for (const auto& kv : points_.map_)
  {
    m0.emplace(kv.first, kv.second.unique());
    logging::check_fatal(
      fuel::is_null_fuel(cell(kv.first)),
      "Spreading in non-fuel");
  }
  logging::check_fatal(
    m0.empty(),
    "No points");
#endif
  // need to merge new points back into cells that didn't spread
  points_.merge(
    *unburnable_,
    points_cur);
#ifdef DEBUG_POINTS
  map<Location, set<InnerPos>> m1{};
  for (const auto& kv : points_cur.map_)
  {
    m1.emplace(kv.first, kv.second.unique());
    logging::check_fatal(
      fuel::is_null_fuel(cell(kv.first)),
      "Spreading in non-fuel");
  }
  logging::check_fatal(
    m0 != m1,
    "Sets of points don't match after swap");
#endif
  // if we move everything out of points_ we can parallelize this check?
  do_each(
    points_.map_,
    [this, &new_time](pair<const Location, CellPoints>& kv) {
      const auto for_cell = cell(kv.first);
#ifdef DEBUG_POINTS
      {
        const Location loc{for_cell.row(), for_cell.column()};
        logging::check_equal(
          for_cell.column(),
          loc.column(),
          "column");
        logging::check_equal(
          for_cell.row(),
          loc.row(),
          "row");
      }
      {
        const Location loc{kv.first.row(), kv.first.column()};
        logging::check_equal(
          for_cell.column(),
          loc.column(),
          "column");
        logging::check_equal(
          for_cell.row(),
          loc.row(),
          "row");
      }
      logging::check_equal(
        for_cell.column(),
        kv.first.column(),
        "column");
      logging::check_equal(
        for_cell.row(),
        kv.first.row(),
        "row");
      const auto c1 = cell(for_cell);
      logging::check_equal(
        for_cell.column(),
        c1.column(),
        "column");
      logging::check_equal(
        for_cell.row(),
        c1.row(),
        "row");
      logging::check_equal(
        c1.hash(),
        for_cell.hash(),
        "for_cell");
#endif
      CellPoints& pts = kv.second;
      // logging::check_fatal(pts.empty(), "Empty points for some reason");
      // ******************* CHECK THIS BECAUSE IF SOMETHING IS IN HERE SHOULD IT ALWAYS HAVE SPREAD????? *****************8
      const auto& seek_spread = spread_info_.find(for_cell.key());
      const auto max_intensity = (spread_info_.end() == seek_spread) ? 0 : seek_spread->second.maxIntensity();
      // // if we don't have empty cells anymore then intensity should always be >0?
      // logging::check_fatal(max_intensity <= 0,
      //                      "Expected max_intensity to be > 0 but got %f",
      //                      max_intensity);
      // HACK: just use side-effect to log and check bounds
      log_points_->log_points(step_, STAGE_SPREAD, new_time, pts);
#ifdef DEBUG_POINTS
      const auto loc = pts.location();
      logging::check_equal(
        for_cell.column(),
        loc.column(),
        "column");
      logging::check_equal(
        for_cell.row(),
        loc.row(),
        "row");
      for (auto& pos : pts.unique())
      {
        const Location loc1{static_cast<Idx>(pos.y()), static_cast<Idx>(pos.x())};
        logging::check_equal(
          loc1.column(),
          loc.column(),
          "column");
        logging::check_equal(
          loc1.row(),
          loc.row(),
          "row");
        // was doing this check after getting for_cell, so it didn't help when out of bounds
        log_check_fatal(pos.x() < 0 || pos.y() < 0 || pos.x() >= this->columns() || pos.y() >= this->rows(),
                        "Tried to spread out of bounds to (%f, %f)",
                        pos.x(),
                        pos.y());
      }
      logging::check_equal(
        cell(kv.first).hash(),
        for_cell.hash(),
        "for_cell.hash()");
#endif
      if ((0 == id_) && (abs(new_time - 154.9987423154746) < 0.001))
      {
        printf("here\n");
      }
      if (canBurn(for_cell) && max_intensity > 0)
      {
        // HACK: make sure it can't round down to 0
        const auto intensity = static_cast<IntensitySize>(max(
          1.0,
          max_intensity));
        // HACK: just use the first cell as the source
        const auto fake_event = Event::makeFireSpread(
          new_time,
          intensity,
          for_cell,
          pts.sources());
        if ((0 == id_) && (abs(new_time - 154.9987423154746) < 0.001))
        {
          printf("here\n");
        }
        burn(fake_event, intensity);
      }
      if (!(*unburnable_)[for_cell.hash()]
          // && canBurn(for_cell)
          && ((survives(new_time, for_cell, new_time - arrival_[for_cell])
               && !isSurrounded(for_cell))))
      {
        log_points_->log_points(step_, STAGE_CONDENSE, new_time, pts);
        const auto r = for_cell.row();
        const auto c = for_cell.column();
        const auto f_c = for_cell.fuelCode();
        if ((0 == id_) && (abs(new_time - 154.9987423154746) < 0.001))
        {
          const auto f = fuel::fuel_by_code(f_c);
          printf(
            "(%d, %d) is fuel code %d with name %s\n",
            c,
            r,
            f_c,
            f->name());
          // if (0 == strcmp(f->name(), "M-1/M-2 (00 PC)"))
          // {
          //   printf(
          //     "(%d, %d) is fuel code %d with name %s\n",
          //     c,
          //     r,
          //     f_c,
          //     f->name());
          // }
          if (1 >= abs(c - 2048) && 1 >= abs(r - 2048))
          {
            printf(
              "spreading in center (%d, %d) (%d) is fuel code %d with name %s: cell unburnable is %s\n",
              c,
              r,
              for_cell.hash(),
              f_c,
              f->name(),
              ((*unburnable_)[for_cell.hash()] ? "true" : "false"));
          }
        }
        const Location loc{r, c};
#ifdef DEBUG_POINTS
        logging::check_equal(
          for_cell.column(),
          loc.column(),
          "column");
        logging::check_equal(
          for_cell.row(),
          loc.row(),
          "row");
        const Location loc1 = pts.location();
        logging::check_equal(
          loc1.column(),
          loc.column(),
          "column");
        logging::check_equal(
          loc1.row(),
          loc.row(),
          "row");
        const auto s0 = pts.unique().size();
        const auto c1 = cell(loc);
        const auto h0 = for_cell.hash();
        const auto h1 = c1.hash();
        logging::check_equal(
          h1,
          h0,
          "for_cell");
        logging::check_fatal(
          fuel::is_null_fuel(for_cell),
          "Invalid fuel but trying to save points");
#endif
        std::swap(points_.map_[loc], pts);
#ifdef DEBUG_POINTS
        const auto s1 = points_.map_[loc].unique().size();
        logging::check_equal(s0, s1, "size");
        logging::check_equal(
          pts.unique().size(),
          0,
          "old size");
#endif
      }
      else
      {
        // just inserted false, so make sure unburnable gets updated
        // whether it went out or is surrounded just mark it as unburnable
        (*unburnable_)[for_cell.hash()] = true;
// not swapping means these points get dropped
#ifdef DEBUG_POINTS
        auto seek = points_.map_.find(loc);
        logging::check_fatal(
          seek != points_.map_.end(),
          "points_.map_ contains (%d, %d) when it shouldn't",
          loc.column(),
          loc.row());
#endif
      }
    });
#ifdef DEBUG_POINTS
  for (const auto& kv : points_.map_)
  {
    const auto& pts = kv.second;
    const auto u = pts.unique();
    logging::check_fatal(
      u.empty(),
      "Empty points after spread");
    for (auto& pos : u)
    {
      const auto loc = pts.location();
      const Location loc1{static_cast<Idx>(pos.y()), static_cast<Idx>(pos.x())};
      logging::check_equal(
        loc1.column(),
        loc.column(),
        "column");
      logging::check_equal(
        loc1.row(),
        loc.row(),
        "row");
      // was doing this check after getting for_cell, so it didn't help when out of bounds
      log_check_fatal(pos.x() < 0 || pos.y() < 0 || pos.x() >= this->columns() || pos.y() >= this->rows(),
                      "Tried to spread out of bounds to (%f, %f)",
                      pos.x(),
                      pos.y());
    }
  }
#endif
  log_extensive("Spreading %d cells until %f", points_.map_.size(), new_time);
  addEvent(Event::makeFireSpread(new_time));
}
double
  Scenario::currentFireSize() const
{
  return intensity_->fireSize();
}
bool Scenario::canBurn(const Cell& location) const
{
  return intensity_->canBurn(location);
}
// bool Scenario::canBurn(const HashSize hash) const
//{
//   return intensity_->canBurn(hash);
// }
bool Scenario::hasBurned(const Location& location) const
{
  return intensity_->hasBurned(location);
}
// bool Scenario::hasBurned(const HashSize hash) const
//{
//   return intensity_->hasBurned(hash);
// }
void Scenario::endSimulation() noexcept
{
  log_verbose("Ending simulation");
  //  scheduler_.clear();
  scheduler_ = set<Event, EventCompare>();
}
void Scenario::addSaveByOffset(const int offset)
{
  // offset is from begging of the day the simulation starts
  // e.g. 1 is midnight, 2 is tomorrow at midnight
  addSave(static_cast<Day>(startTime()) + offset);
}
vector<double> Scenario::savePoints() const
{
  return save_points_;
}
template <class V>
void Scenario::addSave(V time)
{
  last_save_ = max(last_save_, static_cast<double>(time));
  save_points_.push_back(time);
}
void Scenario::addEvent(Event&& event)
{
  scheduler_.insert(std::move(event));
}
// bool Scenario::evaluateNextEvent()
void Scenario::evaluateNextEvent()
{
  // make sure to actually copy it before we erase it
  const auto& event = *scheduler_.begin();
  evaluate(event);
  if (!scheduler_.empty())
  {
    scheduler_.erase(event);
  }
  // return !model_->isOutOfTime();
  // return cancelled_;
}
void Scenario::cancel(bool show_warning) noexcept
{
  // ignore if already cancelled
  if (!cancelled_)
  {
    cancelled_ = true;
    if (show_warning)
    {
      log_warning("Simulation cancelled");
    }
  }
}
}
