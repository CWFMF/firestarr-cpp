/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2021-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "ProbabilityMap.h"

#include "GridMap.h"
#include "IntensityMap.h"

namespace fs
{
static constexpr size_t VALUE_UNPROCESSED = 2;
static constexpr size_t VALUE_PROCESSING = 3;
static constexpr size_t VALUE_PROCESSED = 4;

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
  const DurationSize time,
  const DurationSize start_time,
  const GridBase& grid_info
)
  : all_(GridMap<size_t>(grid_info, 0)),
    time_(time),
    start_time_(start_time),
    perimeter_(nullptr)
{
}

ProbabilityMap*
ProbabilityMap::copyEmpty() const
{
  return new ProbabilityMap(time_, start_time_, all_);
}

void
ProbabilityMap::setPerimeter(
  const Perimeter* const perimeter
)
{
  lock_guard<mutex> lock(mutex_);
  perimeter_ = perimeter;
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
    static_cast<void>(insert_sorted(&sizes_, size));
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
  static_cast<void>(insert_sorted(&sizes_, size));
}

vector<MathSize>
ProbabilityMap::getSizes() const
{
  return sizes_;
}

Statistics
ProbabilityMap::getStatistics() const
{
  return Statistics{getSizes()};
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

[[nodiscard]] FileList
ProbabilityMap::record_if_interim(
  FileList&& files
) const
{
  lock_guard<mutex> lock_interim(PATHS_INTERIM_MUTEX);
  for (const auto& f : files)
  {
    const auto filename = f.c_str();
    logging::verbose("Checking if %s is interim", filename);
    if (nullptr != strstr(filename, "interim_"))
    {
      const auto filename = f.c_str();
      logging::verbose("Checking if %s is interim", filename);
      if (nullptr != strstr(filename, "interim_"))
      {
        logging::verbose("Recording %s as interim", filename);
        // is an interim file, so keep path for later deleting
        PATHS_INTERIM.emplace(filename);
        logging::check_fatal(
          !PATHS_INTERIM.contains(filename),
          "Expected %s to be in interim files list",
          filename
        );
      }
    }
  }
  return files;
}

[[nodiscard]] FileList
ProbabilityMap::saveSizes(
  const string_view output_directory,
  const string_view base_name
) const
{
  const auto filename = string(output_directory) + string(base_name) + ".csv";
  ofstream out{filename};
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
  return record_if_interim({filename});
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
  sxprintf(tmp, mask, name, day, t.tm_year + TM_YEAR_OFFSET, t.tm_mon + TM_MONTH_OFFSET, t.tm_mday);
  return string(tmp);
};

void
ProbabilityMap::deleteInterim()
{
  lock_guard<mutex> lock_interim(PATHS_INTERIM_MUTEX);
  for (const auto& path : PATHS_INTERIM)
  {
    logging::debug("Removing interim file %s", path.c_str());
    if (file_exists(path.c_str()))
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

[[nodiscard]] FileList
ProbabilityMap::saveAll(
  const string_view output_directory,
  const tm& start_time,
  const DurationSize time,
  const bool is_interim
) const
{
  lock_guard<mutex> lock(mutex_);
  FileList files{};
  auto t = start_time;
  auto ticks = mktime(&t);
  const auto day = static_cast<int>(round(time));
  ticks += (static_cast<size_t>(day) - t.tm_yday - 1) * DAY_SECONDS;
  t = *localtime(&ticks);
  auto fix_string = [&t, &day, &is_interim](string prefix) {
    auto text = (is_interim ? "interim_" : "") + prefix;
    return make_string(text.c_str(), t, day);
  };
  vector<std::future<FileList>> results{};
  if (Settings::saveProbability())
  {
    results.push_back(async(
      launch::async,
      &ProbabilityMap::saveTotal,
      this,
      output_directory,
      fix_string("probability"),
      is_interim
    ));
  }
  results.push_back(
    async(launch::async, &ProbabilityMap::saveSizes, this, output_directory, fix_string("sizes"))
  );
  for (auto& result : results)
  {
    result.wait();
    files.append_range(result.get());
  }
  return files;
}

[[nodiscard]] FileList
ProbabilityMap::saveTotal(
  const string_view output_directory,
  const string_view base_name,
  const bool is_interim
) const
{
  // FIX: do this for other outputs too
  auto with_perim = all_;
  if (nullptr != perimeter_)
  {
    for (auto loc : perimeter_->burned())
    {
      // multiply initial perimeter cells so that probability shows processing status
      with_perim.data[loc] *= (is_interim ? VALUE_PROCESSING : VALUE_PROCESSED);
    }
  }
  return saveToProbabilityFile<float>(
    with_perim,
    output_directory,
    base_name,
    static_cast<float>(numSizes())
  );
}

[[nodiscard]] FileList
ProbabilityMap::saveTotalCount(
  const string_view output_directory,
  const string_view base_name
) const
{
  return saveToProbabilityFile<uint32_t>(all_, output_directory, base_name, 1);
}

void
ProbabilityMap::reset()
{
  lock_guard<mutex> lock(mutex_);
  all_.clear();
  sizes_.clear();
}
}
