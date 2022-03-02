#ifndef FS_RELINKABLE_H
#define FS_RELINKABLE_H
#include <stdio.h>
namespace fs
{
using point = struct
{             // may or may not be useful
  double x;   // x coord
  double y;   // y coord
};
/*
 * This structure is a Node in a excList, which contains a point, and has a link
 * to the Node after it
 */
using Node = struct nd
{
  point pt;         // point this node represents
  struct nd* fwd;   // node after this
  void reset() { fwd = nullptr; }
};
/*
 * This structure is a Frame in a excList, which points to both a list of Nodes
 * and the next Frame
 */
using Frame = struct frm
{
  Node* first;        // first node for this frame
  struct frm* next;   // next frame
  void reset()
  {
    first = nullptr;
    next = nullptr;
  }
};
/*
 * An excList is a list in which you can push & pop nodes, but there is no guarantee
 * that items will remain in the same order they were in
 */
using excList = struct
{
  Node* head;   // header node
  int length;   // size of list
  Frame* top;   // 1st Frame of stack
};
using iter = struct
{
  Node* prev;
  Node* cur;
  excList* l;
};
void addFrame(excList* l);
void chkList(excList* l);
void cleanup();
void delExcList(excList* l);
void delFrame(Frame* f);
void delNode(Node* n);
void insert(point p, excList* l);
excList* newExcList();
Frame* newFrame(Frame* next);
Node* newNode(point p, Node* fwd);
void popFrame(excList* l);
void pushAfterNode(Node* n, excList* l);
void removeAfterNode(Node* del, excList* l);
void showList(excList* l);
void showListFull(excList* l);
}
#endif
