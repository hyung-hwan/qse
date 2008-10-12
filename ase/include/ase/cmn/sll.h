/*
 * $Id: sll.h 223 2008-06-26 06:44:41Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_CMN_SLL_H_
#define _ASE_CMN_SLL_H_

#include <ase/types.h>
#include <ase/macros.h>

/****t* ase.cmn.sll/ase_sll_walk_t
 * NAME
 *  ase_sll_walk_t - define return values for ase_sll_walker_t
 *
 * SEE ALSO
 *  ase_sll_walk, ase_sll_walker_t
 *
 * SYNOPSIS
 */
enum ase_sll_walk_t
{
	ASE_SLL_WALK_STOP    = 0,   
	ASE_SLL_WALK_FORWARD = 1
};
/******/

typedef struct ase_sll_t      ase_sll_t;
typedef struct ase_sll_node_t ase_sll_node_t;
typedef enum   ase_sll_walk_t ase_sll_walk_t;

/****b* ase.cmn.sll/ase_sll_copier_t
 * NAME
 *  ase_sll_copier_t - define a node contruction callback
 *
 * DESCRIPTION
 *  The ase_sll_copier_t defines a callback function for node construction.
 *  A node is contructed when a user adds data to a list. The user can
 *  define how the data to add can be maintained in the list. A singly
 *  linked list not specified with any copiers stores the data pointer and
 *  the data length into a node. A special copier ASE_SLL_COPIER_INLINE copies 
 *  the contents of the data a user provided into the node. You can use the
 *  ase_sll_setcopier() function to change the copier. 
 * 
 *  A copier should return the pointer to the copied data. If it fails to copy
 *  data, it may return ASE_NULL. You need to set a proper freeer to free up
 *  memory allocated for copy.
 *
 * SEE ALSO
 *  ase_sll_setcopier, ase_sll_getcopier, ASE_SLL_COPIER
 *
 * SYNOPSIS
 */
typedef void* (*ase_sll_copier_t) (
	ase_sll_t* sll    /* a map */,
	void*      dptr   /* the pointer to data to copy */,
	ase_size_t dlen   /* the length of data to copy */
);
/******/

/****b* ase.cmn.sll/ase_sll_freeer_t
 * NAME
 *  ase_sll_freeer_t - define a node destruction callback
 * SYNOPSIS
 */
typedef void (*ase_sll_freeer_t) (
	ase_sll_t* sll    /* a map */,
	void*      dptr   /* the pointer to data to free */,
	ase_size_t dlen   /* the length of data to free */
);
/******/

/****t* ase.cmn.sll/ase_sll_comper_t
 * NAME
 *  ase_sll_comper_t - define a data comparator
 *
 * DESCRIPTION
 *  The ase_sll_comper_t type defines a key comparator that is called when
 *  the list needs to compare data. A singly linked list is created with a
 *  default comparator that performs bitwise comparison.
 *
 *  The comparator should return 0 if the data are the same and a non-zero
 *  integer otherwise.
 *
 * SYNOPSIS
 */
typedef int (*ase_sll_comper_t) (
	ase_sll_t*  sll    /* a singly linked list */, 
	const void* dptr1  /* a data pointer */,
	ase_size_t  dlen1  /* a data length */,
	const void* dptr2  /* a data pointer */,
	ase_size_t  dlen2  /* a data length */
);
/******/

/****b* ase.cmn.sll/ase_sll_walker_t
 * NAME
 *  ase_sll_walker_t - define a list traversal callback for each node
 *
 * DESCRIPTION
 *  The ase_sll_walk() calls a callback function of the type ase_sll_walker_t
 *  for each node until it returns ASE_SLL_WALK_STOP. The walker should return
 *  ASE_SLL_WALK_FORWARD to let ase_sll_walk() continue visiting the next node.
 *  The third parameter to ase_sll_walk() is passed to the walker as the third
 *  parameter.
 *
 * SEE ALSO
 *  ase_sll_walk, ase_sll_walk_t
 *
 * SYNOPSIS
 */
typedef ase_sll_walk_t (*ase_sll_walker_t) (
	ase_sll_t*      sll   /* a map */,
	ase_sll_node_t* node  /* a visited node */,
	void*           arg   /* user-defined data */
);
/******/

/****s* ase.cmn.sll/ase_sll_t
 * NAME
 *  ase_sll_t - define a singly linked list
 * 
 * DESCRPTION
 *  The ase_sll_t type defines a singly lnked list.
 *
 * SYNOPSIS
 */
struct ase_sll_t
{
	ase_mmgr_t*      mmgr;   /* memory manager */

	ase_sll_copier_t copier; /* data copier */
	ase_sll_freeer_t freeer; /* data freeer */
	ase_sll_comper_t comper; /* data comparator */
	ase_byte_t       scale;  /* scale factor */

