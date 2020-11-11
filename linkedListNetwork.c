/**
 *  linkedListNetwork.c
 *
 *  Implements an abstractNetwork using adjacency lists (linked lists).
 *  This is a structure with two levels of list. There is a master
 *  list of vertices, linked in series. Each vertex points to a subsidiary
 *  linked list with references to all the other vertices to which it
 *  is connected.
 *
 *  Each vertex has an integer weight and a pointer to a parent vertex 
 *  which can be used for route finding and spanning tree algorithms
 *
 *  Key values are strings and are copied when vertices are inserted into
 *  the graph. Every vertex has a void* pointer to ancillary data which
 *  is simply stored. 
 *
 *  Created by Sally Goldin, 1 February 2012 for CPE 113
 *  Modified 18 March 2013 to improve naming.
 *  Modified 20 April 2018 to use JavaDoc keywords in comments

    Modified by Natchapol SHinno 61070503412 Nath Sec C
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "abstractNetwork.h"
#include "abstractQueue.h"
#include "minPriorityQueue.h"
#include "abstractStack.h"


#define WHITE 0
#define GRAY  1
#define BLACK 2

char* colorName[] = {"WHITE", "GRAY", "BLACK"};

/* List items for the adjacency list.
 * Each one is a reference to an existing vertex
 */
typedef struct _adjacent
{
    void * pVertex;           /* pointer to the VERTEX_T this 
                               * item refers to.
                               */
    unsigned int weight;      /* weight of this edge */
    struct _adjacent * next;  /* next item in the ajacency list */ 
} ADJACENT_T;

/* List items for the main vertex list.*/
typedef struct _vertex
{
    char * key;               /* key for this vertex */
    void * data;              /* ancillary data for this vertex */
    int color;                /* used to mark nodes as visited */
    int dValue;               /* sum of weights for shortest path so far to this vertex */
    struct _vertex * parent;  /* pointer to parent found in Dijkstra's algorithm */     
    struct _vertex * next;    /* next vertex in the list */
    ADJACENT_T * adjacentHead;    /* pointer to the head of the
		               * adjacent vertices list
                               */
    ADJACENT_T * adjacentTail;    /* pointer to the tail of the
			       * adjacent vertices list
                               */
}  VERTEX_T;


VERTEX_T * vListHead = NULL;  /* head of the vertex list */
VERTEX_T * vListTail = NULL;  /* tail of the vertex list */
int bGraphDirected = 0;       /* if true, this is a directed graph */


/* Private functions */

/** Finds the vertex that holds the passed key
 * (if any) and returns a pointer to that vertex.
 *
 *@param key    -  Key we are looking for
 *@param pPred  -  used to return the predecessor if any
 *@return pointer to the vertex structure if one is found       
 */
VERTEX_T * findVertexByKey(char* key, VERTEX_T** pPred) 
{
    VERTEX_T * pFoundVtx = NULL;
    VERTEX_T * pCurVertex = vListHead;
    *pPred = NULL;
    /* while there are vertices left and we haven't found
     * the one we want.
     */
    while ((pCurVertex != NULL) && (pFoundVtx == NULL))
       {
       if (strcmp(pCurVertex->key,key) == 0)
          {
	  pFoundVtx = pCurVertex;
	  }
       else
          {
	  *pPred = pCurVertex;
          pCurVertex = pCurVertex->next;
          }
       }
    return pFoundVtx;
}

/** Free the adjacencyList for a vertex 
 * 
 * @param  pVertex  Vertex whose edges we want to delete 
 */
void freeAdjacencyList(VERTEX_T *pVertex)
{
   ADJACENT_T * pCurRef = pVertex->adjacentHead;
   while (pCurRef != NULL)
      {
      ADJACENT_T * pDelRef = pCurRef;
      pCurRef = pCurRef->next;
      free(pDelRef);
      }
   pVertex->adjacentHead = NULL;
   pVertex->adjacentTail = NULL;
}

/** Check if there is already an edge between
 * two vertices. We do not want to add a duplicate.
 * @param pFrom Start point of edge
 * @param pTo   End point of edge
 * @return 1 if an edge already exists, 0 if it does not
 */
