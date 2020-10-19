/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_SCORE_H
#define FS_SCORE_H
#include "stdafx.h"
namespace fs
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
  [[nodiscard]] constexpr MathSize average() const noexcept { return average_; }
  /**
   * \brief Grade for this year
   * \return Grade for this year
   */
  [[nodiscard]] constexpr MathSize grade() const noexcept { return grade_; }
  /**
   * \brief Year this Score is for
   * \return Year this Score is for
   */
  [[nodiscard]] constexpr YearSize year() const noexcept { return year_; }
  /**
   * \brief Constructor
   * \param year Year Score is from
   * \param average Average match percentage
   * \param grade Calculated grade
   */
  constexpr Score(const YearSize year, const MathSize average, const MathSize grade) noexcept
    : average_(average), grade_(grade), year_(year)
  { }

private:
  /**
   * \brief Average match percentage for the year
   */
  MathSize average_;
  /**
   * \brief Grade for this year
   */
  MathSize grade_;
  /**
   * \brief Year this Score is for
   */
  YearSize year_;
};
}
#endif