	ase_size_t       size;   /* the number of nodes */
	ase_sll_node_t*  head;   /* the head node */
	ase_sll_node_t*  tail;   /* the tail node */
};
/******/

/****s* ase.cmn.sll/ase_sll_node_t
 * NAME
 *  ase_sll_node_t - define a list node
 *
 * DESCRIPTION
 *  The ase_sll_node_t type defines a list node containing a data pointer and 
 *  and data length. 
 *
 * SEE ALSO
 *  ASE_SLL_DPTR, ASE_SLL_DLEN, ASE_SLL_NEXT
 *
 * SYNOPSIS
 */
struct ase_sll_node_t
{
	void*           dptr; /* the pointer to data */
	ase_size_t      dlen; /* the length of data */
	ase_sll_node_t* next; /* the pointer to the next node */
};
/******/

#define ASE_SLL_COPIER_SIMPLE ase_sll_copysimple
#define ASE_SLL_COPIER_INLINE ase_sll_copyinline

#define ASE_SLL_MMGR(sll)   ((sll)->mmgr)
#define ASE_SLL_COPIER(sll) ((sll)->copier)
#define ASE_SLL_FREEER(sll) ((sll)->freeer)
#define ASE_SLL_COMPER(sll) ((sll)->comper)

#define ASE_SLL_HEAD(sll)   ((sll)->head)
#define ASE_SLL_TAIL(sll)   ((sll)->tail)
#define ASE_SLL_SIZE(sll)   ((sll)->size)
#define ASE_SLL_SCALE(sll)  ((sll)->scale)

#define ASE_SLL_DPTR(node)  ((node)->dptr)
#define ASE_SLL_DLEN(node)  ((node)->dlen)
#define ASE_SLL_NEXT(node)  ((node)->next)

