/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
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

#include "awk.h"
#include <qse/cmn/fmt.h>

#ifdef DEBUG_RUN
#include <qse/cmn/sio.h>
#endif

#define PRINT_IOERR -99

#define CMP_ERROR -99
#define DEF_BUF_CAPA 256
#define RTX_STACK_INCREMENT 512

#define IDXBUFSIZE 64

#define MMGR(rtx) ((rtx)->awk->mmgr)

enum exit_level_t
{
	EXIT_NONE,
	EXIT_BREAK,
	EXIT_CONTINUE,
	EXIT_FUNCTION,
	EXIT_NEXT,
	EXIT_GLOBAL,
	EXIT_ABORT
};

struct pafv
{
	qse_awk_val_t** args;
	qse_size_t      nargs;
};

#define DEFAULT_CONVFMT  QSE_T("%.6g")
#define DEFAULT_OFMT     QSE_T("%.6g")
#define DEFAULT_OFS      QSE_T(" ")
#define DEFAULT_ORS      QSE_T("\n")
#define DEFAULT_ORS_CRLF QSE_T("\r\n")
#define DEFAULT_SUBSEP   QSE_T("\034")

/* the index of a positional variable should be a positive interger
 * equal to or less than the maximum value of the type by which
 * the index is represented. but it has an extra check against the
 * maximum value of qse_size_t as the reference is represented
 * in a pointer variable of qse_awk_val_ref_t and sizeof(void*) is
 * equal to sizeof(qse_size_t). */
#define IS_VALID_POSIDX(idx) \
	((idx) >= 0 && \
	 (idx) <= QSE_TYPE_MAX(qse_awk_int_t) && \
	 (idx) <= QSE_TYPE_MAX(qse_size_t))

#define SETERR_ARGX_LOC(rtx,code,ea,loc) \
	qse_awk_rtx_seterror ((rtx), (code), (ea), (loc))

#define CLRERR(rtx) SETERR_ARGX_LOC(rtx,QSE_AWK_ENOERR,QSE_NULL,QSE_NULL)

#define SETERR_ARG_LOC(rtx,code,ep,el,loc) \
	do { \
		qse_cstr_t __ea; \
		__ea.len = (el); __ea.ptr = (ep); \
		qse_awk_rtx_seterror ((rtx), (code), &__ea, (loc)); \
	} while (0)

#define SETERR_ARGX(rtx,code,ea) SETERR_ARGX_LOC(rtx,code,ea,QSE_NULL)
#define SETERR_ARG(rtx,code,ep,el) SETERR_ARG_LOC(rtx,code,ep,el,QSE_NULL)
#define SETERR_LOC(rtx,code,loc) SETERR_ARGX_LOC(rtx,code,QSE_NULL,loc)
#define SETERR_COD(rtx,code) SETERR_ARGX_LOC(rtx,code,QSE_NULL,QSE_NULL)

#define ADJERR_LOC(rtx,l) do { (rtx)->errinf.loc = *(l); } while (0)

static qse_size_t push_arg_from_vals (
	qse_awk_rtx_t* rtx, qse_awk_nde_fncall_t* call, void* data);

static int init_rtx (qse_awk_rtx_t* rtx, qse_awk_t* awk, qse_awk_rio_t* rio);
static void fini_rtx (qse_awk_rtx_t* rtx, int fini_globals);

static int init_globals (qse_awk_rtx_t* rtx);
static void refdown_globals (qse_awk_rtx_t* run, int pop);

static int run_pblocks  (qse_awk_rtx_t* rtx);
static int run_pblock_chain (qse_awk_rtx_t* rtx, qse_awk_chain_t* cha);
static int run_pblock (qse_awk_rtx_t* rtx, qse_awk_chain_t* cha, qse_size_t bno);
static int run_block (qse_awk_rtx_t* rtx, qse_awk_nde_blk_t* nde);
static int run_block0 (qse_awk_rtx_t* rtx, qse_awk_nde_blk_t* nde);
static int run_statement (qse_awk_rtx_t* rtx, qse_awk_nde_t* nde);
static int run_if (qse_awk_rtx_t* rtx, qse_awk_nde_if_t* nde);
static int run_while (qse_awk_rtx_t* rtx, qse_awk_nde_while_t* nde);
static int run_for (qse_awk_rtx_t* rtx, qse_awk_nde_for_t* nde);
static int run_foreach (qse_awk_rtx_t* rtx, qse_awk_nde_foreach_t* nde);
static int run_break (qse_awk_rtx_t* rtx, qse_awk_nde_break_t* nde);
static int run_continue (qse_awk_rtx_t* rtx, qse_awk_nde_continue_t* nde);
static int run_return (qse_awk_rtx_t* rtx, qse_awk_nde_return_t* nde);
static int run_exit (qse_awk_rtx_t* rtx, qse_awk_nde_exit_t* nde);
static int run_next (qse_awk_rtx_t* rtx, qse_awk_nde_next_t* nde);
static int run_nextfile (qse_awk_rtx_t* rtx, qse_awk_nde_nextfile_t* nde);
static int run_delete (qse_awk_rtx_t* rtx, qse_awk_nde_delete_t* nde);
static int run_reset (qse_awk_rtx_t* rtx, qse_awk_nde_reset_t* nde);
static int run_print (qse_awk_rtx_t* rtx, qse_awk_nde_print_t* nde);
static int run_printf (qse_awk_rtx_t* rtx, qse_awk_nde_print_t* nde);

static int output_formatted (
	qse_awk_rtx_t* run, int out_type, const qse_char_t* dst, 
	const qse_char_t* fmt, qse_size_t fmt_len, qse_awk_nde_t* args);

