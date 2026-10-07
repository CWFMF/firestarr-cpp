/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "FwiWeather.h"
#include "FwiReference.h"
namespace fs
{
[[nodiscard]] MathSize FwiWeatherImpl::ffmcEffect() const { return ffmc_effect(ffmc); }
[[nodiscard]] MathSize FwiWeatherImpl::mcDmc() const { return mcDmcPct() / 100.0; }
[[nodiscard]] MathSize FwiWeatherImpl::mcFfmc() const { return mcFfmcPct() / 100.0; }
[[nodiscard]] MathSize FwiWeatherImpl::mcFfmcPct() const { return ffmc_to_moisture(ffmc); }
[[nodiscard]] MathSize FwiWeatherImpl::mcDmcPct() const
{
  return exp((dmc.value - 244.72) / -43.43) + 20;
}
mutex FwiWeather::mutex_ = {};
MathSize ffmc_to_moisture(const MathSize ffmc) noexcept
{
  return fwireference::ffmc_to_moisture(ffmc);
}
MathSize ffmc_to_moisture(const Ffmc& ffmc) noexcept
{
  return fwireference::ffmc_to_moisture(ffmc);
}
Ffmc moisture_to_ffmc(const MathSize m) noexcept { return fwireference::moisture_to_ffmc(m); }
Ffmc ffmc_from_moisture(const MathSize m) noexcept { return fwireference::ffmc_from_moisture(m); }
}
