/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2025 Government of Canada */
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
IntensityMap::IntensityMap(
  const Model& model
) noexcept
  : model_(model),
    intensity_max_(model.environment().makeMap<IntensitySize>(false)),
    is_burned_(model.environment().unburnable())
{
}

IntensityMap::IntensityMap(
  const IntensityMap& rhs
)
  // : IntensityMap(rhs.model_, nullptr)
  : IntensityMap(rhs.model_)
{
  intensity_max_ = rhs.intensity_max_;
  is_burned_ = rhs.is_burned_;
}

void
IntensityMap::applyPerimeter(
  const Perimeter& perimeter
) noexcept
{
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
  return is_burned_.at(hash_value);
}

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
  if (!is_burned_.at(hash_value))
  {
    intensity_max_.set(hash_value, 1);
    is_burned_.set(hash_value);
  }
}

MathSize
IntensityMap::fireSize() const
{
  lock_guard<mutex> lock(mutex_);
  return intensity_max_.fireSize();
}

map<HashSize, IntensitySize>::const_iterator
IntensityMap::cend() const noexcept
{
  return intensity_max_.data.cend();
}

map<HashSize, IntensitySize>::const_iterator
IntensityMap::cbegin() const noexcept
{
  return intensity_max_.data.cbegin();
}
}
