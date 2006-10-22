/*
 * $Id: run.c,v 1.241 2006-10-22 12:39:29 bacon Exp $
 */

#include <sse/awk/awk_i.h>

/* TODO: remove this dependency...*/
#include <math.h>

#define CMP_ERROR -99
#define DEF_BUF_CAPA 256
#define STACK_INCREMENT 512

#define STACK_AT(run,n) ((run)->stack[(run)->stack_base+(n)])
#define STACK_NARGS(run) (STACK_AT(run,3))
#define STACK_ARG(run,n) STACK_AT(run,3+1+(n))
#define STACK_LOCAL(run,n) STACK_AT(run,3+(sse_size_t)STACK_NARGS(run)+1+(n))
#define STACK_RETVAL(run) STACK_AT(run,2)
#define STACK_GLOBAL(run,n) ((run)->stack[(n)])
#define STACK_RETVAL_GLOBAL(run) ((run)->stack[(run)->awk->tree.nglobals+2])

enum
{
	EXIT_NONE,
	EXIT_BREAK,
	EXIT_CONTINUE,
	EXIT_FUNCTION,
	EXIT_NEXT,
	EXIT_GLOBAL,
	EXIT_ABORT,
};

#define PANIC(run,code) \
	do { (run)->errnum = (code); return SSE_NULL; } while (0)

#define PANIC_I(run,code) \
	do { (run)->errnum = (code); return -1; } while (0)

#define DEFAULT_CONVFMT SSE_T("%.6g")
#define DEFAULT_OFMT SSE_T("%.6g")
#define DEFAULT_OFS SSE_T(" ")
#define DEFAULT_ORS SSE_T("\n")
#define DEFAULT_SUBSEP SSE_T("\034")

/* the index of a positional variable should be a positive interger
 * equal to or less than the maximum value of the type by which
 * the index is represented. but it has an extra check against the
 * maximum value of sse_size_t as the reference is represented
 * in a pointer variable of sse_awk_val_ref_t and sizeof(void*) is
 * equal to sizeof(sse_size_t). */
#define IS_VALID_POSIDX(idx) \
	((idx) >= 0 && \
	 (idx) < SSE_TYPE_MAX(sse_long_t) && \
	 (idx) < SSE_TYPE_MAX(sse_size_t))

static void __add_run (sse_awk_t* awk, sse_awk_run_t* run);
static void __del_run (sse_awk_t* awk, sse_awk_run_t* run);

static int __init_run (
	sse_awk_run_t* run, sse_awk_runios_t* runios, int* errnum);
static void __deinit_run (sse_awk_run_t* run);

static int __build_runarg (sse_awk_run_t* run, sse_awk_runarg_t* runarg);
static int __set_globals_to_default (sse_awk_run_t* run);

static int __run_main (sse_awk_run_t* run, sse_awk_runarg_t* runarg);
static int __run_pattern_blocks  (sse_awk_run_t* run);
static int __run_pattern_block_chain (
	sse_awk_run_t* run, sse_awk_chain_t* chain);
static int __run_pattern_block (
	sse_awk_run_t* run, sse_awk_chain_t* chain, sse_size_t block_no);
static int __run_block (sse_awk_run_t* run, sse_awk_nde_blk_t* nde);
static int __run_statement (sse_awk_run_t* run, sse_awk_nde_t* nde);
static int __run_if (sse_awk_run_t* run, sse_awk_nde_if_t* nde);
static int __run_while (sse_awk_run_t* run, sse_awk_nde_while_t* nde);
static int __run_for (sse_awk_run_t* run, sse_awk_nde_for_t* nde);
static int __run_foreach (sse_awk_run_t* run, sse_awk_nde_foreach_t* nde);
static int __run_break (sse_awk_run_t* run, sse_awk_nde_break_t* nde);
static int __run_continue (sse_awk_run_t* run, sse_awk_nde_continue_t* nde);
static int __run_return (sse_awk_run_t* run, sse_awk_nde_return_t* nde);
static int __run_exit (sse_awk_run_t* run, sse_awk_nde_exit_t* nde);
static int __run_next (sse_awk_run_t* run, sse_awk_nde_next_t* nde);
static int __run_nextfile (sse_awk_run_t* run, sse_awk_nde_nextfile_t* nde);
static int __run_delete (sse_awk_run_t* run, sse_awk_nde_delete_t* nde);
static int __run_print (sse_awk_run_t* run, sse_awk_nde_print_t* nde);

static sse_awk_val_t* __eval_expression (sse_awk_run_t* run, sse_awk_nde_t* nde);
static sse_awk_val_t* __eval_expression0 (sse_awk_run_t* run, sse_awk_nde_t* nde);

static sse_awk_val_t* __eval_group (sse_awk_run_t* run, sse_awk_nde_t* nde);

static sse_awk_val_t* __eval_assignment (
	sse_awk_run_t* run, sse_awk_nde_t* nde);
static sse_awk_val_t* __do_assignment (
	sse_awk_run_t* run, sse_awk_nde_t* var, sse_awk_val_t* val);
static sse_awk_val_t* __do_assignment_scalar (
	sse_awk_run_t* run, sse_awk_nde_var_t* var, sse_awk_val_t* val);
static sse_awk_val_t* __do_assignment_map (
	sse_awk_run_t* run, sse_awk_nde_var_t* var, sse_awk_val_t* val);
static sse_awk_val_t* __do_assignment_pos (
	sse_awk_run_t* run, sse_awk_nde_pos_t* pos, sse_awk_val_t* val);

static sse_awk_val_t* __eval_binary (
	sse_awk_run_t* run, sse_awk_nde_t* nde);
static sse_awk_val_t* __eval_binop_lor (
	sse_awk_run_t* run, sse_awk_nde_t* left, sse_awk_nde_t* right);
static sse_awk_val_t* __eval_binop_land (
	sse_awk_run_t* run, sse_awk_nde_t* left, sse_awk_nde_t* right);
static sse_awk_val_t* __eval_binop_in (
	sse_awk_run_t* run, sse_awk_nde_t* left, sse_awk_nde_t* right);
static sse_awk_val_t* __eval_binop_bor (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right);
static sse_awk_val_t* __eval_binop_bxor (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right);
static sse_awk_val_t* __eval_binop_band (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right);
static sse_awk_val_t* __eval_binop_eq (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right);
static sse_awk_val_t* __eval_binop_ne (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right);
static sse_awk_val_t* __eval_binop_gt (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right);
static sse_awk_val_t* __eval_binop_ge (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right);
static sse_awk_val_t* __eval_binop_lt (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right);
static sse_awk_val_t* __eval_binop_le (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right);
static sse_awk_val_t* __eval_binop_lshift (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right);
static sse_awk_val_t* __eval_binop_rshift (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right);
static sse_awk_val_t* __eval_binop_plus (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right);
static sse_awk_val_t* __eval_binop_minus (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right);
static sse_awk_val_t* __eval_binop_mul (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right);
static sse_awk_val_t* __eval_binop_div (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right);
static sse_awk_val_t* __eval_binop_mod (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right);
static sse_awk_val_t* __eval_binop_exp (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right);
static sse_awk_val_t* __eval_binop_concat (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right);
static sse_awk_val_t* __eval_binop_ma (
	sse_awk_run_t* run, sse_awk_nde_t* left, sse_awk_nde_t* right);
static sse_awk_val_t* __eval_binop_nm (
	sse_awk_run_t* run, sse_awk_nde_t* left, sse_awk_nde_t* right);
static sse_awk_val_t* __eval_binop_match0 (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right, int ret);

static sse_awk_val_t* __eval_unary (sse_awk_run_t* run, sse_awk_nde_t* nde);
static sse_awk_val_t* __eval_incpre (sse_awk_run_t* run, sse_awk_nde_t* nde);
static sse_awk_val_t* __eval_incpst (sse_awk_run_t* run, sse_awk_nde_t* nde);
static sse_awk_val_t* __eval_cnd (sse_awk_run_t* run, sse_awk_nde_t* nde);

static sse_awk_val_t* __eval_bfn (sse_awk_run_t* run, sse_awk_nde_t* nde);
static sse_awk_val_t* __eval_afn (sse_awk_run_t* run, sse_awk_nde_t* nde);
static sse_awk_val_t* __eval_call (
	sse_awk_run_t* run, sse_awk_nde_t* nde, 
	const sse_char_t* bfn_arg_spec, sse_awk_afn_t* afn);

static int __get_reference (
	sse_awk_run_t* run, sse_awk_nde_t* nde, sse_awk_val_t*** ref);
static sse_awk_val_t** __get_reference_indexed (
	sse_awk_run_t* run, sse_awk_nde_var_t* nde, sse_awk_val_t** val);

static sse_awk_val_t* __eval_int (sse_awk_run_t* run, sse_awk_nde_t* nde);
static sse_awk_val_t* __eval_real (sse_awk_run_t* run, sse_awk_nde_t* nde);
static sse_awk_val_t* __eval_str (sse_awk_run_t* run, sse_awk_nde_t* nde);
static sse_awk_val_t* __eval_rex (sse_awk_run_t* run, sse_awk_nde_t* nde);
static sse_awk_val_t* __eval_named (sse_awk_run_t* run, sse_awk_nde_t* nde);
static sse_awk_val_t* __eval_global (sse_awk_run_t* run, sse_awk_nde_t* nde);
static sse_awk_val_t* __eval_local (sse_awk_run_t* run, sse_awk_nde_t* nde);
static sse_awk_val_t* __eval_arg (sse_awk_run_t* run, sse_awk_nde_t* nde);
static sse_awk_val_t* __eval_namedidx (sse_awk_run_t* run, sse_awk_nde_t* nde);
static sse_awk_val_t* __eval_globalidx (sse_awk_run_t* run, sse_awk_nde_t* nde);
static sse_awk_val_t* __eval_localidx (sse_awk_run_t* run, sse_awk_nde_t* nde);
static sse_awk_val_t* __eval_argidx (sse_awk_run_t* run, sse_awk_nde_t* nde);
static sse_awk_val_t* __eval_pos (sse_awk_run_t* run, sse_awk_nde_t* nde);
static sse_awk_val_t* __eval_getline (sse_awk_run_t* run, sse_awk_nde_t* nde);

static int __raw_push (sse_awk_run_t* run, void* val);
#define __raw_pop(run) \
	do { \
		sse_awk_assert (run->awk, (run)->stack_top > (run)->stack_base); \
		(run)->stack_top--; \
	} while (0)
static void __raw_pop_times (sse_awk_run_t* run, sse_size_t times);

static int __read_record (sse_awk_run_t* run);
static int __shorten_record (sse_awk_run_t* run, sse_size_t nflds);

static sse_char_t* __idxnde_to_str (
	sse_awk_run_t* run, sse_awk_nde_t* nde, sse_size_t* len);

typedef sse_awk_val_t* (*binop_func_t) (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right);
typedef sse_awk_val_t* (*eval_expr_t) (sse_awk_run_t* run, sse_awk_nde_t* nde);

/* TODO: remove this function */
static int __printval (sse_awk_pair_t* pair, void* arg)
{
	xp_printf (SSE_T("%s = "), (const sse_char_t*)pair->key);
	sse_awk_printval ((sse_awk_val_t*)pair->val);
	xp_printf (SSE_T("\n"));
	return 0;
}

sse_size_t sse_awk_getnargs (sse_awk_run_t* run)
{
	return (sse_size_t) STACK_NARGS (run);
}

sse_awk_val_t* sse_awk_getarg (sse_awk_run_t* run, sse_size_t idx)
{
	return STACK_ARG (run, idx);
}

sse_awk_val_t* sse_awk_getglobal (sse_awk_run_t* run, sse_size_t idx)
{
	return STACK_GLOBAL (run, idx);
}

int sse_awk_setglobal (sse_awk_run_t* run, sse_size_t idx, sse_awk_val_t* val)
{
	sse_awk_val_t* old = STACK_GLOBAL (run, idx);
	if (old->type == SSE_AWK_VAL_MAP)
	{	
		/* once a variable becomes an array,
		 * it cannot be changed to a scalar variable */
		PANIC_I (run, SSE_AWK_EMAPTOSCALAR);
	}

/* TODO: is this correct?? */
	if (val->type == SSE_AWK_VAL_MAP &&
	    (idx >= SSE_AWK_GLOBAL_ARGC && idx <= SSE_AWK_GLOBAL_SUBSEP) &&
	    idx != SSE_AWK_GLOBAL_ARGV)
	{
		/* TODO: better error code */
		PANIC_I (run, SSE_AWK_ESCALARTOMAP);
	}

	if (idx == SSE_AWK_GLOBAL_CONVFMT)
	{
		sse_char_t* convfmt_ptr;
		sse_size_t convfmt_len;

		convfmt_ptr = sse_awk_valtostr (
			run, val, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &convfmt_len);
		if (convfmt_ptr == SSE_NULL) return  -1;

		if (run->global.convfmt.ptr != SSE_NULL)
			SSE_AWK_FREE (run->awk, run->global.convfmt.ptr);
		run->global.convfmt.ptr = convfmt_ptr;
		run->global.convfmt.len = convfmt_len;
	}
	else if (idx == SSE_AWK_GLOBAL_FS)
	{
		sse_char_t* fs_ptr;
		sse_size_t fs_len;

		if (val->type == SSE_AWK_VAL_STR)
		{
			fs_ptr = ((sse_awk_val_str_t*)val)->buf;
			fs_len = ((sse_awk_val_str_t*)val)->len;
		}
		else
		{
			/* due to the expression evaluation rule, the 
			 * regular expression can not be an assigned value */
			sse_awk_assert (run->awk, val->type != SSE_AWK_VAL_REX);

			fs_ptr = sse_awk_valtostr (
				run, val, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &fs_len);
			if (fs_ptr == SSE_NULL) return -1;
		}

		if (fs_len > 1)
		{
			void* rex;

			/* compile the regular expression */
			/* TODO: use safebuild */
			rex = sse_awk_buildrex (
				run->awk, fs_ptr, fs_len, &run->errnum);
			if (rex == SSE_NULL)
			{
				if (val->type != SSE_AWK_VAL_STR) 
					SSE_AWK_FREE (run->awk, fs_ptr);
				return -1;
			}

			if (run->global.fs != SSE_NULL) 
			{
				sse_awk_freerex (run->awk, run->global.fs);
			}
			run->global.fs = rex;
		}

		if (val->type != SSE_AWK_VAL_STR) SSE_AWK_FREE (run->awk, fs_ptr);
	}
	else if (idx == SSE_AWK_GLOBAL_IGNORECASE)
	{
		if ((val->type == SSE_AWK_VAL_INT &&
		     ((sse_awk_val_int_t*)val)->val == 0) ||
		    (val->type == SSE_AWK_VAL_REAL &&
		     ((sse_awk_val_real_t*)val)->val == 0.0) ||
		    (val->type == SSE_AWK_VAL_STR &&
		     ((sse_awk_val_str_t*)val)->len == 0))
		{
			run->global.ignorecase = 0;
		}
		else
		{
			run->global.ignorecase = 1;
		}
	}
	else if (idx == SSE_AWK_GLOBAL_NF)
	{
		int n;
		sse_long_t lv;
		sse_real_t rv;

		n = sse_awk_valtonum (run, val, &lv, &rv);
		if (n == -1) return -1;
		if (n == 1) lv = (sse_long_t)rv;

		if (lv < run->inrec.nflds)
		{
			if (__shorten_record (run, (sse_size_t)lv) == -1) return -1;
		}
	}
	else if (idx == SSE_AWK_GLOBAL_OFMT)
	{
		sse_char_t* ofmt_ptr;
		sse_size_t ofmt_len;

		ofmt_ptr = sse_awk_valtostr (
			run, val, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &ofmt_len);
		if (ofmt_ptr == SSE_NULL) return  -1;

		if (run->global.ofmt.ptr != SSE_NULL)
			SSE_AWK_FREE (run->awk, run->global.ofmt.ptr);
		run->global.ofmt.ptr = ofmt_ptr;
		run->global.ofmt.len = ofmt_len;
	}
	else if (idx == SSE_AWK_GLOBAL_OFS)
	{	
		sse_char_t* ofs_ptr;
		sse_size_t ofs_len;

		ofs_ptr = sse_awk_valtostr (
			run, val, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &ofs_len);
		if (ofs_ptr == SSE_NULL) return  -1;

		if (run->global.ofs.ptr != SSE_NULL)
			SSE_AWK_FREE (run->awk, run->global.ofs.ptr);
		run->global.ofs.ptr = ofs_ptr;
		run->global.ofs.len = ofs_len;
	}
	else if (idx == SSE_AWK_GLOBAL_ORS)
	{	
		sse_char_t* ors_ptr;
		sse_size_t ors_len;

		ors_ptr = sse_awk_valtostr (
			run, val, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &ors_len);
		if (ors_ptr == SSE_NULL) return  -1;

		if (run->global.ors.ptr != SSE_NULL)
			SSE_AWK_FREE (run->awk, run->global.ors.ptr);
		run->global.ors.ptr = ors_ptr;
		run->global.ors.len = ors_len;
	}
	else if (idx == SSE_AWK_GLOBAL_RS)
	{
		sse_char_t* rs_ptr;
		sse_size_t rs_len;

		if (val->type == SSE_AWK_VAL_STR)
		{
			rs_ptr = ((sse_awk_val_str_t*)val)->buf;
			rs_len = ((sse_awk_val_str_t*)val)->len;
		}
		else
		{
			/* due to the expression evaluation rule, the 
			 * regular expression can not be an assigned value */
			sse_awk_assert (run->awk, val->type != SSE_AWK_VAL_REX);

			rs_ptr = sse_awk_valtostr (
				run, val, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &rs_len);
			if (rs_ptr == SSE_NULL) return -1;
		}

		if (rs_len > 1)
		{
			void* rex;

			/* compile the regular expression */
			/* TODO: use safebuild */
			rex = sse_awk_buildrex (
				run->awk, rs_ptr, rs_len, &run->errnum);
			if (rex == SSE_NULL)
			{
				if (val->type != SSE_AWK_VAL_STR) 
					SSE_AWK_FREE (run->awk, rs_ptr);
				return -1;
			}

			if (run->global.rs != SSE_NULL) 
			{
				sse_awk_freerex (run->awk, run->global.rs);
			}
			run->global.rs = rex;
		}

		if (val->type != SSE_AWK_VAL_STR) SSE_AWK_FREE (run->awk, rs_ptr);
	}
	else if (idx == SSE_AWK_GLOBAL_SUBSEP)
	{
		sse_char_t* subsep_ptr;
		sse_size_t subsep_len;

		subsep_ptr = sse_awk_valtostr (
			run, val, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &subsep_len);
		if (subsep_ptr == SSE_NULL) return  -1;

		if (run->global.subsep.ptr != SSE_NULL)
			SSE_AWK_FREE (run->awk, run->global.subsep.ptr);
		run->global.subsep.ptr = subsep_ptr;
		run->global.subsep.len = subsep_len;
	}

	sse_awk_refdownval (run, old);
	STACK_GLOBAL(run,idx) = val;
	sse_awk_refupval (val);

	return 0;
}

void sse_awk_setretval (sse_awk_run_t* run, sse_awk_val_t* val)
{
	sse_awk_refdownval (run, STACK_RETVAL(run));
	STACK_RETVAL(run) = val;
	/* should use the same trick as __run_return_statement */
	sse_awk_refupval (val); 
}

int sse_awk_setconsolename (
	sse_awk_run_t* run, const sse_char_t* name, sse_size_t len)
{
	sse_awk_val_t* tmp;
	int n;

	if (len == 0) tmp = sse_awk_val_zls;
	else
	{
		tmp = sse_awk_makestrval (run, name, len);
		if (tmp == SSE_NULL)
		{
			run->errnum = SSE_AWK_ENOMEM;
			return -1;
		}
	}

	sse_awk_refupval (tmp);
	n = sse_awk_setglobal (run, SSE_AWK_GLOBAL_FILENAME, tmp);
	sse_awk_refdownval (run, tmp);

	return n;
}

int sse_awk_getrunerrnum (sse_awk_run_t* run)
{
	return run->errnum;
}

void sse_awk_setrunerrnum (sse_awk_run_t* run, int errnum)
{
	run->errnum = errnum;
}

