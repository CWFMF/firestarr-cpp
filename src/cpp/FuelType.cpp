/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "FireSpread.h"
#include "FuelType.h"
#include "FireWeather.h"
#include "Log.h"
namespace fs::fuel
{
MathSize InvalidFuel::grass_curing(const int, const wx::FwiWeather&) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
MathSize
InvalidFuel::cbh() const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
MathSize
InvalidFuel::cfl() const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
MathSize
InvalidFuel::buiEffect(
  MathSize
) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
MathSize
InvalidFuel::crownConsumption(
  MathSize
) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
MathSize
InvalidFuel::calculateRos(
  const int,
  const wx::FwiWeather&,
  MathSize
) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
MathSize
InvalidFuel::calculateIsf(
  const SpreadInfo&,
  MathSize
) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
MathSize InvalidFuel::surfaceFuelConsumption(const SpreadInfo&) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
MathSize
InvalidFuel::lengthToBreadth(
  MathSize
) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
MathSize
InvalidFuel::finalRos(
  const SpreadInfo&,
  MathSize,
  MathSize,
  MathSize
) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
MathSize InvalidFuel::criticalSurfaceIntensity(const SpreadInfo&) const
{
  throw runtime_error("Invalid fuel type in fuel map");
}
MathSize
InvalidFuel::crownFractionBurned(
  MathSize,
  MathSize
) const noexcept
{
  return logging::fatal<MathSize>("Invalid fuel type in fuel map");
}
MathSize
InvalidFuel::probabilityPeat(
  MathSize
) const noexcept
{
  return logging::fatal<MathSize>("Invalid fuel type in fuel map");
}
MathSize InvalidFuel::survivalProbability(const wx::FwiWeather&) const noexcept
{
  return logging::fatal<MathSize>("Invalid fuel type in fuel map");
}
}
