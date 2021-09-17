/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "stdafx.h"
#include "TimeUtil.h"
namespace fs::util
{
/**
 * \brief Convert from TIMESTAMP_STRUCT to tm
 * \param s Input value
 * \param t Output value
 */
void
to_tm_no_fix(
  const TIMESTAMP_STRUCT& s,
  tm* t
) noexcept
{
  t->tm_year = s.year - 1900;
  t->tm_mon = s.month - 1;
  t->tm_mday = s.day;
  t->tm_hour = s.hour;
  t->tm_min = s.minute;
  t->tm_sec = s.second;
}
void
to_tm(
  const TIMESTAMP_STRUCT& s,
  tm* t
) noexcept
{
  // this doesn't set yday or other things properly, so convert to time and back
  to_tm_no_fix(s, t);
  const time_t t_t = mktime(t);
  t = localtime(&t_t);
}
void
to_tm_gm(
  const TIMESTAMP_STRUCT& s,
  tm* t
) noexcept
{
  // this doesn't set yday or other things properly, so convert to time and back
  to_tm_no_fix(s, t);
  const time_t t_t = mktime(t);
  // HACK: use gmtime_s instead of localtime_s so that it doesn't mess with hours based on DST
  t = gmtime(&t_t);
}
void
to_ts(
  const tm& t,
  TIMESTAMP_STRUCT* s
) noexcept
{
  s->year = static_cast<SQLSMALLINT>(t.tm_year + 1900);
  s->month = static_cast<SQLUSMALLINT>(t.tm_mon + 1);
  s->day = static_cast<SQLUSMALLINT>(t.tm_mday);
  s->hour = static_cast<SQLUSMALLINT>(t.tm_hour);
  s->minute = static_cast<SQLUSMALLINT>(t.tm_min);
  s->second = static_cast<SQLUSMALLINT>(t.tm_sec);
  s->fraction = 0;
}
}
