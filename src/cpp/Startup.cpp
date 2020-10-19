/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "Startup.h"
#include "TimeUtil.h"

namespace fs::wx
{
Startup::Startup(
  string station,
  const TIMESTAMP_STRUCT& generated,
  const topo::Point& point,
  const double distance_from,
  const Ffmc& ffmc,
  const Dmc& dmc,
  const Dc& dc,
  const AccumulatedPrecipitation& apcp_0800,
  const bool overridden
) noexcept
  : station_(std::move(station)),
    generated_(generated),
    point_(point),
    distance_from_(distance_from),
    ffmc_(ffmc),
    dmc_(dmc),
    dc_(dc),
    apcp_0800_(apcp_0800),
    is_overridden_(overridden)
{
}
}
