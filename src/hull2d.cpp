#include "hull2d.h"
#include <malloc.h>
#include "stdafx.h"

edge* junkEdges = nullptr;
NodeX* junkNodeXs = nullptr;


/*
 * adds edge (a,b) to given linkedList
 */
void addEdge(point a, point b, edgeList* l) {
	l->first = newEdge(a, b, l->first);
	l->length++;
}

/*
 * gets rid of all junk edges & nodeXs
 */
void cleanJunk() {
	NodeX* curNodeX = junkNodeXs;
	NodeX* tmpNodeX;
	edge* curEdge = junkEdges;
	edge* tmpEdge;

	while (curNodeX != nullptr) {
		tmpNodeX = curNodeX;
		curNodeX = curNodeX->next;
		free(tmpNodeX);
	}
	junkNodeXs = nullptr;

	while (curEdge != nullptr) {
		tmpEdge = curEdge;
		curEdge = curEdge->next;
		free(tmpEdge);
	}
	junkEdges = nullptr;
}

/*
 * copies given linkedList to an excList
 */
excList* copyList(linkedList* l) {
	NodeX* cur = l->head->next;
	excList* ret = newExcList();
	while(cur != nullptr) {
		insert(cur->pt,ret);
		cur = cur->next;
	}
	return ret;
}

/*
 * deletes given edge
 */
void delEdge(edge* e) {
	e->next = junkEdges;
	junkEdges = e;
}

/*
 * delete edge linkedList
 */
void delEdgeList(edgeList* l) {
	edge* tmp;
	edge* cur = l->first;

	while (cur != nullptr) {
		tmp = cur;
		cur = cur->next;
		delEdge(tmp);
	}
	free(l);
}

/*
 * deletes given linkedList
 */
void delList(linkedList* l) {
	NodeX* cur = l->head;
	NodeX* tmp;

	while (cur != nullptr) {
		tmp = cur;
		cur = cur->next;
		delNodeX(tmp);
	}
	free(l);
}

/*
 * deletes given NodeX
 */
void delNodeX(NodeX* n) {
	n->next = junkNodeXs;
	junkNodeXs = n;
}

/*
 * Calculates distance from line (a,b) to point p
 */
double distLinePt(point* a,point* b,point* p) {
	return ( (b->x - a->x)*(a->y - p->y) - (a->x - p->x)*(b->y - a->y) );
}

/*
 * Calculates distance from point a to point b
 */
double distPtPt(point* a,point* b) {
	int abX = (b->x - a->x);
    int abY = (b->y - a->y);
	
	return (abX*abX + abY*abY);
}
/*
 * inserts point in given linkedList
 */
void insertList(point p, linkedList* l) {		//insert at end
	l->head->next = newNodeX(p,l->head->next);
	l->length++;
}

/*
 * gets new edge, either using malloc or an edge from junkEdges
 */
edge* newEdge(point a, point b, edge* next) {
	edge* ret;
	if (junkEdges != nullptr) {
		ret = junkEdges;
		junkEdges = junkEdges->next;
	}
	else {
		ret = (edge*)malloc(sizeof(edge));
	}
	ret->a = a;
	ret->b = b;
	ret->next = next;
	return ret;
}

/*
 * makes new edgelist
 */
edgeList* newEdgeList() {
	edgeList* ret = (edgeList*)malloc(sizeof(edgeList));
	ret->length = 0;
	ret->first = nullptr;
	return ret;
}

/*
 * makes new linkedList
 */
linkedList* newList() {
  linkedList* ret;// = (linkedList*)malloc(sizeof(linkedList));
	point p = {0,0};
	ret = (linkedList*)malloc(sizeof(linkedList));
	ret->head = newNodeX(p,nullptr);
	ret->length = 0;
	return ret;
}

/*
 * gets new NodeX, either using malloc or one from junkNodeXs
 */
NodeX* newNodeX(point p, NodeX* next) {
	NodeX* ret;// = (NodeX*)malloc(sizeof(NodeX));
	if (junkNodeXs) {
		ret = junkNodeXs;
		junkNodeXs = ret->next;
	}
	else {
		ret = (NodeX*)malloc(sizeof(NodeX));
	}
	ret->pt = p;
	ret->next = next;
	return ret;
}

/*
 * does peel by repeatedly calling hull
 */
