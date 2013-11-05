/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _QSE_LIB_AWK_TREE_H_
#define _QSE_LIB_AWK_TREE_H_

enum qse_awk_in_type_t
{
	/* the order of these values match 
	 * __in_type_map and __in_opt_map in rio.c */

	QSE_AWK_IN_PIPE,
	QSE_AWK_IN_RWPIPE,
	QSE_AWK_IN_FILE,
	QSE_AWK_IN_CONSOLE
};

enum qse_awk_out_type_t
{
	/* the order of these values match 
	 * __out_type_map and __out_opt_map in rio.c */

	QSE_AWK_OUT_PIPE,
	QSE_AWK_OUT_RWPIPE, /* dual direction pipe */
	QSE_AWK_OUT_FILE,
	QSE_AWK_OUT_APFILE, /* file for appending */
	QSE_AWK_OUT_CONSOLE
};

typedef struct qse_awk_nde_blk_t       qse_awk_nde_blk_t;
typedef struct qse_awk_nde_grp_t       qse_awk_nde_grp_t;
typedef struct qse_awk_nde_ass_t       qse_awk_nde_ass_t;
typedef struct qse_awk_nde_exp_t       qse_awk_nde_exp_t;
typedef struct qse_awk_nde_cnd_t       qse_awk_nde_cnd_t;
typedef struct qse_awk_nde_pos_t       qse_awk_nde_pos_t;

typedef struct qse_awk_nde_int_t       qse_awk_nde_int_t;
typedef struct qse_awk_nde_flt_t       qse_awk_nde_flt_t;

typedef struct qse_awk_nde_str_t       qse_awk_nde_str_t;
typedef struct qse_awk_nde_rex_t       qse_awk_nde_rex_t;
typedef struct qse_awk_nde_var_t       qse_awk_nde_var_t;
typedef struct qse_awk_nde_fncall_t    qse_awk_nde_fncall_t;
typedef struct qse_awk_nde_getline_t   qse_awk_nde_getline_t;

typedef struct qse_awk_nde_if_t        qse_awk_nde_if_t;
typedef struct qse_awk_nde_while_t     qse_awk_nde_while_t;
typedef struct qse_awk_nde_for_t       qse_awk_nde_for_t;
typedef struct qse_awk_nde_foreach_t   qse_awk_nde_foreach_t;
typedef struct qse_awk_nde_break_t     qse_awk_nde_break_t;
typedef struct qse_awk_nde_continue_t  qse_awk_nde_continue_t;
typedef struct qse_awk_nde_return_t    qse_awk_nde_return_t;
typedef struct qse_awk_nde_exit_t      qse_awk_nde_exit_t;
typedef struct qse_awk_nde_next_t      qse_awk_nde_next_t;
typedef struct qse_awk_nde_nextfile_t  qse_awk_nde_nextfile_t;
typedef struct qse_awk_nde_delete_t    qse_awk_nde_delete_t;
typedef struct qse_awk_nde_reset_t     qse_awk_nde_reset_t;
typedef struct qse_awk_nde_print_t     qse_awk_nde_print_t;

/* QSE_AWK_NDE_BLK - block statement including top-level blocks */
struct qse_awk_nde_blk_t
{
	QSE_AWK_NDE_HDR;
	qse_size_t nlcls; /* number of local variables */
	qse_awk_nde_t* body;
};

/* QSE_AWK_NDE_GRP - expression group */
struct qse_awk_nde_grp_t
{
	QSE_AWK_NDE_HDR;
	qse_awk_nde_t* body;
};

/* QSE_AWK_NDE_ASS - assignment */
struct qse_awk_nde_ass_t
{
	QSE_AWK_NDE_HDR;
	int opcode;
	qse_awk_nde_t* left;
	qse_awk_nde_t* right;
};

/* QSE_AWK_NDE_EXP_BIN, QSE_AWK_NDE_EXP_UNR, 
 * QSE_AWK_NDE_EXP_INCPRE, QSE_AW_NDE_EXP_INCPST */
struct qse_awk_nde_exp_t
{
	QSE_AWK_NDE_HDR;
	int opcode;
	qse_awk_nde_t* left;
	qse_awk_nde_t* right; /* QSE_NULL for UNR, INCPRE, INCPST */
};

/* QSE_AWK_NDE_CND */
struct qse_awk_nde_cnd_t
{
	QSE_AWK_NDE_HDR;
	qse_awk_nde_t* test;
	qse_awk_nde_t* left;
	qse_awk_nde_t* right;
};

/* QSE_AWK_NDE_POS - positional - $1, $2, $x, etc */
struct qse_awk_nde_pos_t  
{
	QSE_AWK_NDE_HDR;
	qse_awk_nde_t* val;	
};

/* QSE_AWK_NDE_INT */
struct qse_awk_nde_int_t
{
	QSE_AWK_NDE_HDR;
	qse_awk_int_t val;
	qse_char_t*   str; 
	qse_size_t    len;
};

/* QSE_AWK_NDE_FLT */
struct qse_awk_nde_flt_t
{
	QSE_AWK_NDE_HDR;
	qse_awk_flt_t val;
	qse_char_t*   str;
	qse_size_t    len;
};

