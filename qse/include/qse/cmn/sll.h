/*
 * $Id: sll.h 223 2008-06-26 06:44:41Z baconevi $
 *
   Copyright 2006-2008 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef _QSE_CMN_SLL_H_
#define _QSE_CMN_SLL_H_

#include <qse/types.h>
#include <qse/macros.h>

/****o* qse.cmn.sll/singly linked list
 * DESCRIPTION
 *  <qse/cmn/sll.h> provides a singly linked list
 *
 *  #include <qse/cmn/sll.h>
 ******
 */

/****t* qse.cmn.sll/qse_sll_walk_t
 * NAME
 *  qse_sll_walk_t - define return values for qse_sll_walker_t
 * SEE ALSO
 *  qse_sll_walker_t
 * SYNOPSIS
 */
enum qse_sll_walk_t
{
	QSE_SLL_WALK_STOP    = 0,   
	QSE_SLL_WALK_FORWARD = 1
};
/******/

typedef struct qse_sll_t      qse_sll_t;
typedef struct qse_sll_node_t qse_sll_node_t;
typedef enum   qse_sll_walk_t qse_sll_walk_t;

/****b* qse.cmn.sll/qse_sll_copier_t
 * NAME
 *  qse_sll_copier_t - define a node contruction callback
 * DESCRIPTION
 *  The qse_sll_copier_t defines a callback function for node construction.
 *  A node is contructed when a user adds data to a list. The user can
 *  define how the data to add can be maintained in the list. A singly
 *  linked list not specified with any copiers stores the data pointer and
 *  the data length into a node. A special copier QSE_SLL_COPIER_INLINE copies 
 *  the contents of the data a user provided into the node. You can use the
 *  qse_sll_setcopier() function to change the copier. 
 *
 *  A copier should return the pointer to the copied data. If it fails to copy
 *  data, it may return QSE_NULL. You need to set a proper freeer to free up
 *  memory allocated for copy.
 * SEE ALSO
 *  qse_sll_setcopier, qse_sll_getcopier, QSE_SLL_COPIER
 * SYNOPSIS
 */
typedef void* (*qse_sll_copier_t) (
	qse_sll_t* sll    /* a map */,
	void*      dptr   /* the pointer to data to copy */,
	qse_size_t dlen   /* the length of data to copy */
);
/******/

/****b* qse.cmn.sll/qse_sll_freeer_t
 * NAME
 *  qse_sll_freeer_t - define a node destruction callback
 * SYNOPSIS
 */
typedef void (*qse_sll_freeer_t) (
	qse_sll_t* sll    /* a map */,
	void*      dptr   /* the pointer to data to free */,
	qse_size_t dlen   /* the length of data to free */
);
/******/

/****t* qse.cmn.sll/qse_sll_comper_t
 * NAME
 *  qse_sll_comper_t - define a data comparator
 *
 * DESCRIPTION
 *  The qse_sll_comper_t type defines a key comparator that is called when
 *  the list needs to compare data. A singly linked list is created with a
 *  default comparator that performs bitwise comparison.
 *
 *  The comparator should return 0 if the data are the same and a non-zero
 *  integer otherwise.
 *
 * SYNOPSIS
 */
typedef int (*qse_sll_comper_t) (
	qse_sll_t*  sll    /* a singly linked list */, 
	const void* dptr1  /* a data pointer */,
	qse_size_t  dlen1  /* a data length */,
	const void* dptr2  /* a data pointer */,
	qse_size_t  dlen2  /* a data length */
);
/******/

/****b* qse.cmn.sll/qse_sll_walker_t
 * NAME
 *  qse_sll_walker_t - define a list traversal callback for each node
 *
 * DESCRIPTION
 *  The qse_sll_walk() calls a callback function of the type qse_sll_walker_t
 *  for each node until it returns QSE_SLL_WALK_STOP. The walker should return
 *  QSE_SLL_WALK_FORWARD to let qse_sll_walk() continue visiting the next node.
 *  The third parameter to qse_sll_walk() is passed to the walker as the third
 *  parameter.
 *
 * SEE ALSO
 *  qse_sll_walk, qse_sll_walk_t
 *
 * SYNOPSIS
 */
typedef qse_sll_walk_t (*qse_sll_walker_t) (
	qse_sll_t*      sll   /* a map */,
	qse_sll_node_t* node  /* a visited node */,
	void*           arg   /* user-defined data */
);
/******/

/****s* qse.cmn.sll/qse_sll_t
 * NAME
 *  qse_sll_t - define a singly linked list
 * 
 * DESCRPTION
 *  The qse_sll_t type defines a singly lnked list.
 *
 * SYNOPSIS
 */
