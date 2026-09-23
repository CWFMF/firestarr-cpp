/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "SpreadCache.h"
#include "Log.h"
#include "Scenario.h"
namespace fs
{
static MathSize find_min_ros(const Scenario& scenario, const DurationSize time)
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  const MathSize min_ros = settings.minimum_ros;
  return settings.deterministic ? min_ros : std::max(scenario.spreadThresholdByRos(time), min_ros);
}
std::pair<map<SpreadKey, SpreadInfo>::iterator, bool> SpreadCache::add_spread(
  const SpreadKey& key,
  ptr<const Scenario> scenario,
  DurationSize time,
  const ptr<const FwiWeather> weather
) noexcept
{
  logging::check_fatal(
    weather != scenario->weather(time), "scenario->weather({}) doesn't match passed weather", time
  );
  return spread_info_.try_emplace(
    key,
    time,
    find_min_ros(*scenario, time),
    scenario->cellSize(),
    key,
    scenario->nd(time),
    weather,
    scenario->weather_daily(time)
  );
}
SpreadCache::SpreadCacheMap::const_iterator SpreadCache::find(const SpreadKey& key) const noexcept
{
  return spread_info_.find(key);
}
SpreadCache::SpreadCacheMap::const_iterator SpreadCache::end() const noexcept
{
  return spread_info_.end();
}
MathSize SpreadCache::maxIntensity(const SpreadKey& key) const noexcept
{
  auto seek_spread = spread_info_.find(key);
  const auto max_intensity =
    (spread_info_.end() == seek_spread) ? 0 : seek_spread->second.maxIntensity();
  return max_intensity;
}
const OffsetSet& SpreadCache::offsets(const SpreadKey& key) const noexcept
{
  return spread_info_.at(key).offsets();
}
}
