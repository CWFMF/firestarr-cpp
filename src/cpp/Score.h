/* SPDX-FileCopyrightText: 2020-2021 Queen's Printer for Ontario */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef FS_SCORE_H
#define FS_SCORE_H

#include "stdafx.h"
namespace fs
{
namespace wx
{
/**
 * \brief The WeatherSHIELD score for a specific year.
 */
class Score
{
public:
  /**
   * \brief Average match percentage for the year
   * \return Average match percentage for the year
   */
  [[nodiscard]] constexpr double
  average() const noexcept
  {
    return average_;
  }
  /**
   * \brief Grade for this year
   * \return Grade for this year
   */
  [[nodiscard]] constexpr double
  grade() const noexcept
  {
    return grade_;
  }
  /**
   * \brief Year this Score is for
   * \return Year this Score is for
   */
  [[nodiscard]] constexpr int
  year() const noexcept
  {
    return year_;
  }
  /**
   * \brief Constructor
   * \param year Year Score is from
   * \param average Average match percentage
   * \param grade Calculated grade
   */
  constexpr Score(
    const int year,
    const double average,
    const double grade
  ) noexcept
    : average_(average),
      grade_(grade),
      year_(year)
  {
  }
private:
  /**
   * \brief Average match percentage for the year
   */
  double average_;
  /**
   * \brief Grade for this year
   */
  double grade_;
  /**
   * \brief Year this Score is for
   */
  int year_;
};
}
}
#endif
