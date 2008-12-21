/*
 * $Id: func.h 363 2008-09-04 10:58:08Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_LIB_AWK_FUNC_H_
#define _ASE_LIB_AWK_FUNC_H_

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

	int (*handler) (ase_awk_run_t*, const ase_char_t*, ase_size_t);

	/*ase_awk_bfn_t* next;*/
};

#ifdef __cplusplus
extern "C" {
#endif

ase_awk_bfn_t* ase_awk_getbfn (
	ase_awk_t* awk, const ase_char_t* name, ase_size_t len);

#ifdef __cplusplus
}
#endif

#endif
