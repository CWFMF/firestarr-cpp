#include "exclusionlist.h"
#include "stdafx.h"
#include <assert.h>
#include <malloc.h>

mutex mutex_{};

Node* junkNodes = nullptr;
Frame* junkFrames = nullptr;

/*
 * Adds a new Frame to the excList and starts using it as the current Frame
 */
void addFrame(excList* l) {
   l->top = newFrame(l->top);
}

/*
 * Checks integrity of excList
 */
void chkList(excList* l) {
   Frame* curFrame = l->top;
   Node* curNode;
   int lengthA = l->length;

   curNode = l->head->fwd;
   while (curNode != nullptr) {
		assert(curNode->fwd != nullptr);
		 lengthA--;
		 curNode = curNode->fwd;
   }

   while (curFrame != nullptr) {
      curNode = curFrame->first;
      
      while (curNode != nullptr) {
		 assert(curNode->fwd != curNode);
		 lengthA--;
		 curNode = curNode->fwd;
      }
      curFrame = curFrame->next;
   }
   assert(lengthA == 0);
}
//
///*
// * Deletes all items in junkNodes & junkFrames.  Do it here instead of as we go so
// * we can be more efficient by recycling them.
// */
//void cleanup() {
//	Frame* curFrame;
//	Node* curNode;
//
//	while (junkFrames != nullptr) {
//		curFrame = junkFrames;
//		junkFrames = junkFrames->next;
//		free(curFrame);
//	}
//
//	while (junkNodes != nullptr) {
//		curNode = junkNodes;
//		junkNodes = junkNodes->fwd;
//		free(curNode);
//	}
//}

/*
 * deletes given excList, getting rid of items currently in list & on stack
 */
void delExcList(excList* l) {   
   Frame* curFrame = l->top;
   Frame* tmpFrame;
   Node* curNode;
	Node* tmpNode;

	curNode = l->head;				//free the list
	while (curNode != nullptr) {
	  tmpNode = curNode;
	  curNode = curNode->fwd;
	  delNode(tmpNode);
	}

   while (curFrame != nullptr) {		//free the stack
	   tmpFrame = curFrame;
	   curFrame = curFrame->next;
       curNode = tmpFrame->first;
      
	  while (curNode != nullptr) {
		  tmpNode = curNode;
		  curNode = curNode->fwd;	  
		  delNode(tmpNode);
      }
	  delFrame(tmpFrame);
   }
   free(l);
}

/*
 * 'Deletes' a Frame by adding it to the junkNode list
 */
void delFrame(Frame* f) {
  lock_guard<mutex> lock(mutex_);
	f->next = junkFrames;					//place at start of junkNode list
	junkFrames = f;
}

/*
 * 'Deletes' a Node by adding it to the junkNode list
 */
void delNode(Node* n) {
  lock_guard<mutex> lock(mutex_);
	n->fwd = junkNodes;					//place at start of junkNode list
	junkNodes = n;
}

/*
 * Inserts the point p into l.  Insertion is simply at the start of the list
 */
void insert(point p, excList* l) {
	l->head->fwd = newNode(p, l->head->fwd);   //insert as first Node, setting back & fwd
	l->length++;
};

/*
 * Creates and returns a new excList
 */
excList* newExcList() {
   excList* ret = (excList*)malloc(sizeof(excList));
   point nullPt = {-1,-1};
   ret->head = newNode(nullPt,nullptr);
   ret->length = 0;
   ret->top = nullptr;
   return ret;
}

/*
 * Creates a new Frame with Frame next as it's next
 */
Frame* newFrame(Frame* next) {
  lock_guard<mutex> lock(mutex_);
  Frame* ret;
   if (junkFrames) {			//recycle the Frame == (junkFrames != nullptr)
	   ret = junkFrames;
	   junkFrames = ret->next;
   }
   else {
	   ret = (Frame*)malloc(sizeof(Frame));
   }
   ret->first = nullptr;
   ret->next = next;
   return ret;
}

/*
 * Creates a new Node which contains a copy of p, and has back & fwd set as given
 */
Node* newNode(point p, Node* fwd) {
  lock_guard<mutex> lock(mutex_);
  Node* ret;
   if (junkNodes) {			//recycle the stackNode == (junkNodes != nullptr)
	   ret = junkNodes;
	   junkNodes = ret->fwd;
   }
   else {
	   ret = (Node*)malloc(sizeof(Node));
   }
   ret->pt = p;
   ret->fwd = fwd;
   return ret;
}

/*
 * Pops the current Frame for the excList.  This has the effect of re-inserting all
 * items that were removed from the list while this Frame was in effect
 */
void popFrame(excList* l) {
   Frame* tmpFrame;
   Node* curNode;

   while (l->top->first != nullptr) {				//'while (cur) {' same as 'while (cur != nullptr) {'
		curNode = l->top->first;
		l->top->first = curNode->fwd;
	    curNode->fwd = l->head->fwd;
		l->head->fwd = curNode;
	}

   tmpFrame = l->top;                            //get current top Frame
   l->top = l->top->next;                        //set new top of stack
   delFrame(tmpFrame);
}

/*
 * Puts Node after this one on the stack
 */
void pushAfterNode(Node* n, excList* l) {
	Node* tmpNode;
	tmpNode = n->fwd;
	n->fwd = n->fwd->fwd;
	tmpNode->fwd = l->top->first;
	l->top->first = tmpNode;
}

/*
 * Completely removes n from excList
 */
void removeAfterNode(Node* del, excList* l) {		//delete done outside (not here)
   	del->fwd = del->fwd->fwd;
	l->length--;
}

/*
 * shows non-stack info about l
 */
void showList(excList* l) {
	Node* curNode = l->head;
	
//	printf("fwd : { ");
	while (curNode != nullptr) {
//		printf("(%f,%f) ",curNode->pt.x,curNode->pt.y);
		curNode = curNode->fwd;
	}
//	printf("}\n");
}

/*
 * shows everything there is to know about l to stdout
 */
void showListFull(excList* l) {
	Frame* curFrame = l->top;
	Node* curNode;
	showList(l);
	
	while (curFrame != nullptr) {
//		printf("{ ");
		curNode = curFrame->first;
		while (curNode != nullptr) {
//			printf("(%f,%f) ",curNode->pt.x,curNode->pt.y);
			curNode = curNode->fwd;
		}
//		printf("}\n");
		curFrame = curFrame->next;
	}
}