int sse_awk_run (sse_awk_t* awk, 
	sse_awk_runios_t* runios, 
	sse_awk_runcbs_t* runcbs, 
	sse_awk_runarg_t* runarg)
{
	sse_awk_run_t* run;
	int n, errnum;

	awk->errnum = SSE_AWK_ENOERR;

	run = (sse_awk_run_t*) SSE_AWK_MALLOC (awk, sse_sizeof(sse_awk_run_t));
	if (run == SSE_NULL)
	{
		awk->errnum = SSE_AWK_ENOMEM;
		return -1;
	}

	SSE_AWK_MEMSET (awk, run, 0, sse_sizeof(sse_awk_run_t));

	__add_run (awk, run);

	if (__init_run (run, runios, &errnum) == -1) 
	{
		awk->errnum = errnum;
		__del_run (awk, run);
		SSE_AWK_FREE (awk, run);
		return -1;
	}

	if (runcbs != SSE_NULL && runcbs->on_start != SSE_NULL) 
	{
		runcbs->on_start (awk, run, runcbs->custom_data);
	}

	n = __run_main (run, runarg);
	if (n == -1) 
	{
		/* if no callback is specified, awk's error number 
		 * is updated with the run's error number */
		awk->errnum = (runcbs == SSE_NULL)? run->errnum: SSE_AWK_ERUNTIME;
	}

	if (runcbs != SSE_NULL && runcbs->on_end != SSE_NULL) 
	{
		runcbs->on_end (awk, run, 
			((n == -1)? run->errnum: SSE_AWK_ENOERR), 
			runcbs->custom_data);

		/* when using callbacks, the function always returns 0 after
		 * the start callbacks has been triggered */
		n = 0;
	}

	__deinit_run (run);
	__del_run (awk, run);
	SSE_AWK_FREE (awk, run);
	return n;
}

int sse_awk_stop (sse_awk_t* awk, sse_awk_run_t* run)
{
	sse_awk_run_t* r;
	int n = 0;

	SSE_AWK_LOCK (awk);

	/* check if the run handle given is valid */
	for (r = awk->run.ptr; r != SSE_NULL; r = r->next)
	{
		if (r == run)
		{
			sse_awk_assert (run->awk, r->awk == awk);
			r->exit_level = EXIT_ABORT;
			break;
		}
	}

	if (r == SSE_NULL)
	{
		/* if it is not found in the awk's run list, 
		 * it is not a valid handle */
		awk->errnum = SSE_AWK_EINVAL;
		n = -1;
	}

	SSE_AWK_UNLOCK (awk);

	return n;
}

void sse_awk_stopall (sse_awk_t* awk)
{
	sse_awk_run_t* r;

	SSE_AWK_LOCK (awk);

	for (r = awk->run.ptr; r != SSE_NULL; r = r->next)
	{
		r->exit_level = EXIT_ABORT;
	}

	SSE_AWK_UNLOCK (awk);
}

static void __free_namedval (void* run, void* val)
{
	sse_awk_refdownval ((sse_awk_run_t*)run, val);
}

static void __add_run (sse_awk_t* awk, sse_awk_run_t* run)
{
	SSE_AWK_LOCK (awk);

	run->awk = awk;
	run->prev = SSE_NULL;
	run->next = awk->run.ptr;
	if (run->next != SSE_NULL) run->next->prev = run;
	awk->run.ptr = run;
	awk->run.count++;

	SSE_AWK_UNLOCK (awk);
}

static void __del_run (sse_awk_t* awk, sse_awk_run_t* run)
{
	SSE_AWK_LOCK (awk);

	sse_awk_assert (run->awk, awk->run.ptr != SSE_NULL);

	if (run->prev == SSE_NULL)
	{
		awk->run.ptr = run->next;
		if (run->next != SSE_NULL) run->next->prev = SSE_NULL;
	}
	else
	{
		run->prev->next = run->next;
		if (run->next != SSE_NULL) run->next->prev = run->prev;
	}

	run->awk = SSE_NULL;
	awk->run.count--;

	SSE_AWK_UNLOCK (awk);
}

static int __init_run (sse_awk_run_t* run, sse_awk_runios_t* runios, int* errnum)
{
	run->stack = SSE_NULL;
	run->stack_top = 0;
	run->stack_base = 0;
	run->stack_limit = 0;

	run->exit_level = EXIT_NONE;

	run->icache_count = 0;
	run->rcache_count = 0;
	run->fcache_count = 0;

	run->errnum = SSE_AWK_ENOERR;

	run->inrec.buf_pos = 0;
	run->inrec.buf_len = 0;
	run->inrec.flds = SSE_NULL;
	run->inrec.nflds = 0;
	run->inrec.maxflds = 0;
	run->inrec.d0 = sse_awk_val_nil;
	if (sse_awk_str_open (
		&run->inrec.line, DEF_BUF_CAPA, run->awk) == SSE_NULL)
	{
		*errnum = SSE_AWK_ENOMEM; 
		return -1;
	}

	if (sse_awk_map_open (&run->named, 
		run, DEF_BUF_CAPA, __free_namedval, run->awk) == SSE_NULL) 
	{
		sse_awk_str_close (&run->inrec.line);
		*errnum = SSE_AWK_ENOMEM; 
		return -1;
	}

	run->pattern_range_state = (sse_byte_t*) SSE_AWK_MALLOC (
		run->awk, run->awk->tree.chain_size * sse_sizeof(sse_byte_t));
	if (run->pattern_range_state == SSE_NULL)
	{
		sse_awk_map_close (&run->named);
		sse_awk_str_close (&run->inrec.line);
		*errnum = SSE_AWK_ENOMEM; 
		return -1;
	}

	SSE_AWK_MEMSET (run->awk, run->pattern_range_state, 0, 
		run->awk->tree.chain_size * sse_sizeof(sse_byte_t));

	run->extio.handler[SSE_AWK_EXTIO_PIPE] = runios->pipe;
	run->extio.handler[SSE_AWK_EXTIO_COPROC] = runios->coproc;
	run->extio.handler[SSE_AWK_EXTIO_FILE] = runios->file;
	run->extio.handler[SSE_AWK_EXTIO_CONSOLE] = runios->console;
	run->extio.custom_data = runios->custom_data;
	run->extio.chain = SSE_NULL;

	run->global.rs = SSE_NULL;
	run->global.fs = SSE_NULL;
	run->global.ignorecase = 0;

	return 0;
}

static void __deinit_run (sse_awk_run_t* run)
{
	SSE_AWK_FREE (run->awk, run->pattern_range_state);

	/* close all pending eio's */
	/* TODO: what if this operation fails? */
	sse_awk_clearextio (run);
	sse_awk_assert (run->awk, run->extio.chain == SSE_NULL);

	if (run->global.rs != SSE_NULL) 
	{
		SSE_AWK_FREE (run->awk, run->global.rs);
		run->global.rs = SSE_NULL;
	}
	if (run->global.fs != SSE_NULL)
	{
		SSE_AWK_FREE (run->awk, run->global.fs);
		run->global.fs = SSE_NULL;
	}

	if (run->global.convfmt.ptr != SSE_NULL &&
	    run->global.convfmt.ptr != DEFAULT_CONVFMT)
	{
		SSE_AWK_FREE (run->awk, run->global.convfmt.ptr);
		run->global.convfmt.ptr = SSE_NULL;
		run->global.convfmt.len = 0;
	}

	if (run->global.ofmt.ptr != SSE_NULL && 
	    run->global.ofmt.ptr != DEFAULT_OFMT)
	{
		SSE_AWK_FREE (run->awk, run->global.ofmt.ptr);
		run->global.ofmt.ptr = SSE_NULL;
		run->global.ofmt.len = 0;
	}

	if (run->global.ofs.ptr != SSE_NULL && 
	    run->global.ofs.ptr != DEFAULT_OFS)
	{
		SSE_AWK_FREE (run->awk, run->global.ofs.ptr);
		run->global.ofs.ptr = SSE_NULL;
		run->global.ofs.len = 0;
	}

	if (run->global.ors.ptr != SSE_NULL && 
	    run->global.ors.ptr != DEFAULT_ORS)
	{
		SSE_AWK_FREE (run->awk, run->global.ors.ptr);
		run->global.ors.ptr = SSE_NULL;
		run->global.ors.len = 0;
	}

	if (run->global.subsep.ptr != SSE_NULL && 
	    run->global.subsep.ptr != DEFAULT_SUBSEP)
	{
		SSE_AWK_FREE (run->awk, run->global.subsep.ptr);
		run->global.subsep.ptr = SSE_NULL;
		run->global.subsep.len = 0;
	}

	/* destroy input record. sse_awk_clrrec should be called
	 * before the run stack has been destroyed because it may try
	 * to change the value to SSE_AWK_GLOBAL_NF. */
	sse_awk_clrrec (run, sse_false);  
	if (run->inrec.flds != SSE_NULL) 
	{
		SSE_AWK_FREE (run->awk, run->inrec.flds);
		run->inrec.flds = SSE_NULL;
		run->inrec.maxflds = 0;
	}
	sse_awk_str_close (&run->inrec.line);

	/* destroy run stack */
	if (run->stack != SSE_NULL)
	{
		sse_awk_assert (run->awk, run->stack_top == 0);

		SSE_AWK_FREE (run->awk, run->stack);
		run->stack = SSE_NULL;
		run->stack_top = 0;
		run->stack_base = 0;
		run->stack_limit = 0;
	}

	/* destroy named variables */
	sse_awk_map_close (&run->named);

	/* destroy values in free list */
	while (run->icache_count > 0)
	{
		sse_awk_val_int_t* tmp = run->icache[--run->icache_count];
		sse_awk_freeval (run, (sse_awk_val_t*)tmp, sse_false);
	}

	while (run->rcache_count > 0)
	{
		sse_awk_val_real_t* tmp = run->rcache[--run->rcache_count];
		sse_awk_freeval (run, (sse_awk_val_t*)tmp, sse_false);
	}

	while (run->fcache_count > 0)
	{
		sse_awk_val_ref_t* tmp = run->fcache[--run->fcache_count];
		sse_awk_freeval (run, (sse_awk_val_t*)tmp, sse_false);
	}
}

static int __build_runarg (sse_awk_run_t* run, sse_awk_runarg_t* runarg)
{
	sse_awk_runarg_t* p = runarg;
	sse_size_t argc;
	sse_awk_val_t* v_argc;
	sse_awk_val_t* v_argv;
	sse_awk_val_t* v_tmp;
	sse_char_t key[sse_sizeof(sse_long_t)*8+2];
	sse_size_t key_len;

	v_argv = sse_awk_makemapval (run);
	if (v_argv == SSE_NULL)
	{
		run->errnum = SSE_AWK_ENOMEM;
		return -1;
	}
	sse_awk_refupval (v_argv);

	if (runarg == SSE_NULL) argc = 0;
	else
	{
		for (argc = 0, p = runarg; p->ptr != SSE_NULL; argc++, p++)
		{
			v_tmp = sse_awk_makestrval (run, p->ptr, p->len);
			if (v_tmp == SSE_NULL)
			{
				sse_awk_refdownval (run, v_argv);
				run->errnum = SSE_AWK_ENOMEM;
				return -1;
			}

			key_len = sse_awk_longtostr (
				argc, 10, SSE_NULL, key, sse_countof(key));
			sse_awk_assert (run->awk, key_len != (sse_size_t)-1);

			/* increment reference count of v_tmp in advance as if 
			 * it has successfully been assigned into ARGV. */
			sse_awk_refupval (v_tmp);

			if (sse_awk_map_putx (
				((sse_awk_val_map_t*)v_argv)->map,
				key, key_len, v_tmp, SSE_NULL) == -1)
			{
				/* if the assignment operation fails, decrements
				 * the reference of v_tmp to free it */
				sse_awk_refdownval (run, v_tmp);

				/* the values previously assigned into the
				 * map will be freeed when v_argv is freed */
				sse_awk_refdownval (run, v_argv);

				run->errnum = SSE_AWK_ENOMEM;
				return -1;
			}
		}
	}

	v_argc = sse_awk_makeintval (run, (sse_long_t)argc);
	if (v_argc == SSE_NULL)
	{
		sse_awk_refdownval (run, v_argv);
		run->errnum = SSE_AWK_ENOMEM;
		return -1;
	}

	sse_awk_refupval (v_argc);

	sse_awk_assert (run->awk, 
		STACK_GLOBAL(run,SSE_AWK_GLOBAL_ARGC) == sse_awk_val_nil);

	if (sse_awk_setglobal (run, SSE_AWK_GLOBAL_ARGC, v_argc) == -1) 
	{
		sse_awk_refdownval (run, v_argc);
		sse_awk_refdownval (run, v_argv);
		return -1;
	}

	if (sse_awk_setglobal (run, SSE_AWK_GLOBAL_ARGV, v_argv) == -1)
	{
		/* ARGC is assigned nil when ARGV assignment has failed.
		 * However, this requires preconditions, as follows:
		 *  1. __build_runarg should be called in a proper place
		 *     as it is not a generic-purpose routine.
		 *  2. ARGC should be nil before __build_runarg is called 
		 * If the restoration fails, nothing can salvage it. */
		sse_awk_setglobal (run, SSE_AWK_GLOBAL_ARGC, sse_awk_val_nil);
		sse_awk_refdownval (run, v_argc);
		sse_awk_refdownval (run, v_argv);
		return -1;
	}

	sse_awk_refdownval (run, v_argc);
	sse_awk_refdownval (run, v_argv);
	return 0;
}

static int __update_fnr (sse_awk_run_t* run, sse_size_t fnr)
{
	sse_awk_val_t* tmp;

	tmp = sse_awk_makeintval (run, fnr);
	if (tmp == SSE_NULL)
	{
		run->errnum = SSE_AWK_ENOMEM;
		return -1;
	}

	sse_awk_refupval (tmp);
	if (sse_awk_setglobal (run, SSE_AWK_GLOBAL_FNR, tmp) == -1)
	{
		sse_awk_refdownval (run, tmp);
		return -1;
	}

	sse_awk_refdownval (run, tmp);
	run->global.fnr = fnr;
	return 0;
}

static int __set_globals_to_default (sse_awk_run_t* run)
{
	struct __gtab_t
	{
		int idx;
		const sse_char_t* str;
	};
       
	static struct __gtab_t gtab[] =
	{
		{ SSE_AWK_GLOBAL_CONVFMT,  DEFAULT_CONVFMT },
		{ SSE_AWK_GLOBAL_FILENAME, SSE_NULL },
		{ SSE_AWK_GLOBAL_OFMT,     DEFAULT_OFMT },
		{ SSE_AWK_GLOBAL_OFS,      DEFAULT_OFS },
		{ SSE_AWK_GLOBAL_ORS,      DEFAULT_ORS },
		{ SSE_AWK_GLOBAL_SUBSEP,   DEFAULT_SUBSEP },
	};

	sse_awk_val_t* tmp;
	sse_size_t i, j;

	for (i = 0; i < sse_countof(gtab); i++)
	{
		if (gtab[i].str == SSE_NULL || gtab[i].str[0] == SSE_T('\0'))
		{
			tmp = sse_awk_val_zls;
		}
		else 
		{
			tmp = sse_awk_makestrval0 (run, gtab[i].str);
			if (tmp == SSE_NULL)
			{
				run->errnum = SSE_AWK_ENOMEM;
				return -1;
			}
		}
		
		sse_awk_refupval (tmp);

		sse_awk_assert (run->awk, 
			STACK_GLOBAL(run,gtab[i].idx) == sse_awk_val_nil);

		if (sse_awk_setglobal (run, gtab[i].idx, tmp) == -1)
		{
			for (j = 0; j < i; j++)
			{
				sse_awk_setglobal (
					run, gtab[i].idx, sse_awk_val_nil);
			}

			sse_awk_refdownval (run, tmp);
			return -1;
		}

		sse_awk_refdownval (run, tmp);
	}

	return 0;
}

static int __run_main (sse_awk_run_t* run, sse_awk_runarg_t* runarg)
{
	sse_size_t nglobals, nargs, i;
	sse_size_t saved_stack_top;
	sse_awk_val_t* v;
	int n;

	sse_awk_assert (run->awk, run->stack_base == 0 && run->stack_top == 0);

	/* secure space for global variables */
	saved_stack_top = run->stack_top;

	nglobals = run->awk->tree.nglobals;

	while (nglobals > 0)
	{
		--nglobals;
		if (__raw_push(run,sse_awk_val_nil) == -1)
		{
			/* restore the stack_top with the saved value
			 * instead of calling __raw_pop as many times as
			 * the successful __raw_push. it is ok because
			 * the values pushed so far are all sse_awk_val_nil */
			run->stack_top = saved_stack_top;
			PANIC_I (run, SSE_AWK_ENOMEM);
		}
	}	

	if (sse_awk_setglobal (run, SSE_AWK_GLOBAL_NR, sse_awk_val_zero) == -1)
	{
		/* it can simply restore the top of the stack this way
		 * because the values pused onto the stack so far are
		 * all sse_awk_val_nils */
		run->stack_top = saved_stack_top;
		return -1;
	}

	if (sse_awk_setglobal (run, SSE_AWK_GLOBAL_NF, sse_awk_val_zero) == -1)
	{
		/* it can simply restore the top of the stack this way
		 * because the values pused onto the stack so far are
		 * all sse_awk_val_nils  and sse_awk_val_zeros */
		run->stack_top = saved_stack_top;
		return -1;
	}
	
	if (runarg != SSE_NULL && __build_runarg (run, runarg) == -1)
	{
		/* it can simply restore the top of the stack this way
		 * because the values pused onto the stack so far are
		 * all sse_awk_val_nils and sse_awk_val_zeros and 
		 * __build_runarg doesn't push other values than them
		 * when it has failed */
		run->stack_top = saved_stack_top;
		return -1;
	}

	run->exit_level = EXIT_NONE;

	n = __update_fnr (run, 0);
	if (n == 0) n = __set_globals_to_default (run);
	if (n == 0 && (run->awk->option & SSE_AWK_RUNMAIN))
	{
/* TODO: should the main function be user-specifiable? */
		sse_awk_nde_call_t nde;

		nde.type = SSE_AWK_NDE_AFN;
		nde.next = SSE_NULL;
		nde.what.afn.name = SSE_T("main");
		nde.what.afn.name_len = 4;
		nde.args = SSE_NULL;
		nde.nargs = 0;

		v = __eval_afn (run, (sse_awk_nde_t*)&nde);
		if (v == SSE_NULL) n = -1;
		else
		{
			/* destroy the return value if necessary */
			sse_awk_refupval (v);
			sse_awk_refdownval (run, v);
		}
	}
	else if (n == 0)
	{
		saved_stack_top = run->stack_top;
		if (__raw_push(run,(void*)run->stack_base) == -1) 
		{
			/* restore the stack top in a cheesy(?) way */
			run->stack_top = saved_stack_top;
			/* pops off global variables in a decent way */	
			__raw_pop_times (run, run->awk->tree.nglobals);
			PANIC_I (run, SSE_AWK_ENOMEM);
		}

		if (__raw_push(run,(void*)saved_stack_top) == -1) 
		{
			run->stack_top = saved_stack_top;
			__raw_pop_times (run, run->awk->tree.nglobals);
			PANIC_I (run, SSE_AWK_ENOMEM);
		}
	
		/* secure space for a return value */
		if (__raw_push(run,sse_awk_val_nil) == -1)
		{
			run->stack_top = saved_stack_top;
			__raw_pop_times (run, run->awk->tree.nglobals);
			PANIC_I (run, SSE_AWK_ENOMEM);
		}
	
		/* secure space for nargs */
		if (__raw_push(run,sse_awk_val_nil) == -1)
		{
			run->stack_top = saved_stack_top;
			__raw_pop_times (run, run->awk->tree.nglobals);
			PANIC_I (run, SSE_AWK_ENOMEM);
		}
	
		run->stack_base = saved_stack_top;
	
		/* set nargs to zero */
		nargs = 0;
		STACK_NARGS(run) = (void*)nargs;
	
		/* stack set up properly. ready to exeucte statement blocks */
		if (n == 0 && 
		    run->awk->tree.begin != SSE_NULL && 
		    run->exit_level != EXIT_ABORT)
		{
			sse_awk_nde_blk_t* blk;

			blk = (sse_awk_nde_blk_t*)run->awk->tree.begin;
			sse_awk_assert (run->awk, blk->type == SSE_AWK_NDE_BLK);

			run->active_block = blk;
			run->exit_level = EXIT_NONE;
			if (__run_block (run, blk) == -1) n = -1;
		}

		if (n == 0 && 
		    run->awk->tree.chain != SSE_NULL && 
		    run->exit_level != EXIT_ABORT)
		{
			if (__run_pattern_blocks (run) == -1) n = -1;
		}

		if (n == 0 && 
		    run->awk->tree.end != SSE_NULL && 
		    run->exit_level != EXIT_ABORT) 
		{
			sse_awk_nde_blk_t* blk;

			blk = (sse_awk_nde_blk_t*)run->awk->tree.end;
			sse_awk_assert (run->awk, blk->type == SSE_AWK_NDE_BLK);

			run->active_block = blk;
			run->exit_level = EXIT_NONE;
			if (__run_block (run, blk) == -1) n = -1;
		}

		/* restore stack */
		nargs = (sse_size_t)STACK_NARGS(run);
		sse_awk_assert (run->awk, nargs == 0);
		for (i = 0; i < nargs; i++)
		{
			sse_awk_refdownval (run, STACK_ARG(run,i));
		}

		v = STACK_RETVAL(run);
xp_printf (SSE_T("Return Value - "));
sse_awk_printval (v);
xp_printf (SSE_T("\n"));
		/* the life of the global return value is over here
		 * unlike the return value of each function */
		/*sse_awk_refdownval_nofree (awk, v);*/
		sse_awk_refdownval (run, v);

		run->stack_top = 
			(sse_size_t)run->stack[run->stack_base+1];
		run->stack_base = 
			(sse_size_t)run->stack[run->stack_base+0];
	}

	/* pops off the global variables */
	nglobals = run->awk->tree.nglobals; /*run->nglobals */
	while (nglobals > 0)
	{
		--nglobals;
		sse_awk_refdownval (run, STACK_GLOBAL(run,nglobals));
		__raw_pop (run);
	}

	/* just reset the exit level */
	run->exit_level = EXIT_NONE;

xp_printf (SSE_T("-[VARIABLES]------------------------\n"));
sse_awk_map_walk (&run->named, __printval, SSE_NULL);
xp_printf (SSE_T("-[END VARIABLES]--------------------------\n"));

	return n;
}

