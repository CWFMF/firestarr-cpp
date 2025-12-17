/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include "IntensityMap.h"
#include "Model.h"
#include "Perimeter.h"
#include "Settings.h"
#include "unstable.h"
#include "Weather.h"
namespace fs
{
IntensityMap::IntensityMap(const Model& model) noexcept
  : model_(model), intensity_max_(model.environment().makeMap<IntensitySize>(false)),
    rate_of_spread_at_max_(model.environment().makeMap<MathSize>(false)),
    direction_of_spread_at_max_(model.environment().makeMap<DegreesSize>(false)),
    is_burned_{model.environment().unburnable()}
{ }
IntensityMap::IntensityMap(const IntensityMap& rhs)
  // : IntensityMap(rhs.model_, nullptr)
  : IntensityMap(rhs.model_)
{
  intensity_max_ = rhs.intensity_max_;
  rate_of_spread_at_max_ = rhs.rate_of_spread_at_max_;
  direction_of_spread_at_max_ = rhs.direction_of_spread_at_max_;
  is_burned_ = rhs.is_burned_;
}
void IntensityMap::applyPerimeter(const Perimeter& perimeter) noexcept
{
  logging::verbose("Applying burned cells");
  std::for_each(
    std::execution::par_unseq,
    perimeter.burned.cbegin(),
    perimeter.burned.cend(),
    [this](const auto& location) { ignite(location); }
  );
}
bool IntensityMap::canBurn(const Location& location) const { return !hasBurned(location); }
bool IntensityMap::hasBurned(const Location& location) const
{
  lock_guard<mutex> lock(mutex_);
  return is_burned_.at(location.hash());
}
bool IntensityMap::isSurrounded(const Location& location) const
{
  // implement here so we can just lock once
  lock_guard<mutex> lock(mutex_);
  const auto x = location.column();
  const auto y = location.row();
  const auto min_row = static_cast<Idx>(max(y - 1, 0));
  const auto max_row = min(y + 1, this->rows() - 1);
  const auto min_column = static_cast<Idx>(max(x - 1, 0));
  const auto max_column = min(x + 1, this->columns() - 1);
  for (auto r = min_row; r <= max_row; ++r)
  {
    for (auto c = min_column; c <= max_column; ++c)
    {
      // actually check x, y too since we care if the cell itself is burned
      if (!is_burned_.at(Location(r, c).hash()))
      {
        return false;
      }
    }
  }
  return true;
}
void IntensityMap::ignite(const Location& location)
{
  burn(location, 1, 0, fs::Direction::Invalid().value);
}
void IntensityMap::burn(
  const Location& location,
  IntensitySize intensity,
  MathSize ros,
  fs::Direction raz
)
{
  lock_guard<mutex> lock(mutex_);
  if (!is_burned_.at(location.hash()))
  {
    intensity_max_.set(location, intensity);
    rate_of_spread_at_max_.set(location, ros);
    direction_of_spread_at_max_.set(location, static_cast<DegreesSize>(raz.asDegrees().value));
    is_burned_.set(location.hash());
  }
  else
  {
    const auto intensity_old = intensity_max_.at(location);
    if (intensity_old < intensity)
    {
      intensity_max_.set(location, intensity);
    }
    // update ros and direction if higher ros
    const auto ros_old = rate_of_spread_at_max_.at(location);
    if (ros_old < ros)
    {
      rate_of_spread_at_max_.set(location, ros);
      direction_of_spread_at_max_.set(location, static_cast<DegreesSize>(raz.asDegrees().value));
    }
  }
}
FileList IntensityMap::save(const string_view dir, const string_view base_name) const
{
  lock_guard<mutex> lock(mutex_);
  const auto name_intensity = string(base_name) + "_intensity";
  const auto name_ros = string(base_name) + "_ros";
  const auto name_raz = string(base_name) + "_raz";
  // FIX: already done in IntensityObserver?
  auto files = intensity_max_.saveToFile(dir, name_intensity);
  // // HACK: writing a double to a tiff seems to not work?
  // double is way too much precision for outputs
  files.append_range(rate_of_spread_at_max_.saveToFile<float>(dir, name_ros));
  files.append_range(direction_of_spread_at_max_.saveToFile(dir, name_raz));
  return files;
}
MathSize IntensityMap::fireSize() const
{
  lock_guard<mutex> lock(mutex_);
  return intensity_max_.fireSize();
}
map<Location, IntensitySize>::const_iterator IntensityMap::cend() const noexcept
{
  return intensity_max_.data.cend();
}
map<Location, IntensitySize>::const_iterator IntensityMap::cbegin() const noexcept
{
  return intensity_max_.data.cbegin();
}
}
