/* SPDX-FileCopyrightText: 2020 Queen's Printer for Ontario */
/* SPDX-FileCopyrightText: 2024-2025 Government of Canada */
/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "unstable.h"
#include <cmath>
namespace fs
{
MathSize cos(const MathSize angle) noexcept { return static_cast<MathSize>(std::cos(angle)); }
MathSize sin(const MathSize angle) noexcept { return static_cast<MathSize>(std::sin(angle)); }
}
