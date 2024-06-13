/* Copyright (c) Queen's Printer for Ontario, 2020. */
/* Copyright (c) His Majesty the King in Right of Canada as represented by the Minister of Natural Resources, 2021-2024. */

/* SPDX-License-Identifier: AGPL-3.0-or-later */

#pragma once
#include "Location.h"
namespace tbd::topo
{
CellIndex relativeIndex(const Location& for_cell, const Location& from_cell)
{
  const auto r = for_cell.row();
  const auto r_o = from_cell.row();
  const auto c = for_cell.column();
  const auto c_o = from_cell.column();
  const auto r_d = r_o - r;
  const auto c_d = c_o - c;
  const auto h = (c_d + 1) + 3 * (r_d + 1);
  static const CellIndex DIRECTIONS[9] =
    {
      DIRECTION_SW,
      DIRECTION_S,
      DIRECTION_SE,
      DIRECTION_W,
      DIRECTION_NONE,
      DIRECTION_E,
      DIRECTION_NW,
      DIRECTION_N,
      DIRECTION_NE};
  return DIRECTIONS[h];
}
}
