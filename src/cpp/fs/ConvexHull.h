/* SPDX-License-Identifier: AGPL-3.0-or-later */
#ifndef FS_CONVEXHULL_H
#define FS_CONVEXHULL_H
#include "Cell.h"
#include "InnerPos.h"
namespace fs
{
/**
 * Calculates distance from point a to point b (squared I think? - we only
 * care about relative values, so no need to do sqrt)
 * @param a First point
 * @param b Second point
 * @return 'distance' from point a to point b
 */
inline double distPtPt(InnerPos& a, InnerPos& b);
/**
 * Find a convex hull for the points in the given vector and modify the
 * input to only have the hull points on return
 * @param a Points to find a convex hull for
 */
void hull(vector<InnerPos>& a);
vector<InnerPos> hull(map<Cell, vector<InnerPos>>& a);
/**
 * Implementation of the quickhull algorithm to find a convex hull.
 * @param a Points to find hull for
 * @param hullPoints Points already in the hull
 * @param n1 First point
 * @param n2 Second point
 */
void quickHull(const vector<InnerPos>& a, set<InnerPos>& hullPoints, InnerPos& n1, InnerPos& n2);
}
#endif
