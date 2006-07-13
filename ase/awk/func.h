/*
 * $Id: func.h,v 1.4 2006-07-13 15:43:39 bacon Exp $
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
	int valid; /* the entry is valid when this option is set */

	xp_size_t min_args;
	xp_size_t max_args;
	int (*handler) (void* run);

	xp_awk_bfn_t* next;
};

#ifdef __cplusplus
extern "C" {
#endif

xp_awk_bfn_t* xp_awk_getbfn (xp_awk_t* awk, const xp_char_t* name);

#ifdef __cplusplus
}
#endif

#endif
