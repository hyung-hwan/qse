/*
 * $Id: run.c,v 1.244 2006-10-26 09:27:15 bacon Exp $
 */

#include <ase/awk/awk_i.h>

/* TODO: remove this dependency...*/
#include <math.h>

#define CMP_ERROR -99
#define DEF_BUF_CAPA 256
#define STACK_INCREMENT 512

#define STACK_AT(run,n) ((run)->stack[(run)->stack_base+(n)])
#define STACK_NARGS(run) (STACK_AT(run,3))
#define STACK_ARG(run,n) STACK_AT(run,3+1+(n))
#define STACK_LOCAL(run,n) STACK_AT(run,3+(ase_size_t)STACK_NARGS(run)+1+(n))
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
	do { (run)->errnum = (code); return ASE_NULL; } while (0)

#define PANIC_I(run,code) \
	do { (run)->errnum = (code); return -1; } while (0)

#define DEFAULT_CONVFMT ASE_T("%.6g")
#define DEFAULT_OFMT ASE_T("%.6g")
#define DEFAULT_OFS ASE_T(" ")
#define DEFAULT_ORS ASE_T("\n")
#define DEFAULT_SUBSEP ASE_T("\034")

/* the index of a positional variable should be a positive interger
 * equal to or less than the maximum value of the type by which
 * the index is represented. but it has an extra check against the
 * maximum value of ase_size_t as the reference is represented
 * in a pointer variable of ase_awk_val_ref_t and sizeof(void*) is
 * equal to sizeof(ase_size_t). */
#define IS_VALID_POSIDX(idx) \
	((idx) >= 0 && \
	 (idx) < ASE_TYPE_MAX(ase_long_t) && \
	 (idx) < ASE_TYPE_MAX(ase_size_t))

static void __add_run (ase_awk_t* awk, ase_awk_run_t* run);
static void __del_run (ase_awk_t* awk, ase_awk_run_t* run);

static int __init_run (
	ase_awk_run_t* run, ase_awk_runios_t* runios, int* errnum);
static void __deinit_run (ase_awk_run_t* run);

static int __build_runarg (ase_awk_run_t* run, ase_awk_runarg_t* runarg);
static int __set_globals_to_default (ase_awk_run_t* run);

static int __run_main (ase_awk_run_t* run, ase_awk_runarg_t* runarg);
static int __run_pattern_blocks  (ase_awk_run_t* run);
static int __run_pattern_block_chain (
	ase_awk_run_t* run, ase_awk_chain_t* chain);
static int __run_pattern_block (
	ase_awk_run_t* run, ase_awk_chain_t* chain, ase_size_t block_no);
static int __run_block (ase_awk_run_t* run, ase_awk_nde_blk_t* nde);
static int __run_statement (ase_awk_run_t* run, ase_awk_nde_t* nde);
static int __run_if (ase_awk_run_t* run, ase_awk_nde_if_t* nde);
static int __run_while (ase_awk_run_t* run, ase_awk_nde_while_t* nde);
static int __run_for (ase_awk_run_t* run, ase_awk_nde_for_t* nde);
static int __run_foreach (ase_awk_run_t* run, ase_awk_nde_foreach_t* nde);
static int __run_break (ase_awk_run_t* run, ase_awk_nde_break_t* nde);
static int __run_continue (ase_awk_run_t* run, ase_awk_nde_continue_t* nde);
static int __run_return (ase_awk_run_t* run, ase_awk_nde_return_t* nde);
static int __run_exit (ase_awk_run_t* run, ase_awk_nde_exit_t* nde);
static int __run_next (ase_awk_run_t* run, ase_awk_nde_next_t* nde);
static int __run_nextfile (ase_awk_run_t* run, ase_awk_nde_nextfile_t* nde);
static int __run_delete (ase_awk_run_t* run, ase_awk_nde_delete_t* nde);
static int __run_print (ase_awk_run_t* run, ase_awk_nde_print_t* nde);

static ase_awk_val_t* __eval_expression (ase_awk_run_t* run, ase_awk_nde_t* nde);
static ase_awk_val_t* __eval_expression0 (ase_awk_run_t* run, ase_awk_nde_t* nde);

static ase_awk_val_t* __eval_group (ase_awk_run_t* run, ase_awk_nde_t* nde);

static ase_awk_val_t* __eval_assignment (
	ase_awk_run_t* run, ase_awk_nde_t* nde);
static ase_awk_val_t* __do_assignment (
	ase_awk_run_t* run, ase_awk_nde_t* var, ase_awk_val_t* val);
static ase_awk_val_t* __do_assignment_scalar (
	ase_awk_run_t* run, ase_awk_nde_var_t* var, ase_awk_val_t* val);
static ase_awk_val_t* __do_assignment_map (
	ase_awk_run_t* run, ase_awk_nde_var_t* var, ase_awk_val_t* val);
static ase_awk_val_t* __do_assignment_pos (
	ase_awk_run_t* run, ase_awk_nde_pos_t* pos, ase_awk_val_t* val);

static ase_awk_val_t* __eval_binary (
	ase_awk_run_t* run, ase_awk_nde_t* nde);
static ase_awk_val_t* __eval_binop_lor (
	ase_awk_run_t* run, ase_awk_nde_t* left, ase_awk_nde_t* right);
static ase_awk_val_t* __eval_binop_land (
	ase_awk_run_t* run, ase_awk_nde_t* left, ase_awk_nde_t* right);
static ase_awk_val_t* __eval_binop_in (
	ase_awk_run_t* run, ase_awk_nde_t* left, ase_awk_nde_t* right);
static ase_awk_val_t* __eval_binop_bor (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right);
static ase_awk_val_t* __eval_binop_bxor (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right);
static ase_awk_val_t* __eval_binop_band (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right);
static ase_awk_val_t* __eval_binop_eq (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right);
static ase_awk_val_t* __eval_binop_ne (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right);
static ase_awk_val_t* __eval_binop_gt (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right);
static ase_awk_val_t* __eval_binop_ge (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right);
static ase_awk_val_t* __eval_binop_lt (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right);
static ase_awk_val_t* __eval_binop_le (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right);
static ase_awk_val_t* __eval_binop_lshift (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right);
static ase_awk_val_t* __eval_binop_rshift (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right);
static ase_awk_val_t* __eval_binop_plus (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right);
static ase_awk_val_t* __eval_binop_minus (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right);
static ase_awk_val_t* __eval_binop_mul (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right);
static ase_awk_val_t* __eval_binop_div (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right);
static ase_awk_val_t* __eval_binop_mod (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right);
static ase_awk_val_t* __eval_binop_exp (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right);
static ase_awk_val_t* __eval_binop_concat (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right);
static ase_awk_val_t* __eval_binop_ma (
	ase_awk_run_t* run, ase_awk_nde_t* left, ase_awk_nde_t* right);
static ase_awk_val_t* __eval_binop_nm (
	ase_awk_run_t* run, ase_awk_nde_t* left, ase_awk_nde_t* right);
static ase_awk_val_t* __eval_binop_match0 (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right, int ret);

static ase_awk_val_t* __eval_unary (ase_awk_run_t* run, ase_awk_nde_t* nde);
static ase_awk_val_t* __eval_incpre (ase_awk_run_t* run, ase_awk_nde_t* nde);
static ase_awk_val_t* __eval_incpst (ase_awk_run_t* run, ase_awk_nde_t* nde);
static ase_awk_val_t* __eval_cnd (ase_awk_run_t* run, ase_awk_nde_t* nde);

static ase_awk_val_t* __eval_bfn (ase_awk_run_t* run, ase_awk_nde_t* nde);
static ase_awk_val_t* __eval_afn (ase_awk_run_t* run, ase_awk_nde_t* nde);
static ase_awk_val_t* __eval_call (
	ase_awk_run_t* run, ase_awk_nde_t* nde, 
	const ase_char_t* bfn_arg_spec, ase_awk_afn_t* afn);

static int __get_reference (
	ase_awk_run_t* run, ase_awk_nde_t* nde, ase_awk_val_t*** ref);
static ase_awk_val_t** __get_reference_indexed (
	ase_awk_run_t* run, ase_awk_nde_var_t* nde, ase_awk_val_t** val);

static ase_awk_val_t* __eval_int (ase_awk_run_t* run, ase_awk_nde_t* nde);
static ase_awk_val_t* __eval_real (ase_awk_run_t* run, ase_awk_nde_t* nde);
static ase_awk_val_t* __eval_str (ase_awk_run_t* run, ase_awk_nde_t* nde);
static ase_awk_val_t* __eval_rex (ase_awk_run_t* run, ase_awk_nde_t* nde);
static ase_awk_val_t* __eval_named (ase_awk_run_t* run, ase_awk_nde_t* nde);
static ase_awk_val_t* __eval_global (ase_awk_run_t* run, ase_awk_nde_t* nde);
static ase_awk_val_t* __eval_local (ase_awk_run_t* run, ase_awk_nde_t* nde);
static ase_awk_val_t* __eval_arg (ase_awk_run_t* run, ase_awk_nde_t* nde);
static ase_awk_val_t* __eval_namedidx (ase_awk_run_t* run, ase_awk_nde_t* nde);
static ase_awk_val_t* __eval_globalidx (ase_awk_run_t* run, ase_awk_nde_t* nde);
static ase_awk_val_t* __eval_localidx (ase_awk_run_t* run, ase_awk_nde_t* nde);
static ase_awk_val_t* __eval_argidx (ase_awk_run_t* run, ase_awk_nde_t* nde);
static ase_awk_val_t* __eval_pos (ase_awk_run_t* run, ase_awk_nde_t* nde);
static ase_awk_val_t* __eval_getline (ase_awk_run_t* run, ase_awk_nde_t* nde);

static int __raw_push (ase_awk_run_t* run, void* val);
#define __raw_pop(run) \
	do { \
		ASE_AWK_ASSERT (run->awk, (run)->stack_top > (run)->stack_base); \
		(run)->stack_top--; \
	} while (0)
static void __raw_pop_times (ase_awk_run_t* run, ase_size_t times);

static int __read_record (ase_awk_run_t* run);
static int __shorten_record (ase_awk_run_t* run, ase_size_t nflds);

static ase_char_t* __idxnde_to_str (
	ase_awk_run_t* run, ase_awk_nde_t* nde, ase_size_t* len);

typedef ase_awk_val_t* (*binop_func_t) (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right);
typedef ase_awk_val_t* (*eval_expr_t) (ase_awk_run_t* run, ase_awk_nde_t* nde);

/* TODO: remove this function */
static int __printval (ase_awk_pair_t* pair, void* arg)
{
	xp_printf (ASE_T("%s = "), (const ase_char_t*)pair->key);
	ase_awk_printval ((ase_awk_val_t*)pair->val);
	xp_printf (ASE_T("\n"));
	return 0;
}

ase_size_t ase_awk_getnargs (ase_awk_run_t* run)
{
	return (ase_size_t) STACK_NARGS (run);
}

ase_awk_val_t* ase_awk_getarg (ase_awk_run_t* run, ase_size_t idx)
{
	return STACK_ARG (run, idx);
}

ase_awk_val_t* ase_awk_getglobal (ase_awk_run_t* run, ase_size_t idx)
{
	return STACK_GLOBAL (run, idx);
}

int ase_awk_setglobal (ase_awk_run_t* run, ase_size_t idx, ase_awk_val_t* val)
{
	ase_awk_val_t* old = STACK_GLOBAL (run, idx);
	if (old->type == ASE_AWK_VAL_MAP)
	{	
		/* once a variable becomes an array,
		 * it cannot be changed to a scalar variable */
		PANIC_I (run, ASE_AWK_EMAPTOSCALAR);
	}

/* TODO: is this correct?? */
	if (val->type == ASE_AWK_VAL_MAP &&
	    (idx >= ASE_AWK_GLOBAL_ARGC && idx <= ASE_AWK_GLOBAL_SUBSEP) &&
	    idx != ASE_AWK_GLOBAL_ARGV)
	{
		/* TODO: better error code */
		PANIC_I (run, ASE_AWK_ESCALARTOMAP);
	}

	if (idx == ASE_AWK_GLOBAL_CONVFMT)
	{
		ase_char_t* convfmt_ptr;
		ase_size_t convfmt_len;

		convfmt_ptr = ase_awk_valtostr (
			run, val, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &convfmt_len);
		if (convfmt_ptr == ASE_NULL) return  -1;

		if (run->global.convfmt.ptr != ASE_NULL)
			ASE_AWK_FREE (run->awk, run->global.convfmt.ptr);
		run->global.convfmt.ptr = convfmt_ptr;
		run->global.convfmt.len = convfmt_len;
	}
	else if (idx == ASE_AWK_GLOBAL_FS)
	{
		ase_char_t* fs_ptr;
		ase_size_t fs_len;

		if (val->type == ASE_AWK_VAL_STR)
		{
			fs_ptr = ((ase_awk_val_str_t*)val)->buf;
			fs_len = ((ase_awk_val_str_t*)val)->len;
		}
		else
		{
			/* due to the expression evaluation rule, the 
			 * regular expression can not be an assigned value */
			ASE_AWK_ASSERT (run->awk, val->type != ASE_AWK_VAL_REX);

			fs_ptr = ase_awk_valtostr (
				run, val, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &fs_len);
			if (fs_ptr == ASE_NULL) return -1;
		}

		if (fs_len > 1)
		{
			void* rex;

			/* compile the regular expression */
			/* TODO: use safebuild */
			rex = ase_awk_buildrex (
				run->awk, fs_ptr, fs_len, &run->errnum);
			if (rex == ASE_NULL)
			{
				if (val->type != ASE_AWK_VAL_STR) 
					ASE_AWK_FREE (run->awk, fs_ptr);
				return -1;
			}

			if (run->global.fs != ASE_NULL) 
			{
				ase_awk_freerex (run->awk, run->global.fs);
			}
			run->global.fs = rex;
		}

		if (val->type != ASE_AWK_VAL_STR) ASE_AWK_FREE (run->awk, fs_ptr);
	}
	else if (idx == ASE_AWK_GLOBAL_IGNORECASE)
	{
		if ((val->type == ASE_AWK_VAL_INT &&
		     ((ase_awk_val_int_t*)val)->val == 0) ||
		    (val->type == ASE_AWK_VAL_REAL &&
		     ((ase_awk_val_real_t*)val)->val == 0.0) ||
		    (val->type == ASE_AWK_VAL_STR &&
		     ((ase_awk_val_str_t*)val)->len == 0))
		{
			run->global.ignorecase = 0;
		}
		else
		{
			run->global.ignorecase = 1;
		}
	}
	else if (idx == ASE_AWK_GLOBAL_NF)
	{
		int n;
		ase_long_t lv;
		ase_real_t rv;

		n = ase_awk_valtonum (run, val, &lv, &rv);
		if (n == -1) return -1;
		if (n == 1) lv = (ase_long_t)rv;

		if (lv < run->inrec.nflds)
		{
			if (__shorten_record (run, (ase_size_t)lv) == -1) return -1;
		}
	}
	else if (idx == ASE_AWK_GLOBAL_OFMT)
	{
		ase_char_t* ofmt_ptr;
		ase_size_t ofmt_len;

		ofmt_ptr = ase_awk_valtostr (
			run, val, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &ofmt_len);
		if (ofmt_ptr == ASE_NULL) return  -1;

		if (run->global.ofmt.ptr != ASE_NULL)
			ASE_AWK_FREE (run->awk, run->global.ofmt.ptr);
		run->global.ofmt.ptr = ofmt_ptr;
		run->global.ofmt.len = ofmt_len;
	}
	else if (idx == ASE_AWK_GLOBAL_OFS)
	{	
		ase_char_t* ofs_ptr;
		ase_size_t ofs_len;

		ofs_ptr = ase_awk_valtostr (
			run, val, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &ofs_len);
		if (ofs_ptr == ASE_NULL) return  -1;

		if (run->global.ofs.ptr != ASE_NULL)
			ASE_AWK_FREE (run->awk, run->global.ofs.ptr);
		run->global.ofs.ptr = ofs_ptr;
		run->global.ofs.len = ofs_len;
	}
	else if (idx == ASE_AWK_GLOBAL_ORS)
	{	
		ase_char_t* ors_ptr;
		ase_size_t ors_len;

		ors_ptr = ase_awk_valtostr (
			run, val, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &ors_len);
		if (ors_ptr == ASE_NULL) return  -1;

		if (run->global.ors.ptr != ASE_NULL)
			ASE_AWK_FREE (run->awk, run->global.ors.ptr);
		run->global.ors.ptr = ors_ptr;
		run->global.ors.len = ors_len;
	}
	else if (idx == ASE_AWK_GLOBAL_RS)
	{
		ase_char_t* rs_ptr;
		ase_size_t rs_len;

		if (val->type == ASE_AWK_VAL_STR)
		{
			rs_ptr = ((ase_awk_val_str_t*)val)->buf;
			rs_len = ((ase_awk_val_str_t*)val)->len;
		}
		else
		{
			/* due to the expression evaluation rule, the 
			 * regular expression can not be an assigned value */
			ASE_AWK_ASSERT (run->awk, val->type != ASE_AWK_VAL_REX);

			rs_ptr = ase_awk_valtostr (
				run, val, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &rs_len);
			if (rs_ptr == ASE_NULL) return -1;
		}

		if (rs_len > 1)
		{
			void* rex;

			/* compile the regular expression */
			/* TODO: use safebuild */
			rex = ase_awk_buildrex (
				run->awk, rs_ptr, rs_len, &run->errnum);
			if (rex == ASE_NULL)
			{
				if (val->type != ASE_AWK_VAL_STR) 
					ASE_AWK_FREE (run->awk, rs_ptr);
				return -1;
			}

			if (run->global.rs != ASE_NULL) 
			{
				ase_awk_freerex (run->awk, run->global.rs);
			}
			run->global.rs = rex;
		}

		if (val->type != ASE_AWK_VAL_STR) ASE_AWK_FREE (run->awk, rs_ptr);
	}
	else if (idx == ASE_AWK_GLOBAL_SUBSEP)
	{
		ase_char_t* subsep_ptr;
		ase_size_t subsep_len;

		subsep_ptr = ase_awk_valtostr (
			run, val, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &subsep_len);
		if (subsep_ptr == ASE_NULL) return  -1;

		if (run->global.subsep.ptr != ASE_NULL)
			ASE_AWK_FREE (run->awk, run->global.subsep.ptr);
		run->global.subsep.ptr = subsep_ptr;
		run->global.subsep.len = subsep_len;
	}

	ase_awk_refdownval (run, old);
	STACK_GLOBAL(run,idx) = val;
	ase_awk_refupval (val);

	return 0;
}

void ase_awk_setretval (ase_awk_run_t* run, ase_awk_val_t* val)
{
	ase_awk_refdownval (run, STACK_RETVAL(run));
	STACK_RETVAL(run) = val;
	/* should use the same trick as __run_return_statement */
	ase_awk_refupval (val); 
}

int ase_awk_setconsolename (
	ase_awk_run_t* run, const ase_char_t* name, ase_size_t len)
{
	ase_awk_val_t* tmp;
	int n;

	if (len == 0) tmp = ase_awk_val_zls;
	else
	{
		tmp = ase_awk_makestrval (run, name, len);
		if (tmp == ASE_NULL)
		{
			run->errnum = ASE_AWK_ENOMEM;
			return -1;
		}
	}

	ase_awk_refupval (tmp);
	n = ase_awk_setglobal (run, ASE_AWK_GLOBAL_FILENAME, tmp);
	ase_awk_refdownval (run, tmp);

	return n;
}

int ase_awk_getrunerrnum (ase_awk_run_t* run)
{
	return run->errnum;
}

void ase_awk_setrunerrnum (ase_awk_run_t* run, int errnum)
{
	run->errnum = errnum;
}

