/*
 * $Id: func.h,v 1.14 2006-10-24 04:10:12 bacon Exp $
 */

#ifndef _ASE_AWK_FUNC_H_
#define _ASE_AWK_FUNC_H_

#ifndef _ASE_AWK_AWK_H_
#error Never include this file directly. Include <ase/awk/awk.h> instead
#endif

typedef struct ase_awk_bfn_t ase_awk_bfn_t;

struct ase_awk_bfn_t
{
	const ase_char_t* name; 
	ase_size_t name_len;
	int valid; /* the entry is valid when this option is set */

	ase_size_t min_args;
	ase_size_t max_args;
	const ase_char_t* arg_spec;
	int (*handler) (ase_awk_run_t* run);

	ase_awk_bfn_t* next;
};

enum
{
	/* ensure that this matches __sys_bfn in func.c */

	ASE_AWK_BFN_CLOSE,
	ASE_AWK_BFN_INDEX,
	ASE_AWK_BFN_LENGTH,
	ASE_AWK_BFN_SYSTEM,
	ASE_AWK_BFN_SIN
};


#ifdef __cplusplus
extern "C" {
#endif

ase_awk_bfn_t* ase_awk_addbfn (
	ase_awk_t* awk, const ase_char_t* name, ase_size_t name_len, 
	int when_valid, ase_size_t min_args, ase_size_t max_args, 
	const ase_char_t* arg_spec, int (*handler)(ase_awk_run_t*));

int ase_awk_delbfn (ase_awk_t* awk, const ase_char_t* name, ase_size_t name_len);

void ase_awk_clrbfn (ase_awk_t* awk);

ase_awk_bfn_t* ase_awk_getbfn (
	ase_awk_t* awk, const ase_char_t* name, ase_size_t name_len);

#ifdef __cplusplus
}
#endif

#endif
