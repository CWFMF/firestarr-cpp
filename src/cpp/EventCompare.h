/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef FS_EVENTCOMPARE_H
#define FS_EVENTCOMPARE_H

#include "Event.h"
namespace fs
{
namespace sim
{
/**
 * \brief Defines how Events are compared for sorting.
 */
struct EventCompare
{
  /**
   * \brief Defines ordering on Events
   * \param x First Event
   * \param y Second Event
   * \return Whether first Event is less than second Event
   */
  [[nodiscard]] constexpr bool
  operator()(
    const Event& x,
    const Event& y
  ) const
  {
    if (x.time() == y.time())
    {
      if (x.type() == y.type())
      {
        return x.cell().hash() < y.cell().hash();
      }
      return x.type() < y.type();
    }
    return x.time() < y.time();
  }
};
}
}
#endif
