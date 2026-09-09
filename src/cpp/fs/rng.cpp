/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "stdafx.h"
#include "Settings.h"
namespace fs::rng
{
void make_threshold(
  vector<ThresholdSize>* thresholds,
  mt19937_64* mt,
  const Day start_day,
  const Day last_date,
  ThresholdSize (*convert)(double value)
)
{
  // HACK: resolve once and fail if not set already
  static const auto& settings = fs::settings::instance();
  const auto total_weight = settings.threshold_scenario_weight + settings.threshold_daily_weight
                          + settings.threshold_hourly_weight;
  uniform_real_distribution<ThresholdSize> rand(0.0, 1.0);
  const auto general = rand(*mt);
  for (size_t i = start_day; i < MAX_DAYS; ++i)
  {
    const auto daily = rand(*mt);
    for (auto h = 0; h < DAY_HOURS; ++h)
    {
      // generate no matter what so if we extend the time period the results
      // for the first days don't change
      const auto hourly = rand(*mt);
      // only save if we're going to use it
      // HACK: +1 so if it's exactly at the end time there's something there
      if (i <= static_cast<size_t>(last_date + 1))
      {
        // subtract from 1.0 because we want weight to make things more likely not less
        // ensure we stay between 0 and 1
        thresholds->at((i - start_day) * DAY_HOURS + h) = convert(max(
          0.0,
          min(
            1.0,
            1.0
              - (+settings.threshold_scenario_weight * general
                 + +settings.threshold_daily_weight * daily
                 + +settings.threshold_hourly_weight * hourly)
                  / total_weight
          )
        ));
      }
    }
  }
}
}
