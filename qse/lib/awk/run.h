/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _QSE_LIB_AWK_RUN_H_
#define _QSE_LIB_AWK_RUN_H_

enum qse_awk_assop_type_t
{
	/* if you change this, you have to change assop_str in tree.c.
	 * synchronize it wit: 
	 *   - binop_func in eval_assignment of run.c 
	 *   - assop in assing_to_opcode of parse.c
	 *   - TOK_XXX_ASSN in tok_t in parse.c
	 *   - assop_str in tree.c
	 */
	QSE_AWK_ASSOP_NONE, 
	QSE_AWK_ASSOP_PLUS,   /* += */
	QSE_AWK_ASSOP_MINUS,  /* -= */
	QSE_AWK_ASSOP_MUL,    /* *= */
	QSE_AWK_ASSOP_DIV,    /* /= */
	QSE_AWK_ASSOP_IDIV,   /* //= */
	QSE_AWK_ASSOP_MOD,    /* %= */
	QSE_AWK_ASSOP_EXP,    /* **= */
	QSE_AWK_ASSOP_CONCAT, /* %%= */
	QSE_AWK_ASSOP_RS,     /* >>= */
	QSE_AWK_ASSOP_LS,     /* <<= */
	QSE_AWK_ASSOP_BAND,   /* &= */
	QSE_AWK_ASSOP_BXOR,   /* ^^= */
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

	QSE_AWK_BINOP_TEQ,
	QSE_AWK_BINOP_TNE,
	QSE_AWK_BINOP_EQ,
	QSE_AWK_BINOP_NE,
	QSE_AWK_BINOP_GT,
	QSE_AWK_BINOP_GE,
	QSE_AWK_BINOP_LT,
	QSE_AWK_BINOP_LE,

	QSE_AWK_BINOP_LS,
	QSE_AWK_BINOP_RS,

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
	QSE_AWK_UNROP_BNOT,
	QSE_AWK_UNROP_DEF  /* `, is defined */
};

enum qse_awk_incop_type_t
{
	/* if you change this, you have to change 
	 * __incop_str in tree.c accordingly. */ 
	QSE_AWK_INCOP_PLUS,
	QSE_AWK_INCOP_MINUS
};

#if defined(__cplusplus)
extern "C" {
#endif

qse_char_t* qse_awk_rtx_format (
	qse_awk_rtx_t*    run, 
	qse_str_t*        out, 
	qse_str_t*        fbu,
	const qse_char_t* fmt, 
	qse_size_t        fmt_len, 
	qse_size_t        nargs_on_stack, 
	qse_awk_nde_t*    args, 
	qse_size_t*       len
);

#if defined(__cplusplus)
}
#endif

#endif