static int __run_pattern_blocks (sse_awk_run_t* run)
{
	sse_ssize_t n;
	sse_bool_t need_to_close = sse_false;

	run->inrec.buf_pos = 0;
	run->inrec.buf_len = 0;
	run->inrec.eof = sse_false;

	/* run each pattern block */
	while (run->exit_level != EXIT_GLOBAL &&
	       run->exit_level != EXIT_ABORT)
	{
		int x;

		run->exit_level = EXIT_NONE;

		x = __read_record (run);
		if (x == -1)
		{
			int saved = run->errnum;

			/* don't care about the result of input close */
			sse_awk_closeextio_read (
				run, SSE_AWK_IN_CONSOLE, SSE_T(""));

			run->errnum = saved;
			return -1;
		}

		need_to_close = sse_true;
		if (x == 0) break; /* end of input */

		__update_fnr (run, run->global.fnr + 1);

		if (__run_pattern_block_chain (run, run->awk->tree.chain) == -1)
		{
			int saved = run->errnum;

			sse_awk_closeextio_read (
				run, SSE_AWK_IN_CONSOLE, SSE_T(""));

			run->errnum = saved;
			return -1;
		}
	}

	/* In case of getline, the code would make getline return -1, 
	 * set ERRNO, make this function return 0 after having checked 
	 * if closextio has returned -1 and errnum has been set to 
	 * SSE_AWK_EIOHANDLER. But this part of the code ends the input for 
	 * the implicit pattern-block loop, which is totally different 
	 * from getline. so it returns -1 as long as closeextio returns 
	 * -1 regardless of the value of errnum.  */
	if (need_to_close)
	{
		n = sse_awk_closeextio_read (
			run, SSE_AWK_IN_CONSOLE, SSE_T(""));
		if (n == -1) 
		{
			if (run->errnum == SSE_AWK_EIOHANDLER)
				PANIC_I (run, SSE_AWK_ECONINCLOSE);
			else return -1;
		}
	}

	return 0;
}

static int __run_pattern_block_chain (sse_awk_run_t* run, sse_awk_chain_t* chain)
{
	sse_size_t block_no = 0;

	while (run->exit_level != EXIT_GLOBAL &&
	       run->exit_level != EXIT_ABORT && chain != SSE_NULL)
	{
		if (run->exit_level == EXIT_NEXT)
		{
			run->exit_level = EXIT_NONE;
			break;
		}

		if (__run_pattern_block (run, chain, block_no) == -1) return -1;

		chain = chain->next; 
		block_no++;
	}

	return 0;
}

static int __run_pattern_block (
	sse_awk_run_t* run, sse_awk_chain_t* chain, sse_size_t block_no)
{
	sse_awk_nde_t* ptn;
	sse_awk_nde_blk_t* blk;

	ptn = chain->pattern;
	blk = (sse_awk_nde_blk_t*)chain->action;

	if (ptn == SSE_NULL)
	{
		/* just execute the block */
		run->active_block = blk;
		if (__run_block (run, blk) == -1) return -1;
	}
	else
	{
		if (ptn->next == SSE_NULL)
		{
			/* pattern { ... } */
			sse_awk_val_t* v1;

			v1 = __eval_expression (run, ptn);
			if (v1 == SSE_NULL) return -1;

			sse_awk_refupval (v1);

			if (sse_awk_valtobool (run, v1))
			{
				run->active_block = blk;
				if (__run_block (run, blk) == -1) 
				{
					sse_awk_refdownval (run, v1);
					return -1;
				}
			}

			sse_awk_refdownval (run, v1);
		}
		else
		{
			/* pattern, pattern { ... } */
			sse_awk_assert (run->awk, ptn->next->next == SSE_NULL);

			if (run->pattern_range_state[block_no] == 0)
			{
				sse_awk_val_t* v1;

				v1 = __eval_expression (run, ptn);
				if (v1 == SSE_NULL) return -1;
				sse_awk_refupval (v1);

				if (sse_awk_valtobool (run, v1))
				{
					run->active_block = blk;
					if (__run_block (run, blk) == -1) 
					{
						sse_awk_refdownval (run, v1);
						return -1;
					}

					run->pattern_range_state[block_no] = 1;
				}

				sse_awk_refdownval (run, v1);
			}
			else if (run->pattern_range_state[block_no] == 1)
			{
				sse_awk_val_t* v2;

				v2 = __eval_expression (run, ptn->next);
				if (v2 == SSE_NULL) return -1;
				sse_awk_refupval (v2);

				run->active_block = blk;
				if (__run_block (run, blk) == -1) 
				{
					sse_awk_refdownval (run, v2);
					return -1;
				}

				if (sse_awk_valtobool (run, v2)) 
					run->pattern_range_state[block_no] = 0;

				sse_awk_refdownval (run, v2);
			}
		}
	}

	return 0;
}

static int __run_block (sse_awk_run_t* run, sse_awk_nde_blk_t* nde)
{
	sse_awk_nde_t* p;
	sse_size_t nlocals;
	sse_size_t saved_stack_top;
	int n = 0;

	if (nde == SSE_NULL)
	{
		/* blockless pattern - execute print $0*/
		sse_awk_refupval (run->inrec.d0);

		/*n = sse_awk_writeextio_val (run, 
			SSE_AWK_OUT_CONSOLE, SSE_T(""), run->inrec.d0);*/
		n = sse_awk_writeextio_str (run, 
			SSE_AWK_OUT_CONSOLE, SSE_T(""),
			SSE_AWK_STR_BUF(&run->inrec.line),
			SSE_AWK_STR_LEN(&run->inrec.line));
		if (n == -1)
		{
			sse_awk_refdownval (run, run->inrec.d0);

			if (run->errnum == SSE_AWK_EIOHANDLER)
				PANIC_I (run, SSE_AWK_ECONOUTDATA);
			else return -1;
		}

		sse_awk_refdownval (run, run->inrec.d0);
		return 0;
	}

	sse_awk_assert (run->awk, nde->type == SSE_AWK_NDE_BLK);

	p = nde->body;
	nlocals = nde->nlocals;

/*xp_printf (SSE_T("securing space for local variables nlocals = %d\n"), (int)nlocals);*/
	saved_stack_top = run->stack_top;

	/* secure space for local variables */
	while (nlocals > 0)
	{
		--nlocals;
		if (__raw_push(run,sse_awk_val_nil) == -1)
		{
			/* restore stack top */
			run->stack_top = saved_stack_top;
			return -1;
		}

		/* refupval is not required for sse_awk_val_nil */
	}

/*xp_printf (SSE_T("executing block statements\n"));*/
	while (p != SSE_NULL && run->exit_level == EXIT_NONE) 
	{
/*xp_printf (SSE_T("running a statement\n"));*/
		if (__run_statement(run,p) == -1) 
		{
			n = -1;
			break;
		}
		p = p->next;
	}

/*xp_printf (SSE_T("popping off local variables\n"));*/
	/* pop off local variables */
	nlocals = nde->nlocals;
	while (nlocals > 0)
	{
		--nlocals;
		sse_awk_refdownval (run, STACK_LOCAL(run,nlocals));
		__raw_pop (run);
	}

	return n;
}

static int __run_statement (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	switch (nde->type) 
	{
		case SSE_AWK_NDE_NULL:
		{
			/* do nothing */
			break;
		}

		case SSE_AWK_NDE_BLK:
		{
			if (__run_block (run, 
				(sse_awk_nde_blk_t*)nde) == -1) return -1;
			break;
		}

		case SSE_AWK_NDE_IF:
		{
			if (__run_if (run, 
				(sse_awk_nde_if_t*)nde) == -1) return -1;	
			break;
		}

		case SSE_AWK_NDE_WHILE:
		case SSE_AWK_NDE_DOWHILE:
		{
			if (__run_while (run, 
				(sse_awk_nde_while_t*)nde) == -1) return -1;
			break;
		}

		case SSE_AWK_NDE_FOR:
		{
			if (__run_for (run, 
				(sse_awk_nde_for_t*)nde) == -1) return -1;
			break;
		}

		case SSE_AWK_NDE_FOREACH:
		{
			if (__run_foreach (run, 
				(sse_awk_nde_foreach_t*)nde) == -1) return -1;
			break;
		}

		case SSE_AWK_NDE_BREAK:
		{
			if (__run_break (run, 
				(sse_awk_nde_break_t*)nde) == -1) return -1;
			break;
		}

		case SSE_AWK_NDE_CONTINUE:
		{
			if (__run_continue (run, 
				(sse_awk_nde_continue_t*)nde) == -1) return -1;
			break;
		}

		case SSE_AWK_NDE_RETURN:
		{
			if (__run_return (run, 
				(sse_awk_nde_return_t*)nde) == -1) return -1;
			break;
		}

		case SSE_AWK_NDE_EXIT:
		{
			if (__run_exit (run, 
				(sse_awk_nde_exit_t*)nde) == -1) return -1;
			break;
		}

		case SSE_AWK_NDE_NEXT:
		{
			if (__run_next (run, 
				(sse_awk_nde_next_t*)nde) == -1) return -1;
			break;
		}

		case SSE_AWK_NDE_NEXTFILE:
		{
			if (__run_nextfile (run, 
				(sse_awk_nde_nextfile_t*)nde) == -1) return -1;
			break;
		}

		case SSE_AWK_NDE_DELETE:
		{
			if (__run_delete (run, 
				(sse_awk_nde_delete_t*)nde) == -1) return -1;
			break;
		}

		case SSE_AWK_NDE_PRINT:
		{
			if (__run_print (run, 
				(sse_awk_nde_print_t*)nde) == -1) return -1;
			break;
		}

		default:
		{
			sse_awk_val_t* v;
			v = __eval_expression(run,nde);
			if (v == SSE_NULL) return -1;

			sse_awk_refupval (v);
			sse_awk_refdownval (run, v);

			break;
		}
	}

	return 0;
}

static int __run_if (sse_awk_run_t* run, sse_awk_nde_if_t* nde)
{
	sse_awk_val_t* test;
	int n = 0;

	/* the test expression for the if statement cannot have 
	 * chained expressions. this should not be allowed by the
	 * parser first of all */
	sse_awk_assert (run->awk, nde->test->next == SSE_NULL);

	test = __eval_expression (run, nde->test);
	if (test == SSE_NULL) return -1;

	sse_awk_refupval (test);
	if (sse_awk_valtobool (run, test))
	{
		n = __run_statement (run, nde->then_part);
	}
	else if (nde->else_part != SSE_NULL)
	{
		n = __run_statement (run, nde->else_part);
	}

	sse_awk_refdownval (run, test); /* TODO: is this correct?*/
	return n;
}

static int __run_while (sse_awk_run_t* run, sse_awk_nde_while_t* nde)
{
	sse_awk_val_t* test;

	if (nde->type == SSE_AWK_NDE_WHILE)
	{
		/* no chained expressions are allowed for the test 
		 * expression of the while statement */
		sse_awk_assert (run->awk, nde->test->next == SSE_NULL);

		/* TODO: handle run-time abortion... */
		while (1)
		{
			test = __eval_expression (run, nde->test);
			if (test == SSE_NULL) return -1;

			sse_awk_refupval (test);

			if (sse_awk_valtobool (run, test))
			{
				if (__run_statement(run,nde->body) == -1)
				{
					sse_awk_refdownval (run, test);
					return -1;
				}
			}
			else
			{
				sse_awk_refdownval (run, test);
				break;
			}

			sse_awk_refdownval (run, test);

			if (run->exit_level == EXIT_BREAK)
			{	
				run->exit_level = EXIT_NONE;
				break;
			}
			else if (run->exit_level == EXIT_CONTINUE)
			{
				run->exit_level = EXIT_NONE;
			}
			else if (run->exit_level != EXIT_NONE) break;
		}
	}
	else if (nde->type == SSE_AWK_NDE_DOWHILE)
	{
		/* no chained expressions are allowed for the test 
		 * expression of the while statement */
		sse_awk_assert (run->awk, nde->test->next == SSE_NULL);

		/* TODO: handle run-time abortion... */
		do
		{
			if (__run_statement(run,nde->body) == -1) return -1;

			if (run->exit_level == EXIT_BREAK)
			{	
				run->exit_level = EXIT_NONE;
				break;
			}
			else if (run->exit_level == EXIT_CONTINUE)
			{
				run->exit_level = EXIT_NONE;
			}
			else if (run->exit_level != EXIT_NONE) break;

			test = __eval_expression (run, nde->test);
			if (test == SSE_NULL) return -1;

			sse_awk_refupval (test);

			if (!sse_awk_valtobool (run, test))
			{
				sse_awk_refdownval (run, test);
				break;
			}

			sse_awk_refdownval (run, test);
		}
		while (1);
	}

	return 0;
}

static int __run_for (sse_awk_run_t* run, sse_awk_nde_for_t* nde)
{
	sse_awk_val_t* val;

	if (nde->init != SSE_NULL)
	{
		sse_awk_assert (run->awk, nde->init->next == SSE_NULL);
		val = __eval_expression(run,nde->init);
		if (val == SSE_NULL) return -1;

		sse_awk_refupval (val);
		sse_awk_refdownval (run, val);
	}

	while (1)
	{
		if (nde->test != SSE_NULL)
		{
			sse_awk_val_t* test;

			/* no chained expressions for the test expression of
			 * the for statement are allowed */
			sse_awk_assert (run->awk, nde->test->next == SSE_NULL);

			test = __eval_expression (run, nde->test);
			if (test == SSE_NULL) return -1;

			sse_awk_refupval (test);
			if (sse_awk_valtobool (run, test))
			{
				if (__run_statement(run,nde->body) == -1)
				{
					sse_awk_refdownval (run, test);
					return -1;
				}
			}
			else
			{
				sse_awk_refdownval (run, test);
				break;
			}

			sse_awk_refdownval (run, test);
		}	
		else
		{
			if (__run_statement(run,nde->body) == -1)
			{
				return -1;
			}
		}

		if (run->exit_level == EXIT_BREAK)
		{	
			run->exit_level = EXIT_NONE;
			break;
		}
		else if (run->exit_level == EXIT_CONTINUE)
		{
			run->exit_level = EXIT_NONE;
		}
		else if (run->exit_level != EXIT_NONE) break;

		if (nde->incr != SSE_NULL)
		{
			sse_awk_assert (run->awk, nde->incr->next == SSE_NULL);
			val = __eval_expression(run,nde->incr);
			if (val == SSE_NULL) return -1;

			sse_awk_refupval (val);
			sse_awk_refdownval (run, val);
		}
	}

	return 0;
}

struct __foreach_walker_t
{
	sse_awk_run_t* run;
	sse_awk_nde_t* var;
	sse_awk_nde_t* body;
};

static int __walk_foreach (sse_awk_pair_t* pair, void* arg)
{
	struct __foreach_walker_t* w = (struct __foreach_walker_t*)arg;
	sse_awk_val_t* str;

	str = (sse_awk_val_t*) sse_awk_makestrval (
		w->run, pair->key, sse_awk_strlen(pair->key));
	if (str == SSE_NULL) PANIC_I (w->run, SSE_AWK_ENOMEM);

	sse_awk_refupval (str);
	if (__do_assignment (w->run, w->var, str) == SSE_NULL)
	{
		sse_awk_refdownval (w->run, str);
		return -1;
	}

	if (__run_statement (w->run, w->body) == -1)
	{
		sse_awk_refdownval (w->run, str);
		return -1;
	}
	
	sse_awk_refdownval (w->run, str);
	return 0;
}

static int __run_foreach (sse_awk_run_t* run, sse_awk_nde_foreach_t* nde)
{
	int n;
	sse_awk_nde_exp_t* test;
	sse_awk_val_t* rv;
	sse_awk_map_t* map;
	struct __foreach_walker_t walker;

	test = (sse_awk_nde_exp_t*)nde->test;
	sse_awk_assert (run->awk, test->type == SSE_AWK_NDE_EXP_BIN && 
	           test->opcode == SSE_AWK_BINOP_IN);

	/* chained expressions should not be allowed 
	 * by the parser first of all */
	sse_awk_assert (run->awk, test->right->next == SSE_NULL); 

	rv = __eval_expression (run, test->right);
	if (rv == SSE_NULL) return -1;

	sse_awk_refupval (rv);
	if (rv->type != SSE_AWK_VAL_MAP)
	{
		sse_awk_refdownval (run, rv);
		PANIC_I (run, SSE_AWK_ENOTINDEXABLE);
	}
	map = ((sse_awk_val_map_t*)rv)->map;

	walker.run = run;
	walker.var = test->left;
	walker.body = nde->body;
	n = sse_awk_map_walk (map, __walk_foreach, &walker);

	sse_awk_refdownval (run, rv);
	return n;
}

static int __run_break (sse_awk_run_t* run, sse_awk_nde_break_t* nde)
{
	run->exit_level = EXIT_BREAK;
	return 0;
}

static int __run_continue (sse_awk_run_t* run, sse_awk_nde_continue_t* nde)
{
	run->exit_level = EXIT_CONTINUE;
	return 0;
}

static int __run_return (sse_awk_run_t* run, sse_awk_nde_return_t* nde)
{
	if (nde->val != SSE_NULL)
	{
		sse_awk_val_t* val;

		/* chained expressions should not be allowed 
		 * by the parser first of all */
		sse_awk_assert (run->awk, nde->val->next == SSE_NULL); 

/*xp_printf (SSE_T("returning....\n"));*/
		val = __eval_expression (run, nde->val);
		if (val == SSE_NULL) return -1;

		sse_awk_refdownval (run, STACK_RETVAL(run));
		STACK_RETVAL(run) = val;
		sse_awk_refupval (val); /* see __eval_call for the trick */
/*xp_printf (SSE_T("set return value....\n"));*/
	}
	
	run->exit_level = EXIT_FUNCTION;
	return 0;
}

static int __run_exit (sse_awk_run_t* run, sse_awk_nde_exit_t* nde)
{
	if (nde->val != SSE_NULL)
	{
		sse_awk_val_t* val;

		/* chained expressions should not be allowed 
		 * by the parser first of all */
		sse_awk_assert (run->awk, nde->val->next == SSE_NULL); 

		val = __eval_expression (run, nde->val);
		if (val == SSE_NULL) return -1;

		sse_awk_refdownval (run, STACK_RETVAL_GLOBAL(run));
		STACK_RETVAL_GLOBAL(run) = val; /* global return value */

		sse_awk_refupval (val);
	}

	run->exit_level = EXIT_GLOBAL;
	return 0;
}

static int __run_next (sse_awk_run_t* run, sse_awk_nde_next_t* nde)
{
	/* the parser checks if next has been called in the begin/end
	 * block or whereever inappropriate. so the runtime doesn't 
	 * check that explicitly */

	if  (run->active_block == (sse_awk_nde_blk_t*)run->awk->tree.begin ||
	     run->active_block == (sse_awk_nde_blk_t*)run->awk->tree.end)
	{
		PANIC_I (run, SSE_AWK_ENEXTCALL);
	}

	run->exit_level = EXIT_NEXT;
	return 0;
}

static int __run_nextfile (sse_awk_run_t* run, sse_awk_nde_nextfile_t* nde)
{
/* TODO: some extentions such as nextfile "in/out"; 
 *  what about awk -i in1,in2,in3 -o out1,out2,out3 ?
 */
	int n;

	if  (run->active_block == (sse_awk_nde_blk_t*)run->awk->tree.begin ||
	     run->active_block == (sse_awk_nde_blk_t*)run->awk->tree.end)
	{
		run->errnum = SSE_AWK_ENEXTFILECALL;
		return -1;
	}

	n = sse_awk_nextextio_read (run, SSE_AWK_IN_CONSOLE, SSE_T(""));
	if (n == -1)
	{
		if (run->errnum == SSE_AWK_EIOHANDLER)
			run->errnum = SSE_AWK_ECONINNEXT;
		return -1;
	}

	if (n == 0)
	{
		/* no more input file */
		run->exit_level = EXIT_GLOBAL;
		return 0;
	}

	if (__update_fnr (run, 0) == -1) return -1;

/* TODO: Consider using FILENAME_IN and FILENAME_OUT to accomplish nextfile in/out */
	run->exit_level = EXIT_NEXT;
	return 0;
}

