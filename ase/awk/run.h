/*
 * $Id: run.h,v 1.1 2006-03-07 16:02:58 bacon Exp $
 */

#ifndef _XP_AWK_RUN_H_
#define _XP_AWK_RUN_H_

#ifndef _XP_AWK_AWK_H_
#error Never include this file directly. Include <xp/awk/awk.h> instead
#endif

typedef struct xp_awk_frm_t xp_awk_frm_t;

struct xp_awk_frm_t
{
	xp_size_t nparams;
	xp_size_t nlocals;
	xp_awk_val_t* stack;
	xp_awk_frm_t* prev;
};

enum 
{
	XP_AWK_BINOP_PLUS,
	XP_AWK_BINOP_MINUS,
	XP_AWK_BINOP_MUL,
	XP_AWK_BINOP_DIV,
	XP_AWK_BINOP_MOD,
	XP_AWK_BINOP_RSHIFT,
	XP_AWK_BINOP_LSHIFT,
	XP_AWK_BINOP_EQ,
	XP_AWK_BINOP_NE,
	XP_AWK_BINOP_GT,
	XP_AWK_BINOP_GE,
	XP_AWK_BINOP_LT,
	XP_AWK_BINOP_LE
};

#endif
