/*
 * $Id: getopt.h 287 2008-07-24 14:08:37Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_UTL_GETOPT_H_
#define _ASE_UTL_GETOPT_H_

#include <ase/types.h>
#include <ase/macros.h>

#define ASE_OPT_NONE 0
#define ASE_OPT_REQUIRED 1
#define ASE_OPT_OPTIONAL 2

typedef struct ase_opt_t ase_opt_t;
typedef struct ase_opt_lng_t ase_opt_lng_t;

struct ase_opt_lng_t
{
	const ase_char_t* str;
	int has_arg;
	int* flag;
	int val;
};

struct ase_opt_t
{
	/* input */
	const ase_char_t* str; /* option string  */
	ase_opt_lng_t* lng;    /* long options */

	/* output */
	ase_cint_t opt;  /* character checked for validity */
	ase_char_t* arg; /* argument associated with an option */
	int err;

	/* input + output */
	int ind;         /* index into parent argv vector */

	/* output */
	int lngind;

	/* internal */
	ase_char_t* cur;
};

#ifdef __cplusplus
extern "C" {
#endif

ase_cint_t ase_getopt (int argc, ase_char_t* const* argv, ase_opt_t* opt);

#ifdef __cplusplus
}
#endif

#endif
