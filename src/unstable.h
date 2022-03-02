// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// Provides wrappers around functions that have different results when using
// different optimization flags, so they can be compiled with the same
// flags in release and debug mode to avoid changing the outputs.
#pragma once
#include <cmath>
double _cos(double angle) noexcept;
double _sin(double angle) noexcept;
