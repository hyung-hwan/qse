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

struct ase_sll_t
{
	/* map owner. passed to freeval and sameval as the first argument */
	void* owner;

	ase_sll_node_t* head;
	ase_sll_node_t* tail;

	ase_size_t size;

	/* the mmgr pointed at by mmgr should be valid
	 * while the list is alive as the contents are 
	 * not replicated */
	ase_mmgr_t* mmgr;
};

#ifdef __cplusplus
extern "C" {
#endif

ase_sll_t* ase_sll_open (ase_mmgr_t* mmgr);
void ase_sll_close (ase_sll_t* sll);
void ase_sll_clear (ase_sll_t* sll);

ase_size_t ase_sll_getsize (ase_sll_t* sll);

#ifdef __cplusplus
}
#endif

#endif
