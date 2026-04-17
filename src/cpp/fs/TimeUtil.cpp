/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "TimeUtil.h"
#include "Log.h"
namespace fs
{
#ifdef _WIN32
struct tm* localtime_r(const time_t* timer, struct tm* result)
{
  if (localtime_s(result, timer))
  {
    return nullptr;
  }
  return result;
}
#endif
void fix_tm(tm* t)
{
  const time_t t_t = mktime(t);
  if (!localtime_r(&t_t, t))
  {
    logging::fatal("Unable to convert time from {}", t_t);
  }
}
}
