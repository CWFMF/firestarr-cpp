/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2021-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "ProbabilityMap.h"
#include "FBP45.h"
#include "IntensityMap.h"
#include "Model.h"
#include "GridMap.h"
namespace fs::sim
{
/**
 * \brief List of interim files that were saved
 */
static set<string> PATHS_INTERIM{};
static mutex PATHS_INTERIM_MUTEX{};

ProbabilityMap::~ProbabilityMap()
{
  // make sure we don't delete if still locked
  lock_guard<mutex> lock(mutex_);
}

ProbabilityMap::ProbabilityMap(
  const string dir_out,
  const DurationSize time,
  const DurationSize start_time,
  const data::GridBase& grid_info,
  const std::optional<topo::Perimeter>& perimeter
)
  : dir_out_(dir_out),
    all_(data::GridMap<size_t>(grid_info, 0)),
    time_(time),
    start_time_(start_time),
    perimeter_(perimeter)
{
}
shared_ptr<ProbabilityMap>
ProbabilityMap::copyEmpty() const
{
  return make_shared<ProbabilityMap>(dir_out_, time_, start_time_, all_, perimeter_);
}
void
ProbabilityMap::addProbabilities(
  const ProbabilityMap& rhs
)
{
#ifndef DEBUG_PROBABILITY
  logging::check_fatal(rhs.time_ != time_, "Wrong time");
  logging::check_fatal(rhs.start_time_ != start_time_, "Wrong start time");
#endif
  lock_guard<mutex> lock(mutex_);
  // need to lock both maps
  lock_guard<mutex> lock_rhs(rhs.mutex_);
  for (auto&& kv : rhs.all_.data)
  {
    all_.data[kv.first] += kv.second;
  }
  for (auto size : rhs.sizes_)
  {
    static_cast<void>(util::insert_sorted(&sizes_, size));
  }
}
void
ProbabilityMap::addProbability(
  const IntensityMap& for_time
)
{
  lock_guard<mutex> lock(mutex_);
  std::for_each(for_time.cbegin(), for_time.cend(), [this](auto&& kv) {
    const auto k = kv.first;
    all_.data[k] += 1;
  });
  const auto size = for_time.fireSize();
  static_cast<void>(util::insert_sorted(&sizes_, size));
}
vector<MathSize>
ProbabilityMap::getSizes() const
{
  return sizes_;
}
util::Statistics
ProbabilityMap::getStatistics() const
{
  return util::Statistics{getSizes()};
}
size_t
ProbabilityMap::numSizes() const noexcept
{
  return sizes_.size();
}
void
ProbabilityMap::show() const
{
  lock_guard<mutex> lock(mutex_);
  // even if we only ran the actuals we'll still have multiple scenarios
  // with different randomThreshold values
  const auto day = static_cast<int>(time_ - floor(start_time_));
  const auto s = getStatistics();
  logging::note(
    "Fire size at end of day %d: %0.1f ha - %0.1f ha (mean %0.1f ha, median %0.1f ha)",
    day,
    s.min(),
    s.max(),
    s.mean(),
    s.median()
  );
}
bool
ProbabilityMap::record_if_interim(
  const char* filename
) const
{
  lock_guard<mutex> lock_interim(PATHS_INTERIM_MUTEX);
  logging::verbose("Checking if %s is interim", filename);
  if (nullptr != strstr(filename, "interim_"))
  {
    logging::verbose("Recording %s as interim", filename);
    // is an interim file, so keep path for later deleting
    PATHS_INTERIM.emplace(string(filename));
    logging::check_fatal(
      !PATHS_INTERIM.contains(filename),
      "Expected %s to be in interim files list",
      filename
    );
    return true;
  }
  return false;
}
void
ProbabilityMap::saveSizes(
  const string& base_name
) const
{
  ofstream out;
  string filename = dir_out_ + base_name + ".csv";
  record_if_interim(filename.c_str());
  out.open(filename.c_str());
  auto sizes = getSizes();
  if (!sizes.empty())
  {
    // don't want to modify original array so that we can still lookup in correct order
    sort(sizes.begin(), sizes.end());
  }
  for (const auto& s : sizes)
  {
    out << s << "\n";
  }
  out.close();
}
string
make_string(
  const char* name,
  const tm& t,
  const int day
)
{
  constexpr auto mask = "%s_%03d_%04d-%02d-%02d";
  char tmp[100];
  sxprintf(tmp, mask, name, day, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
  return string(tmp);
};
void
ProbabilityMap::deleteInterim()
{
  lock_guard<mutex> lock_interim(PATHS_INTERIM_MUTEX);
  for (const auto& path : PATHS_INTERIM)
  {
    logging::debug("Removing interim file %s", path.c_str());
    if (util::file_exists(path.c_str()))
    {
      try
      {
#ifdef _WIN32
        _unlink(path.c_str());
#else
        unlink(path.c_str());
#endif
      }
      catch (const std::exception& err)
      {
        logging::error("Error trying to remove %s", path.c_str());
        logging::error(err.what());
      }
    }
  }
}
void
ProbabilityMap::saveAll(
  const tm& start_time,
  const DurationSize time,
  const ProcessingStatus processing_status
) const
{
  lock_guard<mutex> lock(mutex_);
  const auto is_interim = processed != processing_status;
  auto t = start_time;
  auto ticks = mktime(&t);
  const auto day = static_cast<int>(round(time));
  ticks += (static_cast<size_t>(day) - t.tm_yday - 1) * DAY_SECONDS;
  t = *localtime(&ticks);
  auto fix_string = [&t, &day, &is_interim](string prefix) {
    auto text = (is_interim ? "interim_" : "") + prefix;
    return make_string(text.c_str(), t, day);
  };
  vector<std::future<void>> results{};
  if (Settings::saveProbability())
  {
    results.push_back(async(
      launch::async,
      &ProbabilityMap::saveTotal,
      this,
      fix_string("probability"),
      processing_status
    ));
  }
  results.push_back(async(launch::async, &ProbabilityMap::saveSizes, this, fix_string("sizes")));
  for (auto& result : results)
  {
    result.wait();
  }
}
void
ProbabilityMap::saveTotal(
  const string& base_name,
  const ProcessingStatus processing_status
) const
{
  // FIX: do this for other outputs too
  auto with_perim = all_;
  if (perimeter_.has_value())
  {
    for (auto loc : perimeter_->burned)
    {
      // multiply initial perimeter cells so that probability shows processing status
      // HACK: value is 0 if nothing is saved yet
      with_perim.data[loc] = max(static_cast<size_t>(1), with_perim.data[loc]) * processing_status;
    }
  }
  // HACK: numSize() is going to be 0 if nothing saved yet
  const auto divisor = max(static_cast<float>(1.0), static_cast<float>(numSizes()));
  saveToProbabilityFile<float>(with_perim, dir_out_, base_name, divisor);
}
void
ProbabilityMap::saveTotalCount(
  const string& base_name
) const
{
  saveToProbabilityFile<uint32_t>(all_, dir_out_, base_name, 1);
}
void
ProbabilityMap::reset()
{
  lock_guard<mutex> lock(mutex_);
  all_.clear();
  sizes_.clear();
}
}
