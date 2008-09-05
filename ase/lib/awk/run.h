/*
 * $Id: run.h 363 2008-09-04 10:58:08Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_LIB_AWK_RUN_H_
#define _ASE_LIB_AWK_RUN_H_

enum ase_awk_assop_type_t
{
	/* if you change this, you have to change assop_str in tree.c.
	 * synchronize it with binop_func of eval_assignment in run.c */
	ASE_AWK_ASSOP_NONE, 
	ASE_AWK_ASSOP_PLUS,   /* += */
	ASE_AWK_ASSOP_MINUS,  /* -= */
	ASE_AWK_ASSOP_MUL,    /* *= */
	ASE_AWK_ASSOP_DIV,    /* /= */
	ASE_AWK_ASSOP_IDIV,   /* //= */
	ASE_AWK_ASSOP_MOD,    /* %= */
	ASE_AWK_ASSOP_EXP,    /* **= */
	ASE_AWK_ASSOP_RSHIFT, /* >>= */
	ASE_AWK_ASSOP_LSHIFT, /* <<= */
	ASE_AWK_ASSOP_BAND,   /* &= */
	ASE_AWK_ASSOP_BXOR,   /* ^= */
	ASE_AWK_ASSOP_BOR     /* |= */
};

enum  ase_awk_binop_type_t
{
	/* if you change this, you have to change 
	 * binop_str in tree.c and binop_func in run.c accordingly. */ 
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
	ASE_AWK_BINOP_IDIV,
	ASE_AWK_BINOP_MOD,
	ASE_AWK_BINOP_EXP,

	ASE_AWK_BINOP_CONCAT,
	ASE_AWK_BINOP_MA,
	ASE_AWK_BINOP_NM
};

enum ase_awk_unrop_type_t
{
	/* if you change this, you have to change 
	 * __unrop_str in tree.c accordingly. */ 
	ASE_AWK_UNROP_PLUS,
	ASE_AWK_UNROP_MINUS,
	ASE_AWK_UNROP_LNOT,
	ASE_AWK_UNROP_BNOT
};

enum ase_awk_incop_type_t
{
	/* if you change this, you have to change 
	 * __incop_str in tree.c accordingly. */ 
	ASE_AWK_INCOP_PLUS,
	ASE_AWK_INCOP_MINUS
};

#ifdef __cplusplus
extern "C" {
#endif

ase_char_t* ase_awk_format (
	ase_awk_run_t* run, ase_str_t* out, ase_str_t* fbu,
	const ase_char_t* fmt, ase_size_t fmt_len, 
	ase_size_t nargs_on_stack, ase_awk_nde_t* args, ase_size_t* len);

#ifdef __cplusplus
}
#endif

#endif
