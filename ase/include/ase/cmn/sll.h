/*
 * $Id: sll.h 223 2008-06-26 06:44:41Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_CMN_SLL_H_
#define _ASE_CMN_SLL_H_

#include <ase/types.h>
#include <ase/macros.h>

/* values to be returned by ase_sll_walker_t */
enum ase_sll_walk_t
{
	ASE_SLL_WALK_STOP = 0,
	ASE_SLL_WALK_FORWARD = 1
};

typedef struct ase_sll_t ase_sll_t;
typedef struct ase_sll_node_t ase_sll_node_t;
typedef enum ase_sll_walk_t ase_sll_walk_t;

/* data copier */
typedef void* (*ase_sll_copier_t) (
	ase_sll_t* sll,
	void* dptr,
	ase_size_t dlen
);

/* data freeer */
typedef void (*ase_sll_freeer_t) (
	ase_sll_t* sll,
	void* dptr,
	ase_size_t dlen
);

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
	ase_sll_t* sll    /* a singly linked list */, 
	const void* dptr1 /* a data pointer */,
	ase_size_t dlen1  /* a data length */,
	const void* dptr2 /* a data pointer */,
	ase_size_t dlen2  /* a data length */
);
/******/

/* node visitor */
typedef ase_sll_walk_t (*ase_sll_walker_t) (
	ase_sll_t* sll, ase_sll_node_t* node, void* arg);

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
	ase_mmgr_t* mmgr;

	ase_sll_copier_t copier;
	ase_sll_freeer_t freeer;
	ase_sll_comper_t comper;
	ase_byte_t scale;

	ase_size_t size;
	ase_sll_node_t* head;
	ase_sll_node_t* tail;
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
	void* dptr;           /* pointer to the beginning of data */
	ase_size_t dlen;      /* length of data in bytes */
	ase_sll_node_t* next; /* pointer to the next node */
};
/******/


#define ASE_SLL_COPIER_INLINE ase_sll_copyinline

#define ASE_SLL_HEAD(sll)  ((sll)->head)
#define ASE_SLL_TAIL(sll)  ((sll)->tail)
#define ASE_SLL_SIZE(sll)  ((sll)->size)
#define ASE_SLL_SCALE(sll) ((sll)->scale)

#define ASE_SLL_DPTR(n) ((n)->dptr)
#define ASE_SLL_DLEN(n) ((n)->dlen)
#define ASE_SLL_NEXT(n) ((n)->next)

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
 *  the pointer to a newly created singly linked list on success.
 *  ASE_NULL on failure.
 *
 * NOTES
 *  In the debug build, it fails the assertion if ASE_MMGR_SETMMGR() returns
 *  ASE_NULL when ASE_NULL is passed as the first parameter. In the release
 *  build, it returns ASE_NULL if such a thing happens. 
 *
 * SYNOPSIS
 */
ase_sll_t* ase_sll_open (
	ase_mmgr_t* mmgr /* memory manager */ , 
	ase_size_t ext   /* size of extension area in bytes */
);
/******/

/****f* ase.cmn.sll/ase_sll_close
 * NAME
 *  ase_sll_close - destroy a singly linked list 
 *
 * DESCRIPTION
 *  The ase_sll_close() function destroys a singly linked list
 *
 * SYNOPSIS
 */
void ase_sll_close (
	ase_sll_t* sll /* a singly linked list */
);
/******/

ase_sll_t* ase_sll_init (
	ase_sll_t* sll   /* an uninitialized singly linked list */,
	ase_mmgr_t* mmgr /* memory manager */
);

void ase_sll_fini (
	ase_sll_t* sll /* a singly linked list */
);

/*
 * NAME: Gets the pointer to the extension area
 * RETURN:: the pointer to the extension area
 */
void* ase_sll_getextension (
	ase_sll_t* sll /* a singly linked list */
);

/*
 * NAME: get the pointer to the memory manager in use 
 */
ase_mmgr_t* ase_sll_getmmgr (
	ase_sll_t* sll /* a singly linked list */
);

void ase_sll_setmmgr (
	ase_sll_t* sll,
	ase_mmgr_t* mmgr
);

/*
 * NAME: Gets the number of elements held in a singly linked list
 * RETURN: the number of elements the list holds
 */
ase_size_t ase_sll_getsize (
	ase_sll_t* sll /* a singly linked list */
);


int ase_sll_getscale (
	ase_sll_t* sll /* a singly linked list */
);

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

