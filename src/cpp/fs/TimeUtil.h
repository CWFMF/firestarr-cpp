/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_TIMEUTIL_H
#define FS_TIMEUTIL_H
#include <ctime>
namespace fs
{
#ifdef _WIN32
struct tm* localtime_r(const time_t* timer, struct tm* result);
#endif
/**
 * \brief Calculate tm fields from values already there
 * @param t tm object to update
 */
void fix_tm(tm* t);
}
#endif