static int __run_delete (sse_awk_run_t* run, sse_awk_nde_delete_t* nde)
{
	sse_awk_nde_var_t* var;

	var = (sse_awk_nde_var_t*) nde->var;

	if (var->type == SSE_AWK_NDE_NAMED ||
	    var->type == SSE_AWK_NDE_NAMEDIDX)
	{
		sse_awk_pair_t* pair;

		sse_awk_assert (run->awk, (var->type == SSE_AWK_NDE_NAMED && 
		            var->idx == SSE_NULL) ||
		           (var->type == SSE_AWK_NDE_NAMEDIDX && 
		            var->idx != SSE_NULL));

		pair = sse_awk_map_get (
			&run->named, var->id.name, var->id.name_len);
		if (pair == SSE_NULL)
		{
			sse_awk_val_t* tmp;

			/* value not set for the named variable. 
			 * create a map and assign it to the variable */

			tmp = sse_awk_makemapval (run);
			if (tmp == SSE_NULL) PANIC_I (run, SSE_AWK_ENOMEM);

			if (sse_awk_map_put (&run->named, 
				var->id.name, var->id.name_len, tmp) == SSE_NULL)
			{
				sse_awk_refupval (tmp);
				sse_awk_refdownval (run, tmp);
				PANIC_I (run, SSE_AWK_ENOMEM);		
			}

			sse_awk_refupval (tmp);
		}
		else
		{
			sse_awk_val_t* val;
			sse_awk_map_t* map;

			val = (sse_awk_val_t*)pair->val;
			sse_awk_assert (run->awk, val != SSE_NULL);

			if (val->type != SSE_AWK_VAL_MAP)
				PANIC_I (run, SSE_AWK_ENOTDELETABLE);

			map = ((sse_awk_val_map_t*)val)->map;
			if (var->type == SSE_AWK_NDE_NAMEDIDX)
			{
				sse_char_t* key;
				sse_size_t key_len;
				sse_awk_val_t* idx;

				sse_awk_assert (run->awk, var->idx != SSE_NULL);

				idx = __eval_expression (run, var->idx);
				if (idx == SSE_NULL) return -1;

				sse_awk_refupval (idx);
				key = sse_awk_valtostr (
					run, idx, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &key_len);
				sse_awk_refdownval (run, idx);

				if (key == SSE_NULL) return -1;

				sse_awk_map_remove (map, key, key_len);
				SSE_AWK_FREE (run->awk, key);
			}
			else
			{
				sse_awk_map_clear (map);
			}
		}
	}
	else if (var->type == SSE_AWK_NDE_GLOBAL ||
	         var->type == SSE_AWK_NDE_LOCAL ||
	         var->type == SSE_AWK_NDE_ARG ||
	         var->type == SSE_AWK_NDE_GLOBALIDX ||
	         var->type == SSE_AWK_NDE_LOCALIDX ||
	         var->type == SSE_AWK_NDE_ARGIDX)
	{
		sse_awk_val_t* val;

		if (var->type == SSE_AWK_NDE_GLOBAL ||
		    var->type == SSE_AWK_NDE_GLOBALIDX)
			val = STACK_GLOBAL (run,var->id.idxa);
		else if (var->type == SSE_AWK_NDE_LOCAL ||
		         var->type == SSE_AWK_NDE_LOCALIDX)
			val = STACK_LOCAL (run,var->id.idxa);
		else val = STACK_ARG (run,var->id.idxa);

		sse_awk_assert (run->awk, val != SSE_NULL);

		if (val->type == SSE_AWK_VAL_NIL)
		{
			sse_awk_val_t* tmp;

			/* value not set for the named variable. 
			 * create a map and assign it to the variable */

			tmp = sse_awk_makemapval (run);
			if (tmp == SSE_NULL) PANIC_I (run, SSE_AWK_ENOMEM);

			/* no need to reduce the reference count of
			 * the previous value because it was nil. */
			if (var->type == SSE_AWK_NDE_GLOBAL ||
			    var->type == SSE_AWK_NDE_GLOBALIDX)
			{
				if (sse_awk_setglobal (
					run, var->id.idxa, tmp) == -1)
				{
					sse_awk_refupval (tmp);
					sse_awk_refdownval (run, tmp);
					return -1;
				}
			}
			else if (var->type == SSE_AWK_NDE_LOCAL ||
			         var->type == SSE_AWK_NDE_LOCALIDX)
			{
				STACK_LOCAL(run,var->id.idxa) = tmp;
				sse_awk_refupval (tmp);
			}
			else 
			{
				STACK_ARG(run,var->id.idxa) = tmp;
				sse_awk_refupval (tmp);
			}
		}
		else
		{
			sse_awk_map_t* map;

			if (val->type != SSE_AWK_VAL_MAP)
				PANIC_I (run, SSE_AWK_ENOTDELETABLE);

			map = ((sse_awk_val_map_t*)val)->map;
			if (var->type == SSE_AWK_NDE_GLOBALIDX ||
			    var->type == SSE_AWK_NDE_LOCALIDX ||
			    var->type == SSE_AWK_NDE_ARGIDX)
			{
				sse_char_t* key;
				sse_size_t key_len;
				sse_awk_val_t* idx;

				sse_awk_assert (run->awk, var->idx != SSE_NULL);

				idx = __eval_expression (run, var->idx);
				if (idx == SSE_NULL) return -1;

				sse_awk_refupval (idx);
				key = sse_awk_valtostr (
					run, idx, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &key_len);
				sse_awk_refdownval (run, idx);

				if (key == SSE_NULL) return -1;

				sse_awk_map_remove (map, key, key_len);
				SSE_AWK_FREE (run->awk, key);
			}
			else
			{
				sse_awk_map_clear (map);
			}
		}
	}
	else
	{
		sse_awk_assert (run->awk, !"should never happen - wrong variable type for delete");
		PANIC_I (run, SSE_AWK_EINTERNAL);
	}

	return 0;
}

static int __run_print (sse_awk_run_t* run, sse_awk_nde_print_t* nde)
{
	sse_awk_nde_print_t* p = (sse_awk_nde_print_t*)nde;
	sse_char_t* out = SSE_NULL;
	const sse_char_t* dst;
	sse_awk_val_t* v;
	sse_awk_nde_t* np;
	int n;

	sse_awk_assert (run->awk, 
		(p->out_type == SSE_AWK_OUT_PIPE && p->out != SSE_NULL) ||
		(p->out_type == SSE_AWK_OUT_COPROC && p->out != SSE_NULL) ||
		(p->out_type == SSE_AWK_OUT_FILE && p->out != SSE_NULL) ||
		(p->out_type == SSE_AWK_OUT_FILE_APPEND && p->out != SSE_NULL) ||
		(p->out_type == SSE_AWK_OUT_CONSOLE && p->out == SSE_NULL));

	if (p->out != SSE_NULL)
	{
		sse_size_t len;

		v = __eval_expression (run, p->out);
		if (v == SSE_NULL) return -1;

		sse_awk_refupval (v);
		out = sse_awk_valtostr (
			run, v, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &len);
		if (out == SSE_NULL) 
		{
			sse_awk_refdownval (run, v);
			return -1;
		}
		sse_awk_refdownval (run, v);

		if (len <= 0) 
		{
			/* the output destination name is empty. */
			SSE_AWK_FREE (run->awk, out);
			n = -1;
			goto skip_write;
		}

		while (len > 0)
		{
			if (out[--len] == SSE_T('\0'))
			{
				/* the output destination name contains a null 
				 * character. */
				SSE_AWK_FREE (run->awk, out);
				n = -1;
				goto skip_write;
				/* TODO: how to handle error???
				 *       make print return -1??? not possible.
				 *       throw an exception??
				 *       set ERRNO or what??? this seems most
				 *       reasonable. or can it have a global
				 *       flag variable for print/printf such
				 *       as PRINT_ERRNO?  */
			}
		}
	}

	dst = (out == SSE_NULL)? SSE_T(""): out;

	if (p->args == SSE_NULL)
	{
		/*
		v = run->inrec.d0;
		sse_awk_refupval (v);
		n = sse_awk_writeextio_val (run, p->out_type, dst, v);
		if (n < 0 && run->errnum != SSE_AWK_EIOHANDLER) 
		{
			if (out != SSE_NULL) SSE_AWK_FREE (run->awk, out);
			sse_awk_refdownval (run, v);
			return -1;
		}
		sse_awk_refdownval (run, v);
		*/
		n = sse_awk_writeextio_str (
			run, p->out_type, dst,
			SSE_AWK_STR_BUF(&run->inrec.line),
			SSE_AWK_STR_LEN(&run->inrec.line));
		if (n < 0 && run->errnum != SSE_AWK_EIOHANDLER) 
		{
			if (out != SSE_NULL) SSE_AWK_FREE (run->awk, out);
			return -1;
		}
		/* TODO: how to handle n == -1 && errnum == SSE_AWK_EIOHANDLER. 
		 * that is the user handler returned an error... */
	}
	else
	{
		for (np = p->args; np != SSE_NULL; np = np->next)
		{
			if (np != p->args)
			{
				n = sse_awk_writeextio_str (
					run, p->out_type, dst, 
					run->global.ofs.ptr, 
					run->global.ofs.len);
				if (n < 0 && run->errnum != SSE_AWK_EIOHANDLER) 
				{
					if (out != SSE_NULL) SSE_AWK_FREE (run->awk, out);
					return -1;
				}
			}

			v = __eval_expression (run, np);
			if (v == SSE_NULL) 
			{
				if (out != SSE_NULL) SSE_AWK_FREE (run->awk, out);
				return -1;
			}
			sse_awk_refupval (v);

			n = sse_awk_writeextio_val (run, p->out_type, dst, v);
			if (n < 0 && run->errnum != SSE_AWK_EIOHANDLER) 
			{
				if (out != SSE_NULL) SSE_AWK_FREE (run->awk, out);
				sse_awk_refdownval (run, v);
				return -1;
			}

			sse_awk_refdownval (run, v);


			/* TODO: how to handle n == -1 && run->errnum == SSE_AWK_EIOHANDLER. 
			 * that is the user handler returned an error... */
		}
	}

	n = sse_awk_writeextio_str (
		run, p->out_type, dst, 
		run->global.ors.ptr, run->global.ors.len);
	if (n < 0 && run->errnum != SSE_AWK_EIOHANDLER)
	{
		if (out != SSE_NULL) SSE_AWK_FREE (run->awk, out);
		return -1;
	}

	/* TODO: how to handle n == -1 && errnum == SSE_AWK_EIOHANDLER.
	 * that is the user handler returned an error... */

	if (out != SSE_NULL) SSE_AWK_FREE (run->awk, out);

skip_write:
	return 0;
}

static sse_awk_val_t* __eval_expression (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	sse_awk_val_t* v;
	int n, errnum;

	v = __eval_expression0 (run, nde);
	if (v == SSE_NULL) return SSE_NULL;

	if (v->type == SSE_AWK_VAL_REX)
	{
		sse_awk_refupval (v);

		if (run->inrec.d0->type == SSE_AWK_VAL_NIL)
		{
			/* the record has never been read. 
			 * probably, this functions has been triggered
			 * by the statements in the BEGIN block */
			n = sse_awk_isemptyrex (run->awk, ((sse_awk_val_rex_t*)v)->code)? 1: 0;
		}
		else
		{
			sse_awk_assert (run->awk, run->inrec.d0->type == SSE_AWK_VAL_STR);

			n = sse_awk_matchrex (
				((sse_awk_run_t*)run)->awk, 
				((sse_awk_val_rex_t*)v)->code,
				((((sse_awk_run_t*)run)->global.ignorecase)? SSE_AWK_REX_IGNORECASE: 0),
				((sse_awk_val_str_t*)run->inrec.d0)->buf,
				((sse_awk_val_str_t*)run->inrec.d0)->len,
				SSE_NULL, SSE_NULL, &errnum);
	
			if (n == -1) 
			{
				sse_awk_refdownval (run, v);
				PANIC (run, errnum);
			}
		}

		sse_awk_refdownval (run, v);

		v = sse_awk_makeintval (run, (n != 0));
		if (v == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);
	}

	return v;
}

static sse_awk_val_t* __eval_expression0 (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	static eval_expr_t __eval_func[] =
	{
		/* the order of functions here should match the order
		 * of node types declared in tree.h */
		__eval_group,
		__eval_assignment,
		__eval_binary,
		__eval_unary,
		__eval_incpre,
		__eval_incpst,
		__eval_cnd,
		__eval_bfn,
		__eval_afn,
		__eval_int,
		__eval_real,
		__eval_str,
		__eval_rex,
		__eval_named,
		__eval_global,
		__eval_local,
		__eval_arg,
		__eval_namedidx,
		__eval_globalidx,
		__eval_localidx,
		__eval_argidx,
		__eval_pos,
		__eval_getline
	};

	sse_awk_assert (run->awk, nde->type >= SSE_AWK_NDE_GRP &&
		(nde->type - SSE_AWK_NDE_GRP) < sse_countof(__eval_func));

	return __eval_func[nde->type-SSE_AWK_NDE_GRP] (run, nde);
}

static sse_awk_val_t* __eval_group (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	/* __eval_binop_in evaluates the SSE_AWK_NDE_GRP specially.
	 * so this function should never be reached. */
	sse_awk_assert (run->awk, !"should never happen - NDE_GRP only for in");
	PANIC (run, SSE_AWK_EINTERNAL);
	return SSE_NULL;
}

static sse_awk_val_t* __eval_assignment (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	sse_awk_val_t* val, * ret;
	sse_awk_nde_ass_t* ass = (sse_awk_nde_ass_t*)nde;

	sse_awk_assert (run->awk, ass->left != SSE_NULL && ass->right != SSE_NULL);

	sse_awk_assert (run->awk, ass->right->next == SSE_NULL);
	val = __eval_expression (run, ass->right);
	if (val == SSE_NULL) return SSE_NULL;

	sse_awk_refupval (val);

	if (ass->opcode != SSE_AWK_ASSOP_NONE)
	{
		sse_awk_val_t* val2, * tmp;

		sse_awk_assert (run->awk, ass->left->next == SSE_NULL);
		val2 = __eval_expression (run, ass->left);
		if (val2 == SSE_NULL)
		{
			sse_awk_refdownval (run, val);
			return SSE_NULL;
		}

		sse_awk_refupval (val2);

		if (ass->opcode == SSE_AWK_ASSOP_PLUS)
		{
			tmp = __eval_binop_plus (run, val2, val);
		}
		else if (ass->opcode == SSE_AWK_ASSOP_MINUS)
		{
			tmp = __eval_binop_minus (run, val2, val);
		}
		else if (ass->opcode == SSE_AWK_ASSOP_MUL)
		{
			tmp = __eval_binop_mul (run, val2, val);
		}
		else if (ass->opcode == SSE_AWK_ASSOP_DIV)
		{
			tmp = __eval_binop_div (run, val2, val);
		}
		else if (ass->opcode == SSE_AWK_ASSOP_MOD)
		{
			tmp = __eval_binop_mod (run, val2, val);
		}
		else if (ass->opcode == SSE_AWK_ASSOP_EXP)
		{
			tmp = __eval_binop_exp (run, val2, val);
		}
		else
		{
			sse_awk_assert (run->awk, !"should never happen - invalid assignment opcode");
			PANIC (run, SSE_AWK_EINTERNAL);
		}

		if (tmp == SSE_NULL)
		{
			sse_awk_refdownval (run, val);
			sse_awk_refdownval (run, val2);
			return SSE_NULL;
		}

		sse_awk_refdownval (run, val);
		val = tmp;
		sse_awk_refupval (val);
	}

	ret = __do_assignment (run, ass->left, val);
	sse_awk_refdownval (run, val);

	return ret;
}

static sse_awk_val_t* __do_assignment (
	sse_awk_run_t* run, sse_awk_nde_t* var, sse_awk_val_t* val)
{
	sse_awk_val_t* ret;

	if (val->type == SSE_AWK_VAL_MAP)
	{
		/* a map cannot be assigned to a variable */
		PANIC (run, SSE_AWK_ENOTASSIGNABLE);
	}

	if (var->type == SSE_AWK_NDE_NAMED ||
	    var->type == SSE_AWK_NDE_GLOBAL ||
	    var->type == SSE_AWK_NDE_LOCAL ||
	    var->type == SSE_AWK_NDE_ARG) 
	{
		ret = __do_assignment_scalar (run, (sse_awk_nde_var_t*)var, val);
	}
	else if (var->type == SSE_AWK_NDE_NAMEDIDX ||
	         var->type == SSE_AWK_NDE_GLOBALIDX ||
	         var->type == SSE_AWK_NDE_LOCALIDX ||
	         var->type == SSE_AWK_NDE_ARGIDX) 
	{
		ret = __do_assignment_map (run, (sse_awk_nde_var_t*)var, val);
	}
	else if (var->type == SSE_AWK_NDE_POS)
	{
		ret = __do_assignment_pos (run, (sse_awk_nde_pos_t*)var, val);
	}
	else
	{
		sse_awk_assert (run->awk, !"should never happen - invalid variable type");
		PANIC (run, SSE_AWK_EINTERNAL);
	}

	return ret;
}

static sse_awk_val_t* __do_assignment_scalar (
	sse_awk_run_t* run, sse_awk_nde_var_t* var, sse_awk_val_t* val)
{
	sse_awk_assert (run->awk, 
		(var->type == SSE_AWK_NDE_NAMED ||
		 var->type == SSE_AWK_NDE_GLOBAL ||
		 var->type == SSE_AWK_NDE_LOCAL ||
		 var->type == SSE_AWK_NDE_ARG) && var->idx == SSE_NULL);

	sse_awk_assert (run->awk, val->type != SSE_AWK_VAL_MAP);

	if (var->type == SSE_AWK_NDE_NAMED) 
	{
		sse_awk_pair_t* pair;
		int n;

		pair = sse_awk_map_get (
			&run->named, var->id.name, var->id.name_len);
		if (pair != SSE_NULL && 
		    ((sse_awk_val_t*)pair->val)->type == SSE_AWK_VAL_MAP)
		{
			/* once a variable becomes an array,
			 * it cannot be changed to a scalar variable */
			PANIC (run, SSE_AWK_EMAPTOSCALAR);
		}

		n = sse_awk_map_putx (&run->named, 
			var->id.name, var->id.name_len, val, SSE_NULL);
		if (n < 0) PANIC (run, SSE_AWK_ENOMEM);

		sse_awk_refupval (val);
	}
	else if (var->type == SSE_AWK_NDE_GLOBAL) 
	{
		if (sse_awk_setglobal (
			run, var->id.idxa, val) == -1) return SSE_NULL;
	}
	else if (var->type == SSE_AWK_NDE_LOCAL) 
	{
		sse_awk_val_t* old = STACK_LOCAL(run,var->id.idxa);
		if (old->type == SSE_AWK_VAL_MAP)
		{	
			/* once the variable becomes an array,
			 * it cannot be changed to a scalar variable */
			PANIC (run, SSE_AWK_EMAPTOSCALAR);
		}

		sse_awk_refdownval (run, old);
		STACK_LOCAL(run,var->id.idxa) = val;
		sse_awk_refupval (val);
	}
	else /* if (var->type == SSE_AWK_NDE_ARG) */
	{
		sse_awk_val_t* old = STACK_ARG(run,var->id.idxa);
		if (old->type == SSE_AWK_VAL_MAP)
		{	
			/* once the variable becomes an array,
			 * it cannot be changed to a scalar variable */
			PANIC (run, SSE_AWK_EMAPTOSCALAR);
		}

		sse_awk_refdownval (run, old);
		STACK_ARG(run,var->id.idxa) = val;
		sse_awk_refupval (val);
	}

	return val;
}

