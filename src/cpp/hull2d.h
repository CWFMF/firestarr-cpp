
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "InnerPos.h"
namespace fs
{
double distPtPt(InnerPos& a, InnerPos& b);
void hull(vector<InnerPos>& a);
void quickHull(const vector<InnerPos>& a, set<InnerPos>& hullPoints, InnerPos& n1, InnerPos& n2);
}
