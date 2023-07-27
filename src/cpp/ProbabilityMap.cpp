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
#include "ProbabilityMap.h"
#include "FBP45.h"
#include "IntensityMap.h"
#include "Model.h"
namespace tbd::sim
{
ProbabilityMap::ProbabilityMap(const double time,
                               const double start_time,
                               const int min_value,
                               const int low_max,
                               const int med_max,
                               const int max_value,
                               const data::GridBase& grid_info)
  : all_(data::GridMap<size_t>(grid_info, 0)),
    high_(data::GridMap<size_t>(grid_info, 0)),
    med_(data::GridMap<size_t>(grid_info, 0)),
    low_(data::GridMap<size_t>(grid_info, 0)),
    time_(time),
    start_time_(start_time),
    min_value_(min_value),
    max_value_(max_value),
    low_max_(low_max),
    med_max_(med_max),
    perimeter_(nullptr)
{
}
ProbabilityMap* ProbabilityMap::copyEmpty() const
{
  return new ProbabilityMap(time_,
                            start_time_,
                            min_value_,
                            low_max_,
                            med_max_,
                            max_value_,
                            all_);
}
void ProbabilityMap::setPerimeter(const topo::Perimeter* const perimeter)
{
  perimeter_ = perimeter;
}
void ProbabilityMap::addProbabilities(const ProbabilityMap& rhs)
{
#ifndef NDEBUG
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
    static_cast<void>(util::insert_sorted(&sizes_, size));
  }
}
void ProbabilityMap::addProbability(const IntensityMap& for_time)
{
  lock_guard<mutex> lock(mutex_);
  std::for_each(
    for_time.cbegin(),
    for_time.cend(),
    [this](auto&& kv) {
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
  static_cast<void>(util::insert_sorted(&sizes_, size));
}
vector<double> ProbabilityMap::getSizes() const
{
  return sizes_;
}
util::Statistics ProbabilityMap::getStatistics() const
{
  return util::Statistics{getSizes()};
}
size_t ProbabilityMap::numSizes() const noexcept
{
  return sizes_.size();
}
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
    s.median());
}
void ProbabilityMap::saveSizes(const string& base_name) const
{
  ofstream out;
  out.open(Settings::outputDirectory() + base_name + ".csv");
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
string make_string(const char* name, const tm& t, const int day)
{
  constexpr auto mask = "%s_%03d_%04d-%02d-%02d";
  static constexpr size_t OutLength = 100;
  char tmp[OutLength];
  sprintf(tmp,
          mask,
          name,
          day,
          t.tm_year + 1900,
          t.tm_mon + 1,
          t.tm_mday);
  return string(tmp);
};

void ProbabilityMap::saveAll(const Model& model,
                             const tm& start_time,
                             const double time,
                             const double start_day) const
{
  auto t = start_time;
  auto ticks = mktime(&t);
  const auto day = static_cast<int>(round(time));
  ticks += (static_cast<size_t>(day) - t.tm_yday - 1) * DAY_SECONDS;
  t = *localtime(&ticks);
  if (sim::Settings::runAsync())
  {
    vector<std::future<void>> results{};
    if (Settings::saveProbability())
    {
      results.push_back(async(launch::async,
                              &ProbabilityMap::saveTotal,
                              this,
                              make_string("probability", t, day)));
    }
    if (Settings::saveOccurrence())
    {
      results.push_back(async(launch::async,
                              &ProbabilityMap::saveTotalCount,
                              this,
                              make_string("occurrence", t, day)));
    }
    if (Settings::saveIntensity())
    {
      results.push_back(async(launch::async,
                              &ProbabilityMap::saveLow,
                              this,
                              make_string("intensity_L", t, day)));
      results.push_back(async(launch::async,
                              &ProbabilityMap::saveModerate,
                              this,
                              make_string("intensity_M", t, day)));
      results.push_back(async(launch::async,
                              &ProbabilityMap::saveHigh,
                              this,
                              make_string("intensity_H", t, day)));
    }
    results.push_back(async(launch::async,
                            &ProbabilityMap::saveSizes,
                            this,
                            make_string("sizes", t, day)));
    for (auto& result : results)
    {
      result.wait();
    }
  }
  else
  {
    if (Settings::saveProbability())
    {
      saveTotal(make_string("probability", t, day));
    }
    if (Settings::saveOccurrence())
    {
      saveTotalCount(make_string("occurrence", t, day));
    }
    if (Settings::saveIntensity())
    {
      saveLow(make_string("intensity_L", t, day));
      saveModerate(make_string("intensity_M", t, day));
      saveHigh(make_string("intensity_H", t, day));
    }
    saveSizes(make_string("sizes", t, day));
  }
  const auto nd = model.nd(day);
  logging::note("Fuels for day %d are %s green-up and grass has %d%% curing",
                day - static_cast<int>(start_day),
                fuel::calculate_is_green(nd) ? "after" : "before",
                fuel::calculate_grass_curing(nd));
}
void ProbabilityMap::saveTotal(const string& base_name) const
{
  auto with_perim = all_;
  if (nullptr != perimeter_)
  {
    for (auto loc : perimeter_->burned())
    {
      // make initial perimeter cells 2* so that probability ends up as 2
      with_perim.data[loc] *= 2;
    }
  }
  with_perim.saveToProbabilityFile<float>(Settings::outputDirectory(), base_name, static_cast<float>(numSizes()));
}
void ProbabilityMap::saveTotalCount(const string& base_name) const
{
  all_.saveToProbabilityFile<uint32_t>(Settings::outputDirectory(), base_name, 1);
}
void ProbabilityMap::saveHigh(const string& base_name) const
{
  high_.saveToProbabilityFile<float>(Settings::outputDirectory(), base_name, static_cast<float>(numSizes()));
}
void ProbabilityMap::saveModerate(const string& base_name) const
{
  med_.saveToProbabilityFile<float>(Settings::outputDirectory(), base_name, static_cast<float>(numSizes()));
}
void ProbabilityMap::saveLow(const string& base_name) const
{
  low_.saveToProbabilityFile<float>(Settings::outputDirectory(), base_name, static_cast<float>(numSizes()));
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