static sse_awk_val_t* __do_assignment_map (
	sse_awk_run_t* run, sse_awk_nde_var_t* var, sse_awk_val_t* val)
{
	sse_awk_val_map_t* map;
	sse_char_t* str;
	sse_size_t len;
	int n;

	sse_awk_assert (run->awk, 
		(var->type == SSE_AWK_NDE_NAMEDIDX ||
		 var->type == SSE_AWK_NDE_GLOBALIDX ||
		 var->type == SSE_AWK_NDE_LOCALIDX ||
		 var->type == SSE_AWK_NDE_ARGIDX) && var->idx != SSE_NULL);
	sse_awk_assert (run->awk, val->type != SSE_AWK_VAL_MAP);

	if (var->type == SSE_AWK_NDE_NAMEDIDX)
	{
		sse_awk_pair_t* pair;
		pair = sse_awk_map_get (
			&run->named, var->id.name, var->id.name_len);
		map = (pair == SSE_NULL)? 
			(sse_awk_val_map_t*)sse_awk_val_nil: 
			(sse_awk_val_map_t*)pair->val;
	}
	else
	{
		map = (var->type == SSE_AWK_NDE_GLOBALIDX)? 
		      	(sse_awk_val_map_t*)STACK_GLOBAL(run,var->id.idxa):
		      (var->type == SSE_AWK_NDE_LOCALIDX)? 
		      	(sse_awk_val_map_t*)STACK_LOCAL(run,var->id.idxa):
		      	(sse_awk_val_map_t*)STACK_ARG(run,var->id.idxa);
	}

	if (map->type == SSE_AWK_VAL_NIL)
	{
		/* the map is not initialized yet */
		sse_awk_val_t* tmp;

		tmp = sse_awk_makemapval (run);
		if (tmp == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);

		if (var->type == SSE_AWK_NDE_NAMEDIDX)
		{
			/* doesn't have to decrease the reference count 
			 * of the previous value here as it is done by 
			 * sse_awk_map_put */
			if (sse_awk_map_put (&run->named, 
				var->id.name, var->id.name_len, tmp) == SSE_NULL)
			{
				sse_awk_refupval (tmp);
				sse_awk_refdownval (run, tmp);
				PANIC (run, SSE_AWK_ENOMEM);		
			}

			sse_awk_refupval (tmp);
		}
		else if (var->type == SSE_AWK_NDE_GLOBALIDX)
		{
			sse_awk_refupval (tmp);
			if (sse_awk_setglobal (run, var->id.idxa, tmp) == -1)
			{
				sse_awk_refdownval (run, tmp);
				return SSE_NULL;
			}
			sse_awk_refdownval (run, tmp);
		}
		else if (var->type == SSE_AWK_NDE_LOCALIDX)
		{
			sse_awk_refdownval (run, (sse_awk_val_t*)map);
			STACK_LOCAL(run,var->id.idxa) = tmp;
			sse_awk_refupval (tmp);
		}
		else /* if (var->type == SSE_AWK_NDE_ARGIDX) */
		{
			sse_awk_refdownval (run, (sse_awk_val_t*)map);
			STACK_ARG(run,var->id.idxa) = tmp;
			sse_awk_refupval (tmp);
		}

		map = (sse_awk_val_map_t*) tmp;
	}
	else if (map->type != SSE_AWK_VAL_MAP)
	{
		PANIC (run, SSE_AWK_ENOTINDEXABLE);
	}

	str = __idxnde_to_str (run, var->idx, &len);
	if (str == SSE_NULL) return SSE_NULL;

/*
xp_printf (SSE_T("**** index str=>%s, map->ref=%d, map->type=%d\n"), str, (int)map->ref, (int)map->type);
*/
	n = sse_awk_map_putx (map->map, str, len, val, SSE_NULL);
	if (n < 0)
	{
		SSE_AWK_FREE (run->awk, str);
		PANIC (run, SSE_AWK_ENOMEM);
	}

	SSE_AWK_FREE (run->awk, str);
	sse_awk_refupval (val);
	return val;
}

static sse_awk_val_t* __do_assignment_pos (
	sse_awk_run_t* run, sse_awk_nde_pos_t* pos, sse_awk_val_t* val)
{
	sse_awk_val_t* v;
	sse_long_t lv;
	sse_real_t rv;
	sse_char_t* str;
	sse_size_t len;
	int n;

	v = __eval_expression (run, pos->val);
	if (v == SSE_NULL) return SSE_NULL;

	sse_awk_refupval (v);
	n = sse_awk_valtonum (run, v, &lv, &rv);
	sse_awk_refdownval (run, v);

	if (n == -1) PANIC (run, SSE_AWK_EPOSIDX); 
	if (n == 1) lv = (sse_long_t)rv;
	if (!IS_VALID_POSIDX(lv)) PANIC (run, SSE_AWK_EPOSIDX);

	if (val->type == SSE_AWK_VAL_STR)
	{
		str = ((sse_awk_val_str_t*)val)->buf;
		len = ((sse_awk_val_str_t*)val)->len;
	}
	else
	{
		str = sse_awk_valtostr (run, val, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &len);
		if (str == SSE_NULL) return SSE_NULL;
	}
	
	n = sse_awk_setrec (run, (sse_size_t)lv, str, len);

	if (val->type != SSE_AWK_VAL_STR) SSE_AWK_FREE (run->awk, str);

	if (n == -1) return SSE_NULL;
	return (lv == 0)? run->inrec.d0: run->inrec.flds[lv-1].val;
}

static sse_awk_val_t* __eval_binary (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	static binop_func_t __binop_func[] =
	{
		/* the order of the functions should be inline with
		 * the operator declaration in run.h */

		SSE_NULL, /* __eval_binop_lor */
		SSE_NULL, /* __eval_binop_land */
		SSE_NULL, /* __eval_binop_in */

		__eval_binop_bor,
		__eval_binop_bxor,
		__eval_binop_band,

		__eval_binop_eq,
		__eval_binop_ne,
		__eval_binop_gt,
		__eval_binop_ge,
		__eval_binop_lt,
		__eval_binop_le,

		__eval_binop_lshift,
		__eval_binop_rshift,
		
		__eval_binop_plus,
		__eval_binop_minus,
		__eval_binop_mul,
		__eval_binop_div,
		__eval_binop_mod,
		__eval_binop_exp,

		__eval_binop_concat,
		SSE_NULL, /* __eval_binop_ma */
		SSE_NULL  /* __eval_binop_nm */
	};

	sse_awk_nde_exp_t* exp = (sse_awk_nde_exp_t*)nde;
	sse_awk_val_t* left, * right, * res;

	sse_awk_assert (run->awk, exp->type == SSE_AWK_NDE_EXP_BIN);

	if (exp->opcode == SSE_AWK_BINOP_LAND)
	{
		res = __eval_binop_land (run, exp->left, exp->right);
	}
	else if (exp->opcode == SSE_AWK_BINOP_LOR)
	{
		res = __eval_binop_lor (run, exp->left, exp->right);
	}
	else if (exp->opcode == SSE_AWK_BINOP_IN)
	{
		/* treat the in operator specially */
		res = __eval_binop_in (run, exp->left, exp->right);
	}
	else if (exp->opcode == SSE_AWK_BINOP_NM)
	{
		res = __eval_binop_nm (run, exp->left, exp->right);
	}
	else if (exp->opcode == SSE_AWK_BINOP_MA)
	{
		res = __eval_binop_ma (run, exp->left, exp->right);
	}
	else
	{
		sse_awk_assert (run->awk, exp->left->next == SSE_NULL);
		left = __eval_expression (run, exp->left);
		if (left == SSE_NULL) return SSE_NULL;

		sse_awk_refupval (left);

		sse_awk_assert (run->awk, exp->right->next == SSE_NULL);
		right = __eval_expression (run, exp->right);
		if (right == SSE_NULL) 
		{
			sse_awk_refdownval (run, left);
			return SSE_NULL;
		}

		sse_awk_refupval (right);

		sse_awk_assert (run->awk, exp->opcode >= 0 && 
			exp->opcode < sse_countof(__binop_func));
		sse_awk_assert (run->awk, __binop_func[exp->opcode] != SSE_NULL);

		res = __binop_func[exp->opcode] (run, left, right);

		sse_awk_refdownval (run, left);
		sse_awk_refdownval (run, right);
	}

	return res;
}

static sse_awk_val_t* __eval_binop_lor (
	sse_awk_run_t* run, sse_awk_nde_t* left, sse_awk_nde_t* right)
{
	/*
	sse_awk_val_t* res = SSE_NULL;

	res = sse_awk_makeintval (run, 
		sse_awk_valtobool(run left) || sse_awk_valtobool(run,right));
	if (res == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);

	return res;
	*/

	/* short-circuit evaluation required special treatment */
	sse_awk_val_t* lv, * rv, * res;

	sse_awk_assert (run->awk, left->next == SSE_NULL);
	lv = __eval_expression (run, left);
	if (lv == SSE_NULL) return SSE_NULL;

	sse_awk_refupval (lv);
	if (sse_awk_valtobool (run, lv)) 
	{
		/*res = sse_awk_makeintval (run, 1);*/
		res = sse_awk_val_one;
	}
	else
	{
		sse_awk_assert (run->awk, right->next == SSE_NULL);
		rv = __eval_expression (run, right);
		if (rv == SSE_NULL)
		{
			sse_awk_refdownval (run, lv);
			return SSE_NULL;
		}
		sse_awk_refupval (rv);

		/*res = sse_awk_makeintval (run, (sse_awk_valtobool(run,rv)? 1: 0));*/
		res = sse_awk_valtobool(run,rv)? sse_awk_val_one: sse_awk_val_zero;
		sse_awk_refdownval (run, rv);
	}

	sse_awk_refdownval (run, lv);

	/*if (res == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);*/
	return res;
}

static sse_awk_val_t* __eval_binop_land (
	sse_awk_run_t* run, sse_awk_nde_t* left, sse_awk_nde_t* right)
{
	/*
	sse_awk_val_t* res = SSE_NULL;

	res = sse_awk_makeintval (run, 
		sse_awk_valtobool(run,left) && sse_awk_valtobool(run,right));
	if (res == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);

	return res;
	*/

	/* short-circuit evaluation required special treatment */
	sse_awk_val_t* lv, * rv, * res;

	sse_awk_assert (run->awk, left->next == SSE_NULL);
	lv = __eval_expression (run, left);
	if (lv == SSE_NULL) return SSE_NULL;

	sse_awk_refupval (lv);
	if (!sse_awk_valtobool (run, lv)) 
	{
		/*res = sse_awk_makeintval (run, 0);*/
		res = sse_awk_val_zero;
	}
	else
	{
		sse_awk_assert (run->awk, right->next == SSE_NULL);
		rv = __eval_expression (run, right);
		if (rv == SSE_NULL)
		{
			sse_awk_refdownval (run, lv);
			return SSE_NULL;
		}
		sse_awk_refupval (rv);

		/*res = sse_awk_makeintval (run, (sse_awk_valtobool(run,rv)? 1: 0));*/
		res = sse_awk_valtobool(run,rv)? sse_awk_val_one: sse_awk_val_zero;
		sse_awk_refdownval (run, rv);
	}

	sse_awk_refdownval (run, lv);

	/*if (res == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);*/
	return res;
}

static sse_awk_val_t* __eval_binop_in (
	sse_awk_run_t* run, sse_awk_nde_t* left, sse_awk_nde_t* right)
{
	sse_awk_val_t* rv;
	sse_char_t* str;
	sse_size_t len;

	if (right->type != SSE_AWK_NDE_GLOBAL &&
	    right->type != SSE_AWK_NDE_LOCAL &&
	    right->type != SSE_AWK_NDE_ARG &&
	    right->type != SSE_AWK_NDE_NAMED)
	{
		/* the compiler should have handled this case */
		sse_awk_assert (run->awk, !"should never happen - in needs a plain variable");
		PANIC (run, SSE_AWK_EINTERNAL);
		return SSE_NULL;
	}

	/* evaluate the left-hand side of the operator */
	str = (left->type == SSE_AWK_NDE_GRP)?
		__idxnde_to_str (run, ((sse_awk_nde_grp_t*)left)->body, &len):
		__idxnde_to_str (run, left, &len);
	if (str == SSE_NULL) return SSE_NULL;

	/* evaluate the right-hand side of the operator */
	sse_awk_assert (run->awk, right->next == SSE_NULL);
	rv = __eval_expression (run, right);
	if (rv == SSE_NULL) 
	{
		SSE_AWK_FREE (run->awk, str);
		return SSE_NULL;
	}

	sse_awk_refupval (rv);

	if (rv->type == SSE_AWK_VAL_NIL)
	{
		SSE_AWK_FREE (run->awk, str);
		sse_awk_refdownval (run, rv);
		return sse_awk_val_zero;
	}
	else if (rv->type == SSE_AWK_VAL_MAP)
	{
		sse_awk_val_t* res;
		sse_awk_map_t* map;

		map = ((sse_awk_val_map_t*)rv)->map;

		/*r = (sse_long_t)(sse_awk_map_get (map, str, len) != SSE_NULL);
		res = sse_awk_makeintval (run, r);
		if (res == SSE_NULL) 
		{
			SSE_AWK_FREE (run->awk, str);
			sse_awk_refdownval (run, rv);
			PANIC (run, SSE_AWK_ENOMEM);
		}*/
		res = (sse_awk_map_get (map, str, len) == SSE_NULL)? 
			sse_awk_val_zero: sse_awk_val_one;

		SSE_AWK_FREE (run->awk, str);
		sse_awk_refdownval (run, rv);
		return res;
	}

	/* need an array */
	/* TODO: change the error code to make it clearer */
	PANIC (run, SSE_AWK_EOPERAND); 
	return SSE_NULL;
}

static sse_awk_val_t* __eval_binop_bor (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	sse_awk_val_t* res = SSE_NULL;

	if (left->type == SSE_AWK_VAL_INT &&
	    right->type == SSE_AWK_VAL_INT)
	{
		sse_long_t r = 
			((sse_awk_val_int_t*)left)->val | 
			((sse_awk_val_int_t*)right)->val;
		res = sse_awk_makeintval (run, r);
	}
	else
	{
		PANIC (run, SSE_AWK_EOPERAND);
	}

	if (res == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);
	return res;
}

static sse_awk_val_t* __eval_binop_bxor (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	sse_awk_val_t* res = SSE_NULL;

	if (left->type == SSE_AWK_VAL_INT &&
	    right->type == SSE_AWK_VAL_INT)
	{
		sse_long_t r = 
			((sse_awk_val_int_t*)left)->val ^ 
			((sse_awk_val_int_t*)right)->val;
		res = sse_awk_makeintval (run, r);
	}
	else
	{
		PANIC (run, SSE_AWK_EOPERAND);
	}

	if (res == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);
	return res;
}

static sse_awk_val_t* __eval_binop_band (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	sse_awk_val_t* res = SSE_NULL;

	if (left->type == SSE_AWK_VAL_INT &&
	    right->type == SSE_AWK_VAL_INT)
	{
		sse_long_t r = 
			((sse_awk_val_int_t*)left)->val &
			((sse_awk_val_int_t*)right)->val;
		res = sse_awk_makeintval (run, r);
	}
	else
	{
		PANIC (run, SSE_AWK_EOPERAND);
	}

	if (res == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);
	return res;
}

static int __cmp_nil_nil (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	return 0;
}

static int __cmp_nil_int (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	if (((sse_awk_val_int_t*)right)->val < 0) return 1;
	if (((sse_awk_val_int_t*)right)->val > 0) return -1;
	return 0;
}

static int __cmp_nil_real (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	if (((sse_awk_val_real_t*)right)->val < 0) return 1;
	if (((sse_awk_val_real_t*)right)->val > 0) return -1;
	return 0;
}

static int __cmp_nil_str (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	return (((sse_awk_val_str_t*)right)->len == 0)? 0: -1;
}

static int __cmp_int_nil (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	if (((sse_awk_val_int_t*)left)->val > 0) return 1;
	if (((sse_awk_val_int_t*)left)->val < 0) return -1;
	return 0;
}

static int __cmp_int_int (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	if (((sse_awk_val_int_t*)left)->val > 
	    ((sse_awk_val_int_t*)right)->val) return 1;
	if (((sse_awk_val_int_t*)left)->val < 
	    ((sse_awk_val_int_t*)right)->val) return -1;
	return 0;
}

static int __cmp_int_real (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	if (((sse_awk_val_int_t*)left)->val > 
	    ((sse_awk_val_real_t*)right)->val) return 1;
	if (((sse_awk_val_int_t*)left)->val < 
	    ((sse_awk_val_real_t*)right)->val) return -1;
	return 0;
}

static int __cmp_int_str (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	sse_char_t* str;
	sse_size_t len;
	sse_long_t r;
	sse_real_t rr;
	int n;

	r = sse_awk_strxtolong (run->awk, 
		((sse_awk_val_str_t*)right)->buf,
		((sse_awk_val_str_t*)right)->len, 0, (const sse_char_t**)&str);
	if (str == ((sse_awk_val_str_t*)right)->buf + 
		   ((sse_awk_val_str_t*)right)->len)
	{
		if (((sse_awk_val_int_t*)left)->val > r) return 1;
		if (((sse_awk_val_int_t*)left)->val < r) return -1;
		return 0;
	}
/* TODO: should i do this???  conversion to real and comparision... */
	else if (*str == SSE_T('.') || *str == SSE_T('E') || *str == SSE_T('e'))
	{
		rr = sse_awk_strxtoreal (run->awk,
			((sse_awk_val_str_t*)right)->buf,
			((sse_awk_val_str_t*)right)->len, 
			(const sse_char_t**)&str);
		if (str == ((sse_awk_val_str_t*)right)->buf + 
			   ((sse_awk_val_str_t*)right)->len)
		{
			if (((sse_awk_val_int_t*)left)->val > rr) return 1;
			if (((sse_awk_val_int_t*)left)->val < rr) return -1;
			return 0;
		}
	}

	str = sse_awk_valtostr (run, left, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &len);
	if (str == SSE_NULL)
	{
		run->errnum = SSE_AWK_ENOMEM;
		return CMP_ERROR;
	}

	if (run->global.ignorecase)
	{
		n = sse_awk_strxncasecmp (
			run->awk, str, len,
			((sse_awk_val_str_t*)right)->buf, 
			((sse_awk_val_str_t*)right)->len);
	}
	else
	{
		n = sse_awk_strxncmp (
			str, len,
			((sse_awk_val_str_t*)right)->buf, 
			((sse_awk_val_str_t*)right)->len);
	}

	SSE_AWK_FREE (run->awk, str);
	return n;
}

static int __cmp_real_nil (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	if (((sse_awk_val_real_t*)left)->val > 0) return 1;
	if (((sse_awk_val_real_t*)left)->val < 0) return -1;
	return 0;
}

static int __cmp_real_int (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	if (((sse_awk_val_real_t*)left)->val > 
	    ((sse_awk_val_int_t*)right)->val) return 1;
	if (((sse_awk_val_real_t*)left)->val < 
	    ((sse_awk_val_int_t*)right)->val) return -1;
	return 0;
}

static int __cmp_real_real (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	if (((sse_awk_val_real_t*)left)->val > 
	    ((sse_awk_val_real_t*)right)->val) return 1;
	if (((sse_awk_val_real_t*)left)->val < 
	    ((sse_awk_val_real_t*)right)->val) return -1;
	return 0;
}

static int __cmp_real_str (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	sse_char_t* str;
	sse_size_t len;
	sse_real_t rr;
	int n;

	rr = sse_awk_strxtoreal (run->awk,
		((sse_awk_val_str_t*)right)->buf,
		((sse_awk_val_str_t*)right)->len, 
		(const sse_char_t**)&str);
	if (str == ((sse_awk_val_str_t*)right)->buf + 
		   ((sse_awk_val_str_t*)right)->len)
	{
		if (((sse_awk_val_real_t*)left)->val > rr) return 1;
		if (((sse_awk_val_real_t*)left)->val < rr) return -1;
		return 0;
	}

	str = sse_awk_valtostr (run, left, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &len);
	if (str == SSE_NULL)
	{
		run->errnum = SSE_AWK_ENOMEM;
		return CMP_ERROR;
	}

	if (run->global.ignorecase)
	{
		n = sse_awk_strxncasecmp (
			run->awk, str, len,
			((sse_awk_val_str_t*)right)->buf, 
			((sse_awk_val_str_t*)right)->len);
	}
	else
	{
		n = sse_awk_strxncmp (
			str, len,
			((sse_awk_val_str_t*)right)->buf, 
			((sse_awk_val_str_t*)right)->len);
	}

	SSE_AWK_FREE (run->awk, str);
	return n;
}

static int __cmp_str_nil (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	return (((sse_awk_val_str_t*)left)->len == 0)? 0: 1;
}

static int __cmp_str_int (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	return -__cmp_int_str (run, right, left);
}

static int __cmp_str_real (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	return -__cmp_real_str (run, right, left);
}

static int __cmp_str_str (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	int n;
	sse_awk_val_str_t* ls, * rs;

	ls = (sse_awk_val_str_t*)left;
	rs = (sse_awk_val_str_t*)right;

	if (run->global.ignorecase)
	{
		n = sse_awk_strxncasecmp (run->awk, 
			ls->buf, ls->len, rs->buf, rs->len);
	}
	else
	{
		n = sse_awk_strxncmp (
			ls->buf, ls->len, rs->buf, rs->len);
	}

	return n;
}

