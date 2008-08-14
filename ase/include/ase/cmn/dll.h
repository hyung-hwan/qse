/*
 * $Id: map.h 223 2008-06-26 06:44:41Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_CMN_DLL_H_
#define _ASE_CMN_DLL_H_

#include <ase/types.h>
#include <ase/macros.h>

/*
 * Singly Linked List
 */
typedef struct ase_dll_t ase_dll_t;
typedef struct ase_dll_node_t ase_dll_node_t;

/* data copier */
typedef void* (*ase_dll_copier_t) (ase_dll_t* dll, void* dptr, ase_size_t dlen);

/* data freeer */
typedef void (*ase_dll_freeer_t) (ase_dll_t* dll, void* dptr, ase_size_t dlen);

/* node visitor */
typedef int (*ase_dll_walker_t) (
	ase_dll_t* dll, ase_dll_node_t* node, void* arg);

struct ase_dll_t
{
	ase_mmgr_t* mmgr;

	ase_dll_copier_t copier;
	ase_dll_freeer_t freeer;

	ase_size_t size;
	ase_dll_node_t* head;
	ase_dll_node_t* tail;
};

struct ase_dll_node_t
{
	void* dptr; /* pointer to the beginning of data */
	ase_size_t dlen; /* length of data in bytes */
	ase_dll_node_t* next; /* pointer to the next node */
	ase_dll_node_t* prev; /* pointer to the prev node */
};


enum ase_dll_walk_t
{
	ASE_DLL_WALK_STOP = 0,
	ASE_DLL_WALK_FORWARD = 1
};

#define ASE_DLL_COPIER_INLINE ase_dll_copyinline

#define ASE_DLL_HEAD(dll) ((dll)->head)
#define ASE_DLL_TAIL(dll) ((dll)->tail)
#define ASE_DLL_SIZE(dll) ((dll)->size)

#define ASE_DLL_DPTR(n) ((n)->dptr)
#define ASE_DLL_DLEN(n) ((n)->dlen)
#define ASE_DLL_NEXT(n) ((n)->next)
#define ASE_DLL_PREV(n) ((n)->prev)

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * NAME creates a new singly linked list 
 * RETURNS a pointer to a newly created singly linked list
 */
ase_dll_t* ase_dll_open (
	ase_mmgr_t* mmgr /* memory manager */
);

/* 
 * NAME creates a new singly linked list with extension 
 * RETURNS a pointer to a newly created singly linked list
 */
ase_dll_t* ase_dll_openx (
	ase_mmgr_t* mmgr /* memory manager */ , 
	ase_size_t extension /* size of extension in bytes */,
	ase_fuser_t fuser
);

/* 
 * NAME destroys a singly linked list 
 */
void ase_dll_close (
	ase_dll_t* dll /* a singly linked list */
);

/* 
 * NAME deletes all elements of a singly linked list 
 */
void ase_dll_clear (
	ase_dll_t* dll /* a singly linked list */
);

/*
 * NAME specifies how to clone an element
 *
 * DESCRIPTION
 *  A special copier ASE_DLL_COPIER_INLINE is provided. This copier enables
 *  you to copy the data inline to the internal node. No freeer is invoked
 *  when the node is freeed.
 *
 *  You may set the copier to ASE_NULL to perform no special operation 
 *  when the data pointer is rememebered.
 */
void ase_dll_setcopier (
	ase_dll_t* dll /* a singly linked list */, 
	ase_dll_copier_t copier /* a element copier */
);

ase_dll_copier_t ase_dll_getcopier (
	ase_dll_t* dll /* a singly linked list */
);

/*
 * NAME specifies how to destroy an element
 *
 * DESCRIPTION
 *  The freeer is called when a node containing the element is destroyed.
 */
void ase_dll_setfreeer (
	ase_dll_t* dll /* a singly linked list */,
	ase_dll_freeer_t freeer /* a element freeer */
);

ase_dll_freeer_t ase_dll_getfreeer (
	ase_dll_t* dll /* a singly linked list */
);

/*
 * NAME Gets the pointer to the extension area
 * RETURN the pointer to the extension area
 */
void* ase_dll_getextension (
	ase_dll_t* dll /* a singly linked list */
);

/*
 * NAME Gets the number of elements held in a singly linked list
 * RETURN the number of elements the list holds
 */
ase_size_t ase_dll_getsize (
	ase_dll_t* dll /* a singly linked list */
);

/*
 * NAME Gets the head(first) node 
 * RETURN the tail node of a singly linked list
 */
ase_dll_node_t* ase_dll_gethead (
	ase_dll_t* dll /* a singly linked list */
);

/* 
 * NAME Gets the tail(last) node 
 * RETURN the tail node of a singly linked list
 */
ase_dll_node_t* ase_dll_gettail (
	ase_dll_t* dll /* a singly linked list */
);

/* 
 * NAME Inserts data before a positional node given 
 *
 * DESCRIPTION
 *   Inserts data.
 */
ase_dll_node_t* ase_dll_insert (
	ase_dll_t* dll /* a singly linked list */,
	ase_dll_node_t* pos /* a node before which a new node is inserted */,
	void* dptr /* the pointer to the data */ ,
	ase_size_t dlen /* the length of the data in bytes */
);

ase_dll_node_t* ase_dll_pushhead (
	ase_dll_t* dll /* a singly linked list */,
	void* dptr, 
	ase_size_t dlen
);

ase_dll_node_t* ase_dll_pushtail (
	ase_dll_t* dll /* a singly linked list */, 
	void* dptr, 
	ase_size_t dlen
);

void ase_dll_delete (
	ase_dll_t* dll, 
	ase_dll_node_t* pos
);

void ase_dll_pophead (
	ase_dll_t* dll
);

void ase_dll_poptail (
	ase_dll_t* dll
);

/* 
 * NAME Traverses s singly linked list 
 *
 * DESCRIPTION
 *   A singly linked list allows uni-directional in-order traversal.
 *   The ase_dll_walk() function traverses a singly linkked list from its 
 *   head node down to its tail node as long as the walker function returns 
 *   ASE_DLL_WALK_FORWARD. A walker can return ASE_DLL_WALK_STOP to cause 
 *   immediate stop of traversal.
 *   For each node, the walker function is called and it is passed three
 *   parameters: the singly linked list, the visiting node, and the 
 *   user-defined data passed as the third parameter in a call to the 
 *   ase_dll_walk() function.
 */
void ase_dll_walk (
	ase_dll_t* dll /* a singly linked list */,
	ase_dll_walker_t walker /* a user-defined walker function */,
	void* arg /* pointer to user-defined data */
);

/* 
 * Causes a singly linked list to copy in data to a node.
 * Use ASE_DLL_COPIER_INLINE instead.
 */
void* ase_dll_copyinline (
	ase_dll_t* dll /* a singly linked list */,
	void* data /* pointer to data to copy */ , 
	ase_size_t len /* length of data in bytes */
);

#ifdef __cplusplus
}
#endif

#endif
