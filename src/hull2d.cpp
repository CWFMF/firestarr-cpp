#include "hull2d.h"
#include <malloc.h>
#include "stdafx.h"

//#define DEBUG_HULL

/*
 * Calculates distance from point a to point b
 */
double distPtPt(firestarr::sim::InnerPos& a, firestarr::sim::InnerPos& b)
{
	int abX = (b.sub_x - a.sub_x);
  int abY = (b.sub_y - a.sub_y);
  return (abX*abX + abY*abY);
}
/*
 * does peel by repeatedly calling hull
 */
void peel(vector<firestarr::sim::InnerPos>& a) {
  vector<std::pair<firestarr::sim::InnerPos, firestarr::sim::InnerPos>> edges{};
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
  if (maxNode != minNode) {
    a.erase(std::remove(a.begin(), a.end(), maxNode), a.end());
    a.erase(std::remove(a.begin(), a.end(), minNode), a.end());
    quickHull(&a, edges, minNode, maxNode);
    quickHull(&a, edges, maxNode, minNode);
  }
  size_t i = 0;
  std::set<firestarr::sim::InnerPos> tmp{};
  for (const auto e : edges)
  {
    ++i;
    tmp.emplace(std::get<0>(e));
  }
  // HACK: does this need to happen?
  tmp.emplace(maxNode);
  if (maxNode != minNode)
  {
    tmp.emplace(minNode);
  }
  a =  {};
  a.insert(a.end(), tmp.cbegin(), tmp.cend());
}

/*
 * Does quickhull, using an excList to push & pop Nodes so that it's a little faster
 */
void quickHull(const vector<firestarr::sim::InnerPos>* a, vector<std::pair<firestarr::sim::InnerPos, firestarr::sim::InnerPos>>& edges, firestarr::sim::InnerPos& n1, firestarr::sim::InnerPos& n2) {
  if (a->empty())
  {
    return;
  }
#ifdef DEBUG_HULL
	firestarr::logging::warning("Checking %d points", a->size());
#endif
  double maxD = -1;				//just make sure it's not >= 0
	firestarr::sim::InnerPos maxPos{0, 0, 0, 0};
  double d;
  double d1,d2,d3;
  // HACK: use ptr so this isn't on the stack
  auto usePoints = new vector<firestarr::sim::InnerPos>();

	//since we do distLinePt so often, calculate the parts that are always the same
	double abX =(n2.sub_x - n1.sub_x);
	double abY = (n2.sub_y - n1.sub_y);
	/* so instead of:
	 * return ( (b->x - a->x)*(a->y - p->y) - (a->x - p->x)*(b->y - a->y) );
	 * we can do the equivalent of:
	 * return ( abX*(a->y - p->y) - (a->x - p->x)*abY );
	 * for distance from the line n1n2 to the current point
	 */

  for (const auto p : *a)
  {
    //loop through points, looking for furthest
		d = ( abX * (n1.sub_y - p.sub_y) - (n1.sub_x - p.sub_x) * abY );
		if (d >= 0 && d > maxD) {						//if further away
			maxD = d;						//update max dist
			maxPos = p;			//update furthest Node
		}
		if (d < 0) {					//if > maxD must be at least 0, so do else if				
      // we don't care about this point?
		}
		else {							//only move forward if didn't push
#ifdef DEBUG_HULL
      firestarr::logging::warning("Adding point (%d, %d) (%f, %f)",
                                  p.x, p.y, p.sub_x, p.sub_y);
#endif
      usePoints->emplace_back(p);
		}
	}
	if (maxD == 0) {							//we have co-linear points
#ifdef DEBUG_HULL
    size_t before = usePoints->size();
#endif
    usePoints->erase(std::remove(usePoints->begin(), usePoints->end(), maxPos), usePoints->end());
#ifdef DEBUG_HULL
    size_t after = usePoints->size();
    firestarr::logging::check_fatal(before == after, "Remove did not get rid of point (%d, %d) (%f, %f)",
                                    maxPos.x, maxPos.y, maxPos.sub_x, maxPos.sub_y);
#endif
		//need to figure out which direction we're going in
		d1 = distPtPt(n1, maxPos);
		d2 = distPtPt(n1, maxPos);
		d3 = distPtPt(maxPos, n2);
		
		if (d1 < d2 && d3 < d2) {				//maxNode bet n1 & n2*/
#ifdef DEBUG_HULL
      firestarr::logging::check_fatal(usePoints->size() == a->size(), "Recursing without eliminating any points");
#endif
			quickHull(usePoints, edges, n1, maxPos);
			quickHull(usePoints, edges, maxPos, n2);
		}
		//n1 -> n2 must be an edge, but then maxNode is on one side of them
		else {
      edges.emplace_back(n1, n2);
		}
	}
	else if (maxD < 0) {					//no valid points, this must be edge
    edges.emplace_back(n1, n2);
	}
	else {										//this is not an edge
#ifdef DEBUG_HULL
    size_t before = usePoints->size();
#endif
    usePoints->erase(std::remove(usePoints->begin(), usePoints->end(), maxPos), usePoints->end());
#ifdef DEBUG_HULL
    size_t after = usePoints->size();
    firestarr::logging::check_fatal(before == after, "Remove did not get rid of point (%d, %d) (%f, %f)",
                                    maxPos.x, maxPos.y, maxPos.sub_x, maxPos.sub_y);
    firestarr::logging::check_fatal(usePoints->size() == a->size(), "Recursing without eliminating any points");
#endif
		quickHull(usePoints, edges, n1, maxPos);
		quickHull(usePoints, edges, maxPos, n2);
	}
  delete usePoints;
}

