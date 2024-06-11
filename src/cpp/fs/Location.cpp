/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "Location.h"
namespace fs
{
CellIndex relativeIndex(const Location& src, const Location& dst) noexcept
{
  static constexpr CellIndex DIRECTIONS[9] = {
    DIRECTION_SW,
    DIRECTION_S,
    DIRECTION_SE,
    DIRECTION_W,
    DIRECTION_NONE,
    DIRECTION_E,
    DIRECTION_NW,
    DIRECTION_N,
    DIRECTION_NE
  };
  return DIRECTIONS[((src.column() - dst.column()) + 1) + 3 * ((src.row() - dst.row()) + 1)];
}
}
