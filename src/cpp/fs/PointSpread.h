/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include "CellPoints.h"
#include "Scenario.h"
#include "SpreadCache.h"
namespace fs
{
class Scenario;
// time spread went to or -1 if no spread
DurationSize do_spread(
  MathSize& max_ros,
  CellPointsMap& points,
  SpreadCache& spread_info,
  ptr<const Scenario> scenario,
  const BurnedData& unburnable,
  const DurationSize time,
  const DurationSize max_duration
) noexcept;
}