int ase_awk_run (ase_awk_t* awk, 
	ase_awk_runios_t* runios, 
	ase_awk_runcbs_t* runcbs, 
	ase_awk_runarg_t* runarg)
{
	ase_awk_run_t* run;
	int n, errnum;

	awk->errnum = ASE_AWK_ENOERR;

	run = (ase_awk_run_t*) ASE_AWK_MALLOC (awk, ase_sizeof(ase_awk_run_t));
	if (run == ASE_NULL)
	{
		awk->errnum = ASE_AWK_ENOMEM;
		return -1;
	}

	ASE_AWK_MEMSET (awk, run, 0, ase_sizeof(ase_awk_run_t));

	__add_run (awk, run);

	if (__init_run (run, runios, &errnum) == -1) 
	{
		awk->errnum = errnum;
		__del_run (awk, run);
		ASE_AWK_FREE (awk, run);
		return -1;
	}

	if (runcbs != ASE_NULL && runcbs->on_start != ASE_NULL) 
	{
		runcbs->on_start (awk, run, runcbs->custom_data);
	}

	n = __run_main (run, runarg);
	if (n == -1) 
	{
		/* if no callback is specified, awk's error number 
		 * is updated with the run's error number */
		awk->errnum = (runcbs == ASE_NULL)? run->errnum: ASE_AWK_ERUNTIME;
	}

	if (runcbs != ASE_NULL && runcbs->on_end != ASE_NULL) 
	{
		runcbs->on_end (awk, run, 
			((n == -1)? run->errnum: ASE_AWK_ENOERR), 
			runcbs->custom_data);

		/* when using callbacks, the function always returns 0 after
		 * the start callbacks has been triggered */
		n = 0;
	}

	__deinit_run (run);
	__del_run (awk, run);
	ASE_AWK_FREE (awk, run);
	return n;
}

int ase_awk_stop (ase_awk_t* awk, ase_awk_run_t* run)
{
	ase_awk_run_t* r;
	int n = 0;

	ASE_AWK_LOCK (awk);

	/* check if the run handle given is valid */
	for (r = awk->run.ptr; r != ASE_NULL; r = r->next)
	{
		if (r == run)
		{
			ASE_AWK_ASSERT (run->awk, r->awk == awk);
			r->exit_level = EXIT_ABORT;
			break;
		}
	}

	if (r == ASE_NULL)
	{
		/* if it is not found in the awk's run list, 
		 * it is not a valid handle */
		awk->errnum = ASE_AWK_EINVAL;
		n = -1;
	}

	ASE_AWK_UNLOCK (awk);

	return n;
}

void ase_awk_stopall (ase_awk_t* awk)
{
	ase_awk_run_t* r;

	ASE_AWK_LOCK (awk);

	for (r = awk->run.ptr; r != ASE_NULL; r = r->next)
	{
		r->exit_level = EXIT_ABORT;
	}

	ASE_AWK_UNLOCK (awk);
}

static void __free_namedval (void* run, void* val)
{
	ase_awk_refdownval ((ase_awk_run_t*)run, val);
}

static void __add_run (ase_awk_t* awk, ase_awk_run_t* run)
{
	ASE_AWK_LOCK (awk);

	run->awk = awk;
	run->prev = ASE_NULL;
	run->next = awk->run.ptr;
	if (run->next != ASE_NULL) run->next->prev = run;
	awk->run.ptr = run;
	awk->run.count++;

	ASE_AWK_UNLOCK (awk);
}

static void __del_run (ase_awk_t* awk, ase_awk_run_t* run)
{
	ASE_AWK_LOCK (awk);

	ASE_AWK_ASSERT (run->awk, awk->run.ptr != ASE_NULL);

	if (run->prev == ASE_NULL)
	{
		awk->run.ptr = run->next;
		if (run->next != ASE_NULL) run->next->prev = ASE_NULL;
	}
	else
	{
		run->prev->next = run->next;
		if (run->next != ASE_NULL) run->next->prev = run->prev;
	}

	run->awk = ASE_NULL;
	awk->run.count--;

	ASE_AWK_UNLOCK (awk);
}

static int __init_run (ase_awk_run_t* run, ase_awk_runios_t* runios, int* errnum)
{
	run->stack = ASE_NULL;
	run->stack_top = 0;
	run->stack_base = 0;
	run->stack_limit = 0;

	run->exit_level = EXIT_NONE;

	run->icache_count = 0;
	run->rcache_count = 0;
	run->fcache_count = 0;

	run->errnum = ASE_AWK_ENOERR;

	run->inrec.buf_pos = 0;
	run->inrec.buf_len = 0;
	run->inrec.flds = ASE_NULL;
	run->inrec.nflds = 0;
	run->inrec.maxflds = 0;
	run->inrec.d0 = ase_awk_val_nil;
	if (ase_awk_str_open (
		&run->inrec.line, DEF_BUF_CAPA, run->awk) == ASE_NULL)
	{
		*errnum = ASE_AWK_ENOMEM; 
		return -1;
	}

	if (ase_awk_map_open (&run->named, 
		run, DEF_BUF_CAPA, __free_namedval, run->awk) == ASE_NULL) 
	{
		ase_awk_str_close (&run->inrec.line);
		*errnum = ASE_AWK_ENOMEM; 
		return -1;
	}

	run->pattern_range_state = (ase_byte_t*) ASE_AWK_MALLOC (
		run->awk, run->awk->tree.chain_size * ase_sizeof(ase_byte_t));
	if (run->pattern_range_state == ASE_NULL)
	{
		ase_awk_map_close (&run->named);
		ase_awk_str_close (&run->inrec.line);
		*errnum = ASE_AWK_ENOMEM; 
		return -1;
	}

	ASE_AWK_MEMSET (run->awk, run->pattern_range_state, 0, 
		run->awk->tree.chain_size * ase_sizeof(ase_byte_t));

	run->extio.handler[ASE_AWK_EXTIO_PIPE] = runios->pipe;
	run->extio.handler[ASE_AWK_EXTIO_COPROC] = runios->coproc;
	run->extio.handler[ASE_AWK_EXTIO_FILE] = runios->file;
	run->extio.handler[ASE_AWK_EXTIO_CONSOLE] = runios->console;
	run->extio.custom_data = runios->custom_data;
	run->extio.chain = ASE_NULL;

	run->global.rs = ASE_NULL;
	run->global.fs = ASE_NULL;
	run->global.ignorecase = 0;

	return 0;
}

static void __deinit_run (ase_awk_run_t* run)
{
	ASE_AWK_FREE (run->awk, run->pattern_range_state);

	/* close all pending eio's */
	/* TODO: what if this operation fails? */
	ase_awk_clearextio (run);
	ASE_AWK_ASSERT (run->awk, run->extio.chain == ASE_NULL);

	if (run->global.rs != ASE_NULL) 
	{
		ASE_AWK_FREE (run->awk, run->global.rs);
		run->global.rs = ASE_NULL;
	}
	if (run->global.fs != ASE_NULL)
	{
		ASE_AWK_FREE (run->awk, run->global.fs);
		run->global.fs = ASE_NULL;
	}

	if (run->global.convfmt.ptr != ASE_NULL &&
	    run->global.convfmt.ptr != DEFAULT_CONVFMT)
	{
		ASE_AWK_FREE (run->awk, run->global.convfmt.ptr);
		run->global.convfmt.ptr = ASE_NULL;
		run->global.convfmt.len = 0;
	}

	if (run->global.ofmt.ptr != ASE_NULL && 
	    run->global.ofmt.ptr != DEFAULT_OFMT)
	{
		ASE_AWK_FREE (run->awk, run->global.ofmt.ptr);
		run->global.ofmt.ptr = ASE_NULL;
		run->global.ofmt.len = 0;
	}

	if (run->global.ofs.ptr != ASE_NULL && 
	    run->global.ofs.ptr != DEFAULT_OFS)
	{
		ASE_AWK_FREE (run->awk, run->global.ofs.ptr);
		run->global.ofs.ptr = ASE_NULL;
		run->global.ofs.len = 0;
	}

	if (run->global.ors.ptr != ASE_NULL && 
	    run->global.ors.ptr != DEFAULT_ORS)
	{
		ASE_AWK_FREE (run->awk, run->global.ors.ptr);
		run->global.ors.ptr = ASE_NULL;
		run->global.ors.len = 0;
	}

	if (run->global.subsep.ptr != ASE_NULL && 
	    run->global.subsep.ptr != DEFAULT_SUBSEP)
	{
		ASE_AWK_FREE (run->awk, run->global.subsep.ptr);
		run->global.subsep.ptr = ASE_NULL;
		run->global.subsep.len = 0;
	}

	/* destroy input record. ase_awk_clrrec should be called
	 * before the run stack has been destroyed because it may try
	 * to change the value to ASE_AWK_GLOBAL_NF. */
	ase_awk_clrrec (run, ase_false);  
	if (run->inrec.flds != ASE_NULL) 
	{
		ASE_AWK_FREE (run->awk, run->inrec.flds);
		run->inrec.flds = ASE_NULL;
		run->inrec.maxflds = 0;
	}
	ase_awk_str_close (&run->inrec.line);

	/* destroy run stack */
	if (run->stack != ASE_NULL)
	{
		ASE_AWK_ASSERT (run->awk, run->stack_top == 0);

		ASE_AWK_FREE (run->awk, run->stack);
		run->stack = ASE_NULL;
		run->stack_top = 0;
		run->stack_base = 0;
		run->stack_limit = 0;
	}

	/* destroy named variables */
	ase_awk_map_close (&run->named);

	/* destroy values in free list */
	while (run->icache_count > 0)
	{
		ase_awk_val_int_t* tmp = run->icache[--run->icache_count];
		ase_awk_freeval (run, (ase_awk_val_t*)tmp, ase_false);
	}

	while (run->rcache_count > 0)
	{
		ase_awk_val_real_t* tmp = run->rcache[--run->rcache_count];
		ase_awk_freeval (run, (ase_awk_val_t*)tmp, ase_false);
	}

	while (run->fcache_count > 0)
	{
		ase_awk_val_ref_t* tmp = run->fcache[--run->fcache_count];
		ase_awk_freeval (run, (ase_awk_val_t*)tmp, ase_false);
	}
}

static int __build_runarg (ase_awk_run_t* run, ase_awk_runarg_t* runarg)
{
	ase_awk_runarg_t* p = runarg;
	ase_size_t argc;
	ase_awk_val_t* v_argc;
	ase_awk_val_t* v_argv;
	ase_awk_val_t* v_tmp;
	ase_char_t key[ase_sizeof(ase_long_t)*8+2];
	ase_size_t key_len;

	v_argv = ase_awk_makemapval (run);
	if (v_argv == ASE_NULL)
	{
		run->errnum = ASE_AWK_ENOMEM;
		return -1;
	}
	ase_awk_refupval (v_argv);

	if (runarg == ASE_NULL) argc = 0;
	else
	{
		for (argc = 0, p = runarg; p->ptr != ASE_NULL; argc++, p++)
		{
			v_tmp = ase_awk_makestrval (run, p->ptr, p->len);
			if (v_tmp == ASE_NULL)
			{
				ase_awk_refdownval (run, v_argv);
				run->errnum = ASE_AWK_ENOMEM;
				return -1;
			}

			key_len = ase_awk_longtostr (
				argc, 10, ASE_NULL, key, ase_countof(key));
			ASE_AWK_ASSERT (run->awk, key_len != (ase_size_t)-1);

			/* increment reference count of v_tmp in advance as if 
			 * it has successfully been assigned into ARGV. */
			ase_awk_refupval (v_tmp);

			if (ase_awk_map_putx (
				((ase_awk_val_map_t*)v_argv)->map,
				key, key_len, v_tmp, ASE_NULL) == -1)
			{
				/* if the assignment operation fails, decrements
				 * the reference of v_tmp to free it */
				ase_awk_refdownval (run, v_tmp);

				/* the values previously assigned into the
				 * map will be freeed when v_argv is freed */
				ase_awk_refdownval (run, v_argv);

				run->errnum = ASE_AWK_ENOMEM;
				return -1;
			}
		}
	}

	v_argc = ase_awk_makeintval (run, (ase_long_t)argc);
	if (v_argc == ASE_NULL)
	{
		ase_awk_refdownval (run, v_argv);
		run->errnum = ASE_AWK_ENOMEM;
		return -1;
	}

	ase_awk_refupval (v_argc);

	ASE_AWK_ASSERT (run->awk, 
		STACK_GLOBAL(run,ASE_AWK_GLOBAL_ARGC) == ase_awk_val_nil);

	if (ase_awk_setglobal (run, ASE_AWK_GLOBAL_ARGC, v_argc) == -1) 
	{
		ase_awk_refdownval (run, v_argc);
		ase_awk_refdownval (run, v_argv);
		return -1;
	}

	if (ase_awk_setglobal (run, ASE_AWK_GLOBAL_ARGV, v_argv) == -1)
	{
		/* ARGC is assigned nil when ARGV assignment has failed.
		 * However, this requires preconditions, as follows:
		 *  1. __build_runarg should be called in a proper place
		 *     as it is not a generic-purpose routine.
		 *  2. ARGC should be nil before __build_runarg is called 
		 * If the restoration fails, nothing can salvage it. */
		ase_awk_setglobal (run, ASE_AWK_GLOBAL_ARGC, ase_awk_val_nil);
		ase_awk_refdownval (run, v_argc);
		ase_awk_refdownval (run, v_argv);
		return -1;
	}

	ase_awk_refdownval (run, v_argc);
	ase_awk_refdownval (run, v_argv);
	return 0;
}

static int __update_fnr (ase_awk_run_t* run, ase_size_t fnr)
{
	ase_awk_val_t* tmp;

	tmp = ase_awk_makeintval (run, fnr);
	if (tmp == ASE_NULL)
	{
		run->errnum = ASE_AWK_ENOMEM;
		return -1;
	}

	ase_awk_refupval (tmp);
	if (ase_awk_setglobal (run, ASE_AWK_GLOBAL_FNR, tmp) == -1)
	{
		ase_awk_refdownval (run, tmp);
		return -1;
	}

	ase_awk_refdownval (run, tmp);
	run->global.fnr = fnr;
	return 0;
}

static int __set_globals_to_default (ase_awk_run_t* run)
{
	struct __gtab_t
	{
		int idx;
		const ase_char_t* str;
	};
       
	static struct __gtab_t gtab[] =
	{
		{ ASE_AWK_GLOBAL_CONVFMT,  DEFAULT_CONVFMT },
		{ ASE_AWK_GLOBAL_FILENAME, ASE_NULL },
		{ ASE_AWK_GLOBAL_OFMT,     DEFAULT_OFMT },
		{ ASE_AWK_GLOBAL_OFS,      DEFAULT_OFS },
		{ ASE_AWK_GLOBAL_ORS,      DEFAULT_ORS },
		{ ASE_AWK_GLOBAL_SUBSEP,   DEFAULT_SUBSEP },
	};

	ase_awk_val_t* tmp;
	ase_size_t i, j;

	for (i = 0; i < ase_countof(gtab); i++)
	{
		if (gtab[i].str == ASE_NULL || gtab[i].str[0] == ASE_T('\0'))
		{
			tmp = ase_awk_val_zls;
		}
		else 
		{
			tmp = ase_awk_makestrval0 (run, gtab[i].str);
			if (tmp == ASE_NULL)
			{
				run->errnum = ASE_AWK_ENOMEM;
				return -1;
			}
		}
		
		ase_awk_refupval (tmp);

		ASE_AWK_ASSERT (run->awk, 
			STACK_GLOBAL(run,gtab[i].idx) == ase_awk_val_nil);

		if (ase_awk_setglobal (run, gtab[i].idx, tmp) == -1)
		{
			for (j = 0; j < i; j++)
			{
				ase_awk_setglobal (
					run, gtab[i].idx, ase_awk_val_nil);
			}

			ase_awk_refdownval (run, tmp);
			return -1;
		}

		ase_awk_refdownval (run, tmp);
	}

	return 0;
}

static int __run_main (ase_awk_run_t* run, ase_awk_runarg_t* runarg)
{
	ase_size_t nglobals, nargs, i;
	ase_size_t saved_stack_top;
	ase_awk_val_t* v;
	int n;

	ASE_AWK_ASSERT (run->awk, run->stack_base == 0 && run->stack_top == 0);

	/* secure space for global variables */
	saved_stack_top = run->stack_top;

	nglobals = run->awk->tree.nglobals;

	while (nglobals > 0)
	{
		--nglobals;
		if (__raw_push(run,ase_awk_val_nil) == -1)
		{
			/* restore the stack_top with the saved value
			 * instead of calling __raw_pop as many times as
			 * the successful __raw_push. it is ok because
			 * the values pushed so far are all ase_awk_val_nil */
			run->stack_top = saved_stack_top;
			PANIC_I (run, ASE_AWK_ENOMEM);
		}
	}	

	if (ase_awk_setglobal (run, ASE_AWK_GLOBAL_NR, ase_awk_val_zero) == -1)
	{
		/* it can simply restore the top of the stack this way
		 * because the values pused onto the stack so far are
		 * all ase_awk_val_nils */
		run->stack_top = saved_stack_top;
		return -1;
	}

	if (ase_awk_setglobal (run, ASE_AWK_GLOBAL_NF, ase_awk_val_zero) == -1)
	{
		/* it can simply restore the top of the stack this way
		 * because the values pused onto the stack so far are
		 * all ase_awk_val_nils  and ase_awk_val_zeros */
		run->stack_top = saved_stack_top;
		return -1;
	}
	
	if (runarg != ASE_NULL && __build_runarg (run, runarg) == -1)
	{
		/* it can simply restore the top of the stack this way
		 * because the values pused onto the stack so far are
		 * all ase_awk_val_nils and ase_awk_val_zeros and 
		 * __build_runarg doesn't push other values than them
		 * when it has failed */
		run->stack_top = saved_stack_top;
		return -1;
	}

	run->exit_level = EXIT_NONE;

	n = __update_fnr (run, 0);
	if (n == 0) n = __set_globals_to_default (run);
	if (n == 0 && (run->awk->option & ASE_AWK_RUNMAIN))
	{
/* TODO: should the main function be user-specifiable? */
		ase_awk_nde_call_t nde;

		nde.type = ASE_AWK_NDE_AFN;
		nde.next = ASE_NULL;
		nde.what.afn.name = ASE_T("main");
		nde.what.afn.name_len = 4;
		nde.args = ASE_NULL;
		nde.nargs = 0;

		v = __eval_afn (run, (ase_awk_nde_t*)&nde);
		if (v == ASE_NULL) n = -1;
		else
		{
			/* destroy the return value if necessary */
			ase_awk_refupval (v);
			ase_awk_refdownval (run, v);
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
			PANIC_I (run, ASE_AWK_ENOMEM);
		}

		if (__raw_push(run,(void*)saved_stack_top) == -1) 
		{
			run->stack_top = saved_stack_top;
			__raw_pop_times (run, run->awk->tree.nglobals);
			PANIC_I (run, ASE_AWK_ENOMEM);
		}
	
		/* secure space for a return value */
		if (__raw_push(run,ase_awk_val_nil) == -1)
		{
			run->stack_top = saved_stack_top;
			__raw_pop_times (run, run->awk->tree.nglobals);
			PANIC_I (run, ASE_AWK_ENOMEM);
		}
	
		/* secure space for nargs */
		if (__raw_push(run,ase_awk_val_nil) == -1)
		{
			run->stack_top = saved_stack_top;
			__raw_pop_times (run, run->awk->tree.nglobals);
			PANIC_I (run, ASE_AWK_ENOMEM);
		}
	
		run->stack_base = saved_stack_top;
	
		/* set nargs to zero */
		nargs = 0;
		STACK_NARGS(run) = (void*)nargs;
	
		/* stack set up properly. ready to exeucte statement blocks */
		if (n == 0 && 
		    run->awk->tree.begin != ASE_NULL && 
		    run->exit_level != EXIT_ABORT)
		{
			ase_awk_nde_blk_t* blk;

			blk = (ase_awk_nde_blk_t*)run->awk->tree.begin;
			ASE_AWK_ASSERT (run->awk, blk->type == ASE_AWK_NDE_BLK);

			run->active_block = blk;
			run->exit_level = EXIT_NONE;
			if (__run_block (run, blk) == -1) n = -1;
		}

		if (n == 0 && 
		    run->awk->tree.chain != ASE_NULL && 
		    run->exit_level != EXIT_ABORT)
		{
			if (__run_pattern_blocks (run) == -1) n = -1;
		}

		if (n == 0 && 
		    run->awk->tree.end != ASE_NULL && 
		    run->exit_level != EXIT_ABORT) 
		{
			ase_awk_nde_blk_t* blk;

			blk = (ase_awk_nde_blk_t*)run->awk->tree.end;
			ASE_AWK_ASSERT (run->awk, blk->type == ASE_AWK_NDE_BLK);

			run->active_block = blk;
			run->exit_level = EXIT_NONE;
			if (__run_block (run, blk) == -1) n = -1;
		}

		/* restore stack */
		nargs = (ase_size_t)STACK_NARGS(run);
		ASE_AWK_ASSERT (run->awk, nargs == 0);
		for (i = 0; i < nargs; i++)
		{
			ase_awk_refdownval (run, STACK_ARG(run,i));
		}

		v = STACK_RETVAL(run);
xp_printf (ASE_T("Return Value - "));
ase_awk_printval (v);
xp_printf (ASE_T("\n"));
		/* the life of the global return value is over here
		 * unlike the return value of each function */
		/*ase_awk_refdownval_nofree (awk, v);*/
		ase_awk_refdownval (run, v);

		run->stack_top = 
			(ase_size_t)run->stack[run->stack_base+1];
		run->stack_base = 
			(ase_size_t)run->stack[run->stack_base+0];
	}

	/* pops off the global variables */
	nglobals = run->awk->tree.nglobals; /*run->nglobals */
	while (nglobals > 0)
	{
		--nglobals;
		ase_awk_refdownval (run, STACK_GLOBAL(run,nglobals));
		__raw_pop (run);
	}

	/* just reset the exit level */
	run->exit_level = EXIT_NONE;

xp_printf (ASE_T("-[VARIABLES]------------------------\n"));
ase_awk_map_walk (&run->named, __printval, ASE_NULL);
xp_printf (ASE_T("-[END VARIABLES]--------------------------\n"));

	return n;
}