#ifdef __cplusplus
extern "C" {
#endif

/****f* ase.cmn.sll/ase_sll_open
 * NAME
 *  ase_sll_open - create a singly linked list with extension area
 *
 * DESCRIPTION
 *  The ase_sll_open() function creates an empty singly linked list.
 *  If the memory manager mmgr is ASE_NULL, the function gets the default
 *  memory manager with ASE_MMGR_GETMMGR() and uses it if it is not ASE_NULL.
 *  The extension area is allocated when the positive extension size extension 
 *  is specified. It calls the extension initialization function initializer 
 *  after initializing the main area. The extension initializer is passed
 *  the pointer to the singly linked list created.
 *
 * RETURN
 *  The ase_sll_open() function returns the pointer to a new singly linked 
 *  list on success and ASE_NULL on failure.
 *
 * NOTES
 *  In the debug build, it fails an assertion if ASE_MMGR_GETMMGR() returns
 *  ASE_NULL when ASE_NULL is passed as the first parameter. In the release
 *  build, it returns ASE_NULL if such a thing happens. 
 *
 * SYNOPSIS
 */
ase_sll_t* ase_sll_open (
	ase_mmgr_t* mmgr  /* memory manager */ , 
	ase_size_t  ext   /* size of extension area in bytes */
);
/******/

/****f* ase.cmn.sll/ase_sll_close
 * NAME
 *  ase_sll_close - destroy a singly linked list 
 *
 * DESCRIPTION
 *  The ase_sll_close() function destroys a singly linked list freeing up
 *  the memory.
 *
 * SYNOPSIS
 */
void ase_sll_close (
	ase_sll_t* sll /* a singly linked list */
);
/******/

/****f* ase.cmn.sll/ase_sll_init
 * NAME
 *  ase_sll_init - initialize a singly linked list
 *
 * DESCRIPTION
 *  The ase_sll_init() function initializes a singly linked list. The memory
 *  should be allocated by a caller and be passed to it. The caller may declare
 *  a static variable of the ase_sll_t type and pass its address. A memory 
 *  manager still needs to be passed for node manipulation later.
 *  
 * RETURN
 *  The ase_sll_init() function returns the first parameter on success and
 *  ASE_NULL on failure.
 * 
 * SYNOPSIS
 */
ase_sll_t* ase_sll_init (
	ase_sll_t*  sll   /* an uninitialized singly linked list */,
	ase_mmgr_t* mmgr  /* a memory manager */
);
/******/

/****f* ase.cmn.sll/ase_sll_fini
 * NAME
 *  ase_sll_init - deinitialize a singly linked list
 *
 * SYNOPSIS
 */
void ase_sll_fini (
	ase_sll_t* sll  /* a singly linked list */
);
/******/

/****f* ase.cmn.sll/ase_sll_getextension
 * NAME
 *  ase_sll_getextension - get the pointer to the extension
 *
 * DESCRIPTION
 *  The ase_sll_getextension() function returns the pointer to the extension.
 *
 * SYNOPSIS
 */
void* ase_sll_getextension (
	ase_sll_t* sll  /* a singly linked list */
);
/******/

/****f* ase.cmn.sll/ase_sll_getmmgr
 * NAME
 *  ase_sll_getmmgr - get the memory manager
 *
 * SYNOPSIS
 */
ase_mmgr_t* ase_sll_getmmgr (
	ase_sll_t* sll  /* a singly linked list */
);
/******/

/****f* ase.cmn.sll/ase_sll_setmmgr
 * NAME
 *  ase_sll_setmmgr - set the memory manager
 *
 * SYNOPSIS
 */
void ase_sll_setmmgr (
	ase_sll_t*  sll   /* a singly linked list */,
	ase_mmgr_t* mmgr  /* a memory manager */
);
/******/

/****f* ase.cmn.sll/ase_sll_getsize
 * NAME
 *  ase_sll_getsize - get the number of nodes
 *
 * DESCRIPTION
 *  The ase_sll_getsize() function returns the number of the data nodes held
 *  in a singly linked list.
 *
 * SYNOPSIS
 */
ase_size_t ase_sll_getsize (
	ase_sll_t* sll  /* a singly linked list */
);
/******/


/****f* ase.cmn.sll/ase_sll_getscale
 * NAME
 *  ase_sll_getscale - get the scale factor
 *
 * SYNOPSIS
 */
int ase_sll_getscale (
	ase_sll_t* sll  /* a singly linked list */
);
/******/

/****f* ase.cmn.sll/ase_sll_setscale
 * NAME
 *  ase_sll_setscale - set the scale factor
 *
 * DESCRIPTION 
 *  The ase_sll_setscale() function sets the scale factor of the data length.
 *  A scale factor determines the actual length of data in bytes. A singly 
 *  linked list created with a scale factor of 1. The scale factor should be
 *  larger than 0 and less than 256.
 *
 * NOTES
 *  It is a bad idea to change the scale factor when a sll is not empty.
 *  
 * SYNOPSIS
 */
void ase_sll_setscale (
	ase_sll_t* sll /* a singly linked list */,
	int scale      /* a scale factor */
);
/******/

/****f* ase.cmn.sll/ase_sll_getcopier
 * NAME
 *  ase_sll_getfreeer - get the data copier
 *
 * SYNOPSIS
 */
ase_sll_copier_t ase_sll_getcopier (
	ase_sll_t* sll  /* a singly linked list */
);
/******/

/****f* ase.cmn.sll/ase_sll_setcopier
 * NAME 
 *  ase_sll_setcopier - set a data copier
 *
 * DESCRIPTION
 *  A special copier ASE_SLL_COPIER_INLINE is provided. This copier enables
 *  you to copy the data inline to the internal node. No freeer is invoked
 *  when the node is freeed.
 *
 *  You may set the copier to ASE_NULL to perform no special operation 
 *  when the data pointer is rememebered.
 * 
 * SYNOPSIS
 */
void ase_sll_setcopier (
	ase_sll_t* sll          /* a singly linked list */, 
	ase_sll_copier_t copier /* a data copier */
);
/******/

/****f* ase.cmn.sll/ase_sll_getfreeer
 * NAME
 *  ase_sll_getfreeer - get the data freeer
 *
 * SYNOPSIS
 */
ase_sll_freeer_t ase_sll_getfreeer (
	ase_sll_t* sll  /* a singly linked list */
);
/******/

/****f* ase.cmn.sll/ase_sll_setfreeer
 * NAME
 *  ase_sll_setfreeer - set a data freeer
 *
 * DESCRIPTION
 *  The freeer is called when a node containing the element is destroyed.
 * 
 * SYNOPSIS
 */
void ase_sll_setfreeer (
	ase_sll_t* sll          /* a singly linked list */,
	ase_sll_freeer_t freeer /* a data freeer */
);
/******/

/****f* ase.cmn.sll/ase_sll_getcomper
 * NAME
 *  ase_sll_getcomper - get the data comparator
 *
 * SYNOPSIS
 */
ase_sll_comper_t ase_sll_getcomper (
	ase_sll_t* sll  /* a singly linked list */
);
/******/

/****f* ase.cmn.sll/ase_sll_setcomper
 * NAME
 *  ase_sll_setcomper - set the data comparator
 *
 * SYNOPSIS
 */
void ase_sll_setcomper (
	ase_sll_t*       sll     /* a singly linked list */,
	ase_sll_comper_t comper  /* a comparator */
);
/******/

/****f* ase.cmn.sll/ase_sll_gethead
 * NAME
 *  ase_sll_gethead - get the head node
 *
 * SYNOPSIS
 */
ase_sll_node_t* ase_sll_gethead (
	ase_sll_t* sll  /* a singly linked list */
);
/******/

/****f* ase.cmn.sll/ase_sll_gettail
 * NAME
 *  ase_sll_gettail - get the tail node
 *
 * SYNOPSIS
 */
ase_sll_node_t* ase_sll_gettail (
	ase_sll_t* sll  /* a singly linked list */
);
/******/

/****f* ase.cmn.sll/ase_sll_search
 * NAME
 *  ase_sll_search - find a node
 *
 * DESCRIPTION
 *  The ase_sll_search() function traverses a list to find a node containing
 *  the same value as the the data pointer and length. The traversal begins
 *  from the next node of the positional node. If the positional node is 
 *  ASE_NULL, the traversal begins from the head node. 
 *
 * RETURN
 *  The pointer to the node found. Otherwise, ASE_NULL.
 *
 * NOTES
 *  No reverse search is provided because a reverse traversal can not be 
 *  achieved efficiently.
 *
 * SYNOPSIS
 */
ase_sll_node_t* ase_sll_search (
	ase_sll_t*      sll   /* a singly linked list */,
	ase_sll_node_t* pos   /* a positional node */,
	const void*     dptr  /* a data pointer */,
	ase_size_t      dlen  /* a data length */
);
/******/

/****f* ase.cmn.sll/ase_sll_insert
 * NAME
 *  ase_sll_insert - insert data to a new node
 *
 * DESCRIPTION
 *  There is performance penalty unless the positional node is neither
 *  the head node nor ASE_NULL. You should consider a different data
 *  structure such as a doubly linked list if you need to insert data
 *  into a random position.
 *
 * RETURN
 *  The pointer to a new node on success and ASE_NULL on failure:w
 *
 * SYNOPSIS
 */
ase_sll_node_t* ase_sll_insert (
	ase_sll_t*      sll  /* a singly linked list */,
	ase_sll_node_t* pos  /* a node before which a new node is inserted */,
	void*           dptr /* the pointer to the data */,
	ase_size_t      dlen /* the length of the data */
);
/******/

/****f* ase.cmn.sll/ase_sll_delete
 * NAME
 *  ase_sll_delete - delete a node
 *
 * DESCRIPTION
 *  The ase_sll_delete() function deletes a node. 
 * 
 * SYNOPSIS
 */
void ase_sll_delete (
	ase_sll_t*      sll  /* a singly linked list */,
	ase_sll_node_t* pos  /* a node to delete */
);
/******/

/****f* ase.cmn.sll/ase_sll_clear 
 * NAME 
 *  ase_sll_clear - delete all nodes
 *
 * DESCRIPTION
 *  The ase_sll_clear() function empties a singly linked list by deletinng
 *  all the nodes.
 *
 * SYNOPSIS
 */
void ase_sll_clear (
	ase_sll_t* sll  /* a singly linked list */
);
/******/

ase_sll_node_t* ase_sll_pushhead (
	ase_sll_t* sll /* a singly linked list */,
	void*      dptr, 
	ase_size_t dlen
);

ase_sll_node_t* ase_sll_pushtail (
	ase_sll_t* sll /* a singly linked list */, 
	void*      dptr, 
	ase_size_t dlen
);


void ase_sll_pophead (
	ase_sll_t* sll
);

void ase_sll_poptail (
	ase_sll_t* sll
);

/****f* ase.cmn.sll/ase_sll_walk
 * NAME
 *  ase_sll_walk - traverse s singly linked list 
 *
 * DESCRIPTION
 *  A singly linked list allows uni-directional in-order traversal.
 *  The ase_sll_walk() function traverses a singly linkked list from its 
 *  head node down to its tail node as long as the walker function returns 
 *  ASE_SLL_WALK_FORWARD. A walker can return ASE_SLL_WALK_STOP to cause 
 *  immediate stop of traversal.
 * 
 *  For each node, the walker function is called and it is passed three
 *  parameters: the singly linked list, the visiting node, and the 
 *  user-defined data passed as the third parameter in a call to the 
 *  ase_sll_walk() function.
 * 
 * SYNOPSIS
 */
void ase_sll_walk (
	ase_sll_t*       sll     /* a singly linked list */,
	ase_sll_walker_t walker  /* a user-defined walker function */,
	void*            arg     /* the pointer to user-defined data */
);
/******/

/* 
 * Causes a singly linked list to remember the pointer and the length.
 * Use ASE_SLL_COPIER_SIMPLE instead.
 */
void* ase_sll_copysimple (
	ase_sll_t* sll   /* a singly linked list */,
	void*      data  /* the pointer to data */,
	ase_size_t len   /* the length of data */
);

/* 
 * Causes a singly linked list to copy in data to a node.
 * Use ASE_SLL_COPIER_INLINE instead.
 */
void* ase_sll_copyinline (
	ase_sll_t* sll   /* a singly linked list */,
	void*      data  /* the pointer to data */, 
	ase_size_t len   /* the length of data */
);

#ifdef __cplusplus
}
#endif

#endif
