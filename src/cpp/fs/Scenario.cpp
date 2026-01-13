/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include "Scenario.h"
#include "Cell.h"
#include "CellPoints.h"
#include "FuelLookup.h"
#include "FuelType.h"
#include "IntensityMap.h"
#include "Location.h"
#include "MergeIterator.h"
#include "Observer.h"
#include "Perimeter.h"
#include "ProbabilityMap.h"
namespace fs
{
using std::cout;
constexpr auto CELL_CENTER = static_cast<InnerSize>(0.5);
// constexpr auto PRECISION = static_cast<MathSize>(0.001);
static atomic<size_t> COUNT = 0;
static atomic<size_t> COMPLETED = 0;
static atomic<size_t> TOTAL_STEPS = 0;
static std::mutex MUTEX_SIM_COUNTS;
static map<size_t, size_t> SIM_COUNTS{};
template <typename T, typename F>
void do_each(T& for_list, F fct)
{
  std::for_each(
#ifndef __APPLE__
    // apple clang doesn't support this?
    std::execution::seq,
#endif
    for_list.begin(),
    for_list.end(),
    fct
  );
}
template <typename T, typename F>
void do_par(T& for_list, F fct)
{
  std::for_each(
#ifndef __APPLE__
    // apple clang doesn't support this?
    std::execution::par_unseq,
#endif
    for_list.begin(),
    for_list.end(),
    fct
  );
}
void IObserver_deleter::operator()(IObserver* ptr) const
{
  // printf("Deleting\n");
  delete ptr;
}
void Scenario::clear() noexcept
{
  unburnable_.clear();
  scheduler_ = set<Event>();
  arrival_ = {};
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
  step_ = 0;
  oob_spread_ = 0;
}
size_t Scenario::completed() noexcept { return COMPLETED; }
size_t Scenario::count() noexcept { return COUNT; }
size_t Scenario::total_steps() noexcept { return TOTAL_STEPS; }
Scenario::~Scenario() { clear(); }
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
static void make_threshold(
  vector<ThresholdSize>* thresholds,
  mt19937* mt,
  const Day start_day,
  const Day last_date,
  ThresholdSize (*convert)(double value)
)
{
  const auto total_weight = Settings::thresholdScenarioWeight() + Settings::thresholdDailyWeight()
                          + Settings::thresholdHourlyWeight();
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
        thresholds->at((i - start_day) * DAY_HOURS + h) = convert(max(
          0.0,
          min(
            1.0,
            1.0
              - (Settings::thresholdScenarioWeight() * general
                 + Settings::thresholdDailyWeight() * daily
                 + Settings::thresholdHourlyWeight() * hourly)
                  / total_weight
          )
        ));
      }
    }
  }
}
template <class V>
constexpr V same(const V value) noexcept
{
  return value;
}
static void make_threshold(
  vector<ThresholdSize>* thresholds,
  mt19937* mt,
  const Day start_day,
  const Day last_date
)
{
  make_threshold(thresholds, mt, start_day, last_date, &same);
}
Scenario::Scenario(
  Model* model,
  const size_t id,
  const ptr<const FireWeather> weather,
  const DurationSize start_time,
  const shared_ptr<Perimeter>& perimeter,
  const StartPoint& start_point,
  const Day start_day,
  const Day last_date
)
  : Scenario(model, id, weather, weather, start_time, perimeter, start_point, start_day, last_date)
{ }
Scenario::Scenario(
  Model* model,
  const size_t id,
  ptr<const FireWeather> weather,
  const DurationSize start_time,
  const shared_ptr<Cell>& start_cell,
  const StartPoint& start_point,
  const Day start_day,
  const Day last_date
)
  : Scenario(model, id, weather, weather, start_time, start_cell, start_point, start_day, last_date)
{ }
Scenario::Scenario(
  Model* model,
  const size_t id,
  ptr<const FireWeather> weather,
  ptr<const FireWeather> weather_daily,
  const DurationSize start_time,
  const shared_ptr<Perimeter>& perimeter,
  const StartPoint& start_point,
  const Day start_day,
  const Day last_date
)
  : Scenario(
      model,
      id,
      weather,
      weather_daily,
      start_time,
      perimeter,
      nullptr,
      start_point,
      start_day,
      last_date
    )
{ }
Scenario::Scenario(
  Model* model,
  const size_t id,
  const ptr<const FireWeather> weather,
  const ptr<const FireWeather> weather_daily,
  const DurationSize start_time,
  const shared_ptr<Cell>& start_cell,
  const StartPoint& start_point,
  const Day start_day,
  const Day last_date
)
  : Scenario(
      model,
      id,
      weather,
      weather_daily,
      start_time,
      // make_unique<IntensityMap>(*model, nullptr),
      nullptr,
      start_cell,
      start_point,
      start_day,
      last_date
    )
{ }
// HACK: just set next start point here for surface right now
Scenario* Scenario::reset_with_new_start(
  const shared_ptr<Cell>& start_cell,
  ptr<SafeVector> final_sizes
)
{
  unburnable_.clear();
  start_cell_ = start_cell;
  cancelled_ = false;
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
  {
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
Scenario* Scenario::reset(mt19937* mt_extinction, mt19937* mt_spread, ptr<SafeVector> final_sizes)
{
  unburnable_.clear();
  cancelled_ = false;
  current_time_ = start_time_;
  intensity_ = nullptr;
  probabilities_ = nullptr;
  final_sizes_ = final_sizes;
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
    make_threshold(
      &spread_thresholds_by_ros_,
      mt_spread,
      start_day_,
      last_date_,
      &SpreadInfo::calculateRosFromThreshold
    );
  }
  for (const auto& o : observers_)
  {
    o->reset();
  }
  current_time_ = start_time_ - 1;
  points_ = {};
  intensity_ = make_unique<IntensityMap>(model());
  spread_info_ = {};
  arrival_ = {};
  max_ros_ = 0;
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
  log_check_fatal(
    event.time < current_time_, "Expected time to be > %f but got %f", current_time_, event.time
  );
#endif
  const auto& p = event.cell;
  const auto x = p.column() + CELL_CENTER;
  const auto y = p.row() + CELL_CENTER;
  const XYPos p0{x, y};
  switch (event.type)
  {
    case Event::Type::FireSpread:
      ++step_;
#ifdef DEBUG_POINTS
      {
        const auto ymd = fs::make_timestamp(model().year(), event.time);
      }
#endif
      scheduleFireSpread(event);
      break;
    case Event::Type::Save:
      std::ignore = saveObservers(model_->outputDirectory(), event.time);
      saveStats(event.time);
      break;
    case Event::Type::NewFire:
      points_log_.log(
        step_, STAGE_NEW, event.time, p.column() + CELL_CENTER, p.row() + CELL_CENTER
      );
      // HACK: don't do this in constructor because scenario creates this in its constructor
      // HACK: insert point as originating from itself
      points_.insert(
        p0,
        SpreadData(event.time, NO_INTENSITY, NO_ROS, Direction::Invalid(), Direction::Invalid()),
        x,
        y
      );
      if (is_null_fuel(event.cell))
      {
        log_fatal("Trying to start a fire in non-fuel");
      }
      log_verbose(
        "Starting fire at point (%f, %f) in fuel type %s at time %f",
        x,
        y,
        FuelType::safeName(check_fuel(event.cell)),
        event.time
      );
      if (!survives(event.time, event.cell, event.time_at_location))
      {
        // HACK: show daily values since that's what survival uses
        const auto wx = weather_daily(event.time);
        log_info(
          "Didn't survive ignition in %s with weather %f, %f",
          FuelType::safeName(check_fuel(event.cell)),
          wx->ffmc,
          wx->dmc
        );
        // HACK: we still want the fire to have existed, so set the intensity of the origin
      }
      // fires start with intensity of 1
      burn(event);
      scheduleFireSpread(event);
      break;
    case Event::Type::EndSimulation:
      log_verbose("End simulation event reached at %f", event.time);
      endSimulation();
      break;
    default:
      throw runtime_error("Invalid event type");
  }
}
Scenario::Scenario(
  Model* model,
  const size_t id,
  const ptr<const FireWeather> weather,
  const ptr<const FireWeather> weather_daily,
  const DurationSize start_time,
  const shared_ptr<Perimeter>& perimeter,
  const shared_ptr<Cell>& start_cell,
  StartPoint start_point,
  const Day start_day,
  const Day last_date
)
  : current_time_(start_time), unburnable_{}, intensity_(nullptr), perimeter_(perimeter),
    max_ros_(0), start_cell_(start_cell), weather_(weather), weather_daily_(weather_daily),
    model_(model), probabilities_(nullptr), final_sizes_(nullptr),
    start_point_(std::move(start_point)), id_(id), start_time_(start_time),
    last_save_(weather_->minDate()), simulation_(-1), start_day_(start_day), last_date_(last_date),
    ran_(false), step_(0),
    points_log_(LogPoints{model_->outputDirectory(), Settings::savePoints(), id_, start_time_})
{
  const auto wx = weather_->at(start_time_);
  logging::check_fatal(
    nullptr == wx,
    "No weather for start time %s",
    make_timestamp(model->year(), start_time_).c_str()
  );
  const auto saves = Settings::outputDateOffsets();
  const auto last_save = start_day_ + saves[saves.size() - 1];
  logging::check_fatal(
    last_save > weather_->maxDate(),
    "No weather for last save time %s",
    make_timestamp(model->year(), last_save).c_str()
  );
}
void Scenario::saveStats(const DurationSize time) const
{
  size_t i = 0;
  while (i < save_points_.size() && save_points_[i] < time)
  {
    ++i;
  }
  logging::check_fatal(i >= save_points_.size(), "Invalid save time %f is out of bounds", time);
  probabilities_->at(i)->addProbability(*intensity_);
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
FileList Scenario::saveObservers(const string_view output_directory, const string_view base_name)
  const
{
  FileList files{};
  for (const auto& o : observers_)
  {
    files.append_range(o->save(output_directory, base_name));
  }
  return files;
}
FileList Scenario::saveObservers(const string_view output_directory, const DurationSize time) const
{
  char buffer[64]{0};
  sxprintf(buffer, "%03zu_%06ld_%03d", id(), simulation(), static_cast<int>(time));
  return saveObservers(output_directory, string(buffer));
}
FileList Scenario::saveIntensity(const string_view output_directory, const string_view base_name)
  const
{
  return intensity_->save(output_directory, base_name);
}
bool Scenario::ran() const noexcept { return ran_; }
Scenario::Scenario(Scenario&& rhs) noexcept
  : observers_(std::move(rhs.observers_)), save_points_(std::move(rhs.save_points_)),
    extinction_thresholds_(std::move(rhs.extinction_thresholds_)),
    spread_thresholds_by_ros_(std::move(rhs.spread_thresholds_by_ros_)),
    current_time_(rhs.current_time_), points_(std::move(rhs.points_)),
    unburnable_(std::move(rhs.unburnable_)), scheduler_(std::move(rhs.scheduler_)),
    intensity_(std::move(rhs.intensity_)), perimeter_(std::move(rhs.perimeter_)),
    spread_info_(std::move(rhs.spread_info_)), arrival_(std::move(rhs.arrival_)),
    max_ros_(rhs.max_ros_), start_cell_(std::move(rhs.start_cell_)), weather_(rhs.weather_),
    weather_daily_(rhs.weather_daily_), model_(rhs.model_), probabilities_(rhs.probabilities_),
    final_sizes_(rhs.final_sizes_), start_point_(std::move(rhs.start_point_)), id_(rhs.id_),
    start_time_(rhs.start_time_), last_save_(rhs.last_save_), simulation_(rhs.simulation_),
    start_day_(rhs.start_day_), last_date_(rhs.last_date_), ran_(rhs.ran_)
{ }
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
    perimeter_ = std::move(rhs.perimeter_);
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
void Scenario::burn(const Event& event)
{
#ifdef DEBUG_SIMULATION
  log_check_fatal(
    intensity_->hasBurned(event.cell),
    "Re-burning cell (%d, %d)",
    event.cell.column(),
    event.cell.row()
  );
#endif
  // #ifdef DEBUG_POINTS
  //   log_check_fatal(
  //     (*unburnable_)[event.cell.hash()],
  //     "Burning unburnable cell (%d, %d)",
  //     event.cell.column(),
  //     event.cell.row()
  //   );
  // #endif
  //  Observers only care about cells burning so do it here
  notify(event);
  intensity_->burn(event.cell, event.intensity, event.ros, event.raz);
#ifdef DEBUG_GRIDS
  log_check_fatal(!intensity_->hasBurned(event.cell), "Wasn't marked as burned after burn");
#endif
  arrival_[event.cell] = event.time;
}
bool Scenario::isSurrounded(const Location& location) const
{
  return intensity_->isSurrounded(location);
}
Cell Scenario::cell(const InnerPos& p) const noexcept { return cell(p.second, p.first); }
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
void saveProbabilities(
  const string_view dir,
  const string_view base_name,
  vector<ThresholdSize>& thresholds
)
{
  ofstream out{string(dir) + string(base_name) + ".csv"};
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
  unburnable_ = model_->environment().unburnable();
  probabilities_ = probabilities;
  log_verbose("Setting save points");
  for (auto time : save_points_)
  {
    // NOTE: these happen in this order because of the way they sort based on type
    addEvent(Event{.time = time, .type = Event::Type::Save});
  }
  if (nullptr == perimeter_)
  {
    addEvent(Event{.time = start_time_, .type = Event::Type::NewFire, .cell = cell(*start_cell_)});
  }
  else
  {
    log_verbose("Applying perimeter");
    intensity_->applyPerimeter(*perimeter_);
    log_verbose("Perimeter applied");
    const auto& env = model().environment();
    log_verbose("Igniting points");
    for (const auto& location : perimeter_->edge)
    {
      const auto cell = env.cell(location);
#ifdef DEBUG_SIMULATION
      log_check_fatal(is_null_fuel(cell), "Null fuel in perimeter");
#endif
      const auto x = cell.column() + CELL_CENTER;
      const auto y = cell.row() + CELL_CENTER;
      const XYPos p0{x, y};
      // log_verbose("Adding point (%d, %d)",
      log_verbose("Adding point (%f, %f)", x, y);
      points_.insert(
        p0,
        SpreadData(start_time_, NO_INTENSITY, NO_ROS, Direction::Invalid(), Direction::Invalid()),
        x,
        y
      );
    }
    addEvent(Event{.time = start_time_, .type = Event::Type::FireSpread});
  }
  // HACK: make a copy of the event so that it still exists after it gets processed
  // NOTE: sorted so that EventSaveASCII is always just before this
  // Only run until last time we asked for a save for
  log_verbose("Creating simulation end event for %f", last_save_);
  addEvent(Event{.time = last_save_, .type = Event::Type::EndSimulation});
  // mark all original points as burned at start
  for (auto& kv : points_.map_)
  {
    const auto& location = cell(kv.first);
    // would be burned already if perimeter applied
    if (canBurn(location))
    {
      const Event fake_event{.time = start_time_, .cell = location, .ros = 0.0};
      burn(fake_event);
    }
  }
  while (!cancelled_ && !scheduler_.empty())
  {
    evaluateNextEvent();
  }
  ++TOTAL_STEPS;
  unburnable_.clear();
  if (cancelled_)
  {
    return nullptr;
  }
  const auto completed = ++COMPLETED;
  // HACK: use + to pull value out of atomic
  const auto count = Settings::surface() ? model_->scenarioCount() : (+COUNT);
  const auto log_level = (0 == (completed % 1000)) ? logging::LOG_NOTE : logging::LOG_INFO;
  if (Settings::surface())
  {
    const auto ratio_done = static_cast<MathSize>(completed) / count;
    const auto s = model_->runTime().count();
    const auto r = static_cast<size_t>(s / ratio_done) - s;
    log_output(
      log_level,
      "[% d of % d] (%0.2f%%) <%lds : %lds remaining> Completed with final size % 0.1f ha",
      completed,
      count,
      100 * ratio_done,
      s,
      r,
      currentFireSize()
    );
  }
  else
  {
#ifdef NDEBUG
    log_output(
      log_level,
      "[% d of % d] Completed with final size % 0.1f ha",
      completed,
      count,
      currentFireSize()
    );
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
    sxprintf(buffer, "%03zu_%06ld_extinction", id(), simulation());
    saveProbabilities(model().outputDirectory(), string(buffer), extinction_thresholds_);
    sxprintf(buffer, "%03zu_%06ld_spread", id(), simulation());
    saveProbabilities(model().outputDirectory(), string(buffer), spread_thresholds_by_ros_);
  }
#endif
  if (oob_spread_ > 0)
  {
    log_warning("Tried to spread out of bounds %ld times", oob_spread_);
  }
  return this;
}
CellPointsMap apply_offsets_spreadkey(
  const DurationSize& arrival_time,
  const DurationSize& duration,
  const OffsetSet& offsets,
  spreading_points::mapped_type& cell_pts_map
)
{
  // NOTE: really tried to do this in parallel, but not enough points
  // in a cell for it to work well
  CellPointsMap r1{};
  OffsetSet offsets_after_duration{};
  logging::verbose("Applying %ld offsets", offsets.size());
  // std::transform(
  //   offsets.cbegin(),
  //   offsets.cend(),
  //   std::back_inserter(offsets_after_duration),
  offsets_after_duration.resize(offsets.size());
  std::transform(
    offsets.cbegin(),
    offsets.cend(),
    offsets_after_duration.begin(),
    [&](const ROSOffset& r_p) {
      const auto& intensity = std::get<0>(r_p);
      const auto& ros = std::get<1>(r_p);
      const auto& raz = std::get<2>(r_p);
      const auto& p = std::get<3>(r_p);
      return ROSOffset(intensity, ros, raz, Offset(p.first * duration, p.second * duration));
    }
  );
  logging::verbose(
    "Calculated %ld offsets after duration %f", offsets_after_duration.size(), duration
  );
  logging::verbose("cell_pts_map has %ld items", cell_pts_map.size());
  for (auto& pts_for_cell : cell_pts_map)
  {
    const Location& location = std::get<0>(pts_for_cell);
    CellPoints& cell_pts = std::get<1>(pts_for_cell);
#ifdef DEBUG_CELLPOINTS
    logging::note("cell_pts for (%d, %d) has %ld items", src.column(), src.row(), cell_pts.size());
#endif
    if (cell_pts.empty())
    {
#ifdef DEBUG_CELLPOINTS
      logging::note("Cell (%d, %d) ignored because empty", src.column(), src.row());
#endif
      continue;
    }
    auto& pts = cell_pts.pts_.points();
    auto& dirs = cell_pts.pts_.directions();
    std::array<std::pair<InnerPos, MathSize>, NUM_DIRECTIONS> pt_dirs{};
    for (size_t i = 0; i < pts.size(); ++i)
    {
      pt_dirs[i] = {pts[i], dirs[i]};
    }
    std::sort(pt_dirs.begin(), pt_dirs.end());
    const auto it_pt_dirs_last = std::unique(pt_dirs.begin(), pt_dirs.end());
    auto it_pt_dirs = pt_dirs.cbegin();
    while (it_pt_dirs != it_pt_dirs_last)
    {
      const auto& pt_dir = *it_pt_dirs;
      const auto& pt = std::get<0>(pt_dir);
      const auto& dir = std::get<1>(pt_dir);
      const auto& cell_x = cell_pts.cell_x_y_.first;
      const auto& cell_y = cell_pts.cell_x_y_.second;
      const XYPos src{pt.first + cell_x, pt.second + cell_y};
      // apply offsets to point
      // should be quicker to loop over offsets in inner loop
      for (const ROSOffset& r_p : offsets_after_duration)
      {
        const auto& intensity = std::get<0>(r_p);
        const auto& ros = std::get<1>(r_p);
        const auto& raz = std::get<2>(r_p);
        const auto& out = std::get<3>(r_p);
        const auto& x_o = out.first;
        const auto& y_o = out.second;
        const auto dir_diff = abs(raz.asDegrees() - dir);
        // #ifdef DEBUG_CELLPOINTS
        logging::verbose(
          "location.x %d; location.y %d;"
          "cell_x %d; cell_y %d;"
          " ros %f; x %f; y %f; duration %f;\n",
          location.column(),
          location.row(),
          cell_x,
          cell_y,
          ros,
          pt.first,
          pt.second,
          duration
        );
        // // NOTE: there should be no change in the extent of the fire if we exclude things behind
        // the normal to the direction it came from
        // //       - but if we exclude too much then it can change how things spread, even if it is
        // a more representative angle for the grids if (is_initial || MAX_DEGREES >= dir_diff)
        {
          const auto new_x = x_o + pt.first + cell_x;
          const auto new_y = y_o + pt.second + cell_y;
          logging::verbose(
            "(%d, %d): %f: [%f => (%f, %f)] + [%f => (%f, %f)] = [%f => (%f, %f)]",
            cell_x,
            cell_y,
            dir_diff,
            dir,
            x_o,
            y_o,
            raz.asDegrees(),
            pt.first,
            pt.second,
            raz.asDegrees(),
            new_x,
            new_y
          );
          r1.insert(
            src,
            SpreadData(arrival_time, intensity, ros, raz, Direction(Degrees{dir})),
            new_x,
            new_y
          );
#ifdef DEBUG_CELLPOINTS
          logging::note("r1 is now %ld items", r1.size());
#endif
        }
      }
      ++it_pt_dirs;
    }
  }
  return r1;
}
void Scenario::scheduleFireSpread(const Event& event)
{
  const auto time = event.time;
  const auto this_time = time_index(time);
  const auto wx = Settings::surface() ? model_->yesterday() : weather(time);
  const auto wx_daily = Settings::surface() ? model_->yesterday() : weather_daily(time);
  current_time_ = time;
  logging::check_fatal(nullptr == wx, "No weather available for time %f", time);
  const auto next_time = static_cast<DurationSize>(this_time + 1) / DAY_HOURS;
  // should be in minutes?
  const auto max_duration = (next_time - time) * DAY_MINUTES;
  const auto max_time = time + max_duration / DAY_MINUTES;
  // HACK: use the old ffmc for this check to be consistent with previous version
  if (wx_daily->ffmc.value < minimumFfmcForSpread(time))
  {
    addEvent(Event{.time = max_time, .type = Event::Type::FireSpread});
    log_extensive("Waiting until %f because of FFMC", max_time);
    return;
  }
  if (current_time_index_ != this_time)
  {
    current_time_index_ = this_time;
    // seemed like it would be good to keep offsets but max_ros_ needs to reset or things slow to a
    // crawl?
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
      const auto key = for_cell.key();
      {
        const auto& origin_inserted = spread_info_.try_emplace(key, *this, time, key, nd(time), wx);
        // any cell that has the same fuel, slope, and aspect has the same spread
        const auto& origin = origin_inserted.first->second;
        // filter out things not spreading fast enough here so they get copied if they aren't
        // isNotSpreading() had better be true if ros is lower than minimum
        const auto ros = origin.headRos();
        if (ros >= ros_min)
        {
          max_ros_ = max(max_ros_, ros);
          // NOTE: shouldn't be Cell if we're looking up by just Location later
          to_spread[key].emplace_back(loc, std::move(it->second));
          it = points_.map_.erase(it);
#ifdef DEBUG_CELLPOINTS
          auto& v = to_spread[key];
          const auto n = v.size();
          const auto& p = v[n - 1].second;
          logging::note(
            "added %ld items to to_spread[%d][(%d, %d)]", p.size(), key, loc.column(), loc.row()
          );
#endif
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
    addEvent(Event{.time = max_time, .type = Event::Type::FireSpread});
    return;
  }
  const auto duration =
    ((max_ros_ > 0) ? min(max_duration, Settings::maximumSpreadDistance() * cellSize() / max_ros_)
                    : max_duration);
  const auto new_time = time + duration / DAY_MINUTES;
  CellPointsMap cell_pts{};
  auto spread =
    std::views::transform(to_spread, [&](spreading_points::value_type& kv0) -> CellPointsMap {
      auto& key = kv0.first;
      const auto& offsets = spread_info_[key].offsets();
      spreading_points::mapped_type& cell_pts = kv0.second;
      auto r = apply_offsets_spreadkey(new_time, duration, offsets, cell_pts);
      return r;
    });
  auto it = spread.begin();
  while (spread.end() != it)
  {
    const CellPointsMap& cell_pts_cur = *it;
    // // HACK: keep old behaviour until we can figure out whey removing isn't the same as not
    // adding const auto h = cell_pts.location().hash(); if (!unburnable[h])
    // {
    cell_pts.merge(unburnable_, cell_pts_cur);
    ++it;
  }
#ifdef DEBUG_CELLPOINTS
  const auto n_c = cell_pts.size();
#endif
  cell_pts.remove_if([this](const pair<Location, CellPoints>& kv) {
    const auto& location = kv.first;
    const auto h = location.hash();
    // clear out if unburnable
    const auto do_clear = unburnable_.at(h);
    return do_clear;
  });
#ifdef DEBUG_CELLPOINTS
  logging::note("%ld cell_pts before remove_if() and %ld after", n_c, cell_pts.size());
#endif
  // need to merge new points back into cells that didn't spread
  points_.merge(unburnable_, cell_pts);
  // if we move everything out of points_ we can parallelize this check?
  do_each(points_.map_, [&](pair<const Location, CellPoints>& kv) {
    const auto for_cell = cell(kv.first);
    CellPoints& pts = kv.second;
    // ******************* CHECK THIS BECAUSE IF SOMETHING IS IN HERE SHOULD IT ALWAYS HAVE
    // SPREAD????? *****************8
    const auto& seek_spread = spread_info_.find(for_cell.key());
    const auto max_intensity =
      (spread_info_.end() == seek_spread) ? 0 : seek_spread->second.maxIntensity();
    // HACK: just use side-effect to log and check bounds
    points_log_.log(step_, STAGE_SPREAD, new_time, pts);
    if (canBurn(for_cell) && max_intensity > 0)
    {
      const auto& spread = pts.spread_arrival_;
      const Event fake_event{
        .time = new_time,
        .cell = for_cell,
        .ros = spread.ros(),
        .intensity = spread.intensity(),
        .raz = spread.direction(),
        .source = pts.sources()
      };
      burn(fake_event);
    }
    if (!unburnable_.at(for_cell.hash())
        // && canBurn(for_cell)
        && ((survives(new_time, for_cell, new_time - arrival_[for_cell]) && !isSurrounded(for_cell))
        ))
    {
      points_log_.log(step_, STAGE_CONDENSE, new_time, pts);
      const auto r = for_cell.row();
      const auto c = for_cell.column();
      const Location loc{r, c};
      std::swap(points_.map_[loc], pts);
    }
    else
    {
      // just inserted false, so make sure unburnable gets updated
      // whether it went out or is surrounded just mark it as unburnable
      unburnable_.set(for_cell.hash());
      // not swapping means these points get dropped
    }
  });
  log_extensive("Spreading %d cells until %f", points_.map_.size(), new_time);
  addEvent(Event{.time = new_time, .type = Event::Type::FireSpread});
}
MathSize Scenario::currentFireSize() const { return intensity_->fireSize(); }
bool Scenario::canBurn(const Cell& location) const { return intensity_->canBurn(location); }
bool Scenario::hasBurned(const Location& location) const { return intensity_->hasBurned(location); }
void Scenario::endSimulation() noexcept
{
  log_verbose("Ending simulation");
  scheduler_ = set<Event>();
}
void Scenario::addSaveByOffset(const int offset)
{
  // offset is from beginning of the day the simulation starts
  // e.g. 1 is midnight, 2 is tomorrow at midnight
  addSave(static_cast<Day>(startTime()) + offset);
}
vector<DurationSize> Scenario::savePoints() const { return save_points_; }
void Scenario::addSave(const DurationSize time)
{
  last_save_ = max(last_save_, time);
  save_points_.push_back(time);
}
void Scenario::addEvent(Event&& event) { scheduler_.insert(std::move(event)); }
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
