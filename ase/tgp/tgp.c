/*
 * $Id$
 */

#include <ase/tgp/tgp.h>
#include <ase/cmn/mem.h>

struct ase_tgp_t
{
	ase_mmgr_t mmgr;
};

ase_tgp_t* ase_tgp_open (ase_mmgr_t* mmgr)
{
	ase_tgp_t* tgp;

	if (mmgr == ASE_NULL) mmgr = ase_get_mmgr ();
	ASE_ASSERT (mmgr != ASE_NULL);

	tgp = ASE_MALLOC (mmgr, ASE_SIZEOF(*tgp));
	if (tgp == ASE_NULL) return ASE_NULL;


	ase_memset (tgp, 0, ASE_SIZEOF(*tgp));
	ase_memcpy (&tgp->mmgr, mmgr, ASE_SIZEOF(*mmgr));


	return tgp;
}

void ase_tgp_close (ase_tgp_t* tgp)
{
	ASE_FREE (&tgp->mmgr, tgp);
}

