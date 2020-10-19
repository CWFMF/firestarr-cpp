/* SPDX-FileCopyrightText: 2020-2021 Queen's Printer for Ontario */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "WeatherModel.h"
#include "TimeUtil.h"
namespace fs
{
using util::operator<;
using util::operator==;
namespace wx
{
WeatherModel::WeatherModel(
  const TIMESTAMP_STRUCT& generated,
  string&& name,
  const topo::Point& point,
  const double distance_from
) noexcept
  : generated_(generated),
    name_(std::forward<string>(name)),
    point_(point),
    distance_from_(distance_from)
{
}
WeatherModel&
WeatherModel::operator=(
  const WeatherModel& rhs
)
{
  if (this != &rhs)
  {
    generated_ = rhs.generated_;
    name_ = rhs.name_;
    point_ = rhs.point_;
    distance_from_ = rhs.distance_from_;
  }
  return *this;
}
bool
ModelCompare::operator()(
  const WeatherModel& x,
  const WeatherModel& y
) const noexcept
{
  const auto cs = x.name().compare(y.name());
  if (0 == cs)
  {
    if (x.generated() == y.generated())
    {
      // HACK: these should always be equal, so just check that they are
      assert(
        x.point().latitude() == y.point().latitude()
        && x.point().longitude() == y.point().longitude() && x.distanceFrom() == y.distanceFrom()
      );
      if (x.point().latitude() == y.point().latitude())
      {
        if (x.point().longitude() == y.point().longitude())
        {
          return x.distanceFrom() < y.distanceFrom();
        }
        return x.point().longitude() < y.point().longitude();
      }
      return x.point().latitude() < y.point().latitude();
    }
    return x.generated() < y.generated();
  }
  return -1 == cs;
}
}
}
