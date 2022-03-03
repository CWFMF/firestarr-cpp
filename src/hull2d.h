#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "InnerPos.h"

double distPtPt(firestarr::sim::InnerPos& a, firestarr::sim::InnerPos& b);
void hull(vector<firestarr::sim::InnerPos>& a);
void quickHull(const vector<firestarr::sim::InnerPos>& a, set<firestarr::sim::InnerPos>& hullPoints, firestarr::sim::InnerPos& n1, firestarr::sim::InnerPos& n2);