static qse_awk_val_t* eval_expression (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_expression0 (qse_awk_rtx_t* run, qse_awk_nde_t* nde);

static qse_awk_val_t* eval_group (qse_awk_rtx_t* run, qse_awk_nde_t* nde);

static qse_awk_val_t* eval_assignment (
	qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* do_assignment (
	qse_awk_rtx_t* run, qse_awk_nde_t* var, qse_awk_val_t* val);
static qse_awk_val_t* do_assignment_nonidx (
	qse_awk_rtx_t* run, qse_awk_nde_var_t* var, qse_awk_val_t* val);
static qse_awk_val_t* do_assignment_idx (
	qse_awk_rtx_t* run, qse_awk_nde_var_t* var, qse_awk_val_t* val);
static qse_awk_val_t* do_assignment_pos (
	qse_awk_rtx_t* run, qse_awk_nde_pos_t* pos, qse_awk_val_t* val);

static qse_awk_val_t* eval_binary (
	qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_binop_lor (
	qse_awk_rtx_t* run, qse_awk_nde_t* left, qse_awk_nde_t* right);
static qse_awk_val_t* eval_binop_land (
	qse_awk_rtx_t* run, qse_awk_nde_t* left, qse_awk_nde_t* right);
static qse_awk_val_t* eval_binop_in (
	qse_awk_rtx_t* run, qse_awk_nde_t* left, qse_awk_nde_t* right);
static qse_awk_val_t* eval_binop_bor (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_bxor (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_band (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);

static qse_awk_val_t* eval_binop_teq (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_tne (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_eq (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_ne (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_gt (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_ge (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_lt (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_le (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_lshift (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_rshift (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_plus (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_minus (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_mul (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_div (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_idiv (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_mod (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_exp (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_concat (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
static qse_awk_val_t* eval_binop_ma (
	qse_awk_rtx_t* run, qse_awk_nde_t* left, qse_awk_nde_t* right);
static qse_awk_val_t* eval_binop_nm (
	qse_awk_rtx_t* run, qse_awk_nde_t* left, qse_awk_nde_t* right);

static qse_awk_val_t* eval_unary (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_incpre (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_incpst (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_cnd (qse_awk_rtx_t* run, qse_awk_nde_t* nde);

static qse_awk_val_t* eval_fun_ex (
	qse_awk_rtx_t* run, qse_awk_nde_t* nde, 
	void(*errhandler)(void*), void* eharg);

static qse_awk_val_t* eval_fnc (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_fun (qse_awk_rtx_t* run, qse_awk_nde_t* nde);

static qse_awk_val_t* __eval_call (
	qse_awk_rtx_t* run,
	qse_awk_nde_t* nde, 
	const qse_char_t* fnc_arg_spec,
	qse_awk_fun_t* fun, 
	qse_size_t(*argpusher)(qse_awk_rtx_t*,qse_awk_nde_fncall_t*,void*),
	void* apdata,
	void(*errhandler)(void*),
	void* eharg);

static qse_awk_val_t* eval_call (
	qse_awk_rtx_t* run, qse_awk_nde_t* nde, 
	const qse_char_t* fnc_arg_spec, qse_awk_fun_t* fun,
	void(*errhandler)(void*), void* eharg);

static int get_reference (
	qse_awk_rtx_t* run, qse_awk_nde_t* nde, qse_awk_val_t*** ref);
static qse_awk_val_t** get_reference_indexed (
	qse_awk_rtx_t* run, qse_awk_nde_var_t* nde, qse_awk_val_t** val);

static qse_awk_val_t* eval_int (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_real (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_str (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_rex (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_named (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_gbl (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_lcl (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_arg (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_namedidx (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_gblidx (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_lclidx (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_argidx (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_pos (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_getline (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_print (qse_awk_rtx_t* run, qse_awk_nde_t* nde);
static qse_awk_val_t* eval_printf (qse_awk_rtx_t* run, qse_awk_nde_t* nde);

static int __raw_push (qse_awk_rtx_t* run, void* val);
#define __raw_pop(run) \
	do { \
		QSE_ASSERT ((run)->stack_top > (run)->stack_base); \
		(run)->stack_top--; \
	} while (0)

static int read_record (qse_awk_rtx_t* run);
static int shorten_record (qse_awk_rtx_t* run, qse_size_t nflds);

static qse_char_t* idxnde_to_str (
	qse_awk_rtx_t* run, qse_awk_nde_t* nde, qse_char_t* buf, qse_size_t* len);

typedef qse_awk_val_t* (*binop_func_t) (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right);
typedef qse_awk_val_t* (*eval_expr_t) (qse_awk_rtx_t* run, qse_awk_nde_t* nde);

#ifdef NDEBUG
#	define xstr_to_cstr(xstr) ((qse_cstr_t*)xstr)
#else
static QSE_INLINE qse_cstr_t* xstr_to_cstr (qse_xstr_t* xstr)
{
	/* i use this function to typecast qse_cstr_t* to 
	 * qse_xstr_t* instead of direct typecasting.
	 * it is just to let the compiler emit some warnings 
	 * if the data type of the actual parameter happened to
	 * haved changed to something else. */ 
	return (qse_cstr_t*)xstr;
}
#endif

QSE_INLINE qse_size_t qse_awk_rtx_getnargs (qse_awk_rtx_t* run)
{
	return (qse_size_t) RTX_STACK_NARGS (run);
}

QSE_INLINE qse_awk_val_t* qse_awk_rtx_getarg (qse_awk_rtx_t* run, qse_size_t idx)
{
	return RTX_STACK_ARG (run, idx);
}

QSE_INLINE qse_awk_val_t* qse_awk_rtx_getgbl (qse_awk_rtx_t* run, int id)
{
	QSE_ASSERT (id >= 0 && id < (int)QSE_LDA_SIZE(run->awk->parse.gbls));
	return RTX_STACK_GBL (run, id);
}

const qse_xstr_t* qse_awk_rtx_getsubsep (qse_awk_rtx_t* run)
{
	return &run->gbl.subsep;
}

/* internal function to set a value to a global variable.
 * this function can handle a few special global variables that
 * require special treatment. */
static int set_global (
	qse_awk_rtx_t* rtx, int idx,
	qse_awk_nde_var_t* var, qse_awk_val_t* val, int assign)
{
	qse_awk_val_t* old;
	qse_awk_rtx_ecb_t* ecb;
       
	old = RTX_STACK_GBL (rtx, idx);

	if (!(rtx->awk->opt.trait & QSE_AWK_FLEXMAP))
	{
		qse_awk_errnum_t errnum = QSE_AWK_ENOERR;

		if (val->type == QSE_AWK_VAL_MAP)
		{
			if (old->type == QSE_AWK_VAL_NIL)
			{
				/* a nil valul can be overridden with any values */
				/* ok. no error */
			}
			else if (!assign && old->type == QSE_AWK_VAL_MAP)
			{
				/* when both are maps, how should this operation be 
				 * interpreted?
				 *
				 * is it an assignment?
				 *    old = new
				 *
				 * or is it to delete all elements in the array
				 * and add new items?
				 *    for (i in old) delete old[i];
				 *    for (i in new) old[i] = new[i];
				 *
				 * i interpret this operation as the latter.
				 */

				/* ok. no error */
			}
			else
			{
				errnum = QSE_AWK_ENSCALARTOMAP;
			}
		}
		else
		{
			if (old->type == QSE_AWK_VAL_MAP) errnum = QSE_AWK_ENMAPTOSCALAR;
		}
	
		if (errnum != QSE_AWK_ENOERR)
		{
			/* once a variable becomes a map, it cannot be assigned 
			 * others value than another map. you can only add a member
			 * using indexing. */
			if (var)
			{
				/* global variable */
				SETERR_ARGX_LOC (rtx, errnum, xstr_to_cstr(&var->id.name), &var->loc);
			}
			else
			{
				/* qse_awk_rtx_setgbl() has been called */
				qse_cstr_t ea;
				ea.ptr = qse_awk_getgblname (rtx->awk, idx, &ea.len);
				SETERR_ARGX (rtx, errnum, &ea);
			}

			return -1;
		}
	}

	if (val->type == QSE_AWK_VAL_MAP)
	{
		if (idx >= QSE_AWK_MIN_GBL_ID && idx <= QSE_AWK_MAX_GBL_ID)
		{
			/* short-circuit check block to prevent the basic built-in 
			 * variables from being assigned a map. if you happen to add
			 * one and if that's allowed to be a map, you may have to 
			 * change the condition here. */

/* TODO: use global variable attribute. can it be a map? can it be a scalar? is it read-only???? */

			qse_cstr_t ea;
			ea.ptr = qse_awk_getgblname (rtx->awk, idx, &ea.len);
			SETERR_ARGX (rtx, QSE_AWK_ENSCALARTOMAP, &ea);
			return -1;
		}
	}

	if (old == val) 
	{
		/* if the old value is the same as the new value, don't take any actions.
		 * note that several inspections have been performed before this check,
		 * mainly for consistency. anyway, this condition can be met if you execute
		 * a statement like 'ARGV=ARGV'. */
		return 0; 
	}

	/* perform actual assignment or assignment-like operation */
	switch (idx)
	{
		case QSE_AWK_GBL_CONVFMT:
		{
			qse_size_t i;
			qse_awk_rtx_valtostr_out_t out;

			out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
			if (qse_awk_rtx_valtostr (rtx, val, &out) <= -1)
				return -1;

			for (i = 0; i < out.u.cpldup.len; i++)
			{
				if (out.u.cpldup.ptr[i] == QSE_T('\0'))
				{
					/* '\0' is included in the value */
					QSE_AWK_FREE (rtx->awk, out.u.cpldup.ptr);
					SETERR_COD (rtx, QSE_AWK_ECONVFMTCHR);
					return -1;
				}
			}

			if (rtx->gbl.convfmt.ptr != QSE_NULL)
				QSE_AWK_FREE (rtx->awk, rtx->gbl.convfmt.ptr);
			rtx->gbl.convfmt.ptr = out.u.cpldup.ptr;
			rtx->gbl.convfmt.len = out.u.cpldup.len;
			break;
		}

		case QSE_AWK_GBL_FNR:
		{
			int n;
			qse_awk_int_t lv;

			n = qse_awk_rtx_valtoint (rtx, val, &lv);
			if (n <= -1) return -1;

			rtx->gbl.fnr = lv;
			break;
		}
	
		case QSE_AWK_GBL_FS:
		{
			qse_char_t* fs_ptr;
			qse_size_t fs_len;

			if (val->type == QSE_AWK_VAL_STR)
			{
				fs_ptr = ((qse_awk_val_str_t*)val)->val.ptr;
				fs_len = ((qse_awk_val_str_t*)val)->val.len;
			}
			else
			{
				qse_awk_rtx_valtostr_out_t out;

				/* due to the expression evaluation rule, the 
				 * regular expression can not be an assigned value */
				QSE_ASSERT (val->type != QSE_AWK_VAL_REX);

				out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
				if (qse_awk_rtx_valtostr (rtx, val, &out) <= -1) return -1;
				fs_ptr = out.u.cpldup.ptr;
				fs_len = out.u.cpldup.len;
			}

			if (fs_len > 1 && !(fs_len == 5 && fs_ptr[0] == QSE_T('?')))
			{
				void* rex, * irex;
			
				qse_awk_errnum_t errnum;

				if (qse_awk_buildrex (rtx->awk, fs_ptr, fs_len, &errnum, &rex, &irex) <= -1)
				{
					SETERR_COD (rtx, errnum);
					if (val->type != QSE_AWK_VAL_STR) 
						QSE_AWK_FREE (rtx->awk, fs_ptr);
					return -1;
				}

				if (rtx->gbl.fs[0])
					qse_awk_freerex (rtx->awk, rtx->gbl.fs[0], rtx->gbl.fs[1]);
				
				rtx->gbl.fs[0] = rex;
				rtx->gbl.fs[1] = irex;
			}

			if (val->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (rtx->awk, fs_ptr);
			break;
		}

		case QSE_AWK_GBL_IGNORECASE:
		{
			rtx->gbl.ignorecase = 
				((val->type == QSE_AWK_VAL_INT &&
				  ((qse_awk_val_int_t*)val)->val != 0) ||
				 (val->type == QSE_AWK_VAL_FLT &&
				  ((qse_awk_val_flt_t*)val)->val != 0.0) ||
				 (val->type == QSE_AWK_VAL_STR &&
				  ((qse_awk_val_str_t*)val)->val.len != 0))? 1: 0;
			break;
		}

		case QSE_AWK_GBL_NF:
		{
			int n;
			qse_awk_int_t lv;

			n = qse_awk_rtx_valtoint (rtx, val, &lv);
			if (n <= -1) return -1;

			if (lv < (qse_awk_int_t)rtx->inrec.nflds)
			{
				if (shorten_record (rtx, (qse_size_t)lv) == -1)
				{
					/* adjust the error line */
					/*if (var) ADJERR_LOC (rtx, &var->loc);*/
					return -1;
				}
			}

			break;
		}


		case QSE_AWK_GBL_NR:
		{
			int n;
			qse_awk_int_t lv;

			n = qse_awk_rtx_valtoint (rtx, val, &lv);
			if (n <= -1) return -1;

			rtx->gbl.nr = lv;
			break;
		}

		case QSE_AWK_GBL_OFMT:
		{
			qse_size_t i;
			qse_awk_rtx_valtostr_out_t out;

			out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
			if (qse_awk_rtx_valtostr (rtx, val, &out) <= -1) return -1;

			for (i = 0; i < out.u.cpldup.len; i++)
			{
				if (out.u.cpldup.ptr[i] == QSE_T('\0'))
				{
					QSE_AWK_FREE (rtx->awk, out.u.cpldup.ptr);
					SETERR_COD (rtx, QSE_AWK_EOFMTCHR);
					return -1;
				}
			}

			if (rtx->gbl.ofmt.ptr != QSE_NULL)
				QSE_AWK_FREE (rtx->awk, rtx->gbl.ofmt.ptr);
			rtx->gbl.ofmt.ptr = out.u.cpldup.ptr;
			rtx->gbl.ofmt.len = out.u.cpldup.len;

			break;
		}

		case QSE_AWK_GBL_OFS:
		{	
			qse_awk_rtx_valtostr_out_t out;

			out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
			if (qse_awk_rtx_valtostr (rtx, val, &out) <= -1) return -1;

			if (rtx->gbl.ofs.ptr != QSE_NULL)
				QSE_AWK_FREE (rtx->awk, rtx->gbl.ofs.ptr);
			rtx->gbl.ofs.ptr = out.u.cpldup.ptr;
			rtx->gbl.ofs.len = out.u.cpldup.len;

			break;
		}

		case QSE_AWK_GBL_ORS:
		{	
			qse_awk_rtx_valtostr_out_t out;

			out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
			if (qse_awk_rtx_valtostr (rtx, val, &out) <= -1) return -1;

			if (rtx->gbl.ors.ptr != QSE_NULL)
				QSE_AWK_FREE (rtx->awk, rtx->gbl.ors.ptr);
			rtx->gbl.ors.ptr = out.u.cpldup.ptr;
			rtx->gbl.ors.len = out.u.cpldup.len;

			break;
		}

		case QSE_AWK_GBL_RS:
		{
			qse_xstr_t rss;

			if (val->type == QSE_AWK_VAL_STR)
			{
				rss = ((qse_awk_val_str_t*)val)->val;
			}
			else
			{
				qse_awk_rtx_valtostr_out_t out;

				/* due to the expression evaluation rule, the 
				 * regular expression can not be an assigned 
				 * value */
				QSE_ASSERT (val->type != QSE_AWK_VAL_REX);

				out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
				if (qse_awk_rtx_valtostr (rtx, val, &out) <= -1) return -1;

				rss = out.u.cpldup;
			}

			if (rtx->gbl.rs[0])
			{
				qse_awk_freerex (rtx->awk, rtx->gbl.rs[0], rtx->gbl.rs[1]);
				rtx->gbl.rs[0] = QSE_NULL;
				rtx->gbl.rs[1] = QSE_NULL;
			}

			if (rss.len > 1)
			{
				void* rex, * irex;
				qse_awk_errnum_t errnum;
				
				/* compile the regular expression */
				if (qse_awk_buildrex (rtx->awk, rss.ptr, rss.len, &errnum, &rex, &irex) <= -1)
				{
					SETERR_COD (rtx, errnum);
					if (val->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (rtx->awk, rss.ptr);
					return -1;
				}

				rtx->gbl.rs[0] = rex;
				rtx->gbl.rs[1] = irex;
			}

			if (val->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (rtx->awk, rss.ptr);

			break;
		}

		case QSE_AWK_GBL_SUBSEP:
		{
			qse_awk_rtx_valtostr_out_t out;

			out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
			if (qse_awk_rtx_valtostr (rtx, val, &out) <= -1) return -1;

			if (rtx->gbl.subsep.ptr)
				QSE_AWK_FREE (rtx->awk, rtx->gbl.subsep.ptr);
			rtx->gbl.subsep.ptr = out.u.cpldup.ptr;
			rtx->gbl.subsep.len = out.u.cpldup.len;

			break;
		}
	}

	qse_awk_rtx_refdownval (rtx, old);
	RTX_STACK_GBL(rtx,idx) = val;
	qse_awk_rtx_refupval (rtx, val);

	for (ecb = (rtx)->ecb; ecb; ecb = ecb->next)
		if (ecb->gblset) ecb->gblset (rtx, idx, val);

	return 0;
}

QSE_INLINE void qse_awk_rtx_setretval (
	qse_awk_rtx_t* rtx, qse_awk_val_t* val)
{
	qse_awk_rtx_refdownval (rtx, RTX_STACK_RETVAL(rtx));
	RTX_STACK_RETVAL(rtx) = val;
	/* should use the same trick as run_return */
	qse_awk_rtx_refupval (rtx, val);
}

QSE_INLINE int qse_awk_rtx_setgbl (
	qse_awk_rtx_t* rtx, int id, qse_awk_val_t* val)
{
	QSE_ASSERT (id >= 0 && id < (int)QSE_LDA_SIZE(rtx->awk->parse.gbls));
	return set_global (rtx, id, QSE_NULL, val, 0);
}

int qse_awk_rtx_setfilename (
	qse_awk_rtx_t* rtx, const qse_char_t* name, qse_size_t len)
{
	qse_awk_val_t* tmp;
	int n;

	if (len == 0) tmp = qse_awk_val_zls;
	else
	{
		tmp = qse_awk_rtx_makestrval (rtx, name, len);
		if (tmp == QSE_NULL) return -1;
	}

	qse_awk_rtx_refupval (rtx, tmp);
	n = qse_awk_rtx_setgbl (rtx, QSE_AWK_GBL_FILENAME, tmp);
	qse_awk_rtx_refdownval (rtx, tmp);

	return n;
}

int qse_awk_rtx_setofilename (
	qse_awk_rtx_t* rtx, const qse_char_t* name, qse_size_t len)
{
	qse_awk_val_t* tmp;
	int n;

	if (rtx->awk->opt.trait & QSE_AWK_NEXTOFILE)
	{
		if (len == 0) tmp = qse_awk_val_zls;
		else
		{
			tmp = qse_awk_rtx_makestrval (rtx, name, len);
			if (tmp == QSE_NULL) return -1;
		}

		qse_awk_rtx_refupval (rtx, tmp);
		n = qse_awk_rtx_setgbl (rtx, QSE_AWK_GBL_OFILENAME, tmp);
		qse_awk_rtx_refdownval (rtx, tmp);
	}
	else n = 0;

	return n;
}

void* qse_awk_rtx_getxtn (qse_awk_rtx_t* rtx)
{
	return QSE_XTN(rtx);
}

qse_awk_t* qse_awk_rtx_getawk (qse_awk_rtx_t* rtx)
{
	return rtx->awk;
}

qse_mmgr_t* qse_awk_rtx_getmmgr (qse_awk_rtx_t* rtx)
{
	return MMGR(rtx);
}

qse_htb_t* qse_awk_rtx_getnvmap (qse_awk_rtx_t* rtx)
{
	return rtx->named;
}

struct module_init_ctx_t
{
	qse_size_t count;
	qse_awk_rtx_t* rtx;
};

struct module_fini_ctx_t
{
	qse_size_t limit;
	qse_size_t count;
	qse_awk_rtx_t* rtx;
};

static qse_rbt_walk_t init_module (qse_rbt_t* rbt, qse_rbt_pair_t* pair, void* ctx)
{
	qse_awk_mod_data_t* md;
	struct module_init_ctx_t* mic;

	mic = (struct module_init_ctx_t*)ctx;

	md = (qse_awk_mod_data_t*)QSE_RBT_VPTR(pair);
	if (md->mod.init && md->mod.init (&md->mod, mic->rtx) <= -1)
		return QSE_RBT_WALK_STOP;

	mic->count++;
	return QSE_RBT_WALK_FORWARD;
}

static qse_rbt_walk_t fini_module (qse_rbt_t* rbt, qse_rbt_pair_t* pair, void* ctx)
{
	qse_awk_mod_data_t* md;
	struct module_fini_ctx_t* mfc;

	mfc = (struct module_fini_ctx_t*)ctx;

	if (mfc->limit > 0 && mfc->count >= mfc->limit) 
		return QSE_RBT_WALK_STOP;

	md = (qse_awk_mod_data_t*)QSE_RBT_VPTR(pair);
	if (md->mod.fini) md->mod.fini (&md->mod, mfc->rtx);

	mfc->count++;
	return QSE_RBT_WALK_FORWARD;
}

qse_awk_rtx_t* qse_awk_rtx_open (
	qse_awk_t* awk, qse_size_t xtnsize, qse_awk_rio_t* rio)
{
	qse_awk_rtx_t* rtx;
	struct module_init_ctx_t mic;

	/* clear the awk error code */
	qse_awk_seterrnum (awk, QSE_AWK_ENOERR, QSE_NULL);

	/* check if the code has ever been parsed */
	if (awk->tree.ngbls == 0 && 
	    awk->tree.begin == QSE_NULL &&
	    awk->tree.end == QSE_NULL &&
	    awk->tree.chain_size == 0 &&
	    qse_htb_getsize(awk->tree.funs) == 0)
	{
		qse_awk_seterrnum (awk, QSE_AWK_ENOPER, QSE_NULL);
		return QSE_NULL;
	}
	
	/* allocate the storage for the rtx object */
	rtx = (qse_awk_rtx_t*) QSE_AWK_ALLOC (
		awk, QSE_SIZEOF(qse_awk_rtx_t) + xtnsize);
	if (rtx == QSE_NULL)
	{
		/* if it fails, the failure is reported thru 
		 * the awk object */
		qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
		return QSE_NULL;
	}

	/* initialize the rtx object */
	QSE_MEMSET (rtx, 0, QSE_SIZEOF(qse_awk_rtx_t) + xtnsize);
	if (init_rtx (rtx, awk, rio) <= -1) 
	{
		QSE_AWK_FREE (awk, rtx);
		return QSE_NULL;
	}

	if (init_globals (rtx) <= -1)
	{
		awk->errinf = rtx->errinf; /* transfer error info */
		fini_rtx (rtx, 0);
		QSE_AWK_FREE (awk, rtx);
		return QSE_NULL;
	}

	mic.count = 0;
	mic.rtx = rtx;
	qse_rbt_walk (rtx->awk->modtab, init_module, &mic);
	if (mic.count != QSE_RBT_SIZE(rtx->awk->modtab))
	{
		awk->errinf = rtx->errinf; /* transfer error info */

		if (mic.count > 0)
		{
			struct module_fini_ctx_t mfc;
			mfc.limit = mic.count;
			mfc.count = 0;
			qse_rbt_walk (rtx->awk->modtab, fini_module, &mfc);
		}

		fini_rtx (rtx, 1);
		QSE_AWK_FREE (awk, rtx);
		return QSE_NULL;
	}

	return rtx;
}

void qse_awk_rtx_close (qse_awk_rtx_t* rtx)
{
	qse_awk_rtx_ecb_t* ecb;
	struct module_fini_ctx_t mfc;

	mfc.limit = 0;
	mfc.count = 0;
	mfc.rtx = rtx;
	qse_rbt_walk (rtx->awk->modtab, fini_module, &mfc);

	for (ecb = rtx->ecb; ecb; ecb = ecb->next)
		if (ecb->close) ecb->close (rtx);

	/* NOTE:
	 *  the close callbacks are called before data in rtx
	 *  is destroyed. if the destruction count on any data
	 *  destroyed by the close callback, something bad 
	 *  will happen.
	 */
	fini_rtx (rtx, 1);

	QSE_AWK_FREE (rtx->awk, rtx);
}

void qse_awk_rtx_stop (qse_awk_rtx_t* rtx)
{
	rtx->exit_level = EXIT_ABORT;
}

int qse_awk_rtx_isstop (qse_awk_rtx_t* rtx)
{
	return (rtx->exit_level == EXIT_ABORT || rtx->awk->stopall);
}

void qse_awk_rtx_getrio (qse_awk_rtx_t* rtx, qse_awk_rio_t* rio)
{
	rio->pipe = rtx->rio.handler[QSE_AWK_RIO_PIPE];
	rio->file = rtx->rio.handler[QSE_AWK_RIO_FILE];
	rio->console = rtx->rio.handler[QSE_AWK_RIO_CONSOLE];
}

void qse_awk_rtx_setrio (qse_awk_rtx_t* rtx, const qse_awk_rio_t* rio)
{
	rtx->rio.handler[QSE_AWK_RIO_PIPE] = rio->pipe;
	rtx->rio.handler[QSE_AWK_RIO_FILE] = rio->file;
	rtx->rio.handler[QSE_AWK_RIO_CONSOLE] = rio->console;
}

qse_awk_rtx_ecb_t* qse_awk_rtx_popecb (qse_awk_rtx_t* rtx)
{
	qse_awk_rtx_ecb_t* top = rtx->ecb;
	if (top) rtx->ecb = top->next;
	return top;
}

void qse_awk_rtx_pushecb (qse_awk_rtx_t* rtx, qse_awk_rtx_ecb_t* ecb)
{
	ecb->next = rtx->ecb;
	rtx->ecb = ecb;
}

static void free_namedval (qse_htb_t* map, void* dptr, qse_size_t dlen)
{
	qse_awk_rtx_refdownval (
		*(qse_awk_rtx_t**)QSE_XTN(map), dptr);
}

static void same_namedval (qse_htb_t* map, void* dptr, qse_size_t dlen)
{
	qse_awk_rtx_refdownval_nofree (
		*(qse_awk_rtx_t**)QSE_XTN(map), dptr);
}

static int init_rtx (qse_awk_rtx_t* rtx, qse_awk_t* awk, qse_awk_rio_t* rio)
{
	static qse_htb_style_t style_for_named =
	{
		{
			QSE_HTB_COPIER_INLINE,
			QSE_HTB_COPIER_DEFAULT 
		},
		{
			QSE_HTB_FREEER_DEFAULT,
			free_namedval 
		},
		QSE_HTB_COMPER_DEFAULT,
		same_namedval,
		QSE_HTB_SIZER_DEFAULT,
		QSE_HTB_HASHER_DEFAULT
	};

	rtx->awk = awk;

	CLRERR (rtx);

	rtx->stack = QSE_NULL;
	rtx->stack_top = 0;
	rtx->stack_base = 0;
	rtx->stack_limit = 0;

	rtx->exit_level = EXIT_NONE;

	rtx->vmgr.ichunk = QSE_NULL;
	rtx->vmgr.ifree = QSE_NULL;
	rtx->vmgr.rchunk = QSE_NULL;
	rtx->vmgr.rfree = QSE_NULL;

	rtx->inrec.buf_pos = 0;
	rtx->inrec.buf_len = 0;
	rtx->inrec.flds = QSE_NULL;
	rtx->inrec.nflds = 0;
	rtx->inrec.maxflds = 0;
	rtx->inrec.d0 = qse_awk_val_nil;

	if (qse_str_init (&rtx->inrec.line, MMGR(rtx), DEF_BUF_CAPA) <= -1) goto oops_0;
	if (qse_str_init (&rtx->inrec.linew, MMGR(rtx), DEF_BUF_CAPA) <= -1) goto oops_1;
	if (qse_str_init (&rtx->inrec.lineg, MMGR(rtx), DEF_BUF_CAPA) <= -1) goto oops_2;
	if (qse_str_init (&rtx->format.out, MMGR(rtx), 256) <= -1) goto oops_3;
	if (qse_str_init (&rtx->format.fmt, MMGR(rtx), 256) <= -1) goto oops_4;

	rtx->named = qse_htb_open (MMGR(rtx), QSE_SIZEOF(rtx), 1024, 70, QSE_SIZEOF(qse_char_t), 1);
	if (rtx->named == QSE_NULL) goto oops_5;
	*(qse_awk_rtx_t**)QSE_XTN(rtx->named) = rtx;
	qse_htb_setstyle (rtx->named, &style_for_named);

	rtx->format.tmp.ptr = (qse_char_t*)
		QSE_AWK_ALLOC (rtx->awk, 4096*QSE_SIZEOF(qse_char_t*));
	if (rtx->format.tmp.ptr == QSE_NULL) goto oops_6;
	rtx->format.tmp.len = 4096;
	rtx->format.tmp.inc = 4096*2;

	if (rtx->awk->tree.chain_size > 0)
	{
		rtx->pattern_range_state = (qse_byte_t*) QSE_AWK_ALLOC (
			rtx->awk, rtx->awk->tree.chain_size*QSE_SIZEOF(qse_byte_t));
		if (rtx->pattern_range_state == QSE_NULL) goto oops_7;

		QSE_MEMSET (
			rtx->pattern_range_state, 0, 
			rtx->awk->tree.chain_size * QSE_SIZEOF(qse_byte_t));
	}
	else rtx->pattern_range_state = QSE_NULL;

	if (rio)
	{
		rtx->rio.handler[QSE_AWK_RIO_PIPE] = rio->pipe;
		rtx->rio.handler[QSE_AWK_RIO_FILE] = rio->file;
		rtx->rio.handler[QSE_AWK_RIO_CONSOLE] = rio->console;
		rtx->rio.chain = QSE_NULL;
	}

	rtx->gbl.rs[0] = QSE_NULL;
	rtx->gbl.rs[1] = QSE_NULL;
	rtx->gbl.fs[0] = QSE_NULL;
	rtx->gbl.fs[1] = QSE_NULL;
	rtx->gbl.ignorecase = 0;

	return 0;

oops_7:
	QSE_AWK_FREE (rtx->awk, rtx->format.tmp.ptr);
oops_6:
	qse_htb_close (rtx->named);
oops_5:
	qse_str_fini (&rtx->format.fmt);
oops_4:
	qse_str_fini (&rtx->format.out);
oops_3:
	qse_str_fini (&rtx->inrec.lineg);
oops_2:
	qse_str_fini (&rtx->inrec.linew);
oops_1:
	qse_str_fini (&rtx->inrec.line);
oops_0:
	qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
	return -1;
}

static void fini_rtx (qse_awk_rtx_t* rtx, int fini_globals)
{
	if (rtx->pattern_range_state != QSE_NULL)
		QSE_AWK_FREE (rtx->awk, rtx->pattern_range_state);

	/* close all pending io's */
	/* TODO: what if this operation fails? */
	qse_awk_rtx_cleario (rtx);
	QSE_ASSERT (rtx->rio.chain == QSE_NULL);

	if (rtx->gbl.rs[0])
	{
		qse_awk_freerex (rtx->awk, rtx->gbl.rs[0], rtx->gbl.rs[1]);
		rtx->gbl.rs[0] = QSE_NULL;
		rtx->gbl.rs[1] = QSE_NULL;
	}
	if (rtx->gbl.fs[0])
	{
		qse_awk_freerex (rtx->awk, rtx->gbl.fs[0], rtx->gbl.fs[1]);
		rtx->gbl.fs[0] = QSE_NULL;
		rtx->gbl.fs[1] = QSE_NULL;
	}

	if (rtx->gbl.convfmt.ptr != QSE_NULL &&
	    rtx->gbl.convfmt.ptr != DEFAULT_CONVFMT)
	{
		QSE_AWK_FREE (rtx->awk, rtx->gbl.convfmt.ptr);
		rtx->gbl.convfmt.ptr = QSE_NULL;
		rtx->gbl.convfmt.len = 0;
	}

	if (rtx->gbl.ofmt.ptr != QSE_NULL && 
	    rtx->gbl.ofmt.ptr != DEFAULT_OFMT)
	{
		QSE_AWK_FREE (rtx->awk, rtx->gbl.ofmt.ptr);
		rtx->gbl.ofmt.ptr = QSE_NULL;
		rtx->gbl.ofmt.len = 0;
	}

	if (rtx->gbl.ofs.ptr != QSE_NULL && 
	    rtx->gbl.ofs.ptr != DEFAULT_OFS)
	{
		QSE_AWK_FREE (rtx->awk, rtx->gbl.ofs.ptr);
		rtx->gbl.ofs.ptr = QSE_NULL;
		rtx->gbl.ofs.len = 0;
	}

	if (rtx->gbl.ors.ptr != QSE_NULL && 
	    rtx->gbl.ors.ptr != DEFAULT_ORS &&
	    rtx->gbl.ors.ptr != DEFAULT_ORS_CRLF)
	{
		QSE_AWK_FREE (rtx->awk, rtx->gbl.ors.ptr);
		rtx->gbl.ors.ptr = QSE_NULL;
		rtx->gbl.ors.len = 0;
	}

	if (rtx->gbl.subsep.ptr != QSE_NULL && 
	    rtx->gbl.subsep.ptr != DEFAULT_SUBSEP)
	{
		QSE_AWK_FREE (rtx->awk, rtx->gbl.subsep.ptr);
		rtx->gbl.subsep.ptr = QSE_NULL;
		rtx->gbl.subsep.len = 0;
	}

	QSE_AWK_FREE (rtx->awk, rtx->format.tmp.ptr);
	rtx->format.tmp.ptr = QSE_NULL;
	rtx->format.tmp.len = 0;
	qse_str_fini (&rtx->format.fmt);
	qse_str_fini (&rtx->format.out);

	/* destroy input record. qse_awk_rtx_clrrec() should be called
	 * before the stack has been destroyed because it may try
	 * to change the value to QSE_AWK_GBL_NF. */
	qse_awk_rtx_clrrec (rtx, 0);  
	if (rtx->inrec.flds)
	{
		QSE_AWK_FREE (rtx->awk, rtx->inrec.flds);
		rtx->inrec.flds = QSE_NULL;
		rtx->inrec.maxflds = 0;
	}
	qse_str_fini (&rtx->inrec.lineg);
	qse_str_fini (&rtx->inrec.linew);
	qse_str_fini (&rtx->inrec.line);

	if (fini_globals) refdown_globals (rtx, 1);

	/* destroy the stack if necessary */
	if (rtx->stack)
	{
		QSE_ASSERT (rtx->stack_top == 0);

		QSE_AWK_FREE (rtx->awk, rtx->stack);
		rtx->stack = QSE_NULL;
		rtx->stack_top = 0;
		rtx->stack_base = 0;
		rtx->stack_limit = 0;
	}

	/* destroy named variables */
	qse_htb_close (rtx->named);

	/* destroy values in free list */
	while (rtx->rcache_count > 0)
	{
		qse_awk_val_ref_t* tmp = rtx->rcache[--rtx->rcache_count];
		qse_awk_rtx_freeval (rtx, (qse_awk_val_t*)tmp, 0);
	}

#ifdef ENABLE_FEATURE_SCACHE
	{
		int i;
		for (i = 0; i < QSE_COUNTOF(rtx->scache_count); i++)
		{
			while (rtx->scache_count[i] > 0)
			{
				qse_awk_val_str_t* t = 
					rtx->scache[i][--rtx->scache_count[i]];
				qse_awk_rtx_freeval (
					rtx, (qse_awk_val_t*)t, 0);
			}
		}
	}
#endif

	qse_awk_rtx_freevalchunk (rtx, rtx->vmgr.ichunk);
	qse_awk_rtx_freevalchunk (rtx, rtx->vmgr.rchunk);
	rtx->vmgr.ichunk = QSE_NULL;
	rtx->vmgr.rchunk = QSE_NULL;
}

static int update_fnr (qse_awk_rtx_t* rtx, qse_awk_int_t fnr, qse_awk_int_t nr)
{
	qse_awk_val_t* tmp1, * tmp2;

	tmp1 = qse_awk_rtx_makeintval (rtx, fnr);
	if (tmp1 == QSE_NULL) return -1;

	qse_awk_rtx_refupval (rtx, tmp1);

	if (nr == fnr) tmp2 = tmp1;
	else
	{
		tmp2 = qse_awk_rtx_makeintval (rtx, nr);
		if (tmp2 == QSE_NULL)
		{
			qse_awk_rtx_refdownval (rtx, tmp1);
			return -1;
		}

		qse_awk_rtx_refupval (rtx, tmp2);
	}

	if (qse_awk_rtx_setgbl (rtx, QSE_AWK_GBL_FNR, tmp1) == -1)
	{
		if (nr != fnr) qse_awk_rtx_refdownval (rtx, tmp2);
		qse_awk_rtx_refdownval (rtx, tmp1);
		return -1;
	}

	if (qse_awk_rtx_setgbl (rtx, QSE_AWK_GBL_NR, tmp2) == -1)
	{
		if (nr != fnr) qse_awk_rtx_refdownval (rtx, tmp2);
		qse_awk_rtx_refdownval (rtx, tmp1);
		return -1;
	}

	if (nr != fnr) qse_awk_rtx_refdownval (rtx, tmp2);
	qse_awk_rtx_refdownval (rtx, tmp1);
	return 0;
}

/* 
 * create global variables into the runtime stack
 * each variable is initialized to nil or zero.
 */
static int prepare_globals (qse_awk_rtx_t* rtx)
{
	qse_size_t saved_stack_top;
	qse_size_t ngbls;

	saved_stack_top = rtx->stack_top;
	ngbls = rtx->awk->tree.ngbls;

	/* initialize all global variables to nil by push nils to the stack */
	while (ngbls > 0)
	{
		--ngbls;
		if (__raw_push(rtx,qse_awk_val_nil) == -1)
		{
			SETERR_COD (rtx, QSE_AWK_ENOMEM);
			goto oops;
		}
	}	

	/* override NF to zero */
	if (qse_awk_rtx_setgbl (
		rtx, QSE_AWK_GBL_NF, qse_awk_val_zero) <= -1) goto oops;

	/* return success */
	return 0;

oops:
	/* restore the stack_top this way instead of calling __raw_pop()
	 * as many times as successful __raw_push(). it is ok because
	 * the values pushed so far are qse_awk_val_nils and qse_awk_val_zeros.
	 */
	rtx->stack_top = saved_stack_top;
	return -1;
}

/*
 * assign initial values to the global variables whose desired initial
 * values are not nil or zero. some are handled in prepare_globals () and 
 * update_fnr().
 */
static int defaultify_globals (qse_awk_rtx_t* rtx)
{
	struct gtab_t
	{
		int idx;
		const qse_char_t* str;
	} gtab[] =
	{
		{ QSE_AWK_GBL_CONVFMT,   DEFAULT_CONVFMT },
		{ QSE_AWK_GBL_FILENAME,  QSE_NULL },
		{ QSE_AWK_GBL_OFILENAME, QSE_NULL },
		{ QSE_AWK_GBL_OFMT,      DEFAULT_OFMT },
		{ QSE_AWK_GBL_OFS,       DEFAULT_OFS },
		{ QSE_AWK_GBL_ORS,       DEFAULT_ORS },
		{ QSE_AWK_GBL_SUBSEP,    DEFAULT_SUBSEP },
	};

	qse_awk_val_t* tmp;
	qse_size_t i, j;

	if (rtx->awk->opt.trait & QSE_AWK_CRLF)
	{
		/* ugly */
		gtab[5].str = DEFAULT_ORS_CRLF;
	}

	for (i = 0; i < QSE_COUNTOF(gtab); i++)
	{
		if (gtab[i].str == QSE_NULL || gtab[i].str[0] == QSE_T('\0'))
		{
			tmp = qse_awk_val_zls;
		}
		else 
		{
			tmp = qse_awk_rtx_makestrvalwithstr (rtx, gtab[i].str);
			if (tmp == QSE_NULL) return -1;
		}
		
		qse_awk_rtx_refupval (rtx, tmp);

		QSE_ASSERT (RTX_STACK_GBL(rtx,gtab[i].idx) == qse_awk_val_nil);

		if (qse_awk_rtx_setgbl (rtx, gtab[i].idx, tmp) == -1)
		{
			for (j = 0; j < i; j++)
			{
				qse_awk_rtx_setgbl (
					rtx, gtab[i].idx, qse_awk_val_nil);
			}

			qse_awk_rtx_refdownval (rtx, tmp);
			return -1;
		}

		qse_awk_rtx_refdownval (rtx, tmp);
	}

	return 0;
}

static void refdown_globals (qse_awk_rtx_t* run, int pop)
{
	qse_size_t ngbls;
       
	ngbls = run->awk->tree.ngbls;
	while (ngbls > 0)
	{
		--ngbls;
		qse_awk_rtx_refdownval (run, RTX_STACK_GBL(run,ngbls));
		if (pop) __raw_pop (run);
		else RTX_STACK_GBL(run,ngbls) = qse_awk_val_nil;
	}
}

static int init_globals (qse_awk_rtx_t* rtx)
{
	/* the stack must be clean when this function is invoked */
	QSE_ASSERTX (rtx->stack_base == 0, "stack not clean");
	QSE_ASSERTX (rtx->stack_top == 0, "stack not clean");

	if (prepare_globals (rtx) == -1) return -1;
	if (update_fnr (rtx, 0, 0) == -1) goto oops;
	if (defaultify_globals (rtx) == -1) goto oops;
	return 0;

oops:
	refdown_globals (rtx, 1);
	return -1;
}

struct capture_retval_data_t
{
	qse_awk_rtx_t* rtx;
	qse_awk_val_t* val;	
};

static void capture_retval_on_exit (void* arg)
{
	struct capture_retval_data_t* data;

	data = (struct capture_retval_data_t*)arg;
	data->val = RTX_STACK_RETVAL(data->rtx);
	qse_awk_rtx_refupval (data->rtx, data->val);
}

static int enter_stack_frame (qse_awk_rtx_t* rtx)
{
	qse_size_t saved_stack_top;

	/* remember the current stack top */
	saved_stack_top = rtx->stack_top;

	/* push the current stack base */
	if (__raw_push(rtx,(void*)rtx->stack_base) == -1) goto oops;

	/* push the current stack top before push the current stack base */
	if (__raw_push(rtx,(void*)saved_stack_top) == -1) goto oops;
	
	/* secure space for a return value */
	if (__raw_push(rtx,qse_awk_val_nil) == -1) goto oops;
	
	/* secure space for RTX_STACK_NARGS */
	if (__raw_push(rtx,qse_awk_val_nil) == -1) goto oops;

	/* let the stack top remembered be the base of a new stack frame */
	rtx->stack_base = saved_stack_top;
	return 0;

oops:
	/* restore the stack top in a cheesy(?) way. 
	 * it is ok to do so as the values pushed are
	 * nils and binary numbers. */
	rtx->stack_top = saved_stack_top;
	SETERR_COD (rtx, QSE_AWK_ENOMEM);
	return -1;
}

static void exit_stack_frame (qse_awk_rtx_t* run)
{
	/* At this point, the current stack frame should have 
	 * the 4 entries pushed in enter_stack_frame(). */
	QSE_ASSERT ((run->stack_top-run->stack_base) == 4);

	run->stack_top = (qse_size_t)run->stack[run->stack_base+1];
	run->stack_base = (qse_size_t)run->stack[run->stack_base+0];
}

static qse_awk_val_t* run_bpae_loop (qse_awk_rtx_t* rtx)
{
	qse_awk_nde_t* nde;
	qse_size_t nargs, i;
	qse_awk_val_t* retv;
	int ret = 0;

	/* set nargs to zero */
	nargs = 0;
	RTX_STACK_NARGS(rtx) = (void*)nargs;

	/* execute the BEGIN block */
	for (nde = rtx->awk->tree.begin; 
	     ret == 0 && nde != QSE_NULL && rtx->exit_level < EXIT_GLOBAL;
	     nde = nde->next)
	{
		qse_awk_nde_blk_t* blk;

		blk = (qse_awk_nde_blk_t*)nde;
		QSE_ASSERT (blk->type == QSE_AWK_NDE_BLK);

		rtx->active_block = blk;
		rtx->exit_level = EXIT_NONE;
		if (run_block (rtx, blk) == -1) ret = -1;
	}

	if (ret <= -1 && rtx->errinf.num == QSE_AWK_ENOERR) 
	{
		/* an error is returned with no error number set.
		 * this trait is used by eval_expression() to
		 * abort the evaluation when exit() is executed 
		 * during function evaluation */
		ret = 0;
		CLRERR (rtx); /* clear it just in case */
	}

	/* run pattern block loops */
	if (ret == 0 && 
	    (rtx->awk->tree.chain != QSE_NULL ||
	     rtx->awk->tree.end != QSE_NULL) && 
	     rtx->exit_level < EXIT_GLOBAL)
	{
		if (run_pblocks(rtx) <= -1) ret = -1;
	}

	if (ret <= -1 && rtx->errinf.num == QSE_AWK_ENOERR)
	{
		/* an error is returned with no error number set.
		 * this trait is used by eval_expression() to
		 * abort the evaluation when exit() is executed 
		 * during function evaluation */
		ret = 0;
		CLRERR (rtx); /* clear it just in case */
	}

	/* execute END blocks. the first END block is executed if the 
	 * program is not explicitly aborted with qse_awk_rtx_stop().*/
	for (nde = rtx->awk->tree.end;
	     ret == 0 && nde != QSE_NULL && rtx->exit_level < EXIT_ABORT;
	     nde = nde->next) 
	{
		qse_awk_nde_blk_t* blk;

		blk = (qse_awk_nde_blk_t*)nde;
		QSE_ASSERT (blk->type == QSE_AWK_NDE_BLK);

		rtx->active_block = blk;
		rtx->exit_level = EXIT_NONE;
		if (run_block (rtx, blk) <= -1) ret = -1;
		else if (rtx->exit_level >= EXIT_GLOBAL) 
		{
			/* once exit is called inside one of END blocks,
			 * subsequent END blocks must not be executed */
			break;
		}
	}

	if (ret <= -1 && rtx->errinf.num == QSE_AWK_ENOERR)
	{
		/* an error is returned with no error number set.
		 * this trait is used by eval_expression() to
		 * abort the evaluation when exit() is executed 
		 * during function evaluation */
		ret = 0;
		CLRERR (rtx); /* clear it just in case */
	}

	/* derefrence all arguments. however, there should be no arguments 
	 * pushed to the stack as asserted below. we didn't push any arguments
	 * for BEGIN/pattern action/END block execution.*/
	nargs = (qse_size_t)RTX_STACK_NARGS(rtx);
	QSE_ASSERT (nargs == 0);
	for (i = 0; i < nargs; i++) 
		qse_awk_rtx_refdownval (rtx, RTX_STACK_ARG(rtx,i));

	/* get the return value in the current stack frame */
	retv = RTX_STACK_RETVAL(rtx);

	if (ret <= -1)
	{
		/* end the life of the global return value upon error */
		qse_awk_rtx_refdownval (rtx, retv);
		retv = QSE_NULL;
	}

	return retv;
}

/* start the BEGIN-pattern block-END loop */
qse_awk_val_t* qse_awk_rtx_loop (qse_awk_rtx_t* rtx)
{
	qse_awk_val_t* retv = QSE_NULL;

	rtx->exit_level = EXIT_NONE;

	if (enter_stack_frame (rtx) == 0)
	{
		retv = run_bpae_loop (rtx);
		exit_stack_frame (rtx);
	}

	/* reset the exit level */
	rtx->exit_level = EXIT_NONE;
	return retv;
}

/* find an AWK function by name */
qse_awk_fun_t* qse_awk_rtx_findfun (qse_awk_rtx_t* rtx, const qse_char_t* name)
{
	qse_htb_pair_t* pair;

	pair = qse_htb_search (
		rtx->awk->tree.funs, 
		name, qse_strlen(name)
	);

	if (pair == QSE_NULL)
	{
		qse_cstr_t nm;

		nm.ptr = name;
		nm.len = qse_strlen(name);

		SETERR_ARGX (rtx, QSE_AWK_EFUNNF, &nm);
		return QSE_NULL;
	}

	return (qse_awk_fun_t*)QSE_HTB_VPTR(pair);
}

/* call an AWK function by the function structure */
qse_awk_val_t* qse_awk_rtx_callfun (
	qse_awk_rtx_t* rtx, qse_awk_fun_t* fun,
	qse_awk_val_t* args[], qse_size_t nargs)
{
	struct capture_retval_data_t crdata;
	qse_awk_val_t* v;
	struct pafv pafv/*= { args, nargs }*/;
	qse_awk_nde_fncall_t call;

	QSE_ASSERT (fun != QSE_NULL);

	pafv.args = args;
	pafv.nargs = nargs;

	if (rtx->exit_level >= EXIT_GLOBAL) 
	{
		/* cannot call the function again when exit() is called
		 * in an AWK program or qse_awk_rtx_stop() is invoked */
		SETERR_COD (rtx, QSE_AWK_ENOPER);
		return QSE_NULL;
	}
	/*rtx->exit_level = EXIT_NONE;*/

	/* forge a fake node containing a function call */
	QSE_MEMSET (&call, 0, QSE_SIZEOF(call));
	call.type = QSE_AWK_NDE_FUN;
	call.u.fun.name = fun->name;
	call.nargs = nargs;

	/* check if the number of arguments given is more than expected */
	if (nargs > fun->nargs)
	{
		/* TODO: is this correct? what if i want to 
		 *       allow arbitarary numbers of arguments? */
		SETERR_COD (rtx, QSE_AWK_EARGTM);
		return QSE_NULL;
	}

	/* now that the function is found and ok, let's execute it */

	crdata.rtx = rtx;
	crdata.val = QSE_NULL;

	v = __eval_call (
		rtx, (qse_awk_nde_t*)&call, QSE_NULL, fun,
		push_arg_from_vals, (void*)&pafv,
		capture_retval_on_exit,
		&crdata
	);

	if (v == QSE_NULL)
	{
		/* an error occurred. let's check if it is caused by exit().
		 * if so, the return value should have been captured into
		 * crdata.val. */
		if (crdata.val != QSE_NULL) v = crdata.val; /* yet it is */
	}
	else
	{
		/* thr return value captured in termination by exit()
		 * is reference-counted up in capture_retval_on_exit().
		 * let's do the same thing for the return value normally
		 * returned. */
		qse_awk_rtx_refupval (rtx, v);
	}

	/* return the return value with its reference count at least 1.
	 * the caller of this function should count down its reference. */
	return v;
}

/* call an AWK function by name */
qse_awk_val_t* qse_awk_rtx_call (
	qse_awk_rtx_t* rtx, const qse_char_t* name, 
	qse_awk_val_t* args[], qse_size_t nargs)
{
	qse_awk_fun_t* fun;

	fun = qse_awk_rtx_findfun (rtx, name);
	if (fun == QSE_NULL) return QSE_NULL;

	return qse_awk_rtx_callfun (rtx, fun, args, nargs);
}

qse_awk_val_t* qse_awk_rtx_callwithstrs (
	qse_awk_rtx_t* rtx, const qse_char_t* name,
	const qse_char_t* args[], qse_size_t nargs)
{
	qse_size_t i;
	qse_awk_val_t** v, * ret;

	v = qse_awk_rtx_allocmem (rtx, QSE_SIZEOF(*v) * nargs);
	if (v == QSE_NULL) return QSE_NULL;

	for (i = 0; i < nargs; i++)
	{
		v[i] = qse_awk_rtx_makestrvalwithstr (rtx, args[i]);
		if (v[i] == QSE_NULL)
		{
			ret = QSE_NULL;
			goto oops;
		}

		qse_awk_rtx_refupval (rtx, v[i]);
	}

	ret = qse_awk_rtx_call (rtx, name, v, nargs);

oops:
	while (i > 0) 
	{
		qse_awk_rtx_refdownval (rtx, v[--i]);
	}
	QSE_MMGR_FREE  (rtx->awk->mmgr, v);
	return ret;
}

static int run_pblocks (qse_awk_rtx_t* run)
{
	int n;

#define ADJUST_ERROR(run) \
	if (run->awk->tree.chain != QSE_NULL) \
	{ \
		if (run->awk->tree.chain->pattern != QSE_NULL) \
			ADJERR_LOC (run, &run->awk->tree.chain->pattern->loc); \
		else if (run->awk->tree.chain->action != QSE_NULL) \
			ADJERR_LOC (run, &run->awk->tree.chain->action->loc); \
	} \
	else if (run->awk->tree.end != QSE_NULL) \
	{ \
		ADJERR_LOC (run, &run->awk->tree.end->loc); \
	} 

	run->inrec.buf_pos = 0;
	run->inrec.buf_len = 0;
	run->inrec.eof = 0;

	/* run each pattern block */
	while (run->exit_level < EXIT_GLOBAL)
	{
		run->exit_level = EXIT_NONE;

		n = read_record (run);
		if (n == -1) 
		{
			ADJUST_ERROR (run);
			return -1; /* error */
		}
		if (n == 0) break; /* end of input */

		if (run->awk->tree.chain != QSE_NULL)
		{
			if (run_pblock_chain (
				run, run->awk->tree.chain) == -1) return -1;
		}
	}

#undef ADJUST_ERROR
	return 0;
}

static int run_pblock_chain (qse_awk_rtx_t* run, qse_awk_chain_t* cha)
{
	qse_size_t bno = 0;

	while (run->exit_level < EXIT_GLOBAL && cha != QSE_NULL)
	{
		if (run->exit_level == EXIT_NEXT)
		{
			run->exit_level = EXIT_NONE;
			break;
		}

		if (run_pblock (run, cha, bno) == -1) return -1;

		cha = cha->next;
		bno++;
	}

	return 0;
}

static int run_pblock (qse_awk_rtx_t* rtx, qse_awk_chain_t* cha, qse_size_t bno)
{
	qse_awk_nde_t* ptn;
	qse_awk_nde_blk_t* blk;

	ptn = cha->pattern;
	blk = (qse_awk_nde_blk_t*)cha->action;

	if (ptn == QSE_NULL)
	{
		/* just execute the block */
		rtx->active_block = blk;
		if (run_block (rtx, blk) == -1) return -1;
	}
	else
	{
		if (ptn->next == QSE_NULL)
		{
			/* pattern { ... } */
			qse_awk_val_t* v1;

			v1 = eval_expression (rtx, ptn);
			if (v1 == QSE_NULL) return -1;

			qse_awk_rtx_refupval (rtx, v1);

			if (qse_awk_rtx_valtobool (rtx, v1))
			{
				rtx->active_block = blk;
				if (run_block (rtx, blk) == -1)
				{
					qse_awk_rtx_refdownval (rtx, v1);
					return -1;
				}
			}

			qse_awk_rtx_refdownval (rtx, v1);
		}
		else
		{
			/* pattern, pattern { ... } */
			QSE_ASSERT (ptn->next->next == QSE_NULL);
			QSE_ASSERT (rtx->pattern_range_state != QSE_NULL);

			if (rtx->pattern_range_state[bno] == 0)
			{
				qse_awk_val_t* v1;

				v1 = eval_expression (rtx, ptn);
				if (v1 == QSE_NULL) return -1;
				qse_awk_rtx_refupval (rtx, v1);

				if (qse_awk_rtx_valtobool (rtx, v1))
				{
					rtx->active_block = blk;
					if (run_block (rtx, blk) == -1)
					{
						qse_awk_rtx_refdownval (rtx, v1);
						return -1;
					}

					rtx->pattern_range_state[bno] = 1;
				}

				qse_awk_rtx_refdownval (rtx, v1);
			}
			else if (rtx->pattern_range_state[bno] == 1)
			{
				qse_awk_val_t* v2;

				v2 = eval_expression (rtx, ptn->next);
				if (v2 == QSE_NULL) return -1;
				qse_awk_rtx_refupval (rtx, v2);

				rtx->active_block = blk;
				if (run_block (rtx, blk) == -1)
				{
					qse_awk_rtx_refdownval (rtx, v2);
					return -1;
				}

				if (qse_awk_rtx_valtobool (rtx, v2))
					rtx->pattern_range_state[bno] = 0;

				qse_awk_rtx_refdownval (rtx, v2);
			}
		}
	}

	return 0;
}

static int run_block (qse_awk_rtx_t* rtx, qse_awk_nde_blk_t* nde)
{
	int n;

	if (rtx->awk->opt.depth.s.block_run > 0 &&
	    rtx->depth.block >= rtx->awk->opt.depth.s.block_run)
	{
		SETERR_LOC (rtx, QSE_AWK_EBLKNST, &nde->loc);
		return -1;;
	}

	rtx->depth.block++;
	n = run_block0 (rtx, nde);
	rtx->depth.block--;
	
	return n;
}

static int run_block0 (qse_awk_rtx_t* rtx, qse_awk_nde_blk_t* nde)
{
	qse_awk_nde_t* p;
	qse_size_t nlcls;
	qse_size_t saved_stack_top;
	int n = 0;

	if (nde == QSE_NULL)
	{
		/* blockless pattern - execute print $0*/
		qse_awk_rtx_refupval (rtx, rtx->inrec.d0);

		n = qse_awk_rtx_writeio_str (rtx,
			QSE_AWK_OUT_CONSOLE, QSE_T(""),
			QSE_STR_PTR(&rtx->inrec.line),
			QSE_STR_LEN(&rtx->inrec.line));
		if (n == -1)
		{
			qse_awk_rtx_refdownval (rtx, rtx->inrec.d0);
			ADJERR_LOC (rtx, &nde->loc);
			return -1;
		}

		n = qse_awk_rtx_writeio_str (
			rtx, QSE_AWK_OUT_CONSOLE, QSE_T(""),
			rtx->gbl.ors.ptr, rtx->gbl.ors.len);
		if (n == -1)
		{
			qse_awk_rtx_refdownval (rtx, rtx->inrec.d0);
			ADJERR_LOC (rtx, &nde->loc);
			return -1;
		}

		qse_awk_rtx_refdownval (rtx, rtx->inrec.d0);
		return 0;
	}

	QSE_ASSERT (nde->type == QSE_AWK_NDE_BLK);

	p = nde->body;
	nlcls = nde->nlcls;

#ifdef DEBUG_RUN
	qse_errputstrf (
		QSE_T("securing space for local variables nlcls = %d\n"), 
		(int)nlcls);
#endif

	saved_stack_top = rtx->stack_top;

	/* secure space for local variables */
	while (nlcls > 0)
	{
		--nlcls;
		if (__raw_push(rtx,qse_awk_val_nil) == -1)
		{
			/* restore stack top */
			rtx->stack_top = saved_stack_top;
			return -1;
		}

		/* refupval is not required for qse_awk_val_nil */
	}

#ifdef DEBUG_RUN
	qse_errputstrf (QSE_T("executing block statements\n"));
#endif

	while (p != QSE_NULL && rtx->exit_level == EXIT_NONE)
	{
		if (run_statement (rtx, p) == -1)
		{
			n = -1;
			break;
		}
		p = p->next;
	}

	/* pop off local variables */
#ifdef DEBUG_RUN
	qse_errputstrf (QSE_T("popping off local variables\n"));
#endif
	nlcls = nde->nlcls;
	while (nlcls > 0)
	{
		--nlcls;
		qse_awk_rtx_refdownval (rtx, RTX_STACK_LCL(rtx,nlcls));
		__raw_pop (rtx);
	}

	return n;
}

#define ON_STATEMENT(rtx,nde) QSE_BLOCK ( \
	qse_awk_rtx_ecb_t* ecb; \
	if ((rtx)->awk->stopall) (rtx)->exit_level = EXIT_ABORT; \
	for (ecb = (rtx)->ecb; ecb; ecb = ecb->next) \
		if (ecb->stmt) ecb->stmt (rtx, nde);  \
)

static int run_statement (qse_awk_rtx_t* rtx, qse_awk_nde_t* nde)
{
	int xret;
	qse_awk_val_t* tmp;

	ON_STATEMENT (rtx, nde);

	switch (nde->type) 
	{
		case QSE_AWK_NDE_NULL:
			/* do nothing */
			xret = 0;
			break;

		case QSE_AWK_NDE_BLK:
			xret = run_block (rtx, (qse_awk_nde_blk_t*)nde);
			break;

		case QSE_AWK_NDE_IF:
			xret = run_if (rtx, (qse_awk_nde_if_t*)nde);
			break;

		case QSE_AWK_NDE_WHILE:
		case QSE_AWK_NDE_DOWHILE:
			xret = run_while (rtx, (qse_awk_nde_while_t*)nde);
			break;

		case QSE_AWK_NDE_FOR:
			xret = run_for (rtx, (qse_awk_nde_for_t*)nde);
			break;

		case QSE_AWK_NDE_FOREACH:
			xret = run_foreach (rtx, (qse_awk_nde_foreach_t*)nde);
			break;

		case QSE_AWK_NDE_BREAK:
			xret = run_break (rtx, (qse_awk_nde_break_t*)nde);
			break;

		case QSE_AWK_NDE_CONTINUE:
			xret = run_continue (rtx, (qse_awk_nde_continue_t*)nde);
			break;

		case QSE_AWK_NDE_RETURN:
			xret = run_return (rtx, (qse_awk_nde_return_t*)nde);
			break;

		case QSE_AWK_NDE_EXIT:
			xret = run_exit (rtx, (qse_awk_nde_exit_t*)nde);
			break;

		case QSE_AWK_NDE_NEXT:
			xret = run_next (rtx, (qse_awk_nde_next_t*)nde);
			break;

		case QSE_AWK_NDE_NEXTFILE:
			xret = run_nextfile (rtx, (qse_awk_nde_nextfile_t*)nde);
			break;

		case QSE_AWK_NDE_DELETE:
			xret = run_delete (rtx, (qse_awk_nde_delete_t*)nde);
			break;

		case QSE_AWK_NDE_RESET:
			xret = run_reset (rtx, (qse_awk_nde_reset_t*)nde);
			break;

		case QSE_AWK_NDE_PRINT:
			if (rtx->awk->opt.trait & QSE_AWK_TOLERANT) goto __fallback__;
			xret = run_print (rtx, (qse_awk_nde_print_t*)nde);
			break;

		case QSE_AWK_NDE_PRINTF:
			if (rtx->awk->opt.trait & QSE_AWK_TOLERANT) goto __fallback__;
			xret = run_printf (rtx, (qse_awk_nde_print_t*)nde);
			break;

		default:
		__fallback__:
			tmp = eval_expression (rtx, nde);
			if (tmp == QSE_NULL) xret = -1;
			else
			{
				/* destroy the value if not referenced */
				qse_awk_rtx_refupval (rtx, tmp);
				qse_awk_rtx_refdownval (rtx, tmp);
				xret = 0;
			}
			break;
	}

	return xret;
}

static int run_if (qse_awk_rtx_t* rtx, qse_awk_nde_if_t* nde)
{
	qse_awk_val_t* test;
	int n = 0;

	/* the test expression for the if statement cannot have 
	 * chained expressions. this should not be allowed by the
	 * parser first of all */
	QSE_ASSERT (nde->test->next == QSE_NULL);

	test = eval_expression (rtx, nde->test);
	if (test == QSE_NULL) return -1;

	qse_awk_rtx_refupval (rtx, test);
	if (qse_awk_rtx_valtobool (rtx, test))
	{
		n = run_statement (rtx, nde->then_part);
	}
	else if (nde->else_part != QSE_NULL)
	{
		n = run_statement (rtx, nde->else_part);
	}

	qse_awk_rtx_refdownval (rtx, test); /* TODO: is this correct?*/
	return n;
}

static int run_while (qse_awk_rtx_t* rtx, qse_awk_nde_while_t* nde)
{
	qse_awk_val_t* test;

	if (nde->type == QSE_AWK_NDE_WHILE)
	{
		/* no chained expressions are allowed for the test 
		 * expression of the while statement */
		QSE_ASSERT (nde->test->next == QSE_NULL);

		while (1)
		{
			ON_STATEMENT (rtx, nde->test);

			test = eval_expression (rtx, nde->test);
			if (test == QSE_NULL) return -1;

			qse_awk_rtx_refupval (rtx, test);

			if (qse_awk_rtx_valtobool (rtx, test))
			{
				if (run_statement(rtx,nde->body) == -1)
				{
					qse_awk_rtx_refdownval (rtx, test);
					return -1;
				}
			}
			else
			{
				qse_awk_rtx_refdownval (rtx, test);
				break;
			}

			qse_awk_rtx_refdownval (rtx, test);

			if (rtx->exit_level == EXIT_BREAK)
			{	
				rtx->exit_level = EXIT_NONE;
				break;
			}
			else if (rtx->exit_level == EXIT_CONTINUE)
			{
				rtx->exit_level = EXIT_NONE;
			}
			else if (rtx->exit_level != EXIT_NONE) break;

		}
	}
	else if (nde->type == QSE_AWK_NDE_DOWHILE)
	{
		/* no chained expressions are allowed for the test 
		 * expression of the while statement */
		QSE_ASSERT (nde->test->next == QSE_NULL);

		do
		{
			if (run_statement(rtx,nde->body) == -1) return -1;

			if (rtx->exit_level == EXIT_BREAK)
			{	
				rtx->exit_level = EXIT_NONE;
				break;
			}
			else if (rtx->exit_level == EXIT_CONTINUE)
			{
				rtx->exit_level = EXIT_NONE;
			}
			else if (rtx->exit_level != EXIT_NONE) break;

			ON_STATEMENT (rtx, nde->test);

			test = eval_expression (rtx, nde->test);
			if (test == QSE_NULL) return -1;

			qse_awk_rtx_refupval (rtx, test);

			if (!qse_awk_rtx_valtobool (rtx, test))
			{
				qse_awk_rtx_refdownval (rtx, test);
				break;
			}

			qse_awk_rtx_refdownval (rtx, test);
		}
		while (1);
	}

	return 0;
}

static int run_for (qse_awk_rtx_t* rtx, qse_awk_nde_for_t* nde)
{
	qse_awk_val_t* val;

	if (nde->init != QSE_NULL)
	{
		QSE_ASSERT (nde->init->next == QSE_NULL);

		ON_STATEMENT (rtx, nde->init);
		val = eval_expression(rtx,nde->init);
		if (val == QSE_NULL) return -1;

		qse_awk_rtx_refupval (rtx, val);
		qse_awk_rtx_refdownval (rtx, val);
	}

	while (1)
	{
		if (nde->test != QSE_NULL)
		{
			qse_awk_val_t* test;

			/* no chained expressions for the test expression of
			 * the for statement are allowed */
			QSE_ASSERT (nde->test->next == QSE_NULL);

			ON_STATEMENT (rtx, nde->test);
			test = eval_expression (rtx, nde->test);
			if (test == QSE_NULL) return -1;

			qse_awk_rtx_refupval (rtx, test);
			if (qse_awk_rtx_valtobool (rtx, test))
			{
				if (run_statement(rtx,nde->body) == -1)
				{
					qse_awk_rtx_refdownval (rtx, test);
					return -1;
				}
			}
			else
			{
				qse_awk_rtx_refdownval (rtx, test);
				break;
			}

			qse_awk_rtx_refdownval (rtx, test);
		}	
		else
		{
			if (run_statement(rtx,nde->body) == -1) return -1;
		}

		if (rtx->exit_level == EXIT_BREAK)
		{	
			rtx->exit_level = EXIT_NONE;
			break;
		}
		else if (rtx->exit_level == EXIT_CONTINUE)
		{
			rtx->exit_level = EXIT_NONE;
		}
		else if (rtx->exit_level != EXIT_NONE) break;

		if (nde->incr != QSE_NULL)
		{
			QSE_ASSERT (nde->incr->next == QSE_NULL);

			ON_STATEMENT (rtx, nde->incr);
			val = eval_expression (rtx, nde->incr);
			if (val == QSE_NULL) return -1;

			qse_awk_rtx_refupval (rtx, val);
			qse_awk_rtx_refdownval (rtx, val);
		}
	}

	return 0;
}

struct foreach_walker_t
{
	qse_awk_rtx_t* rtx;
	qse_awk_nde_t* var;
	qse_awk_nde_t* body;
	int ret;
};

static qse_htb_walk_t walk_foreach (
	qse_htb_t* map, qse_htb_pair_t* pair, void* arg)
{
	struct foreach_walker_t* w = (struct foreach_walker_t*)arg;
	qse_awk_val_t* str;

	str = (qse_awk_val_t*) qse_awk_rtx_makestrval (
		w->rtx, QSE_HTB_KPTR(pair), QSE_HTB_KLEN(pair));
	if (str == QSE_NULL) 
	{
		ADJERR_LOC (w->rtx, &w->var->loc);
		w->ret = -1;
		return QSE_HTB_WALK_STOP;
	}

	qse_awk_rtx_refupval (w->rtx, str);
	if (do_assignment (w->rtx, w->var, str) == QSE_NULL)
	{
		qse_awk_rtx_refdownval (w->rtx, str);
		w->ret = -1;
		return QSE_HTB_WALK_STOP;
	}

	if (run_statement (w->rtx, w->body) == -1)
	{
		qse_awk_rtx_refdownval (w->rtx, str);
		w->ret = -1;
		return QSE_HTB_WALK_STOP;
	}
	
	qse_awk_rtx_refdownval (w->rtx, str);

	if (w->rtx->exit_level == EXIT_BREAK)
	{	
		w->rtx->exit_level = EXIT_NONE;
		return QSE_HTB_WALK_STOP;
	}
	else if (w->rtx->exit_level == EXIT_CONTINUE)
	{
		w->rtx->exit_level = EXIT_NONE;
	}
	else if (w->rtx->exit_level != EXIT_NONE) 
	{
		return QSE_HTB_WALK_STOP;
	}

	return QSE_HTB_WALK_FORWARD;
}

static int run_foreach (qse_awk_rtx_t* rtx, qse_awk_nde_foreach_t* nde)
{
	qse_awk_nde_exp_t* test;
	qse_awk_val_t* rv;
	qse_htb_t* map;
	struct foreach_walker_t walker;

	test = (qse_awk_nde_exp_t*)nde->test;
	QSE_ASSERT (
		test->type == QSE_AWK_NDE_EXP_BIN && 
		test->opcode == QSE_AWK_BINOP_IN);

	/* chained expressions should not be allowed 
	 * by the parser first of all */
	QSE_ASSERT (test->right->next == QSE_NULL); 

	rv = eval_expression (rtx, test->right);
	if (rv == QSE_NULL) return -1;

	qse_awk_rtx_refupval (rtx, rv);
	if (rv->type == QSE_AWK_VAL_NIL) 
	{
		/* just return without excuting the loop body */
		qse_awk_rtx_refdownval (rtx, rv);
		return 0;
	}
	else if (rv->type != QSE_AWK_VAL_MAP)
	{
		qse_awk_rtx_refdownval (rtx, rv);
		SETERR_LOC (rtx, QSE_AWK_ENOTMAPIN, &test->right->loc);
		return -1;
	}
	map = ((qse_awk_val_map_t*)rv)->map;

	walker.rtx = rtx;
	walker.var = test->left;
	walker.body = nde->body;
	walker.ret = 0;
	qse_htb_walk (map, walk_foreach, &walker);

	qse_awk_rtx_refdownval (rtx, rv);
	return walker.ret;
}

static int run_break (qse_awk_rtx_t* run, qse_awk_nde_break_t* nde)
{
	run->exit_level = EXIT_BREAK;
	return 0;
}

static int run_continue (qse_awk_rtx_t* run, qse_awk_nde_continue_t* nde)
{
	run->exit_level = EXIT_CONTINUE;
	return 0;
}

static int run_return (qse_awk_rtx_t* run, qse_awk_nde_return_t* nde)
{
	if (nde->val != QSE_NULL)
	{
		qse_awk_val_t* val;

		/* chained expressions should not be allowed 
		 * by the parser first of all */
		QSE_ASSERT (nde->val->next == QSE_NULL); 

		val = eval_expression (run, nde->val);
		if (val == QSE_NULL) return -1;

		if (!(run->awk->opt.trait & QSE_AWK_FLEXMAP))
		{
			if (val->type == QSE_AWK_VAL_MAP)
			{
				/* cannot return a map */
				qse_awk_rtx_refupval (run, val);
				qse_awk_rtx_refdownval (run, val);
				SETERR_LOC (run, QSE_AWK_EMAPRET, &nde->loc);
				return -1;
			}
		}

		qse_awk_rtx_refdownval (run, RTX_STACK_RETVAL(run));
		RTX_STACK_RETVAL(run) = val;

		/* NOTE: see eval_call() for the trick */
		qse_awk_rtx_refupval (run, val); 
	}
	
	run->exit_level = EXIT_FUNCTION;
	return 0;
}

static int run_exit (qse_awk_rtx_t* run, qse_awk_nde_exit_t* nde)
{
	if (nde->val != QSE_NULL)
	{
		qse_awk_val_t* val;

		/* chained expressions should not be allowed 
		 * by the parser first of all */
		QSE_ASSERT (nde->val->next == QSE_NULL); 

		val = eval_expression (run, nde->val);
		if (val == QSE_NULL) return -1;

		qse_awk_rtx_refdownval (run, RTX_STACK_RETVAL_GBL(run));
		RTX_STACK_RETVAL_GBL(run) = val; /* global return value */

		qse_awk_rtx_refupval (run, val);
	}

	run->exit_level = (nde->abort)? EXIT_ABORT: EXIT_GLOBAL;
	return 0;
}

static int run_next (qse_awk_rtx_t* run, qse_awk_nde_next_t* nde)
{
	/* the parser checks if next has been called in the begin/end
	 * block or whereever inappropriate. so the runtime doesn't 
	 * check that explicitly */
	if  (run->active_block == (qse_awk_nde_blk_t*)run->awk->tree.begin)
	{
		SETERR_LOC (run, QSE_AWK_ERNEXTBEG, &nde->loc);
		return -1;
	}
	else if (run->active_block == (qse_awk_nde_blk_t*)run->awk->tree.end)
	{
		SETERR_LOC (run, QSE_AWK_ERNEXTEND, &nde->loc);
		return -1;
	}

	run->exit_level = EXIT_NEXT;
	return 0;
}

static int run_nextinfile (qse_awk_rtx_t* rtx, qse_awk_nde_nextfile_t* nde)
{
	int n;

	/* normal nextfile statement */
	if  (rtx->active_block == (qse_awk_nde_blk_t*)rtx->awk->tree.begin)
	{
		SETERR_LOC (rtx, QSE_AWK_ERNEXTFBEG, &nde->loc);
		return -1;
	}
	else if (rtx->active_block == (qse_awk_nde_blk_t*)rtx->awk->tree.end)
	{
		SETERR_LOC (rtx, QSE_AWK_ERNEXTFEND, &nde->loc);
		return -1;
	}

	n = qse_awk_rtx_nextio_read (rtx, QSE_AWK_IN_CONSOLE, QSE_T(""));
	if (n == -1)
	{
		ADJERR_LOC (rtx, &nde->loc);
		return -1;
	}

	if (n == 0)
	{
		/* no more input console */
		rtx->exit_level = EXIT_GLOBAL;
		return 0;
	}

	/* FNR resets to 0, NR remains the same */
	if (update_fnr (rtx, 0, rtx->gbl.nr) == -1) 
	{
		ADJERR_LOC (rtx, &nde->loc);
		return -1;
	}

	rtx->exit_level = EXIT_NEXT;
	return 0;

}

static int run_nextoutfile (qse_awk_rtx_t* rtx, qse_awk_nde_nextfile_t* nde)
{
	int n;

	/* nextofile can be called from BEGIN and END block unlike nextfile */

	n = qse_awk_rtx_nextio_write (rtx, QSE_AWK_OUT_CONSOLE, QSE_T(""));
	if (n == -1)
	{
		/* adjust the error line */
		ADJERR_LOC (rtx, &nde->loc);
		return -1;
	}

	if (n == 0)
	{
		/* should it terminate the program there is no more 
		 * output console? no. there will just be no more console 
		 * output */
		/*rtx->exit_level = EXIT_GLOBAL;*/
		return 0;
	}

	return 0;
}

static int run_nextfile (qse_awk_rtx_t* rtx, qse_awk_nde_nextfile_t* nde)
{
	return (nde->out)? 
		run_nextoutfile (rtx, nde): 
		run_nextinfile (rtx, nde);
}

static int delete_indexed (
	qse_awk_rtx_t* rtx, qse_htb_t* map, qse_awk_nde_var_t* var)
{
	qse_awk_val_t* idx;

	QSE_ASSERT (var->idx != QSE_NULL);

	idx = eval_expression (rtx, var->idx);
	if (idx == QSE_NULL) return -1;

	qse_awk_rtx_refupval (rtx, idx);

	if (idx->type == QSE_AWK_VAL_STR)
	{
		/* delete x["abc"] */

		qse_htb_delete (
			map, 
			((qse_awk_val_str_t*)idx)->val.ptr,
			((qse_awk_val_str_t*)idx)->val.len
		);
		qse_awk_rtx_refdownval (rtx, idx);
	}
	else
	{
		/* delete x[20] */
		qse_char_t buf[IDXBUFSIZE];
		qse_awk_rtx_valtostr_out_t out;

		/* try with a fixed-size buffer */
		out.type = QSE_AWK_RTX_VALTOSTR_CPLCPY;
		out.u.cplcpy.ptr = buf;
		out.u.cplcpy.len = QSE_COUNTOF(buf);
		if (qse_awk_rtx_valtostr (rtx, idx, &out) <= -1)
		{
			int n;

			/* retry it in dynamic mode */
			out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
			n = qse_awk_rtx_valtostr (rtx, idx, &out);
			qse_awk_rtx_refdownval (rtx, idx);
			if (n <= -1)
			{
				/* change the error line */
				ADJERR_LOC (rtx, &var->loc);
				return -1;
			}
			else
			{
				qse_htb_delete (map, out.u.cpldup.ptr, out.u.cpldup.len);
				QSE_AWK_FREE (rtx->awk, out.u.cpldup.ptr);
			}
		}
		else 
		{
			qse_awk_rtx_refdownval (rtx, idx);
			qse_htb_delete (map, out.u.cplcpy.ptr, out.u.cplcpy.len);
		}
	}

	return 0;
}

static int run_delete_named (qse_awk_rtx_t* rtx, qse_awk_nde_var_t* var)
{
	qse_htb_pair_t* pair;

	QSE_ASSERTX (
		(var->type == QSE_AWK_NDE_NAMED && var->idx == QSE_NULL) ||
		(var->type == QSE_AWK_NDE_NAMEDIDX && var->idx != QSE_NULL),
		"if a named variable has an index part and a named indexed variable doesn't have an index part, the program is definitely wrong");

	pair = qse_htb_search (
		rtx->named, var->id.name.ptr, var->id.name.len);
	if (pair == QSE_NULL)
	{
		qse_awk_val_t* tmp;

		/* value not set for the named variable. 
		 * create a map and assign it to the variable */

		tmp = qse_awk_rtx_makemapval (rtx);
		if (tmp == QSE_NULL) 
		{
			/* adjust error line */
			ADJERR_LOC (rtx, &var->loc);
			return -1;
		}

		pair = qse_htb_upsert (rtx->named, 
			var->id.name.ptr, var->id.name.len, tmp, 0);
		if (pair == QSE_NULL)
		{
			qse_awk_rtx_refupval (rtx, tmp);
			qse_awk_rtx_refdownval (rtx, tmp);
			SETERR_LOC (rtx, QSE_AWK_ENOMEM, &var->loc);
			return -1;
		}

		/* as this is the assignment, it needs to update
		 * the reference count of the target value. */
		qse_awk_rtx_refupval (rtx, tmp);
	}
	else
	{
		qse_awk_val_t* val;
		qse_htb_t* map;

		val = (qse_awk_val_t*)QSE_HTB_VPTR(pair);
		QSE_ASSERT (val != QSE_NULL);

		if (val->type != QSE_AWK_VAL_MAP)
		{
			SETERR_ARGX_LOC (
				rtx, QSE_AWK_ENOTDEL, 
				xstr_to_cstr(&var->id.name), &var->loc);
			return -1;
		}

		map = ((qse_awk_val_map_t*)val)->map;
		if (var->type == QSE_AWK_NDE_NAMEDIDX)
		{
			if (delete_indexed (rtx, map, var) <= -1)
			{
				return -1;
			}
		}
		else
		{
			qse_htb_clear (map);
		}
	}

	return 0;
}

static int run_delete_unnamed (qse_awk_rtx_t* rtx, qse_awk_nde_var_t* var)
{
	qse_awk_val_t* val;

	switch (var->type)
	{
		case QSE_AWK_NDE_GBL:
		case QSE_AWK_NDE_GBLIDX:
			val = RTX_STACK_GBL (rtx,var->id.idxa);
			break;
	
		case QSE_AWK_NDE_LCL:
		case QSE_AWK_NDE_LCLIDX:
			val = RTX_STACK_LCL (rtx,var->id.idxa);
			break;

		default:
			val = RTX_STACK_ARG (rtx,var->id.idxa);
			break;
	}

	QSE_ASSERT (val != QSE_NULL);

	if (val->type == QSE_AWK_VAL_NIL)
	{
		qse_awk_val_t* tmp;

		/* value not set.
		 * create a map and assign it to the variable */

		tmp = qse_awk_rtx_makemapval (rtx);
		if (tmp == QSE_NULL) 
		{
			ADJERR_LOC (rtx, &var->loc);
			return -1;
		}

		/* no need to reduce the reference count of
		 * the previous value because it was nil. */
		switch (var->type)
		{
			case QSE_AWK_NDE_GBL:
			case QSE_AWK_NDE_GBLIDX:
			{
				int x;

				qse_awk_rtx_refupval (rtx, tmp);
				x = qse_awk_rtx_setgbl (rtx, (int)var->id.idxa, tmp);
				qse_awk_rtx_refdownval (rtx, tmp);

				if (x <= -1)
				{
					ADJERR_LOC (rtx, &var->loc);
					return -1;
				}
				break;
			}

			case QSE_AWK_NDE_LCL:
			case QSE_AWK_NDE_LCLIDX:
				RTX_STACK_LCL(rtx,var->id.idxa) = tmp;
				qse_awk_rtx_refupval (rtx, tmp);
				break;

			default:
				RTX_STACK_ARG(rtx,var->id.idxa) = tmp;
				qse_awk_rtx_refupval (rtx, tmp);
				break;
		}
	}
	else
	{
		qse_htb_t* map;

		if (val->type != QSE_AWK_VAL_MAP)
		{
			SETERR_ARGX_LOC (
				rtx, QSE_AWK_ENOTDEL,
				xstr_to_cstr(&var->id.name), &var->loc);
			return -1;
		}

		map = ((qse_awk_val_map_t*)val)->map;
		if (var->type == QSE_AWK_NDE_GBLIDX ||
		    var->type == QSE_AWK_NDE_LCLIDX ||
		    var->type == QSE_AWK_NDE_ARGIDX)
		{
			if (delete_indexed (rtx, map, var) <= -1)
			{
				return -1;
			}
		}
		else
		{
			qse_htb_clear (map);
		}
	}

	return 0;
}

static int run_delete (qse_awk_rtx_t* rtx, qse_awk_nde_delete_t* nde)
{
	qse_awk_nde_var_t* var;

	var = (qse_awk_nde_var_t*) nde->var;

	switch (var->type)
	{
		case QSE_AWK_NDE_NAMED:
		case QSE_AWK_NDE_NAMEDIDX:
			return run_delete_named (rtx, var);
		
		case QSE_AWK_NDE_GBL:
		case QSE_AWK_NDE_LCL:
		case QSE_AWK_NDE_ARG:
		case QSE_AWK_NDE_GBLIDX:
		case QSE_AWK_NDE_LCLIDX:
		case QSE_AWK_NDE_ARGIDX:
			return run_delete_unnamed (rtx, var);

		default:
			QSE_ASSERTX (
				!"should never happen - wrong target for delete",
				"the delete statement cannot be called with other nodes than the variables such as a named variable, a named indexed variable, etc");

			SETERR_LOC (rtx, QSE_AWK_EBADARG, &var->loc);
			return -1;
	}

}

static int reset_variable (qse_awk_rtx_t* rtx, qse_awk_nde_var_t* var)
{
	switch (var->type)
	{
		case QSE_AWK_NDE_NAMED:
			QSE_ASSERTX (
				var->type == QSE_AWK_NDE_NAMED && var->idx == QSE_NULL,
				"if a named variable has an index part, something is definitely wrong");

			/* a named variable can be reset if removed from a internal map 
			   to manage it */
			qse_htb_delete (rtx->named, var->id.name.ptr, var->id.name.len);
			return 0;

		case QSE_AWK_NDE_GBL:
		case QSE_AWK_NDE_LCL:
		case QSE_AWK_NDE_ARG:
		{
			qse_awk_val_t* val;

			switch (var->type)
			{
				case QSE_AWK_NDE_GBL:
					val = RTX_STACK_GBL(rtx,var->id.idxa);
					break;

				case QSE_AWK_NDE_LCL:
					val = RTX_STACK_LCL(rtx,var->id.idxa);
					break;

				case QSE_AWK_NDE_ARG:
					val = RTX_STACK_ARG(rtx,var->id.idxa);
					break;
			}

			QSE_ASSERT (val != QSE_NULL);

			if (val->type != QSE_AWK_VAL_NIL)
			{
				qse_awk_rtx_refdownval (rtx, val);
				switch (var->type)
				{
					case QSE_AWK_NDE_GBL:
						RTX_STACK_GBL(rtx,var->id.idxa) = qse_awk_val_nil;
						break;
					
					case QSE_AWK_NDE_LCL:
						RTX_STACK_LCL(rtx,var->id.idxa) = qse_awk_val_nil;
						break;

					case QSE_AWK_NDE_ARG:
						RTX_STACK_ARG(rtx,var->id.idxa) = qse_awk_val_nil;
						break;
				}
			}
			return 0;
		}

		default:
			QSE_ASSERTX (
				!"should never happen - wrong target for reset",
				"the reset statement can only be called with plain variables");
			SETERR_LOC (rtx, QSE_AWK_EBADARG, &var->loc);
			return -1;
	}
}

static int run_reset (qse_awk_rtx_t* rtx, qse_awk_nde_reset_t* nde)
{
	return reset_variable (rtx, (qse_awk_nde_var_t*)nde->var);
}

static int run_print (qse_awk_rtx_t* rtx, qse_awk_nde_print_t* nde)
{
	qse_char_t* out = QSE_NULL;
	const qse_char_t* dst;
	qse_awk_val_t* v;
	int n, xret = 0;

	QSE_ASSERT (
		(nde->out_type == QSE_AWK_OUT_PIPE && nde->out != QSE_NULL) ||
		(nde->out_type == QSE_AWK_OUT_RWPIPE && nde->out != QSE_NULL) ||
		(nde->out_type == QSE_AWK_OUT_FILE && nde->out != QSE_NULL) ||
		(nde->out_type == QSE_AWK_OUT_APFILE && nde->out != QSE_NULL) ||
		(nde->out_type == QSE_AWK_OUT_CONSOLE && nde->out == QSE_NULL));

	/* check if destination has been specified. */
	if (nde->out != QSE_NULL)
	{
		qse_awk_rtx_valtostr_out_t vsout;
		qse_size_t len;

		/* if so, resolve the destination name */
		v = eval_expression (rtx, nde->out);
		if (v == QSE_NULL) return -1;

		qse_awk_rtx_refupval (rtx, v);

		vsout.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
		if (qse_awk_rtx_valtostr (rtx, v, &vsout) <= -1)
		{
			qse_awk_rtx_refdownval (rtx, v);
			ADJERR_LOC (rtx, &nde->loc);
			return -1;
		}
		out = vsout.u.cpldup.ptr;
		len = vsout.u.cpldup.len;

		qse_awk_rtx_refdownval (rtx, v);

		if (len <= 0) 
		{
			/* the destination name is empty */
			QSE_AWK_FREE (rtx->awk, out);
			SETERR_LOC (rtx, QSE_AWK_EIONMEM, &nde->loc);
			return -1;
		}

		/* it needs to check if the destination name contains
		 * any invalid characters to the underlying system */
		while (len > 0)
		{
			if (out[--len] == QSE_T('\0'))
			{
				/* provide length up to one character before 
				 * the first null not to contains a null
				 * in an error message */
				SETERR_ARG_LOC (
					rtx, QSE_AWK_EIONMNL, 
					out, qse_strlen(out), &nde->loc);

				/* if so, it skips writing */
				QSE_AWK_FREE (rtx->awk, out);
				return -1;
			}
		}
	}

	/* transforms the destination to suit the usage with io */
	dst = (out == QSE_NULL)? QSE_T(""): out;

	/* check if print is followed by any arguments */
	if (nde->args == QSE_NULL)
	{
		/* if it doesn't have any arguments, print the entire 
		 * input record */
		n = qse_awk_rtx_writeio_str (
			rtx, nde->out_type, dst,
			QSE_STR_PTR(&rtx->inrec.line),
			QSE_STR_LEN(&rtx->inrec.line));
		if (n <= -1 /*&& rtx->errinf.num != QSE_AWK_EIOIMPL*/)
		{
			if (rtx->awk->opt.trait & QSE_AWK_TOLERANT)
			{
				xret = PRINT_IOERR;
			}
			else
			{
				if (out) QSE_AWK_FREE (rtx->awk, out);
				ADJERR_LOC (rtx, &nde->loc);
				return -1;
			}
		}
	}
	else
	{
		/* if it has any arguments, print the arguments separated by
		 * the value OFS */
		qse_awk_nde_t* head, * np;

		if (nde->args->type == QSE_AWK_NDE_GRP)
		{
			/* parenthesized print */
			QSE_ASSERT (nde->args->next == QSE_NULL);
			head = ((qse_awk_nde_grp_t*)nde->args)->body;
		}
		else head = nde->args;

		for (np = head; np != QSE_NULL; np = np->next)
		{
			if (np != head)
			{
				n = qse_awk_rtx_writeio_str (
					rtx, nde->out_type, dst, 
					rtx->gbl.ofs.ptr, 
					rtx->gbl.ofs.len);
				if (n <= -1 /*&& rtx->errinf.num != QSE_AWK_EIOIMPL*/) 
				{
					if (rtx->awk->opt.trait & QSE_AWK_TOLERANT)
					{
						xret = PRINT_IOERR;
					}
					else
					{
						if (out) QSE_AWK_FREE (rtx->awk, out);
						ADJERR_LOC (rtx, &nde->loc);
						return -1;
					}
				}
			}

			v = eval_expression (rtx, np);
			if (v == QSE_NULL) 
			{
				if (out) QSE_AWK_FREE (rtx->awk, out);
				return -1;
			}
			qse_awk_rtx_refupval (rtx, v);

			n = qse_awk_rtx_writeio_val (
				rtx, nde->out_type, dst, v);
			if (n <= -1 /*&& rtx->errinf.num != QSE_AWK_EIOIMPL*/) 
			{
				if (rtx->awk->opt.trait & QSE_AWK_TOLERANT)
				{
					xret = PRINT_IOERR;
				}
				else
				{
					if (out) QSE_AWK_FREE (rtx->awk, out);
					qse_awk_rtx_refdownval (rtx, v);
					ADJERR_LOC (rtx, &nde->loc);
					return -1;
				}
			}

			qse_awk_rtx_refdownval (rtx, v);
		}
	}

	/* print the value ORS to terminate the operation */
	n = qse_awk_rtx_writeio_str (
		rtx, nde->out_type, dst, 
		rtx->gbl.ors.ptr, rtx->gbl.ors.len);
	if (n <= -1 /*&& rtx->errinf.num != QSE_AWK_EIOIMPL*/)
	{
		if (rtx->awk->opt.trait & QSE_AWK_TOLERANT)
		{
			xret = PRINT_IOERR;
		}
		else
		{
			if (out) QSE_AWK_FREE (rtx->awk, out);
			ADJERR_LOC (rtx, &nde->loc);
			return -1;
		}
	}

	/* unlike printf, flushio() is not needed here as print 
	 * inserts <NL> that triggers auto-flush */

	if (out) QSE_AWK_FREE (rtx->awk, out);

/*skip_write:*/
	return xret;
}

static int run_printf (qse_awk_rtx_t* rtx, qse_awk_nde_print_t* nde)
{
	qse_char_t* out = QSE_NULL;
	const qse_char_t* dst;
	qse_awk_val_t* v;
	qse_awk_nde_t* head;
	int n, xret = 0;

	QSE_ASSERT (
		(nde->out_type == QSE_AWK_OUT_PIPE && nde->out != QSE_NULL) ||
		(nde->out_type == QSE_AWK_OUT_RWPIPE && nde->out != QSE_NULL) ||
		(nde->out_type == QSE_AWK_OUT_FILE && nde->out != QSE_NULL) ||
		(nde->out_type == QSE_AWK_OUT_APFILE && nde->out != QSE_NULL) ||
		(nde->out_type == QSE_AWK_OUT_CONSOLE && nde->out == QSE_NULL));

	if (nde->out != QSE_NULL)
	{
		qse_size_t len;
		qse_awk_rtx_valtostr_out_t vsout;

		v = eval_expression (rtx, nde->out);
		if (v == QSE_NULL) return -1;

		qse_awk_rtx_refupval (rtx, v);

		vsout.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
		if (qse_awk_rtx_valtostr (rtx, v, &vsout) <= -1)
		{
			qse_awk_rtx_refdownval (rtx, v);
			ADJERR_LOC (rtx, &nde->loc);
			return -1;
		}
		out = vsout.u.cpldup.ptr;
		len = vsout.u.cpldup.len;

		qse_awk_rtx_refdownval (rtx, v);

		if (len <= 0) 
		{
			/* the output destination name is empty. */
			QSE_AWK_FREE (rtx->awk, out);
			SETERR_LOC (rtx, QSE_AWK_EIONMEM, &nde->loc);
			return -1;
		}

		while (len > 0)
		{
			if (out[--len] == QSE_T('\0'))
			{
				/* provide length up to one character before 
				 * the first null not to contains a null
				 * in an error message */
				SETERR_ARG_LOC (
					rtx, QSE_AWK_EIONMNL,
					out, qse_strlen(out), &nde->loc);

				/* the output destination name contains a null 
				 * character. */
				QSE_AWK_FREE (rtx->awk, out);
				return -1;
			}
		}
	}

	dst = (out == QSE_NULL)? QSE_T(""): out;

	QSE_ASSERTX (nde->args != QSE_NULL, 
		"valid printf statement should have at least one argument. the parser must ensure this.");

	if (nde->args->type == QSE_AWK_NDE_GRP)
	{
		/* parenthesized print */
		QSE_ASSERT (nde->args->next == QSE_NULL);
		head = ((qse_awk_nde_grp_t*)nde->args)->body;
	}
	else head = nde->args;

	QSE_ASSERTX (head != QSE_NULL,
		"valid printf statement should have at least one argument. the parser must ensure this.");

	v = eval_expression (rtx, head);
	if (v == QSE_NULL) 
	{
		if (out != QSE_NULL) QSE_AWK_FREE (rtx->awk, out);
		return -1;
	}

	qse_awk_rtx_refupval (rtx, v);
	if (v->type != QSE_AWK_VAL_STR)
	{
		/* the remaining arguments are ignored as the format cannot 
		 * contain any % characters */
		n = qse_awk_rtx_writeio_val (rtx, nde->out_type, dst, v);
		if (n <= -1 /*&& rtx->errinf.num != QSE_AWK_EIOIMPL*/)
		{
			if (rtx->awk->opt.trait & QSE_AWK_TOLERANT)
			{
				xret = PRINT_IOERR;
			}
			else
			{
				if (out != QSE_NULL) QSE_AWK_FREE (rtx->awk, out);
				qse_awk_rtx_refdownval (rtx, v);
				ADJERR_LOC (rtx, &nde->loc);
				return -1;
			}
		}
	}
	else
	{
		/* perform the formatted output */
		n = output_formatted (
			rtx, nde->out_type, dst,
			((qse_awk_val_str_t*)v)->val.ptr,
			((qse_awk_val_str_t*)v)->val.len,
			head->next);
		if (n <= -1)
		{
			if (n == PRINT_IOERR) xret = n;
			else
			{
				if (out != QSE_NULL) QSE_AWK_FREE (rtx->awk, out);
				qse_awk_rtx_refdownval (rtx, v);
				ADJERR_LOC (rtx, &nde->loc);
				return -1;
			}
		}
	}
	qse_awk_rtx_refdownval (rtx, v);


/*skip_write:*/
	if (qse_awk_rtx_flushio (rtx, nde->out_type, dst) <= -1)
	{
		if (rtx->awk->opt.trait & QSE_AWK_TOLERANT)
		{
			xret = PRINT_IOERR;
		}
		else
		{
			if (out != QSE_NULL) QSE_AWK_FREE (rtx->awk, out);
			return -1;
		}
	}

	if (out != QSE_NULL) QSE_AWK_FREE (rtx->awk, out);
	return xret;
}

static int output_formatted (
	qse_awk_rtx_t* rtx, int out_type, const qse_char_t* dst, 
	const qse_char_t* fmt, qse_size_t fmt_len, qse_awk_nde_t* args)
{
	qse_char_t* ptr;
	qse_size_t len;
	int n;

	ptr = qse_awk_rtx_format (
		rtx, QSE_NULL, QSE_NULL, fmt, fmt_len, 0, args, &len);
	if (ptr == QSE_NULL) return -1;

	n = qse_awk_rtx_writeio_str (rtx, out_type, dst, ptr, len);
	if (n <= -1 /*&& rtx->errinf.num != QSE_AWK_EIOIMPL*/) 
	{
		if (rtx->awk->opt.trait & QSE_AWK_TOLERANT)
		{
			return PRINT_IOERR;
		}
		else
		{
			return -1;
		}
	}

	return 0;
}

static qse_awk_val_t* eval_expression (qse_awk_rtx_t* rtx, qse_awk_nde_t* nde)
{
	qse_awk_val_t* v;
	int n;

#if 0
	if (rtx->exit_level >= EXIT_GLOBAL) 
	{
		/* returns QSE_NULL as if an error occurred but
		 * clears the error number. run_main will 
		 * detect this condition and treat it as a 
		 * non-error condition.*/
		rtx->errinf.num = QSE_AWK_ENOERR;
		return QSE_NULL;
	}
#endif

	v = eval_expression0 (rtx, nde);
	if (v == QSE_NULL) return QSE_NULL;

	if (v->type == QSE_AWK_VAL_REX)
	{
		qse_cstr_t vs;
		int opt = 0;

		/* special case where a regular expression is used in
		 * without any match operators:
		 *    print /abc/;
		 * perform match against $0.
		 */
		qse_awk_rtx_refupval (rtx, v);

		if (rtx->inrec.d0->type == QSE_AWK_VAL_NIL)
		{
			/* the record has never been read. 
			 * probably, this function has been triggered
			 * by the statements in the BEGIN block */
			vs.ptr = QSE_T("");
			vs.len = 0;
		}
		else
		{
			QSE_ASSERTX (
				rtx->inrec.d0->type == QSE_AWK_VAL_STR,
				"the internal value representing $0 should always be of the string type once it has been set/updated. it is nil initially.");

			vs.ptr = ((qse_awk_val_str_t*)rtx->inrec.d0)->val.ptr;
			vs.len = ((qse_awk_val_str_t*)rtx->inrec.d0)->val.len;
		}	

		n = qse_awk_rtx_matchrex (rtx, v, &vs, &vs, QSE_NULL);
		if (n <= -1)
		{
			ADJERR_LOC (rtx, &nde->loc);
			qse_awk_rtx_refdownval (rtx, v);
			return QSE_NULL;
		}
		qse_awk_rtx_refdownval (rtx, v);

		v = qse_awk_rtx_makeintval (rtx, (n != 0));
		if (v == QSE_NULL) 
		{
			/* adjust error line */
			ADJERR_LOC (rtx, &nde->loc);
			return QSE_NULL;
		}
	}

	return v;
}

static qse_awk_val_t* eval_expression0 (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	static eval_expr_t __evaluator[] =
	{
		/* the order of functions here should match the order
		 * of node types(qse_awk_nde_type_t) declared in qse/awk/awk.h */
		eval_group,
		eval_assignment,
		eval_binary,
		eval_unary,
		eval_incpre,
		eval_incpst,
		eval_cnd,
		eval_fnc,
		eval_fun,
		eval_int,
		eval_real,
		eval_str,
		eval_rex,
		eval_named,
		eval_gbl,
		eval_lcl,
		eval_arg,
		eval_namedidx,
		eval_gblidx,
		eval_lclidx,
		eval_argidx,
		eval_pos,
		eval_getline,
		eval_print,
		eval_printf
	};

	qse_awk_val_t* v;

	QSE_ASSERT (nde->type >= QSE_AWK_NDE_GRP &&
		(nde->type - QSE_AWK_NDE_GRP) < QSE_COUNTOF(__evaluator));

	v = __evaluator[nde->type-QSE_AWK_NDE_GRP] (run, nde);

	if (v != QSE_NULL && run->exit_level >= EXIT_GLOBAL)
	{
		qse_awk_rtx_refupval (run, v);	
		qse_awk_rtx_refdownval (run, v);

		/* returns QSE_NULL as if an error occurred but
		 * clears the error number. run_main will 
		 * detect this condition and treat it as a 
		 * non-error condition.*/
		run->errinf.num = QSE_AWK_ENOERR;
		return QSE_NULL;
	}

	return v;
}

static qse_awk_val_t* eval_group (qse_awk_rtx_t* rtx, qse_awk_nde_t* nde)
{
#if 0
	/* eval_binop_in evaluates the QSE_AWK_NDE_GRP specially.
	 * so this function should never be reached. */
	QSE_ASSERT (!"should never happen - NDE_GRP only for in");
	SETERR_LOC (rtx, QSE_AWK_EINTERN, &nde->loc);
	return QSE_NULL;
#endif

	/* a group can be evauluated in a normal context
	 * if a program is parsed with QSE_AWK_TOLERANT on. 
	 * before the introduction of this option, the grouped
	 * expression was valid only coerced with the 'in' 
	 * operator. 
	 * */

	/* when a group is evaluated in a normal context,
	 * we return the last expression as a value. */

	qse_awk_val_t* val;
	qse_awk_nde_t* np;

	np = ((qse_awk_nde_grp_t*)nde)->body;

	QSE_ASSERT (np != QSE_NULL);

loop:
	val = eval_expression (rtx, np);
	if (val == QSE_NULL) return QSE_NULL;

	np = np->next;
	if (np)
	{
		qse_awk_rtx_refupval (rtx, val);
		qse_awk_rtx_refdownval (rtx, val);
		goto loop;
	}

	return val;
}

static qse_awk_val_t* eval_assignment (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_val_t* val, * ret;
	qse_awk_nde_ass_t* ass = (qse_awk_nde_ass_t*)nde;

	QSE_ASSERT (ass->left != QSE_NULL);
	QSE_ASSERT (ass->right != QSE_NULL);

	QSE_ASSERT (ass->right->next == QSE_NULL);
	val = eval_expression (run, ass->right);
	if (val == QSE_NULL) return QSE_NULL;

	qse_awk_rtx_refupval (run, val);

	if (ass->opcode != QSE_AWK_ASSOP_NONE)
	{
		qse_awk_val_t* val2, * tmp;
		static binop_func_t binop_func[] =
		{
			/* this table must match qse_awk_assop_type_t in run.h */
			QSE_NULL, /* QSE_AWK_ASSOP_NONE */
			eval_binop_plus,
			eval_binop_minus,
			eval_binop_mul,
			eval_binop_div,
			eval_binop_idiv,
			eval_binop_mod,
			eval_binop_exp,
			eval_binop_concat,
			eval_binop_rshift,
			eval_binop_lshift,
			eval_binop_band,
			eval_binop_bxor,
			eval_binop_bor
		};

		QSE_ASSERT (ass->left->next == QSE_NULL);
		val2 = eval_expression (run, ass->left);
		if (val2 == QSE_NULL)
		{
			qse_awk_rtx_refdownval (run, val);
			return QSE_NULL;
		}

		qse_awk_rtx_refupval (run, val2);

		QSE_ASSERT (ass->opcode >= 0);
		QSE_ASSERT (ass->opcode < QSE_COUNTOF(binop_func));
		QSE_ASSERT (binop_func[ass->opcode] != QSE_NULL);

		tmp = binop_func[ass->opcode] (run, val2, val);
		if (tmp == QSE_NULL)
		{
			qse_awk_rtx_refdownval (run, val2);
			qse_awk_rtx_refdownval (run, val);
			return QSE_NULL;
		}

		qse_awk_rtx_refdownval (run, val2);
		qse_awk_rtx_refdownval (run, val);

		val = tmp;
		qse_awk_rtx_refupval (run, val);
	}

	ret = do_assignment (run, ass->left, val);
	qse_awk_rtx_refdownval (run, val);

	return ret;
}

static qse_awk_val_t* do_assignment (
	qse_awk_rtx_t* rtx, qse_awk_nde_t* var, qse_awk_val_t* val)
{
	qse_awk_val_t* ret;
	qse_awk_errnum_t errnum;

	switch (var->type)
	{
		case QSE_AWK_NDE_NAMED:
		case QSE_AWK_NDE_GBL:
		case QSE_AWK_NDE_LCL:
		case QSE_AWK_NDE_ARG:
			ret = do_assignment_nonidx (rtx, (qse_awk_nde_var_t*)var, val);
			break;

		case QSE_AWK_NDE_NAMEDIDX:
		case QSE_AWK_NDE_GBLIDX:
		case QSE_AWK_NDE_LCLIDX:
		case QSE_AWK_NDE_ARGIDX:
			if (val->type == QSE_AWK_VAL_MAP)
			{
				/* a map cannot become a member of a map */
				errnum = QSE_AWK_EMAPTOIDX;
				goto exit_on_error;
			}

			ret = do_assignment_idx (rtx, (qse_awk_nde_var_t*)var, val);
			break;

		case QSE_AWK_NDE_POS:
			if (val->type == QSE_AWK_VAL_MAP)
			{
				/* a map cannot be assigned to a positional */
				errnum = QSE_AWK_EMAPTOPOS;
				goto exit_on_error;
			}
	
			ret = do_assignment_pos (rtx, (qse_awk_nde_pos_t*)var, val);
			break;

		default:
			QSE_ASSERT (!"should never happen - invalid variable type");
			errnum = QSE_AWK_EINTERN;
			goto exit_on_error;
	}

	return ret;

exit_on_error:
	SETERR_LOC (rtx, errnum, &var->loc);
	return QSE_NULL;
}

static qse_awk_val_t* do_assignment_nonidx (
	qse_awk_rtx_t* run, qse_awk_nde_var_t* var, qse_awk_val_t* val)
{
	QSE_ASSERT (
		var->type == QSE_AWK_NDE_NAMED ||
		var->type == QSE_AWK_NDE_GBL ||
		var->type == QSE_AWK_NDE_LCL ||
		var->type == QSE_AWK_NDE_ARG
	);

	QSE_ASSERT (var->idx == QSE_NULL);

	switch (var->type)
	{
		case QSE_AWK_NDE_NAMED:
		{
			qse_htb_pair_t* pair;

			pair = qse_htb_search (run->named, var->id.name.ptr, var->id.name.len);

			if (!(run->awk->opt.trait & QSE_AWK_FLEXMAP))
			{
				if (pair && ((qse_awk_val_t*)QSE_HTB_VPTR(pair))->type == QSE_AWK_VAL_MAP)
				{
					/* old value is a map - it can only be accessed through indexing. */
					qse_awk_errnum_t errnum;
					errnum = (val->type == QSE_AWK_VAL_MAP)? 
						QSE_AWK_ENMAPTOMAP: QSE_AWK_ENMAPTOSCALAR;
					SETERR_ARGX_LOC (run, errnum, xstr_to_cstr(&var->id.name), &var->loc);
					return QSE_NULL;
				}
				else if (val->type == QSE_AWK_VAL_MAP)
				{
					/* old value is not a map but a new value is a map.
					 * a map cannot be assigned to a variable if FLEXMAP is off. */
					SETERR_ARGX_LOC (run, QSE_AWK_EMAPTONVAR, xstr_to_cstr(&var->id.name), &var->loc);
					return QSE_NULL;
				}
			}


			if (qse_htb_upsert (run->named, 
				var->id.name.ptr, var->id.name.len, val, 0) == QSE_NULL)
			{
				SETERR_LOC (run, QSE_AWK_ENOMEM, &var->loc);
				return QSE_NULL;
			}

			qse_awk_rtx_refupval (run, val);
			break;
		}

		case QSE_AWK_NDE_GBL:
		{
			if (set_global (run, var->id.idxa, var, val, 1) == -1) 
			{
				ADJERR_LOC (run, &var->loc);
				return QSE_NULL;
			}
			break;
		}

		case QSE_AWK_NDE_LCL:
		{
			qse_awk_val_t* old = RTX_STACK_LCL(run,var->id.idxa);

			if (!(run->awk->opt.trait & QSE_AWK_FLEXMAP))
			{
				if (old->type == QSE_AWK_VAL_MAP)
				{
					/* old value is a map - it can only be accessed through indexing. */
					qse_awk_errnum_t errnum;
					errnum = (val->type == QSE_AWK_VAL_MAP)? 
						QSE_AWK_ENMAPTOMAP: QSE_AWK_ENMAPTOSCALAR;
					SETERR_ARGX_LOC (run, errnum, xstr_to_cstr(&var->id.name), &var->loc);
					return QSE_NULL;
				}
				else if (val->type == QSE_AWK_VAL_MAP)
				{
					/* old value is not a map but a new value is a map.
					 * a map cannot be assigned to a variable if FLEXMAP is off. */
					SETERR_ARGX_LOC (run, QSE_AWK_EMAPTONVAR, xstr_to_cstr(&var->id.name), &var->loc);
					return QSE_NULL;
				}
			}

			qse_awk_rtx_refdownval (run, old);
			RTX_STACK_LCL(run,var->id.idxa) = val;
			qse_awk_rtx_refupval (run, val);
			break;
		}

		case QSE_AWK_NDE_ARG:
		{
			qse_awk_val_t* old = RTX_STACK_ARG(run,var->id.idxa);

			if (!(run->awk->opt.trait & QSE_AWK_FLEXMAP))
			{
				if (old->type == QSE_AWK_VAL_MAP)
				{
					/* old value is a map - it can only be accessed through indexing. */
					qse_awk_errnum_t errnum;
					errnum = (val->type == QSE_AWK_VAL_MAP)? 
						QSE_AWK_ENMAPTOMAP: QSE_AWK_ENMAPTOSCALAR;
					SETERR_ARGX_LOC (run, errnum, xstr_to_cstr(&var->id.name), &var->loc);
					return QSE_NULL;
				}
				else if (val->type == QSE_AWK_VAL_MAP)
				{
					/* old value is not a map but a new value is a map.
					 * a map cannot be assigned to a variable if FLEXMAP is off. */
					SETERR_ARGX_LOC (run, QSE_AWK_EMAPTONVAR, xstr_to_cstr(&var->id.name), &var->loc);
					return QSE_NULL;
				}
			}
	
			qse_awk_rtx_refdownval (run, old);
			RTX_STACK_ARG(run,var->id.idxa) = val;
			qse_awk_rtx_refupval (run, val);
			break;
		}

	}

	return val;
}

static qse_awk_val_t* do_assignment_idx (
	qse_awk_rtx_t* rtx, qse_awk_nde_var_t* var, qse_awk_val_t* val)
{
	qse_awk_val_map_t* map;
	qse_char_t* str;
	qse_size_t len;
	qse_char_t idxbuf[IDXBUFSIZE];

	QSE_ASSERT (
		(var->type == QSE_AWK_NDE_NAMEDIDX ||
		 var->type == QSE_AWK_NDE_GBLIDX ||
		 var->type == QSE_AWK_NDE_LCLIDX ||
		 var->type == QSE_AWK_NDE_ARGIDX) && var->idx != QSE_NULL);
	QSE_ASSERT (val->type != QSE_AWK_VAL_MAP);

retry:
	switch (var->type)
	{
		case QSE_AWK_NDE_NAMEDIDX:
		{
			qse_htb_pair_t* pair;
			pair = qse_htb_search (
				rtx->named, var->id.name.ptr, var->id.name.len);
			map = (pair == QSE_NULL)? 
				(qse_awk_val_map_t*)qse_awk_val_nil: 
				(qse_awk_val_map_t*)QSE_HTB_VPTR(pair);
			break;
		}

		case QSE_AWK_NDE_GBLIDX:
			map = (qse_awk_val_map_t*)RTX_STACK_GBL(rtx,var->id.idxa);
			break;

		case QSE_AWK_NDE_LCLIDX:
			map = (qse_awk_val_map_t*)RTX_STACK_LCL(rtx,var->id.idxa);
			break;

		default:  /* QSE_AWK_NDE_ARGIDX */
			map = (qse_awk_val_map_t*)RTX_STACK_ARG(rtx,var->id.idxa);
			break;
	} 

	if (map->type == QSE_AWK_VAL_NIL)
	{
		qse_awk_val_t* tmp;

		/* the map is not initialized yet */
		tmp = qse_awk_rtx_makemapval (rtx);
		if (tmp == QSE_NULL) 
		{
			ADJERR_LOC (rtx, &var->loc);
			return QSE_NULL;
		}

		switch (var->type)
		{
			case QSE_AWK_NDE_NAMEDIDX:
			{
				/* doesn't have to decrease the reference count 
				 * of the previous value here as it is done by 
				 * qse_htb_upsert */
				qse_awk_rtx_refupval (rtx, tmp);
				if (qse_htb_upsert (rtx->named, var->id.name.ptr, var->id.name.len, tmp, 0) == QSE_NULL)
				{
					qse_awk_rtx_refdownval (rtx, tmp);
					SETERR_LOC (rtx, QSE_AWK_ENOMEM, &var->loc);
					return QSE_NULL;
				}

				break;
			}

			case QSE_AWK_NDE_GBLIDX:
			{
				int x;

				qse_awk_rtx_refupval (rtx, tmp);
				x = qse_awk_rtx_setgbl (rtx, (int)var->id.idxa, tmp);
				qse_awk_rtx_refdownval (rtx, tmp);
				if (x <= -1)
				{
					ADJERR_LOC (rtx, &var->loc);
					return QSE_NULL;
				}
				break;
			}

			case QSE_AWK_NDE_LCLIDX:
				qse_awk_rtx_refdownval (rtx, (qse_awk_val_t*)map);
				RTX_STACK_LCL(rtx,var->id.idxa) = tmp;
				qse_awk_rtx_refupval (rtx, tmp);
				break;
			
			default: /* QSE_AWK_NDE_ARGIDX */
				qse_awk_rtx_refdownval (rtx, (qse_awk_val_t*)map);
				RTX_STACK_ARG(rtx,var->id.idxa) = tmp;
				qse_awk_rtx_refupval (rtx, tmp);
				break;
		}

		map = (qse_awk_val_map_t*)tmp;
	}
	else if (map->type != QSE_AWK_VAL_MAP)
	{
		if (rtx->awk->opt.trait & QSE_AWK_FLEXMAP)
		{
			/* if FLEXMAP is on, you can switch a scalar value to a map */
			qse_awk_nde_var_t fake;

			/* create a fake non-indexed variable node */
			fake = *var;
			/* NOTE: type conversion by decrementing by 4 is 
			 *       dependent on the qse_awk_nde_type_t 
			 *       enumerators defined in <qse/awk/awk.h> */
			fake.type = var->type - 4; 
			fake.idx = QSE_NULL;

			reset_variable (rtx, &fake);
			goto retry;
		}
		else
		{
			/* you can't manipulate a variable pointing to
			 * a scalar value with an index if FLEXMAP is off. */
			SETERR_LOC (rtx, QSE_AWK_ESCALARTOMAP, &var->loc);
			return QSE_NULL;
		}
	}

	len = QSE_COUNTOF(idxbuf);
	str = idxnde_to_str (rtx, var->idx, idxbuf, &len);
	if (str == QSE_NULL) return QSE_NULL;

#ifdef DEBUG_RUN
	qse_errputstrf (QSE_T("**** index str=>%s, map->ref=%d, map->type=%d\n"), 
		str, (int)map->ref, (int)map->type);
#endif

	if (qse_htb_upsert (map->map, str, len, val, 0) == QSE_NULL)
	{
		if (str != idxbuf) QSE_AWK_FREE (rtx->awk, str);
		SETERR_LOC (rtx, QSE_AWK_ENOMEM, &var->loc);
		return QSE_NULL;
	}

	if (str != idxbuf) QSE_AWK_FREE (rtx->awk, str);
	qse_awk_rtx_refupval (rtx, val);
	return val;
}

static qse_awk_val_t* do_assignment_pos (
	qse_awk_rtx_t* run, qse_awk_nde_pos_t* pos, qse_awk_val_t* val)
{
	qse_awk_val_t* v;
	qse_awk_int_t lv;
	qse_xstr_t str;
	int n;

	v = eval_expression (run, pos->val);
	if (v == QSE_NULL) return QSE_NULL;

	qse_awk_rtx_refupval (run, v);
	n = qse_awk_rtx_valtoint (run, v, &lv);
	qse_awk_rtx_refdownval (run, v);

	if (n <= -1) 
	{
		SETERR_LOC (run, QSE_AWK_EPOSIDX, &pos->loc);
		return QSE_NULL;
	}

	if (!IS_VALID_POSIDX(lv)) 
	{
		SETERR_LOC (run, QSE_AWK_EPOSIDX, &pos->loc);
		return QSE_NULL;
	}

	if (val->type == QSE_AWK_VAL_STR)
	{
		str = ((qse_awk_val_str_t*)val)->val;
	}
	else
	{
		qse_awk_rtx_valtostr_out_t out;

		out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
		if (qse_awk_rtx_valtostr (run, val, &out) <= -1)
		{
			ADJERR_LOC (run, &pos->loc);
			return QSE_NULL;
		}

		str = out.u.cpldup;
	}
	
	n = qse_awk_rtx_setrec (run, (qse_size_t)lv, &str);

	if (val->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, str.ptr);

	if (n <= -1) return QSE_NULL;
	return (lv == 0)? run->inrec.d0: run->inrec.flds[lv-1].val;
}

static qse_awk_val_t* eval_binary (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	static binop_func_t binop_func[] =
	{
		/* the order of the functions should be inline with
		 * the operator declaration in run.h */

		QSE_NULL, /* eval_binop_lor */
		QSE_NULL, /* eval_binop_land */
		QSE_NULL, /* eval_binop_in */

		eval_binop_bor,
		eval_binop_bxor,
		eval_binop_band,

		eval_binop_teq,
		eval_binop_tne,
		eval_binop_eq,
		eval_binop_ne,
		eval_binop_gt,
		eval_binop_ge,
		eval_binop_lt,
		eval_binop_le,

		eval_binop_lshift,
		eval_binop_rshift,
		
		eval_binop_plus,
		eval_binop_minus,
		eval_binop_mul,
		eval_binop_div,
		eval_binop_idiv,
		eval_binop_mod,
		eval_binop_exp,

		eval_binop_concat,
		QSE_NULL, /* eval_binop_ma */
		QSE_NULL  /* eval_binop_nm */
	};

	qse_awk_nde_exp_t* exp = (qse_awk_nde_exp_t*)nde;
	qse_awk_val_t* left, * right, * res;

	QSE_ASSERT (exp->type == QSE_AWK_NDE_EXP_BIN);

	if (exp->opcode == QSE_AWK_BINOP_LAND)
	{
		res = eval_binop_land (run, exp->left, exp->right);
	}
	else if (exp->opcode == QSE_AWK_BINOP_LOR)
	{
		res = eval_binop_lor (run, exp->left, exp->right);
	}
	else if (exp->opcode == QSE_AWK_BINOP_IN)
	{
		/* treat the in operator specially */
		res = eval_binop_in (run, exp->left, exp->right);
	}
	else if (exp->opcode == QSE_AWK_BINOP_NM)
	{
		res = eval_binop_nm (run, exp->left, exp->right);
	}
	else if (exp->opcode == QSE_AWK_BINOP_MA)
	{
		res = eval_binop_ma (run, exp->left, exp->right);
	}
	else
	{
		QSE_ASSERT (exp->left->next == QSE_NULL);
		left = eval_expression (run, exp->left);
		if (left == QSE_NULL) return QSE_NULL;

		qse_awk_rtx_refupval (run, left);

		QSE_ASSERT (exp->right->next == QSE_NULL);
		right = eval_expression (run, exp->right);
		if (right == QSE_NULL) 
		{
			qse_awk_rtx_refdownval (run, left);
			return QSE_NULL;
		}

		qse_awk_rtx_refupval (run, right);

		QSE_ASSERT (exp->opcode >= 0 && 
			exp->opcode < QSE_COUNTOF(binop_func));
		QSE_ASSERT (binop_func[exp->opcode] != QSE_NULL);

		res = binop_func[exp->opcode] (run, left, right);
		if (res == QSE_NULL) ADJERR_LOC (run, &nde->loc);

		qse_awk_rtx_refdownval (run, left);
		qse_awk_rtx_refdownval (run, right);
	}

	return res;
}

static qse_awk_val_t* eval_binop_lor (
	qse_awk_rtx_t* run, qse_awk_nde_t* left, qse_awk_nde_t* right)
{
	/*
	qse_awk_val_t* res = QSE_NULL;

	res = qse_awk_rtx_makeintval (
		run, 
		qse_awk_rtx_valtobool(run,left) || 
		qse_awk_rtx_valtobool(run,right)
	);
	if (res == QSE_NULL)
	{
		ADJERR_LOC (run, &left->loc);
		return QSE_NULL;
	}

	return res;
	*/

	/* short-circuit evaluation required special treatment */
	qse_awk_val_t* lv, * rv, * res;

	QSE_ASSERT (left->next == QSE_NULL);
	lv = eval_expression (run, left);
	if (lv == QSE_NULL) return QSE_NULL;

	qse_awk_rtx_refupval (run, lv);
	if (qse_awk_rtx_valtobool (run, lv)) 
	{
		res = qse_awk_val_one;
	}
	else
	{
		QSE_ASSERT (right->next == QSE_NULL);
		rv = eval_expression (run, right);
		if (rv == QSE_NULL)
		{
			qse_awk_rtx_refdownval (run, lv);
			return QSE_NULL;
		}
		qse_awk_rtx_refupval (run, rv);

		res = qse_awk_rtx_valtobool(run,rv)? 
			qse_awk_val_one: qse_awk_val_zero;
		qse_awk_rtx_refdownval (run, rv);
	}

	qse_awk_rtx_refdownval (run, lv);

	return res;
}

static qse_awk_val_t* eval_binop_land (
	qse_awk_rtx_t* run, qse_awk_nde_t* left, qse_awk_nde_t* right)
{
	/*
	qse_awk_val_t* res = QSE_NULL;

	res = qse_awk_rtx_makeintval (
		run, 
		qse_awk_rtx_valtobool(run,left) &&
		qse_awk_rtx_valtobool(run,right)
	);
	if (res == QSE_NULL) 
	{
		ADJERR_LOC (run, &left->loc);
		return QSE_NULL;
	}

	return res;
	*/

	/* short-circuit evaluation required special treatment */
	qse_awk_val_t* lv, * rv, * res;

	QSE_ASSERT (left->next == QSE_NULL);
	lv = eval_expression (run, left);
	if (lv == QSE_NULL) return QSE_NULL;

	qse_awk_rtx_refupval (run, lv);
	if (!qse_awk_rtx_valtobool (run, lv)) 
	{
		res = qse_awk_val_zero;
	}
	else
	{
		QSE_ASSERT (right->next == QSE_NULL);
		rv = eval_expression (run, right);
		if (rv == QSE_NULL)
		{
			qse_awk_rtx_refdownval (run, lv);
			return QSE_NULL;
		}
		qse_awk_rtx_refupval (run, rv);

		res = qse_awk_rtx_valtobool(run,rv)? qse_awk_val_one: qse_awk_val_zero;
		qse_awk_rtx_refdownval (run, rv);
	}

	qse_awk_rtx_refdownval (run, lv);

	return res;
}

static qse_awk_val_t* eval_binop_in (
	qse_awk_rtx_t* run, qse_awk_nde_t* left, qse_awk_nde_t* right)
{
	qse_awk_val_t* rv;
	qse_char_t* str;
	qse_size_t len;
	qse_char_t idxbuf[IDXBUFSIZE];

	if (right->type != QSE_AWK_NDE_GBL &&
	    right->type != QSE_AWK_NDE_LCL &&
	    right->type != QSE_AWK_NDE_ARG &&
	    right->type != QSE_AWK_NDE_NAMED)
	{
		/* the compiler should have handled this case */
		QSE_ASSERT (!"should never happen - it needs a plain variable");
		SETERR_LOC (run, QSE_AWK_EINTERN, &right->loc);
		return QSE_NULL;
	}

	/* evaluate the left-hand side of the operator */
	len = QSE_COUNTOF(idxbuf);
	str = (left->type == QSE_AWK_NDE_GRP)?
		idxnde_to_str (run, ((qse_awk_nde_grp_t*)left)->body, idxbuf, &len):
		idxnde_to_str (run, left, idxbuf, &len);
	if (str == QSE_NULL) return QSE_NULL;

	/* evaluate the right-hand side of the operator */
	QSE_ASSERT (right->next == QSE_NULL);
	rv = eval_expression (run, right);
	if (rv == QSE_NULL) 
	{
		if (str != idxbuf) QSE_AWK_FREE (run->awk, str);
		return QSE_NULL;
	}

	qse_awk_rtx_refupval (run, rv);

	if (rv->type == QSE_AWK_VAL_NIL)
	{
		if (str != idxbuf) QSE_AWK_FREE (run->awk, str);
		qse_awk_rtx_refdownval (run, rv);
		return qse_awk_val_zero;
	}
	else if (rv->type == QSE_AWK_VAL_MAP)
	{
		qse_awk_val_t* res;
		qse_htb_t* map;

		map = ((qse_awk_val_map_t*)rv)->map;
		res = (qse_htb_search (map, str, len) == QSE_NULL)? 
			qse_awk_val_zero: qse_awk_val_one;

		if (str != idxbuf) QSE_AWK_FREE (run->awk, str);
		qse_awk_rtx_refdownval (run, rv);
		return res;
	}

	/* need a map */
	if (str != idxbuf) QSE_AWK_FREE (run->awk, str);
	qse_awk_rtx_refdownval (run, rv);

	SETERR_LOC (run, QSE_AWK_ENOTMAPNILIN, &right->loc);
	return QSE_NULL;
}

static qse_awk_val_t* eval_binop_bor (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	qse_awk_int_t l1, l2;

	if (qse_awk_rtx_valtoint (rtx, left, &l1) <= -1 ||
	    qse_awk_rtx_valtoint (rtx, right, &l2) <= -1)
	{
		SETERR_COD (rtx, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}

	return qse_awk_rtx_makeintval (rtx, l1 | l2);
}

static qse_awk_val_t* eval_binop_bxor (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	qse_awk_int_t l1, l2;

	if (qse_awk_rtx_valtoint (rtx, left, &l1) <= -1 ||
	    qse_awk_rtx_valtoint (rtx, right, &l2) <= -1)
	{
		SETERR_COD (rtx, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}

	return qse_awk_rtx_makeintval (rtx, l1 ^ l2);
}

static qse_awk_val_t* eval_binop_band (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	qse_awk_int_t l1, l2;

	if (qse_awk_rtx_valtoint (rtx, left, &l1) <= -1 ||
	    qse_awk_rtx_valtoint (rtx, right, &l2) <= -1)
	{
		SETERR_COD (rtx, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}

	return qse_awk_rtx_makeintval (rtx, l1 & l2);
}

static int __cmp_nil_nil (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	return 0;
}

static int __cmp_nil_int (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	if (((qse_awk_val_int_t*)right)->val < 0) return 1;
	if (((qse_awk_val_int_t*)right)->val > 0) return -1;
	return 0;
}

static int __cmp_nil_real (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	if (((qse_awk_val_flt_t*)right)->val < 0) return 1;
	if (((qse_awk_val_flt_t*)right)->val > 0) return -1;
	return 0;
}

static int __cmp_nil_str (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	return (((qse_awk_val_str_t*)right)->val.len == 0)? 0: -1;
}

static int __cmp_int_nil (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	if (((qse_awk_val_int_t*)left)->val > 0) return 1;
	if (((qse_awk_val_int_t*)left)->val < 0) return -1;
	return 0;
}

static int __cmp_int_int (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	if (((qse_awk_val_int_t*)left)->val > 
	    ((qse_awk_val_int_t*)right)->val) return 1;
	if (((qse_awk_val_int_t*)left)->val < 
	    ((qse_awk_val_int_t*)right)->val) return -1;
	return 0;
}

static int __cmp_int_real (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	if (((qse_awk_val_int_t*)left)->val > 
	    ((qse_awk_val_flt_t*)right)->val) return 1;
	if (((qse_awk_val_int_t*)left)->val < 
	    ((qse_awk_val_flt_t*)right)->val) return -1;
	return 0;
}

static int __cmp_int_str (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	qse_awk_rtx_valtostr_out_t out;
	int n;

	/* SCO CC doesn't seem to handle right->nstr > 0 properly */
	if (rtx->awk->opt.trait & QSE_AWK_NCMPONSTR || right->nstr /*> 0*/)
	{
		qse_awk_int_t ll;
		qse_awk_flt_t rr;

		n = qse_awk_rtx_strtonum (
			rtx, 1, 
			((qse_awk_val_str_t*)right)->val.ptr,
			((qse_awk_val_str_t*)right)->val.len, 
			&ll, &rr
		);
		if (n == 0)
		{
			/* a numeric integral string */
			return (((qse_awk_val_int_t*)left)->val > ll)? 1:
			       (((qse_awk_val_int_t*)left)->val < ll)? -1: 0;
		}
		else if (n > 0)
		{
			/* a numeric floating-point string */
			return (((qse_awk_val_int_t*)left)->val > rr)? 1:
			       (((qse_awk_val_int_t*)left)->val < rr)? -1: 0;
		}
	}

	out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
	if (qse_awk_rtx_valtostr (rtx, left, &out) <= -1) return CMP_ERROR;

	if (rtx->gbl.ignorecase)
	{
		n = qse_strxncasecmp (
			out.u.cpldup.ptr,
			out.u.cpldup.len,
			((qse_awk_val_str_t*)right)->val.ptr, 
			((qse_awk_val_str_t*)right)->val.len
		);
	}
	else
	{
		n = qse_strxncmp (
			out.u.cpldup.ptr,
			out.u.cpldup.len,
			((qse_awk_val_str_t*)right)->val.ptr, 
			((qse_awk_val_str_t*)right)->val.len
		);
	}

	QSE_AWK_FREE (rtx->awk, out.u.cpldup.ptr);
	return n;
}

static int __cmp_flt_nil (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	if (((qse_awk_val_flt_t*)left)->val > 0) return 1;
	if (((qse_awk_val_flt_t*)left)->val < 0) return -1;
	return 0;
}

static int __cmp_flt_int (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	if (((qse_awk_val_flt_t*)left)->val > 
	    ((qse_awk_val_int_t*)right)->val) return 1;
	if (((qse_awk_val_flt_t*)left)->val < 
	    ((qse_awk_val_int_t*)right)->val) return -1;
	return 0;
}

static int __cmp_flt_real (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	if (((qse_awk_val_flt_t*)left)->val > 
	    ((qse_awk_val_flt_t*)right)->val) return 1;
	if (((qse_awk_val_flt_t*)left)->val < 
	    ((qse_awk_val_flt_t*)right)->val) return -1;
	return 0;
}

static int __cmp_flt_str (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	qse_awk_rtx_valtostr_out_t out;
	int n;

	/* SCO CC doesn't seem to handle right->nstr > 0 properly */
	if (rtx->awk->opt.trait & QSE_AWK_NCMPONSTR || right->nstr /*> 0*/)
	{
		const qse_char_t* end;
		qse_awk_flt_t rr;

		rr = qse_awk_strxtoflt (
			rtx->awk,
			((qse_awk_val_str_t*)right)->val.ptr,
			((qse_awk_val_str_t*)right)->val.len, 
			&end
		);
		if (end == ((qse_awk_val_str_t*)right)->val.ptr + 
		           ((qse_awk_val_str_t*)right)->val.len)
		{
			return (((qse_awk_val_flt_t*)left)->val > rr)? 1:
			       (((qse_awk_val_flt_t*)left)->val < rr)? -1: 0;
		}
	}

	out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
	if (qse_awk_rtx_valtostr (rtx, left, &out) <= -1) return CMP_ERROR;

	if (rtx->gbl.ignorecase)
	{
		n = qse_strxncasecmp (
			out.u.cpldup.ptr,
			out.u.cpldup.len,
			((qse_awk_val_str_t*)right)->val.ptr, 
			((qse_awk_val_str_t*)right)->val.len
		);
	}
	else
	{
		n = qse_strxncmp (
			out.u.cpldup.ptr,
			out.u.cpldup.len,
			((qse_awk_val_str_t*)right)->val.ptr, 
			((qse_awk_val_str_t*)right)->val.len
		);
	}

	QSE_AWK_FREE (rtx->awk, out.u.cpldup.ptr);
	return n;
}

static int __cmp_str_nil (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	return (((qse_awk_val_str_t*)left)->val.len == 0)? 0: 1;
}

static int __cmp_str_int (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	return -__cmp_int_str (run, right, left);
}

static int __cmp_str_real (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	return -__cmp_flt_str (run, right, left);
}

static int __cmp_str_str (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	qse_awk_val_str_t* ls, * rs;

	ls = (qse_awk_val_str_t*)left;
	rs = (qse_awk_val_str_t*)right;

	if (ls->nstr == 0 || rs->nstr == 0)
	{
		/* both are definitely strings */
		return (rtx->gbl.ignorecase)?
			qse_strxncasecmp (ls->val.ptr, ls->val.len, rs->val.ptr, rs->val.len):
			qse_strxncmp (ls->val.ptr, ls->val.len, rs->val.ptr, rs->val.len);
	}

	if (ls->nstr == 1)
	{
		qse_awk_int_t ll;

		ll = qse_awk_strxtoint (
			rtx->awk, ls->val.ptr, ls->val.len, 0, QSE_NULL);

		if (rs->nstr == 1)
		{
			qse_awk_int_t rr;
			
			rr = qse_awk_strxtoint (
				rtx->awk, rs->val.ptr, rs->val.len, 0, QSE_NULL);

			return (ll > rr)? 1:
			       (ll < rr)? -1: 0;
		}
		else 
		{
			qse_awk_flt_t rr;

			QSE_ASSERT (rs->nstr == 2);

			rr = qse_awk_strxtoflt (
				rtx->awk, rs->val.ptr, rs->val.len, QSE_NULL);

			return (ll > rr)? 1:
			       (ll < rr)? -1: 0;
		}
	}
	else
	{
		qse_awk_flt_t ll;

		QSE_ASSERT (ls->nstr == 2);

		ll = qse_awk_strxtoflt (
			rtx->awk, ls->val.ptr, ls->val.len, QSE_NULL);
		
		if (rs->nstr == 1)
		{
			qse_awk_int_t rr;
			
			rr = qse_awk_strxtoint (
				rtx->awk, rs->val.ptr, rs->val.len, 0, QSE_NULL);

			return (ll > rr)? 1:
			       (ll < rr)? -1: 0;
		}
		else 
		{
			qse_awk_flt_t rr;

			QSE_ASSERT (rs->nstr == 2);

			rr = qse_awk_strxtoflt (
				rtx->awk, rs->val.ptr, rs->val.len, QSE_NULL);

			return (ll > rr)? 1:
			       (ll < rr)? -1: 0;
		}
	}
}

static int __cmp_val (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	typedef int (*cmp_val_t) (qse_awk_rtx_t*, qse_awk_val_t*, qse_awk_val_t*);

	static cmp_val_t func[] =
	{
		/* this table must be synchronized with 
		 * the QSE_AWK_VAL_XXX values in awk.h */
		__cmp_nil_nil,  __cmp_nil_int,  __cmp_nil_real,  __cmp_nil_str,
		__cmp_int_nil,  __cmp_int_int,  __cmp_int_real,  __cmp_int_str,
		__cmp_flt_nil,  __cmp_flt_int,  __cmp_flt_real,  __cmp_flt_str,
		__cmp_str_nil,  __cmp_str_int,  __cmp_str_real,  __cmp_str_str,
	};

	if (left->type == QSE_AWK_VAL_MAP || right->type == QSE_AWK_VAL_MAP)
	{
		/* a map can't be compared againt other values */
		SETERR_COD (rtx, QSE_AWK_EOPERAND);
		return CMP_ERROR; 
	}

	QSE_ASSERT (
		left->type >= QSE_AWK_VAL_NIL &&
		left->type <= QSE_AWK_VAL_STR);
	QSE_ASSERT (
		right->type >= QSE_AWK_VAL_NIL &&
		right->type <= QSE_AWK_VAL_STR);

	return func[left->type*4+right->type] (rtx, left, right);
}

static int teq_val (qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n;

	if (left == right) n = 1;
	else if (left->type != right->type) n = 0;
	else
	{
		switch (left->type)
		{
			case QSE_AWK_VAL_NIL:
				n = 1; 
				break;

			case QSE_AWK_VAL_INT:
				n = ((qse_awk_val_int_t*)left)->val == 
				    ((qse_awk_val_int_t*)right)->val;
				break;

			case QSE_AWK_VAL_FLT:
				n = ((qse_awk_val_flt_t*)left)->val == 
				    ((qse_awk_val_flt_t*)right)->val;
				break;

			case QSE_AWK_VAL_STR:
				n = qse_strxncmp (
					((qse_awk_val_str_t*)left)->val.ptr,
					((qse_awk_val_str_t*)left)->val.len,
					((qse_awk_val_str_t*)right)->val.ptr,
					((qse_awk_val_str_t*)right)->val.len) == 0;
				break;

			default:
				/* map-x and map-y are always different regardless of
				 * their contents. however, if they are pointing to the
				 * same map value, it won't reach here but will be 
				 * handled by the first check in this function */
				n = 0;
				break;
		}
	}

	return n;
}

static qse_awk_val_t* eval_binop_teq (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	return teq_val (rtx, left, right)? qse_awk_val_one: qse_awk_val_zero;
}

static qse_awk_val_t* eval_binop_tne (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	return teq_val (rtx, left, right)? qse_awk_val_zero: qse_awk_val_one;
}

static qse_awk_val_t* eval_binop_eq (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n = __cmp_val (rtx, left, right);
	if (n == CMP_ERROR) return QSE_NULL;
	return (n == 0)? qse_awk_val_one: qse_awk_val_zero;
}

static qse_awk_val_t* eval_binop_ne (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n = __cmp_val (rtx, left, right);
	if (n == CMP_ERROR) return QSE_NULL;
	return (n != 0)? qse_awk_val_one: qse_awk_val_zero;
}

static qse_awk_val_t* eval_binop_gt (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n = __cmp_val (rtx, left, right);
	if (n == CMP_ERROR) return QSE_NULL;
	return (n > 0)? qse_awk_val_one: qse_awk_val_zero;
}

static qse_awk_val_t* eval_binop_ge (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n = __cmp_val (rtx, left, right);
	if (n == CMP_ERROR) return QSE_NULL;
	return (n >= 0)? qse_awk_val_one: qse_awk_val_zero;
}

static qse_awk_val_t* eval_binop_lt (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n = __cmp_val (rtx, left, right);
	if (n == CMP_ERROR) return QSE_NULL;
	return (n < 0)? qse_awk_val_one: qse_awk_val_zero;
}

static qse_awk_val_t* eval_binop_le (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n = __cmp_val (rtx, left, right);
	if (n == CMP_ERROR) return QSE_NULL;
	return (n <= 0)? qse_awk_val_one: qse_awk_val_zero;
}

static qse_awk_val_t* eval_binop_lshift (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	qse_awk_int_t l1, l2;

	if (qse_awk_rtx_valtoint (rtx, left, &l1) <= -1 ||
	    qse_awk_rtx_valtoint (rtx, right, &l2) <= -1)
	{
		SETERR_COD (rtx, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}

	return qse_awk_rtx_makeintval (rtx, l1 << l2);
}

static qse_awk_val_t* eval_binop_rshift (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	qse_awk_int_t l1, l2;

	if (qse_awk_rtx_valtoint (rtx, left, &l1) <= -1 ||
	    qse_awk_rtx_valtoint (rtx, right, &l2) <= -1)
	{
		SETERR_COD (rtx, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}

	return qse_awk_rtx_makeintval (rtx, l1 >> l2);
}

static qse_awk_val_t* eval_binop_plus (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n1, n2, n3;
	qse_awk_int_t l1, l2;
	qse_awk_flt_t r1, r2;

	n1 = qse_awk_rtx_valtonum (rtx, left, &l1, &r1);
	n2 = qse_awk_rtx_valtonum (rtx, right, &l2, &r2);

	if (n1 <= -1 || n2 <= -1)
	{
		SETERR_COD (rtx, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}
	/*
	n1  n2    n3
	0   0   = 0
	1   0   = 1
	0   1   = 2
	1   1   = 3
	*/
	n3 = n1 + (n2 << 1);
	QSE_ASSERT (n3 >= 0 && n3 <= 3);

	return (n3 == 0)? qse_awk_rtx_makeintval(rtx,(qse_awk_int_t)l1+(qse_awk_int_t)l2):
	       (n3 == 1)? qse_awk_rtx_makefltval(rtx,(qse_awk_flt_t)r1+(qse_awk_flt_t)l2):
	       (n3 == 2)? qse_awk_rtx_makefltval(rtx,(qse_awk_flt_t)l1+(qse_awk_flt_t)r2):
	                  qse_awk_rtx_makefltval(rtx,(qse_awk_flt_t)r1+(qse_awk_flt_t)r2);
}

static qse_awk_val_t* eval_binop_minus (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n1, n2, n3;
	qse_awk_int_t l1, l2;
	qse_awk_flt_t r1, r2;

	n1 = qse_awk_rtx_valtonum (rtx, left, &l1, &r1);
	n2 = qse_awk_rtx_valtonum (rtx, right, &l2, &r2);

	if (n1 <= -1 || n2 <= -1)
	{
		SETERR_COD (rtx, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}

	n3 = n1 + (n2 << 1);
	QSE_ASSERT (n3 >= 0 && n3 <= 3);
	return (n3 == 0)? qse_awk_rtx_makeintval(rtx,(qse_awk_int_t)l1-(qse_awk_int_t)l2):
	       (n3 == 1)? qse_awk_rtx_makefltval(rtx,(qse_awk_flt_t)r1-(qse_awk_flt_t)l2):
	       (n3 == 2)? qse_awk_rtx_makefltval(rtx,(qse_awk_flt_t)l1-(qse_awk_flt_t)r2):
	                  qse_awk_rtx_makefltval(rtx,(qse_awk_flt_t)r1-(qse_awk_flt_t)r2);
}

static qse_awk_val_t* eval_binop_mul (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n1, n2, n3;
	qse_awk_int_t l1, l2;
	qse_awk_flt_t r1, r2;

	n1 = qse_awk_rtx_valtonum (rtx, left, &l1, &r1);
	n2 = qse_awk_rtx_valtonum (rtx, right, &l2, &r2);

	if (n1 <= -1 || n2 <= -1)
	{
		SETERR_COD (rtx, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}

	n3 = n1 + (n2 << 1);
	QSE_ASSERT (n3 >= 0 && n3 <= 3);
	return (n3 == 0)? qse_awk_rtx_makeintval(rtx,(qse_awk_int_t)l1*(qse_awk_int_t)l2):
	       (n3 == 1)? qse_awk_rtx_makefltval(rtx,(qse_awk_flt_t)r1*(qse_awk_flt_t)l2):
	       (n3 == 2)? qse_awk_rtx_makefltval(rtx,(qse_awk_flt_t)l1*(qse_awk_flt_t)r2):
	                  qse_awk_rtx_makefltval(rtx,(qse_awk_flt_t)r1*(qse_awk_flt_t)r2);
}

static qse_awk_val_t* eval_binop_div (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n1, n2, n3;
	qse_awk_int_t l1, l2;
	qse_awk_flt_t r1, r2;
	qse_awk_val_t* res;

	n1 = qse_awk_rtx_valtonum (rtx, left, &l1, &r1);
	n2 = qse_awk_rtx_valtonum (rtx, right, &l2, &r2);

	if (n1 <= -1 || n2 <= -1) 
	{
		SETERR_COD (rtx, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}

	n3 = n1 + (n2 << 1);
	switch (n3)
	{
		case 0:
			if  (l2 == 0) 
			{
				SETERR_COD (rtx, QSE_AWK_EDIVBY0);
				return QSE_NULL;
			}

			if (((qse_awk_int_t)l1 % (qse_awk_int_t)l2) == 0)
			{
				res = qse_awk_rtx_makeintval (
					rtx, (qse_awk_int_t)l1 / (qse_awk_int_t)l2);
			}
			else
			{
				res = qse_awk_rtx_makefltval (
					rtx, (qse_awk_flt_t)l1 / (qse_awk_flt_t)l2);
			}
			break;

		case 1:
			res = qse_awk_rtx_makefltval (
				rtx, (qse_awk_flt_t)r1 / (qse_awk_flt_t)l2);
			break;

		case 2:
			res = qse_awk_rtx_makefltval (
				rtx, (qse_awk_flt_t)l1 / (qse_awk_flt_t)r2);
			break;

		case 3:
			res = qse_awk_rtx_makefltval (
				rtx, (qse_awk_flt_t)r1 / (qse_awk_flt_t)r2);
			break;
	}

	return res;
}

static qse_awk_val_t* eval_binop_idiv (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n1, n2, n3;
	qse_awk_int_t l1, l2;
	qse_awk_flt_t r1, r2, quo;
	qse_awk_val_t* res;

	n1 = qse_awk_rtx_valtonum (rtx, left, &l1, &r1);
	n2 = qse_awk_rtx_valtonum (rtx, right, &l2, &r2);

	if (n1 <= -1 || n2 <= -1) 
	{
		SETERR_COD (rtx, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}

	n3 = n1 + (n2 << 1);
	switch (n3)
	{
		case 0:
			if (l2 == 0) 
			{
				SETERR_COD (rtx, QSE_AWK_EDIVBY0);
				return QSE_NULL;
			}
			res = qse_awk_rtx_makeintval (
				rtx, (qse_awk_int_t)l1 / (qse_awk_int_t)l2);
			break;

		case 1:
			quo = (qse_awk_flt_t)r1 / (qse_awk_flt_t)l2;
			res = qse_awk_rtx_makeintval (rtx, (qse_awk_int_t)quo);
			break;

		case 2:
			quo = (qse_awk_flt_t)l1 / (qse_awk_flt_t)r2;
			res = qse_awk_rtx_makeintval (rtx, (qse_awk_int_t)quo);
			break;

		case 3:
			quo = (qse_awk_flt_t)r1 / (qse_awk_flt_t)r2;
			res = qse_awk_rtx_makeintval (rtx, (qse_awk_int_t)quo);
			break;
	}

	return res;
}

static qse_awk_val_t* eval_binop_mod (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n1, n2, n3;
	qse_awk_int_t l1, l2;
	qse_awk_flt_t r1, r2;
	qse_awk_val_t* res;

	QSE_ASSERTX (rtx->awk->prm.math.mod != QSE_NULL,
		"the mod function must be provided when the awk object is created to be able to calculate floating-pointer remainder.");

	n1 = qse_awk_rtx_valtonum (rtx, left, &l1, &r1);
	n2 = qse_awk_rtx_valtonum (rtx, right, &l2, &r2);

	if (n1 <= -1 || n2 <= -1)
	{
		SETERR_COD (rtx, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}

	n3 = n1 + (n2 << 1);
	switch (n3)
	{
		case 0:
			if  (l2 == 0) 
			{
				SETERR_COD (rtx, QSE_AWK_EDIVBY0);
				return QSE_NULL;
			}
			res = qse_awk_rtx_makeintval (
				rtx, (qse_awk_int_t)l1 % (qse_awk_int_t)l2);
			break;	

		case 1:
			res = qse_awk_rtx_makefltval (rtx,
				rtx->awk->prm.math.mod (
					rtx->awk, (qse_awk_flt_t)r1, (qse_awk_flt_t)l2)
			);
			break;

		case 2:
			res = qse_awk_rtx_makefltval (rtx,
				rtx->awk->prm.math.mod (
					rtx->awk, (qse_awk_flt_t)l1, (qse_awk_flt_t)r2)
			);
			break;

		case 3:
			res = qse_awk_rtx_makefltval (rtx,
				rtx->awk->prm.math.mod (
					rtx->awk, (qse_awk_flt_t)r1, (qse_awk_flt_t)r2)
			);
			break;
	}

	return res;
}

static qse_awk_val_t* eval_binop_exp (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right)
{
	int n1, n2, n3;
	qse_awk_int_t l1, l2;
	qse_awk_flt_t r1, r2;
	qse_awk_val_t* res;

	n1 = qse_awk_rtx_valtonum (rtx, left, &l1, &r1);
	n2 = qse_awk_rtx_valtonum (rtx, right, &l2, &r2);

	if (n1 <= -1 || n2 <= -1) 
	{
		SETERR_COD (rtx, QSE_AWK_EOPERAND);
		return QSE_NULL;
	}

	n3 = n1 + (n2 << 1);
	switch (n3)
	{
		case 0:
			/* left - int, right - int */
			if (l2 >= 0)
			{
				qse_awk_int_t v = 1;
				while (l2-- > 0) v *= l1;
				res = qse_awk_rtx_makeintval (rtx, v);
			}
			else if (l1 == 0)
			{
				SETERR_COD (rtx, QSE_AWK_EDIVBY0);
				return QSE_NULL;
			}
			else
			{
				qse_awk_flt_t v = 1.0;
				l2 *= -1;
				while (l2-- > 0) v /= l1;
				res = qse_awk_rtx_makefltval (rtx, v);
			}
			break;

		case 1:
			/* left - real, right - int */
			if (l2 >= 0)
			{
				qse_awk_flt_t v = 1.0;
				while (l2-- > 0) v *= r1;
				res = qse_awk_rtx_makefltval (rtx, v);
			}
			else if (r1 == 0.0)
			{
				SETERR_COD (rtx, QSE_AWK_EDIVBY0);
				return QSE_NULL;
			}
			else
			{
				qse_awk_flt_t v = 1.0;
				l2 *= -1;
				while (l2-- > 0) v /= r1;
				res = qse_awk_rtx_makefltval (rtx, v);
			}
			break;

		case 2:
			/* left - int, right - real */
			res = qse_awk_rtx_makefltval (
				rtx, 
				rtx->awk->prm.math.pow (
					rtx->awk, (qse_awk_flt_t)l1, (qse_awk_flt_t)r2
				)
			);
			break;

		case 3:
			/* left - real, right - real */
			res = qse_awk_rtx_makefltval (
				rtx,
				rtx->awk->prm.math.pow (
					rtx->awk, (qse_awk_flt_t)r1,(qse_awk_flt_t)r2
				)
			);
			break;
	}

	return res;
}

static qse_awk_val_t* eval_binop_concat (
	qse_awk_rtx_t* run, qse_awk_val_t* left, qse_awk_val_t* right)
{
	qse_awk_val_t* res;
	qse_awk_rtx_valtostr_out_t lout, rout;

	lout.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
	if (qse_awk_rtx_valtostr (run, left, &lout) <= -1) return QSE_NULL;

	rout.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
	if (qse_awk_rtx_valtostr (run, right, &rout) <= -1)
	{
		QSE_AWK_FREE (run->awk, lout.u.cpldup.ptr);
		return QSE_NULL;
	}

	res = qse_awk_rtx_makestrval2 (
		run, 
		lout.u.cpldup.ptr, lout.u.cpldup.len,
		rout.u.cpldup.ptr, rout.u.cpldup.len
	);

	QSE_AWK_FREE (run->awk, rout.u.cpldup.ptr);
	QSE_AWK_FREE (run->awk, lout.u.cpldup.ptr);

	return res;
}

static qse_awk_val_t* eval_binop_match0 (
	qse_awk_rtx_t* rtx, qse_awk_val_t* left, qse_awk_val_t* right,
	const qse_awk_loc_t* lloc, const qse_awk_loc_t* rloc, int ret)
{
	qse_awk_val_t* res;
	qse_xstr_t out;
	int n;

	out.ptr = qse_awk_rtx_getvalstr (rtx, left, &out.len);
	if (out.ptr == QSE_NULL) return QSE_NULL;

	n = qse_awk_rtx_matchrex (rtx, right, &out, &out, QSE_NULL);
	qse_awk_rtx_freevalstr (rtx, left, out.ptr);

	if (n <= -1) 
	{
		ADJERR_LOC (rtx, lloc);
		return QSE_NULL;
	}

	res = qse_awk_rtx_makeintval (rtx, (n == ret));
	if (res == QSE_NULL) 
	{
		ADJERR_LOC (rtx, lloc);
		return QSE_NULL;
	}

	return res;
}

static qse_awk_val_t* eval_binop_ma (
	qse_awk_rtx_t* run, qse_awk_nde_t* left, qse_awk_nde_t* right)
{
	qse_awk_val_t* lv, * rv, * res;

	QSE_ASSERT (left->next == QSE_NULL);
	QSE_ASSERT (right->next == QSE_NULL);

	lv = eval_expression (run, left);
	if (lv == QSE_NULL) return QSE_NULL;

	qse_awk_rtx_refupval (run, lv);

	rv = eval_expression0 (run, right);
	if (rv == QSE_NULL)
	{
		qse_awk_rtx_refdownval (run, lv);
		return QSE_NULL;
	}

	qse_awk_rtx_refupval (run, rv);

	res = eval_binop_match0 (run, lv, rv, &left->loc, &right->loc, 1);

	qse_awk_rtx_refdownval (run, rv);
	qse_awk_rtx_refdownval (run, lv);

	return res;
}

static qse_awk_val_t* eval_binop_nm (
	qse_awk_rtx_t* run, qse_awk_nde_t* left, qse_awk_nde_t* right)
{
	qse_awk_val_t* lv, * rv, * res;

	QSE_ASSERT (left->next == QSE_NULL);
	QSE_ASSERT (right->next == QSE_NULL);

	lv = eval_expression (run, left);
	if (lv == QSE_NULL) return QSE_NULL;

	qse_awk_rtx_refupval (run, lv);

	rv = eval_expression0 (run, right);
	if (rv == QSE_NULL)
	{
		qse_awk_rtx_refdownval (run, lv);
		return QSE_NULL;
	}

	qse_awk_rtx_refupval (run, rv);

	res = eval_binop_match0 (run, lv, rv, &left->loc, &right->loc, 0);

	qse_awk_rtx_refdownval (run, rv);
	qse_awk_rtx_refdownval (run, lv);

	return res;
}

static qse_awk_val_t* eval_unary (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_val_t* left, * res = QSE_NULL;
	qse_awk_nde_exp_t* exp = (qse_awk_nde_exp_t*)nde;
	int n;
	qse_awk_int_t l;
	qse_awk_flt_t r;

	QSE_ASSERT (
		exp->type == QSE_AWK_NDE_EXP_UNR);
	QSE_ASSERT (
		exp->left != QSE_NULL && exp->right == QSE_NULL);
	QSE_ASSERT (
		exp->opcode == QSE_AWK_UNROP_PLUS ||
		exp->opcode == QSE_AWK_UNROP_MINUS ||
		exp->opcode == QSE_AWK_UNROP_LNOT ||
		exp->opcode == QSE_AWK_UNROP_BNOT ||
		exp->opcode == QSE_AWK_UNROP_DEF);

	QSE_ASSERT (exp->left->next == QSE_NULL);
	left = eval_expression (run, exp->left);
	if (left == QSE_NULL) return QSE_NULL;

	qse_awk_rtx_refupval (run, left);

	switch (exp->opcode)
	{
		case QSE_AWK_UNROP_MINUS:
			n = qse_awk_rtx_valtonum (run, left, &l, &r);
			if (n <= -1) goto exit_func;

			res = (n == 0)? qse_awk_rtx_makeintval (run, -l):
			                qse_awk_rtx_makefltval (run, -r);
			break;

		case QSE_AWK_UNROP_LNOT:
			if (left->type == QSE_AWK_VAL_STR)
			{
				res = qse_awk_rtx_makeintval (
					run, !(((qse_awk_val_str_t*)left)->val.len > 0));
			}
			else
			{
				n = qse_awk_rtx_valtonum (run, left, &l, &r);
				if (n <= -1) goto exit_func;

				res = (n == 0)? qse_awk_rtx_makeintval (run, !l):
				                qse_awk_rtx_makefltval (run, !r);
			}
			break;
	
		case QSE_AWK_UNROP_BNOT:
			n = qse_awk_rtx_valtoint (run, left, &l);
			if (n <= -1) goto exit_func;

			res = qse_awk_rtx_makeintval (run, ~l);
			break;

		case QSE_AWK_UNROP_PLUS:
			n = qse_awk_rtx_valtonum (run, left, &l, &r);
			if (n <= -1) goto exit_func;

			res = (n == 0)? qse_awk_rtx_makeintval (run, l):
			                qse_awk_rtx_makefltval (run, r);
			break;

		case QSE_AWK_UNROP_DEF:
			res = qse_awk_rtx_makeintval (
				run, ((left->type == QSE_AWK_VAL_NIL)? 0: 1));
			break;
	}

exit_func:
	qse_awk_rtx_refdownval (run, left);
	if (res == QSE_NULL) ADJERR_LOC (run, &nde->loc);
	return res;
}

static qse_awk_val_t* eval_incpre (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_val_t* left, * res;
	qse_awk_nde_exp_t* exp = (qse_awk_nde_exp_t*)nde;

	QSE_ASSERT (exp->type == QSE_AWK_NDE_EXP_INCPRE);
	QSE_ASSERT (exp->left != QSE_NULL && exp->right == QSE_NULL);

	/* this way of checking if the l-value is assignable is
	 * ugly as it is dependent on the node types defined in qse/awk/awk.h
	 * but let's keep going this way for the time being. */
	if (exp->left->type < QSE_AWK_NDE_NAMED ||
	    /*exp->left->type > QSE_AWK_NDE_ARGIDX) XXX */
	    exp->left->type > QSE_AWK_NDE_POS)
	{
		SETERR_LOC (run, QSE_AWK_EOPERAND, &nde->loc);
		return QSE_NULL;
	}

	QSE_ASSERT (exp->left->next == QSE_NULL);
	left = eval_expression (run, exp->left);
	if (left == QSE_NULL) return QSE_NULL;

	qse_awk_rtx_refupval (run, left);

	if (exp->opcode == QSE_AWK_INCOP_PLUS) 
	{
		if (left->type == QSE_AWK_VAL_INT)
		{
			qse_awk_int_t r = ((qse_awk_val_int_t*)left)->val;
			res = qse_awk_rtx_makeintval (run, r + 1);
			if (res == QSE_NULL) 
			{
				qse_awk_rtx_refdownval (run, left);
				ADJERR_LOC (run, &nde->loc);
				return QSE_NULL;
			}
		}
		else if (left->type == QSE_AWK_VAL_FLT)
		{
			qse_awk_flt_t r = ((qse_awk_val_flt_t*)left)->val;
			res = qse_awk_rtx_makefltval (run, r + 1.0);
			if (res == QSE_NULL) 
			{
				qse_awk_rtx_refdownval (run, left);
				ADJERR_LOC (run, &nde->loc);
				return QSE_NULL;
			}
		}
		else
		{
			qse_awk_int_t v1;
			qse_awk_flt_t v2;
			int n;

			n = qse_awk_rtx_valtonum (run, left, &v1, &v2);
			if (n <= -1)
			{
				qse_awk_rtx_refdownval (run, left);
				ADJERR_LOC (run, &nde->loc);
				return QSE_NULL;
			}

			if (n == 0) 
			{
				res = qse_awk_rtx_makeintval (run, v1 + 1);
			}
			else /* if (n == 1) */
			{
				QSE_ASSERT (n == 1);
				res = qse_awk_rtx_makefltval (run, v2 + 1.0);
			}

			if (res == QSE_NULL) 
			{
				qse_awk_rtx_refdownval (run, left);
				ADJERR_LOC (run, &nde->loc);
				return QSE_NULL;
			}
		}
	}
	else if (exp->opcode == QSE_AWK_INCOP_MINUS)
	{
		if (left->type == QSE_AWK_VAL_INT)
		{
			qse_awk_int_t r = ((qse_awk_val_int_t*)left)->val;
			res = qse_awk_rtx_makeintval (run, r - 1);
			if (res == QSE_NULL) 
			{
				qse_awk_rtx_refdownval (run, left);
				ADJERR_LOC (run, &nde->loc);
				return QSE_NULL;
			}
		}
		else if (left->type == QSE_AWK_VAL_FLT)
		{
			qse_awk_flt_t r = ((qse_awk_val_flt_t*)left)->val;
			res = qse_awk_rtx_makefltval (run, r - 1.0);
			if (res == QSE_NULL) 
			{
				qse_awk_rtx_refdownval (run, left);
				ADJERR_LOC (run, &nde->loc);
				return QSE_NULL;
			}
		}
		else
		{
			qse_awk_int_t v1;
			qse_awk_flt_t v2;
			int n;

			n = qse_awk_rtx_valtonum (run, left, &v1, &v2);
			if (n == -1)
			{
				qse_awk_rtx_refdownval (run, left);
				ADJERR_LOC (run, &nde->loc);
				return QSE_NULL;
			}

			if (n == 0) 
			{
				res = qse_awk_rtx_makeintval (run, v1 - 1);
			}
			else /* if (n == 1) */
			{
				QSE_ASSERT (n == 1);
				res = qse_awk_rtx_makefltval (run, v2 - 1.0);
			}

			if (res == QSE_NULL) 
			{
				qse_awk_rtx_refdownval (run, left);
				ADJERR_LOC (run, &nde->loc);
				return QSE_NULL;
			}
		}
	}
	else
	{
		QSE_ASSERT (!"should never happen - invalid opcode");
		qse_awk_rtx_refdownval (run, left);
		SETERR_LOC (run, QSE_AWK_EINTERN, &nde->loc);
		return QSE_NULL;
	}

	if (do_assignment (run, exp->left, res) == QSE_NULL)
	{
		qse_awk_rtx_refdownval (run, left);
		return QSE_NULL;
	}

	qse_awk_rtx_refdownval (run, left);
	return res;
}

static qse_awk_val_t* eval_incpst (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_val_t* left, * res, * res2;
	qse_awk_nde_exp_t* exp = (qse_awk_nde_exp_t*)nde;

	QSE_ASSERT (exp->type == QSE_AWK_NDE_EXP_INCPST);
	QSE_ASSERT (exp->left != QSE_NULL && exp->right == QSE_NULL);

	/* this way of checking if the l-value is assignable is
	 * ugly as it is dependent on the node types defined in qse/awk/awk.h.
	 * but let's keep going this way for the time being. */
	if (exp->left->type < QSE_AWK_NDE_NAMED ||
	    /*exp->left->type > QSE_AWK_NDE_ARGIDX) XXX */
	    exp->left->type > QSE_AWK_NDE_POS)
	{
		SETERR_LOC (run, QSE_AWK_EOPERAND, &nde->loc);
		return QSE_NULL;
	}

	QSE_ASSERT (exp->left->next == QSE_NULL);
	left = eval_expression (run, exp->left);
	if (left == QSE_NULL) return QSE_NULL;

	qse_awk_rtx_refupval (run, left);

	if (exp->opcode == QSE_AWK_INCOP_PLUS) 
	{
		if (left->type == QSE_AWK_VAL_INT)
		{
			qse_awk_int_t r = ((qse_awk_val_int_t*)left)->val;
			res = qse_awk_rtx_makeintval (run, r);
			if (res == QSE_NULL) 
			{
				qse_awk_rtx_refdownval (run, left);
				ADJERR_LOC (run, &nde->loc);
				return QSE_NULL;
			}

			res2 = qse_awk_rtx_makeintval (run, r + 1);
			if (res2 == QSE_NULL)
			{
				qse_awk_rtx_refdownval (run, left);
				qse_awk_rtx_freeval (run, res, 1);
				ADJERR_LOC (run, &nde->loc);
				return QSE_NULL;
			}
		}
		else if (left->type == QSE_AWK_VAL_FLT)
		{
			qse_awk_flt_t r = ((qse_awk_val_flt_t*)left)->val;
			res = qse_awk_rtx_makefltval (run, r);
			if (res == QSE_NULL) 
			{
				qse_awk_rtx_refdownval (run, left);
				ADJERR_LOC (run, &nde->loc);
				return QSE_NULL;
			}

			res2 = qse_awk_rtx_makefltval (run, r + 1.0);
			if (res2 == QSE_NULL)
			{
				qse_awk_rtx_refdownval (run, left);
				qse_awk_rtx_freeval (run, res, 1);
				ADJERR_LOC (run, &nde->loc);
				return QSE_NULL;
			}
		}
		else
		{
			qse_awk_int_t v1;
			qse_awk_flt_t v2;
			int n;

			n = qse_awk_rtx_valtonum (run, left, &v1, &v2);
			if (n <= -1)
			{
				qse_awk_rtx_refdownval (run, left);
				ADJERR_LOC (run, &nde->loc);
				return QSE_NULL;
			}

			if (n == 0) 
			{
				res = qse_awk_rtx_makeintval (run, v1);
				if (res == QSE_NULL)
				{
					qse_awk_rtx_refdownval (run, left);
					ADJERR_LOC (run, &nde->loc);
					return QSE_NULL;
				}

				res2 = qse_awk_rtx_makeintval (run, v1 + 1);
				if (res2 == QSE_NULL)
				{
					qse_awk_rtx_refdownval (run, left);
					qse_awk_rtx_freeval (run, res, 1);
					ADJERR_LOC (run, &nde->loc);
					return QSE_NULL;
				}
			}
			else /* if (n == 1) */
			{
				QSE_ASSERT (n == 1);
				res = qse_awk_rtx_makefltval (run, v2);
				if (res == QSE_NULL)
				{
					qse_awk_rtx_refdownval (run, left);
					ADJERR_LOC (run, &nde->loc);
					return QSE_NULL;
				}

				res2 = qse_awk_rtx_makefltval (run, v2 + 1.0);
				if (res2 == QSE_NULL)
				{
					qse_awk_rtx_refdownval (run, left);
					qse_awk_rtx_freeval (run, res, 1);
					ADJERR_LOC (run, &nde->loc);
					return QSE_NULL;
				}
			}
		}
	}
	else if (exp->opcode == QSE_AWK_INCOP_MINUS)
	{
		if (left->type == QSE_AWK_VAL_INT)
		{
			qse_awk_int_t r = ((qse_awk_val_int_t*)left)->val;
			res = qse_awk_rtx_makeintval (run, r);
			if (res == QSE_NULL) 
			{
				qse_awk_rtx_refdownval (run, left);
				ADJERR_LOC (run, &nde->loc);
				return QSE_NULL;
			}

			res2 = qse_awk_rtx_makeintval (run, r - 1);
			if (res2 == QSE_NULL)
			{
				qse_awk_rtx_refdownval (run, left);
				qse_awk_rtx_freeval (run, res, 1);
				ADJERR_LOC (run, &nde->loc);
				return QSE_NULL;
			}
		}
		else if (left->type == QSE_AWK_VAL_FLT)
		{
			qse_awk_flt_t r = ((qse_awk_val_flt_t*)left)->val;
			res = qse_awk_rtx_makefltval (run, r);
			if (res == QSE_NULL) 
			{
				qse_awk_rtx_refdownval (run, left);
				ADJERR_LOC (run, &nde->loc);
				return QSE_NULL;
			}

			res2 = qse_awk_rtx_makefltval (run, r - 1.0);
			if (res2 == QSE_NULL)
			{
				qse_awk_rtx_refdownval (run, left);
				qse_awk_rtx_freeval (run, res, 1);
				ADJERR_LOC (run, &nde->loc);
				return QSE_NULL;
			}
		}
		else
		{
			qse_awk_int_t v1;
			qse_awk_flt_t v2;
			int n;

			n = qse_awk_rtx_valtonum (run, left, &v1, &v2);
			if (n == -1)
			{
				qse_awk_rtx_refdownval (run, left);
				ADJERR_LOC (run, &nde->loc);
				return QSE_NULL;
			}

			if (n == 0) 
			{
				res = qse_awk_rtx_makeintval (run, v1);
				if (res == QSE_NULL)
				{
					qse_awk_rtx_refdownval (run, left);
					ADJERR_LOC (run, &nde->loc);
					return QSE_NULL;
				}

				res2 = qse_awk_rtx_makeintval (run, v1 - 1);
				if (res2 == QSE_NULL)
				{
					qse_awk_rtx_refdownval (run, left);
					qse_awk_rtx_freeval (run, res, 1);
					ADJERR_LOC (run, &nde->loc);
					return QSE_NULL;
				}
			}
			else /* if (n == 1) */
			{
				QSE_ASSERT (n == 1);
				res = qse_awk_rtx_makefltval (run, v2);
				if (res == QSE_NULL)
				{
					qse_awk_rtx_refdownval (run, left);
					ADJERR_LOC (run, &nde->loc);
					return QSE_NULL;
				}

				res2 = qse_awk_rtx_makefltval (run, v2 - 1.0);
				if (res2 == QSE_NULL)
				{
					qse_awk_rtx_refdownval (run, left);
					qse_awk_rtx_freeval (run, res, 1);
					ADJERR_LOC (run, &nde->loc);
					return QSE_NULL;
				}
			}
		}
	}
	else
	{
		QSE_ASSERT (!"should never happen - invalid opcode");
		qse_awk_rtx_refdownval (run, left);
		SETERR_LOC (run, QSE_AWK_EINTERN, &nde->loc);
		return QSE_NULL;
	}

	if (do_assignment (run, exp->left, res2) == QSE_NULL)
	{
		qse_awk_rtx_refdownval (run, left);
		return QSE_NULL;
	}

	qse_awk_rtx_refdownval (run, left);
	return res;
}

static qse_awk_val_t* eval_cnd (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_val_t* tv, * v;
	qse_awk_nde_cnd_t* cnd = (qse_awk_nde_cnd_t*)nde;

	QSE_ASSERT (cnd->test->next == QSE_NULL);

	tv = eval_expression (run, cnd->test);
	if (tv == QSE_NULL) return QSE_NULL;

	qse_awk_rtx_refupval (run, tv);

	QSE_ASSERT (
		cnd->left->next == QSE_NULL && 
		cnd->right->next == QSE_NULL);
	v = (qse_awk_rtx_valtobool (run, tv))?
		eval_expression (run, cnd->left):
		eval_expression (run, cnd->right);

	qse_awk_rtx_refdownval (run, tv);
	return v;
}

static qse_awk_val_t* eval_fnc (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	/* intrinsic function */

	qse_awk_nde_fncall_t* call = (qse_awk_nde_fncall_t*)nde;

	/* the parser must make sure taht the number of arguments 
	 * is proper */ 
	QSE_ASSERT (call->nargs >= call->u.fnc.spec.arg.min && 
	            call->nargs <= call->u.fnc.spec.arg.max); 

	return eval_call (
		run, nde, call->u.fnc.spec.arg.spec,
		QSE_NULL, QSE_NULL, QSE_NULL);
}

static qse_awk_val_t* eval_fun_ex (
	qse_awk_rtx_t* rtx, qse_awk_nde_t* nde,
	void(*errhandler)(void*), void* eharg)
{
	qse_awk_nde_fncall_t* call = (qse_awk_nde_fncall_t*)nde;
	qse_awk_fun_t* fun;
	qse_htb_pair_t* pair;

	pair = qse_htb_search (rtx->awk->tree.funs,
		call->u.fun.name.ptr, call->u.fun.name.len);
	if (pair == QSE_NULL) 
	{
		SETERR_ARGX_LOC (
			rtx, QSE_AWK_EFUNNF,
			xstr_to_cstr(&call->u.fun.name), &nde->loc);
		return QSE_NULL;
	}

	fun = (qse_awk_fun_t*)QSE_HTB_VPTR(pair);
	QSE_ASSERT (fun != QSE_NULL);

	if (call->nargs > fun->nargs)
	{
		/* TODO: is this correct? what if i want to 
		 *       allow arbitarary numbers of arguments? */
		SETERR_LOC (rtx, QSE_AWK_EARGTM, &nde->loc);
		return QSE_NULL;
	}

	return eval_call (rtx, nde, QSE_NULL, fun, errhandler, eharg);
}

static qse_awk_val_t* eval_fun (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	return eval_fun_ex (run, nde, QSE_NULL, QSE_NULL);
}

/* run->stack_base has not been set for this  
 * stack frame. so the RTX_STACK_ARG macro cannot be used as in 
 * qse_awk_rtx_refdownval (run, RTX_STACK_ARG(run,nargs));*/ 
#define UNWIND_RTX_RTX_STACK_ARG(rtx,nargs) \
	do { \
		while ((nargs) > 0) \
		{ \
			--(nargs); \
			qse_awk_rtx_refdownval ((rtx), (rtx)->stack[(rtx)->stack_top-1]); \
			__raw_pop (rtx); \
		} \
	} while (0)

#define UNWIND_RTX_RTX_STACK_BASE(rtx) \
	do { \
		__raw_pop (rtx); /* nargs */ \
		__raw_pop (rtx); /* return */ \
		__raw_pop (rtx); /* prev stack top */ \
		__raw_pop (rtx); /* prev stack back */  \
	} while (0)

#define UNWIND_RTX_STACK(rtx,nargs) \
	do { \
		UNWIND_RTX_RTX_STACK_ARG(rtx,nargs); \
		UNWIND_RTX_RTX_STACK_BASE(rtx); \
	} while (0)

static qse_awk_val_t* __eval_call (
	qse_awk_rtx_t* run, 
	qse_awk_nde_t* nde, 
	const qse_char_t* fnc_arg_spec,
	qse_awk_fun_t* fun, 
	qse_size_t(*argpusher)(qse_awk_rtx_t*,qse_awk_nde_fncall_t*,void*), 
	void* apdata,
	void(*errhandler)(void*),
	void* eharg)
{
	qse_awk_nde_fncall_t* call = (qse_awk_nde_fncall_t*)nde;
	qse_size_t saved_stack_top;
	qse_size_t nargs, i;
	qse_awk_val_t* v;
	int n;

	/* 
	 * ---------------------
	 *  lcln               <- stack top
	 * ---------------------
	 *  ....
	 * ---------------------
	 *  lcl0               local variables are pushed by run_block
	 * =====================
	 *  argn                     
	 * ---------------------
	 *  ....
	 * ---------------------
	 *  arg1
	 * ---------------------
	 *  arg0 
	 * ---------------------
	 *  nargs 
	 * ---------------------
	 *  return value
	 * ---------------------
	 *  previous stack top
	 * ---------------------
	 *  previous stack base  <- stack base
	 * =====================
	 *  0 (nargs)            <- stack top
	 * ---------------------
	 *  return value
	 * ---------------------
	 *  previous stack top
	 * ---------------------
	 *  previous stack base  <- stack base
	 * =====================
	 *  gbln
	 * ---------------------
	 *  ....
	 * ---------------------
	 *  gbl0
	 * ---------------------
	 */

	QSE_ASSERT (QSE_SIZEOF(void*) >= QSE_SIZEOF(run->stack_top));
	QSE_ASSERT (QSE_SIZEOF(void*) >= QSE_SIZEOF(run->stack_base));

	saved_stack_top = run->stack_top;

#ifdef DEBUG_RUN
	qse_errputstrf (QSE_T("setting up function stack frame top=%zd base=%zd\n"), 
		(qse_size_t)run->stack_top, (qse_size_t)run->stack_base);
#endif
	if (__raw_push(run,(void*)run->stack_base) == -1) 
	{
		SETERR_LOC (run, QSE_AWK_ENOMEM, &nde->loc);
		return QSE_NULL;
	}

	if (__raw_push(run,(void*)saved_stack_top) == -1) 
	{
		__raw_pop (run);
		SETERR_LOC (run, QSE_AWK_ENOMEM, &nde->loc);
		return QSE_NULL;
	}

	/* secure space for a return value. */
	if (__raw_push(run,qse_awk_val_nil) == -1)
	{
		__raw_pop (run);
		__raw_pop (run);
		SETERR_LOC (run, QSE_AWK_ENOMEM, &nde->loc);
		return QSE_NULL;
	}

	/* secure space for nargs */
	if (__raw_push(run,qse_awk_val_nil) == -1)
	{
		__raw_pop (run);
		__raw_pop (run);
		__raw_pop (run);
		SETERR_LOC (run, QSE_AWK_ENOMEM, &nde->loc);
		return QSE_NULL;
	}

	/* push all arguments onto the stack */
	nargs = argpusher (run, call, apdata);
	if (nargs == (qse_size_t)-1)
	{
		UNWIND_RTX_RTX_STACK_BASE (run);
		return QSE_NULL;
	}

	QSE_ASSERT (nargs == call->nargs);

	if (fun != QSE_NULL)
	{
		/* extra step for normal awk functions */

		while (nargs < fun->nargs)
		{
			/* push as many nils as the number of missing actual arguments */
			if (__raw_push(run,qse_awk_val_nil) == -1)
			{
				UNWIND_RTX_STACK (run, nargs);
				SETERR_LOC (run, QSE_AWK_ENOMEM, &nde->loc);
				return QSE_NULL;
			}

			nargs++;
		}
	}

	run->stack_base = saved_stack_top;
	RTX_STACK_NARGS(run) = (void*)nargs;
	
#ifdef DEBUG_RUN
	qse_errputstrf (QSE_T("running function body\n"));
#endif

	if (fun != QSE_NULL)
	{
		/* normal awk function */
		QSE_ASSERT (fun->body->type == QSE_AWK_NDE_BLK);
		n = run_block(run,(qse_awk_nde_blk_t*)fun->body);
	}
	else
	{
		n = 0;

		/* intrinsic function */
		QSE_ASSERT (
			call->nargs >= call->u.fnc.spec.arg.min &&
			call->nargs <= call->u.fnc.spec.arg.max);

		if (call->u.fnc.spec.impl)
		{
			run->errinf.num = QSE_AWK_ENOERR;

			n = call->u.fnc.spec.impl (run, &call->u.fnc.info);

			if (n <= -1)
			{
				if (run->errinf.num == QSE_AWK_ENOERR)
				{
					/* the handler has not set the error.
					 * fix it */ 
					SETERR_ARGX_LOC (
						run, QSE_AWK_EFNCIMPL, 
						xstr_to_cstr(&call->u.fnc.info.name),
						&nde->loc
					);
				}
				else
				{
					ADJERR_LOC (run, &nde->loc);
				}

				/* correct the return code just in case */
				if (n < -1) n = -1;
			}
		}
	}

	/* refdown args in the run.stack */
	nargs = (qse_size_t)RTX_STACK_NARGS(run);
#ifdef DEBUG_RUN
	qse_errputstrf (QSE_T("block run complete nargs = %d\n"), (int)nargs); 
#endif

	for (i = 0; i < nargs; i++)
	{
		qse_awk_rtx_refdownval (run, RTX_STACK_ARG(run,i));
	}

#ifdef DEBUG_RUN
	qse_errputstrf (QSE_T("got return value\n"));
#endif

	v = RTX_STACK_RETVAL(run);
	if (n == -1)
	{
		if (run->errinf.num == QSE_AWK_ENOERR && errhandler != QSE_NULL) 
		{
			/* errhandler is passed only when __eval_call() is
			 * invoked from qse_awk_rtx_call(). Under this 
			 * circumstance, this stack frame is the first
			 * activated and the stack base is the first element
			 * after the global variables. so RTX_STACK_RETVAL(run)
			 * effectively becomes RTX_STACK_RETVAL_GBL(run).
			 * As __eval_call() returns QSE_NULL on error and
			 * the reference count of RTX_STACK_RETVAL(run) should be
			 * decremented, it can't get the return value
			 * if it turns out to be terminated by exit().
			 * The return value could be destroyed by then.
			 * Unlikely, run_bpae_loop() just checks if run->errinf.num
			 * is QSE_AWK_ENOERR and gets RTX_STACK_RETVAL_GBL(run)
			 * to determine if it is terminated by exit().
			 *
			 * The handler capture_retval_on_exit() 
			 * increments the reference of RTX_STACK_RETVAL(run)
			 * and stores the pointer into accompanying space.
			 * This way, the return value is preserved upon
			 * termination by exit() out to the caller.
			 */
			errhandler (eharg);
		}

		/* if the earlier operations failed and this function
		 * has to return a error, the return value is just
		 * destroyed and replaced by nil */
		qse_awk_rtx_refdownval (run, v);
		RTX_STACK_RETVAL(run) = qse_awk_val_nil;
	}
	else
	{	
		/* this trick has been mentioned in run_return.
		 * adjust the reference count of the return value.
		 * the value must not be freed even if the reference count
		 * reached zero because its reference has been incremented 
		 * in run_return or directly by qse_awk_rtx_setretval()
		 * regardless of its reference count. */
		qse_awk_rtx_refdownval_nofree (run, v);
	}

	run->stack_top =  (qse_size_t)run->stack[run->stack_base+1];
	run->stack_base = (qse_size_t)run->stack[run->stack_base+0];

	if (run->exit_level == EXIT_FUNCTION) run->exit_level = EXIT_NONE;

#ifdef DEBUG_RUN
	qse_errputstrf (QSE_T("returning from function top=%zd, base=%zd\n"),
		(qse_size_t)run->stack_top, (qse_size_t)run->stack_base); 
#endif
	return (n == -1)? QSE_NULL: v;
}

static qse_size_t push_arg_from_vals (
	qse_awk_rtx_t* rtx, qse_awk_nde_fncall_t* call, void* data)
{
	struct pafv* pafv = (struct pafv*)data;
	qse_size_t nargs = 0;

	for (nargs = 0; nargs < pafv->nargs; nargs++)
	{
		if (__raw_push (rtx, pafv->args[nargs]) == -1) 
		{
			/* ugly - arg needs to be freed if it doesn't have
			 * any reference. but its reference has not been 
			 * updated yet as it is carried out after successful
			 * stack push. so it adds up a reference and 
			 * dereferences it */
			qse_awk_rtx_refupval (rtx, pafv->args[nargs]);
			qse_awk_rtx_refdownval (rtx, pafv->args[nargs]);

			UNWIND_RTX_RTX_STACK_ARG (rtx, nargs);
			SETERR_LOC (rtx, QSE_AWK_ENOMEM, &call->loc);
			return (qse_size_t)-1;
		}

		qse_awk_rtx_refupval (rtx, pafv->args[nargs]);
	}

	return nargs; 
}

static qse_size_t push_arg_from_nde (
	qse_awk_rtx_t* rtx, qse_awk_nde_fncall_t* call, void* data)
{
	qse_awk_nde_t* p;
	qse_awk_val_t* v;
	qse_size_t nargs;
	const qse_char_t* fnc_arg_spec = (const qse_char_t*)data;

	for (p = call->args, nargs = 0; p != QSE_NULL; p = p->next, nargs++)
	{
		QSE_ASSERT (
			fnc_arg_spec == QSE_NULL ||
			(fnc_arg_spec != QSE_NULL && 
			 qse_strlen(fnc_arg_spec) > nargs));

		if (fnc_arg_spec && 
		    (fnc_arg_spec[nargs] == QSE_T('r') ||
		     fnc_arg_spec[0] == QSE_T('R')))
		{
			qse_awk_val_t** ref;
			      
			if (get_reference (rtx, p, &ref) == -1)
			{
				UNWIND_RTX_RTX_STACK_ARG (rtx, nargs);
				return (qse_size_t)-1;
			}

			/* p->type-QSE_AWK_NDE_NAMED assumes that the
			 * derived value matches QSE_AWK_VAL_REF_XXX */
			v = qse_awk_rtx_makerefval (
				rtx, p->type-QSE_AWK_NDE_NAMED, ref);
		}
		else if (fnc_arg_spec && 
		         fnc_arg_spec[nargs] == QSE_T('x'))
		{
			/* a regular expression is passed to 
			 * the function as it is */
			v = eval_expression0 (rtx, p);
		}
		else
		{
			v = eval_expression (rtx, p);
		}

		if (v == QSE_NULL)
		{
			UNWIND_RTX_RTX_STACK_ARG (rtx, nargs);
			return (qse_size_t)-1;
		}

		if (__raw_push(rtx,v) == -1) 
		{
			/* ugly - v needs to be freed if it doesn't have
			 * any reference. but its reference has not been 
			 * updated yet as it is carried out after the 
			 * successful stack push. so it adds up a reference 
			 * and dereferences it */
			qse_awk_rtx_refupval (rtx, v);
			qse_awk_rtx_refdownval (rtx, v);

			UNWIND_RTX_RTX_STACK_ARG (rtx, nargs);
			SETERR_LOC (rtx, QSE_AWK_ENOMEM, &call->loc);
			return (qse_size_t)-1;
		}

		qse_awk_rtx_refupval (rtx, v);
	}

	return nargs;
}

static qse_awk_val_t* eval_call (
	qse_awk_rtx_t* rtx, qse_awk_nde_t* nde, 
	const qse_char_t* fnc_arg_spec, qse_awk_fun_t* fun, 
	void(*errhandler)(void*), void* eharg)
{
	return __eval_call (
		rtx, nde, fnc_arg_spec, fun, 
		push_arg_from_nde, (void*)fnc_arg_spec,
		errhandler, eharg);
}

static int get_reference (
	qse_awk_rtx_t* run, qse_awk_nde_t* nde, qse_awk_val_t*** ref)
{
	qse_awk_nde_var_t* tgt = (qse_awk_nde_var_t*)nde;
	qse_awk_val_t** tmp;

	/* refer to eval_indexed for application of a similar concept */

	switch (nde->type)
	{
		case QSE_AWK_NDE_NAMED:
		{
			qse_htb_pair_t* pair;

			pair = qse_htb_search (
				run->named, tgt->id.name.ptr, tgt->id.name.len);
			if (pair == QSE_NULL)
			{
				/* it is bad that the named variable has to be
				 * created in the function named "__get_refernce".
				 * would there be any better ways to avoid this? */
				pair = qse_htb_upsert (
					run->named, tgt->id.name.ptr,
					tgt->id.name.len, qse_awk_val_nil, 0);
				if (pair == QSE_NULL) 
				{
					SETERR_LOC (run, QSE_AWK_ENOMEM, &nde->loc);
					return -1;
				}
			}

			*ref = (qse_awk_val_t**)&QSE_HTB_VPTR(pair);
			return 0;
		}
		
		case QSE_AWK_NDE_GBL:
			/* *ref = (qse_awk_val_t**)&RTX_STACK_GBL(run,tgt->id.idxa); */
			*ref = (qse_awk_val_t**)((qse_size_t)tgt->id.idxa);
			return 0;

		case QSE_AWK_NDE_LCL:
			*ref = (qse_awk_val_t**)&RTX_STACK_LCL(run,tgt->id.idxa);
			return 0;

		case QSE_AWK_NDE_ARG:
			*ref = (qse_awk_val_t**)&RTX_STACK_ARG(run,tgt->id.idxa);
			return 0;

		case QSE_AWK_NDE_NAMEDIDX:
		{
			qse_htb_pair_t* pair;

			pair = qse_htb_search (
				run->named, tgt->id.name.ptr, tgt->id.name.len);
			if (pair == QSE_NULL)
			{
				pair = qse_htb_upsert (
					run->named, tgt->id.name.ptr,
					tgt->id.name.len, qse_awk_val_nil, 0);
				if (pair == QSE_NULL) 
				{
					SETERR_LOC (run, QSE_AWK_ENOMEM, &nde->loc);
					return -1;
				}
			}

			tmp = get_reference_indexed (
				run, tgt, (qse_awk_val_t**)&QSE_HTB_VPTR(pair));
			if (tmp == QSE_NULL) return -1;
			*ref = tmp;
			return 0;
		}
		
		case QSE_AWK_NDE_GBLIDX:
			tmp = get_reference_indexed (run, tgt, 
				(qse_awk_val_t**)&RTX_STACK_GBL(run,tgt->id.idxa));
			if (tmp == QSE_NULL) return -1;
			*ref = tmp;
			return 0;

		case QSE_AWK_NDE_LCLIDX:
			tmp = get_reference_indexed (run, tgt, 
				(qse_awk_val_t**)&RTX_STACK_LCL(run,tgt->id.idxa));
			if (tmp == QSE_NULL) return -1;
			*ref = tmp;
			return 0;

		case QSE_AWK_NDE_ARGIDX:
			tmp = get_reference_indexed (run, tgt, 
				(qse_awk_val_t**)&RTX_STACK_ARG(run,tgt->id.idxa));
			if (tmp == QSE_NULL) return -1;
			*ref = tmp;
			return 0;
	
		case QSE_AWK_NDE_POS:
		{
			int n;
			qse_awk_int_t lv;
			qse_awk_val_t* v;
	
			/* the position number is returned for the positional 
			 * variable unlike other reference types. */
			v = eval_expression (run, ((qse_awk_nde_pos_t*)nde)->val);
			if (v == QSE_NULL) return -1;
	
			qse_awk_rtx_refupval (run, v);
			n = qse_awk_rtx_valtoint (run, v, &lv);
			qse_awk_rtx_refdownval (run, v);
	
			if (n <= -1) 
			{
				SETERR_LOC (run, QSE_AWK_EPOSIDX, &nde->loc);
				return -1;
			}
	
			if (!IS_VALID_POSIDX(lv)) 
			{
				SETERR_LOC (run, QSE_AWK_EPOSIDX, &nde->loc);
				return -1;
			}

			*ref = (qse_awk_val_t**)((qse_size_t)lv);
			return 0;
		}

		default:
			SETERR_LOC (run, QSE_AWK_ENOTREF, &nde->loc);
			return -1;
	}
}

static qse_awk_val_t** get_reference_indexed (
	qse_awk_rtx_t* run, qse_awk_nde_var_t* nde, qse_awk_val_t** val)
{
	qse_htb_pair_t* pair;
	qse_char_t* str;
	qse_size_t len;
	qse_char_t idxbuf[IDXBUFSIZE];

	QSE_ASSERT (val != QSE_NULL);

	if ((*val)->type == QSE_AWK_VAL_NIL)
	{
		qse_awk_val_t* tmp;

		tmp = qse_awk_rtx_makemapval (run);
		if (tmp == QSE_NULL)
		{
			ADJERR_LOC (run, &nde->loc);
			return QSE_NULL;
		}

		qse_awk_rtx_refdownval (run, *val);
		*val = tmp;
		qse_awk_rtx_refupval (run, (qse_awk_val_t*)*val);
	}
	else if ((*val)->type != QSE_AWK_VAL_MAP) 
	{
		SETERR_LOC (run, QSE_AWK_ENOTMAP, &nde->loc);
		return QSE_NULL;
	}

	QSE_ASSERT (nde->idx != QSE_NULL);

	len = QSE_COUNTOF(idxbuf);
	str = idxnde_to_str (run, nde->idx, idxbuf, &len);
	if (str == QSE_NULL) return QSE_NULL;

	pair = qse_htb_search ((*(qse_awk_val_map_t**)val)->map, str, len);
	if (pair == QSE_NULL)
	{
		pair = qse_htb_upsert (
			(*(qse_awk_val_map_t**)val)->map, 
			str, len, qse_awk_val_nil, 0);
		if (pair == QSE_NULL)
		{
			if (str != idxbuf) QSE_AWK_FREE (run->awk, str);
			SETERR_LOC (run, QSE_AWK_ENOMEM, &nde->loc);
			return QSE_NULL;
		}

		qse_awk_rtx_refupval (run, QSE_HTB_VPTR(pair));
	}

	if (str != idxbuf) QSE_AWK_FREE (run->awk, str);
	return (qse_awk_val_t**)&QSE_HTB_VPTR(pair);
}

static qse_awk_val_t* eval_int (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_val_t* val;

	val = qse_awk_rtx_makeintval (run, ((qse_awk_nde_int_t*)nde)->val);
	if (val == QSE_NULL) ADJERR_LOC (run, &nde->loc);
	else ((qse_awk_val_int_t*)val)->nde = nde;

	return val;
}

static qse_awk_val_t* eval_real (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_val_t* val;

	val = qse_awk_rtx_makefltval (run, ((qse_awk_nde_flt_t*)nde)->val);
	if (val == QSE_NULL) ADJERR_LOC (run, &nde->loc);
	else ((qse_awk_val_flt_t*)val)->nde = nde;

	return val;
}

static qse_awk_val_t* eval_str (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_val_t* val;

	val = qse_awk_rtx_makestrval (run,
		((qse_awk_nde_str_t*)nde)->ptr,
		((qse_awk_nde_str_t*)nde)->len);
	if (val == QSE_NULL) ADJERR_LOC (run, &nde->loc);

	return val;
}

static qse_awk_val_t* eval_rex (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_val_t* val;

	val = qse_awk_rtx_makerexval (run,
		&((qse_awk_nde_rex_t*)nde)->str,
		((qse_awk_nde_rex_t*)nde)->code
	);
	if (val == QSE_NULL) ADJERR_LOC (run, &nde->loc);

	return val;
}

static qse_awk_val_t* eval_named (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_htb_pair_t* pair;
		       
	pair = qse_htb_search (
		run->named, 
		((qse_awk_nde_var_t*)nde)->id.name.ptr, 
		((qse_awk_nde_var_t*)nde)->id.name.len
	);

	return (pair == QSE_NULL)? qse_awk_val_nil: QSE_HTB_VPTR(pair);
}

static qse_awk_val_t* eval_gbl (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	return RTX_STACK_GBL(run,((qse_awk_nde_var_t*)nde)->id.idxa);
}

static qse_awk_val_t* eval_lcl (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	return RTX_STACK_LCL(run,((qse_awk_nde_var_t*)nde)->id.idxa);
}

static qse_awk_val_t* eval_arg (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	return RTX_STACK_ARG(run,((qse_awk_nde_var_t*)nde)->id.idxa);
}

static qse_awk_val_t* eval_indexed (
	qse_awk_rtx_t* run, qse_awk_nde_var_t* nde, qse_awk_val_t** val)
{
	qse_htb_pair_t* pair;
	qse_char_t* str;
	qse_size_t len;
	qse_char_t idxbuf[IDXBUFSIZE];

	QSE_ASSERT (val != QSE_NULL);

	if ((*val)->type == QSE_AWK_VAL_NIL)
	{
		qse_awk_val_t* tmp;

		tmp = qse_awk_rtx_makemapval (run);
		if (tmp == QSE_NULL)
		{
			ADJERR_LOC (run, &nde->loc);
			return QSE_NULL;
		}

		qse_awk_rtx_refdownval (run, *val);
		*val = tmp;
		qse_awk_rtx_refupval (run, (qse_awk_val_t*)*val);
	}
	else if ((*val)->type != QSE_AWK_VAL_MAP) 
	{
		SETERR_LOC (run, QSE_AWK_ENOTMAP, &nde->loc);
		return QSE_NULL;
	}

	QSE_ASSERT (nde->idx != QSE_NULL);

	len = QSE_COUNTOF(idxbuf);
	str = idxnde_to_str (run, nde->idx, idxbuf, &len);
	if (str == QSE_NULL) return QSE_NULL;

	pair = qse_htb_search ((*(qse_awk_val_map_t**)val)->map, str, len);
	if (str != idxbuf) QSE_AWK_FREE (run->awk, str);

	return (pair == QSE_NULL)? qse_awk_val_nil: 
	                           (qse_awk_val_t*)QSE_HTB_VPTR(pair);
}

static qse_awk_val_t* eval_namedidx (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_nde_var_t* tgt = (qse_awk_nde_var_t*)nde;
	qse_htb_pair_t* pair;

	pair = qse_htb_search (run->named, tgt->id.name.ptr, tgt->id.name.len);
	if (pair == QSE_NULL)
	{
		pair = qse_htb_upsert (run->named, 
			tgt->id.name.ptr, tgt->id.name.len, qse_awk_val_nil, 0);
		if (pair == QSE_NULL) 
		{
			SETERR_LOC (run, QSE_AWK_ENOMEM, &nde->loc);
			return QSE_NULL;
		}

		qse_awk_rtx_refupval (run, QSE_HTB_VPTR(pair)); 
	}

	return eval_indexed (run, tgt, (qse_awk_val_t**)&QSE_HTB_VPTR(pair));
}

static qse_awk_val_t* eval_gblidx (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	return eval_indexed (run, (qse_awk_nde_var_t*)nde, 
		(qse_awk_val_t**)&RTX_STACK_GBL(run,((qse_awk_nde_var_t*)nde)->id.idxa));
}

static qse_awk_val_t* eval_lclidx (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	return eval_indexed (run, (qse_awk_nde_var_t*)nde, 
		(qse_awk_val_t**)&RTX_STACK_LCL(run,((qse_awk_nde_var_t*)nde)->id.idxa));
}

static qse_awk_val_t* eval_argidx (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	return eval_indexed (run, (qse_awk_nde_var_t*)nde,
		(qse_awk_val_t**)&RTX_STACK_ARG(run,((qse_awk_nde_var_t*)nde)->id.idxa));
}

static qse_awk_val_t* eval_pos (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	qse_awk_nde_pos_t* pos = (qse_awk_nde_pos_t*)nde;
	qse_awk_val_t* v;
	qse_awk_int_t lv;
	int n;

	v = eval_expression (run, pos->val);
	if (v == QSE_NULL) return QSE_NULL;

	qse_awk_rtx_refupval (run, v);
	n = qse_awk_rtx_valtoint (run, v, &lv);
	qse_awk_rtx_refdownval (run, v);
	if (n <= -1) 
	{
		SETERR_LOC (run, QSE_AWK_EPOSIDX, &nde->loc);
		return QSE_NULL;
	}

	if (lv < 0)
	{
		SETERR_LOC (run, QSE_AWK_EPOSIDX, &nde->loc);
		return QSE_NULL;
	}
	if (lv == 0) v = run->inrec.d0;
	else if (lv > 0 && lv <= (qse_awk_int_t)run->inrec.nflds) 
		v = run->inrec.flds[lv-1].val;
	else v = qse_awk_val_zls; /*qse_awk_val_nil;*/

	return v;
}

static qse_awk_val_t* eval_getline (qse_awk_rtx_t* rtx, qse_awk_nde_t* nde)
{
	qse_awk_nde_getline_t* p;
	qse_awk_val_t* v, * tmp;
	qse_char_t* dst;
	qse_str_t* buf;
	int n, x;

	p = (qse_awk_nde_getline_t*)nde;

	QSE_ASSERT (
		(p->in_type == QSE_AWK_IN_PIPE && p->in != QSE_NULL) ||
		(p->in_type == QSE_AWK_IN_RWPIPE && p->in != QSE_NULL) ||
		(p->in_type == QSE_AWK_IN_FILE && p->in != QSE_NULL) ||
		(p->in_type == QSE_AWK_IN_CONSOLE && p->in == QSE_NULL));

	if (p->in)
	{
		qse_size_t len;
		qse_awk_rtx_valtostr_out_t out;

		v = eval_expression (rtx, p->in);
		if (v == QSE_NULL) return QSE_NULL;

		qse_awk_rtx_refupval (rtx, v);
		if (v->type == QSE_AWK_VAL_STR)
		{
			dst = ((qse_awk_val_str_t*)v)->val.ptr;
			len = ((qse_awk_val_str_t*)v)->val.len;
		}
		else
		{
			out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
			x = qse_awk_rtx_valtostr (rtx, v, &out);
			qse_awk_rtx_refdownval (rtx, v);
			if (x <= -1) return QSE_NULL;

			dst = out.u.cpldup.ptr;
			len = out.u.cpldup.len;
		}

		if (len <= 0) 
		{
			/* the input source name is empty.
			 * make getline return -1 */
			if (v->type == QSE_AWK_VAL_STR) 
				qse_awk_rtx_refdownval (rtx, v);
			else QSE_AWK_FREE (rtx->awk, dst);

			n = -1;
			goto skip_read;
		}

		while (len > 0)
		{
			if (dst[--len] == QSE_T('\0'))
			{
				/* the input source name contains a null 
				 * character. make getline return -1.
				 * unlike print & printf, it is not a hard
				 * error  */
				if (v->type == QSE_AWK_VAL_STR) 
					qse_awk_rtx_refdownval (rtx, v);
				else QSE_AWK_FREE (rtx->awk, dst);

				n = -1;
				goto skip_read;
			}
		}
	}
	else dst = (qse_char_t*)QSE_T("");

	buf = &rtx->inrec.lineg;
read_console_again:
	qse_str_clear (&rtx->inrec.lineg);

	n = qse_awk_rtx_readio (rtx, p->in_type, dst, buf);

	if (p->in)
	{
		if (v->type == QSE_AWK_VAL_STR) 
			qse_awk_rtx_refdownval (rtx, v);
		else QSE_AWK_FREE (rtx->awk, dst);
	}

	if (n <= -1) 
	{
		/* make getline return -1 */
		n = -1;
	}
	else if (n > 0)
	{
		if (p->in_type == QSE_AWK_IN_CONSOLE)
		{
			QSE_ASSERT (p->in == QSE_NULL);
			if (rtx->nrflt.limit > 0)
			{
				if (((rtx->gbl.nr / rtx->nrflt.limit) % rtx->nrflt.size) != rtx->nrflt.rank)
				{
					if (update_fnr (rtx, rtx->gbl.fnr + 1, rtx->gbl.nr + 1) <= -1) return QSE_NULL;
					/* this jump is a bit dirty. the 'if' block below qse_awk_rtx_readion()
					 * will never be true. but this makes code confusing */
					goto read_console_again; 
				}
			}
		}

		if (p->var == QSE_NULL)
		{
			/* set $0 with the input value */
			x = qse_awk_rtx_setrec (rtx, 0, QSE_STR_CSTR(buf));
			if (x <= -1) return QSE_NULL;
		}
		else
		{
			qse_awk_val_t* v;

			v = qse_awk_rtx_makestrvalwithcstr (rtx, QSE_STR_CSTR(buf));
			if (v == QSE_NULL)
			{
				ADJERR_LOC (rtx, &nde->loc);
				return QSE_NULL;
			}

			qse_awk_rtx_refupval (rtx, v);
			tmp = do_assignment(rtx, p->var, v);
			qse_awk_rtx_refdownval (rtx, v);
			if (tmp == QSE_NULL) return QSE_NULL;
		}

		/* update FNR & NR if reading from console */
		if (p->in_type == QSE_AWK_IN_CONSOLE &&
		    update_fnr (rtx, rtx->gbl.fnr + 1, rtx->gbl.nr + 1) <= -1) 
		{
			return QSE_NULL;
		}
	}
	
skip_read:
	tmp = qse_awk_rtx_makeintval (rtx, n);
	if (tmp == QSE_NULL) ADJERR_LOC (rtx, &nde->loc);
	return tmp;
}

static qse_awk_val_t* eval_print (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	int n = run_print (run, (qse_awk_nde_print_t*)nde);
	if (n == PRINT_IOERR) n = -1; /* let print return -1 */
	else if (n <= -1) return QSE_NULL;
	return qse_awk_rtx_makeintval (run, n);
}

static qse_awk_val_t* eval_printf (qse_awk_rtx_t* run, qse_awk_nde_t* nde)
{
	int n = run_printf (run, (qse_awk_nde_print_t*)nde);
	if (n == PRINT_IOERR) n = -1; /* let print return -1 */
	else if (n <= -1) return QSE_NULL;
	return qse_awk_rtx_makeintval (run, n);
}

static int __raw_push (qse_awk_rtx_t* run, void* val)
{
	if (run->stack_top >= run->stack_limit)
	{
		void** tmp;
		qse_size_t n;
	       
		n = run->stack_limit + RTX_STACK_INCREMENT;

		tmp = (void**) QSE_AWK_REALLOC (
			run->awk, run->stack, n * QSE_SIZEOF(void*)); 
		if (tmp == QSE_NULL) return -1;

		run->stack = tmp;
		run->stack_limit = n;
	}

	run->stack[run->stack_top++] = val;
	return 0;
}

static int read_record (qse_awk_rtx_t* rtx)
{
	qse_ssize_t n;
	qse_str_t* buf;

read_again:
	if (qse_awk_rtx_clrrec (rtx, 0) == -1) return -1;

	buf = &rtx->inrec.line;
	n = qse_awk_rtx_readio (rtx, QSE_AWK_IN_CONSOLE, QSE_T(""), buf);
	if (n <= -1) 
	{
		qse_awk_rtx_clrrec (rtx, 0);
		return -1;
	}

#ifdef DEBUG_RUN
	qse_errputstrf (QSE_T("record len = %d str=[%.*s]\n"), 
		(int)QSE_STR_LEN(buf), (int)QSE_STR_LEN(buf), QSE_STR_PTR(buf));
#endif
	if (n == 0) 
	{
		QSE_ASSERT (QSE_STR_LEN(buf) == 0);
		return 0;
	}

	if (rtx->nrflt.limit > 0)
	{
		if (((rtx->gbl.nr / rtx->nrflt.limit) % rtx->nrflt.size) != rtx->nrflt.rank)
		{
			if (update_fnr (rtx, rtx->gbl.fnr + 1, rtx->gbl.nr + 1) <= -1) return -1;
			goto read_again;
		}
	}

	if (qse_awk_rtx_setrec (rtx, 0, QSE_STR_CSTR(buf)) <= -1 ||
	    update_fnr (rtx, rtx->gbl.fnr + 1, rtx->gbl.nr + 1) <= -1) return -1;

	return 1;
}

static int shorten_record (qse_awk_rtx_t* run, qse_size_t nflds)
{
	qse_awk_val_t* v;
	qse_char_t* ofs_free = QSE_NULL, * ofs_ptr;
	qse_size_t ofs_len, i;
	qse_str_t tmp;

	QSE_ASSERT (nflds <= run->inrec.nflds);

	if (nflds > 1)
	{
		v = RTX_STACK_GBL(run, QSE_AWK_GBL_OFS);
		qse_awk_rtx_refupval (run, v);

		if (v->type == QSE_AWK_VAL_NIL)
		{
			/* OFS not set */
			ofs_ptr = QSE_T(" ");
			ofs_len = 1;
		}
		else if (v->type == QSE_AWK_VAL_STR)
		{
			ofs_ptr = ((qse_awk_val_str_t*)v)->val.ptr;
			ofs_len = ((qse_awk_val_str_t*)v)->val.len;
		}
		else
		{
			qse_awk_rtx_valtostr_out_t out;

			out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
			if (qse_awk_rtx_valtostr (run, v, &out) <= -1) return -1;

			ofs_ptr = out.u.cpldup.ptr;
			ofs_len = out.u.cpldup.len;
			ofs_free = ofs_ptr;
		}
	}

	if (qse_str_init (
		&tmp, MMGR(run), QSE_STR_LEN(&run->inrec.line)) <= -1)
	{
		if (ofs_free != QSE_NULL) 
			QSE_AWK_FREE (run->awk, ofs_free);
		if (nflds > 1) qse_awk_rtx_refdownval (run, v);
		SETERR_COD (run, QSE_AWK_ENOMEM);
		return -1;
	}

	for (i = 0; i < nflds; i++)
	{
		if (i > 0 && qse_str_ncat(&tmp,ofs_ptr,ofs_len) == (qse_size_t)-1)
		{
			qse_str_fini (&tmp);
			if (ofs_free != QSE_NULL) 
				QSE_AWK_FREE (run->awk, ofs_free);
			if (nflds > 1) qse_awk_rtx_refdownval (run, v);
			SETERR_COD (run, QSE_AWK_ENOMEM);
			return -1;
		}

		if (qse_str_ncat (&tmp, 
			run->inrec.flds[i].ptr, 
			run->inrec.flds[i].len) == (qse_size_t)-1)
		{
			qse_str_fini (&tmp);
			if (ofs_free != QSE_NULL) 
				QSE_AWK_FREE (run->awk, ofs_free);
			if (nflds > 1) qse_awk_rtx_refdownval (run, v);
			SETERR_COD (run, QSE_AWK_ENOMEM);
			return -1;
		}
	}

	if (ofs_free != QSE_NULL) QSE_AWK_FREE (run->awk, ofs_free);
	if (nflds > 1) qse_awk_rtx_refdownval (run, v);

	v = (qse_awk_val_t*) qse_awk_rtx_makestrvalwithcstr (run, QSE_STR_CSTR(&tmp));
	if (v == QSE_NULL) 
	{
		qse_str_fini (&tmp);
		return -1;
	}

	qse_awk_rtx_refdownval (run, run->inrec.d0);
	run->inrec.d0 = v;
	qse_awk_rtx_refupval (run, run->inrec.d0);

	qse_str_swap (&tmp, &run->inrec.line);
	qse_str_fini (&tmp);

	for (i = nflds; i < run->inrec.nflds; i++)
	{
		qse_awk_rtx_refdownval (run, run->inrec.flds[i].val);
	}

	run->inrec.nflds = nflds;
	return 0;
}

static qse_char_t* idxnde_to_str (
	qse_awk_rtx_t* run, qse_awk_nde_t* nde, qse_char_t* buf, qse_size_t* len)
{
	qse_char_t* str;
	qse_awk_val_t* idx;

	QSE_ASSERT (nde != QSE_NULL);

	if (nde->next == QSE_NULL)
	{
		qse_awk_rtx_valtostr_out_t out;

		/* single node index */
		idx = eval_expression (run, nde);
		if (idx == QSE_NULL) return QSE_NULL;

		qse_awk_rtx_refupval (run, idx);

		str = QSE_NULL;

		if (buf != QSE_NULL)
		{
			/* try with a fixed-size buffer if given */
			out.type = QSE_AWK_RTX_VALTOSTR_CPLCPY;
			out.u.cplcpy.ptr = buf;
			out.u.cplcpy.len = *len;

			if (qse_awk_rtx_valtostr (run, idx, &out) >= 0)
			{
				str = out.u.cplcpy.ptr;
				*len = out.u.cplcpy.len;
				QSE_ASSERT (str == buf);
			}
		}

		if (str == QSE_NULL)
		{
			/* if no fixed-size buffer was given or the fixed-size 
			 * conversion failed, switch to the dynamic mode */
			out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
			if (qse_awk_rtx_valtostr (run, idx, &out) <= -1)
			{
				qse_awk_rtx_refdownval (run, idx);
				ADJERR_LOC (run, &nde->loc);
				return QSE_NULL;
			}

			str = out.u.cpldup.ptr;
			*len = out.u.cpldup.len;
		}

		qse_awk_rtx_refdownval (run, idx);
	}
	else
	{
		/* multidimensional index */
		qse_str_t idxstr;
		qse_xstr_t tmp;
		qse_awk_rtx_valtostr_out_t out;

		out.type = QSE_AWK_RTX_VALTOSTR_STRPCAT;
		out.u.strpcat = &idxstr;

		if (qse_str_init (&idxstr, MMGR(run), DEF_BUF_CAPA) <= -1) 
		{
			SETERR_LOC (run, QSE_AWK_ENOMEM, &nde->loc);
			return QSE_NULL;
		}

		while (nde != QSE_NULL)
		{
			idx = eval_expression (run, nde);
			if (idx == QSE_NULL) 
			{
				qse_str_fini (&idxstr);
				return QSE_NULL;
			}

			qse_awk_rtx_refupval (run, idx);

			if (QSE_STR_LEN(&idxstr) > 0 &&
			    qse_str_ncat (&idxstr, 
			    	run->gbl.subsep.ptr, 
			    	run->gbl.subsep.len) == (qse_size_t)-1)
			{
				qse_awk_rtx_refdownval (run, idx);
				qse_str_fini (&idxstr);
				SETERR_LOC (run, QSE_AWK_ENOMEM, &nde->loc);
				return QSE_NULL;
			}

			if (qse_awk_rtx_valtostr (run, idx, &out) <= -1)
			{
				qse_awk_rtx_refdownval (run, idx);
				qse_str_fini (&idxstr);
				return QSE_NULL;
			}

			qse_awk_rtx_refdownval (run, idx);
			nde = nde->next;
		}

		qse_str_yield (&idxstr, &tmp, 0);
		str = tmp.ptr;
		*len = tmp.len;

		qse_str_fini (&idxstr);
	}

	return str;
}

qse_char_t* qse_awk_rtx_format (
	qse_awk_rtx_t* rtx, qse_str_t* out, qse_str_t* fbu,
	const qse_char_t* fmt, qse_size_t fmt_len, 
	qse_size_t nargs_on_stack, qse_awk_nde_t* args, qse_size_t* len)
{
	qse_size_t i;
	qse_size_t stack_arg_idx = 1;
	qse_awk_val_t* val;

#define OUT_CHAR(c) QSE_BLOCK( \
	if (qse_str_ccat (out, (c)) == -1) \
	{ \
		SETERR_COD (rtx, QSE_AWK_ENOMEM); \
		return QSE_NULL; \
	} \
)

#define OUT_STR(ptr,len) QSE_BLOCK( \
	if (qse_str_ncat (out, (ptr), (len)) == -1) \
	{ \
		SETERR_COD (rtx, QSE_AWK_ENOMEM); \
		return QSE_NULL; \
	} \
)

#define FMT_CHAR(c) QSE_BLOCK( \
	if (qse_str_ccat (fbu, (c)) == -1) \
	{ \
		SETERR_COD (rtx, QSE_AWK_ENOMEM); \
		return QSE_NULL; \
	} \
)

#define FMT_STR(ptr,len) QSE_BLOCK( \
	if (qse_str_ncat (fbu, (ptr), (len)) == -1) \
	{ \
		SETERR_COD (rtx, QSE_AWK_ENOMEM); \
		return QSE_NULL; \
	} \
)

#define GROW(buf) QSE_BLOCK( \
	if ((buf)->ptr) \
	{ \
		QSE_AWK_FREE (rtx->awk, (buf)->ptr); \
		(buf)->ptr = QSE_NULL; \
	} \
	(buf)->len += (buf)->inc; \
	(buf)->ptr = (qse_char_t*)QSE_AWK_ALLOC ( \
		rtx->awk, (buf)->len * QSE_SIZEOF(qse_char_t)); \
	if ((buf)->ptr == QSE_NULL) \
	{ \
		SETERR_COD (rtx, QSE_AWK_ENOMEM); \
		(buf)->len = 0; \
		return QSE_NULL; \
	} \
)

#define GROW_WITH_INC(buf,incv) QSE_BLOCK( \
	if ((buf)->ptr) \
	{ \
		QSE_AWK_FREE (rtx->awk, (buf)->ptr); \
		(buf)->ptr = QSE_NULL; \
	} \
	(buf)->len += ((incv) > (buf)->inc)? (incv): (buf)->inc; \
	(buf)->ptr = (qse_char_t*)QSE_AWK_ALLOC ( \
		rtx->awk, (buf)->len * QSE_SIZEOF(qse_char_t)); \
	if ((buf)->ptr == QSE_NULL) \
	{ \
		SETERR_COD (rtx, QSE_AWK_ENOMEM); \
		(buf)->len = 0; \
		return QSE_NULL; \
	} \
)

	QSE_ASSERTX (rtx->format.tmp.ptr != QSE_NULL,
		"run->format.tmp.ptr should have been assigned a pointer to a block of memory before this function has been called");

	if (nargs_on_stack == (qse_size_t)-1) 
	{
		val = (qse_awk_val_t*)args;
		nargs_on_stack = 2;
	}
	else 
	{
		val = QSE_NULL;
	}

	if (out == QSE_NULL) out = &rtx->format.out;
	if (fbu == QSE_NULL) fbu = &rtx->format.fmt;

	qse_str_clear (out);
	qse_str_clear (fbu);

	for (i = 0; i < fmt_len; i++)
	{
		qse_awk_int_t wp[2];
		int wp_idx;
#define WP_WIDTH     0
#define WP_PRECISION 1

		int flags;
#define FLAG_SPACE (1 << 0)
#define FLAG_HASH  (1 << 1)
#define FLAG_ZERO  (1 << 2)
#define FLAG_PLUS  (1 << 3)
#define FLAG_MINUS (1 << 4)

		if (QSE_STR_LEN(fbu) == 0)
		{
			/* format specifier is empty */
			if (fmt[i] == QSE_T('%')) 
			{
				/* add % to format specifier (fbu) */
				FMT_CHAR (fmt[i]);
			}
			else 
			{
				/* normal output */
				OUT_CHAR (fmt[i]);
			}
			continue;
		}

		/* handle flags */
		flags = 0;
		while (i < fmt_len)
		{
			switch (fmt[i])
			{
				case QSE_T(' '):
					flags |= FLAG_SPACE;
					break;
				case QSE_T('#'):
					flags |= FLAG_HASH;
					break;
				case QSE_T('0'):
					flags |= FLAG_ZERO;
					break;
				case QSE_T('+'):
					flags |= FLAG_PLUS;
					break;
				case QSE_T('-'):
					flags |= FLAG_MINUS;
					break;
				default:
					goto wp_mod_init;
			}

			FMT_CHAR (fmt[i]); i++;
		}

wp_mod_init:
		wp[WP_WIDTH] = -1; /* width */
		wp[WP_PRECISION] = -1; /* precision */
		wp_idx = WP_WIDTH; /* width first */

wp_mod_main:
		if (i < fmt_len && fmt[i] == QSE_T('*'))
		{
			/* variable width/precision modifier.
			 * take the width/precision from a parameter and
			 * transform it to a fixed length format */
			qse_awk_val_t* v;
			int n;

			if (args == QSE_NULL)
			{
				if (stack_arg_idx >= nargs_on_stack)
				{
					SETERR_COD (rtx, QSE_AWK_EFMTARG);
					return QSE_NULL;
				}
				v = qse_awk_rtx_getarg (rtx, stack_arg_idx);
			}
			else
			{
				if (val) 
				{
					if (stack_arg_idx >= nargs_on_stack)
					{
						SETERR_COD (rtx, QSE_AWK_EFMTARG);
						return QSE_NULL;
					}
					v = val;
				}
				else 
				{
					v = eval_expression (rtx, args);
					if (v == QSE_NULL) return QSE_NULL;
				}
			}

			qse_awk_rtx_refupval (rtx, v);
			n = qse_awk_rtx_valtoint (rtx, v, &wp[wp_idx]);
			qse_awk_rtx_refdownval (rtx, v);
			if (n <= -1) return QSE_NULL; 

			do
			{
				n = qse_fmtintmax (
					rtx->format.tmp.ptr,
					rtx->format.tmp.len,
					wp[wp_idx], 
					10 | QSE_FMTINTMAX_NOTRUNC | QSE_FMTINTMAX_NONULL,
					-1,
					QSE_T('\0'),
					QSE_NULL
				);	
				if (n <= -1)
				{
					/* -n is the number of characters required
					 * including terminating null  */
					GROW_WITH_INC (&rtx->format.tmp, -n);
					continue;
				}

				break;
			}
			while (1);

			FMT_STR(rtx->format.tmp.ptr, n);

			if (args == QSE_NULL || val != QSE_NULL) stack_arg_idx++;
			else args = args->next;
			i++;
		}
		else
		{
			/* fixed width/precision modifier */
			if (i < fmt_len && QSE_AWK_ISDIGIT(rtx->awk, fmt[i]))
			{
				wp[wp_idx] = 0;
				do
				{
					wp[wp_idx] = wp[wp_idx] * 10 + fmt[i] - QSE_T('0');
					FMT_CHAR (fmt[i]); i++;
				}
				while (i < fmt_len && QSE_AWK_ISDIGIT(rtx->awk, fmt[i]));
			}
		}

		if (wp_idx == WP_WIDTH && i < fmt_len && fmt[i] == QSE_T('.'))
		{
			wp[WP_PRECISION] = 0;
			FMT_CHAR (fmt[i]); i++;

			wp_idx = WP_PRECISION; /* change index to precision */
			goto wp_mod_main;
		}

		if (i >= fmt_len) break;

		if (fmt[i] == QSE_T('d') || fmt[i] == QSE_T('i') || 
		    fmt[i] == QSE_T('x') || fmt[i] == QSE_T('X') ||
		    fmt[i] == QSE_T('b') || fmt[i] == QSE_T('B') ||
		    fmt[i] == QSE_T('o'))
		{
			qse_awk_val_t* v;
			qse_awk_int_t l;
			int n;

			int fmt_flags;
			int fmt_uint = 0;
			int fmt_width;
			qse_char_t fmt_fill = QSE_T('\0');
			const qse_char_t* fmt_prefix = QSE_NULL;

			if (args == QSE_NULL)
			{
				if (stack_arg_idx >= nargs_on_stack)
				{
					SETERR_COD (rtx, QSE_AWK_EFMTARG);
					return QSE_NULL;
				}
				v = qse_awk_rtx_getarg (rtx, stack_arg_idx);
			}
			else
			{
				if (val != QSE_NULL) 
				{
					if (stack_arg_idx >= nargs_on_stack)
					{
						SETERR_COD (rtx, QSE_AWK_EFMTARG);
						return QSE_NULL;
					}
					v = val;
				}
				else 
				{
					v = eval_expression (rtx, args);
					if (v == QSE_NULL) return QSE_NULL;
				}
			}

			qse_awk_rtx_refupval (rtx, v);
			n = qse_awk_rtx_valtoint (rtx, v, &l);
			qse_awk_rtx_refdownval (rtx, v);
			if (n <= -1) return QSE_NULL; 

			fmt_flags = QSE_FMTINTMAX_NOTRUNC | QSE_FMTINTMAX_NONULL;

			if (l == 0 && wp[WP_PRECISION] == 0)
			{
				/* A zero value with a precision of zero produces
				 * no character. */
				fmt_flags |= QSE_FMTINTMAX_NOZERO;
			}

			if (wp[WP_WIDTH] != -1)
			{
				QSE_ASSERTX (wp[WP_WIDTH] > 0, "Width must be greater than 0 if specified"); 

				/* justification when width is specified. */
				if (flags & FLAG_ZERO)
				{
					if (flags & FLAG_MINUS)
					{
						 /* FLAG_MINUS wins if both FLAG_ZERO 
						  * and FLAG_MINUS are specified. */
						fmt_fill = QSE_T(' ');
						if (flags & FLAG_MINUS)
						{
							/* left justification. need to fill the right side */
							fmt_flags |= QSE_FMTINTMAX_FILLRIGHT;
						}
					}
					else
					{
						if (wp[WP_PRECISION] == -1)
						{
							/* precision not specified. 
							 * FLAG_ZERO can take effect */
							fmt_fill = QSE_T('0');
							fmt_flags |= QSE_FMTINTMAX_FILLCENTER;
						}
						else
						{
							fmt_fill = QSE_T(' ');
						}
					}
				}
				else
				{
					fmt_fill = QSE_T(' ');
					if (flags & FLAG_MINUS)
					{
						/* left justification. need to fill the right side */
						fmt_flags |= QSE_FMTINTMAX_FILLRIGHT;
					}
				}
			}

			switch (fmt[i])
			{
				case QSE_T('B'):
				case QSE_T('b'):
					fmt_flags |= 2;
					fmt_uint = 1;
					if (l && (flags & FLAG_HASH)) 
					{
						/* A nonzero value is prefixed with 0b */
						fmt_prefix = QSE_T("0b");
					}
					break;

				case QSE_T('X'):
					fmt_flags |= QSE_FMTINTMAX_UPPERCASE;
				case QSE_T('x'):
					fmt_flags |= 16;
					fmt_uint = 1;
					if (l && (flags & FLAG_HASH)) 
					{
						/* A nonzero value is prefixed with 0x */
						fmt_prefix = QSE_T("0x");
					}
					break;

				case QSE_T('o'):
					fmt_flags |= 8;
					fmt_uint = 1;
					if (flags & FLAG_HASH)
					{
						/* Force a leading zero digit including zero.
						 * 0 with FLAG_HASH and precision 0 still emits '0'.
						 * On the contrary, 'X' and 'x' emit no digits
						 * for 0 with FLAG_HASH and precision 0. */
						fmt_flags |= QSE_FMTINTMAX_ZEROLEAD;
					}
					break;
	
				default:
					fmt_flags |= 10;
					if (flags & FLAG_PLUS) 
						fmt_flags |= QSE_FMTINTMAX_PLUSSIGN;
					if (flags & FLAG_SPACE)
						fmt_flags |= QSE_FMTINTMAX_EMPTYSIGN;
					break;
			}


			if (wp[WP_WIDTH] > 0)
			{
				if (wp[WP_WIDTH] > rtx->format.tmp.len)
					GROW_WITH_INC (&rtx->format.tmp, wp[WP_WIDTH] - rtx->format.tmp.len);
				fmt_width = wp[WP_WIDTH];
			}
			else fmt_width = rtx->format.tmp.len;
			
			do
			{
				if (fmt_uint)
				{
					/* Explicit type-casting for 'l' from qse_awk_int_t 
					 * to qse_awk_uint_t is needed before passing it to 
					 * qse_fmtuintmax(). 
 					 *
					 * Consider a value of -1 for example. 
					 * -1 is a value with all bits set. 
					 * If qse_awk_int_t is 4 bytes and qse_uintmax_t
					 * is 8 bytes, the value is shown below for 
					 * each type respectively .
					 *     -1 - 0xFFFFFFFF (qse_awk_int_t)
					 *     -1 - 0xFFFFFFFFFFFFFFFF (qse_uintmax_t)
					 * Implicit typecasting of -1 from qse_awk_int_t to
					 * to qse_uintmax_t results in 0xFFFFFFFFFFFFFFFF,
					 * though 0xFFFFFFF is expected in hexadecimal.
					 */
					n = qse_fmtuintmax (
						rtx->format.tmp.ptr,
						fmt_width,
						(qse_awk_uint_t)l, 
						fmt_flags,
						wp[WP_PRECISION],
						fmt_fill,
						fmt_prefix
					);
				}
				else
				{
					n = qse_fmtintmax (
						rtx->format.tmp.ptr,
						fmt_width,
						l,
						fmt_flags,
						wp[WP_PRECISION],
						fmt_fill,
						fmt_prefix
					);
				}
				if (n <= -1)
				{
					/* -n is the number of characters required */
					GROW_WITH_INC (&rtx->format.tmp, -n);
					fmt_width = -n;
					continue;
				}

				break;
			}
			while (1);

			OUT_STR (rtx->format.tmp.ptr, n);
		}
		else if (fmt[i] == QSE_T('e') || fmt[i] == QSE_T('E') ||
		         fmt[i] == QSE_T('g') || fmt[i] == QSE_T('G') ||
		         fmt[i] == QSE_T('f'))
		{
			qse_awk_val_t* v;
			qse_awk_flt_t r;
			int n;
	
		#if defined(QSE_USE_AWK_FLTMAX)
			FMT_CHAR (QSE_T('j'));
		#else
			FMT_CHAR (QSE_T('z'));
		#endif
			FMT_CHAR (fmt[i]);

			if (args == QSE_NULL)
			{
				if (stack_arg_idx >= nargs_on_stack)
				{
					SETERR_COD (rtx, QSE_AWK_EFMTARG);
					return QSE_NULL;
				}
				v = qse_awk_rtx_getarg (rtx, stack_arg_idx);
			}
			else
			{
				if (val != QSE_NULL) 
				{
					if (stack_arg_idx >= nargs_on_stack)
					{
						SETERR_COD (rtx, QSE_AWK_EFMTARG);
						return QSE_NULL;
					}
					v = val;
				}
				else 
				{
					v = eval_expression (rtx, args);
					if (v == QSE_NULL) return QSE_NULL;
				}
			}

			qse_awk_rtx_refupval (rtx, v);
			n = qse_awk_rtx_valtoflt (rtx, v, &r);
			qse_awk_rtx_refdownval (rtx, v);
			if (n <= -1) return QSE_NULL;

			if (qse_str_fcat (out, QSE_STR_PTR(fbu), r) == (qse_size_t)-1)
			{
				SETERR_COD (rtx, QSE_AWK_ENOMEM);
				return QSE_NULL;
			}
		}
		else if (fmt[i] == QSE_T('c')) 
		{
			qse_char_t ch;
			qse_size_t ch_len;
			qse_awk_val_t* v;

			if (args == QSE_NULL)
			{
				if (stack_arg_idx >= nargs_on_stack)
				{
					SETERR_COD (rtx, QSE_AWK_EFMTARG);
					return QSE_NULL;
				}
				v = qse_awk_rtx_getarg (rtx, stack_arg_idx);
			}
			else
			{
				if (val != QSE_NULL) 
				{
					if (stack_arg_idx >= nargs_on_stack)
					{
						SETERR_COD (rtx, QSE_AWK_EFMTARG);
						return QSE_NULL;
					}
					v = val;
				}
				else 
				{
					v = eval_expression (rtx, args);
					if (v == QSE_NULL) return QSE_NULL;
				}
			}

			qse_awk_rtx_refupval (rtx, v);
			switch (v->type)
			{
				case QSE_AWK_VAL_NIL:
					ch = QSE_T('\0');
					ch_len = 0;
					break;
			
				case QSE_AWK_VAL_INT:
					ch = (qse_char_t)((qse_awk_val_int_t*)v)->val;
					ch_len = 1;
					break;

				case QSE_AWK_VAL_FLT:
					ch = (qse_char_t)((qse_awk_val_flt_t*)v)->val;
					ch_len = 1;
					break;

				case QSE_AWK_VAL_STR:
					ch_len = ((qse_awk_val_str_t*)v)->val.len;
					if (ch_len > 0) 
					{
						ch = ((qse_awk_val_str_t*)v)->val.ptr[0];
						ch_len = 1;
					}
					else ch = QSE_T('\0');
					break;

				default:
					qse_awk_rtx_refdownval (rtx, v);
					SETERR_COD (rtx, QSE_AWK_EVALTOCHR);
					return QSE_NULL;
			}

			if (wp[WP_PRECISION] == -1 || wp[WP_PRECISION] == 0 || wp[WP_PRECISION] > (qse_awk_int_t)ch_len) 
			{
				wp[WP_PRECISION] = (qse_awk_int_t)ch_len;
			}

			if (wp[WP_PRECISION] > wp[WP_WIDTH]) wp[WP_WIDTH] = wp[WP_PRECISION];

			if (!(flags & FLAG_MINUS))
			{
				/* right align */
				while (wp[WP_WIDTH] > wp[WP_PRECISION]) 
				{
					if (qse_str_ccat (out, QSE_T(' ')) == -1) 
					{ 
						qse_awk_rtx_refdownval (rtx, v);
						SETERR_COD (rtx, QSE_AWK_ENOMEM);
						return QSE_NULL; 
					} 
					wp[WP_WIDTH]--;
				}
			}

			if (wp[WP_PRECISION] > 0)
			{
				if (qse_str_ccat (out, ch) == -1) 
				{ 
					qse_awk_rtx_refdownval (rtx, v);
					SETERR_COD (rtx, QSE_AWK_ENOMEM);
					return QSE_NULL; 
				} 
			}

			if (flags & FLAG_MINUS)
			{
				/* left align */
				while (wp[WP_WIDTH] > wp[WP_PRECISION]) 
				{
					if (qse_str_ccat (out, QSE_T(' ')) == -1) 
					{ 
						qse_awk_rtx_refdownval (rtx, v);
						SETERR_COD (rtx, QSE_AWK_ENOMEM);
						return QSE_NULL; 
					} 
					wp[WP_WIDTH]--;
				}
			}

			qse_awk_rtx_refdownval (rtx, v);
		}
		else if (fmt[i] == QSE_T('s')) 
		{
			qse_char_t* str_ptr, * str_free = QSE_NULL;
			qse_size_t str_len;
			qse_awk_int_t k;
			qse_awk_val_t* v;

			if (args == QSE_NULL)
			{
				if (stack_arg_idx >= nargs_on_stack)
				{
					SETERR_COD (rtx, QSE_AWK_EFMTARG);
					return QSE_NULL;
				}
				v = qse_awk_rtx_getarg (rtx, stack_arg_idx);
			}
			else
			{
				if (val)
				{
					if (stack_arg_idx >= nargs_on_stack)
					{
						SETERR_COD (rtx, QSE_AWK_EFMTARG);
						return QSE_NULL;
					}
					v = val;
				}
				else 
				{
					v = eval_expression (rtx, args);
					if (v == QSE_NULL) return QSE_NULL;
				}
			}

			qse_awk_rtx_refupval (rtx, v);
			if (v->type == QSE_AWK_VAL_NIL)
			{
				str_ptr = QSE_T("");
				str_len = 0;
			}
			else if (v->type == QSE_AWK_VAL_STR)
			{
				str_ptr = ((qse_awk_val_str_t*)v)->val.ptr;
				str_len = ((qse_awk_val_str_t*)v)->val.len;
			}
			else
			{
				qse_awk_rtx_valtostr_out_t out;

				if (v == val)
				{
					qse_awk_rtx_refdownval (rtx, v);
					SETERR_COD (rtx, QSE_AWK_EFMTCNV);
					return QSE_NULL;
				}
	
				out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP;
				if (qse_awk_rtx_valtostr (rtx, v, &out) <= -1)
				{
					qse_awk_rtx_refdownval (rtx, v);
					return QSE_NULL;
				}

				str_ptr = out.u.cpldup.ptr;
				str_len = out.u.cpldup.len;
				str_free = str_ptr;
			}

			if (wp[WP_PRECISION] == -1 || wp[WP_PRECISION] > (qse_awk_int_t)str_len) wp[WP_PRECISION] = (qse_awk_int_t)str_len;
			if (wp[WP_PRECISION] > wp[WP_WIDTH]) wp[WP_WIDTH] = wp[WP_PRECISION];

			if (!(flags & FLAG_MINUS))
			{
				/* right align */
				while (wp[WP_WIDTH] > wp[WP_PRECISION]) 
				{
					if (qse_str_ccat (out, QSE_T(' ')) == -1) 
					{ 
						if (str_free != QSE_NULL) 
							QSE_AWK_FREE (rtx->awk, str_free);
						qse_awk_rtx_refdownval (rtx, v);
						SETERR_COD (rtx, QSE_AWK_ENOMEM);
						return QSE_NULL; 
					} 
					wp[WP_WIDTH]--;
				}
			}

			for (k = 0; k < wp[WP_PRECISION]; k++) 
			{
				if (qse_str_ccat (out, str_ptr[k]) == -1) 
				{ 
					if (str_free != QSE_NULL) 
						QSE_AWK_FREE (rtx->awk, str_free);
					qse_awk_rtx_refdownval (rtx, v);
					SETERR_COD (rtx, QSE_AWK_ENOMEM);
					return QSE_NULL; 
				} 
			}

			if (str_free != QSE_NULL) QSE_AWK_FREE (rtx->awk, str_free);

			if (flags & FLAG_MINUS)
			{
				/* left align */
				while (wp[WP_WIDTH] > wp[WP_PRECISION]) 
				{
					if (qse_str_ccat (out, QSE_T(' ')) == -1) 
					{ 
						qse_awk_rtx_refdownval (rtx, v);
						SETERR_COD (rtx, QSE_AWK_ENOMEM);
						return QSE_NULL; 
					} 
					wp[WP_WIDTH]--;
				}
			}

			qse_awk_rtx_refdownval (rtx, v);

		}
		else 
		{
			if (fmt[i] != QSE_T('%')) 
				OUT_STR (QSE_STR_PTR(fbu), QSE_STR_LEN(fbu));
			OUT_CHAR (fmt[i]);
			goto skip_taking_arg;
		}

		if (args == QSE_NULL || val != QSE_NULL) stack_arg_idx++;
		else args = args->next;
	skip_taking_arg:
		qse_str_clear (fbu);
	}

	/* flush uncompleted formatting sequence */
	OUT_STR (QSE_STR_PTR(fbu), QSE_STR_LEN(fbu));

	*len = QSE_STR_LEN(out);
	return QSE_STR_PTR(out);
}

void qse_awk_rtx_setnrflt (qse_awk_rtx_t* rtx, const qse_awk_nrflt_t* nrflt)
{
	rtx->nrflt = *nrflt;
}

void qse_awk_rtx_getnrflt (qse_awk_rtx_t* rtx, qse_awk_nrflt_t* nrflt)
{
	*nrflt = rtx->nrflt;
}
