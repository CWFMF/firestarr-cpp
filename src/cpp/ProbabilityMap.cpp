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
ProbabilityMap::ProbabilityMap(
  const DurationSize time,
  const DurationSize start_time,
  const int min_value,
  const int low_max,
  const int med_max,
  const int max_value,
  const GridBase& grid_info
)
  : all_(GridMap<size_t>(grid_info, 0)), high_(GridMap<size_t>(grid_info, 0)),
    med_(GridMap<size_t>(grid_info, 0)), low_(GridMap<size_t>(grid_info, 0)), time_(time),
    start_time_(start_time), min_value_(min_value), max_value_(max_value), low_max_(low_max),
    med_max_(med_max), perimeter_(nullptr)
{ }
ProbabilityMap* ProbabilityMap::copyEmpty() const
{
  return new ProbabilityMap(time_, start_time_, min_value_, low_max_, med_max_, max_value_, all_);
}
void ProbabilityMap::setPerimeter(const Perimeter* const perimeter) { perimeter_ = perimeter; }
void ProbabilityMap::addProbabilities(const ProbabilityMap& rhs)
{
#ifndef DEBUG_PROBABILITY
  logging::check_fatal(rhs.time_ != time_, "Wrong time");
  logging::check_fatal(rhs.start_time_ != start_time_, "Wrong start time");
  logging::check_fatal(rhs.min_value_ != min_value_, "Wrong min value");
  logging::check_fatal(rhs.max_value_ != max_value_, "Wrong max value");
  logging::check_fatal(rhs.low_max_ != low_max_, "Wrong low max value");
  logging::check_fatal(rhs.med_max_ != med_max_, "Wrong med max value");
#endif
  lock_guard<mutex> lock(mutex_);
  if (Settings::saveIntensity())
  {
    for (auto&& kv : rhs.low_.data)
    {
      low_.data[kv.first] += kv.second;
    }
    for (auto&& kv : rhs.med_.data)
    {
      med_.data[kv.first] += kv.second;
    }
    for (auto&& kv : rhs.high_.data)
    {
      high_.data[kv.first] += kv.second;
    }
  }
  for (auto&& kv : rhs.all_.data)
  {
    all_.data[kv.first] += kv.second;
  }
  for (auto size : rhs.sizes_)
  {
    static_cast<void>(insert_sorted(&sizes_, size));
  }
}
void ProbabilityMap::addProbability(const IntensityMap& for_time)
{
  lock_guard<mutex> lock(mutex_);
  std::for_each(for_time.cbegin(), for_time.cend(), [this](auto&& kv) {
    const auto k = kv.first;
    const auto v = kv.second;
    all_.data[k] += 1;
    if (Settings::saveIntensity())
    {
      if (v >= min_value_ && v <= low_max_)
      {
        low_.data[k] += 1;
      }
      else if (v > low_max_ && v <= med_max_)
      {
        med_.data[k] += 1;
      }
      else if (v > med_max_ && v <= max_value_)
      {
        high_.data[k] += 1;
      }
      else
      {
        logging::fatal("Value %d doesn't fit into any range", v);
      }
    }
  });
  const auto size = for_time.fireSize();
  static_cast<void>(insert_sorted(&sizes_, size));
}
vector<MathSize> ProbabilityMap::getSizes() const { return sizes_; }
Statistics ProbabilityMap::getStatistics() const { return Statistics{getSizes()}; }
size_t ProbabilityMap::numSizes() const noexcept { return sizes_.size(); }
void ProbabilityMap::show() const
{
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
FileList ProbabilityMap::record_if_interim(FileList&& files) const
{
  lock_guard<mutex> lock(PATHS_INTERIM_MUTEX);
  for (const auto& f : files)
  {
    const auto filename = f.c_str();
    logging::verbose("Checking if %s is interim", filename);
    if (nullptr != strstr(filename, "interim_"))
    {
      logging::verbose("Recording %s as interim", filename);
      // is an interim file, so keep path for later deleting
      PATHS_INTERIM.emplace(filename);
      logging::check_fatal(
        !PATHS_INTERIM.contains(filename), "Expected %s to be in interim files list", filename
      );
    }
  }
  return files;
}
FileList ProbabilityMap::saveSizes(const string_view output_directory, const string_view base_name)
  const
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
string make_string(const char* name, const tm& t, const int day)
{
  constexpr auto mask = "%s_%03d_%04d-%02d-%02d";
  char tmp[100];
  sxprintf(tmp, mask, name, day, t.tm_year + TM_YEAR_OFFSET, t.tm_mon + TM_MONTH_OFFSET, t.tm_mday);
  return string(tmp);
};
void ProbabilityMap::deleteInterim()
{
  lock_guard<mutex> lock(PATHS_INTERIM_MUTEX);
  for (const auto& path : PATHS_INTERIM)
  {
    logging::debug("Removing interim file %s", path.c_str());
    if (file_exists(path.c_str()))
    {
      try
      {
        unlink(path.c_str());
      }
      catch (const std::exception& err)
      {
        logging::error("Error trying to remove %s", path.c_str());
        logging::error(err.what());
      }
    }
  }
}
FileList ProbabilityMap::saveAll(
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
  if (Settings::runAsync())
  {
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
    if (Settings::saveOccurrence())
    {
      results.push_back(async(
        launch::async,
        &ProbabilityMap::saveTotalCount,
        this,
        output_directory,
        fix_string("occurrence")
      ));
    }
    if (Settings::saveIntensity())
    {
      results.push_back(async(
        launch::async, &ProbabilityMap::saveLow, this, output_directory, fix_string("intensity_L")
      ));
      results.push_back(async(
        launch::async,
        &ProbabilityMap::saveModerate,
        this,
        output_directory,
        fix_string("intensity_M")
      ));
      results.push_back(async(
        launch::async, &ProbabilityMap::saveHigh, this, output_directory, fix_string("intensity_H")
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
  }
  else
  {
    if (Settings::saveProbability())
    {
      files.append_range(saveTotal(output_directory, fix_string("probability"), is_interim));
    }
    if (Settings::saveOccurrence())
    {
      files.append_range(saveTotalCount(output_directory, fix_string("occurrence")));
    }
    if (Settings::saveIntensity())
    {
      files.append_range(saveLow(output_directory, fix_string("intensity_L")));
      files.append_range(saveModerate(output_directory, fix_string("intensity_M")));
      files.append_range(saveHigh(output_directory, fix_string("intensity_H")));
    }
    files.append_range(saveSizes(output_directory, fix_string("sizes")));
  }
  return files;
}
FileList ProbabilityMap::saveTotal(
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
    with_perim, output_directory, base_name, static_cast<float>(numSizes())
  );
}
FileList ProbabilityMap::saveTotalCount(
  const string_view output_directory,
  const string_view base_name
) const
{
  return saveToProbabilityFile<uint32_t>(all_, output_directory, base_name, 1);
}
FileList ProbabilityMap::saveHigh(const string_view output_directory, const string_view base_name)
  const
{
  return saveToProbabilityFile<float>(
    high_, output_directory, base_name, static_cast<float>(numSizes())
  );
}
FileList ProbabilityMap::saveModerate(
  const string_view output_directory,
  const string_view base_name
) const
{
  return saveToProbabilityFile<float>(
    med_, output_directory, base_name, static_cast<float>(numSizes())
  );
}
FileList ProbabilityMap::saveLow(const string_view output_directory, const string_view base_name)
  const
{
  return saveToProbabilityFile<float>(
    low_, output_directory, base_name, static_cast<float>(numSizes())
  );
}
void ProbabilityMap::reset()
{
  all_.clear();
  low_.clear();
  med_.clear();
  high_.clear();
  sizes_.clear();
}
}
