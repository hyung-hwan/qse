/*
 * $Id: getopt.h 289 2008-07-26 15:37:38Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_UTL_GETOPT_H_
#define _ASE_UTL_GETOPT_H_

#include <ase/types.h>
#include <ase/macros.h>

enum
{
	ASE_OPT_ARG_NONE = 0,
	ASE_OPT_ARG_REQUIRED = 1,
	ASE_OPT_ARG_OPTIONAL = 2
};

typedef struct ase_opt_t ase_opt_t;
typedef struct ase_opt_lng_t ase_opt_lng_t;

struct ase_opt_lng_t
{
	const ase_char_t* str;
	int has_arg;
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

	/* output */
	ase_char_t* lngopt; 

	/* input + output */
	int ind;         /* index into parent argv vector */

	/* input + output - internal*/
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