edgeList* peel(excList* l) {
  edgeList* edges = newEdgeList();
	double maxX, minX;
	Node* curNode;
	Node* maxNode;
	Node* minNode;
	Node* maxPrev;
	Node* minPrev;
	
//	while(l->length >= 3) {
		maxX = std::numeric_limits<double>::min();
		minX = std::numeric_limits<double>::max();
		curNode = l->head;
		maxNode = nullptr;
		minNode = nullptr;
		
		while (curNode->fwd != nullptr) {	//find max & min
			if (curNode->fwd->pt.x > maxX) {
				maxX = curNode->fwd->pt.x;
				maxPrev = curNode;
			}
			if (curNode->fwd->pt.x < minX) {
				minX = curNode->fwd->pt.x;
				minPrev = curNode;
			}
			curNode = curNode->fwd;
		}

		maxNode = maxPrev->fwd;
		minNode = minPrev->fwd;

		//get rid of max & min nodes & call quickhull
		if (maxNode != minNode) {
			removeAfterNode(minPrev,l);
			removeAfterNode(maxPrev,l);
			quickHull(l, edges, minNode, maxNode);
			quickHull(l, edges, maxNode, minNode);
		}
//		else {
//			break;
//		}
//	}
//  delEdgeList(edges);
  return(edges);
}

/*
 * Does quickhull, using an excList to push & pop Nodes so that it's a little faster
 */
void quickHull(excList* l, edgeList* edges, Node* n1, Node* n2) {
	double maxD = -1;				//just make sure it's not >= 0
	double d;
	Node* curNode = l->head;
	Node* maxPrev;
	Node* maxNode;// = nullptr;
	double d1,d2,d3;

	//since we do distLinePt so often, calculate the parts that are always the same
	double abX =(n2->pt.x - n1->pt.x);
	double abY = (n2->pt.y - n1->pt.y);
	/* so instead of:
	 * return ( (b->x - a->x)*(a->y - p->y) - (a->x - p->x)*(b->y - a->y) );
	 * we can do the equivalent of:
	 * return ( abX*(a->y - p->y) - (a->x - p->x)*abY );
	 * for distance from the line n1n2 to the current point
	 */

	addFrame(l);

	while (curNode->fwd != nullptr) {				//loop through points, looking for furthest
		d = ( abX*(n1->pt.y - curNode->fwd->pt.y) - (n1->pt.x - curNode->fwd->pt.x)*abY );
		if (d > maxD) {						//if further away
			maxD = d;						//update max dist
			maxPrev = curNode;			//update furthest Node
			//maxPrev = curNode;
		}
		if (d < 0) {					//if > maxD must be at least 0, so do else if				
			pushAfterNode(curNode,l);
		}
		else {							//only move forward if didn't push
			curNode = curNode->fwd;
		}
	}
	if (maxD == 0) {							//we have co-linear points
		maxNode = maxPrev->fwd;
		removeAfterNode(maxPrev,l);			//maxNode removed, but not deleted
		//need to figure out which direction we're going in
		d1 = distPtPt(&n1->pt,&maxNode->pt);
		d2 = distPtPt(&n1->pt,&n2->pt);
		d3 = distPtPt(&maxNode->pt,&n2->pt);
		
		if (d1 < d2 && d3 < d2) {				//maxNode bet n1 & n2*/
			quickHull(l, edges, n1, maxNode);
			quickHull(l, edges, maxNode, n2);
		}
		//n1 -> n2 must be an edge, but then maxNode is on one side of them
		else {			
			addEdge(n1->pt,n2->pt, edges);
//			glColor3fv(globVars.curCol);
//			glBegin(GL_LINES);						//draw in this edge
//				glVertex2i(n1->pt.x,n1->pt.y);
//				glVertex2i(n2->pt.x,n2->pt.y);
//			glEnd();
//			glFlush();								//flush buffer
		}
		delNode(maxNode);
	}
	else if (maxD < 0) {					//no valid points, this must be edge
		addEdge(n1->pt,n2->pt, edges);
//		glColor3fv(globVars.curCol);
//		glBegin(GL_LINES);						//draw in this edge
//			glVertex2i(n1->pt.x,n1->pt.y);
//			glVertex2i(n2->pt.x,n2->pt.y);
//		glEnd();
//		glFlush();								//flush buffer
	}
	else {										//this is not an edge
		maxNode = maxPrev->fwd;
		removeAfterNode(maxPrev,l);			//maxNode removed, but not deleted
		quickHull(l, edges, n1, maxNode);
		quickHull(l, edges, maxNode, n2);
		delNode(maxNode);
	}

	popFrame(l);
}