ase_sll_copier_t ase_sll_getcopier (
	ase_sll_t* sll /* a singly linked list */
);

/****f* ase.cmn.sll/ase_sll_setcopier
 * NAME 
 *  ase_sll_setcopier - specify how to clone an element
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
	ase_sll_copier_t copier /* a element copier */
);
/******/

ase_sll_freeer_t ase_sll_getfreeer (
	ase_sll_t* sll /* a singly linked list */
);

/****f* ase.cmn.sll/ase_sll_setfreeer
 * NAME
 *  ase_sll_setfreeer - specify how to destroy an element
 *
 * DESCRIPTION
 *  The freeer is called when a node containing the element is destroyed.
 * 
 * SYNOPSIS
 */
void ase_sll_setfreeer (
	ase_sll_t* sll          /* a singly linked list */,
	ase_sll_freeer_t freeer /* a element freeer */
);
/******/

ase_sll_comper_t ase_sll_getcomper (
	ase_sll_t* sll
);

void ase_sll_setcomper (
	ase_sll_t* sll          /* a singly linked list */,
	ase_sll_comper_t comper /* a comparator */
);

/*
 * NAME: Gets the head(first) node 
 * RETURN: the tail node of a singly linked list
 */
ase_sll_node_t* ase_sll_gethead (
	ase_sll_t* sll /* a singly linked list */
);

/* 
 * NAME: Gets the tail(last) node 
 * RETURN: the tail node of a singly linked list
 */
ase_sll_node_t* ase_sll_gettail (
	ase_sll_t* sll /* a singly linked list */
);

/****f* ase.cmn.sll/ase_sll_search
 * NAME
 *  ase_sll_search - find a node
 *
 * DESCRIPTION
 *  The ase_sll_search() function traverses the list to find a node containing
 *  the same value as the the data pointer and length. 
 *
 * SYNOPSIS
 */
ase_sll_node_t* ase_sll_search (
	ase_sll_t* sll    /* a singly linked list */,
	const void* dptr  /* a data pointer */,
	ase_size_t dlen   /* a data length */
);

/****f* ase.cmn.sll/ase_sll_insert
 * NAME
 *  ase_sll_insert - insert data before a positional node given 
 *
 * DESCRIPTION
 *  There is performance penalty unless the positional node is neither
 *  the head node nor ASE_NULL. You should consider a different data
 *  structure such as a doubly linked list if you need to insert data
 *  into a random position.
 *
 * SYNOPSIS
 */
ase_sll_node_t* ase_sll_insert (
	ase_sll_t* sll      /* a singly linked list */,
	ase_sll_node_t* pos /* a node before which a new node is inserted */,
	void* dptr          /* the pointer to the data */,
	ase_size_t dlen     /* the length of the data */
);
/******/

void ase_sll_delete (
	ase_sll_t* sll       /* a singly linked list */,
	ase_sll_node_t* pos /* a node to delete */
);

/****f* ase.cmn.sll/ase_sll_clear 
 * NAME 
 *  ase_sll_clear - delete all nodes
 *
 * DESCRIPTION
 *  The ase_sll_clear() function empties a singly linked list by deletinng
 *  all data nodes.
 *
 * SYNOPSIS
 */
void ase_sll_clear (
	ase_sll_t* sll /* a singly linked list */
);
/******/

ase_sll_node_t* ase_sll_pushhead (
	ase_sll_t* sll /* a singly linked list */,
	void* dptr, 
	ase_size_t dlen
);

ase_sll_node_t* ase_sll_pushtail (
	ase_sll_t* sll /* a singly linked list */, 
	void* dptr, 
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
 *  For each node, the walker function is called and it is passed three
 *  parameters: the singly linked list, the visiting node, and the 
 *  user-defined data passed as the third parameter in a call to the 
 *  ase_sll_walk() function.
 * 
 * SYNOPSIS
 */
void ase_sll_walk (
	ase_sll_t* sll          /* a singly linked list */,
	ase_sll_walker_t walker /* a user-defined walker function */,
	void* arg               /* pointer to user-defined data */
);
/******/

/* 
 * Causes a singly linked list to copy in data to a node.
 * Use ASE_SLL_COPIER_INLINE instead.
 */
void* ase_sll_copyinline (
	ase_sll_t* sll /* a singly linked list */,
	void* data /* pointer to data to copy */ , 
	ase_size_t len /* length of data in bytes */
);

#ifdef __cplusplus
}
#endif

#endif