/* QSE_AWK_NDE_STR */
struct qse_awk_nde_str_t
{
	QSE_AWK_NDE_HDR;
	qse_char_t* ptr;
	qse_size_t  len;
};

/* QSE_AWK_NDE_REX */
struct qse_awk_nde_rex_t
{
	QSE_AWK_NDE_HDR;
	qse_xstr_t  str;
	void*       code[2]; /* [0]: case sensitive, [1]: case insensitive */
};

/* QSE_AWK_NDE_NAMED, QSE_AWK_NDE_GBL, 
 * QSE_AWK_NDE_LCL, QSE_AWK_NDE_ARG 
 * QSE_AWK_NDE_NAMEDIDX, QSE_AWK_NDE_GBLIDX, 
 * QSE_AWK_NDE_LCLIDX, QSE_AWK_NDE_ARGIDX */
struct qse_awk_nde_var_t
{
	QSE_AWK_NDE_HDR;
	struct 
	{
		qse_xstr_t name;
		qse_size_t idxa;
	} id;
	qse_awk_nde_t* idx; /* QSE_NULL for non-XXXXIDX */
};

/* QSE_AWK_NDE_FNC, QSE_AWK_NDE_FUN */
struct qse_awk_nde_fncall_t
{
	QSE_AWK_NDE_HDR;
	union
	{
		struct
		{
			qse_xstr_t name;
		} fun;

		/* minimum information of a intrinsic function 
		 * needed during run-time. */
		struct
		{
			qse_awk_fnc_info_t info;
			qse_awk_fnc_spec_t spec;
		} fnc;
	} u;
	qse_awk_nde_t* args;
	qse_size_t nargs;
};

/* QSE_AWK_NDE_GETLINE */
struct qse_awk_nde_getline_t
{
	QSE_AWK_NDE_HDR;
	qse_awk_nde_t* var;
	int in_type; /* QSE_AWK_GETLINE_XXX */
	qse_awk_nde_t* in;
};

/* QSE_AWK_NDE_IF */
struct qse_awk_nde_if_t
{
	QSE_AWK_NDE_HDR;
	qse_awk_nde_t* test;
	qse_awk_nde_t* then_part;
	qse_awk_nde_t* else_part; /* optional */
};

/* QSE_AWK_NDE_WHILE, QSE_AWK_NDE_DOWHILE */
struct qse_awk_nde_while_t
{
	QSE_AWK_NDE_HDR; 
	qse_awk_nde_t* test;
	qse_awk_nde_t* body;
};

/* QSE_AWK_NDE_FOR */
struct qse_awk_nde_for_t
{
	QSE_AWK_NDE_HDR;
	qse_awk_nde_t* init; /* optional */
	qse_awk_nde_t* test; /* optional */
	qse_awk_nde_t* incr; /* optional */
	qse_awk_nde_t* body;
};

/* QSE_AWK_NDE_FOREACH */
struct qse_awk_nde_foreach_t
{
	QSE_AWK_NDE_HDR;
	qse_awk_nde_t* test;
	qse_awk_nde_t* body;
};

/* QSE_AWK_NDE_BREAK */
struct qse_awk_nde_break_t
{
	QSE_AWK_NDE_HDR;
};

/* QSE_AWK_NDE_CONTINUE */
struct qse_awk_nde_continue_t
{
	QSE_AWK_NDE_HDR;
};

/* QSE_AWK_NDE_RETURN */
struct qse_awk_nde_return_t
{
	QSE_AWK_NDE_HDR;
	qse_awk_nde_t* val; /* optional (no return code if QSE_NULL) */	
};

/* QSE_AWK_NDE_EXIT */
struct qse_awk_nde_exit_t
{
	QSE_AWK_NDE_HDR;
	qse_awk_nde_t* val; /* optional (no exit code if QSE_NULL) */
	int abort;
};

/* QSE_AWK_NDE_NEXT */
struct qse_awk_nde_next_t
{
	QSE_AWK_NDE_HDR;
};

/* QSE_AWK_NDE_NEXTFILE */
struct qse_awk_nde_nextfile_t
{
	QSE_AWK_NDE_HDR;
	int out;
};

/* QSE_AWK_NDE_DELETE */
struct qse_awk_nde_delete_t
{
	QSE_AWK_NDE_HDR;
	qse_awk_nde_t* var;
};

/* QSE_AWK_NDE_RESET */
struct qse_awk_nde_reset_t
{
	QSE_AWK_NDE_HDR;
	qse_awk_nde_t* var;
};

/* QSE_AWK_NDE_PRINT */
struct qse_awk_nde_print_t
{
	QSE_AWK_NDE_HDR;
	qse_awk_nde_t* args;
	int out_type; /* QSE_AWK_OUT_XXX */
	qse_awk_nde_t* out;
};

#ifdef __cplusplus
extern "C" {
#endif

/* print the entire tree */
int qse_awk_prnpt (qse_awk_t* awk, qse_awk_nde_t* tree);
/* print a single top-level node */
int qse_awk_prnnde (qse_awk_t* awk, qse_awk_nde_t* node); 
/* print the pattern part */
int qse_awk_prnptnpt (qse_awk_t* awk, qse_awk_nde_t* tree);

void qse_awk_clrpt (qse_awk_t* awk, qse_awk_nde_t* tree);

#ifdef __cplusplus
}
#endif

#endif
