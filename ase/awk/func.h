/*
 * $Id: func.h,v 1.12 2006-10-10 07:02:38 bacon Exp $
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
	xp_size_t name_len;
	int valid; /* the entry is valid when this option is set */

	xp_size_t min_args;
	xp_size_t max_args;
	const xp_char_t* arg_spec;
	int (*handler) (xp_awk_run_t* run);

	xp_awk_bfn_t* next;
};

enum
{
	/* ensure that this matches __sys_bfn in func.c */

	XP_AWK_BFN_CLOSE,
	XP_AWK_BFN_INDEX,
	XP_AWK_BFN_LENGTH,
	XP_AWK_BFN_SYSTEM,
	XP_AWK_BFN_SIN
};


#ifdef __cplusplus
extern "C" {
#endif

xp_awk_bfn_t* xp_awk_addbfn (
	xp_awk_t* awk, const xp_char_t* name, xp_size_t name_len, 
	int when_valid, xp_size_t min_args, xp_size_t max_args, 
	const xp_char_t* arg_spec, int (*handler)(xp_awk_run_t*));

int xp_awk_delbfn (xp_awk_t* awk, const xp_char_t* name, xp_size_t name_len);

void xp_awk_clrbfn (xp_awk_t* awk);

xp_awk_bfn_t* xp_awk_getbfn (
	xp_awk_t* awk, const xp_char_t* name, xp_size_t name_len);

#ifdef __cplusplus
}
#endif

#endif
