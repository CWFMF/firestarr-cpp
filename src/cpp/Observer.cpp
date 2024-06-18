/* Copyright (c) Queen's Printer for Ontario, 2020. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "Observer.h"
namespace tbd::sim
{
string IObserver::makeName(const string& base_name, const string& suffix)
{
  if (base_name.length() > 0)
  {
    return base_name + "_" + suffix;
  }
  return suffix;
}
constexpr double NODATA_ARRIVAL = 0;
ArrivalObserver::ArrivalObserver(const Scenario& scenario)
  : MapObserver<double>(scenario, NODATA_ARRIVAL, "arrival")
{
#ifdef DEBUG_GRIDS
  // enforce converting to an int and back produces same V
  const auto n0 = NODATA_ARRIVAL;
  const auto n1 = static_cast<NodataIntType>(n0);
  const auto n2 = static_cast<double>(n1);
  const auto n3 = static_cast<NodataIntType>(n2);
  logging::check_equal(
    n1,
    n3,
    "nodata_value_ as int");
  logging::check_equal(
    n0,
    n2,
    "nodata_value_ from int");
#endif
}
double ArrivalObserver::getValue(const Event& event) const noexcept
{
  return event.time();
}
SourceObserver::SourceObserver(const Scenario& scenario)
  : MapObserver<CellIndex>(scenario, static_cast<CellIndex>(255), "source")
{
}
CellIndex SourceObserver::getValue(const Event& event) const noexcept
{
  return event.source();
}
IntensityObserver::IntensityObserver(const Scenario& scenario, string suffix) noexcept
  : scenario_(scenario), suffix_(std::move(suffix))
{
}
void IntensityObserver::handleEvent(const Event&) noexcept
{
  // HACK: do nothing because Scenario tracks intensity
}
void IntensityObserver::save(const string& dir, const string& base_name) const
{
  scenario_.saveIntensity(dir, makeName(base_name, suffix_));
}
void IntensityObserver::reset() noexcept
{
  // HACK: do nothing because Scenario tracks intensity
}
}
