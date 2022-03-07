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

#include "ConvexHull.h"

constexpr double MIN_X = std::numeric_limits<double>::min();
constexpr double MAX_X = std::numeric_limits<double>::max();

inline double distPtPt(firestarr::sim::InnerPos& a, firestarr::sim::InnerPos& b) noexcept
{
  return (std::pow((b.x - a.x), 2) + std::pow((b.y - a.y), 2));
}

void hull(vector<firestarr::sim::InnerPos>& a) noexcept
{
  vector<firestarr::sim::InnerPos> hullPoints{};
  firestarr::sim::InnerPos maxPos{MIN_X, MIN_X};
  firestarr::sim::InnerPos minPos{MAX_X, MAX_X};

  for (const auto p : a)
  {
    if (p.x > maxPos.x)
    {
      maxPos = p;
    }
    // don't use else if because first point should be set for both
    if (p.x < minPos.x)
    {
      minPos = p;
    }
  }

  //get rid of max & min nodes & call quickhull
  if (maxPos != minPos)
  {
    a.erase(std::remove(a.begin(), a.end(), maxPos), a.end());
    a.erase(std::remove(a.begin(), a.end(), minPos), a.end());
    quickHull(a, hullPoints, minPos, maxPos);
    quickHull(a, hullPoints, maxPos, minPos);
    // make sure we have unique points
    std::sort(hullPoints.begin(), hullPoints.end());
    hullPoints.erase(std::unique(hullPoints.begin(), hullPoints.end()), hullPoints.end());
    std::swap(a, hullPoints);
  }
}

void quickHull(const vector<firestarr::sim::InnerPos>& a,
               vector<firestarr::sim::InnerPos>& hullPoints,
               firestarr::sim::InnerPos& n1,
               firestarr::sim::InnerPos& n2) noexcept
{
  double maxD = -1.0;   //just make sure it's not >= 0
  firestarr::sim::InnerPos maxPos{MIN_X, MIN_X};
  vector<firestarr::sim::InnerPos> usePoints{};
  // worst case scenario
  usePoints.reserve(a.size());

  //since we do distLinePt so often, calculate the parts that are always the same
  const auto abX = (n2.x - n1.x);
  const auto abY = (n2.y - n1.y);
  /* so instead of:
	 * return ( (b->x - a->x)*(a->y - p->y) - (a->x - p->x)*(b->y - a->y) );
	 * we can do the equivalent of:
	 * return ( abX*(a->y - p->y) - (a->x - p->x)*abY );
	 * for distance from the line n1n2 to the current point
	 */

  for (const auto p : a)
  {
    //loop through points, looking for furthest
    const auto d = (abX * (n1.y - p.y) - (n1.x - p.x) * abY);
    if (d >= 0)
    {
      if (d > maxD)
      {
        // if further away
        if (maxD >= 0)
        {
          // already have one, so add old one to the list
          // NOTE: delayed add instead of erasing maxPos later
          usePoints.emplace_back(maxPos);
        }
        // update max dist
        maxD = d;
        maxPos = p;
      }
      else
      {
        // only use in next step if on positive side of line
        usePoints.emplace_back(p);
      }
    }
  }
  auto is_not_edge = maxD > 0;
  if (0 == maxD)
  {
    //we have co-linear points
    //need to figure out which direction we're going in
    const auto d2 = distPtPt(n1, n2);

    // if either of these isn't true then this must be an edge
    is_not_edge = (distPtPt(n1, maxPos) < d2) && (distPtPt(maxPos, n2) < d2);
  }
  if (is_not_edge)
  {
    // this is not an edge, so recurse on the lines between n1, n2, & maxPos
    quickHull(usePoints, hullPoints, n1, maxPos);
    quickHull(usePoints, hullPoints, maxPos, n2);
  }
  else
  {
    // n1 -> n2 must be an edge
    hullPoints.emplace_back(n1);
    // Must add n2 as the first point of a different line
  }
}
