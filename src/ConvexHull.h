// Copyright (c) 2005-2022, Jordan Evens
//
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

#pragma once

#include "InnerPos.h"

/**
 * Calculates distance from point a to point b (squared I think? - we only
 * care about relative values, so no need to do sqrt)
 * @param a First point
 * @param b Second point
 * @return 'distance' from point a to point b
 */
inline double distPtPt(firestarr::sim::InnerPos& a, firestarr::sim::InnerPos& b) noexcept;

/**
 * Find a convex hull for the points in the given vector and modify the
 * input to only have the hull points on return
 * @param a Points to find a convex hull for
 */
void hull(vector<firestarr::sim::InnerPos>& a) noexcept;

/**
 * Implementation of the quickhull algorithm to find a convex hull.
 * @param a Points to find hull for
 * @param hullPoints Points already in the hull
 * @param n1 First point
 * @param n2 Second point
 */
void quickHull(const vector<firestarr::sim::InnerPos>& a,
               vector<firestarr::sim::InnerPos>& hullPoints,
               firestarr::sim::InnerPos& n1,
               firestarr::sim::InnerPos& n2) noexcept;
