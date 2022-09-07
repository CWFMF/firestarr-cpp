// Copyright (c) 2020-2021, Queen's Printer for Ontario.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "stdafx.h"
#include "FireWeather.h"
#include "FuelType.h"
namespace tbd::wx
{
/*!
 * \page weather Hourly fire and weather indices
 *
 * Hourly weather is read from input and used as-is with no validation.
 */
FireWeather::~FireWeather()
{
  delete weather_by_hour_by_day_;
  delete survival_probability_;
}
static unique_ptr<SurvivalMap> make_survival(
  const set<const fuel::FuelType*>& used_fuels,
  const Day min_date,
  const Day max_date,
  const vector<const FwiWeather*>& weather_by_hour_by_day)
{
  auto result = make_unique<SurvivalMap>();
  for (const auto& in_fuel : used_fuels)
  {
    if (nullptr != in_fuel && 0 != strcmp("Invalid", fuel::FuelType::safeName(in_fuel)))
    {
      // initialize with proper size
      const auto code = fuel::FuelType::safeCode(in_fuel);
      auto by_fuel = vector<float>{};
      by_fuel.resize((static_cast<size_t>(max_date) - min_date + 2) * DAY_HOURS);
      // calculate the entire stream for this fuel
      for (auto day = min_date; day <= max_date; ++day)
      {
        for (auto h = 0; h < DAY_HOURS; ++h)
        {
          const auto wx = weather_by_hour_by_day.at(util::time_index(day, h, min_date));
          const auto i = util::time_index(day, h, min_date);
          by_fuel.at(i) = static_cast<float>(nullptr != wx
                                               ? in_fuel->survivalProbability(*wx)
                                               : 0.0);
        }
      }
      result->at(code) = std::move(by_fuel);
    }
  }
  return result;
}
FireWeather::FireWeather(const set<const fuel::FuelType*>& used_fuels,
                         const Day min_date,
                         const Day max_date,
                         vector<const FwiWeather*>* weather_by_hour_by_day)
  : weather_by_hour_by_day_(weather_by_hour_by_day),
    survival_probability_(
      make_survival(used_fuels, min_date, max_date, *weather_by_hour_by_day).release()),
    min_date_(min_date),
    max_date_(max_date)
{
  weighted_dsr_ = 0;
  // make it so that dsr near start of scenario matters more
  auto weight = 1000000000.0;
  for (auto& w : *weather_by_hour_by_day_)
  {
    if (nullptr != w)
    {
      const auto dsr = 0.0272 * pow(w->fwi().asDouble(), 1.77);
      weighted_dsr_ += static_cast<size_t>(weight * dsr);
      weight *= 0.8;
    }
  }
}
}
