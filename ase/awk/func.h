/*
 * $Id: func.h,v 1.16 2006-11-27 15:10:34 bacon Exp $
 */

#ifndef _ASE_AWK_FUNC_H_
#define _ASE_AWK_FUNC_H_

#ifndef _ASE_AWK_AWK_H_
#error Never include this file directly. Include <ase/awk/awk.h> instead
#endif

typedef struct ase_awk_bfn_t ase_awk_bfn_t;

struct ase_awk_bfn_t
{
	struct
	{
		ase_char_t* ptr;
		ase_size_t  len;
	} name;

	int valid; /* the entry is valid when this option is set */

	struct
	{
		ase_size_t min;
		ase_size_t max;
		ase_char_t* spec;
	} arg;
	/*
	ase_size_t min_args;
	ase_size_t max_args;
	ase_char_t* arg_spec;
	*/

	int (*handler) (ase_awk_run_t* run);

	ase_awk_bfn_t* next;
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
