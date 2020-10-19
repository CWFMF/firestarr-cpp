/* SPDX-FileCopyrightText: 2020-2021 Queen's Printer for Ontario */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef FS_FWISTREAM_H
#define FS_FWISTREAM_H

namespace fs
{
namespace wx
{
class FwiWeather;
class Weather;
class Startup;
using FwiVector = vector<FwiWeather>;
/**
 * \brief A stream of FwiWeather values used for a Scenario.
 */
class FwiStream
{
public:
  /**
   * \brief Construct from a series of FwiWeather
   * \param wx FwiVector to use for stream
   */
  explicit FwiStream(FwiVector&& wx) noexcept;
  /**
   * \brief Calculate based on weather and dates using startup/shutdown criteria
   * \param dates Dates that apply to Weather vector
   * \param startup Startup values to use
   * \param latitude Latitude to use for calculations
   * \param cur_member vector of Weather to use for calculations
   */
  FwiStream(
    const vector<TIMESTAMP_STRUCT>& dates,
    Startup* startup,
    double latitude,
    vector<Weather> cur_member
  );
  /**
   * \brief Weather stream as a vector of FwiWeather
   * \return Weather stream as a vector of FwiWeather
   */
  [[nodiscard]] const FwiVector&
  wx() const noexcept
  {
    return wx_;
  }
private:
  /**
   * \brief Weather stream as a vector of FwiWeather
   */
  FwiVector wx_;
};
}
}
#endif
