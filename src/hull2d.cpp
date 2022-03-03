#include "hull2d.h"

//#define DEBUG_HULL

/*
 * Calculates distance from point a to point b
 */
double distPtPt(firestarr::sim::InnerPos& a, firestarr::sim::InnerPos& b)
{
  int abX = (b.sub_x - a.sub_x);
  int abY = (b.sub_y - a.sub_y);
  return (abX * abX + abY * abY);
}

void hull(vector<firestarr::sim::InnerPos>& a)
{
#ifdef DEBUG_HULL
  size_t orig_size = a.size();
#endif
//  // first thing is just making sure all points are unique
//  set<firestarr::sim::InnerPos> tmp{};
//  tmp.insert(a.cbegin(), a.cend());
//  a = {};
//  a.insert(a.end(), tmp.cbegin(), tmp.cend());
#ifdef DEBUG_HULL
  size_t set_size = a.size();
#endif
//  vector<std::pair<firestarr::sim::InnerPos, firestarr::sim::InnerPos>> edges{};
  set<firestarr::sim::InnerPos> hullPoints{};
  double maxX = std::numeric_limits<double>::min();
  double minX = std::numeric_limits<double>::max();
  firestarr::sim::InnerPos maxNode{0, 0, 0, 0};
  firestarr::sim::InnerPos minNode{0, 0, 0, 0};

  for (const auto p : a)
  {
    if (p.sub_x > maxX)
    {
      maxX = p.sub_x;
      maxNode = p;
    }
    if (p.sub_x < minX)
    {
      minX = p.sub_x;
      minNode = p;
    }
  }

  //get rid of max & min nodes & call quickhull
  if (maxNode != minNode)
  {
    a.erase(std::remove(a.begin(), a.end(), maxNode), a.end());
    a.erase(std::remove(a.begin(), a.end(), minNode), a.end());
    quickHull(a, hullPoints, minNode, maxNode);
    quickHull(a, hullPoints, maxNode, minNode);
    // points should all be unique
    a = {};
    a.insert(a.end(), hullPoints.cbegin(), hullPoints.cend());
  }
  else
  {
    // points might not be unique
      set<firestarr::sim::InnerPos> tmp{};
      tmp.insert(a.cbegin(), a.cend());
      a = {};
      a.insert(a.end(), tmp.cbegin(), tmp.cend());
  }
}

/*
 * Does quickhull, using an excList to push & pop Nodes so that it's a little faster
 */
void quickHull(const vector<firestarr::sim::InnerPos>& a, set<firestarr::sim::InnerPos>& hullPoints, firestarr::sim::InnerPos& n1, firestarr::sim::InnerPos& n2)
{
#ifdef DEBUG_HULL
  firestarr::logging::warning("Checking %d points", a->size());
#endif
  double maxD = -1;   //just make sure it's not >= 0
  firestarr::sim::InnerPos maxPos{0, 0, 0, 0};
  vector<firestarr::sim::InnerPos> usePoints{};

  //since we do distLinePt so often, calculate the parts that are always the same
  double abX = (n2.sub_x - n1.sub_x);
  double abY = (n2.sub_y - n1.sub_y);
  /* so instead of:
	 * return ( (b->x - a->x)*(a->y - p->y) - (a->x - p->x)*(b->y - a->y) );
	 * we can do the equivalent of:
	 * return ( abX*(a->y - p->y) - (a->x - p->x)*abY );
	 * for distance from the line n1n2 to the current point
	 */

  for (const auto p : a)
  {
    //loop through points, looking for furthest
    const double d = (abX * (n1.sub_y - p.sub_y) - (n1.sub_x - p.sub_x) * abY);
    if (d >= 0 && d > maxD)
    {               //if further away
      maxD = d;     //update max dist
      maxPos = p;   //update furthest Node
    }
    if (d >= 0)
    {
      // only use in next step if on positive side of line
#ifdef DEBUG_HULL
      firestarr::logging::warning("Adding point (%d, %d) (%f, %f)",
                                  p.x,
                                  p.y,
                                  p.sub_x,
                                  p.sub_y);
#endif
      usePoints.emplace_back(p);
    }
  }
  if (maxD == 0)
  {   //we have co-linear points
#ifdef DEBUG_HULL
    size_t before = usePoints->size();
#endif
    usePoints.erase(std::remove(usePoints.begin(), usePoints.end(), maxPos), usePoints.end());
#ifdef DEBUG_HULL
    size_t after = usePoints->size();
    firestarr::logging::check_fatal(before == after, "Remove did not get rid of point (%d, %d) (%f, %f)", maxPos.x, maxPos.y, maxPos.sub_x, maxPos.sub_y);
#endif
    //need to figure out which direction we're going in
    const double d1 = distPtPt(n1, maxPos);
    const double d2 = distPtPt(n1, n2);
    const double d3 = distPtPt(maxPos, n2);

    if (d1 < d2 && d3 < d2)
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
    //this is not an edge
#ifdef DEBUG_HULL
    size_t before = usePoints->size();
#endif
    usePoints.erase(std::remove(usePoints.begin(), usePoints.end(), maxPos), usePoints.end());
#ifdef DEBUG_HULL
    size_t after = usePoints->size();
    firestarr::logging::check_fatal(before == after, "Remove did not get rid of point (%d, %d) (%f, %f)", maxPos.x, maxPos.y, maxPos.sub_x, maxPos.sub_y);
    firestarr::logging::check_fatal(usePoints->size() == a->size(), "Recursing without eliminating any points");
#endif
    quickHull(usePoints, hullPoints, n1, maxPos);
    quickHull(usePoints, hullPoints, maxPos, n2);
  }
}