int edgeExists(VERTEX_T* pFrom, VERTEX_T* pTo)
{
    int bEdgeExists = 0;
    ADJACENT_T * pCurRef = pFrom->adjacentHead;
    while ((pCurRef != NULL) && (!bEdgeExists))
       {
       if (pCurRef->pVertex == pTo)
          {
	  bEdgeExists = 1;  /* the 'To' vertex is already in the
                             * 'From' vertex's adjacency list */ 
          }
       else
          {
	  pCurRef = pCurRef->next;
          }
       } 
    return bEdgeExists;
}

/** Component of removeVertex. Removes all references
 * to this vertex as the end point of edges in other
 * vertices' adjacency lists.
 * @param pTarget Vertex whose references should be removed
 */
void removeReferences(VERTEX_T * pTarget)
{
   VERTEX_T * pCurrentVtx = vListHead;
   while (pCurrentVtx != NULL)
      {
      if (pCurrentVtx != pTarget)
         {
	 /* skip the target vertex */
	 ADJACENT_T* pAdjacent = pCurrentVtx->adjacentHead;
	 ADJACENT_T* pPrevAdjacent = NULL;
	 while (pAdjacent != NULL)
	    {
	    if (pAdjacent->pVertex == pTarget)  /* if this edge involves the target*/
	       {
	       if (pPrevAdjacent != NULL)
	          {
		  pPrevAdjacent->next = pAdjacent->next;
	          }
               else
	          {
		  pCurrentVtx->adjacentHead = pAdjacent->next;   
	          }
               if (pAdjacent == pCurrentVtx->adjacentTail)
	          {
		  pCurrentVtx->adjacentTail = NULL;
		  }
               free(pAdjacent);
               pAdjacent = NULL;
	       break;    /* can only show up once in the adjacency list*/
	       }
            else
	       {
	       pPrevAdjacent = pAdjacent;
               pAdjacent = pAdjacent->next;
	       }  
	    }
	 }
      pCurrentVtx = pCurrentVtx->next;      
      } 
}

/** Count adjacent vertices to a vertex.
 * @param  pVertex  Vertex whose adjacent nodes we want to count
 * @return integer value for count (could be zero)
 */
int countAdjacent(VERTEX_T * pVertex)
{
    int count = 0;
    ADJACENT_T * pAdjacent = pVertex->adjacentHead;
    while (pAdjacent != NULL)
       {
       count += 1;
       pAdjacent = pAdjacent->next;
       }
    return count;
}

/** Color all vertices to the passed color.
 *  @param  A color constant
 */
void colorAll(int color)
{
    VERTEX_T* pVertex = vListHead;
    while (pVertex != NULL)
       {
       pVertex->color = color;
       pVertex = pVertex->next;
       }
}

/** Initialize the dValue and parent for all
 * vertices. dValue should be very big, parent
 * will be set to NULL. Also add to the minPriority queue.
 */
void initAll()
{
    VERTEX_T* pVertex = vListHead;
    while (pVertex != NULL)
       {
       pVertex->dValue = 999999999;
       pVertex->parent = NULL;
       enqueueMin(pVertex);
       pVertex = pVertex->next;
       }
}


/** Execute a breadth first traversal from a vertex,
 * calling the passed function (*vFunction) on each vertex
 * as we visit it and color it black.
 * @param pVertex  Starting vertex for traversal
 */
void traverseBreadthFirst(VERTEX_T* pVertex, void (*vFunction)(VERTEX_T*))
{
    VERTEX_T * pCurrentVertex = NULL;
    VERTEX_T * pAdjVertex = NULL;    
    queueClear();
    colorAll(WHITE);
    pVertex->color = GRAY;
    enqueue(pVertex);
    while (queueSize() > 0)
       {
       pCurrentVertex = (VERTEX_T*) dequeue();
       if (pCurrentVertex->color != BLACK)
          {
          (*vFunction)(pCurrentVertex);
          pCurrentVertex->color = BLACK;
	  ADJACENT_T* pAdjacent = pCurrentVertex->adjacentHead;
	  while (pAdjacent != NULL)
             {
	     pAdjVertex = (VERTEX_T*) pAdjacent->pVertex;
	     if (pAdjVertex ->color != BLACK)
	         {
		 pAdjVertex->color = GRAY;
		 enqueue(pAdjVertex);
	         }
	     pAdjacent = pAdjacent->next;
             }
	  }
       } /* end while queue has data */
}


/** Execute a depth first traversal from a single vertex,
 * calling the passed function (*vFunction) on the lowest level
 * vertex we visit, and coloring it black.
 * @param pVertex Starting vertex for traversal
 */
