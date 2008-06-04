#ifndef _ASE_TGP_H_
#define _ASE_TGP_H_

#include <ase/cmn/types.h>
#include <ase/cmn/macros.h>

typedef struct ase_tgp_t ase_tgp_t;

#ifdef __cplusplus
extern "C" {
#endif

ase_tgp_t* ase_tgp_open (ase_mmgr_t* mmgr);
void ase_tgp_close (ase_tgp_t* tgp);

#ifdef __cplusplus
}
#endif

#endif
