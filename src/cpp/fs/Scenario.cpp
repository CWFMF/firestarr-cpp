/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include "Scenario.h"
#include "ConvexHull.h"
#include "FireSpread.h"
#include "Observer.h"
#include "Perimeter.h"
#include "ProbabilityMap.h"
namespace fs
{
using std::cout;
constexpr auto CELL_CENTER = 0.5;
constexpr auto PRECISION = 0.001;
static atomic<size_t> COUNT = 0;
static atomic<size_t> COMPLETED = 0;
static atomic<size_t> TOTAL_STEPS = 0;
static std::mutex MUTEX_SIM_COUNTS;
static map<size_t, size_t> SIM_COUNTS{};
template <class K, class V>
class MergeMap
{
public:
  constexpr MergeMap() { }
  MergeMap(const MergeMap<K, V>& rhs) : map_(std::copy(rhs.map_)) { }
  MergeMap(MergeMap<K, V>&& rhs) noexcept : map_(std::move(rhs.map_)) { }
  inline void merge_value(const K& key, const V& value)
  {
    std::lock_guard<mutex> lock(mutex_);
    merge_value(key, value);
  }
  inline void merge_value(const pair<const K, const V>& p) { merge_value(p.first, p.second); }
  template <class L>
  inline void merge_values(const K& key, const L& values)
  {
    std::lock_guard<mutex> lock(mutex_);
    merge_values_(key, values);
  }
  template <class L>
  inline void merge_values(const L& values)
  {
    std::lock_guard<mutex> lock(mutex_);
    std::for_each(
      // std::execution::par_unseq,
      values.begin(),
      values.end(),
      [this](const pair<const K, const V>& v) { merge_value_(v); }
    );
  }
  void merge(const MergeMap& rhs)
  {
    std::lock_guard<mutex> lock(mutex_);
    std::lock_guard<mutex> lock_rhs(rhs.mutex_);
    std::for_each(
      // std::execution::par_unseq,
      rhs.map_.begin(),
      rhs.map_.end(),
      [this](const pair<const K, const vector<V>>& kv) {
        merge_values_(std::get<0>(kv), std::get<1>(kv));
      }
    );
  }
  template <class F>
  void for_each(F fct)
  {
    std::lock_guard<mutex> lock(mutex_);
    std::for_each(map_.begin(), map_.end(), fct);
  }

private:
  map<const K, vector<V>> map_;
  // actual functions don't get a lock
  inline void merge_value_(const K& key, const V& value) { (map_)[key].emplace_back(value); }
  inline void merge_value_(const pair<const K, const V>& p) { merge_value_(p.first, p.second); }
  template <class L>
  inline void merge_values_(const K& key, const L& values)
  {
    std::for_each(
      // std::execution::par_unseq,
      values.begin(),
      values.end(),
      [this, &key](const V& v) { merge_value_(key, v); }
    );
  }
  mutable mutex mutex_;
};
using PointsMap = MergeMap<Cell, InnerPos>;
using SourcesMap = map<Cell, CellIndex>;
template <typename T, typename F>
void do_each(T& for_list, F fct)
{
  std::for_each(
    // std::execution::par_unseq,
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
  scheduler_ = set<Event, EventCompare>();
  arrival_ = {};
  points_ = {};
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
  vector<double>* thresholds,
  mt19937* mt,
  const Day start_day,
  const Day last_date,
  double (*convert)(double value)
)
{
  const auto total_weight = Settings::thresholdScenarioWeight() + Settings::thresholdDailyWeight()
                          + Settings::thresholdHourlyWeight();
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
constexpr double same(const double value) noexcept { return value; }
static void make_threshold(
  vector<double>* thresholds,
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
  max_ros_ = 0;
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
    event.time() < current_time_, "Expected time to be > %f but got %f", current_time_, event.time()
  );
#endif
  const auto& p = event.cell();
  switch (event.type())
  {
    case Event::FIRE_SPREAD:
      ++step_;
#ifdef DEBUG_POINTS
      {
        const auto ymd = fs::make_timestamp(model().year(), event.time());
        log_note(
          "Handling spread event for time %f representing %s with %ld points",
          event.time(),
          ymd.c_str(),
          points_.size()
        );
      }
#endif
      scheduleFireSpread(event);
      break;
    case Event::SAVE:
      std::ignore = saveObservers(model_->outputDirectory(), event.time());
      saveStats(event.time());
      break;
    case Event::NEW_FIRE:
      points_log_.log(
        step_, STAGE_NEW, event.time(), p.column() + CELL_CENTER, p.row() + CELL_CENTER
      );
      // HACK: don't do this in constructor because scenario creates this in its constructor
      points_[p].emplace_back(p.column() + CELL_CENTER, p.row() + CELL_CENTER);
      if (is_null_fuel(event.cell()))
      {
        log_fatal("Trying to start a fire in non-fuel");
      }
      log_verbose(
        "Starting fire at point (%f, %f) in fuel type %s at time %f",
        p.column() + CELL_CENTER,
        p.row() + CELL_CENTER,
        FuelType::safeName(check_fuel(event.cell())),
        event.time()
      );
      if (!survives(event.time(), event.cell(), event.timeAtLocation()))
      {
        // HACK: show daily values since that's what survival uses
        const auto wx = weather_daily(event.time());
        log_info(
          "Didn't survive ignition in %s with weather %f, %f",
          FuelType::safeName(check_fuel(event.cell())),
          wx->ffmc(),
          wx->dmc()
        );
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
  static const size_t BufferSize = 64;
  char buffer[BufferSize + 1] = {0};
  sprintf(buffer, "%03zu_%06ld_%03d", id(), simulation(), static_cast<int>(time));
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
void Scenario::burn(const Event& event, const IntensitySize)
{
#ifdef DEBUG_SIMULATION
  log_check_fatal(intensity_->hasBurned(event.cell()), "Re-burning cell");
#endif
  //  Observers only care about cells burning so do it here
  notify(event);
  // WIP: call burn without proper information for now so we can commit IntensityMap changes
  intensity_->burn(event.cell(), event.intensity(), 0, fs::Direction::Zero);
  arrival_[event.cell()] = event.time();
}
bool Scenario::isSurrounded(const Location& location) const
{
  return intensity_->isSurrounded(location);
}
Cell Scenario::cell(const InnerPos& p) const noexcept { return cell(p.y(), p.x()); }
string Scenario::add_log(const char* format) const noexcept
{
  const string tmp;
  stringstream iss(tmp);
  static char buffer[1024]{0};
  sprintf(buffer, "Scenario %4ld.%04ld (%3f): ", id(), simulation(), current_time_);
  iss << buffer << format;
  return iss.str();
}
#ifdef DEBUG_PROBABILITY
void saveProbabilities(
  const string_view dir,
  const string_view base_name,
  vector<double>& thresholds
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
Scenario* Scenario::run(map<double, ProbabilityMap*>* probabilities)
{
#ifdef DEBUG_SIMULATION
  log_check_fatal(ran(), "Scenario has already run");
#endif
  log_verbose("Starting");
  CriticalSection _(Model::task_limiter);
  unburnable_ = model_->environment().unburnable();
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
      const auto cell = env.cell(location);
#ifdef DEBUG_SIMULATION
      log_check_fatal(is_null_fuel(cell), "Null fuel in perimeter");
#endif
      log_verbose("Adding point (%d, %d)", cell.column() + CELL_CENTER, cell.row() + CELL_CENTER);
      points_[cell].emplace_back(cell.column() + CELL_CENTER, cell.row() + CELL_CENTER);
    }
    addEvent(Event::makeFireSpread(start_time_));
  }
  // HACK: make a copy of the event so that it still exists after it gets processed
  // NOTE: sorted so that EventSaveASCII is always just before this
  // Only run until last time we asked for a save for
  log_verbose("Creating simulation end event for %f", last_save_);
  addEvent(Event::makeEnd(last_save_));
  // mark all original points as burned at start
  for (auto& kv : points_)
  {
    const auto& location = kv.first;
    // would be burned already if perimeter applied
    if (canBurn(location))
    {
      const auto fake_event =
        Event::makeFireSpread(start_time_, static_cast<IntensitySize>(1), location);
      burn(fake_event, static_cast<IntensitySize>(1));
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
  ++COMPLETED;
  // HACK: use + to pull value out of atomic
#ifdef NDEBUG
  log_info(
    "[% d of % d] Completed with final size % 0.1f ha", +COMPLETED, +COUNT, currentFireSize()
  );
#else
  // try to make output consistent if in debug mode
  log_info("Completed with final size %0.1f ha", currentFireSize());
#endif
  ran_ = true;
#ifdef DEBUG_PROBABILITY
  // nice to have this get output when debugging, but only need it in extreme cases
  if (logging::Log::getLogLevel() <= logging::LOG_EXTENSIVE)
  {
    static const size_t BufferSize = 64;
    char buffer[BufferSize + 1] = {0};
    sprintf(buffer, "%03zu_%06ld_extinction", id(), simulation());
    saveProbabilities(model_->outputDirectory(), string(buffer), extinction_thresholds_);
    sprintf(buffer, "%03zu_%06ld_spread", id(), simulation());
    saveProbabilities(model_->outputDirectory(), string(buffer), spread_thresholds_by_ros_);
  }
#endif
  if (oob_spread_ > 0)
  {
    log_warning("Tried to spread out of bounds %ld times", oob_spread_);
  }
  return this;
}
/**
 * Determine the direction that a given cell is in from another cell. This is the
 * same convention as wind (i.e. the direction it is coming from, not the direction
 * it is going towards).
 * @param for_cell The cell to find directions relative to
 * @param from_cell The cell to find the direction of
 * @return Direction that you would have to go in to get to from_cell from for_cell
 */
CellIndex relativeIndex(const Cell& for_cell, const Cell& from_cell)
{
  const auto r = for_cell.row();
  const auto r_o = from_cell.row();
  const auto c = for_cell.column();
  const auto c_o = from_cell.column();
  if (r == r_o)
  {
    // center row
    // same cell, so source is 0
    if (c == c_o)
    {
      return DIRECTION_NONE;
    }
    if (c < c_o)
    {
      // center right
      return DIRECTION_E;
    }
    // else has to be c > c_o
    // center left
    return DIRECTION_W;
  }
  if (r < r_o)
  {
    // came from the row to the north
    if (c == c_o)
    {
      // center top
      return DIRECTION_N;
    }
    if (c < c_o)
    {
      // top right
      return DIRECTION_NE;
    }
    // else has to be c > c_o
    // top left
    return DIRECTION_NW;
  }
  // else r > r_o
  // came from the row to the south
  if (c == c_o)
  {
    // center bottom
    return DIRECTION_S;
  }
  if (c < c_o)
  {
    // bottom right
    return DIRECTION_SE;
  }
  // else has to be c > c_o
  // bottom left
  return DIRECTION_SW;
}
void Scenario::scheduleFireSpread(const Event& event)
{
  const auto time = event.time();
  current_time_ = time;
  const auto wx = weather(time);
  logging::check_fatal(nullptr == wx, "No weather available for time %f", time);
  const auto this_time = time_index(time);
  const auto next_time = static_cast<double>(this_time + 1) / DAY_HOURS;
  // should be in minutes?
  const auto max_duration = (next_time - time) * DAY_MINUTES;
  const auto max_time = time + max_duration / DAY_MINUTES;
  // HACK: use the old ffmc for this check to be consistent with previous version
  if (weather_daily(time)->ffmc().asValue() < minimumFfmcForSpread(time))
  {
    addEvent(Event::makeFireSpread(max_time));
    log_verbose("Waiting until %f because of FFMC", max_time);
    return;
  }
  if (current_time_index_ != this_time)
  {
    current_time_index_ = this_time;
    spread_info_ = {};
    max_ros_ = 0.0;
  }
  // get once and keep
  const auto ros_min = Settings::minimumRos();
  using CellPts = tuple<Cell, const PointSet>;
  map<SpreadKey, vector<CellPts>> to_spread{};
  // make block to prevent it being visible beyond use
  {
    // if we use an iterator this way we don't need to copy keys to erase things
    auto it = points_.begin();
    while (it != points_.end())
    {
      const auto location = it->first;
      const auto key = location.key();
      const auto& origin_inserted = spread_info_.try_emplace(key, *this, time, key, nd(time), wx);
      // any cell that has the same fuel, slope, and aspect has the same spread
      const auto& origin = origin_inserted.first->second;
      // filter out things not spreading fast enough here so they get copied if they aren't
      // isNotSpreading() had better be true if ros is lower than minimum
      const auto ros = origin.headRos();
      if (ros >= ros_min)
      {
        max_ros_ = max(max_ros_, ros);
        to_spread[key].emplace_back(location, std::move(it->second));
        it = points_.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }
  // if nothing in to_spread then nothing is spreading
  if (to_spread.empty())
  {
    // if no spread then we swapped everything back into points_ already
    log_verbose("Waiting until %f", max_time);
    addEvent(Event::makeFireSpread(max_time));
    return;
  }
  const auto duration =
    ((max_ros_ > 0) ? min(max_duration, Settings::maximumSpreadDistance() * cellSize() / max_ros_)
                    : max_duration);
  map<Cell, CellIndex> sources{};
  const auto new_time = time + duration / DAY_MINUTES;
  mutex mutex_spreading;
  map<Cell, bool> spreading{};
  auto can_spread = [this, &spreading, &mutex_spreading, &new_time](const Cell for_cell) {
    lock_guard<mutex> lock(mutex_spreading);
    return spreading.try_emplace(
      for_cell,
      !unburnable_.at(for_cell.hash())
        && ((survives(new_time, for_cell, new_time - arrival_[for_cell]) && !isSurrounded(for_cell))
        )
    );
  };
  // for (auto& location : std::vector<Cell>(cells_old.begin(), cells_old.end()))
  auto apply_offsets = [this, &new_time, &duration, &sources, &can_spread](
                         //  const OffsetSet offsets,
                         //  const Cell location,
                         //  const PointSet pts
                         const tuple<Cell, PointSet, const OffsetSet*>& t
                       ) {
    const auto& location = std::get<0>(t);
    const auto& pts = std::get<1>(t);
    const auto& offsets = *std::get<2>(t);
    auto p_o = std::views::transform(
      std::views::cartesian_product(
        std::views::transform(offsets, [duration](const Offset& o) { return o.after(duration); }),
        pts
      ),
      [this, &new_time](const pair<const Offset&, const InnerPos&>& o_p) {
        const auto pos = std::get<1>(o_p).add(std::get<0>(o_p));
        const auto for_cell = cell(pos);
        // HACK: just use side-effect to log and check bounds
        points_log_.log(step_, STAGE_SPREAD, new_time, pos.x(), pos.y());
#ifdef DEBUG_POINTS
        // was doing this check after getting for_cell, so it didn't help when out of bounds
        log_check_fatal(
          pos.x() < 0 || pos.y() < 0 || pos.x() >= this->columns() || pos.y() >= this->rows(),
          "Tried to spread out of bounds to (%f, %f)",
          pos.x(),
          pos.y()
        );
#endif
        return std::pair<Cell, InnerPos>(for_cell, pos);
      }
    );
    PointsMap points_map{};
    points_map.merge_values(p_o);
    points_map.for_each([this, &location, &sources](const auto& kv) {
      const auto& for_cell = kv.first;
      const auto source = relativeIndex(for_cell, location);
      sources[for_cell] |= source;
      if (!unburnable_.at(for_cell.hash()))
      {
        auto& pts = points_[for_cell];
        pts.insert(pts.end(), kv.second.begin(), kv.second.end());
        // works the same if we hull here
        if (pts.size() > MAX_BEFORE_CONDENSE)
        {
          // 3 points should just be a triangle usually (could be co-linear, but that's fine
          hull(pts);
        }
      }
    });
  };
  using CellPair = pair<const SpreadKey, vector<CellPts>>;
  auto apply_spread = [this, &apply_offsets](const CellPair& kv0) {
    auto& key = kv0.first;
    auto& offsets = spread_info_[key].offsets();
    do_each(kv0.second, [&apply_offsets, &offsets](const tuple<Cell, PointSet> pts_for_cell) {
      apply_offsets(std::tuple(std::get<0>(pts_for_cell), std::get<1>(pts_for_cell), &offsets));
    });
  };
  do_each(to_spread, apply_spread);
  map<Cell, PointSet> points_cur{};
  std::swap(points_, points_cur);
  // if we move everything out of points_ we can parallelize this check?
  do_each(points_cur, [this, &sources, &new_time](pair<const Cell, PointSet>& kv) {
    auto& for_cell = kv.first;
    auto& pts = kv.second;
    logging::check_fatal(pts.empty(), "Empty points for some reason");
    const auto& seek_spread = spread_info_.find(for_cell.key());
    const auto max_intensity =
      (spread_info_.end() == seek_spread) ? 0 : seek_spread->second.maxIntensity();
    if (canBurn(for_cell) && max_intensity > 0)
    {
      // HACK: make sure it can't round down to 0
      const auto intensity = static_cast<IntensitySize>(max(1.0, max_intensity));
      // HACK: just use the first cell as the source
      const auto source = sources[for_cell];
      const auto fake_event = Event::makeFireSpread(new_time, intensity, for_cell, source);
      burn(fake_event, intensity);
    }
    if (!unburnable_.at(for_cell.hash())
        && ((survives(new_time, for_cell, new_time - arrival_[for_cell]) && !isSurrounded(for_cell))
        ))
    {
      points_log_.log(step_, STAGE_CONDENSE, new_time, pts);
      std::swap(points_[for_cell], pts);
    }
    else
    {
      // just inserted false, so make sure unburnable gets updated
      // whether it went out or is surrounded just mark it as unburnable
      unburnable_.set(for_cell.hash());
      // not swapping means these points get dropped
    }
  });
  log_extensive("Spreading %d points until %f", points_.size(), new_time);
  addEvent(Event::makeFireSpread(new_time));
}
double Scenario::currentFireSize() const { return intensity_->fireSize(); }
bool Scenario::canBurn(const Cell& location) const { return intensity_->canBurn(location); }
bool Scenario::hasBurned(const Location& location) const { return intensity_->hasBurned(location); }
void Scenario::endSimulation() noexcept
{
  log_verbose("Ending simulation");
  scheduler_ = set<Event, EventCompare>();
}
void Scenario::addSaveByOffset(const int offset)
{
  // offset is from beginning of the day the simulation starts
  // e.g. 1 is midnight, 2 is tomorrow at midnight
  addSave(static_cast<Day>(startTime()) + offset);
}
vector<double> Scenario::savePoints() const { return save_points_; }
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
