/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include "unstable.h"
#include <cmath>

MathSize
_cos(
  const double angle
) noexcept
{
  return static_cast<MathSize>(cos(angle));
}
MathSize
_sin(
  const double angle
) noexcept
{
  return static_cast<MathSize>(sin(angle));
}