static int __cmp_val (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	typedef int (*cmp_val_t) (sse_awk_run_t*, sse_awk_val_t*, sse_awk_val_t*);

	static cmp_val_t func[] =
	{
		/* this table must be synchronized with 
		 * the SSE_AWK_VAL_XXX values in val.h */
		__cmp_nil_nil,  __cmp_nil_int,  __cmp_nil_real,  __cmp_nil_str,
		__cmp_int_nil,  __cmp_int_int,  __cmp_int_real,  __cmp_int_str,
		__cmp_real_nil, __cmp_real_int, __cmp_real_real, __cmp_real_str,
		__cmp_str_nil,  __cmp_str_int,  __cmp_str_real,  __cmp_str_str,
	};

	if (left->type == SSE_AWK_VAL_MAP || right->type == SSE_AWK_VAL_MAP)
	{
		/* a map can't be compared againt other values */
		run->errnum = SSE_AWK_EOPERAND;
		return CMP_ERROR; 
	}

	sse_awk_assert (run->awk, left->type >= SSE_AWK_VAL_NIL &&
	           left->type <= SSE_AWK_VAL_STR);
	sse_awk_assert (run->awk, right->type >= SSE_AWK_VAL_NIL &&
	           right->type <= SSE_AWK_VAL_STR);

	return func[left->type*4+right->type] (run, left, right);
}

static sse_awk_val_t* __eval_binop_eq (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	int n = __cmp_val (run, left, right);
	if (n == CMP_ERROR) return SSE_NULL;
	return (n == 0)? sse_awk_val_one: sse_awk_val_zero;
}

static sse_awk_val_t* __eval_binop_ne (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	int n = __cmp_val (run, left, right);
	if (n == CMP_ERROR) return SSE_NULL;
	return (n != 0)? sse_awk_val_one: sse_awk_val_zero;
}

static sse_awk_val_t* __eval_binop_gt (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	int n = __cmp_val (run, left, right);
	if (n == CMP_ERROR) return SSE_NULL;
	return (n > 0)? sse_awk_val_one: sse_awk_val_zero;
}

static sse_awk_val_t* __eval_binop_ge (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	int n = __cmp_val (run, left, right);
	if (n == CMP_ERROR) return SSE_NULL;
	return (n >= 0)? sse_awk_val_one: sse_awk_val_zero;
}

static sse_awk_val_t* __eval_binop_lt (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	int n = __cmp_val (run, left, right);
	if (n == CMP_ERROR) return SSE_NULL;
	return (n < 0)? sse_awk_val_one: sse_awk_val_zero;
}

static sse_awk_val_t* __eval_binop_le (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	int n = __cmp_val (run, left, right);
	if (n == CMP_ERROR) return SSE_NULL;
	return (n <= 0)? sse_awk_val_one: sse_awk_val_zero;
}

static sse_awk_val_t* __eval_binop_lshift (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	int n1, n2, n3;
	sse_long_t l1, l2;
	sse_real_t r1, r2;
	sse_awk_val_t* res;

	n1 = sse_awk_valtonum (run, left, &l1, &r1);
	n2 = sse_awk_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (run, SSE_AWK_EOPERAND); 

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		if  (l2 == 0) PANIC (run, SSE_AWK_EDIVBYZERO);
		res = sse_awk_makeintval (run, (sse_long_t)l1 << (sse_long_t)l2);
	}
	else PANIC (run, SSE_AWK_EOPERAND);

	if (res == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);
	return res;
}

static sse_awk_val_t* __eval_binop_rshift (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	int n1, n2, n3;
	sse_long_t l1, l2;
	sse_real_t r1, r2;
	sse_awk_val_t* res;

	n1 = sse_awk_valtonum (run, left, &l1, &r1);
	n2 = sse_awk_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (run, SSE_AWK_EOPERAND); 

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		if  (l2 == 0) PANIC (run, SSE_AWK_EDIVBYZERO);
		res = sse_awk_makeintval (run, (sse_long_t)l1 >> (sse_long_t)l2);
	}
	else PANIC (run, SSE_AWK_EOPERAND);

	if (res == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);
	return res;
}

static sse_awk_val_t* __eval_binop_plus (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	int n1, n2, n3;
	sse_long_t l1, l2;
	sse_real_t r1, r2;
	sse_awk_val_t* res;

	n1 = sse_awk_valtonum (run, left, &l1, &r1);
	n2 = sse_awk_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (run, SSE_AWK_EOPERAND); 
	/*
	n1  n2    n3
	0   0   = 0
	1   0   = 1
	0   1   = 2
	1   1   = 3
	*/
	n3 = n1 + (n2 << 1);
	sse_awk_assert (run->awk, n3 >= 0 && n3 <= 3);
	res = (n3 == 0)? sse_awk_makeintval(run,(sse_long_t)l1+(sse_long_t)l2):
	      (n3 == 1)? sse_awk_makerealval(run,(sse_real_t)r1+(sse_real_t)l2):
	      (n3 == 2)? sse_awk_makerealval(run,(sse_real_t)l1+(sse_real_t)r2):
	                 sse_awk_makerealval(run,(sse_real_t)r1+(sse_real_t)r2);

	if (res == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);
	return res;
}

static sse_awk_val_t* __eval_binop_minus (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	int n1, n2, n3;
	sse_long_t l1, l2;
	sse_real_t r1, r2;
	sse_awk_val_t* res;

	n1 = sse_awk_valtonum (run, left, &l1, &r1);
	n2 = sse_awk_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (run, SSE_AWK_EOPERAND);

	n3 = n1 + (n2 << 1);
	sse_awk_assert (run->awk, n3 >= 0 && n3 <= 3);
	res = (n3 == 0)? sse_awk_makeintval(run,(sse_long_t)l1-(sse_long_t)l2):
	      (n3 == 1)? sse_awk_makerealval(run,(sse_real_t)r1-(sse_real_t)l2):
	      (n3 == 2)? sse_awk_makerealval(run,(sse_real_t)l1-(sse_real_t)r2):
	                 sse_awk_makerealval(run,(sse_real_t)r1-(sse_real_t)r2);

	if (res == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);
	return res;
}

static sse_awk_val_t* __eval_binop_mul (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	int n1, n2, n3;
	sse_long_t l1, l2;
	sse_real_t r1, r2;
	sse_awk_val_t* res;

	n1 = sse_awk_valtonum (run, left, &l1, &r1);
	n2 = sse_awk_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (run, SSE_AWK_EOPERAND);

	n3 = n1 + (n2 << 1);
	sse_awk_assert (run->awk, n3 >= 0 && n3 <= 3);
	res = (n3 == 0)? sse_awk_makeintval(run,(sse_long_t)l1*(sse_long_t)l2):
	      (n3 == 1)? sse_awk_makerealval(run,(sse_real_t)r1*(sse_real_t)l2):
	      (n3 == 2)? sse_awk_makerealval(run,(sse_real_t)l1*(sse_real_t)r2):
	                 sse_awk_makerealval(run,(sse_real_t)r1*(sse_real_t)r2);

	if (res == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);
	return res;
}

static sse_awk_val_t* __eval_binop_div (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	int n1, n2, n3;
	sse_long_t l1, l2;
	sse_real_t r1, r2;
	sse_awk_val_t* res;

	n1 = sse_awk_valtonum (run, left, &l1, &r1);
	n2 = sse_awk_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (run, SSE_AWK_EOPERAND);

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		if  (l2 == 0) PANIC (run, SSE_AWK_EDIVBYZERO);
		res = sse_awk_makeintval (run, (sse_long_t)l1 / (sse_long_t)l2);
	}
	else if (n3 == 1)
	{
		res = sse_awk_makerealval (run, (sse_real_t)r1 / (sse_real_t)l2);
	}
	else if (n3 == 2)
	{
		res = sse_awk_makerealval (run, (sse_real_t)l1 / (sse_real_t)r2);
	}
	else
	{
		sse_awk_assert (run->awk, n3 == 3);
		res = sse_awk_makerealval (run, (sse_real_t)r1 / (sse_real_t)r2);
	}

	if (res == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);
	return res;
}

static sse_awk_val_t* __eval_binop_mod (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	int n1, n2, n3;
	sse_long_t l1, l2;
	sse_real_t r1, r2;
	sse_awk_val_t* res;

	n1 = sse_awk_valtonum (run, left, &l1, &r1);
	n2 = sse_awk_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (run, SSE_AWK_EOPERAND);

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		if  (l2 == 0) PANIC (run, SSE_AWK_EDIVBYZERO);
		res = sse_awk_makeintval (run, (sse_long_t)l1 % (sse_long_t)l2);
	}
	else PANIC (run, SSE_AWK_EOPERAND);

	if (res == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);
	return res;
}

static sse_awk_val_t* __eval_binop_exp (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	int n1, n2, n3;
	sse_long_t l1, l2;
	sse_real_t r1, r2;
	sse_awk_val_t* res;

	n1 = sse_awk_valtonum (run, left, &l1, &r1);
	n2 = sse_awk_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (run, SSE_AWK_EOPERAND);

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		sse_long_t v = 1;
		while (l2-- > 0) v *= l1;
		res = sse_awk_makeintval (run, v);
	}
	else if (n3 == 1)
	{
		/*res = sse_awk_makerealval (
			run, pow((sse_real_t)r1,(sse_real_t)l2));*/
		sse_real_t v = 1.0;
		while (l2-- > 0) v *= r1;
		res = sse_awk_makerealval (run, v);
	}
	else if (n3 == 2)
	{
		res = sse_awk_makerealval (
			run, pow((sse_real_t)l1,(sse_real_t)r2));
	}
	else
	{
		sse_awk_assert (run->awk, n3 == 3);
		res = sse_awk_makerealval (
			run, pow((sse_real_t)r1,(sse_real_t)r2));
	}

	if (res == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);
	return res;
}

static sse_awk_val_t* __eval_binop_concat (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right)
{
	sse_char_t* strl, * strr;
	sse_size_t strl_len, strr_len;
	sse_awk_val_t* res;

	strl = sse_awk_valtostr (
		run, left, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &strl_len);
	if (strl == SSE_NULL) return SSE_NULL;

	strr = sse_awk_valtostr (
		run, right, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &strr_len);
	if (strr == SSE_NULL) 
	{
		SSE_AWK_FREE (run->awk, strl);
		return SSE_NULL;
	}

	res = sse_awk_makestrval2 (run, strl, strl_len, strr, strr_len);
	if (res == SSE_NULL)
	{
		SSE_AWK_FREE (run->awk, strl);
		SSE_AWK_FREE (run->awk, strr);
		PANIC (run, SSE_AWK_ENOMEM);
	}

	SSE_AWK_FREE (run->awk, strl);
	SSE_AWK_FREE (run->awk, strr);

	return res;
}

static sse_awk_val_t* __eval_binop_ma (
	sse_awk_run_t* run, sse_awk_nde_t* left, sse_awk_nde_t* right)
{
	sse_awk_val_t* lv, * rv, * res;

	sse_awk_assert (run->awk, left->next == SSE_NULL);
	sse_awk_assert (run->awk, right->next == SSE_NULL);

	lv = __eval_expression (run, left);
	if (lv == SSE_NULL) 
	{
		return SSE_NULL;
	}

	sse_awk_refupval (lv);

	rv = __eval_expression0 (run, right);
	if (rv == SSE_NULL)
	{
		sse_awk_refdownval (run, lv);
		return SSE_NULL;
	}

	sse_awk_refupval (rv);

	res = __eval_binop_match0 (run, lv, rv, 1);

	sse_awk_refdownval (run, lv);
	sse_awk_refdownval (run, rv);

	return res;
}

static sse_awk_val_t* __eval_binop_nm (
	sse_awk_run_t* run, sse_awk_nde_t* left, sse_awk_nde_t* right)
{
	sse_awk_val_t* lv, * rv, * res;

	sse_awk_assert (run->awk, left->next == SSE_NULL);
	sse_awk_assert (run->awk, right->next == SSE_NULL);

	lv = __eval_expression (run, left);
	if (lv == SSE_NULL) return SSE_NULL;

	sse_awk_refupval (lv);

	rv = __eval_expression0 (run, right);
	if (rv == SSE_NULL)
	{
		sse_awk_refdownval (run, lv);
		return SSE_NULL;
	}

	sse_awk_refupval (rv);

	res = __eval_binop_match0 (run, lv, rv, 0);

	sse_awk_refdownval (run, lv);
	sse_awk_refdownval (run, rv);

	return res;
}

static sse_awk_val_t* __eval_binop_match0 (
	sse_awk_run_t* run, sse_awk_val_t* left, sse_awk_val_t* right, int ret)
{
	sse_awk_val_t* res;
	int n, errnum;
	sse_char_t* str;
	sse_size_t len;
	void* rex_code;

	if (right->type == SSE_AWK_VAL_REX)
	{
		rex_code = ((sse_awk_val_rex_t*)right)->code;
	}
	else if (right->type == SSE_AWK_VAL_STR)
	{
		rex_code = sse_awk_buildrex ( 
			run->awk,
			((sse_awk_val_str_t*)right)->buf,
			((sse_awk_val_str_t*)right)->len, &errnum);
		if (rex_code == SSE_NULL)
			PANIC (run, errnum);
	}
	else
	{
		str = sse_awk_valtostr (
			run, right, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &len);
		if (str == SSE_NULL) return SSE_NULL;

		rex_code = sse_awk_buildrex (run->awk, str, len, &errnum);
		if (rex_code == SSE_NULL)
		{
			SSE_AWK_FREE (run->awk, str);
			PANIC (run, errnum);
		}

		SSE_AWK_FREE (run->awk, str);
	}

	if (left->type == SSE_AWK_VAL_STR)
	{
		n = sse_awk_matchrex (
			run->awk, rex_code,
			((run->global.ignorecase)? SSE_AWK_REX_IGNORECASE: 0),
			((sse_awk_val_str_t*)left)->buf,
			((sse_awk_val_str_t*)left)->len,
			SSE_NULL, SSE_NULL, &errnum);
		if (n == -1) 
		{
			if (right->type != SSE_AWK_VAL_REX) 
				SSE_AWK_FREE (run->awk, rex_code);
			PANIC (run, errnum);
		}

		res = sse_awk_makeintval (run, (n == ret));
		if (res == SSE_NULL) 
		{
			if (right->type != SSE_AWK_VAL_REX) 
				SSE_AWK_FREE (run->awk, rex_code);
			PANIC (run, SSE_AWK_ENOMEM);
		}
	}
	else
	{
		str = sse_awk_valtostr (
			run, left, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &len);
		if (str == SSE_NULL) 
		{
			if (right->type != SSE_AWK_VAL_REX) 
				SSE_AWK_FREE (run->awk, rex_code);
			return SSE_NULL;
		}

		n = sse_awk_matchrex (
			run->awk, rex_code, 
			((run->global.ignorecase)? SSE_AWK_REX_IGNORECASE: 0),
			str, len, SSE_NULL, SSE_NULL, &errnum);
		if (n == -1) 
		{
			SSE_AWK_FREE (run->awk, str);
			if (right->type != SSE_AWK_VAL_REX) 
				SSE_AWK_FREE (run->awk, rex_code);
			PANIC (run, errnum);
		}

		res = sse_awk_makeintval (run, (n == ret));
		if (res == SSE_NULL) 
		{
			SSE_AWK_FREE (run->awk, str);
			if (right->type != SSE_AWK_VAL_REX) 
				SSE_AWK_FREE (run->awk, rex_code);
			PANIC (run, SSE_AWK_ENOMEM);
		}

		SSE_AWK_FREE (run->awk, str);
	}

	if (right->type != SSE_AWK_VAL_REX) SSE_AWK_FREE (run->awk, rex_code);
	return res;
}

static sse_awk_val_t* __eval_unary (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	sse_awk_val_t* left, * res = SSE_NULL;
	sse_awk_nde_exp_t* exp = (sse_awk_nde_exp_t*)nde;

	sse_awk_assert (run->awk, exp->type == SSE_AWK_NDE_EXP_UNR);
	sse_awk_assert (run->awk, exp->left != SSE_NULL && exp->right == SSE_NULL);

	sse_awk_assert (run->awk, exp->left->next == SSE_NULL);
	left = __eval_expression (run, exp->left);
	if (left == SSE_NULL) return SSE_NULL;

	sse_awk_refupval (left);

	if (exp->opcode == SSE_AWK_UNROP_PLUS) 
	{
		if (left->type == SSE_AWK_VAL_INT)
		{
			sse_long_t r = ((sse_awk_val_int_t*)left)->val;
			res = sse_awk_makeintval (run, r);
		}
		else if (left->type == SSE_AWK_VAL_REAL)
		{
			sse_real_t r = ((sse_awk_val_real_t*)left)->val;
			res = sse_awk_makerealval (run, r);
		}
		else
		{
			sse_awk_refdownval (run, left);
			PANIC (run, SSE_AWK_EOPERAND);
		}
	}
	else if (exp->opcode == SSE_AWK_UNROP_MINUS)
	{
		if (left->type == SSE_AWK_VAL_INT)
		{
			sse_long_t r = ((sse_awk_val_int_t*)left)->val;
			res = sse_awk_makeintval (run, -r);
		}
		else if (left->type == SSE_AWK_VAL_REAL)
		{
			sse_real_t r = ((sse_awk_val_real_t*)left)->val;
			res = sse_awk_makerealval (run, -r);
		}
		else
		{
			sse_awk_refdownval (run, left);
			PANIC (run, SSE_AWK_EOPERAND);
		}
	}
	else if (exp->opcode == SSE_AWK_UNROP_NOT)
	{
		if (left->type == SSE_AWK_VAL_INT)
		{
			sse_long_t r = ((sse_awk_val_int_t*)left)->val;
			res = sse_awk_makeintval (run, !r);
		}
		else if (left->type == SSE_AWK_VAL_REAL)
		{
			sse_real_t r = ((sse_awk_val_real_t*)left)->val;
			res = sse_awk_makerealval (run, !r);
		}
		else
		{
			sse_awk_refdownval (run, left);
			PANIC (run, SSE_AWK_EOPERAND);
		}
	}
	else if (exp->opcode == SSE_AWK_UNROP_BNOT)
	{
		if (left->type == SSE_AWK_VAL_INT)
		{
			sse_long_t r = ((sse_awk_val_int_t*)left)->val;
			res = sse_awk_makeintval (run, ~r);
		}
		else
		{
			sse_awk_refdownval (run, left);
			PANIC (run, SSE_AWK_EOPERAND);
		}
	}

	if (res == SSE_NULL)
	{
		sse_awk_refdownval (run, left);
		PANIC (run, SSE_AWK_ENOMEM);
	}

	sse_awk_refdownval (run, left);
	return res;
}

