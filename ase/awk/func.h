/*
 * $Id: func.h,v 1.1 2006-06-16 14:31:42 bacon Exp $
 */

#ifndef _XP_AWK_FUNC_H_
#define _XP_AWK_FUNC_H_

#ifndef _XP_AWK_AWK_H_
#error Never include this file directly. Include <xp/awk/awk.h> instead
#endif

typedef struct xp_awk_bfn_t xp_awk_bfn_t;

struct xp_awk_bfn_t
{
	const xp_char_t* name; 

	int type;
	int valid; /* the entry is valid when this option is set */

	int max_args;
	int min_args;
	xp_awk_val_t* (*handler) ();
};

#ifdef __cplusplus
extern "C" {
#endif

xp_awk_bfn_t* xp_awk_getbfn (const xp_char_t* name);

#ifdef __cplusplus
}
#endif

#endif
