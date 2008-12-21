/*
 * $Id: run.h 363 2008-09-04 10:58:08Z baconevi $
 *
 * {License}
 */

#ifndef _QSE_LIB_AWK_RUN_H_
#define _QSE_LIB_AWK_RUN_H_

enum qse_awk_assop_type_t
{
	/* if you change this, you have to change assop_str in tree.c.
	 * synchronize it with binop_func of eval_assignment in run.c */
	QSE_AWK_ASSOP_NONE, 
	QSE_AWK_ASSOP_PLUS,   /* += */
	QSE_AWK_ASSOP_MINUS,  /* -= */
	QSE_AWK_ASSOP_MUL,    /* *= */
	QSE_AWK_ASSOP_DIV,    /* /= */
	QSE_AWK_ASSOP_IDIV,   /* //= */
	QSE_AWK_ASSOP_MOD,    /* %= */
	QSE_AWK_ASSOP_EXP,    /* **= */
	QSE_AWK_ASSOP_RSHIFT, /* >>= */
	QSE_AWK_ASSOP_LSHIFT, /* <<= */
	QSE_AWK_ASSOP_BAND,   /* &= */
	QSE_AWK_ASSOP_BXOR,   /* ^= */
	QSE_AWK_ASSOP_BOR     /* |= */
};

enum  qse_awk_binop_type_t
{
	/* if you change this, you have to change 
	 * binop_str in tree.c and binop_func in run.c accordingly. */ 
	QSE_AWK_BINOP_LOR,
	QSE_AWK_BINOP_LAND,
	QSE_AWK_BINOP_IN,

	QSE_AWK_BINOP_BOR,
	QSE_AWK_BINOP_BXOR,
	QSE_AWK_BINOP_BAND,

	QSE_AWK_BINOP_EQ,
	QSE_AWK_BINOP_NE,
	QSE_AWK_BINOP_GT,
	QSE_AWK_BINOP_GE,
	QSE_AWK_BINOP_LT,
	QSE_AWK_BINOP_LE,

	QSE_AWK_BINOP_LSHIFT,
	QSE_AWK_BINOP_RSHIFT,

	QSE_AWK_BINOP_PLUS,
	QSE_AWK_BINOP_MINUS,
	QSE_AWK_BINOP_MUL,
	QSE_AWK_BINOP_DIV,
	QSE_AWK_BINOP_IDIV,
	QSE_AWK_BINOP_MOD,
	QSE_AWK_BINOP_EXP,

	QSE_AWK_BINOP_CONCAT,
	QSE_AWK_BINOP_MA,
	QSE_AWK_BINOP_NM
};

enum qse_awk_unrop_type_t
{
	/* if you change this, you have to change 
	 * __unrop_str in tree.c accordingly. */ 
	QSE_AWK_UNROP_PLUS,
	QSE_AWK_UNROP_MINUS,
	QSE_AWK_UNROP_LNOT,
	QSE_AWK_UNROP_BNOT
};

enum qse_awk_incop_type_t
{
	/* if you change this, you have to change 
	 * __incop_str in tree.c accordingly. */ 
	QSE_AWK_INCOP_PLUS,
	QSE_AWK_INCOP_MINUS
};

#ifdef __cplusplus
extern "C" {
#endif

qse_char_t* qse_awk_format (
	qse_awk_run_t* run, qse_str_t* out, qse_str_t* fbu,
	const qse_char_t* fmt, qse_size_t fmt_len, 
	qse_size_t nargs_on_stack, qse_awk_nde_t* args, qse_size_t* len);

#ifdef __cplusplus
}
#endif

#endif
