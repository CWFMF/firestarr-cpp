
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "InnerPos.h"

double
distPtPt(fs::sim::InnerPos& a, fs::sim::InnerPos& b);
void
hull(vector<fs::sim::InnerPos>& a);
void
quickHull(
  const vector<fs::sim::InnerPos>& a,
  set<fs::sim::InnerPos>& hullPoints,
  fs::sim::InnerPos& n1,
  fs::sim::InnerPos& n2
);
