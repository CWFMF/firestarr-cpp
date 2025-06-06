/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "IntensityMap.h"
#include "Model.h"
#include "Perimeter.h"
#include "Weather.h"
namespace fs::sim
{

// FIX: maybe this can be more generic but just want to keep allocated objects and reuse them
template <class K>
class GridMapCache
{
public:
  GridMapCache(
    K nodata
  )
    : nodata_(nodata)
  {
  }
  // // use maximum value as nodata if not given
  // // HACK: need to be able to convert to int, so don't use a value bigger than that can hold
  // GridMapCache()
  //   : GridMapCache(
  //       static_cast<K>(
  //         min(static_cast<long double>(std::numeric_limits<int>::max()),
  //             static_cast<long double>(std::numeric_limits<K>::max()))))
  // {
  // }
  void
  release_map(
    unique_ptr<data::GridMap<K>> map
  ) noexcept
  {
    map->clear();
    try
    {
      lock_guard<mutex> lock(mutex_);
      maps_.push_back(std::move(map));
    }
    catch (const std::exception& ex)
    {
      logging::fatal(ex);
      std::terminate();
    }
  }
  unique_ptr<data::GridMap<K>>
  acquire_map(
    const Model& model
  ) noexcept
  {
    try
    {
      lock_guard<mutex> lock(mutex_);
      if (!maps_.empty())
      {
        auto result = std::move(maps_.at(maps_.size() - 1));
        maps_.pop_back();
        return result;
      }
      return model.environment().makeMap<K>(nodata_);
    }
    catch (const std::exception& ex)
    {
      logging::fatal(ex);
      std::terminate();
    }
  }
protected:
  K nodata_;
  vector<unique_ptr<data::GridMap<K>>> maps_;
  mutex mutex_;
};

static auto CacheIntensitySize = GridMapCache<IntensitySize>(NO_INTENSITY);
IntensityMap::IntensityMap(
  const Model& model
) noexcept
  : model_(model),
    intensity_max_(CacheIntensitySize.acquire_map(model)),
    is_burned_(model.getBurnedVector())
{
}

IntensityMap::IntensityMap(
  const IntensityMap& rhs
)
  // : IntensityMap(rhs.model_, nullptr)
  : IntensityMap(rhs.model_)
{
  *intensity_max_ = *rhs.intensity_max_;
  is_burned_ = rhs.is_burned_;
}

// IntensityMap::IntensityMap(IntensityMap&& rhs)
//   : IntensityMap(rhs.model_)
// {
//   *intensity_max_ = *rhs.intensity_max_;
//   is_burned_ = rhs.is_burned_;
// }

IntensityMap::~IntensityMap() noexcept
{
  model_.releaseBurnedVector(is_burned_);
  CacheIntensitySize.release_map(std::move(intensity_max_));
}
void
IntensityMap::applyPerimeter(
  const topo::Perimeter& perimeter
) noexcept
{
  // logging::verbose("Attaining lock");
  // lock_guard<mutex> lock(mutex_);
  logging::verbose("Applying burned cells");
  std::for_each(
    std::execution::par_unseq,
    perimeter.burned().begin(),
    perimeter.burned().end(),
    [this](const auto& location) {
      ignite(location);
    }
  );
}
// bool IntensityMap::canBurn(const HashSize hash_value) const
//{
//   return !hasBurned(hash);
// }
bool
IntensityMap::canBurn(
  const HashSize hash_value
) const
{
  return !hasBurned(hash_value);
}
bool
IntensityMap::hasBurned(
  const HashSize hash_value
) const
{
  lock_guard<mutex> lock(mutex_);
  return (*is_burned_)[hash_value];
  //  return hasBurned(location.hash());
}
// bool IntensityMap::hasBurned(const HashSize hash_value) const
//{
//   lock_guard<mutex> lock(mutex_);
//   return (*is_burned_)[hash];
// }
bool
IntensityMap::isSurrounded(
  const HashSize hash_value
) const
{
  // implement here so we can just lock once
  lock_guard<mutex> lock(mutex_);
  const Location location{hash_value};
  const auto x = location.column();
  const auto y = location.row();
  const auto min_row = static_cast<Idx>(max(y - 1, 0));
  const auto max_row = min(y + 1, this->rows() - 1);
  const auto min_column = static_cast<Idx>(max(x - 1, 0));
  const auto max_column = min(x + 1, this->columns() - 1);
  for (auto r = min_row; r <= max_row; ++r)
  {
    //    auto h = static_cast<size_t>(r) * MAX_COLUMNS + min_column;
    for (auto c = min_column; c <= max_column; ++c)
    {
      // actually check x, y too since we care if the cell itself is burned
      //      if (!(*is_burned_)[h])
      if (!(*is_burned_)[Location(r, c).hash()])
      {
        return false;
      }
      //      ++h;
    }
  }
  return true;
}
void
IntensityMap::ignite(
  const HashSize hash_value
)
{
  burn(hash_value);
}
void
IntensityMap::burn(
  const HashSize hash_value
)
{
  lock_guard<mutex> lock(mutex_);
  // const auto is_new = !(*is_burned_)[location.hash()];
  // if (is_new || intensity_max_->at(location) < intensity)
  // {
  //   intensity_max_->set(location, intensity);
  // }
  // // update ros and direction if higher ros
  // if (is_new || rate_of_spread_at_max_->at(location) < ros)
  // {
  //   rate_of_spread_at_max_->set(location, ros);
  //   direction_of_spread_at_max_->set(location, static_cast<DegreesSize>(raz.asDegrees()));
  // }
  // // just set anyway since it's probably faster than checking if we should
  // (*is_burned_).set(location.hash());
  // if (check_valid)
  // {
  //   // FIX: new fire uses intensity = 1, ros = 0 so this breaks
  //   logging::check_fatal(0 >= intensity, "Negative or 0 intensity given: %d", intensity);
  //   logging::check_fatal(0 >= ros, "Negative or 0 ros given: %f", ros);
  // }
  if (!(*is_burned_)[hash_value])
  {
    intensity_max_->set(hash_value, 1);
    (*is_burned_).set(hash_value);
  }
}
MathSize
IntensityMap::fireSize() const
{
  lock_guard<mutex> lock(mutex_);
  return intensity_max_->fireSize();
}
map<HashSize, IntensitySize>::const_iterator
IntensityMap::cend() const noexcept
{
  return intensity_max_->data.cend();
}
map<HashSize, IntensitySize>::const_iterator
IntensityMap::cbegin() const noexcept
{
  return intensity_max_->data.cbegin();
}
}