static sse_awk_val_t* __eval_incpre (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	sse_awk_val_t* left, * res;
	sse_awk_nde_exp_t* exp = (sse_awk_nde_exp_t*)nde;

	sse_awk_assert (run->awk, exp->type == SSE_AWK_NDE_EXP_INCPRE);
	sse_awk_assert (run->awk, exp->left != SSE_NULL && exp->right == SSE_NULL);

	/* this way of checking if the l-value is assignable is
	 * ugly as it is dependent of the values defined in tree.h.
	 * but let's keep going this way for the time being. */
	if (exp->left->type < SSE_AWK_NDE_NAMED ||
	    exp->left->type > SSE_AWK_NDE_ARGIDX)
	{
		PANIC (run, SSE_AWK_EOPERAND);
	}

	sse_awk_assert (run->awk, exp->left->next == SSE_NULL);
	left = __eval_expression (run, exp->left);
	if (left == SSE_NULL) return SSE_NULL;

	sse_awk_refupval (left);

	if (exp->opcode == SSE_AWK_INCOP_PLUS) 
	{
		if (left->type == SSE_AWK_VAL_INT)
		{
			sse_long_t r = ((sse_awk_val_int_t*)left)->val;
			res = sse_awk_makeintval (run, r + 1);
			if (res == SSE_NULL) 
			{
				sse_awk_refdownval (run, left);
				PANIC (run, SSE_AWK_ENOMEM);
			}
		}
		else if (left->type == SSE_AWK_VAL_REAL)
		{
			sse_real_t r = ((sse_awk_val_real_t*)left)->val;
			res = sse_awk_makerealval (run, r + 1.0);
			if (res == SSE_NULL) 
			{
				sse_awk_refdownval (run, left);
				PANIC (run, SSE_AWK_ENOMEM);
			}
		}
		else
		{
			sse_long_t v1;
			sse_real_t v2;
			int n;

			n = sse_awk_valtonum (run, left, &v1, &v2);
			if (n == -1)
			{
				sse_awk_refdownval (run, left);
				PANIC (run, SSE_AWK_EOPERAND);
			}

			if (n == 0) 
			{
				res = sse_awk_makeintval (run, v1 + 1);
			}
			else /* if (n == 1) */
			{
				sse_awk_assert (run->awk, n == 1);
				res = sse_awk_makerealval (run, v2 + 1.0);
			}

			if (res == SSE_NULL) 
			{
				sse_awk_refdownval (run, left);
				PANIC (run, SSE_AWK_ENOMEM);
			}
		}
	}
	else if (exp->opcode == SSE_AWK_INCOP_MINUS)
	{
		if (left->type == SSE_AWK_VAL_INT)
		{
			sse_long_t r = ((sse_awk_val_int_t*)left)->val;
			res = sse_awk_makeintval (run, r - 1);
			if (res == SSE_NULL) 
			{
				sse_awk_refdownval (run, left);
				PANIC (run, SSE_AWK_ENOMEM);
			}
		}
		else if (left->type == SSE_AWK_VAL_REAL)
		{
			sse_real_t r = ((sse_awk_val_real_t*)left)->val;
			res = sse_awk_makerealval (run, r - 1.0);
			if (res == SSE_NULL) 
			{
				sse_awk_refdownval (run, left);
				PANIC (run, SSE_AWK_ENOMEM);
			}
		}
		else
		{
			sse_long_t v1;
			sse_real_t v2;
			int n;

			n = sse_awk_valtonum (run, left, &v1, &v2);
			if (n == -1)
			{
				sse_awk_refdownval (run, left);
				PANIC (run, SSE_AWK_EOPERAND);
			}

			if (n == 0) 
			{
				res = sse_awk_makeintval (run, v1 - 1);
			}
			else /* if (n == 1) */
			{
				sse_awk_assert (run->awk, n == 1);
				res = sse_awk_makerealval (run, v2 - 1.0);
			}

			if (res == SSE_NULL) 
			{
				sse_awk_refdownval (run, left);
				PANIC (run, SSE_AWK_ENOMEM);
			}
		}
	}
	else
	{
		sse_awk_assert (run->awk, !"should never happen - invalid opcode");
		sse_awk_refdownval (run, left);
		PANIC (run, SSE_AWK_EINTERNAL);
	}

	if (__do_assignment (run, exp->left, res) == SSE_NULL)
	{
		sse_awk_refdownval (run, left);
		return SSE_NULL;
	}

	sse_awk_refdownval (run, left);
	return res;
}

static sse_awk_val_t* __eval_incpst (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	sse_awk_val_t* left, * res, * res2;
	sse_awk_nde_exp_t* exp = (sse_awk_nde_exp_t*)nde;

	sse_awk_assert (run->awk, exp->type == SSE_AWK_NDE_EXP_INCPST);
	sse_awk_assert (run->awk, exp->left != SSE_NULL && exp->right == SSE_NULL);

	/* this way of checking if the l-value is assignable is
	 * ugly as it is dependent of the values defined in tree.h.
	 * but let's keep going this way for the time being. */
	if (exp->left->type < SSE_AWK_NDE_NAMED ||
	    exp->left->type > SSE_AWK_NDE_ARGIDX)
	{
		PANIC (run, SSE_AWK_EOPERAND);
	}

	sse_awk_assert (run->awk, exp->left->next == SSE_NULL);
	left = __eval_expression (run, exp->left);
	if (left == SSE_NULL) return SSE_NULL;

	sse_awk_refupval (left);

	if (exp->opcode == SSE_AWK_INCOP_PLUS) 
	{
		if (left->type == SSE_AWK_VAL_INT)
		{
			sse_long_t r = ((sse_awk_val_int_t*)left)->val;
			res = sse_awk_makeintval (run, r);
			if (res == SSE_NULL) 
			{
				sse_awk_refdownval (run, left);
				PANIC (run, SSE_AWK_ENOMEM);
			}

			res2 = sse_awk_makeintval (run, r + 1);
			if (res2 == SSE_NULL)
			{
				sse_awk_refdownval (run, left);
				sse_awk_freeval (run, res, sse_true);
				PANIC (run, SSE_AWK_ENOMEM);
			}
		}
		else if (left->type == SSE_AWK_VAL_REAL)
		{
			sse_real_t r = ((sse_awk_val_real_t*)left)->val;
			res = sse_awk_makerealval (run, r);
			if (res == SSE_NULL) 
			{
				sse_awk_refdownval (run, left);
				PANIC (run, SSE_AWK_ENOMEM);
			}

			res2 = sse_awk_makerealval (run, r + 1.0);
			if (res2 == SSE_NULL)
			{
				sse_awk_refdownval (run, left);
				sse_awk_freeval (run, res, sse_true);
				PANIC (run, SSE_AWK_ENOMEM);
			}
		}
		else
		{
			sse_long_t v1;
			sse_real_t v2;
			int n;

			n = sse_awk_valtonum (run, left, &v1, &v2);
			if (n == -1)
			{
				sse_awk_refdownval (run, left);
				PANIC (run, SSE_AWK_EOPERAND);
			}

			if (n == 0) 
			{
				res = sse_awk_makeintval (run, v1);
				if (res == SSE_NULL)
				{
					sse_awk_refdownval (run, left);
					PANIC (run, SSE_AWK_ENOMEM);
				}

				res2 = sse_awk_makeintval (run, v1 + 1);
				if (res2 == SSE_NULL)
				{
					sse_awk_refdownval (run, left);
					sse_awk_freeval (run, res, sse_true);
					PANIC (run, SSE_AWK_ENOMEM);
				}
			}
			else /* if (n == 1) */
			{
				sse_awk_assert (run->awk, n == 1);
				res = sse_awk_makerealval (run, v2);
				if (res == SSE_NULL)
				{
					sse_awk_refdownval (run, left);
					PANIC (run, SSE_AWK_ENOMEM);
				}

				res2 = sse_awk_makerealval (run, v2 + 1.0);
				if (res2 == SSE_NULL)
				{
					sse_awk_refdownval (run, left);
					sse_awk_freeval (run, res, sse_true);
					PANIC (run, SSE_AWK_ENOMEM);
				}
			}
		}
	}
	else if (exp->opcode == SSE_AWK_INCOP_MINUS)
	{
		if (left->type == SSE_AWK_VAL_INT)
		{
			sse_long_t r = ((sse_awk_val_int_t*)left)->val;
			res = sse_awk_makeintval (run, r);
			if (res == SSE_NULL) 
			{
				sse_awk_refdownval (run, left);
				PANIC (run, SSE_AWK_ENOMEM);
			}

			res2 = sse_awk_makeintval (run, r - 1);
			if (res2 == SSE_NULL)
			{
				sse_awk_refdownval (run, left);
				sse_awk_freeval (run, res, sse_true);
				PANIC (run, SSE_AWK_ENOMEM);
			}
		}
		else if (left->type == SSE_AWK_VAL_REAL)
		{
			sse_real_t r = ((sse_awk_val_real_t*)left)->val;
			res = sse_awk_makerealval (run, r);
			if (res == SSE_NULL) 
			{
				sse_awk_refdownval (run, left);
				PANIC (run, SSE_AWK_ENOMEM);
			}

			res2 = sse_awk_makerealval (run, r - 1.0);
			if (res2 == SSE_NULL)
			{
				sse_awk_refdownval (run, left);
				sse_awk_freeval (run, res, sse_true);
				PANIC (run, SSE_AWK_ENOMEM);
			}
		}
		else
		{
			sse_long_t v1;
			sse_real_t v2;
			int n;

			n = sse_awk_valtonum (run, left, &v1, &v2);
			if (n == -1)
			{
				sse_awk_refdownval (run, left);
				PANIC (run, SSE_AWK_EOPERAND);
			}

			if (n == 0) 
			{
				res = sse_awk_makeintval (run, v1);
				if (res == SSE_NULL)
				{
					sse_awk_refdownval (run, left);
					PANIC (run, SSE_AWK_ENOMEM);
				}

				res2 = sse_awk_makeintval (run, v1 - 1);
				if (res2 == SSE_NULL)
				{
					sse_awk_refdownval (run, left);
					sse_awk_freeval (run, res, sse_true);
					PANIC (run, SSE_AWK_ENOMEM);
				}
			}
			else /* if (n == 1) */
			{
				sse_awk_assert (run->awk, n == 1);
				res = sse_awk_makerealval (run, v2);
				if (res == SSE_NULL)
				{
					sse_awk_refdownval (run, left);
					PANIC (run, SSE_AWK_ENOMEM);
				}

				res2 = sse_awk_makerealval (run, v2 - 1.0);
				if (res2 == SSE_NULL)
				{
					sse_awk_refdownval (run, left);
					sse_awk_freeval (run, res, sse_true);
					PANIC (run, SSE_AWK_ENOMEM);
				}
			}
		}
	}
	else
	{
		sse_awk_assert (run->awk, !"should never happen - invalid opcode");
		sse_awk_refdownval (run, left);
		PANIC (run, SSE_AWK_EINTERNAL);
	}

	if (__do_assignment (run, exp->left, res2) == SSE_NULL)
	{
		sse_awk_refdownval (run, left);
		return SSE_NULL;
	}

	sse_awk_refdownval (run, left);
	return res;
}

static sse_awk_val_t* __eval_cnd (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	sse_awk_val_t* tv, * v;
	sse_awk_nde_cnd_t* cnd = (sse_awk_nde_cnd_t*)nde;

	sse_awk_assert (run->awk, cnd->test->next == SSE_NULL);
	tv = __eval_expression (run, cnd->test);
	if (tv == SSE_NULL) return SSE_NULL;

	sse_awk_refupval (tv);

	sse_awk_assert (run->awk, cnd->left->next == SSE_NULL &&
	           cnd->right->next == SSE_NULL);
	v = (sse_awk_valtobool (run, tv))?
		__eval_expression (run, cnd->left):
		__eval_expression (run, cnd->right);

	sse_awk_refdownval (run, tv);
	return v;
}

static sse_awk_val_t* __eval_bfn (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	sse_awk_nde_call_t* call = (sse_awk_nde_call_t*)nde;

	/* built-in function */
	if (call->nargs < call->what.bfn.min_args)
	{
		PANIC (run, SSE_AWK_ETOOFEWARGS);
	}

	if (call->nargs > call->what.bfn.max_args)
	{
		PANIC (run, SSE_AWK_ETOOMANYARGS);
	}

	return __eval_call (run, nde, call->what.bfn.arg_spec, SSE_NULL);
}

static sse_awk_val_t* __eval_afn (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	sse_awk_nde_call_t* call = (sse_awk_nde_call_t*)nde;
	sse_awk_afn_t* afn;
	sse_awk_pair_t* pair;

	pair = sse_awk_map_get (&run->awk->tree.afns, 
		call->what.afn.name, call->what.afn.name_len);
	if (pair == SSE_NULL) PANIC (run, SSE_AWK_ENOSUCHFUNC);

	afn = (sse_awk_afn_t*)pair->val;
	sse_awk_assert (run->awk, afn != SSE_NULL);

	if (call->nargs > afn->nargs)
	{
		/* TODO: is this correct? what if i want to allow arbitarary numbers of arguments? */
		PANIC (run, SSE_AWK_ETOOMANYARGS);
	}

	return __eval_call (run, nde, SSE_NULL, afn);
}


/* run->stack_base has not been set for this  
 * stack frame. so STACK_ARG cannot be used */ 
/*sse_awk_refdownval (run, STACK_ARG(run,nargs));*/ 
#define UNWIND_RUN_STACK(run,nargs) \
	do { \
		while ((nargs) > 0) \
		{ \
			--(nargs); \
			sse_awk_refdownval ((run), \
				(run)->stack[(run)->stack_top-1]); \
			__raw_pop (run); \
		} \
		__raw_pop (run); \
		__raw_pop (run); \
		__raw_pop (run); \
	} while (0)

static sse_awk_val_t* __eval_call (
	sse_awk_run_t* run, sse_awk_nde_t* nde, 
	const sse_char_t* bfn_arg_spec, sse_awk_afn_t* afn)
{
	sse_awk_nde_call_t* call = (sse_awk_nde_call_t*)nde;
	sse_size_t saved_stack_top;
	sse_size_t nargs, i;
	sse_awk_nde_t* p;
	sse_awk_val_t* v;
	int n;

	/* 
	 * ---------------------
	 *  localn               <- stack top
	 * ---------------------
	 *  ....
	 * ---------------------
	 *  local0               local variables are pushed by __run_block
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
	 *  globaln
	 * ---------------------
	 *  ....
	 * ---------------------
	 *  global0
	 * ---------------------
	 */

	sse_awk_assert (run->awk, sse_sizeof(void*) >= sse_sizeof(run->stack_top));
	sse_awk_assert (run->awk, sse_sizeof(void*) >= sse_sizeof(run->stack_base));

	saved_stack_top = run->stack_top;

/*xp_printf (SSE_T("setting up function stack frame stack_top = %ld stack_base = %ld\n"), run->stack_top, run->stack_base); */
	if (__raw_push(run,(void*)run->stack_base) == -1) 
	{
		PANIC (run, SSE_AWK_ENOMEM);
	}

	if (__raw_push(run,(void*)saved_stack_top) == -1) 
	{
		__raw_pop (run);
		PANIC (run, SSE_AWK_ENOMEM);
	}

	/* secure space for a return value. */
	if (__raw_push(run,sse_awk_val_nil) == -1)
	{
		__raw_pop (run);
		__raw_pop (run);
		PANIC (run, SSE_AWK_ENOMEM);
	}

	/* secure space for nargs */
	if (__raw_push(run,sse_awk_val_nil) == -1)
	{
		__raw_pop (run);
		__raw_pop (run);
		__raw_pop (run);
		PANIC (run, SSE_AWK_ENOMEM);
	}

	nargs = 0;
	p = call->args;
	while (p != SSE_NULL)
	{
		sse_awk_assert (run->awk, bfn_arg_spec == SSE_NULL ||
		           (bfn_arg_spec != SSE_NULL && 
		            sse_awk_strlen(bfn_arg_spec) > nargs));

		if (bfn_arg_spec != SSE_NULL && 
		    bfn_arg_spec[nargs] == SSE_T('r'))
		{
			sse_awk_val_t** ref;
			      
			if (__get_reference (run, p, &ref) == -1)
			{
				UNWIND_RUN_STACK (run, nargs);
				return SSE_NULL;
			}

			/* p->type-SSE_AWK_NDE_NAMED assumes that the
			 * derived value matches SSE_AWK_VAL_REF_XXX */
			v = sse_awk_makerefval (
				run, p->type-SSE_AWK_NDE_NAMED, ref);
		}
		else if (bfn_arg_spec != SSE_NULL && 
		         bfn_arg_spec[nargs] == SSE_T('x'))
		{
			/* a regular expression is passed to 
			 * the function as it is */
			v = __eval_expression0 (run, p);
		}
		else
		{
			v = __eval_expression (run, p);
		}
		if (v == SSE_NULL)
		{
			UNWIND_RUN_STACK (run, nargs);
			return SSE_NULL;
		}

#if 0
		if (bfn_arg_spec != SSE_NULL && 
		    bfn_arg_spec[nargs] == SSE_T('r'))
		{
			sse_awk_val_t** ref;
			sse_awk_val_t* tmp;
			      
			ref = __get_reference (run, p);
			if (ref == SSE_NULL)
			{
				sse_awk_refupval (v);
				sse_awk_refdownval (run, v);

				UNWIND_RUN_STACK (run, nargs);
				return SSE_NULL;
			}

			/* p->type-SSE_AWK_NDE_NAMED assumes that the
			 * derived value matches SSE_AWK_VAL_REF_XXX */
			tmp = sse_awk_makerefval (
				run, p->type-SSE_AWK_NDE_NAMED, ref);
			if (tmp == SSE_NULL)
			{
				sse_awk_refupval (v);
				sse_awk_refdownval (run, v);

				UNWIND_RUN_STACK (run, nargs);
				PANIC (run, SSE_AWK_ENOMEM);
			}

			sse_awk_refupval (v);
			sse_awk_refdownval (run, v);

			v = tmp;
		}
#endif

		if (__raw_push(run,v) == -1) 
		{
			/* ugly - v needs to be freed if it doesn't have
			 * any reference. but its reference has not been 
			 * updated yet as it is carried out after the 
			 * successful stack push. so it adds up a reference 
			 * and dereferences it */
			sse_awk_refupval (v);
			sse_awk_refdownval (run, v);

			UNWIND_RUN_STACK (run, nargs);
			PANIC (run, SSE_AWK_ENOMEM);
		}

		sse_awk_refupval (v);
		nargs++;
		p = p->next;
	}

	sse_awk_assert (run->awk, nargs == call->nargs);

	if (afn != SSE_NULL)
	{
		/* extra step for normal awk functions */

		while (nargs < afn->nargs)
		{
			/* push as many nils as the number of missing actual arguments */
			if (__raw_push(run,sse_awk_val_nil) == -1)
			{
				UNWIND_RUN_STACK (run, nargs);
				PANIC (run, SSE_AWK_ENOMEM);
			}

			nargs++;
		}
	}

	run->stack_base = saved_stack_top;
	STACK_NARGS(run) = (void*)nargs;
	
/*xp_printf (SSE_T("running function body\n")); */

	if (afn != SSE_NULL)
	{
		/* normal awk function */
		sse_awk_assert (run->awk, afn->body->type == SSE_AWK_NDE_BLK);
		n = __run_block(run,(sse_awk_nde_blk_t*)afn->body);
	}
	else
	{
		n = 0;

		/* built-in function */
		sse_awk_assert (run->awk, call->nargs >= call->what.bfn.min_args &&
		           call->nargs <= call->what.bfn.max_args);

		if (call->what.bfn.handler != SSE_NULL)
			n = call->what.bfn.handler (run);
	}

/*xp_printf (SSE_T("block run complete\n")); */

	/* refdown args in the run.stack */
	nargs = (sse_size_t)STACK_NARGS(run);
/*xp_printf (SSE_T("block run complete nargs = %d\n"), (int)nargs); */
	for (i = 0; i < nargs; i++)
	{
		sse_awk_refdownval (run, STACK_ARG(run,i));
	}
/*xp_printf (SSE_T("got return value\n")); */

	/* this trick has been mentioned in __run_return.
	 * adjust the reference count of the return value.
	 * the value must not be freed even if the reference count
	 * is decremented to zero because its reference has been incremented 
	 * in __run_return regardless of its reference count. */
	v = STACK_RETVAL(run);
	sse_awk_refdownval_nofree (run, v);

	run->stack_top =  (sse_size_t)run->stack[run->stack_base+1];
	run->stack_base = (sse_size_t)run->stack[run->stack_base+0];

	if (run->exit_level == EXIT_FUNCTION) run->exit_level = EXIT_NONE;

/*xp_printf (SSE_T("returning from function stack_top=%ld, stack_base=%ld\n"), run->stack_top, run->stack_base); */
	return (n == -1)? SSE_NULL: v;
}