struct qse_sll_t
{
	QSE_DEFINE_COMMON_FIELDS (sll)

	qse_sll_copier_t copier; /* data copier */
	qse_sll_freeer_t freeer; /* data freeer */
	qse_sll_comper_t comper; /* data comparator */
	qse_byte_t       scale;  /* scale factor */

	qse_size_t       size;   /* the number of nodes */
	qse_sll_node_t*  head;   /* the head node */
	qse_sll_node_t*  tail;   /* the tail node */
};
/******/

/****s* cmn/qse_sll_node_t
 * NAME
 *  qse_sll_node_t - define a list node
 * DESCRIPTION
 *  The qse_sll_node_t type defines a list node containing a data pointer and 
 *  and data length. 
 * SEE ALSO
 *  QSE_SLL_DPTR, QSE_SLL_DLEN, QSE_SLL_NEXT
 * SYNOPSIS
 */
struct qse_sll_node_t
{
	void*           dptr; /* the pointer to data */
	qse_size_t      dlen; /* the length of data */
	qse_sll_node_t* next; /* the pointer to the next node */
};
/******/

#define QSE_SLL_COPIER_SIMPLE ((qse_sll_copier_t)1)
#define QSE_SLL_COPIER_INLINE ((qse_sll_copier_t)2)

#define QSE_SLL_COPIER(sll) ((sll)->copier)
#define QSE_SLL_FREEER(sll) ((sll)->freeer)
#define QSE_SLL_COMPER(sll) ((sll)->comper)

#define QSE_SLL_HEAD(sll)   ((sll)->head)
#define QSE_SLL_TAIL(sll)   ((sll)->tail)
#define QSE_SLL_SIZE(sll)   ((sll)->size)
#define QSE_SLL_SCALE(sll)  ((sll)->scale)

/****d* cmn/QSE_SLL_DPTR
 * NAME
 *  QSE_SLL_DPTR - get the data pointer in a node
 * SYNOPSIS
 */
#define QSE_SLL_DPTR(node)  ((node)->dptr)
/******/

/****d* cmn/QSE_SLL_DLEN
 * NAME
 *  QSE_SLL_DLEN - get the length of data in a node
 * SYNOPSIS
 */
#define QSE_SLL_DLEN(node)  ((node)->dlen)
/******/

/****d* cmn/QSE_SLL_NEXT
 * NAME
 *  QSE_SLL_NEXT - get the next node
 * SYNOPSIS
 */
#define QSE_SLL_NEXT(node)  ((node)->next)
/******/

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (sll)

/****f* qse.cmn.sll/qse_sll_open
 * NAME
 *  qse_sll_open - create a singly linked list with extension area
 *
 * DESCRIPTION
 *  The qse_sll_open() function creates an empty singly linked list.
 *  If the memory manager mmgr is QSE_NULL, the function gets the default
 *  memory manager with QSE_MMGR_GETMMGR() and uses it if it is not QSE_NULL.
 *  The extension area is allocated when the positive extension size extension 
 *  is specified. It calls the extension initialization function initializer 
 *  after initializing the main area. The extension initializer is passed
 *  the pointer to the singly linked list created.
 *
 * RETURN
 *  The qse_sll_open() function returns the pointer to a new singly linked 
 *  list on success and QSE_NULL on failure.
 *
 * NOTES
 *  In the debug build, it fails an assertion if QSE_MMGR_GETMMGR() returns
 *  QSE_NULL when QSE_NULL is passed as the first parameter. In the release
 *  build, it returns QSE_NULL if such a thing happens. 
 *
 * SYNOPSIS
 */
qse_sll_t* qse_sll_open (
	qse_mmgr_t* mmgr  /* memory manager */ , 
	qse_size_t  ext   /* size of extension area in bytes */
);
/******/

/****f* qse.cmn.sll/qse_sll_close
 * NAME
 *  qse_sll_close - destroy a singly linked list 
 *
 * DESCRIPTION
 *  The qse_sll_close() function destroys a singly linked list freeing up
 *  the memory.
 *
 * SYNOPSIS
 */
void qse_sll_close (
	qse_sll_t* sll /* a singly linked list */
);
/******/

