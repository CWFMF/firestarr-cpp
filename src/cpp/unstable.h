/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2024 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

// Provides wrappers around functions that have different results when using
// different optimization flags, so they can be compiled with the same
// flags in release and debug mode to avoid changing the outputs.

#ifdef _WIN32
#endif

#include <cmath>
// HACK: alias these so unstable code doesn't change results of functions in debug vs release
// version
double
_cos(double angle) noexcept;
double
_sin(double angle) noexcept;
// instead of defining as a call just define directly from cmath functions
// #define _cos(x) (cos(x))
// #define _sin(x) (sin(x))
// constexpr auto _cos = cos;
// constexpr auto _sin = sin;
