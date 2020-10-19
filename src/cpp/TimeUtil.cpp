/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "TimeUtil.h"
namespace fs
{
void fix_tm(tm* t)
{
  const time_t t_t = mktime(t);
  t = localtime(&t_t);
}
}
