/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_EVENT_H
#define FS_EVENT_H
#include "stdafx.h"
#include <compare>
#include "Cell.h"
#include "FireSpread.h"
namespace fs
{
using fs::Direction;
/**
 * \brief A specific Event scheduled in a specific Scenario.
 */
struct Event
{
  /**
   * \brief Type of Event
   */
  enum class Type
  {
    Invalid,
    Save,
    EndSimulation,
    NewFire,
    FireSpread,
  };
  /**
   * \brief Cell representing no location
   */
  static constexpr Cell NoLocation{};
  /**
   * \brief Time of Event (decimal days)
   */
  DurationSize time{INVALID_TIME};
  /**
   * \brief Type of Event
   */
  Type type{Type::Invalid};
  /**
   * \brief Duration that Event Cell has been burning (decimal days)
   */
  DurationSize time_at_location{0.0};
  /**
   * \brief Cell Event takes place in
   */
  Cell cell{};
  /**
   * \brief Head fire rate of spread (m/min)
   */
  ROSSize ros{};
  /**
   * \brief Burn Intensity (kW/m)
   */
  IntensitySize intensity{};
  /**
   * \brief Head fire spread direction
   */
  Direction raz{INVALID_DIRECTION, false};
  /**
   * \brief CellIndex for relative Cell that spread into from
   */
  CellIndex source{};
  std::partial_ordering operator<=>(const Event& rhs) const;
};
}
#endif
