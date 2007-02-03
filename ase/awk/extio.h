/*
 * $Id: extio.h,v 1.17 2007-02-03 10:47:40 bacon Exp $
 *
 * {License}
 */

#ifndef _ASE_AWK_EXTIO_H_
#define _ASE_AWK_EXTIO_H_

#ifndef _ASE_AWK_AWK_H_
#error Never include this file directly. Include <ase/awk/awk.h> instead
#endif

#ifdef __cplusplus
extern "C"
#endif

int ase_awk_readextio (
	ase_awk_run_t* run, int in_type, 
	const ase_char_t* name, ase_awk_str_t* buf);

int ase_awk_writeextio_val (
	ase_awk_run_t* run, int out_type, 
	const ase_char_t* name, ase_awk_val_t* v);

int ase_awk_writeextio_str (
	ase_awk_run_t* run, int out_type, 
	const ase_char_t* name, ase_char_t* str, ase_size_t len);

int ase_awk_flushextio (
	ase_awk_run_t* run, int out_type, const ase_char_t* name);

int ase_awk_nextextio_read (
	ase_awk_run_t* run, int in_type, const ase_char_t* name);

int ase_awk_nextextio_write (
	ase_awk_run_t* run, int out_type, const ase_char_t* name);

int ase_awk_closeextio_read (
	ase_awk_run_t* run, int in_type, const ase_char_t* name);
int ase_awk_closeextio_write (
	ase_awk_run_t* run, int out_type, const ase_char_t* name);
int ase_awk_closeextio (ase_awk_run_t* run, const ase_char_t* name);

void ase_awk_clearextio (ase_awk_run_t* run);

#ifdef __cplusplus
}
#endif

#endif
