/*
 * $Id: tgp.h 233 2008-06-29 13:41:11Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_TGP_TGP_H_
#define _ASE_TGP_TGP_H_

#include <ase/types.h>
#include <ase/macros.h>

typedef struct ase_tgp_t ase_tgp_t;

enum ase_tgp_iocmd_t
{
	ASE_TGP_IO_OPEN = 0,
	ASE_TGP_IO_CLOSE = 1,
	ASE_TGP_IO_READ = 2,
	ASE_TGP_IO_WRITE = 3
};

typedef ase_ssize_t (*ase_tgp_io_t) (
	int cmd, void* arg, ase_char_t* data, ase_size_t count);

#ifdef __cplusplus
extern "C" {
#endif

ase_tgp_t* ase_tgp_open (ase_mmgr_t* mmgr);
void ase_tgp_close (ase_tgp_t* tgp);

void ase_tgp_setassocdata (ase_tgp_t* tgp, void* data);
void* ase_tgp_getassocdata (ase_tgp_t* tgp);

void ase_tgp_setio (ase_tgp_t* tgp, ase_tgp_io_t* io);
void ase_tgp_getio (ase_tgp_t* tgp, ase_tgp_io_t* io);

#ifdef __cplusplus
}
#endif

#endif