static int __run_pattern_blocks (ase_awk_run_t* run)
{
	ase_ssize_t n;
	ase_bool_t need_to_close = ase_false;

	run->inrec.buf_pos = 0;
	run->inrec.buf_len = 0;
	run->inrec.eof = ase_false;

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
			ase_awk_closeextio_read (
				run, ASE_AWK_IN_CONSOLE, ASE_T(""));

			run->errnum = saved;
			return -1;
		}

		need_to_close = ase_true;
		if (x == 0) break; /* end of input */

		__update_fnr (run, run->global.fnr + 1);

		if (__run_pattern_block_chain (run, run->awk->tree.chain) == -1)
		{
			int saved = run->errnum;

			ase_awk_closeextio_read (
				run, ASE_AWK_IN_CONSOLE, ASE_T(""));

			run->errnum = saved;
			return -1;
		}
	}

	/* In case of getline, the code would make getline return -1, 
	 * set ERRNO, make this function return 0 after having checked 
	 * if closextio has returned -1 and errnum has been set to 
	 * ASE_AWK_EIOHANDLER. But this part of the code ends the input for 
	 * the implicit pattern-block loop, which is totally different 
	 * from getline. so it returns -1 as long as closeextio returns 
	 * -1 regardless of the value of errnum.  */
	if (need_to_close)
	{
		n = ase_awk_closeextio_read (
			run, ASE_AWK_IN_CONSOLE, ASE_T(""));
		if (n == -1) 
		{
			if (run->errnum == ASE_AWK_EIOHANDLER)
				PANIC_I (run, ASE_AWK_ECONINCLOSE);
			else return -1;
		}
	}

	return 0;
}

static int __run_pattern_block_chain (ase_awk_run_t* run, ase_awk_chain_t* chain)
{
	ase_size_t block_no = 0;

	while (run->exit_level != EXIT_GLOBAL &&
	       run->exit_level != EXIT_ABORT && chain != ASE_NULL)
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
	ase_awk_run_t* run, ase_awk_chain_t* chain, ase_size_t block_no)
{
	ase_awk_nde_t* ptn;
	ase_awk_nde_blk_t* blk;

	ptn = chain->pattern;
	blk = (ase_awk_nde_blk_t*)chain->action;

	if (ptn == ASE_NULL)
	{
		/* just execute the block */
		run->active_block = blk;
		if (__run_block (run, blk) == -1) return -1;
	}
	else
	{
		if (ptn->next == ASE_NULL)
		{
			/* pattern { ... } */
			ase_awk_val_t* v1;

			v1 = __eval_expression (run, ptn);
			if (v1 == ASE_NULL) return -1;

			ase_awk_refupval (v1);

			if (ase_awk_valtobool (run, v1))
			{
				run->active_block = blk;
				if (__run_block (run, blk) == -1) 
				{
					ase_awk_refdownval (run, v1);
					return -1;
				}
			}

			ase_awk_refdownval (run, v1);
		}
		else
		{
			/* pattern, pattern { ... } */
			ASE_AWK_ASSERT (run->awk, ptn->next->next == ASE_NULL);

			if (run->pattern_range_state[block_no] == 0)
			{
				ase_awk_val_t* v1;

				v1 = __eval_expression (run, ptn);
				if (v1 == ASE_NULL) return -1;
				ase_awk_refupval (v1);

				if (ase_awk_valtobool (run, v1))
				{
					run->active_block = blk;
					if (__run_block (run, blk) == -1) 
					{
						ase_awk_refdownval (run, v1);
						return -1;
					}

					run->pattern_range_state[block_no] = 1;
				}

				ase_awk_refdownval (run, v1);
			}
			else if (run->pattern_range_state[block_no] == 1)
			{
				ase_awk_val_t* v2;

				v2 = __eval_expression (run, ptn->next);
				if (v2 == ASE_NULL) return -1;
				ase_awk_refupval (v2);

				run->active_block = blk;
				if (__run_block (run, blk) == -1) 
				{
					ase_awk_refdownval (run, v2);
					return -1;
				}

				if (ase_awk_valtobool (run, v2)) 
					run->pattern_range_state[block_no] = 0;

				ase_awk_refdownval (run, v2);
			}
		}
	}

	return 0;
}

static int __run_block (ase_awk_run_t* run, ase_awk_nde_blk_t* nde)
{
	ase_awk_nde_t* p;
	ase_size_t nlocals;
	ase_size_t saved_stack_top;
	int n = 0;

	if (nde == ASE_NULL)
	{
		/* blockless pattern - execute print $0*/
		ase_awk_refupval (run->inrec.d0);

		/*n = ase_awk_writeextio_val (run, 
			ASE_AWK_OUT_CONSOLE, ASE_T(""), run->inrec.d0);*/
		n = ase_awk_writeextio_str (run, 
			ASE_AWK_OUT_CONSOLE, ASE_T(""),
			ASE_AWK_STR_BUF(&run->inrec.line),
			ASE_AWK_STR_LEN(&run->inrec.line));
		if (n == -1)
		{
			ase_awk_refdownval (run, run->inrec.d0);

			if (run->errnum == ASE_AWK_EIOHANDLER)
				PANIC_I (run, ASE_AWK_ECONOUTDATA);
			else return -1;
		}

		ase_awk_refdownval (run, run->inrec.d0);
		return 0;
	}

	ASE_AWK_ASSERT (run->awk, nde->type == ASE_AWK_NDE_BLK);

	p = nde->body;
	nlocals = nde->nlocals;

/*xp_printf (ASE_T("securing space for local variables nlocals = %d\n"), (int)nlocals);*/
	saved_stack_top = run->stack_top;

	/* secure space for local variables */
	while (nlocals > 0)
	{
		--nlocals;
		if (__raw_push(run,ase_awk_val_nil) == -1)
		{
			/* restore stack top */
			run->stack_top = saved_stack_top;
			return -1;
		}

		/* refupval is not required for ase_awk_val_nil */
	}

/*xp_printf (ASE_T("executing block statements\n"));*/
	while (p != ASE_NULL && run->exit_level == EXIT_NONE) 
	{
/*xp_printf (ASE_T("running a statement\n"));*/
		if (__run_statement(run,p) == -1) 
		{
			n = -1;
			break;
		}
		p = p->next;
	}

/*xp_printf (ASE_T("popping off local variables\n"));*/
	/* pop off local variables */
	nlocals = nde->nlocals;
	while (nlocals > 0)
	{
		--nlocals;
		ase_awk_refdownval (run, STACK_LOCAL(run,nlocals));
		__raw_pop (run);
	}

	return n;
}

static int __run_statement (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	switch (nde->type) 
	{
		case ASE_AWK_NDE_NULL:
		{
			/* do nothing */
			break;
		}

		case ASE_AWK_NDE_BLK:
		{
			if (__run_block (run, 
				(ase_awk_nde_blk_t*)nde) == -1) return -1;
			break;
		}

		case ASE_AWK_NDE_IF:
		{
			if (__run_if (run, 
				(ase_awk_nde_if_t*)nde) == -1) return -1;	
			break;
		}

		case ASE_AWK_NDE_WHILE:
		case ASE_AWK_NDE_DOWHILE:
		{
			if (__run_while (run, 
				(ase_awk_nde_while_t*)nde) == -1) return -1;
			break;
		}

		case ASE_AWK_NDE_FOR:
		{
			if (__run_for (run, 
				(ase_awk_nde_for_t*)nde) == -1) return -1;
			break;
		}

		case ASE_AWK_NDE_FOREACH:
		{
			if (__run_foreach (run, 
				(ase_awk_nde_foreach_t*)nde) == -1) return -1;
			break;
		}

		case ASE_AWK_NDE_BREAK:
		{
			if (__run_break (run, 
				(ase_awk_nde_break_t*)nde) == -1) return -1;
			break;
		}

		case ASE_AWK_NDE_CONTINUE:
		{
			if (__run_continue (run, 
				(ase_awk_nde_continue_t*)nde) == -1) return -1;
			break;
		}

		case ASE_AWK_NDE_RETURN:
		{
			if (__run_return (run, 
				(ase_awk_nde_return_t*)nde) == -1) return -1;
			break;
		}

		case ASE_AWK_NDE_EXIT:
		{
			if (__run_exit (run, 
				(ase_awk_nde_exit_t*)nde) == -1) return -1;
			break;
		}

		case ASE_AWK_NDE_NEXT:
		{
			if (__run_next (run, 
				(ase_awk_nde_next_t*)nde) == -1) return -1;
			break;
		}

		case ASE_AWK_NDE_NEXTFILE:
		{
			if (__run_nextfile (run, 
				(ase_awk_nde_nextfile_t*)nde) == -1) return -1;
			break;
		}

		case ASE_AWK_NDE_DELETE:
		{
			if (__run_delete (run, 
				(ase_awk_nde_delete_t*)nde) == -1) return -1;
			break;
		}

		case ASE_AWK_NDE_PRINT:
		{
			if (__run_print (run, 
				(ase_awk_nde_print_t*)nde) == -1) return -1;
			break;
		}

		default:
		{
			ase_awk_val_t* v;
			v = __eval_expression(run,nde);
			if (v == ASE_NULL) return -1;

			ase_awk_refupval (v);
			ase_awk_refdownval (run, v);

			break;
		}
	}

	return 0;
}

static int __run_if (ase_awk_run_t* run, ase_awk_nde_if_t* nde)
{
	ase_awk_val_t* test;
	int n = 0;

	/* the test expression for the if statement cannot have 
	 * chained expressions. this should not be allowed by the
	 * parser first of all */
	ASE_AWK_ASSERT (run->awk, nde->test->next == ASE_NULL);

	test = __eval_expression (run, nde->test);
	if (test == ASE_NULL) return -1;

	ase_awk_refupval (test);
	if (ase_awk_valtobool (run, test))
	{
		n = __run_statement (run, nde->then_part);
	}
	else if (nde->else_part != ASE_NULL)
	{
		n = __run_statement (run, nde->else_part);
	}

	ase_awk_refdownval (run, test); /* TODO: is this correct?*/
	return n;
}

