/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include "CellPoints.h"
#include "FireSpread.h"
#include "Scenario.h"
#include "SpreadCache.h"
namespace fs
{
class Scenario;
void spread_points(
  CellPointsMap& result,
  const CellPoints& cell_pts,
  const OffsetSet& offsets_after_duration,
  const DurationSize arrival_time
) noexcept;
CellPointsMap spread_map(
  const BurnedData& unburnable,
  const SpreadCache& spread_info,
  const spreading_points& to_spread,
  const DurationSize new_time,
  const DurationSize duration
) noexcept;
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
