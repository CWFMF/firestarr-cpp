/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "SpreadCache.h"
#include "Scenario.h"
namespace fs
{
std::pair<map<SpreadKey, SpreadInfo>::iterator, bool> SpreadCache::add_spread(
  const SpreadKey& key,
  ptr<const Scenario> scenario,
  DurationSize time,
  const ptr<const FwiWeather> weather
) noexcept
{
  return spread_info_.try_emplace(
    key, *scenario, time, key, scenario->nd(time), weather, scenario->weather_daily(time)
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