static int __get_reference (
	sse_awk_run_t* run, sse_awk_nde_t* nde, sse_awk_val_t*** ref)
{
	sse_awk_nde_var_t* tgt = (sse_awk_nde_var_t*)nde;
	sse_awk_val_t** tmp;

	/* refer to __eval_indexed for application of a similar concept */

	if (nde->type == SSE_AWK_NDE_NAMED)
	{
		sse_awk_pair_t* pair;

		pair = sse_awk_map_get (
			&run->named, tgt->id.name, tgt->id.name_len);
		if (pair == SSE_NULL)
		{
			/* it is bad that the named variable has to be
			 * created in the function named "__get_refernce".
			 * would there be any better ways to avoid this? */
			pair = sse_awk_map_put (
				&run->named, tgt->id.name,
				tgt->id.name_len, sse_awk_val_nil);
			if (pair == SSE_NULL) PANIC_I (run, SSE_AWK_ENOMEM);
		}

		*ref = (sse_awk_val_t**)&pair->val;
		return 0;
	}

	if (nde->type == SSE_AWK_NDE_GLOBAL)
	{
		*ref = (sse_awk_val_t**)&STACK_GLOBAL(run,tgt->id.idxa);
		return 0;
	}

	if (nde->type == SSE_AWK_NDE_LOCAL)
	{
		*ref = (sse_awk_val_t**)&STACK_LOCAL(run,tgt->id.idxa);
		return 0;
	}

	if (nde->type == SSE_AWK_NDE_ARG)
	{
		*ref = (sse_awk_val_t**)&STACK_ARG(run,tgt->id.idxa);
		return 0;
	}

	if (nde->type == SSE_AWK_NDE_NAMEDIDX)
	{
		sse_awk_pair_t* pair;

		pair = sse_awk_map_get (
			&run->named, tgt->id.name, tgt->id.name_len);
		if (pair == SSE_NULL)
		{
			pair = sse_awk_map_put (
				&run->named, tgt->id.name,
				tgt->id.name_len, sse_awk_val_nil);
			if (pair == SSE_NULL) PANIC_I (run, SSE_AWK_ENOMEM);
		}

		tmp = __get_reference_indexed (
			run, tgt, (sse_awk_val_t**)&pair->val);
		if (tmp == SSE_NULL) return -1;
		*ref = tmp;
	}

	if (nde->type == SSE_AWK_NDE_GLOBALIDX)
	{
		tmp = __get_reference_indexed (run, tgt, 
			(sse_awk_val_t**)&STACK_GLOBAL(run,tgt->id.idxa));
		if (tmp == SSE_NULL) return -1;
		*ref = tmp;
	}

	if (nde->type == SSE_AWK_NDE_LOCALIDX)
	{
		tmp = __get_reference_indexed (run, tgt, 
			(sse_awk_val_t**)&STACK_LOCAL(run,tgt->id.idxa));
		if (tmp == SSE_NULL) return -1;
		*ref = tmp;
	}

	if (nde->type == SSE_AWK_NDE_ARGIDX)
	{
		tmp = __get_reference_indexed (run, tgt, 
			(sse_awk_val_t**)&STACK_ARG(run,tgt->id.idxa));
		if (tmp == SSE_NULL) return -1;
		*ref = tmp;
	}

	if (nde->type == SSE_AWK_NDE_POS)
	{
		int n;
		sse_long_t lv;
		sse_real_t rv;
		sse_awk_val_t* v;

		/* the position number is returned for the positional 
		 * variable unlike other reference types. */
		v = __eval_expression (run, ((sse_awk_nde_pos_t*)nde)->val);
		if (v == SSE_NULL) return -1;

		sse_awk_refupval (v);
		n = sse_awk_valtonum (run, v, &lv, &rv);
		sse_awk_refdownval (run, v);

		if (n == -1) PANIC_I (run, SSE_AWK_EPOSIDX);
		if (n == 1) lv = (sse_long_t)rv;
		if (!IS_VALID_POSIDX(lv)) PANIC_I (run, SSE_AWK_EPOSIDX);

		*ref = (sse_awk_val_t**)((sse_size_t)lv);
		return 0;
	}

	PANIC_I (run, SSE_AWK_ENOTREFERENCEABLE);
}

static sse_awk_val_t** __get_reference_indexed (
	sse_awk_run_t* run, sse_awk_nde_var_t* nde, sse_awk_val_t** val)
{
	sse_awk_pair_t* pair;
	sse_char_t* str;
	sse_size_t len;

	sse_awk_assert (run->awk, val != SSE_NULL);

	if ((*val)->type == SSE_AWK_VAL_NIL)
	{
		sse_awk_val_t* tmp;

		tmp = sse_awk_makemapval (run);
		if (tmp == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);

		sse_awk_refdownval (run, *val);
		*val = tmp;
		sse_awk_refupval ((sse_awk_val_t*)*val);
	}
	else if ((*val)->type != SSE_AWK_VAL_MAP) 
	{
		PANIC (run, SSE_AWK_ENOTINDEXABLE);
	}

	sse_awk_assert (run->awk, nde->idx != SSE_NULL);

	str = __idxnde_to_str (run, nde->idx, &len);
	if (str == SSE_NULL) return SSE_NULL;

	pair = sse_awk_map_get ((*(sse_awk_val_map_t**)val)->map, str, len);
	if (pair == SSE_NULL)
	{
		pair = sse_awk_map_put (
			(*(sse_awk_val_map_t**)val)->map, 
			str, len, sse_awk_val_nil);
		if (pair == SSE_NULL)
		{
			SSE_AWK_FREE (run->awk, str);
			PANIC (run, SSE_AWK_ENOMEM);
		}

		sse_awk_refupval (pair->val);
	}

	SSE_AWK_FREE (run->awk, str);
	return (sse_awk_val_t**)&pair->val;
}

static sse_awk_val_t* __eval_int (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	sse_awk_val_t* val;

	val = sse_awk_makeintval (run, ((sse_awk_nde_int_t*)nde)->val);
	if (val == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);
	((sse_awk_val_int_t*)val)->nde = (sse_awk_nde_int_t*)nde; 

	return val;
}

static sse_awk_val_t* __eval_real (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	sse_awk_val_t* val;

	val = sse_awk_makerealval (run, ((sse_awk_nde_real_t*)nde)->val);
	if (val == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);
	((sse_awk_val_real_t*)val)->nde = (sse_awk_nde_real_t*)nde;

	return val;
}

static sse_awk_val_t* __eval_str (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	sse_awk_val_t* val;

	val = sse_awk_makestrval (run,
		((sse_awk_nde_str_t*)nde)->buf,
		((sse_awk_nde_str_t*)nde)->len);
	if (val == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);

	return val;
}

static sse_awk_val_t* __eval_rex (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	sse_awk_val_t* val;

	val = sse_awk_makerexval (run,
		((sse_awk_nde_rex_t*)nde)->buf,
		((sse_awk_nde_rex_t*)nde)->len,
		((sse_awk_nde_rex_t*)nde)->code);
	if (val == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);

	return val;
}

static sse_awk_val_t* __eval_named (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	sse_awk_pair_t* pair;
		       
	pair = sse_awk_map_get (&run->named, 
		((sse_awk_nde_var_t*)nde)->id.name, 
		((sse_awk_nde_var_t*)nde)->id.name_len);

	return (pair == SSE_NULL)? sse_awk_val_nil: pair->val;
}

static sse_awk_val_t* __eval_global (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	return STACK_GLOBAL(run,((sse_awk_nde_var_t*)nde)->id.idxa);
}

static sse_awk_val_t* __eval_local (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	return STACK_LOCAL(run,((sse_awk_nde_var_t*)nde)->id.idxa);
}

static sse_awk_val_t* __eval_arg (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	return STACK_ARG(run,((sse_awk_nde_var_t*)nde)->id.idxa);
}

static sse_awk_val_t* __eval_indexed (
	sse_awk_run_t* run, sse_awk_nde_var_t* nde, sse_awk_val_t** val)
{
	sse_awk_pair_t* pair;
	sse_char_t* str;
	sse_size_t len;

	sse_awk_assert (run->awk, val != SSE_NULL);

	if ((*val)->type == SSE_AWK_VAL_NIL)
	{
		sse_awk_val_t* tmp;

		tmp = sse_awk_makemapval (run);
		if (tmp == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);

		sse_awk_refdownval (run, *val);
		*val = tmp;
		sse_awk_refupval ((sse_awk_val_t*)*val);
	}
	else if ((*val)->type != SSE_AWK_VAL_MAP) 
	{
	        PANIC (run, SSE_AWK_ENOTINDEXABLE);
	}

	sse_awk_assert (run->awk, nde->idx != SSE_NULL);

	str = __idxnde_to_str (run, nde->idx, &len);
	if (str == SSE_NULL) return SSE_NULL;

	pair = sse_awk_map_get ((*(sse_awk_val_map_t**)val)->map, str, len);
	SSE_AWK_FREE (run->awk, str);

	return (pair == SSE_NULL)? sse_awk_val_nil: (sse_awk_val_t*)pair->val;
}

static sse_awk_val_t* __eval_namedidx (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	sse_awk_nde_var_t* tgt = (sse_awk_nde_var_t*)nde;
	sse_awk_pair_t* pair;

	pair = sse_awk_map_get (&run->named, tgt->id.name, tgt->id.name_len);
	if (pair == SSE_NULL)
	{
		pair = sse_awk_map_put (&run->named, 
			tgt->id.name, tgt->id.name_len, sse_awk_val_nil);
		if (pair == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);

		sse_awk_refupval (pair->val); 
	}

	return __eval_indexed (run, tgt, (sse_awk_val_t**)&pair->val);
}

static sse_awk_val_t* __eval_globalidx (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	return __eval_indexed (run, (sse_awk_nde_var_t*)nde, 
		(sse_awk_val_t**)&STACK_GLOBAL(run,((sse_awk_nde_var_t*)nde)->id.idxa));
}

static sse_awk_val_t* __eval_localidx (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	return __eval_indexed (run, (sse_awk_nde_var_t*)nde, 
		(sse_awk_val_t**)&STACK_LOCAL(run,((sse_awk_nde_var_t*)nde)->id.idxa));
}

static sse_awk_val_t* __eval_argidx (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	return __eval_indexed (run, (sse_awk_nde_var_t*)nde,
		(sse_awk_val_t**)&STACK_ARG(run,((sse_awk_nde_var_t*)nde)->id.idxa));
}

static sse_awk_val_t* __eval_pos (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	sse_awk_nde_pos_t* pos = (sse_awk_nde_pos_t*)nde;
	sse_awk_val_t* v;
	sse_long_t lv;
	sse_real_t rv;
	int n;

	v = __eval_expression (run, pos->val);
	if (v == SSE_NULL) return SSE_NULL;

	sse_awk_refupval (v);
	n = sse_awk_valtonum (run, v, &lv, &rv);
	sse_awk_refdownval (run, v);

	if (n == -1) PANIC (run, SSE_AWK_EPOSIDX);
	if (n == 1) lv = (sse_long_t)rv;

	if (lv < 0) PANIC (run, SSE_AWK_EPOSIDX);
	if (lv == 0) v = run->inrec.d0;
	else if (lv > 0 && lv <= run->inrec.nflds) 
		v = run->inrec.flds[lv-1].val;
	else v = sse_awk_val_zls; /*sse_awk_val_nil;*/

	return v;
}

static sse_awk_val_t* __eval_getline (sse_awk_run_t* run, sse_awk_nde_t* nde)
{
	sse_awk_nde_getline_t* p;
	sse_awk_val_t* v, * res;
	sse_char_t* in = SSE_NULL;
	const sse_char_t* dst;
	sse_awk_str_t buf;
	int n;

	p = (sse_awk_nde_getline_t*)nde;

	sse_awk_assert (run->awk, (p->in_type == SSE_AWK_IN_PIPE && p->in != SSE_NULL) ||
	           (p->in_type == SSE_AWK_IN_COPROC && p->in != SSE_NULL) ||
		   (p->in_type == SSE_AWK_IN_FILE && p->in != SSE_NULL) ||
	           (p->in_type == SSE_AWK_IN_CONSOLE && p->in == SSE_NULL));

	if (p->in != SSE_NULL)
	{
		sse_size_t len;

		v = __eval_expression (run, p->in);
		if (v == SSE_NULL) return SSE_NULL;

		/* TODO: distinction between v->type == SSE_AWK_VAL_STR 
		 *       and v->type != SSE_AWK_VAL_STR
		 *       if you use the buffer the v directly when
		 *       v->type == SSE_AWK_VAL_STR, sse_awk_refdownval(v)
		 *       should not be called immediately below */
		sse_awk_refupval (v);
		in = sse_awk_valtostr (
			run, v, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &len);
		if (in == SSE_NULL) 
		{
			sse_awk_refdownval (run, v);
			return SSE_NULL;
		}
		sse_awk_refdownval (run, v);

		if (len <= 0) 
		{
			/* the input source name is empty.
			 * make getline return -1 */
			SSE_AWK_FREE (run->awk, in);
			n = -1;
			goto skip_read;
		}

		while (len > 0)
		{
			if (in[--len] == SSE_T('\0'))
			{
				/* the input source name contains a null 
				 * character. make getline return -1 */
				/* TODO: set ERRNO */
				SSE_AWK_FREE (run->awk, in);
				n = -1;
				goto skip_read;
			}
		}
	}

	dst = (in == SSE_NULL)? SSE_T(""): in;

	/* TODO: optimize the line buffer management */
	if (sse_awk_str_open (&buf, DEF_BUF_CAPA, run->awk) == SSE_NULL)
	{
		if (in != SSE_NULL) SSE_AWK_FREE (run->awk, in);
		PANIC (run, SSE_AWK_ENOMEM);
	}

	n = sse_awk_readextio (run, p->in_type, dst, &buf);
	if (in != SSE_NULL) SSE_AWK_FREE (run->awk, in);

	if (n < 0) 
	{
		if (run->errnum != SSE_AWK_EIOHANDLER)
		{
			sse_awk_str_close (&buf);
			return SSE_NULL;
		}

		/* if run->errnum == SSE_AWK_EIOHANDLER, 
		 * make getline return -1 */
		n = -1;
	}

	if (n > 0)
	{
		if (p->var == SSE_NULL)
		{
			/* set $0 with the input value */
			if (sse_awk_setrec (run, 0,
				SSE_AWK_STR_BUF(&buf),
				SSE_AWK_STR_LEN(&buf)) == -1)
			{
				sse_awk_str_close (&buf);
				return SSE_NULL;
			}

			sse_awk_str_close (&buf);
		}
		else
		{
			sse_awk_val_t* v;

			v = sse_awk_makestrval (
				run, SSE_AWK_STR_BUF(&buf), SSE_AWK_STR_LEN(&buf));
			sse_awk_str_close (&buf);
			if (v == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);

			sse_awk_refupval (v);
			if (__do_assignment(run, p->var, v) == SSE_NULL)
			{
				sse_awk_refdownval (run, v);
				return SSE_NULL;
			}
			sse_awk_refdownval (run, v);
		}
	}
	else
	{
		sse_awk_str_close (&buf);
	}
	
skip_read:
	res = sse_awk_makeintval (run, n);
	if (res == SSE_NULL) PANIC (run, SSE_AWK_ENOMEM);

	return res;
}

static int __raw_push (sse_awk_run_t* run, void* val)
{
	if (run->stack_top >= run->stack_limit)
	{
		void** tmp;
		sse_size_t n;
	       
		n = run->stack_limit + STACK_INCREMENT;

		if (run->awk->syscas.realloc != SSE_NULL)
		{
			tmp = (void**) SSE_AWK_REALLOC (
				run->awk, run->stack, n * sse_sizeof(void*)); 
			if (tmp == SSE_NULL) return -1;
		}
		else
		{
			tmp = (void**) SSE_AWK_MALLOC (
				run->awk, n * sse_sizeof(void*));
			if (tmp == SSE_NULL) return -1;
			if (run->stack != SSE_NULL)
			{
				SSE_AWK_MEMCPY (run->awk, tmp, run->stack, 
					run->stack_limit * sse_sizeof(void*)); 
				SSE_AWK_FREE (run->awk, run->stack);
			}
		}
		run->stack = tmp;
		run->stack_limit = n;
	}

	run->stack[run->stack_top++] = val;
	return 0;
}

static void __raw_pop_times (sse_awk_run_t* run, sse_size_t times)
{
	while (times > 0)
	{
		--times;
		__raw_pop (run);
	}
}

static int __read_record (sse_awk_run_t* run)
{
	sse_ssize_t n;

	if (sse_awk_clrrec (run, sse_false) == -1) return -1;

	n = sse_awk_readextio (
		run, SSE_AWK_IN_CONSOLE, SSE_T(""), &run->inrec.line);
	if (n < 0) 
	{
		int errnum = run->errnum;
		sse_awk_clrrec (run, sse_false);
		run->errnum = 
			(errnum == SSE_AWK_EIOHANDLER)? 
			SSE_AWK_ECONINDATA: errnum;
		return -1;
	}
/*
xp_printf (SSE_T("len = %d str=[%s]\n"), 
		(int)SSE_AWK_STR_LEN(&run->inrec.line),
		SSE_AWK_STR_BUF(&run->inrec.line));
*/
	if (n == 0) 
	{
		sse_awk_assert (run->awk, SSE_AWK_STR_LEN(&run->inrec.line) == 0);
		return 0;
	}

	if (sse_awk_setrec (run, 0, 
		SSE_AWK_STR_BUF(&run->inrec.line), 
		SSE_AWK_STR_LEN(&run->inrec.line)) == -1) return -1;

	return 1;
}

static int __shorten_record (sse_awk_run_t* run, sse_size_t nflds)
{
	sse_awk_val_t* v;
	sse_char_t* ofs_free = SSE_NULL, * ofs;
	sse_size_t ofs_len, i;
	sse_awk_str_t tmp;

	sse_awk_assert (run->awk, nflds <= run->inrec.nflds);

	if (nflds > 1)
	{
		v = STACK_GLOBAL(run, SSE_AWK_GLOBAL_OFS);
		sse_awk_refupval (v);

		if (v->type == SSE_AWK_VAL_NIL)
		{
			/* OFS not set */
			ofs = SSE_T(" ");
			ofs_len = 1;
		}
		else if (v->type == SSE_AWK_VAL_STR)
		{
			ofs = ((sse_awk_val_str_t*)v)->buf;
			ofs_len = ((sse_awk_val_str_t*)v)->len;
		}
		else
		{
			ofs = sse_awk_valtostr (
				run, v, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &ofs_len);
			if (ofs == SSE_NULL) return -1;

			ofs_free = ofs;
		}
	}

	if (sse_awk_str_open (&tmp, 
		SSE_AWK_STR_LEN(&run->inrec.line), run->awk) == SSE_NULL)
	{
		run->errnum = SSE_AWK_ENOMEM;
		return -1;
	}

	for (i = 0; i < nflds; i++)
	{
		if (i > 0 && sse_awk_str_ncat (&tmp, ofs, ofs_len) == (sse_size_t)-1)
		{
			if (ofs_free != SSE_NULL) 
				SSE_AWK_FREE (run->awk, ofs_free);
			if (nflds > 1) sse_awk_refdownval (run, v);
			run->errnum = SSE_AWK_ENOMEM;
			return -1;
		}

		if (sse_awk_str_ncat (&tmp, 
			run->inrec.flds[i].ptr, 
			run->inrec.flds[i].len) == (sse_size_t)-1)
		{
			if (ofs_free != SSE_NULL) 
				SSE_AWK_FREE (run->awk, ofs_free);
			if (nflds > 1) sse_awk_refdownval (run, v);
			run->errnum = SSE_AWK_ENOMEM;
			return -1;
		}
	}

	if (ofs_free != SSE_NULL) SSE_AWK_FREE (run->awk, ofs_free);
	if (nflds > 1) sse_awk_refdownval (run, v);

	v = (sse_awk_val_t*) sse_awk_makestrval (
		run, SSE_AWK_STR_BUF(&tmp), SSE_AWK_STR_LEN(&tmp));
	if (v == SSE_NULL)
	{
		run->errnum = SSE_AWK_ENOMEM;
		return -1;
	}

	sse_awk_refdownval (run, run->inrec.d0);
	run->inrec.d0 = v;
	sse_awk_refupval (run->inrec.d0);

	sse_awk_str_swap (&tmp, &run->inrec.line);
	sse_awk_str_close (&tmp);

	for (i = nflds; i < run->inrec.nflds; i++)
	{
		sse_awk_refdownval (run, run->inrec.flds[i].val);
	}

	run->inrec.nflds = nflds;
	return 0;
}

static sse_char_t* __idxnde_to_str (
	sse_awk_run_t* run, sse_awk_nde_t* nde, sse_size_t* len)
{
	sse_char_t* str;
	sse_awk_val_t* idx;

	sse_awk_assert (run->awk, nde != SSE_NULL);

	if (nde->next == SSE_NULL)
	{
		/* single node index */
		idx = __eval_expression (run, nde);
		if (idx == SSE_NULL) return SSE_NULL;

		sse_awk_refupval (idx);

		str = sse_awk_valtostr (
			run, idx, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, len);
		if (str == SSE_NULL) 
		{
			sse_awk_refdownval (run, idx);
			return SSE_NULL;
		}

		sse_awk_refdownval (run, idx);
	}
	else
	{
		/* multidimensional index */
		sse_awk_str_t idxstr;

		if (sse_awk_str_open (
			&idxstr, DEF_BUF_CAPA, run->awk) == SSE_NULL) 
		{
			PANIC (run, SSE_AWK_ENOMEM);
		}

		while (nde != SSE_NULL)
		{
			idx = __eval_expression (run, nde);
			if (idx == SSE_NULL) 
			{
				sse_awk_str_close (&idxstr);
				return SSE_NULL;
			}

			sse_awk_refupval (idx);

			if (SSE_AWK_STR_LEN(&idxstr) > 0 &&
			    sse_awk_str_ncat (&idxstr, 
			    	run->global.subsep.ptr, 
			    	run->global.subsep.len) == (sse_size_t)-1)
			{
				sse_awk_refdownval (run, idx);
				sse_awk_str_close (&idxstr);
				PANIC (run, SSE_AWK_ENOMEM);
			}

			if (sse_awk_valtostr (
				run, idx, 0, &idxstr, SSE_NULL) == SSE_NULL)
			{
				sse_awk_refdownval (run, idx);
				sse_awk_str_close (&idxstr);
				return SSE_NULL;
			}

			sse_awk_refdownval (run, idx);
			nde = nde->next;
		}

		str = SSE_AWK_STR_BUF(&idxstr);
		*len = SSE_AWK_STR_LEN(&idxstr);
		sse_awk_str_forfeit (&idxstr);
	}

	return str;
}
