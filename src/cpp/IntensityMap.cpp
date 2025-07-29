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
    unburnable_(model.environment().unburnable()),
    arrival_({}),
    is_burned_(model.environment().unburnable())
{
}

void
IntensityMap::applyPerimeter(
  const Perimeter& perimeter
) noexcept
{
  logging::verbose("Applying burned cells");
  // NOTE: this breaks if burned is a vector and not a list
  std::for_each(
    std::execution::par_unseq,
    perimeter.burned.cbegin(),
    perimeter.burned.cend(),
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
  if (!is_burned_.at(hash_value))
  {
    intensity_max_.set(hash_value, 1);
    is_burned_.set(hash_value);
  }
}

bool
IntensityMap::cannotSpread(
  const HashSize hash_value
) const
{
  return unburnable_.at(hash_value);
}

void
IntensityMap::burn(
  const Event& event
)
{
  lock_guard<mutex> lock(mutex_);
  const auto hash_value = event.cell().hash();
#ifdef DEBUG_SIMULATION
  logging::check_fatal(
    hasBurned(hash_value),
    "Re-burning cell (%d, %d)",
    event.cell().column(),
    event.cell().row()
  );
#endif
#ifdef DEBUG_POINTS
  logging::check_fatal(
    cannotSpread(hash_value),
    "Burning unburnable cell (%d, %d)",
    event.cell().column(),
    event.cell().row()
  );
#endif
  ignite(hash_value);
#ifdef DEBUG_GRIDS
  logging::check_fatal(hasBurned(hash_value), "Wasn't marked as burned after burn");
#endif
  arrival_[hash_value] = event.time();
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

bool
IntensityMap::hasNotBurned(
  const HashSize hash_value
) const
{
  return canBurn(hash_value);
}

bool
IntensityMap::isUnburnable(
  const HashSize hash_value
) const
{
  return model_.isUnburnable(hash_value);
}
}