void traverseDepthFirst(VERTEX_T* pVertex, void (*vFunction)(VERTEX_T*))
{
    VERTEX_T * pAdjVertex = NULL;    
    ADJACENT_T* pAdjacent = pVertex->adjacentHead;
    while (pAdjacent != NULL)
       {
       pAdjVertex = (VERTEX_T*) pAdjacent->pVertex;
       if (pAdjVertex->color == WHITE)
	   {
	   pAdjVertex->color = GRAY;
           traverseDepthFirst(pAdjVertex,vFunction);
           }
       pAdjacent = pAdjacent->next;  
       } /* end while queue has data */
    /* when we return from the bottom, call the 
     * function and color this node black.
     */
    (*vFunction)(pVertex);
    pVertex->color = BLACK;
}


/**  Function to print the information about a vertex
 * @param pVertex Vertex we want to print
 */
void printVertexInfo(VERTEX_T* pVertex)
{
    printf("== Vertex key |%s| - data |%s|\n",
	   pVertex->key, (char*) pVertex->data);
}


/********************************/
/** Public functions start here */
/********************************/

/** Initialize or reintialize the graph.
 * @param   maxVertices How many vertices can this graph handle.
 * @param   bDirected   If true this is a directed graph, otherwise undirected.
 * @return 1 unless there is a memory allocation error, then 0
 */
int initGraph(int maxVertices, int bDirected)
{ 
    /* for a linked list graph, we call
     * clearGraph and then initialize bGraphDirected
     */
    clearGraph();
    bGraphDirected = bDirected;
    return 1;  /* this implementation of initGraph can never fail */ 
}


/** Free all memory associated with the graph and
 * reset all parameters.
 */
void clearGraph()
{
    VERTEX_T * pCurVertex = vListHead;
    while (pCurVertex != NULL)
       {
       freeAdjacencyList(pCurVertex);
       VERTEX_T * pDelVtx = pCurVertex;
       pCurVertex = pCurVertex->next;
       free(pDelVtx->key);
       free(pDelVtx);
       }

    vListHead = NULL;  
    vListTail = NULL; 
    bGraphDirected = 0;

}

/** Add a vertex into the graph.
 * @param  key   Key value or label for the  vertex
 * @param  pData Additional information that can be associated with the vertex.
 * @return  1 unless there is an error, in which case
 * it returns a 0. An error could mean a memory allocation 
 * error or running out of space, depending on how the 
 * graph is implemented. Returns -1 if the caller tries
 * to add a vertex with a key that matches a vertex
 * already in the graph.
 */
int addVertex(char* key, void* pData)
{
    int bOk = 1;
    VERTEX_T * pPred;
    VERTEX_T * pFound = findVertexByKey(key, &pPred);
    if (pFound != NULL)  /* key is already in the graph */
       {
       bOk = -1;
       }
    else
       {
       VERTEX_T * pNewVtx = (VERTEX_T *) calloc(1,sizeof(VERTEX_T));
       char * pKeyval = strdup(key);
       if ((pNewVtx == NULL) || (pKeyval == NULL))
          {
	  bOk = 0;  /* allocation error */
	  }
       else
          {
	  pNewVtx->key = pKeyval;
          pNewVtx->data = pData;
	  if (vListHead == NULL)  /* first vertex */
	     {
	     vListHead = pNewVtx;
	     }
	  else
	     {
	     vListTail->next = pNewVtx; 
	     }
	  vListTail = pNewVtx;
	  }
       }
    return bOk;
}


/** Remove a vertex from the graph.
 * @param  key  Key value or label for the vertex to remove
 * @return a pointer to the data stored at that
 * vertex, or NULL if the vertex could not be 
 * found.
 */
void* removeVertex(char* key)
{
   void * pData = NULL; /* data to return */
   VERTEX_T * pPredVtx = NULL;
   VERTEX_T * pRemoveVtx = findVertexByKey(key,&pPredVtx);
   if (pRemoveVtx != NULL)
      {
      removeReferences(pRemoveVtx);
      freeAdjacencyList(pRemoveVtx);
      if (pPredVtx != NULL)
         {
	 pPredVtx->next = pRemoveVtx->next;
         }
      else /* if there is no predecessor that means this was the head */
         {
         vListHead = pRemoveVtx->next;
         }   
      if (pRemoveVtx == vListTail)
	 vListTail = pPredVtx;
      free(pRemoveVtx->key);
      pData = pRemoveVtx->data;
      free(pRemoveVtx);
      } 
   return pData;
}


