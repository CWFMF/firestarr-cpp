/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2024 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */

// Provides wrappers around functions that have different results when using
// different optimization flags, so they can be compiled with the same
// flags in release and debug mode to avoid changing the outputs.

#ifdef _WIN32
#endif

/**
 * \brief Type used for math operations
 */
using MathSize = double;

MathSize
_cos(double angle) noexcept;
MathSize
_sin(double angle) noexcept;
