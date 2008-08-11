/*
 * $Id: map.h 223 2008-06-26 06:44:41Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_CMN_SLL_H_
#define _ASE_CMN_SLL_H_

#include <ase/types.h>
#include <ase/macros.h>

/*
 * Singly Linked List
 */
typedef struct ase_sll_t ase_sll_t;
typedef struct ase_sll_node_t ase_sll_node_t;
typedef enum ase_sll_walk_t ase_sll_walk_t;

struct ase_sll_node_t
{
	struct
	{
		void* ptr;      /* pointer to the beginning of data */
		ase_size_t len; /* length of data in bytes */
	} data;

	ase_sll_node_t* next;   /* pointer to the next node */
};

typedef void* (*ase_sll_copier_t) (ase_sll_t* sll, void* data, ase_size_t len);
typedef void (*ase_sll_freeer_t) (ase_sll_t* sll, void* data, ase_size_t len);
typedef ase_sll_walk_t (*ase_sll_walker_t) (ase_sll_t* sll, ase_sll_node_t* node, void* arg);

enum ase_sll_walk_t
{
	ASE_SLL_WALK_STOP = 0,
	ASE_SLL_WALK_FORWARD = 1
};

#define ASE_SLL_COPIER_INLINE ase_sll_copyinline

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * NAME creates a new singly linked list 
 * RETURNS a pointer to a newly created singly linked list
 */
ase_sll_t* ase_sll_open (
	ase_mmgr_t* mmgr /* memory manager */
);

/* 
 * NAME creates a new singly linked list with extension 
 * RETURNS a pointer to a newly created singly linked list
 */
ase_sll_t* ase_sll_openx (
	ase_mmgr_t* mmgr /* memory manager */ , 
	ase_size_t extension /* size of extension in bytes */,
	ase_fuser_t fuser
);

/* 
 * NAME destroys a singly linked list 
 */
void ase_sll_close (
	ase_sll_t* sll /* a singly linked list */
);

/* 
 * NAME deletes all elements of a singly linked list 
 */
void ase_sll_clear (
	ase_sll_t* sll /* a singly linked list */
);

/*
 * NAME specifies how to clone an element
 *
 * DESCRIPTION
 *  A special copier ASE_SLL_COPIER_INLINE is provided. This copier enables
 *  you to copy the data inline to the internal node. No freeer is invoked
 *  when the node is freeed.
 *
 *  You may set the copier to ASE_NULL to perform no special operation 
 *  when the data pointer is rememebered.
 */
void ase_sll_setcopier (
	ase_sll_t* sll /* a singly linked list */, 
	ase_sll_copier_t copier /* a element copier */
);

ase_sll_copier_t ase_sll_getcopier (
	ase_sll_t* sll /* a singly linked list */
);

/*
 * NAME specifies how to destroy an element
 *
 * DESCRIPTION
 *  The freeer is called when a node containing the element is destroyed.
 */
void ase_sll_setfreeer (
	ase_sll_t* sll /* a singly linked list */,
	ase_sll_freeer_t freeer /* a element freeer */
);

ase_sll_freeer_t ase_sll_getfreeer (
	ase_sll_t* sll /* a singly linked list */
);

/*
 * NAME Gets the pointer to the extension area
 * RETURN the pointer to the extension area
 */
void* ase_sll_getextension (
	ase_sll_t* sll /* a singly linked list */
);

/*
 * NAME Gets the number of elements held in a singly linked list
 * RETURN the number of elements the list holds
 */
ase_size_t ase_sll_getsize (
	ase_sll_t* sll /* a singly linked list */
);

/*
 * NAME Gets the head(first) node 
 * RETURN the tail node of a singly linked list
 */
ase_sll_node_t* ase_sll_gethead (
	ase_sll_t* sll /* a singly linked list */
);

/* 
 * NAME Gets the tail(last) node 
 * RETURN the tail node of a singly linked list
 */
ase_sll_node_t* ase_sll_gettail (
	ase_sll_t* sll /* a singly linked list */
);

/* 
 * NAME Inserts data before a positional node given 
 *
 * DESCRIPTION
 *   There is performance penalty unless the positional node is neither
 *   the head node nor ASE_NULL. You should consider a different data
 *   structure such as a doubly linked list if you need to insert data
 *   into a random position.
 */
ase_sll_node_t* ase_sll_insert (
	ase_sll_t* sll /* a singly linked list */,
	ase_sll_node_t* pos /* a node before which a new node is inserted */,
	void* dptr /* the pointer to the data */ ,
	ase_size_t dlen /* the length of the data in bytes */
);

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

void ase_sll_delete (
	ase_sll_t* sll, 
	ase_sll_node_t* pos
);

void ase_sll_pophead (
	ase_sll_t* sll
);

void ase_sll_poptail (
	ase_sll_t* sll
);

/* 
 * NAME Traverses s singly linked list 
 *
 * DESCRIPTION
 *   A singly linked list allows uni-directional in-order traversal.
 *   The ase_sll_walk() function traverses a singly linkked list from its 
 *   head node down to its tail node as long as the walker function returns 
 *   ASE_SLL_WALK_FORWARD. A walker can return ASE_SLL_WALK_STOP to cause 
 *   immediate stop of traversal.
 *   For each node, the walker function is called and it is passed three
 *   parameters: the singly linked list, the visiting node, and the 
 *   user-defined data passed as the third parameter in a call to the 
 *   ase_sll_walk() function.
 */
void ase_sll_walk (
	ase_sll_t* sll /* a singly linked list */,
	ase_sll_walker_t walker /* a user-defined walker function */,
	void* arg /* pointer to user-defined data */
);

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
