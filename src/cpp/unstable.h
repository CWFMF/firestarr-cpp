/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2024 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

// Provides wrappers around functions that have different results when using
// different optimization flags, so they can be compiled with the same
// flags in release and debug mode to avoid changing the outputs.
#include <cmath>
double
_cos(double angle) noexcept;
double
_sin(double angle) noexcept;