/** Add an edge between two vertices
 * @param   key1  Key for the first vertex in the edge
 * @param   key2  Key for the second vertex
 * @param   weight Weight for this edge
 * @return  1 if successful, 0 if failed due to
 * memory allocation error, or if either vertex
 * is not found. Returns -1 if an edge already
 * exists in this direction.
 */
int addEdge(char* key1, char* key2, unsigned int weight)
{
    int bOk = 1;
    VERTEX_T * pDummy = NULL;
    VERTEX_T * pFromVtx = findVertexByKey(key1,&pDummy);
    VERTEX_T * pToVtx = findVertexByKey(key2,&pDummy);
    if ((pFromVtx == NULL) || (pToVtx == NULL))
       {
       bOk = 0;
       }
    else if (edgeExists(pFromVtx,pToVtx))
       {
       bOk = -1;	   
       }
    else
       {
       ADJACENT_T * pNewRef = (ADJACENT_T*) calloc(1,sizeof(ADJACENT_T));
       if (pNewRef == NULL)
          {
	  bOk = 0;
          }
       else
          {
	  pNewRef->pVertex = pToVtx;
          pNewRef->weight = weight; 
	  if (pFromVtx->adjacentTail != NULL)
              {
	      pFromVtx->adjacentTail->next = pNewRef;
	      }
          else
	      {
	      pFromVtx->adjacentHead = pNewRef;
	      }
	  pFromVtx->adjacentTail = pNewRef;
          } 
       } 
    /* If undirected, add an edge in the other direction */
    if ((bOk) && (!bGraphDirected))
       {
       ADJACENT_T * pNewRef2 = (ADJACENT_T*) calloc(1,sizeof(ADJACENT_T));
       if (pNewRef2 == NULL)
          {
	  bOk = 0;
          }
       else
          {
	  pNewRef2->pVertex = pFromVtx;
          pNewRef2->weight = weight; 
	  if (pToVtx->adjacentTail != NULL)
              {
	      pToVtx->adjacentTail->next = pNewRef2;
	      }
          else
	      {
	      pToVtx->adjacentHead = pNewRef2;
	      }
	  pToVtx->adjacentTail = pNewRef2;
          } 
       } 
    return bOk;
}


/** Remove an edge between two vertices
 * @param  key1  Key for the first vertex in the edge
 * @param  key2  Key for the second vertex
 * @return 1 if successful, 0 if failed 
 * because either vertex is not found or there
 * is no edge between these items.
 */
int removeEdge(char* key1, char* key2)
{
   int bOk = 1;
   VERTEX_T * pDummy = NULL;
   VERTEX_T * pFromVtx = findVertexByKey(key1,&pDummy);
   VERTEX_T * pToVtx = findVertexByKey(key2,&pDummy);
   if ((pFromVtx == NULL) || (pToVtx == NULL))
       {
       bOk = 0;
       }
   else if (!edgeExists(pFromVtx,pToVtx))
       {
       bOk = 0;
       }
   else
       {
       ADJACENT_T* pAdjacent = pFromVtx->adjacentHead;
       ADJACENT_T* pPrevAdjacent = NULL;
       while (pAdjacent != NULL)
          {  
	  if (pAdjacent->pVertex == pToVtx)  /* if this edge involves the target*/
	     {
	     if (pPrevAdjacent != NULL)
	        {
	        pPrevAdjacent->next = pAdjacent->next;
		}
             else
	        {
		pFromVtx->adjacentHead = pAdjacent->next;   
	        }
             if (pAdjacent == pFromVtx->adjacentTail)
	        {
		pFromVtx->adjacentTail = NULL;
		}
	     free(pAdjacent);
	     break;    /* can only show up once in the adjacency list*/
	     }
	  else
	     {
	     pPrevAdjacent = pAdjacent;
	     pAdjacent = pAdjacent->next;
	     }  
	  }
       /* If undirected, remove edge in the other direction */
       if ((bOk) && (!bGraphDirected))
          {
          ADJACENT_T* pAdjacent2 = pToVtx->adjacentHead;
          ADJACENT_T* pPrevAdjacent2 = NULL;
	  while (pAdjacent2 != NULL)
             {   
	     if (pAdjacent2->pVertex == pFromVtx)  
	        {
	        if (pPrevAdjacent2 != NULL)
	           {
	           pPrevAdjacent2->next = pAdjacent2->next;
		   }
		else
	           {
		   pToVtx->adjacentHead = pAdjacent2->next;   
		   }
		if (pAdjacent2 == pToVtx->adjacentTail)
	           {
		   pToVtx->adjacentTail = NULL;
		   }
		free(pAdjacent2);
		break;    /* can only show up once in the adjacency list*/
		}
	     else
	        {
		pPrevAdjacent2 = pAdjacent2;
		pAdjacent2 = pAdjacent2->next;
		}  
	     }
	  }
       }
    return bOk;
}

