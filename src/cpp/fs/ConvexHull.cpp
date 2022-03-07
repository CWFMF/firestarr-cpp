/* SPDX-License-Identifier: AGPL-3.0-or-later */
#include "ConvexHull.h"
namespace fs
{
constexpr double MIN_X = std::numeric_limits<double>::min();
constexpr double MAX_X = std::numeric_limits<double>::max();
inline constexpr double distPtPt(const InnerPos& a, const InnerPos& b) noexcept
{
#ifdef _WIN32
  return (((b.x - a.x) * (b.x - a.x)) + ((b.y - a.y) * (b.y - a.y)));
#else
  return (std::pow((b.x - a.x), 2) + std::pow((b.y - a.y), 2));
#endif
}
void hull(vector<InnerPos>& a) noexcept
{
  vector<InnerPos> hullPoints{};
  InnerPos maxPos{MIN_X, MIN_X};
  InnerPos minPos{MAX_X, MAX_X};
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
  // get rid of max & min nodes & call quickhull
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
void quickHull(
  const vector<InnerPos>& a,
  vector<InnerPos>& hullPoints,
  const InnerPos& n1,
  const InnerPos& n2
) noexcept
{
  double maxD = -1.0;   // just make sure it's not >= 0
  InnerPos maxPos{MIN_X, MIN_X};
  vector<InnerPos> usePoints{};
  // worst case scenario
  usePoints.reserve(a.size());
  // since we do distLinePt so often, calculate the parts that are always the same
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
    // loop through points, looking for furthest
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
  if (maxD > 0
      || (0 == maxD
          //we have co-linear points
          // if either of these isn't true then this must be an edge
          && (distPtPt(n1, maxPos) < distPtPt(n1, n2))
          && (distPtPt(maxPos, n2) < distPtPt(n1, n2))))
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
}
