/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2025. */

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

#include "FuelLookup.h"
#include "Location.h"
#include "Cell.h"
#include "LogPoints.h"

namespace fs::sim
{
using topo::Position;
using topo::Cell;
using topo::Perimeter;
using topo::SpreadKey;
using topo::StartPoint;

constexpr auto CELL_CENTER = static_cast<InnerSize>(0.5);
constexpr auto PRECISION = static_cast<MathSize>(0.001);
static atomic<size_t> COUNT = 0;
static atomic<size_t> COMPLETED = 0;
static atomic<size_t> TOTAL_STEPS = 0;
static std::mutex MUTEX_SIM_COUNTS;
static map<size_t, size_t> SIM_COUNTS{};

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
void Scenario::clear() noexcept
{
  //  scheduler_.clear();
  scheduler_ = set<Event, EventCompare>();
  spread_info_ = {};
  extinction_thresholds_.clear();
  spread_thresholds_by_ros_.clear();
  max_ros_ = 0;
#ifdef DEBUG_SIMULATION
  log_check_fatal(!scheduler_.empty(), "Scheduler isn't empty after clear()");
#endif
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
static void make_threshold(vector<ThresholdSize>* thresholds,
                           mt19937* mt,
                           const Day start_day,
                           const Day last_date,
                           ThresholdSize (*convert)(double value))
{
  const auto total_weight = Settings::thresholdScenarioWeight() + Settings::thresholdDailyWeight() + Settings::thresholdHourlyWeight();
  uniform_real_distribution<ThresholdSize> rand(0.0, 1.0);
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
template <class V>
constexpr V same(const V value) noexcept
{
  return value;
}
static void make_threshold(vector<ThresholdSize>* thresholds,
                           mt19937* mt,
                           const Day start_day,
                           const Day last_date)
{
  make_threshold(thresholds, mt, start_day, last_date, &same);
}
Scenario::Scenario(
  Model* model,
  const size_t id,
  wx::FireWeather* weather,
  wx::FireWeather* weather_daily,
  const DurationSize start_time,
  const Perimeter* perimeter,
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
             std::optional<HashSize>(),
             start_point,
             start_day,
             last_date)
{
}
Scenario::Scenario(Model* model,
                   const size_t id,
                   wx::FireWeather* weather,
                   wx::FireWeather* weather_daily,
                   const DurationSize start_time,
                   const HashSize start_cell,
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
             std::optional<HashSize>(start_cell),
             start_point,
             start_day,
             last_date)
{
}
// HACK: just set next start point here for surface right now
Scenario* Scenario::reset_with_new_start(
  const HashSize start_cell,
  util::SafeVector* final_sizes)
{
  logging::check_fatal(
    nullptr != perimeter_,
    "Resetting start cell to %d when perimeter exists",
    start_cell);
  start_cell_ = start_cell;
  // FIX: remove duplicated code
  // logging::extensive("Set cell; resetting");
  // return reset(nullptr, nullptr, final_sizes);
  cancelled_ = false;
  current_time_ = start_time_;
  intensity_new_ = nullptr;
  probabilities_ = nullptr;
  final_sizes_ = final_sizes;
  ran_ = false;
  clear();
  current_time_ = start_time_ - 1;
  intensity_new_ = make_unique<IntensityMap>(model());
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
  current_time_ = start_time_;
  intensity_new_ = nullptr;
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
  current_time_ = start_time_ - 1;
  // don't do this until we run so that we don't allocate memory too soon
  // log_verbose("Applying initial intensity map");
  // // HACK: if initial_intensity is null then perimeter must be too?
  // intensity_ = (nullptr == initial_intensity_)
  //   ? make_unique<IntensityMap>(model(), nullptr)
  //   : make_unique<IntensityMap>(*initial_intensity_);
  intensity_new_ = make_unique<IntensityMap>(model());
  spread_info_ = {};
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
  const auto x = p.column() + CELL_CENTER;
  const auto y = p.row() + CELL_CENTER;
  const XYPos p0{x, y};
  switch (event.type())
  {
    case Event::FIRE_SPREAD:
      ++step_;
#ifdef DEBUG_POINTS
      // if (fs::logging::Log::getLogLevel() >= fs::logging::LOG_VERBOSE)
      {
        const auto ymd = fs::make_timestamp(model().year(), event.time());
        // log_verbose("Handling spread event for time %f representing %s with %ld points", event.time(), ymd.c_str(), points_.size());
      }
#endif
      scheduleFireSpread(event);
      break;
    case Event::SAVE:
      saveStats(event.time());
      break;
    case Event::NEW_FIRE:
      // HACK: don't do this in constructor because scenario creates this in its constructor
      // HACK: insert point as originating from itself
      points_new_.insert(
        (*intensity_new_->unburnable_)[p.hash()],
        p0);
      if (fuel::is_null_fuel(event.cell()))
      {
        log_fatal("Trying to start a fire in non-fuel");
      }
      log_verbose("Starting fire at point (%f, %f) in fuel type %s at time %f",
                  x,
                  y,
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
      intensity_new_->ignite(event.cell().hash());
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
Scenario::Scenario(
  Model* model,
  const size_t id,
  wx::FireWeather* weather,
  wx::FireWeather* weather_daily,
  const DurationSize start_time,
  //  const shared_ptr<IntensityMap>& initial_intensity,
  const Perimeter* perimeter,
  const std::optional<HashSize> start_cell,
  StartPoint start_point,
  const Day start_day,
  const Day last_date)
  : current_time_(start_time),
    intensity_new_(nullptr),
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
}
void Scenario::saveStats(const DurationSize time) const
{
  size_t i = 0;
  while (i < save_points_.size() && save_points_[i] < time)
  {
    ++i;
  }
  logging::check_fatal(
    i >= save_points_.size(),
    "Invalid save time %f is out of bounds",
    time);
  probabilities_->at(i)->addProbability(*intensity_new_);
  if (time == last_save_)
  {
    final_sizes_->addValue(intensity_new_->fireSize());
  }
}
bool Scenario::ran() const noexcept
{
  return ran_;
}
Scenario::Scenario(Scenario&& rhs) noexcept
  : save_points_(std::move(rhs.save_points_)),
    extinction_thresholds_(std::move(rhs.extinction_thresholds_)),
    spread_thresholds_by_ros_(std::move(rhs.spread_thresholds_by_ros_)),
    current_time_(rhs.current_time_),
    points_new_(std::move(rhs.points_new_)),
    scheduler_(std::move(rhs.scheduler_)),
    intensity_new_(std::move(rhs.intensity_new_)),
    // initial_intensity_(std::move(rhs.initial_intensity_)),
    perimeter_(std::move(rhs.perimeter_)),
    spread_info_(std::move(rhs.spread_info_)),
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
    save_points_ = std::move(rhs.save_points_);
    extinction_thresholds_ = std::move(rhs.extinction_thresholds_);
    spread_thresholds_by_ros_ = std::move(rhs.spread_thresholds_by_ros_);
    points_new_ = std::move(rhs.points_new_);
    current_time_ = rhs.current_time_;
    scheduler_ = std::move(rhs.scheduler_);
    intensity_new_ = std::move(rhs.intensity_new_);
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
bool Scenario::isSurrounded(const HashSize hash_value) const
{
  return intensity_new_->isSurrounded(hash_value);
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
                       vector<ThresholdSize>& thresholds)
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
Scenario* Scenario::run(vector<shared_ptr<ProbabilityMap>>* probabilities)
{
#ifdef DEBUG_SIMULATION
  log_check_fatal(ran(), "Scenario has already run");
#endif
  log_verbose("Starting");
  CriticalSection _(Model::task_limiter);
  logging::debug("Concurrent Scenario limit is %d", Model::task_limiter.limit());
  probabilities_ = probabilities;
  log_verbose("Setting save points");
  for (auto time : save_points_)
  {
    // NOTE: these happen in this order because of the way they sort based on type
    addEvent(Event::makeSave(static_cast<DurationSize>(time)));
  }
  if (nullptr == perimeter_)
  {
    addEvent(Event::makeNewFire(start_time_, cell(*start_cell_)));
  }
  else
  {
    log_verbose("Applying perimeter");
    intensity_new_->applyPerimeter(*perimeter_);
    log_verbose("Perimeter applied");
    const auto& env = model().environment();
    log_verbose("Igniting points");
    for (const auto& location : perimeter_->edge)
    {
      //      const auto cell = env.cell(location.hash());
      const auto cell = env.cell(location);
#ifdef DEBUG_SIMULATION
      log_check_fatal(fuel::is_null_fuel(cell), "Null fuel in perimeter");
#endif
      const auto x = cell.column() + CELL_CENTER;
      const auto y = cell.row() + CELL_CENTER;
      const XYPos p0{x, y};
      // log_extensive("Adding point (%d, %d)",
      log_extensive("Adding point (%f, %f)",
                    x,
                    y);
      points_new_.insert(
        (*intensity_new_->unburnable_)[p0.hash()],
        p0);
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
  for (const auto hash_value : points_new_.keys())
  {
    const auto& location = cell(hash_value);
    // const auto& location = kv.first;
    // would be burned already if perimeter applied
    // if (hasNotBurned(hash_value))
    // if (!isUnburnable(hash_value))
    if (intensity_new_->canBurn(hash_value))
    {
      const auto fake_event = Event::makeFireSpread(
        start_time_,
        location);
      intensity_new_->burn(fake_event);
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
  if (cancelled_)
  {
    return nullptr;
  }
  const auto completed = ++COMPLETED;
  const auto log_level = (0 == (completed % 1000)) ? logging::LOG_NOTE : logging::LOG_INFO;
  {
#ifdef NDEBUG
    // HACK: use + to pull value out of atomic
    const auto count = (+COUNT);
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
void apply_offsets_spreadkey(
  PtMap& points_new,
  spreading_points_new::mapped_type& pts_spreading_new,
  const Scenario& scenario,
  const DurationSize& arrival_time,
  const DurationSize& duration,
  const OffsetSet& offsets)
{
  // NOTE: really tried to do this in parallel, but not enough points
  // in a cell for it to work well
  // vector<XYPos> r1{};
  OffsetSet offsets_after_duration{};
  // // offsets_after_duration.resize(offsets.size());
  // std::transform(
  //   offsets.cbegin(),
  //   offsets.cend(),
  //   std::back_inserter(offsets_after_duration),
  offsets_after_duration.resize(offsets.size());
  std::transform(
    offsets.cbegin(),
    offsets.cend(),
    offsets_after_duration.begin(),
    [&duration, &arrival_time](const ROSOffset& r_p) {
      const auto& p = std::get<0>(r_p);
      // logging::verbose("ros %f; x %f; y %f; duration %f;",
      //               ros,
      //               p.x(),
      //               p.y(),
      //               duration);
      return ROSOffset(
        Offset(p.x() * duration, p.y() * duration));
    });
  size_t i = 0;
  for (auto& kv : pts_spreading_new)
  {
    ++i;
    auto& cell_pts = kv.second;
    // auto cell_pts_new = points_new.map_.at(hash_value);
    if (cell_pts.empty())
    {
      continue;
    }
    auto& pts = cell_pts;
    for (const auto& pt : pts)
    {
      for (const ROSOffset& r_p : offsets_after_duration)
      {
        const auto& out = std::get<0>(r_p);
        const auto& x_o = out.x();
        const auto& y_o = out.y();
        {
          const auto new_x = x_o + pt.x();
          const auto new_y = y_o + pt.y();
          const XYPos p0{new_x, new_y};
          points_new.insert(
            (*scenario.intensity_new_->unburnable_)[p0.hash()],
            p0);
        }
      }
    }
  }
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
  const auto wx =
    weather(time);
  const auto wx_daily =
    weather_daily(time);
  // note("time is %f", time);
  current_time_ = time;
  logging::check_fatal(nullptr == wx, "No weather available for time %f", time);
  //  log_note("%d points", points_->size());
  const auto next_time = static_cast<DurationSize>(this_time + 1) / DAY_HOURS;
  // should be in minutes?
  const auto max_duration = (next_time - time) * DAY_MINUTES;
  // log_verbose("time is %f, next_time is %f, max_duration is %f",
  //      time,
  //      next_time,
  //      max_duration);
  const auto max_time = time + max_duration / DAY_MINUTES;
  // if (wx->ffmc().asValue() < minimumFfmcForSpread(time))
  // HACK: use the old ffmc for this check to be consistent with previous version
  if (wx_daily->ffmc().asValue() < minimumFfmcForSpread(time))
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
    {
      spread_info_ = {};
    }
    max_ros_ = 0.0;
  }
  // get once and keep
  const auto ros_min = Settings::minimumRos();
  spreading_points_new to_spread_new{};
  // make block to prevent it being visible beyond use
  {
    // if we use an iterator this way we don't need to copy keys to erase things
    auto it = points_new_.map_.begin();
    while (it != points_new_.map_.end())
    {
      const HashSize& hash_value = it->first;
      const Location loc{hash_value};
      const auto& for_cell = cell(hash_value);
      const auto key = for_cell.key();
      ROSSize ros = 0;
      // HACK: need to lookup before emplace since might try to create Cell without fuel
      // if (!fuel::is_null_fuel(loc))
      // const auto h = for_cell.hash();
      // if (!(cannotSpread(hash_value)))
      // if (hasNotBurned(hash_value))
      // if (!isUnburnable(hash_value))
      {
        // const SpreadInfo tmp{*this, time, key, nd(time), wx};
        {
          const auto& origin_inserted = spread_info_.try_emplace(key, *this, time, key, nd(time), wx);
          // any cell that has the same fuel, slope, and aspect has the same spread
          const auto& origin = origin_inserted.first->second;
          // filter out things not spreading fast enough here so they get copied if they aren't
          // isNotSpreading() had better be true if ros is lower than minimum
          ros = origin.headRos();
        }
        if (ros >= ros_min)
        {
          max_ros_ = max(max_ros_, ros);
          // NOTE: shouldn't be Cell if we're looking up by just Location later
          to_spread_new[key].emplace_back(hash_value, it->second.unique());
          it = points_new_.map_.erase(it);
        }
        else
        {
          ++it;
        }
      }
    }
  }
  // if nothing in to_spread then nothing is spreading
  if (to_spread_new.empty())
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
  PtMap cell_pts_new{};
  for (auto& kv0 : to_spread_new)
  {
    auto& key = kv0.first;
    const auto& offsets = spread_info_[key].offsets();
    spreading_points_new::mapped_type& pts_spreading_new = kv0.second;
    // FIX: just decomposing for now
    // auto r =
    apply_offsets_spreadkey(
      cell_pts_new,
      pts_spreading_new,
      *this,
      new_time,
      duration,
      offsets);
    // for (XYPos& p : r)
    // {
    //   // cell_pts.insert(p.x(), p.y());
    //   points_.insert(p.x(), p.y());
    // }
  }
  // check after inserting new points since cells that didn't spread could be surrounded now
  for (auto& p : points_new_.unique())
  {
    cell_pts_new.insert((*intensity_new_->unburnable_)[p.hash()], p);
  }
  points_new_ = cell_pts_new;
  // if we move everything out of points_ we can parallelize this check?
  {
    // indent to keep keys local
    auto keys = points_new_.keys();
    for (auto hash_value : keys)
    {
      if ((*intensity_new_->unburnable_)[hash_value] || intensity_new_->isSurrounded(hash_value))
      {
        points_new_.erase(hash_value);
      }
    }
  }
  auto it = points_new_.map_.begin();
  while (it != points_new_.map_.end())
  {
    auto& kv = *it;
    // CellPoints& pts = kv.second;
    const auto hash_value = kv.first;
    const auto for_cell = cell(hash_value);
#ifdef DEBUG_TEMPORARY
    const auto h_target = Location{2081, 1962}.hash();
    if (h_target == hash_value)
    {
      logging::info("Looking at cell (%d, %d)", kv.second.cell_x_y_.x(), kv.second.cell_x_y_.y());
    }
#endif
    // logging::check_fatal(pts.empty(), "Empty points for some reason");
    // ******************* CHECK THIS BECAUSE IF SOMETHING IS IN HERE SHOULD IT ALWAYS HAVE SPREAD????? *****************8
    const auto& seek_spread = spread_info_.find(for_cell.key());
    const auto max_intensity = (spread_info_.end() == seek_spread) ? 0 : seek_spread->second.maxIntensity();
    // // if we don't have empty cells anymore then intensity should always be >0?
    // logging::check_fatal(max_intensity <= 0,
    //                      "Expected max_intensity to be > 0 but got %f",
    //                      max_intensity);
    // HACK: just use side-effect to log and check bounds
    if (
      intensity_new_->hasNotBurned(hash_value)
      // !isUnburnable(hash_value)
      && max_intensity > 0)
    {
      // // HACK: make sure it can't round down to 0
      // const auto intensity = static_cast<IntensitySize>(max(
      //   1.0,
      //   max_intensity));
      // HACK: just use the first cell as the source
      // FIX: HACK: only output spread within for now
      // const auto& spread = pts.spread_internal_;
      const auto fake_event = Event::makeFireSpread(
        new_time,
        for_cell);
      intensity_new_->burn(fake_event);
    }
    if (!(intensity_new_->cannotSpread(hash_value))
        // && hasNotBurned(for_cell)
        && ((survives(new_time, for_cell, new_time - intensity_new_->arrival_[hash_value])
             && !intensity_new_->isSurrounded(hash_value))))
    {
      // keep points because they could go somewhere
      ++it;
    }
    else
    {
      // just inserted false, so make sure unburnable gets updated
      // whether it went out or is surrounded just mark it as unburnable
      // ++it;
      it = points_new_.map_.erase(it);
      (*intensity_new_->unburnable_)[hash_value] = true;
      // not swapping means these points get dropped
    }
  }
  addEvent(Event::makeFireSpread(new_time));
}
MathSize
  Scenario::currentFireSize() const
{
  return intensity_new_->fireSize();
}
// bool Scenario::canBurn(const HashSize hash_value) const
// {
//   return intensity_->canBurn(hash_value);
//   // return !isUnburnable(hash_value);
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
vector<DurationSize> Scenario::savePoints() const
{
  return save_points_;
}
template <class V>
void Scenario::addSave(V time)
{
  last_save_ = max(last_save_, static_cast<DurationSize>(time));
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