/** Find a vertex and return its data
 * @param   key Key for the vertex to find
 * @return the data for the vertex or NULL if not found.
 */
void* findVertex(char* key)
{
    void* pData = NULL;
    VERTEX_T * pDummy = NULL;
    VERTEX_T * pFoundVtx = findVertexByKey(key,&pDummy);
    if (pFoundVtx != NULL)
       {
       pData = pFoundVtx->data;
       }
    return pData; 
}

/** Find the edge between two vertices (if any) and return
 * its weight
 * @param   key1  Key for the first vertex in the edge
 * @param   key2  Key for the second vertex
 * @return  Weight if successful and edge exists, or -1 if an edge is not found
 */
int findEdge(char* key1, char* key2)
{
    int weight = -1;
    int bEdgeExists = 0;
    VERTEX_T * pDummy = NULL;
    VERTEX_T * pFrom = findVertexByKey(key1,&pDummy);	
    ADJACENT_T * pCurRef = pFrom->adjacentHead;
    while ((pCurRef != NULL) && (!bEdgeExists))
       {
       VERTEX_T * pFrom = (VERTEX_T*) pCurRef->pVertex;
       if (strcmp(pFrom->key,key2) == 0)
          {
	  weight = pCurRef->weight;  
          bEdgeExists = 1;
          }
       else
          {
	  pCurRef = pCurRef->next;
          }
       } 
    return weight;
}



/** Return an array of copies of the keys for all nodes
 * adjacent to a node. The array and its
 * contents should be freed by the caller when it 
 * is no longer needed.
 * @param   key  Key for the node whose adjacents we want
 * @param   pCount Return number of elements in the array
 * @return  array of char* which are the keys of adjacent
 * nodes. Returns number of adjacent vertices in pCount.
 * If pCount holds -1, the vertex does not exist.
 */
char** getAdjacentVertices(char* key, int* pCount)
{
    char** keyArray = NULL;
    VERTEX_T * pDummy = NULL;
    VERTEX_T * pFoundVtx = findVertexByKey(key,&pDummy);
    if (pFoundVtx != NULL)
       {
       *pCount = countAdjacent(pFoundVtx);
       if (*pCount > 0)
          {
	  int i = 0;
	  keyArray = (char**) calloc(*pCount, sizeof(char*));
          if (keyArray != NULL)
	     {
	     ADJACENT_T * pAdjacent = pFoundVtx->adjacentHead;
	     while (pAdjacent != NULL)
	        {
		VERTEX_T* pVertex = (VERTEX_T*) pAdjacent->pVertex;
		keyArray[i] = strdup(pVertex->key);
		pAdjacent = pAdjacent->next;
		i += 1;
	        }
	     }
          } 
       } 
    else
       {
       *pCount = -1;
       }
    return keyArray;
}


/** Print out all the nodes reachable from a vertex by a 
 * breadth-first traversal
 * @param  startKey Key for start vertex
 * @return 1 if successful, -1 if the start vertex does not exist.
 */
int printBreadthFirst(char* startKey)
{
   int retval = 1;
   VERTEX_T * pDummy = NULL;
   VERTEX_T * pVertex = findVertexByKey(startKey,&pDummy);
   if (pVertex == NULL)
      {
      retval = -1;
      }
   else
      {
      traverseBreadthFirst(pVertex,&printVertexInfo);
      }
   return retval;
}

/** Comparison function to send to the minPriorityQueue
 * Compares dValue values
 * @param  pV1  First vertex (will be cast to VERTEX_T *)
 * @param  pV2  Second vertex (will be cast to VERTEX_T *)
 * @return -1 if V1 < V2, 0 if dValues are the same, 1 if V1 > V2.
 */
