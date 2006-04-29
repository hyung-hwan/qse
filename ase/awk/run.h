/*
 * $Id: run.h,v 1.13 2006-04-29 12:09:29 bacon Exp $
 */

#ifndef _XP_AWK_RUN_H_
#define _XP_AWK_RUN_H_

#ifndef _XP_AWK_AWK_H_
#error Never include this file directly. Include <xp/awk/awk.h> instead
#endif

enum
{
	/* if you change this, you have to change __assop_str in tree.c */
	XP_AWK_ASSOP_NONE,
	XP_AWK_ASSOP_PLUS,
	XP_AWK_ASSOP_MINUS,
	XP_AWK_ASSOP_MUL,
	XP_AWK_ASSOP_DIV,
	XP_AWK_ASSOP_MOD,
	XP_AWK_ASSOP_EXP
};

enum 
{
	/* if you change this, you have to change 
	 * __binop_str in tree.c and __binop_func in run.c accordingly. */ 
	XP_AWK_BINOP_LOR,
	XP_AWK_BINOP_LAND,
	XP_AWK_BINOP_IN,

	XP_AWK_BINOP_BOR,
	XP_AWK_BINOP_BXOR,
	XP_AWK_BINOP_BAND,

	XP_AWK_BINOP_EQ,
	XP_AWK_BINOP_NE,
	XP_AWK_BINOP_GT,
	XP_AWK_BINOP_GE,
	XP_AWK_BINOP_LT,
	XP_AWK_BINOP_LE,

	XP_AWK_BINOP_LSHIFT,
	XP_AWK_BINOP_RSHIFT,

	XP_AWK_BINOP_PLUS,
	XP_AWK_BINOP_MINUS,
	XP_AWK_BINOP_MUL,
	XP_AWK_BINOP_DIV,
	XP_AWK_BINOP_MOD,
	XP_AWK_BINOP_EXP,

	XP_AWK_BINOP_MA,
	XP_AWK_BINOP_NM
};

enum
{
	/* if you change this, you have to change 
	 * __unrop_str in tree.c accordingly. */ 
	XP_AWK_UNROP_PLUS,
	XP_AWK_UNROP_MINUS,
	XP_AWK_UNROP_NOT,
	XP_AWK_UNROP_BNOT
};

enum
{
	/* if you change this, you have to change 
	 * __incop_str in tree.c accordingly. */ 
	XP_AWK_INCOP_PLUS,
	XP_AWK_INCOP_MINUS
};

#endif
