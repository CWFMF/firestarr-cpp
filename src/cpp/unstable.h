/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2024 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef FS_UNSTABLE_H
#define FS_UNSTABLE_H

// Provides wrappers around functions that have different results when using
// different optimization flags, so they can be compiled with the same
// flags in release and debug mode to avoid changing the outputs.
#include <cmath>
namespace fs
{
namespace sim
{
double
_cos(double angle) noexcept;
double
_sin(double angle) noexcept;
}
}
#endif
