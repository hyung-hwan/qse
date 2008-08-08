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

typedef void* (*ase_sll_copier_t) (ase_sll_t* sll, void* data, ase_size_t len);
typedef void (*ase_sll_freeer_t) (ase_sll_t* sll, void* data, ase_size_t len);
typedef int (*ase_sll_walker_t) (ase_sll_t* sll, void* data, ase_size_t len);

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

/*
 * Returns the pointer to the extension area
 */
void* ase_sll_getextension (
	ase_sll_t* sll /* a singly linked list */
);

/*
 * Gets the number of elements held in a singly linked list
 * RETURNS the number of elements the list holds
 */
ase_size_t ase_sll_getsize (
	ase_sll_t* sll /* a singly linked list */
);

/* Traverses s singly linked list */
void ase_sll_walk (ase_sll_t* sll, ase_sll_walker_t walker);

/* 
 * Causes a singly linked list to copy in data to a node.
 * Use ASE_SLL_COPIER_INLINE instead.
 */
void* ase_sll_copyinline (ase_sll_t* sll, void* data, ase_size_t len);

#ifdef __cplusplus
}
#endif

#endif