/****f* qse.cmn.sll/qse_sll_init
 * NAME
 *  qse_sll_init - initialize a singly linked list
 *
 * DESCRIPTION
 *  The qse_sll_init() function initializes a singly linked list. The memory
 *  should be allocated by a caller and be passed to it. The caller may declare
 *  a static variable of the qse_sll_t type and pass its address. A memory 
 *  manager still needs to be passed for node manipulation later.
 *  
 * RETURN
 *  The qse_sll_init() function returns the first parameter on success and
 *  QSE_NULL on failure.
 * 
 * SYNOPSIS
 */
qse_sll_t* qse_sll_init (
	qse_sll_t*  sll   /* an uninitialized singly linked list */,
	qse_mmgr_t* mmgr  /* a memory manager */
);
/******/

/****f* qse.cmn.sll/qse_sll_fini
 * NAME
 *  qse_sll_init - deinitialize a singly linked list
 *
 * SYNOPSIS
 */
void qse_sll_fini (
	qse_sll_t* sll  /* a singly linked list */
);
/******/

/****f* qse.cmn.sll/qse_sll_getsize
 * NAME
 *  qse_sll_getsize - get the number of nodes
 *
 * DESCRIPTION
 *  The qse_sll_getsize() function returns the number of the data nodes held
 *  in a singly linked list.
 *
 * SYNOPSIS
 */
qse_size_t qse_sll_getsize (
	qse_sll_t* sll  /* a singly linked list */
);
/******/


/****f* qse.cmn.sll/qse_sll_getscale
 * NAME
 *  qse_sll_getscale - get the scale factor
 *
 * SYNOPSIS
 */
int qse_sll_getscale (
	qse_sll_t* sll  /* a singly linked list */
);
/******/

/****f* qse.cmn.sll/qse_sll_setscale
 * NAME
 *  qse_sll_setscale - set the scale factor
 *
 * DESCRIPTION 
 *  The qse_sll_setscale() function sets the scale factor of the data length.
 *  A scale factor determines the actual length of data in bytes. A singly 
 *  linked list created with a scale factor of 1. The scale factor should be
 *  larger than 0 and less than 256.
 *
 * NOTES
 *  It is a bad idea to change the scale factor when a sll is not empty.
 *  
 * SYNOPSIS
 */
void qse_sll_setscale (
	qse_sll_t* sll /* a singly linked list */,
	int scale      /* a scale factor */
);
/******/

/****f* qse.cmn.sll/qse_sll_getcopier
 * NAME
 *  qse_sll_getfreeer - get the data copier
 *
 * SYNOPSIS
 */
qse_sll_copier_t qse_sll_getcopier (
	qse_sll_t* sll  /* a singly linked list */
);
/******/

/****f* qse.cmn.sll/qse_sll_setcopier
 * NAME 
 *  qse_sll_setcopier - set a data copier
 *
 * DESCRIPTION
 *  A special copier QSE_SLL_COPIER_INLINE is provided. This copier enables
 *  you to copy the data inline to the internal node. No freeer is invoked
 *  when the node is freeed.
 *
 *  You may set the copier to QSE_NULL to perform no special operation 
 *  when the data pointer is rememebered.
 * 
 * SYNOPSIS
 */
void qse_sll_setcopier (
	qse_sll_t* sll          /* a singly linked list */, 
	qse_sll_copier_t copier /* a data copier */
);
/******/

/****f* qse.cmn.sll/qse_sll_getfreeer
 * NAME
 *  qse_sll_getfreeer - get the data freeer
 *
 * SYNOPSIS
 */
qse_sll_freeer_t qse_sll_getfreeer (
	qse_sll_t* sll  /* a singly linked list */
);
/******/

/****f* qse.cmn.sll/qse_sll_setfreeer
 * NAME
 *  qse_sll_setfreeer - set a data freeer
 *
 * DESCRIPTION
 *  The freeer is called when a node containing the element is destroyed.
 * 
 * SYNOPSIS
 */
void qse_sll_setfreeer (
	qse_sll_t* sll          /* a singly linked list */,
	qse_sll_freeer_t freeer /* a data freeer */
);
/******/

/****f* qse.cmn.sll/qse_sll_getcomper
 * NAME
 *  qse_sll_getcomper - get the data comparator
 *
 * SYNOPSIS
 */
qse_sll_comper_t qse_sll_getcomper (
	qse_sll_t* sll  /* a singly linked list */
);
/******/

/****f* qse.cmn.sll/qse_sll_setcomper
 * NAME
 *  qse_sll_setcomper - set the data comparator
 *
 * SYNOPSIS
 */
void qse_sll_setcomper (
	qse_sll_t*       sll     /* a singly linked list */,
	qse_sll_comper_t comper  /* a comparator */
);
/******/

