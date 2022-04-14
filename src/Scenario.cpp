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
#include "Scenario.h"
#include "Observer.h"
#include "FireSpread.h"
#include "Perimeter.h"
#include "ProbabilityMap.h"
#include "ConvexHull.h"
namespace tbd::sim
{
// FIX: why is this not just 0.5?
constexpr auto CELL_CENTER = 0.5555555555555555;
constexpr auto PRECISION = 0.001;
static atomic<size_t> COUNT = 0;
static atomic<size_t> COMPLETED = 0;
static std::mutex MUTEX_SIM_COUNTS;
static map<size_t, size_t> SIM_COUNTS{};
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
  //  offsets_.clear();
  offsets_ = {};
  extinction_thresholds_.clear();
  spread_thresholds_by_ros_.clear();
  max_ros_ = 0;
#ifndef NDEBUG
  log_check_fatal(!scheduler_.empty(), "Scheduler isn't empty after clear()");
#endif
  model_->releaseBurnedVector(unburnable_);
  unburnable_ = nullptr;
}
size_t Scenario::completed() noexcept
{
  return COMPLETED;
}
size_t Scenario::count() noexcept
{
  return COUNT;
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
      if (i <= last_date)
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
                   const double start_time,
                   const shared_ptr<topo::Perimeter>& perimeter,
                   const topo::StartPoint& start_point,
                   const Day start_day,
                   const Day last_date)
  : Scenario(model, id, weather, start_time, start_point, start_day, last_date)
{
  perimeter_ = perimeter;
  start_cell_ = nullptr;
}
Scenario::Scenario(Model* model,
                   const size_t id,
                   wx::FireWeather* weather,
                   const double start_time,
                   const shared_ptr<topo::Cell>& start_cell,
                   const topo::StartPoint& start_point,
                   const Day start_day,
                   const Day last_date)
  : Scenario(model, id, weather, start_time, start_point, start_day, last_date)
{
  perimeter_ = nullptr;
  start_cell_ = start_cell;
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
  max_ros_ = 0;
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
  const auto num = (static_cast<size_t>(last_date_) - start_day_ + 1) * DAY_HOURS;
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
  //std::fill(extinction_thresholds_.begin(), extinction_thresholds_.end(), 0.5);
  //  std::fill(spread_thresholds_by_ros_.begin(), spread_thresholds_by_ros_.end(), SpreadInfo::calculateRosFromThreshold(0.5));
  for (const auto& o : observers_)
  {
    o->reset();
  }
  current_time_ = start_time_ - 1;
  points_ = {};
  // don't do this until we run so that we don't allocate memory too soon
  intensity_ = make_unique<IntensityMap>(model());
  offsets_ = {};
  max_intensity_ = {};
  arrival_ = {};
  max_ros_ = 0;
  //surrounded_ = POOL_BURNED_DATA.acquire();
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
#ifndef NDEBUG
  log_check_fatal(event.time() < current_time_,
                  "Expected time to be > %f but got %f",
                  current_time_,
                  event.time());
#endif
  const auto& p = event.cell();
  switch (event.type())
  {
    case Event::FIRE_SPREAD:
      scheduleFireSpread(event);
      break;
    case Event::SAVE:
      saveObservers(event.time());
      saveStats(event.time());
      break;
    case Event::NEW_FIRE:
      // HACK: don't do this in constructor because scenario creates this in its constructor
      points_[p].emplace_back(p.column() + CELL_CENTER, p.row() + CELL_CENTER);
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
        const auto wx = weather(event.time());
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
                   const double start_time,
                   topo::StartPoint start_point,
                   const Day start_day,
                   const Day last_date)
  : current_time_(start_time),
    unburnable_(nullptr),
    intensity_(nullptr),
    //surrounded_(nullptr),
    max_ros_(0),
    weather_(weather),
    model_(model),
    probabilities_(nullptr),
    final_sizes_(nullptr),
    start_point_(std::move(start_point)),
    id_(id),
    start_time_(start_time),
    simulation_(-1),
    start_day_(start_day),
    last_date_(last_date),
    ran_(false)
{
  last_save_ = weather_->minDate();
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
    o->save(Settings::outputDirectory(), base_name);
  }
}
void Scenario::saveObservers(const double time) const
{
  static const size_t BufferSize = 64;
  char buffer[BufferSize + 1] = {0};
  sprintf(buffer,
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
    perimeter_(std::move(rhs.perimeter_)),
    offsets_(std::move(rhs.offsets_)),
    arrival_(std::move(rhs.arrival_)),
    max_ros_(rhs.max_ros_),
    start_cell_(std::move(rhs.start_cell_)),
    weather_(rhs.weather_),
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
    perimeter_ = std::move(rhs.perimeter_);
    //surrounded_ = rhs.surrounded_;
    start_cell_ = std::move(rhs.start_cell_);
    weather_ = rhs.weather_;
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
void Scenario::burn(const Event& event, const IntensitySize burn_intensity)
{
#ifndef NDEBUG
  log_check_fatal(intensity_->hasBurned(event.cell()), "Re-burning cell");
#endif
  // Observers only care about cells burning so do it here
  notify(event);
  intensity_->burn(event.cell(), burn_intensity);
  arrival_[event.cell()] = event.time();
  //scheduleFireSpread(event);
}
bool Scenario::isSurrounded(const Location& location) const
{
  return intensity_->isSurrounded(location);
}
topo::Cell Scenario::cell(const InnerPos& p) const noexcept
{
  return cell(p.y, p.x);
}
string Scenario::add_log(const char* format) const noexcept
{
  const string tmp;
  stringstream iss(tmp);
  static char buffer[1024]{0};
  sprintf(buffer, "Scenario %4ld.%04ld (%3f): ", id(), simulation(), current_time_);
  iss << buffer << format;
  //  cout << '"' << iss.str() << '"' << '\n';
  return iss.str();
}
#ifndef NDEBUG
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
#ifndef NDEBUG
  log_check_fatal(ran(), "Scenario has already run");
#endif
  log_verbose("Starting");
  CriticalSection _(Model::task_limiter);
  unburnable_ = model_->getBurnedVector();
  probabilities_ = probabilities;
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
    intensity_->applyPerimeter(*perimeter_);
    const auto& env = model().environment();
    for (const auto& location : perimeter_->edge())
    {
      //      const auto cell = env.cell(location.hash());
      const auto cell = env.cell(location);
#ifndef NDEBUG
      log_check_fatal(fuel::is_null_fuel(cell), "Null fuel in perimeter");
#endif
      log_verbose("Adding point (%d, %d)",
                  cell.column() + CELL_CENTER,
                  cell.row() + CELL_CENTER);
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
  }
  model_->releaseBurnedVector(unburnable_);
  unburnable_ = nullptr;
  if (cancelled_)
  {
    return nullptr;
  }
  ++COMPLETED;
  // HACK: use + to pull value out of atomic
#ifdef NDEBUG
  log_info("[% d of % d] Completed with final size % 0.1f ha",
           +COMPLETED,
           +COUNT,
           currentFireSize());
#else
  // try to make output consistent if in debug mode
  log_info("Completed with final size %0.1f ha",
           currentFireSize());
#endif
  ran_ = true;
#ifndef NDEBUG
  static const size_t BufferSize = 64;
  char buffer[BufferSize + 1] = {0};
  sprintf(buffer,
          "%03zu_%06ld_extinction",
          id(),
          simulation());
  saveProbabilities(Settings::outputDirectory(), string(buffer), extinction_thresholds_);
  sprintf(buffer,
          "%03zu_%06ld_spread",
          id(),
          simulation());
  saveProbabilities(Settings::outputDirectory(), string(buffer), spread_thresholds_by_ros_);
#endif
  return this;
}
[[nodiscard]] ostream& operator<<(ostream& os, const PointSet& a)
{
  for (auto pt : a)
  {
    os << "(" << pt.x << ", " << pt.y << "), ";
  }
  return os;
}

// want to be able to make a bitmask of all directions it came from
//  064  008  032
//  001  000  002
//  016  004  128
static constexpr CellIndex DIRECTION_NONE = 0b00000000;
static constexpr CellIndex DIRECTION_W = 0b00000001;
static constexpr CellIndex DIRECTION_E = 0b00000010;
static constexpr CellIndex DIRECTION_S = 0b00000100;
static constexpr CellIndex DIRECTION_N = 0b00001000;
static constexpr CellIndex DIRECTION_SW = 0b00010000;
static constexpr CellIndex DIRECTION_NE = 0b00100000;
static constexpr CellIndex DIRECTION_NW = 0b01000000;
static constexpr CellIndex DIRECTION_SE = 0b10000000;

/**
 * Determine the direction that a given cell is in from another cell. This is the
 * same convention as wind (i.e. the direction it is coming from, not the direction
 * it is going towards).
 * @param for_cell The cell to find directions relative to
 * @param from_cell The cell to find the direction of
 * @return Direction that you would have to go in to get to from_cell from for_cell
 */
CellIndex relativeIndex(const topo::Cell& for_cell, const topo::Cell& from_cell)
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
      //center right
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
  //note("time is %f", time);
  current_time_ = time;
  const auto wx = weather(time);
  logging::check_fatal(nullptr == wx, "No weather available for time %f", time);
  //  log_note("%d points", points_->size());
  const auto this_time = util::time_index(time);
  const auto next_time = static_cast<double>(this_time + 1) / DAY_HOURS;
  // should be in minutes?
  const auto max_duration = (next_time - time) * DAY_MINUTES;
  //note("time is %f, next_time is %f, max_duration is %f",
  //     time,
  //     next_time,
  //     max_duration);
  const auto max_time = time + max_duration / DAY_MINUTES;
  if (wx->ffmc().asDouble() < minimumFfmcForSpread(time))
  {
    addEvent(Event::makeFireSpread(max_time));
    log_verbose("Waiting until %f because of FFMC", max_time);
    return;
  }
  if (current_time_index_ != this_time)
  {
    current_time_index_ = this_time;
    //    offsets_.clear();
    //    max_intensity_.clear();
    offsets_ = {};
    max_intensity_ = {};
    max_ros_ = 0.0;
  }
  auto any_spread = false;
  for (const auto& kv : points_)
  {
    const auto& location = kv.first;
    // any cell that has the same fuel, slope, and aspect has the same spread
    const auto key = location.key();
    const auto seek_spreading = offsets_.find(key);
    if (seek_spreading == offsets_.end())
    {
      // have not calculated spread for this cell yet
      const SpreadInfo origin(*this, time, location, nd(time), wx);
      // will be empty if invalid
      offsets_.emplace(key, origin.offsets());
      if (!origin.isNotSpreading())
      {
        any_spread = true;
        max_ros_ = max(max_ros_, origin.headRos());
        max_intensity_[key] = max(max_intensity_[key], origin.maxIntensity());
      }
    }
    else
    {
      // already did the lookup so use the result
      any_spread |= !seek_spreading->second.empty();
    }
  }
  if (!any_spread || max_ros_ < Settings::minimumRos())
  {
    log_verbose("Waiting until %f", max_time);
    addEvent(Event::makeFireSpread(max_time));
    return;
  }
  //note("Max spread is %f, max_ros is %f",
  //     Settings::maximumSpreadDistance() * cellSize(),
  //     max_ros_);
  const auto duration = ((max_ros_ > 0)
                           ? min(max_duration,
                                 Settings::maximumSpreadDistance() * cellSize() / max_ros_)
                           : max_duration);
  //note("Spreading for %f minutes", duration);
  map<topo::Cell, CellIndex> sources{};
  const auto new_time = time + duration / DAY_MINUTES;
  map<topo::Cell, PointSet> point_map_{};
  map<topo::Cell, size_t> count{};
  for (auto& kv : points_)
  {
    const auto& location = kv.first;
    count[location] = kv.second.size();
    const auto key = location.key();
    auto& offsets = offsets_.at(key);
    if (!offsets.empty())
    {
      for (auto& o : offsets)
      {
        // offsets in meters
        const auto offset_x = o.x * duration;
        const auto offset_y = o.y * duration;
        const Offset offset{offset_x, offset_y};
        //note("%f, %f", offset_x, offset_y);
        for (auto& p : kv.second)
        {
          const InnerPos pos = p.add(offset);
          const auto for_cell = cell(pos);
          const auto source = relativeIndex(for_cell, location);
          sources[for_cell] |= source;
          if (!(*unburnable_)[for_cell.hash()])
          {
            //log_extensive("Adding point (%f, %f)", pos.x, pos.y);
            point_map_[for_cell].emplace_back(pos);
          }
        }
      }
    }
    else
    {
      // can't just keep existing points by swapping because something may have spread into this cell
      auto& pts = point_map_[location];
      pts.insert(pts.end(), kv.second.begin(), kv.second.end());
    }
    //    kv.second.clear();
    kv.second = {};
  }
  vector<topo::Cell> erase_what{};
  for (auto& kv : point_map_)
  {
    auto& for_cell = kv.first;
    if (!kv.second.empty())
    {
      if (canBurn(for_cell) && max_intensity_[for_cell.key()] > 0)
      {
        // HACK: make sure it can't round down to 0
        const auto intensity = static_cast<IntensitySize>(max(
          1.0,
          max_intensity_[for_cell.key()]));
        // HACK: just use the first cell as the source
        const auto source = sources[for_cell];
        const auto fake_event = Event::makeFireSpread(
          new_time,
          intensity,
          for_cell,
          source);
        burn(fake_event, intensity);
      }
      // check if this cell is surrounded by burned cells or non-fuels
      // if surrounded then just drop all the points inside this cell
      if (!(*unburnable_)[for_cell.hash()])
      {
        // do survival check first since it should be easier
        if (survives(new_time, for_cell, new_time - arrival_[for_cell]) && !isSurrounded(for_cell))
        {
          if (count[for_cell] > 1 && kv.second.size() > 3)
          {
            // no point in doing hull if only one point spread
            // 3 points should just be a triangle usually (could be co-linear, but that's fine
            hull(kv.second);
          }
          std::swap(points_[for_cell], kv.second);
        }
        else
        {
          // whether it went out or is surrounded just mark it as unburnable
          (*unburnable_)[for_cell.hash()] = true;
          erase_what.emplace_back(for_cell);
        }
      }
      //      kv.second.clear();
      kv.second = {};
    }
    else
    {
      erase_what.emplace_back(for_cell);
    }
  }
  for (auto& c : erase_what)
  {
    points_.erase(c);
  }
  log_verbose("Spreading %d points until %f", points_.size(), new_time);
  addEvent(Event::makeFireSpread(new_time));
}
double Scenario::currentFireSize() const
{
  return intensity_->fireSize();
}
bool Scenario::canBurn(const topo::Cell& location) const
{
  return intensity_->canBurn(location);
}
//bool Scenario::canBurn(const HashSize hash) const
//{
//  return intensity_->canBurn(hash);
//}
bool Scenario::hasBurned(const Location& location) const
{
  return intensity_->hasBurned(location);
}
//bool Scenario::hasBurned(const HashSize hash) const
//{
//  return intensity_->hasBurned(hash);
//}
void Scenario::endSimulation() noexcept
{
  log_verbose("Ending simulation");
  //  scheduler_.clear();
  scheduler_ = set<Event, EventCompare>();
}
void Scenario::addSaveByOffset(const int offset)
{
  // +1 since yesterday is in here too
  addSave(weather_->minDate() + offset + 1);
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
void Scenario::cancel() noexcept
{
  cancelled_ = true;
}
}
