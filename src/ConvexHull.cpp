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

//#define DEBUG_HULL
constexpr double MIN_X = std::numeric_limits<double>::min();
constexpr double MAX_X = std::numeric_limits<double>::max();

inline double distPtPt(firestarr::sim::InnerPos& a, firestarr::sim::InnerPos& b)
{
  const auto abX = (b.x - a.x);
  const auto abY = (b.y - a.y);
  return (abX * abX + abY * abY);
}

void hull(vector<firestarr::sim::InnerPos>& a)
{
  set<firestarr::sim::InnerPos> hullPoints{};
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
    // points should all be unique, so just insert them
    a = {};
    a.insert(a.end(), hullPoints.cbegin(), hullPoints.cend());
  }
//  else
//  {
//    // points might not be unique, so use a set<> to make sure they are
//    set<firestarr::sim::InnerPos> tmp{};
//    tmp.insert(a.cbegin(), a.cend());
//    a = {};
//    a.insert(a.end(), tmp.cbegin(), tmp.cend());
//  }
}


vector<firestarr::sim::InnerPos> hull(map<firestarr::topo::Cell, vector<firestarr::sim::InnerPos>>& a)
{
//  size_t before = 0;
  vector<firestarr::sim::InnerPos> pts{};
  set<firestarr::sim::InnerPos> hullPoints{};
  firestarr::sim::InnerPos maxPos{MIN_X, MIN_X};
  firestarr::sim::InnerPos minPos{MAX_X, MAX_X};

  for (const auto& kv : a)
  {
    for (const auto& p : kv.second)
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
      pts.emplace_back(p);
//      ++before;
    }
  }
  if (pts.empty())
  {
    return pts;
  }
  //get rid of max & min nodes & call quickhull
  if (maxPos != minPos)
  {
    pts.erase(std::remove(pts.begin(), pts.end(), maxPos), pts.end());
    pts.erase(std::remove(pts.begin(), pts.end(), minPos), pts.end());
    quickHull(pts, hullPoints, minPos, maxPos);
    quickHull(pts, hullPoints, maxPos, minPos);
    // points should all be unique, so just insert them
    pts = {};
    pts.insert(pts.end(), hullPoints.cbegin(), hullPoints.cend());
  }
//  else
//  {
//    // points might not be unique, so use a set<> to make sure they are
//    set<firestarr::sim::InnerPos> tmp{};
//    tmp.insert(pts.cbegin(), pts.cend());
//    pts = {};
//    pts.insert(pts.end(), tmp.cbegin(), tmp.cend());
//  }
//  firestarr::logging::warning("Started with %d points and ended with %d", before, pts.size());
  return pts;
}

void quickHull(const vector<firestarr::sim::InnerPos>& a, set<firestarr::sim::InnerPos>& hullPoints, firestarr::sim::InnerPos& n1, firestarr::sim::InnerPos& n2)
{
#ifdef DEBUG_HULL
  firestarr::logging::warning("Checking %d points", a->size());
#endif
  double maxD = -1.0;   //just make sure it's not >= 0
  firestarr::sim::InnerPos maxPos{std::numeric_limits<double>::min(), std::numeric_limits<double>::min()};
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
      {               // if further away
        maxD = d;     // update max dist
        maxPos = p;
      }
      // only use in next step if on positive side of line
#ifdef DEBUG_HULL
      firestarr::logging::warning("Adding point (%d, %d) (%f, %f)",
                                  p.x,
                                  p.y,
                                  p.x,
                                  p.y);
#endif
      usePoints.emplace_back(p);
    }
  }
  if (maxD == 0)
  {
//we have co-linear points
#ifdef DEBUG_HULL
    size_t before = usePoints->size();
#endif
    usePoints.erase(std::remove(usePoints.begin(), usePoints.end(), maxPos), usePoints.end());
#ifdef DEBUG_HULL
    size_t after = usePoints->size();
    firestarr::logging::check_fatal(before == after, "Remove did not get rid of point (%d, %d) (%f, %f)", maxPos.x, maxPos.y, maxPos.x, maxPos.y);
#endif
    //need to figure out which direction we're going in
    const auto d1 = distPtPt(n1, maxPos);
    const auto d2 = distPtPt(n1, n2);

    // if either of these isn't true then this must be an edge
    auto is_not_edge = (d1 < d2) && (distPtPt(maxPos, n2) < d2);
    if (is_not_edge)
    {
      // maxNode is between n1 & n2
#ifdef DEBUG_HULL
      firestarr::logging::check_fatal(usePoints->size() == a->size(), "Recursing without eliminating any points");
#endif
      quickHull(usePoints, hullPoints, n1, maxPos);
      quickHull(usePoints, hullPoints, maxPos, n2);
    }
    //n1 -> n2 must be an edge, but then maxNode is on one side of them
    else
    {
      hullPoints.emplace(n1);
      hullPoints.emplace(n2);
    }
  }
  else if (maxD < 0)
  {
    //no valid points, this must be edge
    hullPoints.emplace(n1);
    hullPoints.emplace(n2);
  }
  else
  {
    //this is not an edge, so recurse on the lines between n1, n2, & maxPos
#ifdef DEBUG_HULL
    size_t before = usePoints->size();
#endif
    usePoints.erase(std::remove(usePoints.begin(), usePoints.end(), maxPos), usePoints.end());
#ifdef DEBUG_HULL
    size_t after = usePoints->size();
    firestarr::logging::check_fatal(before == after, "Remove did not get rid of point (%d, %d) (%f, %f)", maxPos.x, maxPos.y, maxPos.x, maxPos.y);
    firestarr::logging::check_fatal(usePoints->size() == a->size(), "Recursing without eliminating any points");
#endif
    quickHull(usePoints, hullPoints, n1, maxPos);
    quickHull(usePoints, hullPoints, maxPos, n2);
  }
}
