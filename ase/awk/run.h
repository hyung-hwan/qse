/*
 * $Id: run.h,v 1.19 2006-10-22 11:34:53 bacon Exp $
 */

#ifndef _SSE_AWK_RUN_H_
#define _SSE_AWK_RUN_H_

#ifndef _SSE_AWK_AWK_H_
#error Never include this file directly. Include <sse/awk/awk.h> instead
#endif

enum
{
	/* if you change this, you have to change __assop_str in tree.c */
	SSE_AWK_ASSOP_NONE,
	SSE_AWK_ASSOP_PLUS,
	SSE_AWK_ASSOP_MINUS,
	SSE_AWK_ASSOP_MUL,
	SSE_AWK_ASSOP_DIV,
	SSE_AWK_ASSOP_MOD,
	SSE_AWK_ASSOP_ESSE
};

enum 
{
	/* if you change this, you have to change 
	 * __binop_str in tree.c and __binop_func in run.c accordingly. */ 
	SSE_AWK_BINOP_LOR,
	SSE_AWK_BINOP_LAND,
	SSE_AWK_BINOP_IN,

	SSE_AWK_BINOP_BOR,
	SSE_AWK_BINOP_BXOR,
	SSE_AWK_BINOP_BAND,

	SSE_AWK_BINOP_EQ,
	SSE_AWK_BINOP_NE,
	SSE_AWK_BINOP_GT,
	SSE_AWK_BINOP_GE,
	SSE_AWK_BINOP_LT,
	SSE_AWK_BINOP_LE,

	SSE_AWK_BINOP_LSHIFT,
	SSE_AWK_BINOP_RSHIFT,

	SSE_AWK_BINOP_PLUS,
	SSE_AWK_BINOP_MINUS,
	SSE_AWK_BINOP_MUL,
	SSE_AWK_BINOP_DIV,
	SSE_AWK_BINOP_MOD,
	SSE_AWK_BINOP_ESSE,

	SSE_AWK_BINOP_CONCAT,
	SSE_AWK_BINOP_MA,
	SSE_AWK_BINOP_NM
};

enum
{
	/* if you change this, you have to change 
	 * __unrop_str in tree.c accordingly. */ 
	SSE_AWK_UNROP_PLUS,
	SSE_AWK_UNROP_MINUS,
	SSE_AWK_UNROP_NOT,
	SSE_AWK_UNROP_BNOT
};

enum
{
	/* if you change this, you have to change 
	 * __incop_str in tree.c accordingly. */ 
	SSE_AWK_INCOP_PLUS,
	SSE_AWK_INCOP_MINUS
};

enum
{
	/* this table should match __bvtab in parse.c.
	 * in addition, sse_awk_setglobal also counts 
	 * on the order of these values */

	SSE_AWK_GLOBAL_ARGC,
	SSE_AWK_GLOBAL_ARGV,
	SSE_AWK_GLOBAL_CONVFMT,
	SSE_AWK_GLOBAL_ENVIRON,
	SSE_AWK_GLOBAL_ERRNO,
	SSE_AWK_GLOBAL_FILENAME,
	SSE_AWK_GLOBAL_FNR,
	SSE_AWK_GLOBAL_FS,
	SSE_AWK_GLOBAL_IGNORECASE,
	SSE_AWK_GLOBAL_NF,
	SSE_AWK_GLOBAL_NR,
	SSE_AWK_GLOBAL_OFMT,
	SSE_AWK_GLOBAL_OFS,
	SSE_AWK_GLOBAL_ORS,
	SSE_AWK_GLOBAL_RS,
	SSE_AWK_GLOBAL_RT,
	SSE_AWK_GLOBAL_RSTART,
	SSE_AWK_GLOBAL_RLENGTH,
	SSE_AWK_GLOBAL_SUBSEP
};

#endif