int compareVertices(void * pV1, void * pV2)
{
   VERTEX_T * pVertex1 = (VERTEX_T*) pV1;
   VERTEX_T * pVertex2 = (VERTEX_T*) pV2;
   if (pVertex1->dValue < pVertex2->dValue)
      return -1;
   else if (pVertex1->dValue > pVertex2->dValue)
      return 1;
   else 
     return 0;
}

/**  Print out all the vertices by a depth-first traversal.
 * Iterates through all possible starting vertices.
 */
void printDepthFirst()
{
   VERTEX_T* pVertex = vListHead;
   if (pVertex == NULL)
      {
      printf("The graph is empty\n");
      }
   else
      {
      colorAll(WHITE);
      while (pVertex != NULL)
         {
	 if (pVertex->color == WHITE)
	    {
	    printf("\nStarting new traversal from |%s|\n",
                   pVertex->key);
	    pVertex->color = GRAY;
            traverseDepthFirst(pVertex,&printVertexInfo);
	    }
         pVertex = pVertex->next;
	 }
      }
}


void printPath(VERTEX_T* vertex)
{
  if (vertex->parent != NULL)
    printPath(vertex->parent);
  printf("%s -> ",vertex->key );
}




/* Print out the lowest weight path from one vertex to 
 * another through the network using Dijkstra's
 * algorithm. 
 * Arguments
 *    startKey    -  Key of start vertex
 *    endKey      -  Key of ending vertex
 * Returns the sum of the weights along the path.
 * Returns -1 if either key is invalid. Returns -2
 * if network is not directed. Returns -3 if the
 * network has negative weights. Returns -4 if end is not 
 * reachable from the start.
 */
int printShortestPath(char* startKey, char* endKey)
{
    int pathWeight = 0;
    int weightSum = 0;
    int bHasNeg = 0;

    VERTEX_T * pDummy = NULL;
    VERTEX_T * pStartVertex = findVertexByKey(startKey,&pDummy);
    VERTEX_T * pEndVertex = findVertexByKey(endKey,&pDummy);
    VERTEX_T * pMinVertex = NULL;
    VERTEX_T * pCurrentVertex = NULL;
    if ((pStartVertex == NULL) || (pEndVertex == NULL))
       return -1;
    if (!bGraphDirected)
       return -2;
    if (bHasNeg)
       return -3;
    /* Return immediately if we have error conditions. Otherwise
     * initialize the queue. 
     */
    queueMinInit(&compareVertices); 
    colorAll(WHITE);    
    initAll();
    pStartVertex->dValue = 0;
    while (queueMinSize() > 0)
      {
      pMinVertex = (VERTEX_T*) dequeueMin();
      pMinVertex->color = BLACK;
      ADJACENT_T * pAdjacentEdge = pMinVertex->adjacentHead;
      while (pAdjacentEdge != NULL)
   {
   VERTEX_T* pAdj = pAdjacentEdge->pVertex;
         /* if this adjacent vertex has not yet been added to the tree
          * and if this path is shorter than any previous path stored
          * in the adjacent vertex, adjust the dValue and set the parent.
          */
         int distance = pMinVertex->dValue + pAdjacentEdge->weight;          
         if ((pAdj->color == WHITE) && (distance < pAdj->dValue))
      {
      pAdj->dValue = distance;
            pAdj->parent = pMinVertex;
      }
         pAdjacentEdge = pAdjacentEdge->next; 
   }
      }
    /* When we exit the loop, all reachable vertices will have their
     * distance and parents set. We start at the destination (end)
     * vertex and work backward to create the path. The easiest
     * way to do this is to have a stack. */
    pathWeight =  pEndVertex->dValue;
    if (pathWeight > weightSum) /* never changed */  
      {
      pathWeight = -4; 
      }   
    else
      { 
      printf("Minimum weight path from %s to %s:\n",startKey,endKey);
      printPath(pEndVertex);
      }
    return pathWeight;
}




/** Print out the minimum spanning tree with total
 * weight, using Prim's algorithm.
 * @param   startKey  Key of start vertex
 * @return The sum of the weights along all edges in
 * the network. Returns -1 if start key is invalid.
 * Returns -2 if network is directed
 */
