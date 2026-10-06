/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include "CellPoints.h"
#include "SpreadCache.h"
namespace fs
{
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
}
