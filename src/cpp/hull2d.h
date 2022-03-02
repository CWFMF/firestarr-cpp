#ifndef HULL2D_H
#define HULL2D_H
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "exclusionlist.h"
#include "InnerPos.h"
namespace fs
{
enum
{
  RANDOM = 1   // random points
};
// node in a point list (NodeX since excList uses node)
struct NodeX
{
  point pt;
  NodeX* next;
};
// list of points
struct linkedList
{
  NodeX* head;
  int length;
};
// edge in edge list
struct edge
{
  point a;
  point b;
  edge* next;
};
// list of edges
struct edgeList
{
  edge* first;
  int length;
};
void addEdge(point a, point b, edgeList* l);
void cleanJunk();
void click(int btn, int state, int x, int y);
excList* copyList(linkedList* l);
void delEdge(edge* e);
void delEdgeList(edgeList* l);
void delList(linkedList* l);
void delNodeX(NodeX* n);
double distPtPt(InnerPos& a, InnerPos& b);
double distLinePt(point* a, point* b, point* p);
linkedList* gridList(int numX, int numY);
void insertList(point p, linkedList* l);
edge* newEdge(point a, point b, edge* next);
linkedList* newList();
edgeList* newEdgeList();
NodeX* newNodeX(point p, NodeX* next);
void peel(vector<InnerPos>& a);
void quickHull(
  const vector<InnerPos>* a,
  vector<std::pair<InnerPos, InnerPos>>& edges,
  InnerPos& n1,
  InnerPos& n2
);
}
#endif