/****f* qse.cmn.sll/qse_sll_gethead
 * NAME
 *  qse_sll_gethead - get the head node
 *
 * SYNOPSIS
 */
qse_sll_node_t* qse_sll_gethead (
	qse_sll_t* sll  /* a singly linked list */
);
/******/

/****f* qse.cmn.sll/qse_sll_gettail
 * NAME
 *  qse_sll_gettail - get the tail node
 *
 * SYNOPSIS
 */
qse_sll_node_t* qse_sll_gettail (
	qse_sll_t* sll  /* a singly linked list */
);
/******/

/****f* qse.cmn.sll/qse_sll_search
 * NAME
 *  qse_sll_search - find a node
 *
 * DESCRIPTION
 *  The qse_sll_search() function traverses a list to find a node containing
 *  the same value as the the data pointer and length. The traversal begins
 *  from the next node of the positional node. If the positional node is 
 *  QSE_NULL, the traversal begins from the head node. 
 *
 * RETURN
 *  The pointer to the node found. Otherwise, QSE_NULL.
 *
 * NOTES
 *  No reverse search is provided because a reverse traversal can not be 
 *  achieved efficiently.
 *
 * SYNOPSIS
 */
qse_sll_node_t* qse_sll_search (
	qse_sll_t*      sll   /* a singly linked list */,
	qse_sll_node_t* pos   /* a positional node */,
	const void*     dptr  /* a data pointer */,
	qse_size_t      dlen  /* a data length */
);
/******/

/****f* qse.cmn.sll/qse_sll_insert
 * NAME
 *  qse_sll_insert - insert data to a new node
 *
 * DESCRIPTION
 *  There is performance penalty unless the positional node is neither
 *  the head node nor QSE_NULL. You should consider a different data
 *  structure such as a doubly linked list if you need to insert data
 *  into a random position.
 *
 * RETURN
 *  The pointer to a new node on success and QSE_NULL on failure:w
 *
 * SYNOPSIS
 */
qse_sll_node_t* qse_sll_insert (
	qse_sll_t*      sll  /* a singly linked list */,
	qse_sll_node_t* pos  /* a node before which a new node is inserted */,
	void*           dptr /* the pointer to the data */,
	qse_size_t      dlen /* the length of the data */
);
/******/

/****f* qse.cmn.sll/qse_sll_delete
 * NAME
 *  qse_sll_delete - delete a node
 *
 * DESCRIPTION
 *  The qse_sll_delete() function deletes a node. 
 * 
 * SYNOPSIS
 */
void qse_sll_delete (
	qse_sll_t*      sll  /* a singly linked list */,
	qse_sll_node_t* pos  /* a node to delete */
);
/******/

/****f* qse.cmn.sll/qse_sll_clear 
 * NAME 
 *  qse_sll_clear - delete all nodes
 *
 * DESCRIPTION
 *  The qse_sll_clear() function empties a singly linked list by deletinng
 *  all the nodes.
 *
 * SYNOPSIS
 */
void qse_sll_clear (
	qse_sll_t* sll  /* a singly linked list */
);
/******/

qse_sll_node_t* qse_sll_pushhead (
	qse_sll_t* sll /* a singly linked list */,
	void*      dptr, 
	qse_size_t dlen
);

qse_sll_node_t* qse_sll_pushtail (
	qse_sll_t* sll /* a singly linked list */, 
	void*      dptr, 
	qse_size_t dlen
);


void qse_sll_pophead (
	qse_sll_t* sll
);

void qse_sll_poptail (
	qse_sll_t* sll
);

/****f* qse.cmn.sll/qse_sll_walk
 * NAME
 *  qse_sll_walk - traverse s singly linked list 
 *
 * DESCRIPTION
 *  A singly linked list allows uni-directional in-order traversal.
 *  The qse_sll_walk() function traverses a singly linkked list from its 
 *  head node down to its tail node as long as the walker function returns 
 *  QSE_SLL_WALK_FORWARD. A walker can return QSE_SLL_WALK_STOP to cause 
 *  immediate stop of traversal.
 * 
 *  For each node, the walker function is called and it is passed three
 *  parameters: the singly linked list, the visiting node, and the 
 *  user-defined data passed as the third parameter in a call to the 
 *  qse_sll_walk() function.
 * 
 * SYNOPSIS
 */
void qse_sll_walk (
	qse_sll_t*       sll     /* a singly linked list */,
	qse_sll_walker_t walker  /* a user-defined walker function */,
	void*            arg     /* the pointer to user-defined data */
);
/******/

#ifdef __cplusplus
}
#endif

#endif
