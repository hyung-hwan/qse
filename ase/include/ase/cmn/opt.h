/*
 * $Id: getopt.h 290 2008-07-27 06:16:54Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_CMN_OPT_H_
#define _ASE_CMN_OPT_H_

#include <ase/types.h>
#include <ase/macros.h>

typedef struct ase_opt_t ase_opt_t;
typedef struct ase_opt_lng_t ase_opt_lng_t;

struct ase_opt_lng_t
{
	const ase_char_t* str;
	ase_cint_t val;
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
	const ase_char_t* lngopt; 

	/* input + output */
	int ind;         /* index into parent argv vector */

	/* input + output - internal*/
	ase_char_t* cur;
};

#ifdef __cplusplus
extern "C" {
#endif

/****f* ase.cmn.opt/ase_getopt
 * NAME
 *  ase_getopt - parse command line options
 *
 * SYNOPSIS
 */
ase_cint_t ase_getopt (
	int                argc  /* argument count */, 
	ase_char_t* const* argv  /* argument array */,
	ase_opt_t*         opt   /* option configuration */
);
/******/

#ifdef __cplusplus
}
#endif

#endif
