#ifndef HULL2D_H
#define HULL2D_H

#include "exclusionlist.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "InnerPos.h"
#define RANDOM 1   // random points

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

void
addEdge(point a, point b, edgeList* l);
void
cleanJunk();
void
click(int btn, int state, int x, int y);
// void copyImageToFIBITMAP(FIBITMAP *dib, Image* Im);
excList*
copyList(linkedList* l);
void
delEdge(edge* e);
void
delEdgeList(edgeList* l);
void
delList(linkedList* l);
void
delNodeX(NodeX* n);
double
distPtPt(fs::sim::InnerPos& a, fs::sim::InnerPos& b);
double
distLinePt(point* a, point* b, point* p);
// void drawpic(void);
linkedList*
gridList(int numX, int numY);
void
insertList(point p, linkedList* l);
// void inputClick(int btn, int state, int x, int y);
// void keyboard(unsigned char key, int x, int y);
// void moveKeys(int key, int x, int y);
edge*
newEdge(point a, point b, edge* next);
linkedList*
newList();
edgeList*
newEdgeList();
NodeX*
newNodeX(point p, NodeX* next);
void
peel(vector<fs::sim::InnerPos>& a);
void
quickHull(
  const vector<fs::sim::InnerPos>* a,
  vector<std::pair<fs::sim::InnerPos, fs::sim::InnerPos>>& edges,
  fs::sim::InnerPos& n1,
  fs::sim::InnerPos& n2
);

#endif
