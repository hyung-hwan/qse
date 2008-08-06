/*
 * $Id$
 *
 * {License}
 */

#include <ase/cmn/sll.h>
#include <ase/cmn/mem.h>

ase_sll_t* ase_sll_open (ase_mmgr_t* mmgr)
{
	return ase_sll_openx (mmgr, 0, ASE_NULL);
}

ase_sll_openx (ase_mmgr_t* mmgr, ase_size_t extension, ase_fuser_t fuser)
{
	ase_sll_t* sll;

	ssll = ASE_MALLOC (mmgr, ASE_SIZEOF(ase_sll_t) + extension);
	if (sll == ASE_NULL) return ASE_NULL;

	ase_memset (sll, 0, ASE_SIZEOF(ase_sll_t) + extension);
	if (mmgr_fuser) mmgr = fuser (mmgr, sll + 1);
	sll->mmgr = mmgr;


	return mmgr;
}