static int __run_while (ase_awk_run_t* run, ase_awk_nde_while_t* nde)
{
	ase_awk_val_t* test;

	if (nde->type == ASE_AWK_NDE_WHILE)
	{
		/* no chained expressions are allowed for the test 
		 * expression of the while statement */
		ASE_AWK_ASSERT (run->awk, nde->test->next == ASE_NULL);

		/* TODO: handle run-time abortion... */
		while (1)
		{
			test = __eval_expression (run, nde->test);
			if (test == ASE_NULL) return -1;

			ase_awk_refupval (test);

			if (ase_awk_valtobool (run, test))
			{
				if (__run_statement(run,nde->body) == -1)
				{
					ase_awk_refdownval (run, test);
					return -1;
				}
			}
			else
			{
				ase_awk_refdownval (run, test);
				break;
			}

			ase_awk_refdownval (run, test);

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
	else if (nde->type == ASE_AWK_NDE_DOWHILE)
	{
		/* no chained expressions are allowed for the test 
		 * expression of the while statement */
		ASE_AWK_ASSERT (run->awk, nde->test->next == ASE_NULL);

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
			if (test == ASE_NULL) return -1;

			ase_awk_refupval (test);

			if (!ase_awk_valtobool (run, test))
			{
				ase_awk_refdownval (run, test);
				break;
			}

			ase_awk_refdownval (run, test);
		}
		while (1);
	}

	return 0;
}

static int __run_for (ase_awk_run_t* run, ase_awk_nde_for_t* nde)
{
	ase_awk_val_t* val;

	if (nde->init != ASE_NULL)
	{
		ASE_AWK_ASSERT (run->awk, nde->init->next == ASE_NULL);
		val = __eval_expression(run,nde->init);
		if (val == ASE_NULL) return -1;

		ase_awk_refupval (val);
		ase_awk_refdownval (run, val);
	}

	while (1)
	{
		if (nde->test != ASE_NULL)
		{
			ase_awk_val_t* test;

			/* no chained expressions for the test expression of
			 * the for statement are allowed */
			ASE_AWK_ASSERT (run->awk, nde->test->next == ASE_NULL);

			test = __eval_expression (run, nde->test);
			if (test == ASE_NULL) return -1;

			ase_awk_refupval (test);
			if (ase_awk_valtobool (run, test))
			{
				if (__run_statement(run,nde->body) == -1)
				{
					ase_awk_refdownval (run, test);
					return -1;
				}
			}
			else
			{
				ase_awk_refdownval (run, test);
				break;
			}

			ase_awk_refdownval (run, test);
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

		if (nde->incr != ASE_NULL)
		{
			ASE_AWK_ASSERT (run->awk, nde->incr->next == ASE_NULL);
			val = __eval_expression(run,nde->incr);
			if (val == ASE_NULL) return -1;

			ase_awk_refupval (val);
			ase_awk_refdownval (run, val);
		}
	}

	return 0;
}

struct __foreach_walker_t
{
	ase_awk_run_t* run;
	ase_awk_nde_t* var;
	ase_awk_nde_t* body;
};

static int __walk_foreach (ase_awk_pair_t* pair, void* arg)
{
	struct __foreach_walker_t* w = (struct __foreach_walker_t*)arg;
	ase_awk_val_t* str;

	str = (ase_awk_val_t*) ase_awk_makestrval (
		w->run, pair->key, ase_awk_strlen(pair->key));
	if (str == ASE_NULL) PANIC_I (w->run, ASE_AWK_ENOMEM);

	ase_awk_refupval (str);
	if (__do_assignment (w->run, w->var, str) == ASE_NULL)
	{
		ase_awk_refdownval (w->run, str);
		return -1;
	}

	if (__run_statement (w->run, w->body) == -1)
	{
		ase_awk_refdownval (w->run, str);
		return -1;
	}
	
	ase_awk_refdownval (w->run, str);
	return 0;
}

static int __run_foreach (ase_awk_run_t* run, ase_awk_nde_foreach_t* nde)
{
	int n;
	ase_awk_nde_exp_t* test;
	ase_awk_val_t* rv;
	ase_awk_map_t* map;
	struct __foreach_walker_t walker;

	test = (ase_awk_nde_exp_t*)nde->test;
	ASE_AWK_ASSERT (run->awk, test->type == ASE_AWK_NDE_EXP_BIN && 
	           test->opcode == ASE_AWK_BINOP_IN);

	/* chained expressions should not be allowed 
	 * by the parser first of all */
	ASE_AWK_ASSERT (run->awk, test->right->next == ASE_NULL); 

	rv = __eval_expression (run, test->right);
	if (rv == ASE_NULL) return -1;

	ase_awk_refupval (rv);
	if (rv->type != ASE_AWK_VAL_MAP)
	{
		ase_awk_refdownval (run, rv);
		PANIC_I (run, ASE_AWK_ENOTINDEXABLE);
	}
	map = ((ase_awk_val_map_t*)rv)->map;

	walker.run = run;
	walker.var = test->left;
	walker.body = nde->body;
	n = ase_awk_map_walk (map, __walk_foreach, &walker);

	ase_awk_refdownval (run, rv);
	return n;
}

static int __run_break (ase_awk_run_t* run, ase_awk_nde_break_t* nde)
{
	run->exit_level = EXIT_BREAK;
	return 0;
}

static int __run_continue (ase_awk_run_t* run, ase_awk_nde_continue_t* nde)
{
	run->exit_level = EXIT_CONTINUE;
	return 0;
}

static int __run_return (ase_awk_run_t* run, ase_awk_nde_return_t* nde)
{
	if (nde->val != ASE_NULL)
	{
		ase_awk_val_t* val;

		/* chained expressions should not be allowed 
		 * by the parser first of all */
		ASE_AWK_ASSERT (run->awk, nde->val->next == ASE_NULL); 

/*xp_printf (ASE_T("returning....\n"));*/
		val = __eval_expression (run, nde->val);
		if (val == ASE_NULL) return -1;

		ase_awk_refdownval (run, STACK_RETVAL(run));
		STACK_RETVAL(run) = val;
		ase_awk_refupval (val); /* see __eval_call for the trick */
/*xp_printf (ASE_T("set return value....\n"));*/
	}
	
	run->exit_level = EXIT_FUNCTION;
	return 0;
}

static int __run_exit (ase_awk_run_t* run, ase_awk_nde_exit_t* nde)
{
	if (nde->val != ASE_NULL)
	{
		ase_awk_val_t* val;

		/* chained expressions should not be allowed 
		 * by the parser first of all */
		ASE_AWK_ASSERT (run->awk, nde->val->next == ASE_NULL); 

		val = __eval_expression (run, nde->val);
		if (val == ASE_NULL) return -1;

		ase_awk_refdownval (run, STACK_RETVAL_GLOBAL(run));
		STACK_RETVAL_GLOBAL(run) = val; /* global return value */

		ase_awk_refupval (val);
	}

	run->exit_level = EXIT_GLOBAL;
	return 0;
}

static int __run_next (ase_awk_run_t* run, ase_awk_nde_next_t* nde)
{
	/* the parser checks if next has been called in the begin/end
	 * block or whereever inappropriate. so the runtime doesn't 
	 * check that explicitly */

	if  (run->active_block == (ase_awk_nde_blk_t*)run->awk->tree.begin ||
	     run->active_block == (ase_awk_nde_blk_t*)run->awk->tree.end)
	{
		PANIC_I (run, ASE_AWK_ENEXTCALL);
	}

	run->exit_level = EXIT_NEXT;
	return 0;
}

static int __run_nextfile (ase_awk_run_t* run, ase_awk_nde_nextfile_t* nde)
{
/* TODO: some extentions such as nextfile "in/out"; 
 *  what about awk -i in1,in2,in3 -o out1,out2,out3 ?
 */
	int n;

	if  (run->active_block == (ase_awk_nde_blk_t*)run->awk->tree.begin ||
	     run->active_block == (ase_awk_nde_blk_t*)run->awk->tree.end)
	{
		run->errnum = ASE_AWK_ENEXTFILECALL;
		return -1;
	}

	n = ase_awk_nextextio_read (run, ASE_AWK_IN_CONSOLE, ASE_T(""));
	if (n == -1)
	{
		if (run->errnum == ASE_AWK_EIOHANDLER)
			run->errnum = ASE_AWK_ECONINNEXT;
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

static int __run_delete (ase_awk_run_t* run, ase_awk_nde_delete_t* nde)
{
	ase_awk_nde_var_t* var;

	var = (ase_awk_nde_var_t*) nde->var;

	if (var->type == ASE_AWK_NDE_NAMED ||
	    var->type == ASE_AWK_NDE_NAMEDIDX)
	{
		ase_awk_pair_t* pair;

		ASE_AWK_ASSERT (run->awk, (var->type == ASE_AWK_NDE_NAMED && 
		            var->idx == ASE_NULL) ||
		           (var->type == ASE_AWK_NDE_NAMEDIDX && 
		            var->idx != ASE_NULL));

		pair = ase_awk_map_get (
			&run->named, var->id.name, var->id.name_len);
		if (pair == ASE_NULL)
		{
			ase_awk_val_t* tmp;

			/* value not set for the named variable. 
			 * create a map and assign it to the variable */

			tmp = ase_awk_makemapval (run);
			if (tmp == ASE_NULL) PANIC_I (run, ASE_AWK_ENOMEM);

			if (ase_awk_map_put (&run->named, 
				var->id.name, var->id.name_len, tmp) == ASE_NULL)
			{
				ase_awk_refupval (tmp);
				ase_awk_refdownval (run, tmp);
				PANIC_I (run, ASE_AWK_ENOMEM);		
			}

			ase_awk_refupval (tmp);
		}
		else
		{
			ase_awk_val_t* val;
			ase_awk_map_t* map;

			val = (ase_awk_val_t*)pair->val;
			ASE_AWK_ASSERT (run->awk, val != ASE_NULL);

			if (val->type != ASE_AWK_VAL_MAP)
				PANIC_I (run, ASE_AWK_ENOTDELETABLE);

			map = ((ase_awk_val_map_t*)val)->map;
			if (var->type == ASE_AWK_NDE_NAMEDIDX)
			{
				ase_char_t* key;
				ase_size_t key_len;
				ase_awk_val_t* idx;

				ASE_AWK_ASSERT (run->awk, var->idx != ASE_NULL);

				idx = __eval_expression (run, var->idx);
				if (idx == ASE_NULL) return -1;

				ase_awk_refupval (idx);
				key = ase_awk_valtostr (
					run, idx, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &key_len);
				ase_awk_refdownval (run, idx);

				if (key == ASE_NULL) return -1;

				ase_awk_map_remove (map, key, key_len);
				ASE_AWK_FREE (run->awk, key);
			}
			else
			{
				ase_awk_map_clear (map);
			}
		}
	}
	else if (var->type == ASE_AWK_NDE_GLOBAL ||
	         var->type == ASE_AWK_NDE_LOCAL ||
	         var->type == ASE_AWK_NDE_ARG ||
	         var->type == ASE_AWK_NDE_GLOBALIDX ||
	         var->type == ASE_AWK_NDE_LOCALIDX ||
	         var->type == ASE_AWK_NDE_ARGIDX)
	{
		ase_awk_val_t* val;

		if (var->type == ASE_AWK_NDE_GLOBAL ||
		    var->type == ASE_AWK_NDE_GLOBALIDX)
			val = STACK_GLOBAL (run,var->id.idxa);
		else if (var->type == ASE_AWK_NDE_LOCAL ||
		         var->type == ASE_AWK_NDE_LOCALIDX)
			val = STACK_LOCAL (run,var->id.idxa);
		else val = STACK_ARG (run,var->id.idxa);

		ASE_AWK_ASSERT (run->awk, val != ASE_NULL);

		if (val->type == ASE_AWK_VAL_NIL)
		{
			ase_awk_val_t* tmp;

			/* value not set for the named variable. 
			 * create a map and assign it to the variable */

			tmp = ase_awk_makemapval (run);
			if (tmp == ASE_NULL) PANIC_I (run, ASE_AWK_ENOMEM);

			/* no need to reduce the reference count of
			 * the previous value because it was nil. */
			if (var->type == ASE_AWK_NDE_GLOBAL ||
			    var->type == ASE_AWK_NDE_GLOBALIDX)
			{
				if (ase_awk_setglobal (
					run, var->id.idxa, tmp) == -1)
				{
					ase_awk_refupval (tmp);
					ase_awk_refdownval (run, tmp);
					return -1;
				}
			}
			else if (var->type == ASE_AWK_NDE_LOCAL ||
			         var->type == ASE_AWK_NDE_LOCALIDX)
			{
				STACK_LOCAL(run,var->id.idxa) = tmp;
				ase_awk_refupval (tmp);
			}
			else 
			{
				STACK_ARG(run,var->id.idxa) = tmp;
				ase_awk_refupval (tmp);
			}
		}
		else
		{
			ase_awk_map_t* map;

			if (val->type != ASE_AWK_VAL_MAP)
				PANIC_I (run, ASE_AWK_ENOTDELETABLE);

			map = ((ase_awk_val_map_t*)val)->map;
			if (var->type == ASE_AWK_NDE_GLOBALIDX ||
			    var->type == ASE_AWK_NDE_LOCALIDX ||
			    var->type == ASE_AWK_NDE_ARGIDX)
			{
				ase_char_t* key;
				ase_size_t key_len;
				ase_awk_val_t* idx;

				ASE_AWK_ASSERT (run->awk, var->idx != ASE_NULL);

				idx = __eval_expression (run, var->idx);
				if (idx == ASE_NULL) return -1;

				ase_awk_refupval (idx);
				key = ase_awk_valtostr (
					run, idx, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &key_len);
				ase_awk_refdownval (run, idx);

				if (key == ASE_NULL) return -1;

				ase_awk_map_remove (map, key, key_len);
				ASE_AWK_FREE (run->awk, key);
			}
			else
			{
				ase_awk_map_clear (map);
			}
		}
	}
	else
	{
		ASE_AWK_ASSERT (run->awk, !"should never happen - wrong variable type for delete");
		PANIC_I (run, ASE_AWK_EINTERNAL);
	}

	return 0;
}

static int __run_print (ase_awk_run_t* run, ase_awk_nde_print_t* nde)
{
	ase_awk_nde_print_t* p = (ase_awk_nde_print_t*)nde;
	ase_char_t* out = ASE_NULL;
	const ase_char_t* dst;
	ase_awk_val_t* v;
	ase_awk_nde_t* np;
	int n;

	ASE_AWK_ASSERT (run->awk, 
		(p->out_type == ASE_AWK_OUT_PIPE && p->out != ASE_NULL) ||
		(p->out_type == ASE_AWK_OUT_COPROC && p->out != ASE_NULL) ||
		(p->out_type == ASE_AWK_OUT_FILE && p->out != ASE_NULL) ||
		(p->out_type == ASE_AWK_OUT_FILE_APPEND && p->out != ASE_NULL) ||
		(p->out_type == ASE_AWK_OUT_CONSOLE && p->out == ASE_NULL));

	if (p->out != ASE_NULL)
	{
		ase_size_t len;

		v = __eval_expression (run, p->out);
		if (v == ASE_NULL) return -1;

		ase_awk_refupval (v);
		out = ase_awk_valtostr (
			run, v, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &len);
		if (out == ASE_NULL) 
		{
			ase_awk_refdownval (run, v);
			return -1;
		}
		ase_awk_refdownval (run, v);

		if (len <= 0) 
		{
			/* the output destination name is empty. */
			ASE_AWK_FREE (run->awk, out);
			n = -1;
			goto skip_write;
		}

		while (len > 0)
		{
			if (out[--len] == ASE_T('\0'))
			{
				/* the output destination name contains a null 
				 * character. */
				ASE_AWK_FREE (run->awk, out);
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

	dst = (out == ASE_NULL)? ASE_T(""): out;

	if (p->args == ASE_NULL)
	{
		/*
		v = run->inrec.d0;
		ase_awk_refupval (v);
		n = ase_awk_writeextio_val (run, p->out_type, dst, v);
		if (n < 0 && run->errnum != ASE_AWK_EIOHANDLER) 
		{
			if (out != ASE_NULL) ASE_AWK_FREE (run->awk, out);
			ase_awk_refdownval (run, v);
			return -1;
		}
		ase_awk_refdownval (run, v);
		*/
		n = ase_awk_writeextio_str (
			run, p->out_type, dst,
			ASE_AWK_STR_BUF(&run->inrec.line),
			ASE_AWK_STR_LEN(&run->inrec.line));
		if (n < 0 && run->errnum != ASE_AWK_EIOHANDLER) 
		{
			if (out != ASE_NULL) ASE_AWK_FREE (run->awk, out);
			return -1;
		}
		/* TODO: how to handle n == -1 && errnum == ASE_AWK_EIOHANDLER. 
		 * that is the user handler returned an error... */
	}
	else
	{
		for (np = p->args; np != ASE_NULL; np = np->next)
		{
			if (np != p->args)
			{
				n = ase_awk_writeextio_str (
					run, p->out_type, dst, 
					run->global.ofs.ptr, 
					run->global.ofs.len);
				if (n < 0 && run->errnum != ASE_AWK_EIOHANDLER) 
				{
					if (out != ASE_NULL) ASE_AWK_FREE (run->awk, out);
					return -1;
				}
			}

			v = __eval_expression (run, np);
			if (v == ASE_NULL) 
			{
				if (out != ASE_NULL) ASE_AWK_FREE (run->awk, out);
				return -1;
			}
			ase_awk_refupval (v);

			n = ase_awk_writeextio_val (run, p->out_type, dst, v);
			if (n < 0 && run->errnum != ASE_AWK_EIOHANDLER) 
			{
				if (out != ASE_NULL) ASE_AWK_FREE (run->awk, out);
				ase_awk_refdownval (run, v);
				return -1;
			}

			ase_awk_refdownval (run, v);


			/* TODO: how to handle n == -1 && run->errnum == ASE_AWK_EIOHANDLER. 
			 * that is the user handler returned an error... */
		}
	}

	n = ase_awk_writeextio_str (
		run, p->out_type, dst, 
		run->global.ors.ptr, run->global.ors.len);
	if (n < 0 && run->errnum != ASE_AWK_EIOHANDLER)
	{
		if (out != ASE_NULL) ASE_AWK_FREE (run->awk, out);
		return -1;
	}

	/* TODO: how to handle n == -1 && errnum == ASE_AWK_EIOHANDLER.
	 * that is the user handler returned an error... */

	if (out != ASE_NULL) ASE_AWK_FREE (run->awk, out);

skip_write:
	return 0;
}

static ase_awk_val_t* __eval_expression (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	ase_awk_val_t* v;
	int n, errnum;

	v = __eval_expression0 (run, nde);
	if (v == ASE_NULL) return ASE_NULL;

	if (v->type == ASE_AWK_VAL_REX)
	{
		ase_awk_refupval (v);

		if (run->inrec.d0->type == ASE_AWK_VAL_NIL)
		{
			/* the record has never been read. 
			 * probably, this functions has been triggered
			 * by the statements in the BEGIN block */
			n = ase_awk_isemptyrex (run->awk, ((ase_awk_val_rex_t*)v)->code)? 1: 0;
		}
		else
		{
			ASE_AWK_ASSERT (run->awk, run->inrec.d0->type == ASE_AWK_VAL_STR);

			n = ase_awk_matchrex (
				((ase_awk_run_t*)run)->awk, 
				((ase_awk_val_rex_t*)v)->code,
				((((ase_awk_run_t*)run)->global.ignorecase)? ASE_AWK_REX_IGNORECASE: 0),
				((ase_awk_val_str_t*)run->inrec.d0)->buf,
				((ase_awk_val_str_t*)run->inrec.d0)->len,
				ASE_NULL, ASE_NULL, &errnum);
	
			if (n == -1) 
			{
				ase_awk_refdownval (run, v);
				PANIC (run, errnum);
			}
		}

		ase_awk_refdownval (run, v);

		v = ase_awk_makeintval (run, (n != 0));
		if (v == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);
	}

	return v;
}

static ase_awk_val_t* __eval_expression0 (ase_awk_run_t* run, ase_awk_nde_t* nde)
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

	ASE_AWK_ASSERT (run->awk, nde->type >= ASE_AWK_NDE_GRP &&
		(nde->type - ASE_AWK_NDE_GRP) < ase_countof(__eval_func));

	return __eval_func[nde->type-ASE_AWK_NDE_GRP] (run, nde);
}

static ase_awk_val_t* __eval_group (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	/* __eval_binop_in evaluates the ASE_AWK_NDE_GRP specially.
	 * so this function should never be reached. */
	ASE_AWK_ASSERT (run->awk, !"should never happen - NDE_GRP only for in");
	PANIC (run, ASE_AWK_EINTERNAL);
	return ASE_NULL;
}

static ase_awk_val_t* __eval_assignment (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	ase_awk_val_t* val, * ret;
	ase_awk_nde_ass_t* ass = (ase_awk_nde_ass_t*)nde;

	ASE_AWK_ASSERT (run->awk, ass->left != ASE_NULL && ass->right != ASE_NULL);

	ASE_AWK_ASSERT (run->awk, ass->right->next == ASE_NULL);
	val = __eval_expression (run, ass->right);
	if (val == ASE_NULL) return ASE_NULL;

	ase_awk_refupval (val);

	if (ass->opcode != ASE_AWK_ASSOP_NONE)
	{
		ase_awk_val_t* val2, * tmp;

		ASE_AWK_ASSERT (run->awk, ass->left->next == ASE_NULL);
		val2 = __eval_expression (run, ass->left);
		if (val2 == ASE_NULL)
		{
			ase_awk_refdownval (run, val);
			return ASE_NULL;
		}

		ase_awk_refupval (val2);

		if (ass->opcode == ASE_AWK_ASSOP_PLUS)
		{
			tmp = __eval_binop_plus (run, val2, val);
		}
		else if (ass->opcode == ASE_AWK_ASSOP_MINUS)
		{
			tmp = __eval_binop_minus (run, val2, val);
		}
		else if (ass->opcode == ASE_AWK_ASSOP_MUL)
		{
			tmp = __eval_binop_mul (run, val2, val);
		}
		else if (ass->opcode == ASE_AWK_ASSOP_DIV)
		{
			tmp = __eval_binop_div (run, val2, val);
		}
		else if (ass->opcode == ASE_AWK_ASSOP_MOD)
		{
			tmp = __eval_binop_mod (run, val2, val);
		}
		else if (ass->opcode == ASE_AWK_ASSOP_EXP)
		{
			tmp = __eval_binop_exp (run, val2, val);
		}
		else
		{
			ASE_AWK_ASSERT (run->awk, !"should never happen - invalid assignment opcode");
			PANIC (run, ASE_AWK_EINTERNAL);
		}

		if (tmp == ASE_NULL)
		{
			ase_awk_refdownval (run, val);
			ase_awk_refdownval (run, val2);
			return ASE_NULL;
		}

		ase_awk_refdownval (run, val);
		val = tmp;
		ase_awk_refupval (val);
	}

	ret = __do_assignment (run, ass->left, val);
	ase_awk_refdownval (run, val);

	return ret;
}

static ase_awk_val_t* __do_assignment (
	ase_awk_run_t* run, ase_awk_nde_t* var, ase_awk_val_t* val)
{
	ase_awk_val_t* ret;

	if (val->type == ASE_AWK_VAL_MAP)
	{
		/* a map cannot be assigned to a variable */
		PANIC (run, ASE_AWK_ENOTASSIGNABLE);
	}

	if (var->type == ASE_AWK_NDE_NAMED ||
	    var->type == ASE_AWK_NDE_GLOBAL ||
	    var->type == ASE_AWK_NDE_LOCAL ||
	    var->type == ASE_AWK_NDE_ARG) 
	{
		ret = __do_assignment_scalar (run, (ase_awk_nde_var_t*)var, val);
	}
	else if (var->type == ASE_AWK_NDE_NAMEDIDX ||
	         var->type == ASE_AWK_NDE_GLOBALIDX ||
	         var->type == ASE_AWK_NDE_LOCALIDX ||
	         var->type == ASE_AWK_NDE_ARGIDX) 
	{
		ret = __do_assignment_map (run, (ase_awk_nde_var_t*)var, val);
	}
	else if (var->type == ASE_AWK_NDE_POS)
	{
		ret = __do_assignment_pos (run, (ase_awk_nde_pos_t*)var, val);
	}
	else
	{
		ASE_AWK_ASSERT (run->awk, !"should never happen - invalid variable type");
		PANIC (run, ASE_AWK_EINTERNAL);
	}

	return ret;
}

static ase_awk_val_t* __do_assignment_scalar (
	ase_awk_run_t* run, ase_awk_nde_var_t* var, ase_awk_val_t* val)
{
	ASE_AWK_ASSERT (run->awk, 
		(var->type == ASE_AWK_NDE_NAMED ||
		 var->type == ASE_AWK_NDE_GLOBAL ||
		 var->type == ASE_AWK_NDE_LOCAL ||
		 var->type == ASE_AWK_NDE_ARG) && var->idx == ASE_NULL);

	ASE_AWK_ASSERT (run->awk, val->type != ASE_AWK_VAL_MAP);

	if (var->type == ASE_AWK_NDE_NAMED) 
	{
		ase_awk_pair_t* pair;
		int n;

		pair = ase_awk_map_get (
			&run->named, var->id.name, var->id.name_len);
		if (pair != ASE_NULL && 
		    ((ase_awk_val_t*)pair->val)->type == ASE_AWK_VAL_MAP)
		{
			/* once a variable becomes an array,
			 * it cannot be changed to a scalar variable */
			PANIC (run, ASE_AWK_EMAPTOSCALAR);
		}

		n = ase_awk_map_putx (&run->named, 
			var->id.name, var->id.name_len, val, ASE_NULL);
		if (n < 0) PANIC (run, ASE_AWK_ENOMEM);

		ase_awk_refupval (val);
	}
	else if (var->type == ASE_AWK_NDE_GLOBAL) 
	{
		if (ase_awk_setglobal (
			run, var->id.idxa, val) == -1) return ASE_NULL;
	}
	else if (var->type == ASE_AWK_NDE_LOCAL) 
	{
		ase_awk_val_t* old = STACK_LOCAL(run,var->id.idxa);
		if (old->type == ASE_AWK_VAL_MAP)
		{	
			/* once the variable becomes an array,
			 * it cannot be changed to a scalar variable */
			PANIC (run, ASE_AWK_EMAPTOSCALAR);
		}

		ase_awk_refdownval (run, old);
		STACK_LOCAL(run,var->id.idxa) = val;
		ase_awk_refupval (val);
	}
	else /* if (var->type == ASE_AWK_NDE_ARG) */
	{
		ase_awk_val_t* old = STACK_ARG(run,var->id.idxa);
		if (old->type == ASE_AWK_VAL_MAP)
		{	
			/* once the variable becomes an array,
			 * it cannot be changed to a scalar variable */
			PANIC (run, ASE_AWK_EMAPTOSCALAR);
		}

		ase_awk_refdownval (run, old);
		STACK_ARG(run,var->id.idxa) = val;
		ase_awk_refupval (val);
	}

	return val;
}

static ase_awk_val_t* __do_assignment_map (
	ase_awk_run_t* run, ase_awk_nde_var_t* var, ase_awk_val_t* val)
{
	ase_awk_val_map_t* map;
	ase_char_t* str;
	ase_size_t len;
	int n;

	ASE_AWK_ASSERT (run->awk, 
		(var->type == ASE_AWK_NDE_NAMEDIDX ||
		 var->type == ASE_AWK_NDE_GLOBALIDX ||
		 var->type == ASE_AWK_NDE_LOCALIDX ||
		 var->type == ASE_AWK_NDE_ARGIDX) && var->idx != ASE_NULL);
	ASE_AWK_ASSERT (run->awk, val->type != ASE_AWK_VAL_MAP);

	if (var->type == ASE_AWK_NDE_NAMEDIDX)
	{
		ase_awk_pair_t* pair;
		pair = ase_awk_map_get (
			&run->named, var->id.name, var->id.name_len);
		map = (pair == ASE_NULL)? 
			(ase_awk_val_map_t*)ase_awk_val_nil: 
			(ase_awk_val_map_t*)pair->val;
	}
	else
	{
		map = (var->type == ASE_AWK_NDE_GLOBALIDX)? 
		      	(ase_awk_val_map_t*)STACK_GLOBAL(run,var->id.idxa):
		      (var->type == ASE_AWK_NDE_LOCALIDX)? 
		      	(ase_awk_val_map_t*)STACK_LOCAL(run,var->id.idxa):
		      	(ase_awk_val_map_t*)STACK_ARG(run,var->id.idxa);
	}

	if (map->type == ASE_AWK_VAL_NIL)
	{
		/* the map is not initialized yet */
		ase_awk_val_t* tmp;

		tmp = ase_awk_makemapval (run);
		if (tmp == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);

		if (var->type == ASE_AWK_NDE_NAMEDIDX)
		{
			/* doesn't have to decrease the reference count 
			 * of the previous value here as it is done by 
			 * ase_awk_map_put */
			if (ase_awk_map_put (&run->named, 
				var->id.name, var->id.name_len, tmp) == ASE_NULL)
			{
				ase_awk_refupval (tmp);
				ase_awk_refdownval (run, tmp);
				PANIC (run, ASE_AWK_ENOMEM);		
			}

			ase_awk_refupval (tmp);
		}
		else if (var->type == ASE_AWK_NDE_GLOBALIDX)
		{
			ase_awk_refupval (tmp);
			if (ase_awk_setglobal (run, var->id.idxa, tmp) == -1)
			{
				ase_awk_refdownval (run, tmp);
				return ASE_NULL;
			}
			ase_awk_refdownval (run, tmp);
		}
		else if (var->type == ASE_AWK_NDE_LOCALIDX)
		{
			ase_awk_refdownval (run, (ase_awk_val_t*)map);
			STACK_LOCAL(run,var->id.idxa) = tmp;
			ase_awk_refupval (tmp);
		}
		else /* if (var->type == ASE_AWK_NDE_ARGIDX) */
		{
			ase_awk_refdownval (run, (ase_awk_val_t*)map);
			STACK_ARG(run,var->id.idxa) = tmp;
			ase_awk_refupval (tmp);
		}

		map = (ase_awk_val_map_t*) tmp;
	}
	else if (map->type != ASE_AWK_VAL_MAP)
	{
		PANIC (run, ASE_AWK_ENOTINDEXABLE);
	}

	str = __idxnde_to_str (run, var->idx, &len);
	if (str == ASE_NULL) return ASE_NULL;

/*
xp_printf (ASE_T("**** index str=>%s, map->ref=%d, map->type=%d\n"), str, (int)map->ref, (int)map->type);
*/
	n = ase_awk_map_putx (map->map, str, len, val, ASE_NULL);
	if (n < 0)
	{
		ASE_AWK_FREE (run->awk, str);
		PANIC (run, ASE_AWK_ENOMEM);
	}

	ASE_AWK_FREE (run->awk, str);
	ase_awk_refupval (val);
	return val;
}

static ase_awk_val_t* __do_assignment_pos (
	ase_awk_run_t* run, ase_awk_nde_pos_t* pos, ase_awk_val_t* val)
{
	ase_awk_val_t* v;
	ase_long_t lv;
	ase_real_t rv;
	ase_char_t* str;
	ase_size_t len;
	int n;

	v = __eval_expression (run, pos->val);
	if (v == ASE_NULL) return ASE_NULL;

	ase_awk_refupval (v);
	n = ase_awk_valtonum (run, v, &lv, &rv);
	ase_awk_refdownval (run, v);

	if (n == -1) PANIC (run, ASE_AWK_EPOSIDX); 
	if (n == 1) lv = (ase_long_t)rv;
	if (!IS_VALID_POSIDX(lv)) PANIC (run, ASE_AWK_EPOSIDX);

	if (val->type == ASE_AWK_VAL_STR)
	{
		str = ((ase_awk_val_str_t*)val)->buf;
		len = ((ase_awk_val_str_t*)val)->len;
	}
	else
	{
		str = ase_awk_valtostr (run, val, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &len);
		if (str == ASE_NULL) return ASE_NULL;
	}
	
	n = ase_awk_setrec (run, (ase_size_t)lv, str, len);

	if (val->type != ASE_AWK_VAL_STR) ASE_AWK_FREE (run->awk, str);

	if (n == -1) return ASE_NULL;
	return (lv == 0)? run->inrec.d0: run->inrec.flds[lv-1].val;
}

static ase_awk_val_t* __eval_binary (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	static binop_func_t __binop_func[] =
	{
		/* the order of the functions should be inline with
		 * the operator declaration in run.h */

		ASE_NULL, /* __eval_binop_lor */
		ASE_NULL, /* __eval_binop_land */
		ASE_NULL, /* __eval_binop_in */

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
		ASE_NULL, /* __eval_binop_ma */
		ASE_NULL  /* __eval_binop_nm */
	};

	ase_awk_nde_exp_t* exp = (ase_awk_nde_exp_t*)nde;
	ase_awk_val_t* left, * right, * res;

	ASE_AWK_ASSERT (run->awk, exp->type == ASE_AWK_NDE_EXP_BIN);

	if (exp->opcode == ASE_AWK_BINOP_LAND)
	{
		res = __eval_binop_land (run, exp->left, exp->right);
	}
	else if (exp->opcode == ASE_AWK_BINOP_LOR)
	{
		res = __eval_binop_lor (run, exp->left, exp->right);
	}
	else if (exp->opcode == ASE_AWK_BINOP_IN)
	{
		/* treat the in operator specially */
		res = __eval_binop_in (run, exp->left, exp->right);
	}
	else if (exp->opcode == ASE_AWK_BINOP_NM)
	{
		res = __eval_binop_nm (run, exp->left, exp->right);
	}
	else if (exp->opcode == ASE_AWK_BINOP_MA)
	{
		res = __eval_binop_ma (run, exp->left, exp->right);
	}
	else
	{
		ASE_AWK_ASSERT (run->awk, exp->left->next == ASE_NULL);
		left = __eval_expression (run, exp->left);
		if (left == ASE_NULL) return ASE_NULL;

		ase_awk_refupval (left);

		ASE_AWK_ASSERT (run->awk, exp->right->next == ASE_NULL);
		right = __eval_expression (run, exp->right);
		if (right == ASE_NULL) 
		{
			ase_awk_refdownval (run, left);
			return ASE_NULL;
		}

		ase_awk_refupval (right);

		ASE_AWK_ASSERT (run->awk, exp->opcode >= 0 && 
			exp->opcode < ase_countof(__binop_func));
		ASE_AWK_ASSERT (run->awk, __binop_func[exp->opcode] != ASE_NULL);

		res = __binop_func[exp->opcode] (run, left, right);

		ase_awk_refdownval (run, left);
		ase_awk_refdownval (run, right);
	}

	return res;
}

static ase_awk_val_t* __eval_binop_lor (
	ase_awk_run_t* run, ase_awk_nde_t* left, ase_awk_nde_t* right)
{
	/*
	ase_awk_val_t* res = ASE_NULL;

	res = ase_awk_makeintval (run, 
		ase_awk_valtobool(run left) || ase_awk_valtobool(run,right));
	if (res == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);

	return res;
	*/

	/* short-circuit evaluation required special treatment */
	ase_awk_val_t* lv, * rv, * res;

	ASE_AWK_ASSERT (run->awk, left->next == ASE_NULL);
	lv = __eval_expression (run, left);
	if (lv == ASE_NULL) return ASE_NULL;

	ase_awk_refupval (lv);
	if (ase_awk_valtobool (run, lv)) 
	{
		/*res = ase_awk_makeintval (run, 1);*/
		res = ase_awk_val_one;
	}
	else
	{
		ASE_AWK_ASSERT (run->awk, right->next == ASE_NULL);
		rv = __eval_expression (run, right);
		if (rv == ASE_NULL)
		{
			ase_awk_refdownval (run, lv);
			return ASE_NULL;
		}
		ase_awk_refupval (rv);

		/*res = ase_awk_makeintval (run, (ase_awk_valtobool(run,rv)? 1: 0));*/
		res = ase_awk_valtobool(run,rv)? ase_awk_val_one: ase_awk_val_zero;
		ase_awk_refdownval (run, rv);
	}

	ase_awk_refdownval (run, lv);

	/*if (res == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);*/
	return res;
}

static ase_awk_val_t* __eval_binop_land (
	ase_awk_run_t* run, ase_awk_nde_t* left, ase_awk_nde_t* right)
{
	/*
	ase_awk_val_t* res = ASE_NULL;

	res = ase_awk_makeintval (run, 
		ase_awk_valtobool(run,left) && ase_awk_valtobool(run,right));
	if (res == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);

	return res;
	*/

	/* short-circuit evaluation required special treatment */
	ase_awk_val_t* lv, * rv, * res;

	ASE_AWK_ASSERT (run->awk, left->next == ASE_NULL);
	lv = __eval_expression (run, left);
	if (lv == ASE_NULL) return ASE_NULL;

	ase_awk_refupval (lv);
	if (!ase_awk_valtobool (run, lv)) 
	{
		/*res = ase_awk_makeintval (run, 0);*/
		res = ase_awk_val_zero;
	}
	else
	{
		ASE_AWK_ASSERT (run->awk, right->next == ASE_NULL);
		rv = __eval_expression (run, right);
		if (rv == ASE_NULL)
		{
			ase_awk_refdownval (run, lv);
			return ASE_NULL;
		}
		ase_awk_refupval (rv);

		/*res = ase_awk_makeintval (run, (ase_awk_valtobool(run,rv)? 1: 0));*/
		res = ase_awk_valtobool(run,rv)? ase_awk_val_one: ase_awk_val_zero;
		ase_awk_refdownval (run, rv);
	}

	ase_awk_refdownval (run, lv);

	/*if (res == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);*/
	return res;
}

static ase_awk_val_t* __eval_binop_in (
	ase_awk_run_t* run, ase_awk_nde_t* left, ase_awk_nde_t* right)
{
	ase_awk_val_t* rv;
	ase_char_t* str;
	ase_size_t len;

	if (right->type != ASE_AWK_NDE_GLOBAL &&
	    right->type != ASE_AWK_NDE_LOCAL &&
	    right->type != ASE_AWK_NDE_ARG &&
	    right->type != ASE_AWK_NDE_NAMED)
	{
		/* the compiler should have handled this case */
		ASE_AWK_ASSERT (run->awk, !"should never happen - in needs a plain variable");
		PANIC (run, ASE_AWK_EINTERNAL);
		return ASE_NULL;
	}

	/* evaluate the left-hand side of the operator */
	str = (left->type == ASE_AWK_NDE_GRP)?
		__idxnde_to_str (run, ((ase_awk_nde_grp_t*)left)->body, &len):
		__idxnde_to_str (run, left, &len);
	if (str == ASE_NULL) return ASE_NULL;

	/* evaluate the right-hand side of the operator */
	ASE_AWK_ASSERT (run->awk, right->next == ASE_NULL);
	rv = __eval_expression (run, right);
	if (rv == ASE_NULL) 
	{
		ASE_AWK_FREE (run->awk, str);
		return ASE_NULL;
	}

	ase_awk_refupval (rv);

	if (rv->type == ASE_AWK_VAL_NIL)
	{
		ASE_AWK_FREE (run->awk, str);
		ase_awk_refdownval (run, rv);
		return ase_awk_val_zero;
	}
	else if (rv->type == ASE_AWK_VAL_MAP)
	{
		ase_awk_val_t* res;
		ase_awk_map_t* map;

		map = ((ase_awk_val_map_t*)rv)->map;

		/*r = (ase_long_t)(ase_awk_map_get (map, str, len) != ASE_NULL);
		res = ase_awk_makeintval (run, r);
		if (res == ASE_NULL) 
		{
			ASE_AWK_FREE (run->awk, str);
			ase_awk_refdownval (run, rv);
			PANIC (run, ASE_AWK_ENOMEM);
		}*/
		res = (ase_awk_map_get (map, str, len) == ASE_NULL)? 
			ase_awk_val_zero: ase_awk_val_one;

		ASE_AWK_FREE (run->awk, str);
		ase_awk_refdownval (run, rv);
		return res;
	}

	/* need an array */
	/* TODO: change the error code to make it clearer */
	PANIC (run, ASE_AWK_EOPERAND); 
	return ASE_NULL;
}

static ase_awk_val_t* __eval_binop_bor (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	ase_awk_val_t* res = ASE_NULL;

	if (left->type == ASE_AWK_VAL_INT &&
	    right->type == ASE_AWK_VAL_INT)
	{
		ase_long_t r = 
			((ase_awk_val_int_t*)left)->val | 
			((ase_awk_val_int_t*)right)->val;
		res = ase_awk_makeintval (run, r);
	}
	else
	{
		PANIC (run, ASE_AWK_EOPERAND);
	}

	if (res == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);
	return res;
}

static ase_awk_val_t* __eval_binop_bxor (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	ase_awk_val_t* res = ASE_NULL;

	if (left->type == ASE_AWK_VAL_INT &&
	    right->type == ASE_AWK_VAL_INT)
	{
		ase_long_t r = 
			((ase_awk_val_int_t*)left)->val ^ 
			((ase_awk_val_int_t*)right)->val;
		res = ase_awk_makeintval (run, r);
	}
	else
	{
		PANIC (run, ASE_AWK_EOPERAND);
	}

	if (res == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);
	return res;
}

static ase_awk_val_t* __eval_binop_band (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	ase_awk_val_t* res = ASE_NULL;

	if (left->type == ASE_AWK_VAL_INT &&
	    right->type == ASE_AWK_VAL_INT)
	{
		ase_long_t r = 
			((ase_awk_val_int_t*)left)->val &
			((ase_awk_val_int_t*)right)->val;
		res = ase_awk_makeintval (run, r);
	}
	else
	{
		PANIC (run, ASE_AWK_EOPERAND);
	}

	if (res == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);
	return res;
}

static int __cmp_nil_nil (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	return 0;
}

static int __cmp_nil_int (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	if (((ase_awk_val_int_t*)right)->val < 0) return 1;
	if (((ase_awk_val_int_t*)right)->val > 0) return -1;
	return 0;
}

static int __cmp_nil_real (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	if (((ase_awk_val_real_t*)right)->val < 0) return 1;
	if (((ase_awk_val_real_t*)right)->val > 0) return -1;
	return 0;
}

static int __cmp_nil_str (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	return (((ase_awk_val_str_t*)right)->len == 0)? 0: -1;
}

static int __cmp_int_nil (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	if (((ase_awk_val_int_t*)left)->val > 0) return 1;
	if (((ase_awk_val_int_t*)left)->val < 0) return -1;
	return 0;
}

static int __cmp_int_int (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	if (((ase_awk_val_int_t*)left)->val > 
	    ((ase_awk_val_int_t*)right)->val) return 1;
	if (((ase_awk_val_int_t*)left)->val < 
	    ((ase_awk_val_int_t*)right)->val) return -1;
	return 0;
}

static int __cmp_int_real (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	if (((ase_awk_val_int_t*)left)->val > 
	    ((ase_awk_val_real_t*)right)->val) return 1;
	if (((ase_awk_val_int_t*)left)->val < 
	    ((ase_awk_val_real_t*)right)->val) return -1;
	return 0;
}

static int __cmp_int_str (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	ase_char_t* str;
	ase_size_t len;
	ase_long_t r;
	ase_real_t rr;
	int n;

	r = ase_awk_strxtolong (run->awk, 
		((ase_awk_val_str_t*)right)->buf,
		((ase_awk_val_str_t*)right)->len, 0, (const ase_char_t**)&str);
	if (str == ((ase_awk_val_str_t*)right)->buf + 
		   ((ase_awk_val_str_t*)right)->len)
	{
		if (((ase_awk_val_int_t*)left)->val > r) return 1;
		if (((ase_awk_val_int_t*)left)->val < r) return -1;
		return 0;
	}
/* TODO: should i do this???  conversion to real and comparision... */
	else if (*str == ASE_T('.') || *str == ASE_T('E') || *str == ASE_T('e'))
	{
		rr = ase_awk_strxtoreal (run->awk,
			((ase_awk_val_str_t*)right)->buf,
			((ase_awk_val_str_t*)right)->len, 
			(const ase_char_t**)&str);
		if (str == ((ase_awk_val_str_t*)right)->buf + 
			   ((ase_awk_val_str_t*)right)->len)
		{
			if (((ase_awk_val_int_t*)left)->val > rr) return 1;
			if (((ase_awk_val_int_t*)left)->val < rr) return -1;
			return 0;
		}
	}

	str = ase_awk_valtostr (run, left, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &len);
	if (str == ASE_NULL)
	{
		run->errnum = ASE_AWK_ENOMEM;
		return CMP_ERROR;
	}

	if (run->global.ignorecase)
	{
		n = ase_awk_strxncasecmp (
			run->awk, str, len,
			((ase_awk_val_str_t*)right)->buf, 
			((ase_awk_val_str_t*)right)->len);
	}
	else
	{
		n = ase_awk_strxncmp (
			str, len,
			((ase_awk_val_str_t*)right)->buf, 
			((ase_awk_val_str_t*)right)->len);
	}

	ASE_AWK_FREE (run->awk, str);
	return n;
}

static int __cmp_real_nil (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	if (((ase_awk_val_real_t*)left)->val > 0) return 1;
	if (((ase_awk_val_real_t*)left)->val < 0) return -1;
	return 0;
}

static int __cmp_real_int (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	if (((ase_awk_val_real_t*)left)->val > 
	    ((ase_awk_val_int_t*)right)->val) return 1;
	if (((ase_awk_val_real_t*)left)->val < 
	    ((ase_awk_val_int_t*)right)->val) return -1;
	return 0;
}

static int __cmp_real_real (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	if (((ase_awk_val_real_t*)left)->val > 
	    ((ase_awk_val_real_t*)right)->val) return 1;
	if (((ase_awk_val_real_t*)left)->val < 
	    ((ase_awk_val_real_t*)right)->val) return -1;
	return 0;
}

static int __cmp_real_str (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	ase_char_t* str;
	ase_size_t len;
	ase_real_t rr;
	int n;

	rr = ase_awk_strxtoreal (run->awk,
		((ase_awk_val_str_t*)right)->buf,
		((ase_awk_val_str_t*)right)->len, 
		(const ase_char_t**)&str);
	if (str == ((ase_awk_val_str_t*)right)->buf + 
		   ((ase_awk_val_str_t*)right)->len)
	{
		if (((ase_awk_val_real_t*)left)->val > rr) return 1;
		if (((ase_awk_val_real_t*)left)->val < rr) return -1;
		return 0;
	}

	str = ase_awk_valtostr (run, left, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &len);
	if (str == ASE_NULL)
	{
		run->errnum = ASE_AWK_ENOMEM;
		return CMP_ERROR;
	}

	if (run->global.ignorecase)
	{
		n = ase_awk_strxncasecmp (
			run->awk, str, len,
			((ase_awk_val_str_t*)right)->buf, 
			((ase_awk_val_str_t*)right)->len);
	}
	else
	{
		n = ase_awk_strxncmp (
			str, len,
			((ase_awk_val_str_t*)right)->buf, 
			((ase_awk_val_str_t*)right)->len);
	}

	ASE_AWK_FREE (run->awk, str);
	return n;
}

static int __cmp_str_nil (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	return (((ase_awk_val_str_t*)left)->len == 0)? 0: 1;
}

static int __cmp_str_int (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	return -__cmp_int_str (run, right, left);
}

static int __cmp_str_real (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	return -__cmp_real_str (run, right, left);
}

static int __cmp_str_str (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	int n;
	ase_awk_val_str_t* ls, * rs;

	ls = (ase_awk_val_str_t*)left;
	rs = (ase_awk_val_str_t*)right;

	if (run->global.ignorecase)
	{
		n = ase_awk_strxncasecmp (run->awk, 
			ls->buf, ls->len, rs->buf, rs->len);
	}
	else
	{
		n = ase_awk_strxncmp (
			ls->buf, ls->len, rs->buf, rs->len);
	}

	return n;
}

static int __cmp_val (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	typedef int (*cmp_val_t) (ase_awk_run_t*, ase_awk_val_t*, ase_awk_val_t*);

	static cmp_val_t func[] =
	{
		/* this table must be synchronized with 
		 * the ASE_AWK_VAL_XXX values in val.h */
		__cmp_nil_nil,  __cmp_nil_int,  __cmp_nil_real,  __cmp_nil_str,
		__cmp_int_nil,  __cmp_int_int,  __cmp_int_real,  __cmp_int_str,
		__cmp_real_nil, __cmp_real_int, __cmp_real_real, __cmp_real_str,
		__cmp_str_nil,  __cmp_str_int,  __cmp_str_real,  __cmp_str_str,
	};

	if (left->type == ASE_AWK_VAL_MAP || right->type == ASE_AWK_VAL_MAP)
	{
		/* a map can't be compared againt other values */
		run->errnum = ASE_AWK_EOPERAND;
		return CMP_ERROR; 
	}

	ASE_AWK_ASSERT (run->awk, 
		left->type >= ASE_AWK_VAL_NIL &&
		left->type <= ASE_AWK_VAL_STR);
	ASE_AWK_ASSERT (run->awk, 
		right->type >= ASE_AWK_VAL_NIL &&
		right->type <= ASE_AWK_VAL_STR);

	return func[left->type*4+right->type] (run, left, right);
}

static ase_awk_val_t* __eval_binop_eq (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	int n = __cmp_val (run, left, right);
	if (n == CMP_ERROR) return ASE_NULL;
	return (n == 0)? ase_awk_val_one: ase_awk_val_zero;
}

static ase_awk_val_t* __eval_binop_ne (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	int n = __cmp_val (run, left, right);
	if (n == CMP_ERROR) return ASE_NULL;
	return (n != 0)? ase_awk_val_one: ase_awk_val_zero;
}

static ase_awk_val_t* __eval_binop_gt (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	int n = __cmp_val (run, left, right);
	if (n == CMP_ERROR) return ASE_NULL;
	return (n > 0)? ase_awk_val_one: ase_awk_val_zero;
}

static ase_awk_val_t* __eval_binop_ge (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	int n = __cmp_val (run, left, right);
	if (n == CMP_ERROR) return ASE_NULL;
	return (n >= 0)? ase_awk_val_one: ase_awk_val_zero;
}

static ase_awk_val_t* __eval_binop_lt (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	int n = __cmp_val (run, left, right);
	if (n == CMP_ERROR) return ASE_NULL;
	return (n < 0)? ase_awk_val_one: ase_awk_val_zero;
}

static ase_awk_val_t* __eval_binop_le (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	int n = __cmp_val (run, left, right);
	if (n == CMP_ERROR) return ASE_NULL;
	return (n <= 0)? ase_awk_val_one: ase_awk_val_zero;
}

static ase_awk_val_t* __eval_binop_lshift (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	int n1, n2, n3;
	ase_long_t l1, l2;
	ase_real_t r1, r2;
	ase_awk_val_t* res;

	n1 = ase_awk_valtonum (run, left, &l1, &r1);
	n2 = ase_awk_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (run, ASE_AWK_EOPERAND); 

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		if  (l2 == 0) PANIC (run, ASE_AWK_EDIVBYZERO);
		res = ase_awk_makeintval (run, (ase_long_t)l1 << (ase_long_t)l2);
	}
	else PANIC (run, ASE_AWK_EOPERAND);

	if (res == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);
	return res;
}

static ase_awk_val_t* __eval_binop_rshift (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	int n1, n2, n3;
	ase_long_t l1, l2;
	ase_real_t r1, r2;
	ase_awk_val_t* res;

	n1 = ase_awk_valtonum (run, left, &l1, &r1);
	n2 = ase_awk_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (run, ASE_AWK_EOPERAND); 

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		if  (l2 == 0) PANIC (run, ASE_AWK_EDIVBYZERO);
		res = ase_awk_makeintval (run, (ase_long_t)l1 >> (ase_long_t)l2);
	}
	else PANIC (run, ASE_AWK_EOPERAND);

	if (res == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);
	return res;
}

static ase_awk_val_t* __eval_binop_plus (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	int n1, n2, n3;
	ase_long_t l1, l2;
	ase_real_t r1, r2;
	ase_awk_val_t* res;

	n1 = ase_awk_valtonum (run, left, &l1, &r1);
	n2 = ase_awk_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (run, ASE_AWK_EOPERAND); 
	/*
	n1  n2    n3
	0   0   = 0
	1   0   = 1
	0   1   = 2
	1   1   = 3
	*/
	n3 = n1 + (n2 << 1);
	ASE_AWK_ASSERT (run->awk, n3 >= 0 && n3 <= 3);
	res = (n3 == 0)? ase_awk_makeintval(run,(ase_long_t)l1+(ase_long_t)l2):
	      (n3 == 1)? ase_awk_makerealval(run,(ase_real_t)r1+(ase_real_t)l2):
	      (n3 == 2)? ase_awk_makerealval(run,(ase_real_t)l1+(ase_real_t)r2):
	                 ase_awk_makerealval(run,(ase_real_t)r1+(ase_real_t)r2);

	if (res == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);
	return res;
}

static ase_awk_val_t* __eval_binop_minus (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	int n1, n2, n3;
	ase_long_t l1, l2;
	ase_real_t r1, r2;
	ase_awk_val_t* res;

	n1 = ase_awk_valtonum (run, left, &l1, &r1);
	n2 = ase_awk_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (run, ASE_AWK_EOPERAND);

	n3 = n1 + (n2 << 1);
	ASE_AWK_ASSERT (run->awk, n3 >= 0 && n3 <= 3);
	res = (n3 == 0)? ase_awk_makeintval(run,(ase_long_t)l1-(ase_long_t)l2):
	      (n3 == 1)? ase_awk_makerealval(run,(ase_real_t)r1-(ase_real_t)l2):
	      (n3 == 2)? ase_awk_makerealval(run,(ase_real_t)l1-(ase_real_t)r2):
	                 ase_awk_makerealval(run,(ase_real_t)r1-(ase_real_t)r2);

	if (res == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);
	return res;
}

static ase_awk_val_t* __eval_binop_mul (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	int n1, n2, n3;
	ase_long_t l1, l2;
	ase_real_t r1, r2;
	ase_awk_val_t* res;

	n1 = ase_awk_valtonum (run, left, &l1, &r1);
	n2 = ase_awk_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (run, ASE_AWK_EOPERAND);

	n3 = n1 + (n2 << 1);
	ASE_AWK_ASSERT (run->awk, n3 >= 0 && n3 <= 3);
	res = (n3 == 0)? ase_awk_makeintval(run,(ase_long_t)l1*(ase_long_t)l2):
	      (n3 == 1)? ase_awk_makerealval(run,(ase_real_t)r1*(ase_real_t)l2):
	      (n3 == 2)? ase_awk_makerealval(run,(ase_real_t)l1*(ase_real_t)r2):
	                 ase_awk_makerealval(run,(ase_real_t)r1*(ase_real_t)r2);

	if (res == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);
	return res;
}

static ase_awk_val_t* __eval_binop_div (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	int n1, n2, n3;
	ase_long_t l1, l2;
	ase_real_t r1, r2;
	ase_awk_val_t* res;

	n1 = ase_awk_valtonum (run, left, &l1, &r1);
	n2 = ase_awk_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (run, ASE_AWK_EOPERAND);

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		if  (l2 == 0) PANIC (run, ASE_AWK_EDIVBYZERO);
		res = ase_awk_makeintval (run, (ase_long_t)l1 / (ase_long_t)l2);
	}
	else if (n3 == 1)
	{
		res = ase_awk_makerealval (run, (ase_real_t)r1 / (ase_real_t)l2);
	}
	else if (n3 == 2)
	{
		res = ase_awk_makerealval (run, (ase_real_t)l1 / (ase_real_t)r2);
	}
	else
	{
		ASE_AWK_ASSERT (run->awk, n3 == 3);
		res = ase_awk_makerealval (run, (ase_real_t)r1 / (ase_real_t)r2);
	}

	if (res == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);
	return res;
}

static ase_awk_val_t* __eval_binop_mod (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	int n1, n2, n3;
	ase_long_t l1, l2;
	ase_real_t r1, r2;
	ase_awk_val_t* res;

	n1 = ase_awk_valtonum (run, left, &l1, &r1);
	n2 = ase_awk_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (run, ASE_AWK_EOPERAND);

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		if  (l2 == 0) PANIC (run, ASE_AWK_EDIVBYZERO);
		res = ase_awk_makeintval (run, (ase_long_t)l1 % (ase_long_t)l2);
	}
	else PANIC (run, ASE_AWK_EOPERAND);

	if (res == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);
	return res;
}

static ase_awk_val_t* __eval_binop_exp (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	int n1, n2, n3;
	ase_long_t l1, l2;
	ase_real_t r1, r2;
	ase_awk_val_t* res;

	n1 = ase_awk_valtonum (run, left, &l1, &r1);
	n2 = ase_awk_valtonum (run, right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (run, ASE_AWK_EOPERAND);

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		ase_long_t v = 1;
		while (l2-- > 0) v *= l1;
		res = ase_awk_makeintval (run, v);
	}
	else if (n3 == 1)
	{
		/*res = ase_awk_makerealval (
			run, pow((ase_real_t)r1,(ase_real_t)l2));*/
		ase_real_t v = 1.0;
		while (l2-- > 0) v *= r1;
		res = ase_awk_makerealval (run, v);
	}
	else if (n3 == 2)
	{
		res = ase_awk_makerealval (
			run, pow((ase_real_t)l1,(ase_real_t)r2));
	}
	else
	{
		ASE_AWK_ASSERT (run->awk, n3 == 3);
		res = ase_awk_makerealval (
			run, pow((ase_real_t)r1,(ase_real_t)r2));
	}

	if (res == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);
	return res;
}

static ase_awk_val_t* __eval_binop_concat (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right)
{
	ase_char_t* strl, * strr;
	ase_size_t strl_len, strr_len;
	ase_awk_val_t* res;

	strl = ase_awk_valtostr (
		run, left, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &strl_len);
	if (strl == ASE_NULL) return ASE_NULL;

	strr = ase_awk_valtostr (
		run, right, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &strr_len);
	if (strr == ASE_NULL) 
	{
		ASE_AWK_FREE (run->awk, strl);
		return ASE_NULL;
	}

	res = ase_awk_makestrval2 (run, strl, strl_len, strr, strr_len);
	if (res == ASE_NULL)
	{
		ASE_AWK_FREE (run->awk, strl);
		ASE_AWK_FREE (run->awk, strr);
		PANIC (run, ASE_AWK_ENOMEM);
	}

	ASE_AWK_FREE (run->awk, strl);
	ASE_AWK_FREE (run->awk, strr);

	return res;
}

static ase_awk_val_t* __eval_binop_ma (
	ase_awk_run_t* run, ase_awk_nde_t* left, ase_awk_nde_t* right)
{
	ase_awk_val_t* lv, * rv, * res;

	ASE_AWK_ASSERT (run->awk, left->next == ASE_NULL);
	ASE_AWK_ASSERT (run->awk, right->next == ASE_NULL);

	lv = __eval_expression (run, left);
	if (lv == ASE_NULL) 
	{
		return ASE_NULL;
	}

	ase_awk_refupval (lv);

	rv = __eval_expression0 (run, right);
	if (rv == ASE_NULL)
	{
		ase_awk_refdownval (run, lv);
		return ASE_NULL;
	}

	ase_awk_refupval (rv);

	res = __eval_binop_match0 (run, lv, rv, 1);

	ase_awk_refdownval (run, lv);
	ase_awk_refdownval (run, rv);

	return res;
}

static ase_awk_val_t* __eval_binop_nm (
	ase_awk_run_t* run, ase_awk_nde_t* left, ase_awk_nde_t* right)
{
	ase_awk_val_t* lv, * rv, * res;

	ASE_AWK_ASSERT (run->awk, left->next == ASE_NULL);
	ASE_AWK_ASSERT (run->awk, right->next == ASE_NULL);

	lv = __eval_expression (run, left);
	if (lv == ASE_NULL) return ASE_NULL;

	ase_awk_refupval (lv);

	rv = __eval_expression0 (run, right);
	if (rv == ASE_NULL)
	{
		ase_awk_refdownval (run, lv);
		return ASE_NULL;
	}

	ase_awk_refupval (rv);

	res = __eval_binop_match0 (run, lv, rv, 0);

	ase_awk_refdownval (run, lv);
	ase_awk_refdownval (run, rv);

	return res;
}

static ase_awk_val_t* __eval_binop_match0 (
	ase_awk_run_t* run, ase_awk_val_t* left, ase_awk_val_t* right, int ret)
{
	ase_awk_val_t* res;
	int n, errnum;
	ase_char_t* str;
	ase_size_t len;
	void* rex_code;

	if (right->type == ASE_AWK_VAL_REX)
	{
		rex_code = ((ase_awk_val_rex_t*)right)->code;
	}
	else if (right->type == ASE_AWK_VAL_STR)
	{
		rex_code = ase_awk_buildrex ( 
			run->awk,
			((ase_awk_val_str_t*)right)->buf,
			((ase_awk_val_str_t*)right)->len, &errnum);
		if (rex_code == ASE_NULL)
			PANIC (run, errnum);
	}
	else
	{
		str = ase_awk_valtostr (
			run, right, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &len);
		if (str == ASE_NULL) return ASE_NULL;

		rex_code = ase_awk_buildrex (run->awk, str, len, &errnum);
		if (rex_code == ASE_NULL)
		{
			ASE_AWK_FREE (run->awk, str);
			PANIC (run, errnum);
		}

		ASE_AWK_FREE (run->awk, str);
	}

	if (left->type == ASE_AWK_VAL_STR)
	{
		n = ase_awk_matchrex (
			run->awk, rex_code,
			((run->global.ignorecase)? ASE_AWK_REX_IGNORECASE: 0),
			((ase_awk_val_str_t*)left)->buf,
			((ase_awk_val_str_t*)left)->len,
			ASE_NULL, ASE_NULL, &errnum);
		if (n == -1) 
		{
			if (right->type != ASE_AWK_VAL_REX) 
				ASE_AWK_FREE (run->awk, rex_code);
			PANIC (run, errnum);
		}

		res = ase_awk_makeintval (run, (n == ret));
		if (res == ASE_NULL) 
		{
			if (right->type != ASE_AWK_VAL_REX) 
				ASE_AWK_FREE (run->awk, rex_code);
			PANIC (run, ASE_AWK_ENOMEM);
		}
	}
	else
	{
		str = ase_awk_valtostr (
			run, left, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &len);
		if (str == ASE_NULL) 
		{
			if (right->type != ASE_AWK_VAL_REX) 
				ASE_AWK_FREE (run->awk, rex_code);
			return ASE_NULL;
		}

		n = ase_awk_matchrex (
			run->awk, rex_code, 
			((run->global.ignorecase)? ASE_AWK_REX_IGNORECASE: 0),
			str, len, ASE_NULL, ASE_NULL, &errnum);
		if (n == -1) 
		{
			ASE_AWK_FREE (run->awk, str);
			if (right->type != ASE_AWK_VAL_REX) 
				ASE_AWK_FREE (run->awk, rex_code);
			PANIC (run, errnum);
		}

		res = ase_awk_makeintval (run, (n == ret));
		if (res == ASE_NULL) 
		{
			ASE_AWK_FREE (run->awk, str);
			if (right->type != ASE_AWK_VAL_REX) 
				ASE_AWK_FREE (run->awk, rex_code);
			PANIC (run, ASE_AWK_ENOMEM);
		}

		ASE_AWK_FREE (run->awk, str);
	}

	if (right->type != ASE_AWK_VAL_REX) ASE_AWK_FREE (run->awk, rex_code);
	return res;
}

static ase_awk_val_t* __eval_unary (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	ase_awk_val_t* left, * res = ASE_NULL;
	ase_awk_nde_exp_t* exp = (ase_awk_nde_exp_t*)nde;

	ASE_AWK_ASSERT (run->awk, exp->type == ASE_AWK_NDE_EXP_UNR);
	ASE_AWK_ASSERT (run->awk, exp->left != ASE_NULL && exp->right == ASE_NULL);

	ASE_AWK_ASSERT (run->awk, exp->left->next == ASE_NULL);
	left = __eval_expression (run, exp->left);
	if (left == ASE_NULL) return ASE_NULL;

	ase_awk_refupval (left);

	if (exp->opcode == ASE_AWK_UNROP_PLUS) 
	{
		if (left->type == ASE_AWK_VAL_INT)
		{
			ase_long_t r = ((ase_awk_val_int_t*)left)->val;
			res = ase_awk_makeintval (run, r);
		}
		else if (left->type == ASE_AWK_VAL_REAL)
		{
			ase_real_t r = ((ase_awk_val_real_t*)left)->val;
			res = ase_awk_makerealval (run, r);
		}
		else
		{
			ase_awk_refdownval (run, left);
			PANIC (run, ASE_AWK_EOPERAND);
		}
	}
	else if (exp->opcode == ASE_AWK_UNROP_MINUS)
	{
		if (left->type == ASE_AWK_VAL_INT)
		{
			ase_long_t r = ((ase_awk_val_int_t*)left)->val;
			res = ase_awk_makeintval (run, -r);
		}
		else if (left->type == ASE_AWK_VAL_REAL)
		{
			ase_real_t r = ((ase_awk_val_real_t*)left)->val;
			res = ase_awk_makerealval (run, -r);
		}
		else
		{
			ase_awk_refdownval (run, left);
			PANIC (run, ASE_AWK_EOPERAND);
		}
	}
	else if (exp->opcode == ASE_AWK_UNROP_NOT)
	{
		if (left->type == ASE_AWK_VAL_INT)
		{
			ase_long_t r = ((ase_awk_val_int_t*)left)->val;
			res = ase_awk_makeintval (run, !r);
		}
		else if (left->type == ASE_AWK_VAL_REAL)
		{
			ase_real_t r = ((ase_awk_val_real_t*)left)->val;
			res = ase_awk_makerealval (run, !r);
		}
		else
		{
			ase_awk_refdownval (run, left);
			PANIC (run, ASE_AWK_EOPERAND);
		}
	}
	else if (exp->opcode == ASE_AWK_UNROP_BNOT)
	{
		if (left->type == ASE_AWK_VAL_INT)
		{
			ase_long_t r = ((ase_awk_val_int_t*)left)->val;
			res = ase_awk_makeintval (run, ~r);
		}
		else
		{
			ase_awk_refdownval (run, left);
			PANIC (run, ASE_AWK_EOPERAND);
		}
	}

	if (res == ASE_NULL)
	{
		ase_awk_refdownval (run, left);
		PANIC (run, ASE_AWK_ENOMEM);
	}

	ase_awk_refdownval (run, left);
	return res;
}

static ase_awk_val_t* __eval_incpre (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	ase_awk_val_t* left, * res;
	ase_awk_nde_exp_t* exp = (ase_awk_nde_exp_t*)nde;

	ASE_AWK_ASSERT (run->awk, exp->type == ASE_AWK_NDE_EXP_INCPRE);
	ASE_AWK_ASSERT (run->awk, exp->left != ASE_NULL && exp->right == ASE_NULL);

	/* this way of checking if the l-value is assignable is
	 * ugly as it is dependent of the values defined in tree.h.
	 * but let's keep going this way for the time being. */
	if (exp->left->type < ASE_AWK_NDE_NAMED ||
	    exp->left->type > ASE_AWK_NDE_ARGIDX)
	{
		PANIC (run, ASE_AWK_EOPERAND);
	}

	ASE_AWK_ASSERT (run->awk, exp->left->next == ASE_NULL);
	left = __eval_expression (run, exp->left);
	if (left == ASE_NULL) return ASE_NULL;

	ase_awk_refupval (left);

	if (exp->opcode == ASE_AWK_INCOP_PLUS) 
	{
		if (left->type == ASE_AWK_VAL_INT)
		{
			ase_long_t r = ((ase_awk_val_int_t*)left)->val;
			res = ase_awk_makeintval (run, r + 1);
			if (res == ASE_NULL) 
			{
				ase_awk_refdownval (run, left);
				PANIC (run, ASE_AWK_ENOMEM);
			}
		}
		else if (left->type == ASE_AWK_VAL_REAL)
		{
			ase_real_t r = ((ase_awk_val_real_t*)left)->val;
			res = ase_awk_makerealval (run, r + 1.0);
			if (res == ASE_NULL) 
			{
				ase_awk_refdownval (run, left);
				PANIC (run, ASE_AWK_ENOMEM);
			}
		}
		else
		{
			ase_long_t v1;
			ase_real_t v2;
			int n;

			n = ase_awk_valtonum (run, left, &v1, &v2);
			if (n == -1)
			{
				ase_awk_refdownval (run, left);
				PANIC (run, ASE_AWK_EOPERAND);
			}

			if (n == 0) 
			{
				res = ase_awk_makeintval (run, v1 + 1);
			}
			else /* if (n == 1) */
			{
				ASE_AWK_ASSERT (run->awk, n == 1);
				res = ase_awk_makerealval (run, v2 + 1.0);
			}

			if (res == ASE_NULL) 
			{
				ase_awk_refdownval (run, left);
				PANIC (run, ASE_AWK_ENOMEM);
			}
		}
	}
	else if (exp->opcode == ASE_AWK_INCOP_MINUS)
	{
		if (left->type == ASE_AWK_VAL_INT)
		{
			ase_long_t r = ((ase_awk_val_int_t*)left)->val;
			res = ase_awk_makeintval (run, r - 1);
			if (res == ASE_NULL) 
			{
				ase_awk_refdownval (run, left);
				PANIC (run, ASE_AWK_ENOMEM);
			}
		}
		else if (left->type == ASE_AWK_VAL_REAL)
		{
			ase_real_t r = ((ase_awk_val_real_t*)left)->val;
			res = ase_awk_makerealval (run, r - 1.0);
			if (res == ASE_NULL) 
			{
				ase_awk_refdownval (run, left);
				PANIC (run, ASE_AWK_ENOMEM);
			}
		}
		else
		{
			ase_long_t v1;
			ase_real_t v2;
			int n;

			n = ase_awk_valtonum (run, left, &v1, &v2);
			if (n == -1)
			{
				ase_awk_refdownval (run, left);
				PANIC (run, ASE_AWK_EOPERAND);
			}

			if (n == 0) 
			{
				res = ase_awk_makeintval (run, v1 - 1);
			}
			else /* if (n == 1) */
			{
				ASE_AWK_ASSERT (run->awk, n == 1);
				res = ase_awk_makerealval (run, v2 - 1.0);
			}

			if (res == ASE_NULL) 
			{
				ase_awk_refdownval (run, left);
				PANIC (run, ASE_AWK_ENOMEM);
			}
		}
	}
	else
	{
		ASE_AWK_ASSERT (run->awk, !"should never happen - invalid opcode");
		ase_awk_refdownval (run, left);
		PANIC (run, ASE_AWK_EINTERNAL);
	}

	if (__do_assignment (run, exp->left, res) == ASE_NULL)
	{
		ase_awk_refdownval (run, left);
		return ASE_NULL;
	}

	ase_awk_refdownval (run, left);
	return res;
}

static ase_awk_val_t* __eval_incpst (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	ase_awk_val_t* left, * res, * res2;
	ase_awk_nde_exp_t* exp = (ase_awk_nde_exp_t*)nde;

	ASE_AWK_ASSERT (run->awk, exp->type == ASE_AWK_NDE_EXP_INCPST);
	ASE_AWK_ASSERT (run->awk, exp->left != ASE_NULL && exp->right == ASE_NULL);

	/* this way of checking if the l-value is assignable is
	 * ugly as it is dependent of the values defined in tree.h.
	 * but let's keep going this way for the time being. */
	if (exp->left->type < ASE_AWK_NDE_NAMED ||
	    exp->left->type > ASE_AWK_NDE_ARGIDX)
	{
		PANIC (run, ASE_AWK_EOPERAND);
	}

	ASE_AWK_ASSERT (run->awk, exp->left->next == ASE_NULL);
	left = __eval_expression (run, exp->left);
	if (left == ASE_NULL) return ASE_NULL;

	ase_awk_refupval (left);

	if (exp->opcode == ASE_AWK_INCOP_PLUS) 
	{
		if (left->type == ASE_AWK_VAL_INT)
		{
			ase_long_t r = ((ase_awk_val_int_t*)left)->val;
			res = ase_awk_makeintval (run, r);
			if (res == ASE_NULL) 
			{
				ase_awk_refdownval (run, left);
				PANIC (run, ASE_AWK_ENOMEM);
			}

			res2 = ase_awk_makeintval (run, r + 1);
			if (res2 == ASE_NULL)
			{
				ase_awk_refdownval (run, left);
				ase_awk_freeval (run, res, ase_true);
				PANIC (run, ASE_AWK_ENOMEM);
			}
		}
		else if (left->type == ASE_AWK_VAL_REAL)
		{
			ase_real_t r = ((ase_awk_val_real_t*)left)->val;
			res = ase_awk_makerealval (run, r);
			if (res == ASE_NULL) 
			{
				ase_awk_refdownval (run, left);
				PANIC (run, ASE_AWK_ENOMEM);
			}

			res2 = ase_awk_makerealval (run, r + 1.0);
			if (res2 == ASE_NULL)
			{
				ase_awk_refdownval (run, left);
				ase_awk_freeval (run, res, ase_true);
				PANIC (run, ASE_AWK_ENOMEM);
			}
		}
		else
		{
			ase_long_t v1;
			ase_real_t v2;
			int n;

			n = ase_awk_valtonum (run, left, &v1, &v2);
			if (n == -1)
			{
				ase_awk_refdownval (run, left);
				PANIC (run, ASE_AWK_EOPERAND);
			}

			if (n == 0) 
			{
				res = ase_awk_makeintval (run, v1);
				if (res == ASE_NULL)
				{
					ase_awk_refdownval (run, left);
					PANIC (run, ASE_AWK_ENOMEM);
				}

				res2 = ase_awk_makeintval (run, v1 + 1);
				if (res2 == ASE_NULL)
				{
					ase_awk_refdownval (run, left);
					ase_awk_freeval (run, res, ase_true);
					PANIC (run, ASE_AWK_ENOMEM);
				}
			}
			else /* if (n == 1) */
			{
				ASE_AWK_ASSERT (run->awk, n == 1);
				res = ase_awk_makerealval (run, v2);
				if (res == ASE_NULL)
				{
					ase_awk_refdownval (run, left);
					PANIC (run, ASE_AWK_ENOMEM);
				}

				res2 = ase_awk_makerealval (run, v2 + 1.0);
				if (res2 == ASE_NULL)
				{
					ase_awk_refdownval (run, left);
					ase_awk_freeval (run, res, ase_true);
					PANIC (run, ASE_AWK_ENOMEM);
				}
			}
		}
	}
	else if (exp->opcode == ASE_AWK_INCOP_MINUS)
	{
		if (left->type == ASE_AWK_VAL_INT)
		{
			ase_long_t r = ((ase_awk_val_int_t*)left)->val;
			res = ase_awk_makeintval (run, r);
			if (res == ASE_NULL) 
			{
				ase_awk_refdownval (run, left);
				PANIC (run, ASE_AWK_ENOMEM);
			}

			res2 = ase_awk_makeintval (run, r - 1);
			if (res2 == ASE_NULL)
			{
				ase_awk_refdownval (run, left);
				ase_awk_freeval (run, res, ase_true);
				PANIC (run, ASE_AWK_ENOMEM);
			}
		}
		else if (left->type == ASE_AWK_VAL_REAL)
		{
			ase_real_t r = ((ase_awk_val_real_t*)left)->val;
			res = ase_awk_makerealval (run, r);
			if (res == ASE_NULL) 
			{
				ase_awk_refdownval (run, left);
				PANIC (run, ASE_AWK_ENOMEM);
			}

			res2 = ase_awk_makerealval (run, r - 1.0);
			if (res2 == ASE_NULL)
			{
				ase_awk_refdownval (run, left);
				ase_awk_freeval (run, res, ase_true);
				PANIC (run, ASE_AWK_ENOMEM);
			}
		}
		else
		{
			ase_long_t v1;
			ase_real_t v2;
			int n;

			n = ase_awk_valtonum (run, left, &v1, &v2);
			if (n == -1)
			{
				ase_awk_refdownval (run, left);
				PANIC (run, ASE_AWK_EOPERAND);
			}

			if (n == 0) 
			{
				res = ase_awk_makeintval (run, v1);
				if (res == ASE_NULL)
				{
					ase_awk_refdownval (run, left);
					PANIC (run, ASE_AWK_ENOMEM);
				}

				res2 = ase_awk_makeintval (run, v1 - 1);
				if (res2 == ASE_NULL)
				{
					ase_awk_refdownval (run, left);
					ase_awk_freeval (run, res, ase_true);
					PANIC (run, ASE_AWK_ENOMEM);
				}
			}
			else /* if (n == 1) */
			{
				ASE_AWK_ASSERT (run->awk, n == 1);
				res = ase_awk_makerealval (run, v2);
				if (res == ASE_NULL)
				{
					ase_awk_refdownval (run, left);
					PANIC (run, ASE_AWK_ENOMEM);
				}

				res2 = ase_awk_makerealval (run, v2 - 1.0);
				if (res2 == ASE_NULL)
				{
					ase_awk_refdownval (run, left);
					ase_awk_freeval (run, res, ase_true);
					PANIC (run, ASE_AWK_ENOMEM);
				}
			}
		}
	}
	else
	{
		ASE_AWK_ASSERT (run->awk, !"should never happen - invalid opcode");
		ase_awk_refdownval (run, left);
		PANIC (run, ASE_AWK_EINTERNAL);
	}

	if (__do_assignment (run, exp->left, res2) == ASE_NULL)
	{
		ase_awk_refdownval (run, left);
		return ASE_NULL;
	}

	ase_awk_refdownval (run, left);
	return res;
}

static ase_awk_val_t* __eval_cnd (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	ase_awk_val_t* tv, * v;
	ase_awk_nde_cnd_t* cnd = (ase_awk_nde_cnd_t*)nde;

	ASE_AWK_ASSERT (run->awk, cnd->test->next == ASE_NULL);
	tv = __eval_expression (run, cnd->test);
	if (tv == ASE_NULL) return ASE_NULL;

	ase_awk_refupval (tv);

	ASE_AWK_ASSERT (run->awk, cnd->left->next == ASE_NULL &&
	           cnd->right->next == ASE_NULL);
	v = (ase_awk_valtobool (run, tv))?
		__eval_expression (run, cnd->left):
		__eval_expression (run, cnd->right);

	ase_awk_refdownval (run, tv);
	return v;
}

static ase_awk_val_t* __eval_bfn (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	ase_awk_nde_call_t* call = (ase_awk_nde_call_t*)nde;

	/* built-in function */
	if (call->nargs < call->what.bfn.min_args)
	{
		PANIC (run, ASE_AWK_ETOOFEWARGS);
	}

	if (call->nargs > call->what.bfn.max_args)
	{
		PANIC (run, ASE_AWK_ETOOMANYARGS);
	}

	return __eval_call (run, nde, call->what.bfn.arg_spec, ASE_NULL);
}

static ase_awk_val_t* __eval_afn (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	ase_awk_nde_call_t* call = (ase_awk_nde_call_t*)nde;
	ase_awk_afn_t* afn;
	ase_awk_pair_t* pair;

	pair = ase_awk_map_get (&run->awk->tree.afns, 
		call->what.afn.name, call->what.afn.name_len);
	if (pair == ASE_NULL) PANIC (run, ASE_AWK_ENOSUCHFUNC);

	afn = (ase_awk_afn_t*)pair->val;
	ASE_AWK_ASSERT (run->awk, afn != ASE_NULL);

	if (call->nargs > afn->nargs)
	{
		/* TODO: is this correct? what if i want to allow arbitarary numbers of arguments? */
		PANIC (run, ASE_AWK_ETOOMANYARGS);
	}

	return __eval_call (run, nde, ASE_NULL, afn);
}


/* run->stack_base has not been set for this  
 * stack frame. so STACK_ARG cannot be used */ 
/*ase_awk_refdownval (run, STACK_ARG(run,nargs));*/ 
#define UNWIND_RUN_STACK(run,nargs) \
	do { \
		while ((nargs) > 0) \
		{ \
			--(nargs); \
			ase_awk_refdownval ((run), \
				(run)->stack[(run)->stack_top-1]); \
			__raw_pop (run); \
		} \
		__raw_pop (run); \
		__raw_pop (run); \
		__raw_pop (run); \
	} while (0)

static ase_awk_val_t* __eval_call (
	ase_awk_run_t* run, ase_awk_nde_t* nde, 
	const ase_char_t* bfn_arg_spec, ase_awk_afn_t* afn)
{
	ase_awk_nde_call_t* call = (ase_awk_nde_call_t*)nde;
	ase_size_t saved_stack_top;
	ase_size_t nargs, i;
	ase_awk_nde_t* p;
	ase_awk_val_t* v;
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

	ASE_AWK_ASSERT (run->awk, ase_sizeof(void*) >= ase_sizeof(run->stack_top));
	ASE_AWK_ASSERT (run->awk, ase_sizeof(void*) >= ase_sizeof(run->stack_base));

	saved_stack_top = run->stack_top;

/*xp_printf (ASE_T("setting up function stack frame stack_top = %ld stack_base = %ld\n"), run->stack_top, run->stack_base); */
	if (__raw_push(run,(void*)run->stack_base) == -1) 
	{
		PANIC (run, ASE_AWK_ENOMEM);
	}

	if (__raw_push(run,(void*)saved_stack_top) == -1) 
	{
		__raw_pop (run);
		PANIC (run, ASE_AWK_ENOMEM);
	}

	/* secure space for a return value. */
	if (__raw_push(run,ase_awk_val_nil) == -1)
	{
		__raw_pop (run);
		__raw_pop (run);
		PANIC (run, ASE_AWK_ENOMEM);
	}

	/* secure space for nargs */
	if (__raw_push(run,ase_awk_val_nil) == -1)
	{
		__raw_pop (run);
		__raw_pop (run);
		__raw_pop (run);
		PANIC (run, ASE_AWK_ENOMEM);
	}

	nargs = 0;
	p = call->args;
	while (p != ASE_NULL)
	{
		ASE_AWK_ASSERT (run->awk, bfn_arg_spec == ASE_NULL ||
		           (bfn_arg_spec != ASE_NULL && 
		            ase_awk_strlen(bfn_arg_spec) > nargs));

		if (bfn_arg_spec != ASE_NULL && 
		    bfn_arg_spec[nargs] == ASE_T('r'))
		{
			ase_awk_val_t** ref;
			      
			if (__get_reference (run, p, &ref) == -1)
			{
				UNWIND_RUN_STACK (run, nargs);
				return ASE_NULL;
			}

			/* p->type-ASE_AWK_NDE_NAMED assumes that the
			 * derived value matches ASE_AWK_VAL_REF_XXX */
			v = ase_awk_makerefval (
				run, p->type-ASE_AWK_NDE_NAMED, ref);
		}
		else if (bfn_arg_spec != ASE_NULL && 
		         bfn_arg_spec[nargs] == ASE_T('x'))
		{
			/* a regular expression is passed to 
			 * the function as it is */
			v = __eval_expression0 (run, p);
		}
		else
		{
			v = __eval_expression (run, p);
		}
		if (v == ASE_NULL)
		{
			UNWIND_RUN_STACK (run, nargs);
			return ASE_NULL;
		}

#if 0
		if (bfn_arg_spec != ASE_NULL && 
		    bfn_arg_spec[nargs] == ASE_T('r'))
		{
			ase_awk_val_t** ref;
			ase_awk_val_t* tmp;
			      
			ref = __get_reference (run, p);
			if (ref == ASE_NULL)
			{
				ase_awk_refupval (v);
				ase_awk_refdownval (run, v);

				UNWIND_RUN_STACK (run, nargs);
				return ASE_NULL;
			}

			/* p->type-ASE_AWK_NDE_NAMED assumes that the
			 * derived value matches ASE_AWK_VAL_REF_XXX */
			tmp = ase_awk_makerefval (
				run, p->type-ASE_AWK_NDE_NAMED, ref);
			if (tmp == ASE_NULL)
			{
				ase_awk_refupval (v);
				ase_awk_refdownval (run, v);

				UNWIND_RUN_STACK (run, nargs);
				PANIC (run, ASE_AWK_ENOMEM);
			}

			ase_awk_refupval (v);
			ase_awk_refdownval (run, v);

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
			ase_awk_refupval (v);
			ase_awk_refdownval (run, v);

			UNWIND_RUN_STACK (run, nargs);
			PANIC (run, ASE_AWK_ENOMEM);
		}

		ase_awk_refupval (v);
		nargs++;
		p = p->next;
	}

	ASE_AWK_ASSERT (run->awk, nargs == call->nargs);

	if (afn != ASE_NULL)
	{
		/* extra step for normal awk functions */

		while (nargs < afn->nargs)
		{
			/* push as many nils as the number of missing actual arguments */
			if (__raw_push(run,ase_awk_val_nil) == -1)
			{
				UNWIND_RUN_STACK (run, nargs);
				PANIC (run, ASE_AWK_ENOMEM);
			}

			nargs++;
		}
	}

	run->stack_base = saved_stack_top;
	STACK_NARGS(run) = (void*)nargs;
	
/*xp_printf (ASE_T("running function body\n")); */

	if (afn != ASE_NULL)
	{
		/* normal awk function */
		ASE_AWK_ASSERT (run->awk, afn->body->type == ASE_AWK_NDE_BLK);
		n = __run_block(run,(ase_awk_nde_blk_t*)afn->body);
	}
	else
	{
		n = 0;

		/* built-in function */
		ASE_AWK_ASSERT (run->awk, call->nargs >= call->what.bfn.min_args &&
		           call->nargs <= call->what.bfn.max_args);

		if (call->what.bfn.handler != ASE_NULL)
			n = call->what.bfn.handler (run);
	}

/*xp_printf (ASE_T("block run complete\n")); */

	/* refdown args in the run.stack */
	nargs = (ase_size_t)STACK_NARGS(run);
/*xp_printf (ASE_T("block run complete nargs = %d\n"), (int)nargs); */
	for (i = 0; i < nargs; i++)
	{
		ase_awk_refdownval (run, STACK_ARG(run,i));
	}
/*xp_printf (ASE_T("got return value\n")); */

	/* this trick has been mentioned in __run_return.
	 * adjust the reference count of the return value.
	 * the value must not be freed even if the reference count
	 * is decremented to zero because its reference has been incremented 
	 * in __run_return regardless of its reference count. */
	v = STACK_RETVAL(run);
	ase_awk_refdownval_nofree (run, v);

	run->stack_top =  (ase_size_t)run->stack[run->stack_base+1];
	run->stack_base = (ase_size_t)run->stack[run->stack_base+0];

	if (run->exit_level == EXIT_FUNCTION) run->exit_level = EXIT_NONE;

/*xp_printf (ASE_T("returning from function stack_top=%ld, stack_base=%ld\n"), run->stack_top, run->stack_base); */
	return (n == -1)? ASE_NULL: v;
}

static int __get_reference (
	ase_awk_run_t* run, ase_awk_nde_t* nde, ase_awk_val_t*** ref)
{
	ase_awk_nde_var_t* tgt = (ase_awk_nde_var_t*)nde;
	ase_awk_val_t** tmp;

	/* refer to __eval_indexed for application of a similar concept */

	if (nde->type == ASE_AWK_NDE_NAMED)
	{
		ase_awk_pair_t* pair;

		pair = ase_awk_map_get (
			&run->named, tgt->id.name, tgt->id.name_len);
		if (pair == ASE_NULL)
		{
			/* it is bad that the named variable has to be
			 * created in the function named "__get_refernce".
			 * would there be any better ways to avoid this? */
			pair = ase_awk_map_put (
				&run->named, tgt->id.name,
				tgt->id.name_len, ase_awk_val_nil);
			if (pair == ASE_NULL) PANIC_I (run, ASE_AWK_ENOMEM);
		}

		*ref = (ase_awk_val_t**)&pair->val;
		return 0;
	}

	if (nde->type == ASE_AWK_NDE_GLOBAL)
	{
		*ref = (ase_awk_val_t**)&STACK_GLOBAL(run,tgt->id.idxa);
		return 0;
	}

	if (nde->type == ASE_AWK_NDE_LOCAL)
	{
		*ref = (ase_awk_val_t**)&STACK_LOCAL(run,tgt->id.idxa);
		return 0;
	}

	if (nde->type == ASE_AWK_NDE_ARG)
	{
		*ref = (ase_awk_val_t**)&STACK_ARG(run,tgt->id.idxa);
		return 0;
	}

	if (nde->type == ASE_AWK_NDE_NAMEDIDX)
	{
		ase_awk_pair_t* pair;

		pair = ase_awk_map_get (
			&run->named, tgt->id.name, tgt->id.name_len);
		if (pair == ASE_NULL)
		{
			pair = ase_awk_map_put (
				&run->named, tgt->id.name,
				tgt->id.name_len, ase_awk_val_nil);
			if (pair == ASE_NULL) PANIC_I (run, ASE_AWK_ENOMEM);
		}

		tmp = __get_reference_indexed (
			run, tgt, (ase_awk_val_t**)&pair->val);
		if (tmp == ASE_NULL) return -1;
		*ref = tmp;
	}

	if (nde->type == ASE_AWK_NDE_GLOBALIDX)
	{
		tmp = __get_reference_indexed (run, tgt, 
			(ase_awk_val_t**)&STACK_GLOBAL(run,tgt->id.idxa));
		if (tmp == ASE_NULL) return -1;
		*ref = tmp;
	}

	if (nde->type == ASE_AWK_NDE_LOCALIDX)
	{
		tmp = __get_reference_indexed (run, tgt, 
			(ase_awk_val_t**)&STACK_LOCAL(run,tgt->id.idxa));
		if (tmp == ASE_NULL) return -1;
		*ref = tmp;
	}

	if (nde->type == ASE_AWK_NDE_ARGIDX)
	{
		tmp = __get_reference_indexed (run, tgt, 
			(ase_awk_val_t**)&STACK_ARG(run,tgt->id.idxa));
		if (tmp == ASE_NULL) return -1;
		*ref = tmp;
	}

	if (nde->type == ASE_AWK_NDE_POS)
	{
		int n;
		ase_long_t lv;
		ase_real_t rv;
		ase_awk_val_t* v;

		/* the position number is returned for the positional 
		 * variable unlike other reference types. */
		v = __eval_expression (run, ((ase_awk_nde_pos_t*)nde)->val);
		if (v == ASE_NULL) return -1;

		ase_awk_refupval (v);
		n = ase_awk_valtonum (run, v, &lv, &rv);
		ase_awk_refdownval (run, v);

		if (n == -1) PANIC_I (run, ASE_AWK_EPOSIDX);
		if (n == 1) lv = (ase_long_t)rv;
		if (!IS_VALID_POSIDX(lv)) PANIC_I (run, ASE_AWK_EPOSIDX);

		*ref = (ase_awk_val_t**)((ase_size_t)lv);
		return 0;
	}

	PANIC_I (run, ASE_AWK_ENOTREFERENCEABLE);
}

static ase_awk_val_t** __get_reference_indexed (
	ase_awk_run_t* run, ase_awk_nde_var_t* nde, ase_awk_val_t** val)
{
	ase_awk_pair_t* pair;
	ase_char_t* str;
	ase_size_t len;

	ASE_AWK_ASSERT (run->awk, val != ASE_NULL);

	if ((*val)->type == ASE_AWK_VAL_NIL)
	{
		ase_awk_val_t* tmp;

		tmp = ase_awk_makemapval (run);
		if (tmp == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);

		ase_awk_refdownval (run, *val);
		*val = tmp;
		ase_awk_refupval ((ase_awk_val_t*)*val);
	}
	else if ((*val)->type != ASE_AWK_VAL_MAP) 
	{
		PANIC (run, ASE_AWK_ENOTINDEXABLE);
	}

	ASE_AWK_ASSERT (run->awk, nde->idx != ASE_NULL);

	str = __idxnde_to_str (run, nde->idx, &len);
	if (str == ASE_NULL) return ASE_NULL;

	pair = ase_awk_map_get ((*(ase_awk_val_map_t**)val)->map, str, len);
	if (pair == ASE_NULL)
	{
		pair = ase_awk_map_put (
			(*(ase_awk_val_map_t**)val)->map, 
			str, len, ase_awk_val_nil);
		if (pair == ASE_NULL)
		{
			ASE_AWK_FREE (run->awk, str);
			PANIC (run, ASE_AWK_ENOMEM);
		}

		ase_awk_refupval (pair->val);
	}

	ASE_AWK_FREE (run->awk, str);
	return (ase_awk_val_t**)&pair->val;
}

static ase_awk_val_t* __eval_int (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	ase_awk_val_t* val;

	val = ase_awk_makeintval (run, ((ase_awk_nde_int_t*)nde)->val);
	if (val == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);
	((ase_awk_val_int_t*)val)->nde = (ase_awk_nde_int_t*)nde; 

	return val;
}

static ase_awk_val_t* __eval_real (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	ase_awk_val_t* val;

	val = ase_awk_makerealval (run, ((ase_awk_nde_real_t*)nde)->val);
	if (val == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);
	((ase_awk_val_real_t*)val)->nde = (ase_awk_nde_real_t*)nde;

	return val;
}

static ase_awk_val_t* __eval_str (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	ase_awk_val_t* val;

	val = ase_awk_makestrval (run,
		((ase_awk_nde_str_t*)nde)->buf,
		((ase_awk_nde_str_t*)nde)->len);
	if (val == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);

	return val;
}

static ase_awk_val_t* __eval_rex (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	ase_awk_val_t* val;

	val = ase_awk_makerexval (run,
		((ase_awk_nde_rex_t*)nde)->buf,
		((ase_awk_nde_rex_t*)nde)->len,
		((ase_awk_nde_rex_t*)nde)->code);
	if (val == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);

	return val;
}

static ase_awk_val_t* __eval_named (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	ase_awk_pair_t* pair;
		       
	pair = ase_awk_map_get (&run->named, 
		((ase_awk_nde_var_t*)nde)->id.name, 
		((ase_awk_nde_var_t*)nde)->id.name_len);

	return (pair == ASE_NULL)? ase_awk_val_nil: pair->val;
}

static ase_awk_val_t* __eval_global (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	return STACK_GLOBAL(run,((ase_awk_nde_var_t*)nde)->id.idxa);
}

static ase_awk_val_t* __eval_local (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	return STACK_LOCAL(run,((ase_awk_nde_var_t*)nde)->id.idxa);
}

static ase_awk_val_t* __eval_arg (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	return STACK_ARG(run,((ase_awk_nde_var_t*)nde)->id.idxa);
}

static ase_awk_val_t* __eval_indexed (
	ase_awk_run_t* run, ase_awk_nde_var_t* nde, ase_awk_val_t** val)
{
	ase_awk_pair_t* pair;
	ase_char_t* str;
	ase_size_t len;

	ASE_AWK_ASSERT (run->awk, val != ASE_NULL);

	if ((*val)->type == ASE_AWK_VAL_NIL)
	{
		ase_awk_val_t* tmp;

		tmp = ase_awk_makemapval (run);
		if (tmp == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);

		ase_awk_refdownval (run, *val);
		*val = tmp;
		ase_awk_refupval ((ase_awk_val_t*)*val);
	}
	else if ((*val)->type != ASE_AWK_VAL_MAP) 
	{
	        PANIC (run, ASE_AWK_ENOTINDEXABLE);
	}

	ASE_AWK_ASSERT (run->awk, nde->idx != ASE_NULL);

	str = __idxnde_to_str (run, nde->idx, &len);
	if (str == ASE_NULL) return ASE_NULL;

	pair = ase_awk_map_get ((*(ase_awk_val_map_t**)val)->map, str, len);
	ASE_AWK_FREE (run->awk, str);

	return (pair == ASE_NULL)? ase_awk_val_nil: (ase_awk_val_t*)pair->val;
}

static ase_awk_val_t* __eval_namedidx (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	ase_awk_nde_var_t* tgt = (ase_awk_nde_var_t*)nde;
	ase_awk_pair_t* pair;

	pair = ase_awk_map_get (&run->named, tgt->id.name, tgt->id.name_len);
	if (pair == ASE_NULL)
	{
		pair = ase_awk_map_put (&run->named, 
			tgt->id.name, tgt->id.name_len, ase_awk_val_nil);
		if (pair == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);

		ase_awk_refupval (pair->val); 
	}

	return __eval_indexed (run, tgt, (ase_awk_val_t**)&pair->val);
}

static ase_awk_val_t* __eval_globalidx (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	return __eval_indexed (run, (ase_awk_nde_var_t*)nde, 
		(ase_awk_val_t**)&STACK_GLOBAL(run,((ase_awk_nde_var_t*)nde)->id.idxa));
}

static ase_awk_val_t* __eval_localidx (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	return __eval_indexed (run, (ase_awk_nde_var_t*)nde, 
		(ase_awk_val_t**)&STACK_LOCAL(run,((ase_awk_nde_var_t*)nde)->id.idxa));
}

static ase_awk_val_t* __eval_argidx (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	return __eval_indexed (run, (ase_awk_nde_var_t*)nde,
		(ase_awk_val_t**)&STACK_ARG(run,((ase_awk_nde_var_t*)nde)->id.idxa));
}

static ase_awk_val_t* __eval_pos (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	ase_awk_nde_pos_t* pos = (ase_awk_nde_pos_t*)nde;
	ase_awk_val_t* v;
	ase_long_t lv;
	ase_real_t rv;
	int n;

	v = __eval_expression (run, pos->val);
	if (v == ASE_NULL) return ASE_NULL;

	ase_awk_refupval (v);
	n = ase_awk_valtonum (run, v, &lv, &rv);
	ase_awk_refdownval (run, v);

	if (n == -1) PANIC (run, ASE_AWK_EPOSIDX);
	if (n == 1) lv = (ase_long_t)rv;

	if (lv < 0) PANIC (run, ASE_AWK_EPOSIDX);
	if (lv == 0) v = run->inrec.d0;
	else if (lv > 0 && lv <= run->inrec.nflds) 
		v = run->inrec.flds[lv-1].val;
	else v = ase_awk_val_zls; /*ase_awk_val_nil;*/

	return v;
}

static ase_awk_val_t* __eval_getline (ase_awk_run_t* run, ase_awk_nde_t* nde)
{
	ase_awk_nde_getline_t* p;
	ase_awk_val_t* v, * res;
	ase_char_t* in = ASE_NULL;
	const ase_char_t* dst;
	ase_awk_str_t buf;
	int n;

	p = (ase_awk_nde_getline_t*)nde;

	ASE_AWK_ASSERT (run->awk, (p->in_type == ASE_AWK_IN_PIPE && p->in != ASE_NULL) ||
	           (p->in_type == ASE_AWK_IN_COPROC && p->in != ASE_NULL) ||
		   (p->in_type == ASE_AWK_IN_FILE && p->in != ASE_NULL) ||
	           (p->in_type == ASE_AWK_IN_CONSOLE && p->in == ASE_NULL));

	if (p->in != ASE_NULL)
	{
		ase_size_t len;

		v = __eval_expression (run, p->in);
		if (v == ASE_NULL) return ASE_NULL;

		/* TODO: distinction between v->type == ASE_AWK_VAL_STR 
		 *       and v->type != ASE_AWK_VAL_STR
		 *       if you use the buffer the v directly when
		 *       v->type == ASE_AWK_VAL_STR, ase_awk_refdownval(v)
		 *       should not be called immediately below */
		ase_awk_refupval (v);
		in = ase_awk_valtostr (
			run, v, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &len);
		if (in == ASE_NULL) 
		{
			ase_awk_refdownval (run, v);
			return ASE_NULL;
		}
		ase_awk_refdownval (run, v);

		if (len <= 0) 
		{
			/* the input source name is empty.
			 * make getline return -1 */
			ASE_AWK_FREE (run->awk, in);
			n = -1;
			goto skip_read;
		}

		while (len > 0)
		{
			if (in[--len] == ASE_T('\0'))
			{
				/* the input source name contains a null 
				 * character. make getline return -1 */
				/* TODO: set ERRNO */
				ASE_AWK_FREE (run->awk, in);
				n = -1;
				goto skip_read;
			}
		}
	}

	dst = (in == ASE_NULL)? ASE_T(""): in;

	/* TODO: optimize the line buffer management */
	if (ase_awk_str_open (&buf, DEF_BUF_CAPA, run->awk) == ASE_NULL)
	{
		if (in != ASE_NULL) ASE_AWK_FREE (run->awk, in);
		PANIC (run, ASE_AWK_ENOMEM);
	}

	n = ase_awk_readextio (run, p->in_type, dst, &buf);
	if (in != ASE_NULL) ASE_AWK_FREE (run->awk, in);

	if (n < 0) 
	{
		if (run->errnum != ASE_AWK_EIOHANDLER)
		{
			ase_awk_str_close (&buf);
			return ASE_NULL;
		}

		/* if run->errnum == ASE_AWK_EIOHANDLER, 
		 * make getline return -1 */
		n = -1;
	}

	if (n > 0)
	{
		if (p->var == ASE_NULL)
		{
			/* set $0 with the input value */
			if (ase_awk_setrec (run, 0,
				ASE_AWK_STR_BUF(&buf),
				ASE_AWK_STR_LEN(&buf)) == -1)
			{
				ase_awk_str_close (&buf);
				return ASE_NULL;
			}

			ase_awk_str_close (&buf);
		}
		else
		{
			ase_awk_val_t* v;

			v = ase_awk_makestrval (
				run, ASE_AWK_STR_BUF(&buf), ASE_AWK_STR_LEN(&buf));
			ase_awk_str_close (&buf);
			if (v == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);

			ase_awk_refupval (v);
			if (__do_assignment(run, p->var, v) == ASE_NULL)
			{
				ase_awk_refdownval (run, v);
				return ASE_NULL;
			}
			ase_awk_refdownval (run, v);
		}
	}
	else
	{
		ase_awk_str_close (&buf);
	}
	
skip_read:
	res = ase_awk_makeintval (run, n);
	if (res == ASE_NULL) PANIC (run, ASE_AWK_ENOMEM);

	return res;
}

static int __raw_push (ase_awk_run_t* run, void* val)
{
	if (run->stack_top >= run->stack_limit)
	{
		void** tmp;
		ase_size_t n;
	       
		n = run->stack_limit + STACK_INCREMENT;

		if (run->awk->syscas.realloc != ASE_NULL)
		{
			tmp = (void**) ASE_AWK_REALLOC (
				run->awk, run->stack, n * ase_sizeof(void*)); 
			if (tmp == ASE_NULL) return -1;
		}
		else
		{
			tmp = (void**) ASE_AWK_MALLOC (
				run->awk, n * ase_sizeof(void*));
			if (tmp == ASE_NULL) return -1;
			if (run->stack != ASE_NULL)
			{
				ASE_AWK_MEMCPY (run->awk, tmp, run->stack, 
					run->stack_limit * ase_sizeof(void*)); 
				ASE_AWK_FREE (run->awk, run->stack);
			}
		}
		run->stack = tmp;
		run->stack_limit = n;
	}

	run->stack[run->stack_top++] = val;
	return 0;
}

static void __raw_pop_times (ase_awk_run_t* run, ase_size_t times)
{
	while (times > 0)
	{
		--times;
		__raw_pop (run);
	}
}

static int __read_record (ase_awk_run_t* run)
{
	ase_ssize_t n;

	if (ase_awk_clrrec (run, ase_false) == -1) return -1;

	n = ase_awk_readextio (
		run, ASE_AWK_IN_CONSOLE, ASE_T(""), &run->inrec.line);
	if (n < 0) 
	{
		int errnum = run->errnum;
		ase_awk_clrrec (run, ase_false);
		run->errnum = 
			(errnum == ASE_AWK_EIOHANDLER)? 
			ASE_AWK_ECONINDATA: errnum;
		return -1;
	}
/*
xp_printf (ASE_T("len = %d str=[%s]\n"), 
		(int)ASE_AWK_STR_LEN(&run->inrec.line),
		ASE_AWK_STR_BUF(&run->inrec.line));
*/
	if (n == 0) 
	{
		ASE_AWK_ASSERT (run->awk, ASE_AWK_STR_LEN(&run->inrec.line) == 0);
		return 0;
	}

	if (ase_awk_setrec (run, 0, 
		ASE_AWK_STR_BUF(&run->inrec.line), 
		ASE_AWK_STR_LEN(&run->inrec.line)) == -1) return -1;

	return 1;
}

static int __shorten_record (ase_awk_run_t* run, ase_size_t nflds)
{
	ase_awk_val_t* v;
	ase_char_t* ofs_free = ASE_NULL, * ofs;
	ase_size_t ofs_len, i;
	ase_awk_str_t tmp;

	ASE_AWK_ASSERT (run->awk, nflds <= run->inrec.nflds);

	if (nflds > 1)
	{
		v = STACK_GLOBAL(run, ASE_AWK_GLOBAL_OFS);
		ase_awk_refupval (v);

		if (v->type == ASE_AWK_VAL_NIL)
		{
			/* OFS not set */
			ofs = ASE_T(" ");
			ofs_len = 1;
		}
		else if (v->type == ASE_AWK_VAL_STR)
		{
			ofs = ((ase_awk_val_str_t*)v)->buf;
			ofs_len = ((ase_awk_val_str_t*)v)->len;
		}
		else
		{
			ofs = ase_awk_valtostr (
				run, v, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &ofs_len);
			if (ofs == ASE_NULL) return -1;

			ofs_free = ofs;
		}
	}

	if (ase_awk_str_open (&tmp, 
		ASE_AWK_STR_LEN(&run->inrec.line), run->awk) == ASE_NULL)
	{
		run->errnum = ASE_AWK_ENOMEM;
		return -1;
	}

	for (i = 0; i < nflds; i++)
	{
		if (i > 0 && ase_awk_str_ncat (&tmp, ofs, ofs_len) == (ase_size_t)-1)
		{
			if (ofs_free != ASE_NULL) 
				ASE_AWK_FREE (run->awk, ofs_free);
			if (nflds > 1) ase_awk_refdownval (run, v);
			run->errnum = ASE_AWK_ENOMEM;
			return -1;
		}

		if (ase_awk_str_ncat (&tmp, 
			run->inrec.flds[i].ptr, 
			run->inrec.flds[i].len) == (ase_size_t)-1)
		{
			if (ofs_free != ASE_NULL) 
				ASE_AWK_FREE (run->awk, ofs_free);
			if (nflds > 1) ase_awk_refdownval (run, v);
			run->errnum = ASE_AWK_ENOMEM;
			return -1;
		}
	}

	if (ofs_free != ASE_NULL) ASE_AWK_FREE (run->awk, ofs_free);
	if (nflds > 1) ase_awk_refdownval (run, v);

	v = (ase_awk_val_t*) ase_awk_makestrval (
		run, ASE_AWK_STR_BUF(&tmp), ASE_AWK_STR_LEN(&tmp));
	if (v == ASE_NULL)
	{
		run->errnum = ASE_AWK_ENOMEM;
		return -1;
	}

	ase_awk_refdownval (run, run->inrec.d0);
	run->inrec.d0 = v;
	ase_awk_refupval (run->inrec.d0);

	ase_awk_str_swap (&tmp, &run->inrec.line);
	ase_awk_str_close (&tmp);

	for (i = nflds; i < run->inrec.nflds; i++)
	{
		ase_awk_refdownval (run, run->inrec.flds[i].val);
	}

	run->inrec.nflds = nflds;
	return 0;
}

static ase_char_t* __idxnde_to_str (
	ase_awk_run_t* run, ase_awk_nde_t* nde, ase_size_t* len)
{
	ase_char_t* str;
	ase_awk_val_t* idx;

	ASE_AWK_ASSERT (run->awk, nde != ASE_NULL);

	if (nde->next == ASE_NULL)
	{
		/* single node index */
		idx = __eval_expression (run, nde);
		if (idx == ASE_NULL) return ASE_NULL;

		ase_awk_refupval (idx);

		str = ase_awk_valtostr (
			run, idx, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, len);
		if (str == ASE_NULL) 
		{
			ase_awk_refdownval (run, idx);
			return ASE_NULL;
		}

		ase_awk_refdownval (run, idx);
	}
	else
	{
		/* multidimensional index */
		ase_awk_str_t idxstr;

		if (ase_awk_str_open (
			&idxstr, DEF_BUF_CAPA, run->awk) == ASE_NULL) 
		{
			PANIC (run, ASE_AWK_ENOMEM);
		}

		while (nde != ASE_NULL)
		{
			idx = __eval_expression (run, nde);
			if (idx == ASE_NULL) 
			{
				ase_awk_str_close (&idxstr);
				return ASE_NULL;
			}

			ase_awk_refupval (idx);

			if (ASE_AWK_STR_LEN(&idxstr) > 0 &&
			    ase_awk_str_ncat (&idxstr, 
			    	run->global.subsep.ptr, 
			    	run->global.subsep.len) == (ase_size_t)-1)
			{
				ase_awk_refdownval (run, idx);
				ase_awk_str_close (&idxstr);
				PANIC (run, ASE_AWK_ENOMEM);
			}

			if (ase_awk_valtostr (
				run, idx, 0, &idxstr, ASE_NULL) == ASE_NULL)
			{
				ase_awk_refdownval (run, idx);
				ase_awk_str_close (&idxstr);
				return ASE_NULL;
			}

			ase_awk_refdownval (run, idx);
			nde = nde->next;
		}

		str = ASE_AWK_STR_BUF(&idxstr);
		*len = ASE_AWK_STR_LEN(&idxstr);
		ase_awk_str_forfeit (&idxstr);
	}

	return str;
}