int printMinSpanningTreePrim(char* startKey)
{
    int sumWeight = 0;
    VERTEX_T * pDummy = NULL;
    VERTEX_T * pStartVertex = findVertexByKey(startKey,&pDummy);
    VERTEX_T * pMinVertex = NULL;
    if (pStartVertex == NULL)
       return -1;
    if (bGraphDirected)
       return -2;
    /* Return immediately if we have error conditions. Otherwise
     * initialize the queue. 
     */
    queueMinInit(&compareVertices); 
    colorAll(WHITE);    
    initAll();
    pStartVertex->dValue = 0;
    while (queueMinSize() > 0)
      {
      pMinVertex = (VERTEX_T*) dequeueMin();
      pMinVertex->color = BLACK;
      sumWeight += pMinVertex->dValue;
      printf("Adding vertex |%s| to min spanning tree ",pMinVertex->key);
      if (pMinVertex->parent != NULL)
  	  printf("as child of |%s|\n",pMinVertex->parent->key);
      else
  	  printf("as root\n");
      ADJACENT_T * pAdjacent = pMinVertex->adjacentHead;
      while (pAdjacent != NULL)
	 {
	 VERTEX_T* pAdj = pAdjacent->pVertex;
         /* if this adjacent vertex has not yet been added to the tree
          * and if the weight on this edge is less than the current dValue
          * for this vertex, adjust the dValue and set the parent.
          */
         if ((pAdj->color == WHITE) && (pAdjacent->weight < pAdj->dValue))
	    {
	    pAdj->dValue = pAdjacent->weight;
            pAdj->parent = pMinVertex;
	    }
         pAdjacent = pAdjacent->next; 
	 }
      }
    return sumWeight;
}



/** Function that push all the vertex in to the stack
*/
void pushPath(VERTEX_T* vertex)
{
  if (vertex->parent != NULL)
    pushPath(vertex->parent);
  push(vertex->key);
}


/** I'm edited in the last part of function. I used be "printPath" funcetion but I created
the new function call "pushPath" (to push all vertex in to the stack) to instead printPath function.
*/

/* Return information as to whether two vertices are
 * connected by a path. Also print the path if it
 * exists. We do this by keeping track of source vertices
 * as we do the breadth first traversal.And pust all vertex in path into stack
 * Arguments
 *    key1 -  Key for the start vertex 
 *    key2 -  Key for the second vertex to check 
 * Returns 1 if the two vertices are connected, 0 if they
 * are not. Returns -1 if either vertex does not exist.
 */
char ** isReachablePrintPath(char* key1, char* key2)
{
   int retval = 1;
   VERTEX_T * pDummy = NULL;
   VERTEX_T * pAdjacent = NULL;
   VERTEX_T * pStartVertex = findVertexByKey(key1,&pDummy);
   VERTEX_T * pEndVertex = findVertexByKey(key2,&pDummy);
   char ** keyArray = NULL;
   int count = 0;
   if ((pStartVertex == NULL) || (pEndVertex == NULL))
      {
      retval = -1;
      }
   else
      {
      queueClear();
      colorAll(WHITE);
      enqueue(pStartVertex);
      while (queueSize() > 0)
        {
        VERTEX_T* pCurrent = (VERTEX_T*) dequeue();
        if (pCurrent->color != BLACK)
            {
            pCurrent->color = BLACK;
      ADJACENT_T* pRef = pCurrent->adjacentHead;
      while (pRef != NULL)
        {
        pAdjacent = (VERTEX_T*) pRef->pVertex;
        if (pAdjacent->color != BLACK)
      {
      pAdjacent->parent = pCurrent;
            enqueue(pAdjacent);
      }
        pRef = pRef->next;
        }
      }
  } /* end while queue has data */
      if (pEndVertex->color != BLACK)
    { 
          retval = 0; 
          } 
      else
    {
        //EDITED
      
        pushPath(pEndVertex);
    
          }
      }
   return keyArray;

}
/** Count Adjacent Of each key and return the count
    @key - key that you want to count the adjacent
    @return - count of the adjacent
*/
int countAdjacentByKey (char * key)
{
  int count = 0;
  VERTEX_T * pDummy = NULL;
  ADJACENT_T * pAdjacent = NULL;
  VERTEX_T * pVertex = findVertexByKey(key,&pDummy);
  if (pVertex == NULL)
    count = -1;
  else
  {
    ADJACENT_T *pAdjacent = pVertex->adjacentHead;
    while(pAdjacent != NULL)
    {
      count+=1;
      pAdjacent = pAdjacent->next;
    }
  }
  return count;
}
 
