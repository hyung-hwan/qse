/*
 * $Id: run.h,v 1.23 2006-11-14 14:54:18 bacon Exp $
 */

#ifndef _ASE_AWK_RUN_H_
#define _ASE_AWK_RUN_H_

#ifndef _ASE_AWK_AWK_H_
#error Never include this file directly. Include <ase/awk/awk.h> instead
#endif

enum
{
	/* if you change this, you have to change __assop_str in tree.c */
	ASE_AWK_ASSOP_NONE,
	ASE_AWK_ASSOP_PLUS,
	ASE_AWK_ASSOP_MINUS,
	ASE_AWK_ASSOP_MUL,
	ASE_AWK_ASSOP_DIV,
	ASE_AWK_ASSOP_MOD,
	ASE_AWK_ASSOP_EXP
};

enum 
{
	/* if you change this, you have to change 
	 * __binop_str in tree.c and __binop_func in run.c accordingly. */ 
	ASE_AWK_BINOP_LOR,
	ASE_AWK_BINOP_LAND,
	ASE_AWK_BINOP_IN,

	ASE_AWK_BINOP_BOR,
	ASE_AWK_BINOP_BXOR,
	ASE_AWK_BINOP_BAND,

	ASE_AWK_BINOP_EQ,
	ASE_AWK_BINOP_NE,
	ASE_AWK_BINOP_GT,
	ASE_AWK_BINOP_GE,
	ASE_AWK_BINOP_LT,
	ASE_AWK_BINOP_LE,

	ASE_AWK_BINOP_LSHIFT,
	ASE_AWK_BINOP_RSHIFT,

	ASE_AWK_BINOP_PLUS,
	ASE_AWK_BINOP_MINUS,
	ASE_AWK_BINOP_MUL,
	ASE_AWK_BINOP_DIV,
	ASE_AWK_BINOP_MOD,
	ASE_AWK_BINOP_EXP,

	ASE_AWK_BINOP_CONCAT,
	ASE_AWK_BINOP_MA,
	ASE_AWK_BINOP_NM
};

enum
{
	/* if you change this, you have to change 
	 * __unrop_str in tree.c accordingly. */ 
	ASE_AWK_UNROP_PLUS,
	ASE_AWK_UNROP_MINUS,
	ASE_AWK_UNROP_NOT,
	ASE_AWK_UNROP_BNOT
};

enum
{
	/* if you change this, you have to change 
	 * __incop_str in tree.c accordingly. */ 
	ASE_AWK_INCOP_PLUS,
	ASE_AWK_INCOP_MINUS
};

enum
{
	/* this table should match __bvtab in parse.c.
	 * in addition, ase_awk_setglobal also counts 
	 * on the order of these values */

	ASE_AWK_GLOBAL_ARGC,
	ASE_AWK_GLOBAL_ARGV,
	ASE_AWK_GLOBAL_CONVFMT,
	ASE_AWK_GLOBAL_ENVIRON,
	ASE_AWK_GLOBAL_ERRNO,
	ASE_AWK_GLOBAL_FILENAME,
	ASE_AWK_GLOBAL_FNR,
	ASE_AWK_GLOBAL_FS,
	ASE_AWK_GLOBAL_IGNORECASE,
	ASE_AWK_GLOBAL_NF,
	ASE_AWK_GLOBAL_NR,
	ASE_AWK_GLOBAL_OFMT,
	ASE_AWK_GLOBAL_OFS,
	ASE_AWK_GLOBAL_ORS,
	ASE_AWK_GLOBAL_RS,
	ASE_AWK_GLOBAL_RT,
	ASE_AWK_GLOBAL_RSTART,
	ASE_AWK_GLOBAL_RLENGTH,
	ASE_AWK_GLOBAL_SUBSEP
};

#ifdef __cplusplus
extern "C" {
#endif

ase_char_t* ase_awk_sprintf (
	ase_awk_run_t* run, const ase_char_t* fmt, ase_size_t fmt_len, 
	ase_size_t nargs_on_stack, ase_awk_nde_t* args, ase_size_t* len);

#ifdef __cplusplus
}
#endif

#endif
