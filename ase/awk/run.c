/*
 * $Id: run.c,v 1.167 2006-08-16 11:35:53 bacon Exp $
 */

#include <xp/awk/awk_i.h>

/* TODO: remove this dependency...*/
#include <math.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/assert.h>
#include <xp/bas/string.h>
#include <xp/bas/memory.h>
#endif

#define DEF_BUF_CAPA 256

#define STACK_INCREMENT 512

#define STACK_AT(run,n) ((run)->stack[(run)->stack_base+(n)])
#define STACK_NARGS(run) (STACK_AT(run,3))
#define STACK_ARG(run,n) STACK_AT(run,3+1+(n))
#define STACK_LOCAL(run,n) STACK_AT(run,3+(xp_size_t)STACK_NARGS(run)+1+(n))
#define STACK_RETVAL(run) STACK_AT(run,2)
#define STACK_GLOBAL(run,n) ((run)->stack[(n)])
/*#define STACK_RETVAL_GLOBAL(run) ((run)->stack[(run)->nglobals+2])*/
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
	do { (run)->errnum = (code); return XP_NULL; } while (0)

#define PANIC_I(run,code) \
	do { (run)->errnum = (code); return -1; } while (0)

static void __add_run (xp_awk_t* awk, xp_awk_run_t* run);
static void __del_run (xp_awk_t* awk, xp_awk_run_t* run);

static int __init_run (
	xp_awk_run_t* run, xp_awk_runios_t* runios, int* errnum);
static void __deinit_run (xp_awk_run_t* run);

static int __run_main (xp_awk_run_t* run);
static int __run_pattern_blocks  (xp_awk_run_t* run);
static int __run_pattern_block_chain (
	xp_awk_run_t* run, xp_awk_chain_t* chain);
static int __run_pattern_block (
	xp_awk_run_t* run, xp_awk_chain_t* chain, xp_size_t block_no);
static int __run_block (xp_awk_run_t* run, xp_awk_nde_blk_t* nde);
static int __run_statement (xp_awk_run_t* run, xp_awk_nde_t* nde);
static int __run_if (xp_awk_run_t* run, xp_awk_nde_if_t* nde);
static int __run_while (xp_awk_run_t* run, xp_awk_nde_while_t* nde);
static int __run_for (xp_awk_run_t* run, xp_awk_nde_for_t* nde);
static int __run_foreach (xp_awk_run_t* run, xp_awk_nde_foreach_t* nde);
static int __run_break (xp_awk_run_t* run, xp_awk_nde_break_t* nde);
static int __run_continue (xp_awk_run_t* run, xp_awk_nde_continue_t* nde);
static int __run_return (xp_awk_run_t* run, xp_awk_nde_return_t* nde);
static int __run_exit (xp_awk_run_t* run, xp_awk_nde_exit_t* nde);
static int __run_next (xp_awk_run_t* run, xp_awk_nde_next_t* nde);
static int __run_nextfile (xp_awk_run_t* run, xp_awk_nde_nextfile_t* nde);
static int __run_delete (xp_awk_run_t* run, xp_awk_nde_delete_t* nde);
static int __run_print (xp_awk_run_t* run, xp_awk_nde_print_t* nde);

static xp_awk_val_t* __eval_expression (xp_awk_run_t* run, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_expression0 (xp_awk_run_t* run, xp_awk_nde_t* nde);

static xp_awk_val_t* __eval_group (xp_awk_run_t* run, xp_awk_nde_t* nde);

static xp_awk_val_t* __eval_assignment (
	xp_awk_run_t* run, xp_awk_nde_t* nde);
static xp_awk_val_t* __do_assignment (
	xp_awk_run_t* run, xp_awk_nde_t* var, xp_awk_val_t* val);
static xp_awk_val_t* __do_assignment_scalar (
	xp_awk_run_t* run, xp_awk_nde_var_t* var, xp_awk_val_t* val);
static xp_awk_val_t* __do_assignment_map (
	xp_awk_run_t* run, xp_awk_nde_var_t* var, xp_awk_val_t* val);
static xp_awk_val_t* __do_assignment_pos (
	xp_awk_run_t* run, xp_awk_nde_pos_t* pos, xp_awk_val_t* val);

static xp_awk_val_t* __eval_binary (
	xp_awk_run_t* run, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_binop_lor (
	xp_awk_run_t* run, xp_awk_nde_t* left, xp_awk_nde_t* right);
static xp_awk_val_t* __eval_binop_land (
	xp_awk_run_t* run, xp_awk_nde_t* left, xp_awk_nde_t* right);
static xp_awk_val_t* __eval_binop_in (
	xp_awk_run_t* run, xp_awk_nde_t* left, xp_awk_nde_t* right);
static xp_awk_val_t* __eval_binop_bor (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_bxor (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_band (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_eq (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_ne (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_gt (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_ge (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_lt (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_le (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_lshift (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_rshift (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_plus (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_minus (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_mul (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_div (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_mod (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_exp (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_concat (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_ma (
	xp_awk_run_t* run, xp_awk_nde_t* left, xp_awk_nde_t* right);
static xp_awk_val_t* __eval_binop_nm (
	xp_awk_run_t* run, xp_awk_nde_t* left, xp_awk_nde_t* right);
static xp_awk_val_t* __eval_binop_match0 (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right, int ret);

static xp_awk_val_t* __eval_unary (xp_awk_run_t* run, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_incpre (xp_awk_run_t* run, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_incpst (xp_awk_run_t* run, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_cnd (xp_awk_run_t* run, xp_awk_nde_t* nde);

static xp_awk_val_t* __eval_bfn (xp_awk_run_t* run, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_afn (xp_awk_run_t* run, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_call (
	xp_awk_run_t* run, xp_awk_nde_t* nde, xp_awk_afn_t* afn);

static xp_awk_val_t* __eval_int (xp_awk_run_t* run, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_real (xp_awk_run_t* run, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_str (xp_awk_run_t* run, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_rex (xp_awk_run_t* run, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_named (xp_awk_run_t* run, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_global (xp_awk_run_t* run, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_local (xp_awk_run_t* run, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_arg (xp_awk_run_t* run, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_namedidx (xp_awk_run_t* run, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_globalidx (xp_awk_run_t* run, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_localidx (xp_awk_run_t* run, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_argidx (xp_awk_run_t* run, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_pos (xp_awk_run_t* run, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_getline (xp_awk_run_t* run, xp_awk_nde_t* nde);

static int __raw_push (xp_awk_run_t* run, void* val);
static void __raw_pop (xp_awk_run_t* run);
static void __raw_pop_times (xp_awk_run_t* run, xp_size_t times);

static int __read_record (xp_awk_run_t* run);
static int __set_record (
	xp_awk_run_t* run, const xp_char_t* str, xp_size_t len);
static int __split_record (xp_awk_run_t* run);
static void __clear_record (xp_awk_run_t* run, xp_bool_t noline);
static int __recomp_record_fields (xp_awk_run_t* run, 
	xp_size_t lv, xp_char_t* str, xp_size_t len, int* errnum);

static xp_char_t* __idxnde_to_str (
	xp_awk_run_t* run, xp_awk_nde_t* nde, xp_size_t* len);

typedef xp_awk_val_t* (*binop_func_t) (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right);
typedef xp_awk_val_t* (*eval_expr_t) (xp_awk_run_t* run, xp_awk_nde_t* nde);

/* TODO: remove this function */
static int __printval (xp_awk_pair_t* pair, void* arg)
{
	xp_printf (XP_T("%s = "), (const xp_char_t*)pair->key);
	xp_awk_printval ((xp_awk_val_t*)pair->val);
	xp_printf (XP_T("\n"));
	return 0;
}

xp_size_t xp_awk_getnargs (void* run)
{
	return (xp_size_t) STACK_NARGS ((xp_awk_run_t*)run);
}

xp_awk_val_t* xp_awk_getarg (void* run, xp_size_t idx)
{
	return STACK_ARG ((xp_awk_run_t*)run, idx);
}

void xp_awk_setretval (void* run, xp_awk_val_t* val)
{
	xp_awk_run_t* r = (xp_awk_run_t*)run;
	xp_awk_refdownval (r, STACK_RETVAL(r));
	STACK_RETVAL(r) = val;
	/* should use the same trick as __run_return_statement */
	xp_awk_refupval (val); 
}

void xp_awk_setglobal (void* run, xp_size_t idx, xp_awk_val_t* val)
{
	xp_awk_run_t* r = (xp_awk_run_t*)run;
	xp_awk_refdownval (run, STACK_GLOBAL(r,idx));
	STACK_GLOBAL(r,idx) = val;
	xp_awk_refupval (val);
}

void xp_awk_seterrnum (void* run, int errnum)
{
	xp_awk_run_t* r = (xp_awk_run_t*)run;
	r->errnum = errnum;
}

int xp_awk_run (xp_awk_t* awk, xp_awk_runios_t* runios, xp_awk_runcbs_t* runcbs)
{
	xp_awk_run_t* run;
	int n, errnum;

	awk->errnum = XP_AWK_ENOERR;

	run = (xp_awk_run_t*) xp_malloc (xp_sizeof(xp_awk_run_t));
	if (run == XP_NULL)
	{
		awk->errnum = XP_AWK_ENOMEM;
		return -1;
	}

	__add_run (awk, run);

	if (__init_run (run, runios, &errnum) == -1) 
	{
		awk->errnum = errnum;
		__del_run (awk, run);
		xp_free (run);
		return -1;
	}

	if (runcbs != XP_NULL && runcbs->on_start != XP_NULL) 
	{
		runcbs->on_start (awk, run, runcbs->custom_data);
	}

	n = __run_main (run);
	if (n == -1) 
	{
		/* if no callback is specified, awk's error number 
		 * is updated with the run's error number */
		awk->errnum = (runcbs == XP_NULL)? run->errnum: XP_AWK_ERUNTIME;
	}

	if (runcbs != XP_NULL && runcbs->on_end != XP_NULL) 
	{
		runcbs->on_end (awk, run, 
			((n == -1)? run->errnum: XP_AWK_ENOERR), 
			runcbs->custom_data);

		/* when using callbacks, the function always returns 0 after
		 * the start callbacks has been triggered */
		n = 0;
	}

	__deinit_run (run);

	__del_run (awk, run);
	xp_free (run);

	return n;
}

int xp_awk_stop (xp_awk_t* awk, void* run)
{
	xp_awk_run_t* r;
	int n = 0;

	if (awk->thr.lks != XP_NULL)
		awk->thr.lks->lock (awk, awk->thr.lks->custom_data);

	/* check if the run handle given is valid */
	for (r = awk->run.ptr; r != XP_NULL; r = r->next)
	{
		if (r == run)
		{
			xp_assert (r->awk == awk);
			r->exit_level = EXIT_ABORT;
			break;
		}
	}

	if (r == XP_NULL)
	{
		/* if it is not found in the awk's run list, 
		 * it is not a valid handle */
		awk->errnum = XP_AWK_EINVAL;
		n = -1;
	}

	if (awk->thr.lks != XP_NULL)
		awk->thr.lks->unlock (awk, awk->thr.lks->custom_data);

	return n;
}

void xp_awk_stopall (xp_awk_t* awk)
{
	xp_awk_run_t* r;

	if (awk->thr.lks != XP_NULL)
		awk->thr.lks->lock (awk, awk->thr.lks->custom_data);

	for (r = awk->run.ptr; r != XP_NULL; r = r->next)
	{
		r->exit_level = EXIT_ABORT;
	}

	if (awk->thr.lks != XP_NULL)
		awk->thr.lks->unlock (awk, awk->thr.lks->custom_data);
}

int xp_awk_getrunerrnum (xp_awk_t* awk, void* run, int* errnum)
{
	xp_awk_run_t* r;
	int n = 0;

	if (awk->thr.lks != XP_NULL)
		awk->thr.lks->lock (awk, awk->thr.lks->custom_data);

	for (r = awk->run.ptr; r != XP_NULL; r = r->next)
	{
		if (r == run)
		{
			xp_assert (r->awk == awk);
			*errnum = r->errnum;
			break;
		}
	}

	if (r == XP_NULL)
	{
		awk->errnum = XP_AWK_EINVAL;
		n = -1;
	}

	if (awk->thr.lks != XP_NULL)
		awk->thr.lks->unlock (awk, awk->thr.lks->custom_data);

	return n;
}

static void __free_namedval (void* run, void* val)
{
	xp_awk_refdownval ((xp_awk_run_t*)run, val);
}

static void __add_run (xp_awk_t* awk, xp_awk_run_t* run)
{
	if (awk->thr.lks != XP_NULL)
		awk->thr.lks->lock (awk, awk->thr.lks->custom_data);

	run->awk = awk;
	run->prev = XP_NULL;
	run->next = awk->run.ptr;
	if (run->next != XP_NULL) run->next->prev = run;
	awk->run.ptr = run;
	awk->run.count++;

	if (awk->thr.lks != XP_NULL)
		awk->thr.lks->unlock (awk, awk->thr.lks->custom_data);
}

static void __del_run (xp_awk_t* awk, xp_awk_run_t* run)
{
	if (awk->thr.lks != XP_NULL)
		awk->thr.lks->lock (awk, awk->thr.lks->custom_data);

	xp_assert (awk->run.ptr != XP_NULL);

	if (run->prev == XP_NULL)
	{
		awk->run.ptr = run->next;
		if (run->next != XP_NULL) run->next->prev = XP_NULL;
	}
	else
	{
		run->prev->next = run->next;
		if (run->next != XP_NULL) run->next->prev = run->prev;
	}

	run->awk = XP_NULL;
	awk->run.count--;

	if (awk->thr.lks != XP_NULL)
		awk->thr.lks->unlock (awk, awk->thr.lks->custom_data);
}

static int __init_run (
	xp_awk_run_t* run, xp_awk_runios_t* runios, int* errnum)
{
	run->stack = XP_NULL;
	run->stack_top = 0;
	run->stack_base = 0;
	run->stack_limit = 0;

	run->exit_level = EXIT_NONE;

	run->icache_count = 0;
	run->rcache_count = 0;

	run->errnum = XP_AWK_ENOERR;

	run->inrec.buf_pos = 0;
	run->inrec.buf_len = 0;
	run->inrec.flds = XP_NULL;
	run->inrec.nflds = 0;
	run->inrec.maxflds = 0;
	run->inrec.d0 = xp_awk_val_nil;
	if (xp_str_open (&run->inrec.line, DEF_BUF_CAPA) == XP_NULL)
	{
		*errnum = XP_AWK_ENOMEM; 
		return -1;
	}

	if (xp_awk_map_open (&run->named, 
		run, DEF_BUF_CAPA, __free_namedval) == XP_NULL) 
	{
		xp_str_close (&run->inrec.line);
		*errnum = XP_AWK_ENOMEM; 
		return -1;
	}

	run->pattern_range_state = (xp_byte_t*)
		xp_calloc (run->awk->tree.chain_size, xp_sizeof(xp_byte_t));
	if (run->pattern_range_state == XP_NULL)
	{
		xp_awk_map_close (&run->named);
		xp_str_close (&run->inrec.line);
		*errnum = XP_AWK_ENOMEM; 
		return -1;
	}

	run->extio.handler[XP_AWK_EXTIO_PIPE] = runios->pipe;
	run->extio.handler[XP_AWK_EXTIO_COPROC] = runios->coproc;
	run->extio.handler[XP_AWK_EXTIO_FILE] = runios->file;
	run->extio.handler[XP_AWK_EXTIO_CONSOLE] = runios->console;
	run->extio.chain = XP_NULL;

	return 0;
}

static void __deinit_run (xp_awk_run_t* run)
{
	xp_free (run->pattern_range_state);

	/* close all pending eio's */
	/* TODO: what if this operation fails? */
	xp_awk_clearextio (run);
	xp_assert (run->extio.chain == XP_NULL);

	/* destroy input record. __clear_record should be called
	 * before the run stack has been destroyed because it may try
	 * to change the value to XP_AWK_GLOBAL_NF. */
	__clear_record (run, xp_false); 
	if (run->inrec.flds != XP_NULL) 
	{
		xp_free (run->inrec.flds);
		run->inrec.flds = XP_NULL;
		run->inrec.maxflds = 0;
	}
	xp_str_close (&run->inrec.line);

	/* destroy run stack */
	if (run->stack != XP_NULL)
	{
		xp_assert (run->stack_top == 0);

		xp_free (run->stack);
		run->stack = XP_NULL;
		run->stack_top = 0;
		run->stack_base = 0;
		run->stack_limit = 0;
	}

	/* destroy named variables */
	xp_awk_map_close (&run->named);

	/* destroy values in free list */
	while (run->icache_count > 0)
	{
		xp_awk_val_int_t* tmp = run->icache[--run->icache_count];
		xp_awk_freeval (run, (xp_awk_val_t*)tmp, xp_false);
	}

	while (run->rcache_count > 0)
	{
		xp_awk_val_real_t* tmp = run->rcache[--run->rcache_count];
		xp_awk_freeval (run, (xp_awk_val_t*)tmp, xp_false);
	}
}

static int __run_main (xp_awk_run_t* run)
{
	xp_size_t nglobals, nargs, i;
	xp_size_t saved_stack_top;
	xp_awk_val_t* v;
	int n = 0;

	xp_assert (run->stack_base == 0 && run->stack_top == 0);

	/* secure space for global variables */
	saved_stack_top = run->stack_top;

	nglobals = run->awk->tree.nglobals;

	while (nglobals > 0)
	{
		--nglobals;
		if (__raw_push(run,xp_awk_val_nil) == -1)
		{
			/* restore the stack_top with the saved value
			 * instead of calling __raw_pop as many times as
			 * the successful __raw_push. it is ok because
			 * the values pushed so fare are all xp_awk_val_nil */
			run->stack_top = saved_stack_top;
			PANIC_I (run, XP_AWK_ENOMEM);
		}
	}	

	run->exit_level = EXIT_NONE;

	if (run->awk->option & XP_AWK_RUNMAIN)
	{
/* TODO: should the main function be user-specifiable? */
		xp_awk_nde_call_t nde;

		nde.type = XP_AWK_NDE_AFN;
		nde.next = XP_NULL;
		nde.what.afn.name = XP_T("main");
		nde.what.afn.name_len = 4;
		nde.args = XP_NULL;
		nde.nargs = 0;

		v = __eval_afn (run, (xp_awk_nde_t*)&nde);
		if (v == XP_NULL) n = -1;
		else
		{
			/* destroy the return value if necessary */
			xp_awk_refupval (v);
			xp_awk_refdownval (run, v);
		}
	}
	else
	{
		saved_stack_top = run->stack_top;
		if (__raw_push(run,(void*)run->stack_base) == -1) 
		{
			/* restore the stack top in a cheesy(?) way */
			run->stack_top = saved_stack_top;
			/* pops off global variables in a decent way */	
			/*__raw_pop_times (run, run->nglobals);*/
			__raw_pop_times (run, run->awk->tree.nglobals);
			PANIC_I (run, XP_AWK_ENOMEM);
		}

		if (__raw_push(run,(void*)saved_stack_top) == -1) 
		{
			run->stack_top = saved_stack_top;
			/*__raw_pop_times (run, run->nglobals);*/
			__raw_pop_times (run, run->awk->tree.nglobals);
			PANIC_I (run, XP_AWK_ENOMEM);
		}
	
		/* secure space for a return value */
		if (__raw_push(run,xp_awk_val_nil) == -1)
		{
			run->stack_top = saved_stack_top;
			/*__raw_pop_times (run, run->nglobals);*/
			__raw_pop_times (run, run->awk->tree.nglobals);
			PANIC_I (run, XP_AWK_ENOMEM);
		}
	
		/* secure space for nargs */
		if (__raw_push(run,xp_awk_val_nil) == -1)
		{
			run->stack_top = saved_stack_top;
			/*__raw_pop_times (run, run->nglobals);*/
			__raw_pop_times (run, run->awk->tree.nglobals);
			PANIC_I (run, XP_AWK_ENOMEM);
		}
	
		run->stack_base = saved_stack_top;
	
		/* set nargs to zero */
		nargs = 0;
		STACK_NARGS(run) = (void*)nargs;
	
		/* stack set up properly. ready to exeucte statement blocks */
		if (n == 0 && 
		    run->awk->tree.begin != XP_NULL && 
		    run->exit_level != EXIT_ABORT)
		{
			xp_awk_nde_blk_t* blk;

			blk = (xp_awk_nde_blk_t*)run->awk->tree.begin;
			xp_assert (blk->type == XP_AWK_NDE_BLK);

			run->active_block = blk;
			run->exit_level = EXIT_NONE;
			if (__run_block (run, blk) == -1) n = -1;
		}

		if (n == 0 && 
		    run->awk->tree.chain != XP_NULL && 
		    run->exit_level != EXIT_ABORT)
		{
			if (__run_pattern_blocks (run) == -1) n = -1;
		}

		if (n == 0 && 
		    run->awk->tree.end != XP_NULL && 
		    run->exit_level != EXIT_ABORT) 
		{
			xp_awk_nde_blk_t* blk;

			blk = (xp_awk_nde_blk_t*)run->awk->tree.end;
			xp_assert (blk->type == XP_AWK_NDE_BLK);

			run->active_block = blk;
			run->exit_level = EXIT_NONE;
			if (__run_block (run, blk) == -1) n = -1;
		}

		/* restore stack */
		nargs = (xp_size_t)STACK_NARGS(run);
		xp_assert (nargs == 0);
		for (i = 0; i < nargs; i++)
		{
			xp_awk_refdownval (run, STACK_ARG(run,i));
		}

		v = STACK_RETVAL(run);
xp_printf (XP_T("Return Value - "));
xp_awk_printval (v);
xp_printf (XP_T("\n"));
		/* the life of the global return value is over here
		 * unlike the return value of each function */
		/*xp_awk_refdownval_nofree (awk, v);*/
		xp_awk_refdownval (run, v);

		run->stack_top = 
			(xp_size_t)run->stack[run->stack_base+1];
		run->stack_base = 
			(xp_size_t)run->stack[run->stack_base+0];
	}

	/* pops off the global variables */
	nglobals = run->awk->tree.nglobals; /*run->nglobals */
	while (nglobals > 0)
	{
		--nglobals;
		xp_awk_refdownval (run, STACK_GLOBAL(run,nglobals));
		__raw_pop (run);
	}

	/* just reset the exit level */
	run->exit_level = EXIT_NONE;

xp_printf (XP_T("-[VARIABLES]------------------------\n"));
xp_awk_map_walk (&run->named, __printval, XP_NULL);
xp_printf (XP_T("-[END VARIABLES]--------------------------\n"));

	return n;
}

static int __run_pattern_blocks (xp_awk_run_t* run)
{
	xp_ssize_t n;
	xp_bool_t need_to_close = xp_false;
	int errnum;

	run->inrec.buf_pos = 0;
	run->inrec.buf_len = 0;
	run->inrec.eof = xp_false;

	/* run each pattern block */
	while (run->exit_level != EXIT_GLOBAL &&
	       run->exit_level != EXIT_ABORT)
	{
		int x;

		run->exit_level = EXIT_NONE;

		x = __read_record (run);
		if (x == -1)
		{
			/* don't care about the result of input close */
			xp_awk_closeextio_read (
				run, XP_AWK_IN_CONSOLE, XP_T(""), &errnum);
			return -1;
		}

		need_to_close = xp_true;
		if (x == 0) break; /* end of input */

		if (__run_pattern_block_chain (run, run->awk->tree.chain) == -1)
		{
			xp_awk_closeextio_read (
				run, XP_AWK_IN_CONSOLE, XP_T(""), &errnum);
			return -1;
		}
	}

	/* In case of getline, the code would make getline return -1, 
	 * set ERRNO, make this function return 0 after having checked 
	 * if closextio has returned -1 and errnum has been set to 
	 * XP_AWK_ENOERR. But this part of the code ends the input for 
	 * the implicit pattern-block loop, which is totally different 
	 * from getline. so it returns -1 as long as closeextio returns 
	 * -1 regardless of the value of errnum.  */
	if (need_to_close)
	{
		n = xp_awk_closeextio_read (
			run, XP_AWK_IN_CONSOLE, XP_T(""), &errnum);
		if (n == -1) 
		{
			if (errnum == XP_AWK_ENOERR)
				PANIC_I (run, XP_AWK_ECONINCLOSE);
			else
				PANIC_I (run, errnum);
		}
	}

	return 0;
}

static int __run_pattern_block_chain (xp_awk_run_t* run, xp_awk_chain_t* chain)
{
	xp_size_t block_no = 0;

	while (run->exit_level != EXIT_GLOBAL &&
	       run->exit_level != EXIT_ABORT && chain != XP_NULL)
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
	xp_awk_run_t* run, xp_awk_chain_t* chain, xp_size_t block_no)
{
	xp_awk_nde_t* ptn;
	xp_awk_nde_blk_t* blk;

	ptn = chain->pattern;
	blk = (xp_awk_nde_blk_t*)chain->action;

	if (ptn == XP_NULL)
	{
		/* just execute the block */
		run->active_block = blk;
		if (__run_block (run, blk) == -1) return -1;
	}
	else
	{
		if (ptn->next == XP_NULL)
		{
			/* pattern { ... } */
			xp_awk_val_t* v1;

			v1 = __eval_expression (run, ptn);
			if (v1 == XP_NULL) return -1;

			xp_awk_refupval (v1);

			if (xp_awk_valtobool(v1))
			{
				run->active_block = blk;
				if (__run_block (run, blk) == -1) 
				{
					xp_awk_refdownval (run, v1);
					return -1;
				}
			}

			xp_awk_refdownval (run, v1);
		}
		else
		{
			/* pattern, pattern { ... } */
			xp_assert (ptn->next->next == XP_NULL);

			if (run->pattern_range_state[block_no] == 0)
			{
				xp_awk_val_t* v1;

				v1 = __eval_expression (run, ptn);
				if (v1 == XP_NULL) return -1;
				xp_awk_refupval (v1);

				if (xp_awk_valtobool(v1))
				{
					run->active_block = blk;
					if (__run_block (run, blk) == -1) 
					{
						xp_awk_refdownval (run, v1);
						return -1;
					}

					run->pattern_range_state[block_no] = 1;
				}

				xp_awk_refdownval (run, v1);
			}
			else if (run->pattern_range_state[block_no] == 1)
			{
				xp_awk_val_t* v2;

				v2 = __eval_expression (run, ptn->next);
				if (v2 == XP_NULL) return -1;
				xp_awk_refupval (v2);

				run->active_block = blk;
				if (__run_block (run, blk) == -1) 
				{
					xp_awk_refdownval (run, v2);
					return -1;
				}

				if (xp_awk_valtobool(v2)) 
					run->pattern_range_state[block_no] = 0;

				xp_awk_refdownval (run, v2);
			}
		}
	}

	return 0;
}

static int __run_block (xp_awk_run_t* run, xp_awk_nde_blk_t* nde)
{
	xp_awk_nde_t* p;
	xp_size_t nlocals;
	xp_size_t saved_stack_top;
	int n = 0;

	if (nde == XP_NULL)
	{
		/* blockless pattern - execute print $0*/
		int errnum;

		xp_awk_refupval (run->inrec.d0);

		n = xp_awk_writeextio_nl (run, 
			XP_AWK_OUT_CONSOLE, XP_T(""), run->inrec.d0, &errnum);
		if (n == -1)
		{
			xp_awk_refdownval (run, run->inrec.d0);

			if (errnum == XP_AWK_ENOERR)
				PANIC_I (run, XP_AWK_ECONOUTDATA);
			else
				PANIC_I (run, errnum);
		}

		xp_awk_refdownval (run, run->inrec.d0);

		return 0;
	}

	xp_assert (nde->type == XP_AWK_NDE_BLK);

	p = nde->body;
	nlocals = nde->nlocals;

/*xp_printf (XP_T("securing space for local variables nlocals = %d\n"), nlocals);*/
	saved_stack_top = run->stack_top;

	/* secure space for local variables */
	while (nlocals > 0)
	{
		--nlocals;
		if (__raw_push(run,xp_awk_val_nil) == -1)
		{
			/* restore stack top */
			run->stack_top = saved_stack_top;
			return -1;
		}

		/* refupval is not required for xp_awk_val_nil */
	}

/*xp_printf (XP_T("executing block statements\n"));*/
	while (p != XP_NULL && run->exit_level == EXIT_NONE) 
	{
/*xp_printf (XP_T("running a statement\n"));*/
		if (__run_statement(run,p) == -1) 
		{
			n = -1;
			break;
		}
		p = p->next;
	}

/*xp_printf (XP_T("popping off local variables\n"));*/
	/* pop off local variables */
	nlocals = nde->nlocals;
	while (nlocals > 0)
	{
		--nlocals;
		xp_awk_refdownval (run, STACK_LOCAL(run,nlocals));
		__raw_pop (run);
	}

	return n;
}

static int __run_statement (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	switch (nde->type) 
	{
		case XP_AWK_NDE_NULL:
		{
			/* do nothing */
			break;
		}

		case XP_AWK_NDE_BLK:
		{
			if (__run_block (run, 
				(xp_awk_nde_blk_t*)nde) == -1) return -1;
			break;
		}

		case XP_AWK_NDE_IF:
		{
			if (__run_if (run, 
				(xp_awk_nde_if_t*)nde) == -1) return -1;	
			break;
		}

		case XP_AWK_NDE_WHILE:
		case XP_AWK_NDE_DOWHILE:
		{
			if (__run_while (run, 
				(xp_awk_nde_while_t*)nde) == -1) return -1;
			break;
		}

		case XP_AWK_NDE_FOR:
		{
			if (__run_for (run, 
				(xp_awk_nde_for_t*)nde) == -1) return -1;
			break;
		}

		case XP_AWK_NDE_FOREACH:
		{
			if (__run_foreach (run, 
				(xp_awk_nde_foreach_t*)nde) == -1) return -1;
			break;
		}

		case XP_AWK_NDE_BREAK:
		{
			if (__run_break (run, 
				(xp_awk_nde_break_t*)nde) == -1) return -1;
			break;
		}

		case XP_AWK_NDE_CONTINUE:
		{
			if (__run_continue (run, 
				(xp_awk_nde_continue_t*)nde) == -1) return -1;
			break;
		}

		case XP_AWK_NDE_RETURN:
		{
			if (__run_return (run, 
				(xp_awk_nde_return_t*)nde) == -1) return -1;
			break;
		}

		case XP_AWK_NDE_EXIT:
		{
			if (__run_exit (run, 
				(xp_awk_nde_exit_t*)nde) == -1) return -1;
			break;
		}

		case XP_AWK_NDE_NEXT:
		{
			if (__run_next (run, 
				(xp_awk_nde_next_t*)nde) == -1) return -1;
			break;
		}

		case XP_AWK_NDE_NEXTFILE:
		{
			if (__run_nextfile (run, 
				(xp_awk_nde_nextfile_t*)nde) == -1) return -1;
			break;
		}

		case XP_AWK_NDE_DELETE:
		{
			if (__run_delete (run, 
				(xp_awk_nde_delete_t*)nde) == -1) return -1;
			break;
		}

		case XP_AWK_NDE_PRINT:
		{
			if (__run_print (run, 
				(xp_awk_nde_print_t*)nde) == -1) return -1;
			break;
		}

		default:
		{
			xp_awk_val_t* v;
			v = __eval_expression(run,nde);
			if (v == XP_NULL) return -1;

			xp_awk_refupval (v);
			xp_awk_refdownval (run, v);

			break;
		}
	}

	return 0;
}

static int __run_if (xp_awk_run_t* run, xp_awk_nde_if_t* nde)
{
	xp_awk_val_t* test;
	int n = 0;

	/* the test expression for the if statement cannot have 
	 * chained expressions. this should not be allowed by the
	 * parser first of all */
	xp_assert (nde->test->next == XP_NULL);

	test = __eval_expression (run, nde->test);
	if (test == XP_NULL) return -1;

	xp_awk_refupval (test);
	if (xp_awk_valtobool(test))
	{
		n = __run_statement (run, nde->then_part);
	}
	else if (nde->else_part != XP_NULL)
	{
		n = __run_statement (run, nde->else_part);
	}

	xp_awk_refdownval (run, test); /* TODO: is this correct?*/
	return n;
}

static int __run_while (xp_awk_run_t* run, xp_awk_nde_while_t* nde)
{
	xp_awk_val_t* test;

	if (nde->type == XP_AWK_NDE_WHILE)
	{
		/* no chained expressions are allowed for the test 
		 * expression of the while statement */
		xp_assert (nde->test->next == XP_NULL);

		/* TODO: handle run-time abortion... */
		while (1)
		{
			test = __eval_expression (run, nde->test);
			if (test == XP_NULL) return -1;

			xp_awk_refupval (test);

			if (xp_awk_valtobool(test))
			{
				if (__run_statement(run,nde->body) == -1)
				{
					xp_awk_refdownval (run, test);
					return -1;
				}
			}
			else
			{
				xp_awk_refdownval (run, test);
				break;
			}

			xp_awk_refdownval (run, test);

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
	else if (nde->type == XP_AWK_NDE_DOWHILE)
	{
		/* no chained expressions are allowed for the test 
		 * expression of the while statement */
		xp_assert (nde->test->next == XP_NULL);

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
			if (test == XP_NULL) return -1;

			xp_awk_refupval (test);

			if (!xp_awk_valtobool(test))
			{
				xp_awk_refdownval (run, test);
				break;
			}

			xp_awk_refdownval (run, test);
		}
		while (1);
	}

	return 0;
}

static int __run_for (xp_awk_run_t* run, xp_awk_nde_for_t* nde)
{
	xp_awk_val_t* val;

	if (nde->init != XP_NULL)
	{
		xp_assert (nde->init->next == XP_NULL);
		val = __eval_expression(run,nde->init);
		if (val == XP_NULL) return -1;

		xp_awk_refupval (val);
		xp_awk_refdownval (run, val);
	}

	while (1)
	{
		if (nde->test != XP_NULL)
		{
			xp_awk_val_t* test;

			/* no chained expressions for the test expression of
			 * the for statement are allowed */
			xp_assert (nde->test->next == XP_NULL);

			test = __eval_expression (run, nde->test);
			if (test == XP_NULL) return -1;

			xp_awk_refupval (test);
			if (xp_awk_valtobool(test))
			{
				if (__run_statement(run,nde->body) == -1)
				{
					xp_awk_refdownval (run, test);
					return -1;
				}
			}
			else
			{
				xp_awk_refdownval (run, test);
				break;
			}

			xp_awk_refdownval (run, test);
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

		if (nde->incr != XP_NULL)
		{
			xp_assert (nde->incr->next == XP_NULL);
			val = __eval_expression(run,nde->incr);
			if (val == XP_NULL) return -1;

			xp_awk_refupval (val);
			xp_awk_refdownval (run, val);
		}
	}

	return 0;
}

struct __foreach_walker_t
{
	xp_awk_run_t* run;
	xp_awk_nde_t* var;
	xp_awk_nde_t* body;
};

static int __walk_foreach (xp_awk_pair_t* pair, void* arg)
{
	struct __foreach_walker_t* w = (struct __foreach_walker_t*)arg;
	xp_awk_val_t* str;

	str = (xp_awk_val_t*)xp_awk_makestrval(pair->key,xp_strlen(pair->key));
	if (str == XP_NULL) PANIC_I (w->run, XP_AWK_ENOMEM);

	xp_awk_refupval (str);
	if (__do_assignment (w->run, w->var, str) == XP_NULL)
	{
		xp_awk_refdownval (w->run, str);
		return -1;
	}

	if (__run_statement (w->run, w->body) == -1)
	{
		xp_awk_refdownval (w->run, str);
		return -1;
	}
	
	xp_awk_refdownval (w->run, str);
	return 0;
}

static int __run_foreach (xp_awk_run_t* run, xp_awk_nde_foreach_t* nde)
{
	int n;
	xp_awk_nde_exp_t* test;
	xp_awk_val_t* rv;
	xp_awk_map_t* map;
	struct __foreach_walker_t walker;

	test = (xp_awk_nde_exp_t*)nde->test;
	xp_assert (test->type == XP_AWK_NDE_EXP_BIN && 
	           test->opcode == XP_AWK_BINOP_IN);

	/* chained expressions should not be allowed 
	 * by the parser first of all */
	xp_assert (test->right->next == XP_NULL); 

	rv = __eval_expression (run, test->right);
	if (rv == XP_NULL) return -1;

	xp_awk_refupval (rv);
	if (rv->type != XP_AWK_VAL_MAP)
	{
		xp_awk_refdownval (run, rv);
		PANIC_I (run, XP_AWK_ENOTINDEXABLE);
	}
	map = ((xp_awk_val_map_t*)rv)->map;

	walker.run = run;
	walker.var = test->left;
	walker.body = nde->body;
	n = xp_awk_map_walk (map, __walk_foreach, &walker);

	xp_awk_refdownval (run, rv);
	return n;
}

static int __run_break (xp_awk_run_t* run, xp_awk_nde_break_t* nde)
{
	run->exit_level = EXIT_BREAK;
	return 0;
}

static int __run_continue (xp_awk_run_t* run, xp_awk_nde_continue_t* nde)
{
	run->exit_level = EXIT_CONTINUE;
	return 0;
}

static int __run_return (xp_awk_run_t* run, xp_awk_nde_return_t* nde)
{
	if (nde->val != XP_NULL)
	{
		xp_awk_val_t* val;

		/* chained expressions should not be allowed 
		 * by the parser first of all */
		xp_assert (nde->val->next == XP_NULL); 

/*xp_printf (XP_T("returning....\n"));*/
		val = __eval_expression(run, nde->val);
		if (val == XP_NULL) return -1;

		xp_awk_refdownval (run, STACK_RETVAL(run));
		STACK_RETVAL(run) = val;
		xp_awk_refupval (val); /* see __eval_call for the trick */
/*xp_printf (XP_T("set return value....\n"));*/
	}
	
	run->exit_level = EXIT_FUNCTION;
	return 0;
}

static int __run_exit (xp_awk_run_t* run, xp_awk_nde_exit_t* nde)
{
	if (nde->val != XP_NULL)
	{
		xp_awk_val_t* val;

		/* chained expressions should not be allowed 
		 * by the parser first of all */
		xp_assert (nde->val->next == XP_NULL); 

		val = __eval_expression(run, nde->val);
		if (val == XP_NULL) return -1;

		xp_awk_refdownval (run, STACK_RETVAL_GLOBAL(run));
		STACK_RETVAL_GLOBAL(run) = val; /* global return value */

		xp_awk_refupval (val);
	}

	run->exit_level = EXIT_GLOBAL;
	return 0;
}

static int __run_next (xp_awk_run_t* run, xp_awk_nde_next_t* nde)
{
	/* the parser checks if next has been called in the begin/end
	 * block or whereever inappropriate. so the runtime doesn't 
	 * check that explicitly */

	if  (run->active_block == (xp_awk_nde_blk_t*)run->awk->tree.begin ||
	     run->active_block == (xp_awk_nde_blk_t*)run->awk->tree.end)
	{
		PANIC_I (run, XP_AWK_ENEXTCALL);
	}

	run->exit_level = EXIT_NEXT;
	return 0;
}

static int __run_nextfile (xp_awk_run_t* run, xp_awk_nde_nextfile_t* nde)
{
/* TODO: some extentions such as nextfile "in/out"; 
 *  what about awk -i in1,in2,in3 -o out1,out2,out3 ?
 */
	int n, errnum;

	if  (run->active_block == (xp_awk_nde_blk_t*)run->awk->tree.begin ||
	     run->active_block == (xp_awk_nde_blk_t*)run->awk->tree.end)
	{
		PANIC_I (run, XP_AWK_ENEXTFILECALL);
	}

	n = xp_awk_nextextio_read (
		run, XP_AWK_IN_CONSOLE, XP_T(""), &errnum);
	if (n == -1)
	{
		if (errnum == XP_AWK_ENOERR)
			PANIC_I (run, XP_AWK_ECONINNEXT);
		else
			PANIC_I (run, errnum);
	}

	if (n == 0)
	{
		/* no more input file */
		run->exit_level = EXIT_GLOBAL;
		return 0;
	}

/* TODO: update FILENAME, ARGIND. reset FNR to 1.
 * some significant changes will be required to do this */
/* Consider using FILENAME_IN and FILENAME_OUT to accomplish nextfile in/out */
	run->exit_level = EXIT_NEXT;
	return 0;
}

static int __run_delete (xp_awk_run_t* run, xp_awk_nde_delete_t* nde)
{
	xp_awk_nde_var_t* var;

	var = (xp_awk_nde_var_t*) nde->var;

	if (var->type == XP_AWK_NDE_NAMED ||
	    var->type == XP_AWK_NDE_NAMEDIDX)
	{
		xp_awk_pair_t* pair;

		xp_assert ((var->type == XP_AWK_NDE_NAMED && 
		            var->idx == XP_NULL) ||
		           (var->type == XP_AWK_NDE_NAMEDIDX && 
		            var->idx != XP_NULL));

		pair = xp_awk_map_get (
			&run->named, var->id.name, var->id.name_len);
		if (pair == XP_NULL)
		{
			xp_awk_val_t* tmp;

			/* value not set for the named variable. 
			 * create a map and assign it to the variable */

			tmp = xp_awk_makemapval (run);
			if (tmp == XP_NULL) PANIC_I (run, XP_AWK_ENOMEM);

			if (xp_awk_map_put (&run->named, 
				var->id.name, var->id.name_len, tmp) == XP_NULL)
			{
				xp_awk_refupval (tmp);
				xp_awk_refdownval (run, tmp);
				PANIC_I (run, XP_AWK_ENOMEM);		
			}

			xp_awk_refupval (tmp);
		}
		else
		{
			xp_awk_val_t* val;
			xp_awk_map_t* map;

			val = (xp_awk_val_t*)pair->val;
			xp_assert (val != XP_NULL);

			if (val->type != XP_AWK_VAL_MAP)
				PANIC_I (run, XP_AWK_ENOTDELETABLE);

			map = ((xp_awk_val_map_t*)val)->map;
			if (var->type == XP_AWK_NDE_NAMEDIDX)
			{
				xp_char_t* key;
				xp_size_t key_len;
				xp_awk_val_t* idx;
				int errnum;

				xp_assert (var->idx != XP_NULL);

				idx = __eval_expression (run, var->idx);
				if (idx == XP_NULL) return -1;

				xp_awk_refupval (idx);
				key = xp_awk_valtostr (idx, 
					&errnum, xp_true, XP_NULL, &key_len);
				xp_awk_refdownval (run, idx);

				if (key == XP_NULL) PANIC_I (run, errnum);

				xp_awk_map_remove (map, key, key_len);
				xp_free (key);
			}
			else
			{
				xp_awk_map_clear (map);
			}
		}
	}
	else if (var->type == XP_AWK_NDE_GLOBAL ||
	         var->type == XP_AWK_NDE_LOCAL ||
	         var->type == XP_AWK_NDE_ARG ||
	         var->type == XP_AWK_NDE_GLOBALIDX ||
	         var->type == XP_AWK_NDE_LOCALIDX ||
	         var->type == XP_AWK_NDE_ARGIDX)
	{
		xp_awk_val_t* val;

		if (var->type == XP_AWK_NDE_GLOBAL ||
		    var->type == XP_AWK_NDE_GLOBALIDX)
			val = STACK_GLOBAL (run,var->id.idxa);
		else if (var->type == XP_AWK_NDE_LOCAL ||
		         var->type == XP_AWK_NDE_LOCALIDX)
			val = STACK_LOCAL (run,var->id.idxa);
		else val = STACK_ARG (run,var->id.idxa);

		xp_assert (val != XP_NULL);

		if (val->type == XP_AWK_VAL_NIL)
		{
			xp_awk_val_t* tmp;

			/* value not set for the named variable. 
			 * create a map and assign it to the variable */

			tmp = xp_awk_makemapval (run);
			if (tmp == XP_NULL) PANIC_I (run, XP_AWK_ENOMEM);

			/* no need to reduce the reference count of
			 * the previous value because it was nil. */
			if (var->type == XP_AWK_NDE_GLOBAL ||
			    var->type == XP_AWK_NDE_GLOBALIDX)
				STACK_GLOBAL(run,var->id.idxa) = tmp;
			else if (var->type == XP_AWK_NDE_LOCAL ||
			         var->type == XP_AWK_NDE_LOCALIDX)
				STACK_LOCAL(run,var->id.idxa) = tmp;
			else STACK_ARG(run,var->id.idxa) = tmp;

			xp_awk_refupval (tmp);
		}
		else
		{
			xp_awk_map_t* map;

			if (val->type != XP_AWK_VAL_MAP)
				PANIC_I (run, XP_AWK_ENOTDELETABLE);

			map = ((xp_awk_val_map_t*)val)->map;
			if (var->type == XP_AWK_NDE_GLOBALIDX ||
			    var->type == XP_AWK_NDE_LOCALIDX ||
			    var->type == XP_AWK_NDE_ARGIDX)
			{
				xp_char_t* key;
				xp_size_t key_len;
				xp_awk_val_t* idx;
				int errnum;

				xp_assert (var->idx != XP_NULL);

				idx = __eval_expression (run, var->idx);
				if (idx == XP_NULL) return -1;

				xp_awk_refupval (idx);
				key = xp_awk_valtostr (idx, 
					&errnum, xp_true, XP_NULL, &key_len);
				xp_awk_refdownval (run, idx);

				if (key == XP_NULL) PANIC_I (run, errnum);

				xp_awk_map_remove (map, key, key_len);
				xp_free (key);
			}
			else
			{
				xp_awk_map_clear (map);
			}
		}
	}
	else
	{
		xp_assert (!"should never happen - wrong variable type for delete");
		PANIC_I (run, XP_AWK_EINTERNAL);
	}

	return 0;
}

static int __run_print (xp_awk_run_t* run, xp_awk_nde_print_t* nde)
{
	xp_awk_nde_print_t* p = (xp_awk_nde_print_t*)nde;
	xp_char_t* out = XP_NULL;
	const xp_char_t* dst;
	xp_awk_val_t* v;
	xp_awk_nde_t* np;
	int errnum, n;

	xp_assert (
		(p->out_type == XP_AWK_OUT_PIPE && p->out != XP_NULL) ||
		(p->out_type == XP_AWK_OUT_COPROC && p->out != XP_NULL) ||
		(p->out_type == XP_AWK_OUT_FILE && p->out != XP_NULL) ||
		(p->out_type == XP_AWK_OUT_FILE_APPEND && p->out != XP_NULL) ||
		(p->out_type == XP_AWK_OUT_CONSOLE && p->out == XP_NULL));

	if (p->out != XP_NULL)
	{
		xp_size_t len;

		v = __eval_expression (run, p->out);
		if (v == XP_NULL) return -1;

		xp_awk_refupval (v);
		out = xp_awk_valtostr (v, &errnum, xp_true, XP_NULL, &len);
		if (out == XP_NULL) 
		{
			xp_awk_refdownval (run, v);
			PANIC_I (run, errnum);
		}
		xp_awk_refdownval (run, v);

		if (len <= 0) 
		{
			/* the output destination name is empty. */
			xp_free (out);
			n = -1;
			goto skip_write;
		}

		while (len > 0)
		{
			if (out[--len] == XP_T('\0'))
			{
				/* the output destination name contains a null 
				 * character. */
				xp_free (out);
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

	dst = (out == XP_NULL)? XP_T(""): out;

	if (p->args == XP_NULL)
	{
		v = run->inrec.d0;

		xp_awk_refupval (v);
		n = xp_awk_writeextio (run, p->out_type, dst, v, &errnum);
		if (n < 0 && errnum != XP_AWK_ENOERR) 
		{
			if (out != XP_NULL) xp_free (out);
			xp_awk_refdownval (run, v);
			PANIC_I (run, errnum);
		}
		xp_awk_refdownval (run, v);
		/* TODO: how to handle n == -1 && errnum == XP_AWK_ENOERR. that is the user handler returned an error... */
	}
	else
	{
		for (np = p->args; np != XP_NULL; np = np->next)
		{
			v = __eval_expression (run, np);
			if (v == XP_NULL) 
			{
				if (out != XP_NULL) xp_free (out);
				return -1;
			}
			xp_awk_refupval (v);

			n = xp_awk_writeextio (
				run, p->out_type, dst, v, &errnum);
			if (n < 0 && errnum != XP_AWK_ENOERR) 
			{
				if (out != XP_NULL) xp_free (out);
				xp_awk_refdownval (run, v);
				PANIC_I (run, errnum);
			}
			xp_awk_refdownval (run, v);

			/* TODO: how to handle n == -1 && errnum == XP_AWK_ENOERR. that is the user handler returned an error... */

			/* TODO: print proper field separator */
		}
	}

	/* TODO: change xp_awk_val_nil to 
	 *       xp_awk_val_empty_string or something */
	n = xp_awk_writeextio_nl (
		run, p->out_type, dst, xp_awk_val_nil, &errnum);
	if (n < 0 && errnum != XP_AWK_ENOERR)
	{
		if (out != XP_NULL) xp_free (out);
		PANIC_I (run, errnum);
	}

	/* TODO: how to handle n == -1 && errnum == XP_AWK_ENOERR. that is the user handler returned an error... */

	if (out != XP_NULL) xp_free (out);

skip_write:
	return 0;
}

static xp_awk_val_t* __eval_expression (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	xp_awk_val_t* v;
	int n, errnum;

	v = __eval_expression0 (run, nde);
	if (v == XP_NULL) return XP_NULL;

	if (v->type == XP_AWK_VAL_REX)
	{
		xp_assert (run->inrec.d0->type == XP_AWK_VAL_STR);

		xp_awk_refupval (v);
		n = xp_awk_matchrex (
			((xp_awk_val_rex_t*)v)->code,
			((xp_awk_val_str_t*)run->inrec.d0)->buf,
			((xp_awk_val_str_t*)run->inrec.d0)->len,
			XP_NULL, XP_NULL, &errnum);

		if (n == -1) 
		{
			xp_awk_refdownval (run, v);
			PANIC (run, errnum);
		}

		xp_awk_refdownval (run, v);

		v = xp_awk_makeintval (run, (n != 0));
		if (v == XP_NULL) PANIC (run, XP_AWK_ENOMEM);
	}

	return v;
}

static xp_awk_val_t* __eval_expression0 (xp_awk_run_t* run, xp_awk_nde_t* nde)
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

	xp_assert (nde->type >= XP_AWK_NDE_GRP &&
		(nde->type - XP_AWK_NDE_GRP) < xp_countof(__eval_func));

	return __eval_func[nde->type-XP_AWK_NDE_GRP] (run, nde);
}

static xp_awk_val_t* __eval_group (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	/* __eval_binop_in evaluates the XP_AWK_NDE_GRP specially.
	 * so this function should never be reached. */
	xp_assert (!"should never happen - NDE_GRP only for in");
	PANIC (run, XP_AWK_EINTERNAL);
	return XP_NULL;
}

static xp_awk_val_t* __eval_assignment (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	xp_awk_val_t* val, * ret;
	xp_awk_nde_ass_t* ass = (xp_awk_nde_ass_t*)nde;

	xp_assert (ass->left != XP_NULL && ass->right != XP_NULL);

	xp_assert (ass->right->next == XP_NULL);
	val = __eval_expression (run, ass->right);
	if (val == XP_NULL) return XP_NULL;

	xp_awk_refupval (val);

	if (ass->opcode != XP_AWK_ASSOP_NONE)
	{
		xp_awk_val_t* val2, * tmp;

		xp_assert (ass->left->next == XP_NULL);
		val2 = __eval_expression (run, ass->left);
		if (val2 == XP_NULL)
		{
			xp_awk_refdownval (run, val);
			return XP_NULL;
		}

		xp_awk_refupval (val2);

		if (ass->opcode == XP_AWK_ASSOP_PLUS)
		{
			tmp = __eval_binop_plus (run, val2, val);
		}
		else if (ass->opcode == XP_AWK_ASSOP_MINUS)
		{
			tmp = __eval_binop_minus (run, val2, val);
		}
		else if (ass->opcode == XP_AWK_ASSOP_MUL)
		{
			tmp = __eval_binop_mul (run, val2, val);
		}
		else if (ass->opcode == XP_AWK_ASSOP_DIV)
		{
			tmp = __eval_binop_div (run, val2, val);
		}
		else if (ass->opcode == XP_AWK_ASSOP_MOD)
		{
			tmp = __eval_binop_mod (run, val2, val);
		}
		else if (ass->opcode == XP_AWK_ASSOP_EXP)
		{
			tmp = __eval_binop_exp (run, val2, val);
		}
		else
		{
			xp_assert (!"should never happen - invalid assignment opcode");
			PANIC (run, XP_AWK_EINTERNAL);
		}

		if (tmp == XP_NULL)
		{
			xp_awk_refdownval (run, val);
			xp_awk_refdownval (run, val2);
			return XP_NULL;
		}

		xp_awk_refdownval (run, val);
		val = tmp;
		xp_awk_refupval (val);
	}

	ret = __do_assignment (run, ass->left, val);
	xp_awk_refdownval (run, val);

	return ret;
}

static xp_awk_val_t* __do_assignment (
	xp_awk_run_t* run, xp_awk_nde_t* var, xp_awk_val_t* val)
{
	xp_awk_val_t* ret;

	if (val->type == XP_AWK_VAL_MAP)
	{
		/* a map cannot be assigned to a variable */
		PANIC (run, XP_AWK_ENOTASSIGNABLE);
	}

	if (var->type == XP_AWK_NDE_NAMED ||
	    var->type == XP_AWK_NDE_GLOBAL ||
	    var->type == XP_AWK_NDE_LOCAL ||
	    var->type == XP_AWK_NDE_ARG) 
	{
		ret = __do_assignment_scalar (run, (xp_awk_nde_var_t*)var, val);
	}
	else if (var->type == XP_AWK_NDE_NAMEDIDX ||
	         var->type == XP_AWK_NDE_GLOBALIDX ||
	         var->type == XP_AWK_NDE_LOCALIDX ||
	         var->type == XP_AWK_NDE_ARGIDX) 
	{
		ret = __do_assignment_map (run, (xp_awk_nde_var_t*)var, val);
	}
	else if (var->type == XP_AWK_NDE_POS)
	{
		ret = __do_assignment_pos (run, (xp_awk_nde_pos_t*)var, val);
	}
	else
	{
		xp_assert (!"should never happen - invalid variable type");
		PANIC (run, XP_AWK_EINTERNAL);
	}

	return ret;
}

static xp_awk_val_t* __do_assignment_scalar (
	xp_awk_run_t* run, xp_awk_nde_var_t* var, xp_awk_val_t* val)
{
	xp_assert (
		(var->type == XP_AWK_NDE_NAMED ||
		 var->type == XP_AWK_NDE_GLOBAL ||
		 var->type == XP_AWK_NDE_LOCAL ||
		 var->type == XP_AWK_NDE_ARG) && var->idx == XP_NULL);

	xp_assert (val->type != XP_AWK_VAL_MAP);

	if (var->type == XP_AWK_NDE_NAMED) 
	{
		xp_awk_pair_t* pair;
		int n;

		pair = xp_awk_map_get (
			&run->named, var->id.name, var->id.name_len);
		if (pair != XP_NULL && 
		    ((xp_awk_val_t*)pair->val)->type == XP_AWK_VAL_MAP)
		{
			/* once a variable becomes an array,
			 * it cannot be changed to a scalar variable */
			PANIC (run, XP_AWK_ENOTSCALARIZABLE);
		}

		n = xp_awk_map_putx (&run->named, 
			var->id.name, var->id.name_len, val, XP_NULL);
		if (n < 0) PANIC (run, XP_AWK_ENOMEM);

		xp_awk_refupval (val);
	}
	else if (var->type == XP_AWK_NDE_GLOBAL) 
	{
		xp_awk_val_t* old = STACK_GLOBAL(run,var->id.idxa);
		if (old->type == XP_AWK_VAL_MAP)
		{	
			/* once a variable becomes an array,
			 * it cannot be changed to a scalar variable */
			PANIC (run, XP_AWK_ENOTSCALARIZABLE);
		}

/* TODO: if var->id.idxa == XP_AWK_GLOBAL_NF recompute $0, etc */
		xp_awk_refdownval (run, old);
		STACK_GLOBAL(run,var->id.idxa) = val;
		xp_awk_refupval (val);
	}
	else if (var->type == XP_AWK_NDE_LOCAL) 
	{
		xp_awk_val_t* old = STACK_LOCAL(run,var->id.idxa);
		if (old->type == XP_AWK_VAL_MAP)
		{	
			/* once the variable becomes an array,
			 * it cannot be changed to a scalar variable */
			PANIC (run, XP_AWK_ENOTSCALARIZABLE);
		}

		xp_awk_refdownval (run, old);
		STACK_LOCAL(run,var->id.idxa) = val;
		xp_awk_refupval (val);
	}
	else /* if (var->type == XP_AWK_NDE_ARG) */
	{
		xp_awk_val_t* old = STACK_ARG(run,var->id.idxa);
		if (old->type == XP_AWK_VAL_MAP)
		{	
			/* once the variable becomes an array,
			 * it cannot be changed to a scalar variable */
			PANIC (run, XP_AWK_ENOTSCALARIZABLE);
		}

		xp_awk_refdownval (run, old);
		STACK_ARG(run,var->id.idxa) = val;
		xp_awk_refupval (val);
	}

	return val;
}

static xp_awk_val_t* __do_assignment_map (
	xp_awk_run_t* run, xp_awk_nde_var_t* var, xp_awk_val_t* val)
{
	xp_awk_val_map_t* map;
	xp_char_t* str;
	xp_size_t len;
	int n;

	xp_assert (
		(var->type == XP_AWK_NDE_NAMEDIDX ||
		 var->type == XP_AWK_NDE_GLOBALIDX ||
		 var->type == XP_AWK_NDE_LOCALIDX ||
		 var->type == XP_AWK_NDE_ARGIDX) && var->idx != XP_NULL);
	xp_assert (val->type != XP_AWK_VAL_MAP);

	if (var->type == XP_AWK_NDE_NAMEDIDX)
	{
		xp_awk_pair_t* pair;
		pair = xp_awk_map_get (
			&run->named, var->id.name, var->id.name_len);
		map = (pair == XP_NULL)? 
			(xp_awk_val_map_t*)xp_awk_val_nil: 
			(xp_awk_val_map_t*)pair->val;
	}
	else
	{
		map = (var->type == XP_AWK_NDE_GLOBALIDX)? 
		      	(xp_awk_val_map_t*)STACK_GLOBAL(run,var->id.idxa):
		      (var->type == XP_AWK_NDE_LOCALIDX)? 
		      	(xp_awk_val_map_t*)STACK_LOCAL(run,var->id.idxa):
		      	(xp_awk_val_map_t*)STACK_ARG(run,var->id.idxa);
	}

	if (map->type == XP_AWK_VAL_NIL)
	{
		/* the map is not initialized yet */
		xp_awk_val_t* tmp;

		tmp = xp_awk_makemapval (run);
		if (tmp == XP_NULL) PANIC (run, XP_AWK_ENOMEM);

		if (var->type == XP_AWK_NDE_NAMEDIDX)
		{
			/* doesn't have to decrease the reference count 
			 * of the previous value here as it is done by 
			 * xp_awk_map_put */
			if (xp_awk_map_put (&run->named, 
				var->id.name, var->id.name_len, tmp) == XP_NULL)
			{
				xp_awk_refupval (tmp);
				xp_awk_refdownval (run, tmp);
				PANIC (run, XP_AWK_ENOMEM);		
			}
		}
		else if (var->type == XP_AWK_NDE_GLOBALIDX)
		{
			/* decrease the reference count of the previous value.
			 * in fact, this is not necessary as map is always 
			 * xp_awk_val_nil here. */
			xp_awk_refdownval (run, (xp_awk_val_t*)map);
			STACK_GLOBAL(run,var->id.idxa) = tmp;
		}
		else if (var->type == XP_AWK_NDE_LOCALIDX)
		{
			xp_awk_refdownval (run, (xp_awk_val_t*)map);
			STACK_LOCAL(run,var->id.idxa) = tmp;
		}
		else /* if (var->type == XP_AWK_NDE_ARGIDX) */
		{
			xp_awk_refdownval (run, (xp_awk_val_t*)map);
			STACK_ARG(run,var->id.idxa) = tmp;
		}

		xp_awk_refupval (tmp);
		map = (xp_awk_val_map_t*) tmp;
	}
	else if (map->type != XP_AWK_VAL_MAP)
	{
		PANIC (run, XP_AWK_ENOTINDEXABLE);
	}

	str = __idxnde_to_str (run, var->idx, &len);
	if (str == XP_NULL) return XP_NULL;

/*
xp_printf (XP_T("**** index str=>%s, map->ref=%d, map->type=%d\n"), str, map->ref, map->type);
*/
	n = xp_awk_map_putx (map->map, str, len, val, XP_NULL);
	if (n < 0)
	{
		xp_free (str);
		PANIC (run, XP_AWK_ENOMEM);
	}

	xp_free (str);
	xp_awk_refupval (val);
	return val;
}

static xp_awk_val_t* __do_assignment_pos (
	xp_awk_run_t* run, xp_awk_nde_pos_t* pos, xp_awk_val_t* val)
{
	xp_awk_val_t* v;
	xp_long_t lv;
	xp_real_t rv;
	xp_char_t* str;
	xp_size_t len;
	int n, errnum;

	/* get the position number */
	v = __eval_expression (run, pos->val);
	if (v == XP_NULL) return XP_NULL;

	xp_awk_refupval (v);
	n = xp_awk_valtonum (v, &lv, &rv);
	xp_awk_refdownval (run, v);

	if (n == -1) PANIC (run, XP_AWK_EPOSIDX); 
	if (n == 1) lv = (xp_long_t)rv;
	if (lv < 0) PANIC (run, XP_AWK_EPOSIDX);

	/* convert the value to the string */
	str = xp_awk_valtostr (val, &errnum, xp_true, XP_NULL, &len);
	if (str == XP_NULL) PANIC (run, errnum);

	if (lv == 0)
	{
		__clear_record (run, xp_false);

		if (xp_str_ncpy (&run->inrec.line, str, len) == (xp_size_t)-1)
		{
			xp_free (str);
			PANIC (run, errnum);
		}
		xp_free (str);

		if (val->type == XP_AWK_VAL_STR)
		{
			xp_awk_refdownval (run, run->inrec.d0);
			run->inrec.d0 = val;
			xp_awk_refupval (val);
		}
		else
		{
			v = xp_awk_makestrval (
				XP_STR_BUF(&run->inrec.line), 
				XP_STR_LEN(&run->inrec.line));
			if (v == XP_NULL)
			{
				__clear_record (run, xp_false);
				PANIC (run, XP_AWK_ENOMEM);
			}

			xp_awk_refdownval (run, run->inrec.d0);
			run->inrec.d0 = v;
			xp_awk_refupval (v);

			val = v;
		}

		if (__split_record (run) == -1) 
		{
			__clear_record (run, xp_false);
			return XP_NULL;
		}
	}
	else
	{
		if (__recomp_record_fields (
			run, (xp_size_t)lv, str, len, &errnum) == -1)
		{
			xp_free (str);
			__clear_record (run, xp_false);
			PANIC (run, errnum);
		}
		xp_free (str);

		/* recompose $0 */
		v = xp_awk_makestrval (
			XP_STR_BUF(&run->inrec.line), 
			XP_STR_LEN(&run->inrec.line));
		if (v == XP_NULL)
		{
			__clear_record (run, xp_false);
			PANIC (run, XP_AWK_ENOMEM);
		}

		xp_awk_refdownval (run, run->inrec.d0);
		run->inrec.d0 = v;
		xp_awk_refupval (v);

		val = v;
	}

	return val;
}

static xp_awk_val_t* __eval_binary (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	static binop_func_t __binop_func[] =
	{
		/* the order of the functions should be inline with
		 * the operator declaration in run.h */

		XP_NULL, /* __eval_binop_lor */
		XP_NULL, /* __eval_binop_land */
		XP_NULL, /* __eval_binop_in */

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
		XP_NULL, /* __eval_binop_ma */
		XP_NULL  /* __eval_binop_nm */
	};

	xp_awk_nde_exp_t* exp = (xp_awk_nde_exp_t*)nde;
	xp_awk_val_t* left, * right, * res;

	xp_assert (exp->type == XP_AWK_NDE_EXP_BIN);

	if (exp->opcode == XP_AWK_BINOP_LAND)
	{
		res = __eval_binop_land (run, exp->left, exp->right);
	}
	else if (exp->opcode == XP_AWK_BINOP_LOR)
	{
		res = __eval_binop_lor (run, exp->left, exp->right);
	}
	else if (exp->opcode == XP_AWK_BINOP_IN)
	{
		/* treat the in operator specially */
		res = __eval_binop_in (run, exp->left, exp->right);
	}
	else if (exp->opcode == XP_AWK_BINOP_NM)
	{
		res = __eval_binop_nm (run, exp->left, exp->right);
	}
	else if (exp->opcode == XP_AWK_BINOP_MA)
	{
		res = __eval_binop_ma (run, exp->left, exp->right);
	}
	else
	{
		xp_assert (exp->left->next == XP_NULL);
		left = __eval_expression (run, exp->left);
		if (left == XP_NULL) return XP_NULL;

		xp_awk_refupval (left);

		xp_assert (exp->right->next == XP_NULL);
		right = __eval_expression (run, exp->right);
		if (right == XP_NULL) 
		{
			xp_awk_refdownval (run, left);
			return XP_NULL;
		}

		xp_awk_refupval (right);

		xp_assert (exp->opcode >= 0 && 
			exp->opcode < xp_countof(__binop_func));
		xp_assert (__binop_func[exp->opcode] != XP_NULL);

		res = __binop_func[exp->opcode] (run, left, right);

		xp_awk_refdownval (run, left);
		xp_awk_refdownval (run, right);
	}

	return res;
}

static xp_awk_val_t* __eval_binop_lor (
	xp_awk_run_t* run, xp_awk_nde_t* left, xp_awk_nde_t* right)
{
	/*
	xp_awk_val_t* res = XP_NULL;

	res = xp_awk_makeintval (run, 
		xp_awk_valtobool(left) || xp_awk_valtobool(right));
	if (res == XP_NULL) PANIC (run, XP_AWK_ENOMEM);

	return res;
	*/

	/* short-circuit evaluation required special treatment */
	xp_awk_val_t* lv, * rv, * res;

	xp_assert (left->next == XP_NULL);
	lv = __eval_expression (run, left);
	if (lv == XP_NULL) return XP_NULL;

	xp_awk_refupval (lv);
	if (xp_awk_valtobool(lv)) 
	{
		res = xp_awk_makeintval (run, 1);
	}
	else
	{
		xp_assert (right->next == XP_NULL);
		rv = __eval_expression (run, right);
		if (rv == XP_NULL)
		{
			xp_awk_refdownval (run, lv);
			return XP_NULL;
		}
		xp_awk_refupval (rv);

		res = xp_awk_makeintval (run, (xp_awk_valtobool(rv)? 1: 0));
		xp_awk_refdownval (run, rv);
	}

	xp_awk_refdownval (run, lv);

	if (res == XP_NULL) PANIC (run, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_land (
	xp_awk_run_t* run, xp_awk_nde_t* left, xp_awk_nde_t* right)
{
	/*
	xp_awk_val_t* res = XP_NULL;

	res = xp_awk_makeintval (run, 
		xp_awk_valtobool(left) && xp_awk_valtobool(right));
	if (res == XP_NULL) PANIC (run, XP_AWK_ENOMEM);

	return res;
	*/

	/* short-circuit evaluation required special treatment */
	xp_awk_val_t* lv, * rv, * res;

	xp_assert (left->next == XP_NULL);
	lv = __eval_expression (run, left);
	if (lv == XP_NULL) return XP_NULL;

	xp_awk_refupval (lv);
	if (!xp_awk_valtobool(lv)) 
	{
		res = xp_awk_makeintval (run, 0);
	}
	else
	{
		xp_assert (right->next == XP_NULL);
		rv = __eval_expression (run, right);
		if (rv == XP_NULL)
		{
			xp_awk_refdownval (run, lv);
			return XP_NULL;
		}
		xp_awk_refupval (rv);

		res = xp_awk_makeintval (run, (xp_awk_valtobool(rv)? 1: 0));
		xp_awk_refdownval (run, rv);
	}

	xp_awk_refdownval (run, lv);

	if (res == XP_NULL) PANIC (run, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_in (
	xp_awk_run_t* run, xp_awk_nde_t* left, xp_awk_nde_t* right)
{
	xp_awk_val_t* rv, * res;
	xp_char_t* str;
	xp_size_t len;

	if (right->type != XP_AWK_NDE_GLOBAL &&
	    right->type != XP_AWK_NDE_LOCAL &&
	    right->type != XP_AWK_NDE_ARG &&
	    right->type != XP_AWK_NDE_NAMED)
	{
		/* the compiler should have handled this case */
		xp_assert (!"should never happen - in needs a plain variable");
		PANIC (run, XP_AWK_EINTERNAL);
		return XP_NULL;
	}

	/* evaluate the left-hand side of the operator */
	str = (left->type == XP_AWK_NDE_GRP)?
		__idxnde_to_str (run, ((xp_awk_nde_grp_t*)left)->body, &len):
		__idxnde_to_str (run, left, &len);
	if (str == XP_NULL) return XP_NULL;

	/* evaluate the right-hand side of the operator */
	xp_assert (right->next == XP_NULL);
	rv = __eval_expression (run, right);
	if (rv == XP_NULL) 
	{
		xp_free (str);
		return XP_NULL;
	}

	xp_awk_refupval (rv);

	if (rv->type == XP_AWK_VAL_NIL)
	{
		res = xp_awk_makeintval (run, 0);
		if (res == XP_NULL) 
		{
			xp_free (str);
			xp_awk_refdownval (run, rv);
			PANIC (run, XP_AWK_ENOMEM);
		}

		xp_free (str);
		xp_awk_refdownval (run, rv);
		return res;
	}
	else if (rv->type == XP_AWK_VAL_MAP)
	{
		xp_long_t r;

		r = (xp_long_t)(xp_awk_map_get(((xp_awk_val_map_t*)rv)->map,str,len) != XP_NULL);

		res = xp_awk_makeintval (run, r);
		if (res == XP_NULL) 
		{
			xp_free (str);
			xp_awk_refdownval (run, rv);
			PANIC (run, XP_AWK_ENOMEM);
		}

		xp_free (str);
		xp_awk_refdownval (run, rv);
		return res;
	}

	/* need an array */
	/* TODO: change the error code to make it clearer */
	PANIC (run, XP_AWK_EOPERAND); 
	return XP_NULL;
}

static xp_awk_val_t* __eval_binop_bor (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right)
{
	xp_awk_val_t* res = XP_NULL;

	if (left->type == XP_AWK_VAL_INT &&
	    right->type == XP_AWK_VAL_INT)
	{
		xp_long_t r = 
			((xp_awk_val_int_t*)left)->val | 
			((xp_awk_val_int_t*)right)->val;
		res = xp_awk_makeintval (run, r);
	}
	else
	{
		PANIC (run, XP_AWK_EOPERAND);
	}

	if (res == XP_NULL) PANIC (run, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_bxor (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right)
{
	xp_awk_val_t* res = XP_NULL;

	if (left->type == XP_AWK_VAL_INT &&
	    right->type == XP_AWK_VAL_INT)
	{
		xp_long_t r = 
			((xp_awk_val_int_t*)left)->val ^ 
			((xp_awk_val_int_t*)right)->val;
		res = xp_awk_makeintval (run, r);
	}
	else
	{
		PANIC (run, XP_AWK_EOPERAND);
	}

	if (res == XP_NULL) PANIC (run, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_band (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right)
{
	xp_awk_val_t* res = XP_NULL;

	if (left->type == XP_AWK_VAL_INT &&
	    right->type == XP_AWK_VAL_INT)
	{
		xp_long_t r = 
			((xp_awk_val_int_t*)left)->val &
			((xp_awk_val_int_t*)right)->val;
		res = xp_awk_makeintval (run, r);
	}
	else
	{
		PANIC (run, XP_AWK_EOPERAND);
	}

	if (res == XP_NULL) PANIC (run, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_eq (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right)
{
	xp_awk_val_t* res = XP_NULL;
	xp_long_t r = 0;

	if (left->type == XP_AWK_VAL_NIL || 
	    right->type == XP_AWK_VAL_NIL)
	{
		r = (left->type == right->type);
	}
	else if (left->type == XP_AWK_VAL_INT &&
	         right->type == XP_AWK_VAL_INT)
	{
		r = ((xp_awk_val_int_t*)left)->val ==
		    ((xp_awk_val_int_t*)right)->val;
	}
	else if (left->type == XP_AWK_VAL_REAL && 
	         right->type == XP_AWK_VAL_REAL)
	{
		r = ((xp_awk_val_real_t*)left)->val ==
		    ((xp_awk_val_real_t*)right)->val;
	}
	else if (left->type == XP_AWK_VAL_INT && 
	         right->type == XP_AWK_VAL_REAL)
	{
		r = ((xp_awk_val_int_t*)left)->val ==
		    ((xp_awk_val_real_t*)right)->val;
	}
	else if (left->type == XP_AWK_VAL_REAL &&
		 right->type == XP_AWK_VAL_INT)
	{
		r = ((xp_awk_val_real_t*)left)->val ==
		    ((xp_awk_val_int_t*)right)->val;
	}
	else if (left->type == XP_AWK_VAL_STR &&
	         right->type == XP_AWK_VAL_STR)
	{
		r = xp_strxncmp (
			((xp_awk_val_str_t*)left)->buf,
			((xp_awk_val_str_t*)left)->len,
			((xp_awk_val_str_t*)right)->buf,
			((xp_awk_val_str_t*)right)->len) == 0;
	}
	else
	{
		PANIC (run, XP_AWK_EOPERAND);
	}

	res = xp_awk_makeintval (run, r);
	if (res == XP_NULL) PANIC (run, XP_AWK_ENOMEM);

	return res;
}

static xp_awk_val_t* __eval_binop_ne (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right)
{
	xp_awk_val_t* res = XP_NULL;
	xp_long_t r = 0;

	if (left->type == XP_AWK_VAL_NIL || 
	    right->type == XP_AWK_VAL_NIL)
	{
		r = (left->type != right->type);
	}
	else if (left->type == XP_AWK_VAL_INT &&
	         right->type == XP_AWK_VAL_INT)
	{
		r = ((xp_awk_val_int_t*)left)->val !=
		    ((xp_awk_val_int_t*)right)->val;
	}
	else if (left->type == XP_AWK_VAL_REAL && 
	         right->type == XP_AWK_VAL_REAL)
	{
		r = ((xp_awk_val_real_t*)left)->val !=
		    ((xp_awk_val_real_t*)right)->val;
	}
	else if (left->type == XP_AWK_VAL_INT && 
	         right->type == XP_AWK_VAL_REAL)
	{
		r = ((xp_awk_val_int_t*)left)->val !=
		    ((xp_awk_val_real_t*)right)->val;
	}
	else if (left->type == XP_AWK_VAL_REAL &&
		 right->type == XP_AWK_VAL_INT)
	{
		r = ((xp_awk_val_real_t*)left)->val !=
		    ((xp_awk_val_int_t*)right)->val;
	}
	else if (left->type == XP_AWK_VAL_STR &&
	         right->type == XP_AWK_VAL_STR)
	{
		r = xp_strxncmp (
			((xp_awk_val_str_t*)left)->buf,
			((xp_awk_val_str_t*)left)->len,
			((xp_awk_val_str_t*)right)->buf,
			((xp_awk_val_str_t*)right)->len) != 0;
	}
	else
	{
		PANIC (run, XP_AWK_EOPERAND);
	}

	res = xp_awk_makeintval (run, r);
	if (res == XP_NULL) PANIC (run, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_gt (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right)
{
	xp_awk_val_t* res = XP_NULL;
	xp_long_t r = 0;

	if (left->type == XP_AWK_VAL_INT &&
	    right->type == XP_AWK_VAL_INT)
	{
		r = ((xp_awk_val_int_t*)left)->val >
		    ((xp_awk_val_int_t*)right)->val;
	}
	else if (left->type == XP_AWK_VAL_REAL && 
	         right->type == XP_AWK_VAL_REAL)
	{
		r = ((xp_awk_val_real_t*)left)->val >
		    ((xp_awk_val_real_t*)right)->val;
	}
	else if (left->type == XP_AWK_VAL_INT && 
	         right->type == XP_AWK_VAL_REAL)
	{
		r = ((xp_awk_val_int_t*)left)->val >
		    ((xp_awk_val_real_t*)right)->val;
	}
	else if (left->type == XP_AWK_VAL_REAL &&
		 right->type == XP_AWK_VAL_INT)
	{
		r = ((xp_awk_val_real_t*)left)->val >
		    ((xp_awk_val_int_t*)right)->val;
	}
	else if (left->type == XP_AWK_VAL_STR &&
	         right->type == XP_AWK_VAL_STR)
	{
		r = xp_strxncmp (
			((xp_awk_val_str_t*)left)->buf,
			((xp_awk_val_str_t*)left)->len,
			((xp_awk_val_str_t*)right)->buf,
			((xp_awk_val_str_t*)right)->len) > 0;
	}
	else
	{
		PANIC (run, XP_AWK_EOPERAND);
	}

	res = xp_awk_makeintval (run, r);
	if (res == XP_NULL) PANIC (run, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_ge (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right)
{
	xp_awk_val_t* res = XP_NULL;
	xp_long_t r = 0;

	if (left->type == XP_AWK_VAL_INT &&
	    right->type == XP_AWK_VAL_INT)
	{
		r = ((xp_awk_val_int_t*)left)->val >=
		    ((xp_awk_val_int_t*)right)->val;
	}
	else if (left->type == XP_AWK_VAL_REAL && 
	         right->type == XP_AWK_VAL_REAL)
	{
		r = ((xp_awk_val_real_t*)left)->val >=
		    ((xp_awk_val_real_t*)right)->val;
	}
	else if (left->type == XP_AWK_VAL_INT && 
	         right->type == XP_AWK_VAL_REAL)
	{
		r = ((xp_awk_val_int_t*)left)->val >=
		    ((xp_awk_val_real_t*)right)->val;
	}
	else if (left->type == XP_AWK_VAL_REAL &&
		 right->type == XP_AWK_VAL_INT)
	{
		r = ((xp_awk_val_real_t*)left)->val >=
		    ((xp_awk_val_int_t*)right)->val;
	}
	else if (left->type == XP_AWK_VAL_STR &&
	         right->type == XP_AWK_VAL_STR)
	{
		r = xp_strxncmp (
			((xp_awk_val_str_t*)left)->buf,
			((xp_awk_val_str_t*)left)->len,
			((xp_awk_val_str_t*)right)->buf,
			((xp_awk_val_str_t*)right)->len) >= 0;
	}
	else
	{
		PANIC (run, XP_AWK_EOPERAND);
	}

	res = xp_awk_makeintval (run, r);
	if (res == XP_NULL) PANIC (run, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_lt (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right)
{
	xp_awk_val_t* res = XP_NULL;
	xp_long_t r = 0;

	if (left->type == XP_AWK_VAL_INT &&
	    right->type == XP_AWK_VAL_INT)
	{
		r = ((xp_awk_val_int_t*)left)->val <
		    ((xp_awk_val_int_t*)right)->val;
	}
	else if (left->type == XP_AWK_VAL_REAL && 
	         right->type == XP_AWK_VAL_REAL)
	{
		r = ((xp_awk_val_real_t*)left)->val <
		    ((xp_awk_val_real_t*)right)->val;
	}
	else if (left->type == XP_AWK_VAL_INT && 
	         right->type == XP_AWK_VAL_REAL)
	{
		r = ((xp_awk_val_int_t*)left)->val <
		    ((xp_awk_val_real_t*)right)->val;
	}
	else if (left->type == XP_AWK_VAL_REAL &&
		 right->type == XP_AWK_VAL_INT)
	{
		r = ((xp_awk_val_real_t*)left)->val <
		    ((xp_awk_val_int_t*)right)->val;
	}
	else if (left->type == XP_AWK_VAL_STR &&
	         right->type == XP_AWK_VAL_STR)
	{
		r = xp_strxncmp (
			((xp_awk_val_str_t*)left)->buf,
			((xp_awk_val_str_t*)left)->len,
			((xp_awk_val_str_t*)right)->buf,
			((xp_awk_val_str_t*)right)->len) < 0;
	}
	else
	{
		PANIC (run, XP_AWK_EOPERAND);
	}

	res = xp_awk_makeintval (run, r);
	if (res == XP_NULL) PANIC (run, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_le (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right)
{
	xp_awk_val_t* res = XP_NULL;
	xp_long_t r = 0;

	if (left->type == XP_AWK_VAL_INT &&
	    right->type == XP_AWK_VAL_INT)
	{
		r = ((xp_awk_val_int_t*)left)->val <=
		    ((xp_awk_val_int_t*)right)->val;
	}
	else if (left->type == XP_AWK_VAL_REAL && 
	         right->type == XP_AWK_VAL_REAL)
	{
		r = ((xp_awk_val_real_t*)left)->val <=
		    ((xp_awk_val_real_t*)right)->val;
	}
	else if (left->type == XP_AWK_VAL_INT && 
	         right->type == XP_AWK_VAL_REAL)
	{
		r = ((xp_awk_val_int_t*)left)->val <=
		    ((xp_awk_val_real_t*)right)->val;
	}
	else if (left->type == XP_AWK_VAL_REAL &&
		 right->type == XP_AWK_VAL_INT)
	{
		r = ((xp_awk_val_real_t*)left)->val <=
		    ((xp_awk_val_int_t*)right)->val;
	}
	else if (left->type == XP_AWK_VAL_STR &&
	         right->type == XP_AWK_VAL_STR)
	{
		r = xp_strxncmp (
			((xp_awk_val_str_t*)left)->buf,
			((xp_awk_val_str_t*)left)->len,
			((xp_awk_val_str_t*)right)->buf,
			((xp_awk_val_str_t*)right)->len) <= 0;
	}
	else
	{
		PANIC (run, XP_AWK_EOPERAND);
	}

	res = xp_awk_makeintval (run, r);
	if (res == XP_NULL) PANIC (run, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_lshift (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right)
{
	int n1, n2, n3;
	xp_long_t l1, l2;
	xp_real_t r1, r2;
	xp_awk_val_t* res;

	n1 = xp_awk_valtonum (left, &l1, &r1);
	n2 = xp_awk_valtonum (right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (run, XP_AWK_EOPERAND); 

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		if  (l2 == 0) PANIC (run, XP_AWK_EDIVBYZERO);
		res = xp_awk_makeintval (run, (xp_long_t)l1 << (xp_long_t)l2);
	}
	else PANIC (run, XP_AWK_EOPERAND);

	if (res == XP_NULL) PANIC (run, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_rshift (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right)
{
	int n1, n2, n3;
	xp_long_t l1, l2;
	xp_real_t r1, r2;
	xp_awk_val_t* res;

	n1 = xp_awk_valtonum (left, &l1, &r1);
	n2 = xp_awk_valtonum (right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (run, XP_AWK_EOPERAND); 

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		if  (l2 == 0) PANIC (run, XP_AWK_EDIVBYZERO);
		res = xp_awk_makeintval (run, (xp_long_t)l1 >> (xp_long_t)l2);
	}
	else PANIC (run, XP_AWK_EOPERAND);

	if (res == XP_NULL) PANIC (run, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_plus (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right)
{
	int n1, n2, n3;
	xp_long_t l1, l2;
	xp_real_t r1, r2;
	xp_awk_val_t* res;

	n1 = xp_awk_valtonum (left, &l1, &r1);
	n2 = xp_awk_valtonum (right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (run, XP_AWK_EOPERAND); 
	/*
	n1  n2    n3
	0   0   = 0
	1   0   = 1
	0   1   = 2
	1   1   = 3
	*/
	n3 = n1 + (n2 << 1);
	xp_assert (n3 >= 0 && n3 <= 3);
	res = (n3 == 0)? xp_awk_makeintval(run,(xp_long_t)l1+(xp_long_t)l2):
	      (n3 == 1)? xp_awk_makerealval(run,(xp_real_t)r1+(xp_real_t)l2):
	      (n3 == 2)? xp_awk_makerealval(run,(xp_real_t)l1+(xp_real_t)r2):
	                 xp_awk_makerealval(run,(xp_real_t)r1+(xp_real_t)r2);

	if (res == XP_NULL) PANIC (run, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_minus (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right)
{
	int n1, n2, n3;
	xp_long_t l1, l2;
	xp_real_t r1, r2;
	xp_awk_val_t* res;

	n1 = xp_awk_valtonum (left, &l1, &r1);
	n2 = xp_awk_valtonum (right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (run, XP_AWK_EOPERAND);

	n3 = n1 + (n2 << 1);
	xp_assert (n3 >= 0 && n3 <= 3);
	res = (n3 == 0)? xp_awk_makeintval(run,(xp_long_t)l1-(xp_long_t)l2):
	      (n3 == 1)? xp_awk_makerealval(run,(xp_real_t)r1-(xp_real_t)l2):
	      (n3 == 2)? xp_awk_makerealval(run,(xp_real_t)l1-(xp_real_t)r2):
	                 xp_awk_makerealval(run,(xp_real_t)r1-(xp_real_t)r2);

	if (res == XP_NULL) PANIC (run, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_mul (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right)
{
	int n1, n2, n3;
	xp_long_t l1, l2;
	xp_real_t r1, r2;
	xp_awk_val_t* res;

	n1 = xp_awk_valtonum (left, &l1, &r1);
	n2 = xp_awk_valtonum (right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (run, XP_AWK_EOPERAND);

	n3 = n1 + (n2 << 1);
	xp_assert (n3 >= 0 && n3 <= 3);
	res = (n3 == 0)? xp_awk_makeintval(run,(xp_long_t)l1*(xp_long_t)l2):
	      (n3 == 1)? xp_awk_makerealval(run,(xp_real_t)r1*(xp_real_t)l2):
	      (n3 == 2)? xp_awk_makerealval(run,(xp_real_t)l1*(xp_real_t)r2):
	                 xp_awk_makerealval(run,(xp_real_t)r1*(xp_real_t)r2);

	if (res == XP_NULL) PANIC (run, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_div (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right)
{
	int n1, n2, n3;
	xp_long_t l1, l2;
	xp_real_t r1, r2;
	xp_awk_val_t* res;

	n1 = xp_awk_valtonum (left, &l1, &r1);
	n2 = xp_awk_valtonum (right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (run, XP_AWK_EOPERAND);

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		if  (l2 == 0) PANIC (run, XP_AWK_EDIVBYZERO);
		res = xp_awk_makeintval (run, (xp_long_t)l1 / (xp_long_t)l2);
	}
	else if (n3 == 1)
	{
		res = xp_awk_makerealval (run, (xp_real_t)r1 / (xp_real_t)l2);
	}
	else if (n3 == 2)
	{
		res = xp_awk_makerealval (run, (xp_real_t)l1 / (xp_real_t)r2);
	}
	else
	{
		xp_assert (n3 == 3);
		res = xp_awk_makerealval (run, (xp_real_t)r1 / (xp_real_t)r2);
	}

	if (res == XP_NULL) PANIC (run, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_mod (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right)
{
	int n1, n2, n3;
	xp_long_t l1, l2;
	xp_real_t r1, r2;
	xp_awk_val_t* res;

	n1 = xp_awk_valtonum (left, &l1, &r1);
	n2 = xp_awk_valtonum (right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (run, XP_AWK_EOPERAND);

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		if  (l2 == 0) PANIC (run, XP_AWK_EDIVBYZERO);
		res = xp_awk_makeintval (run, (xp_long_t)l1 % (xp_long_t)l2);
	}
	else PANIC (run, XP_AWK_EOPERAND);

	if (res == XP_NULL) PANIC (run, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_exp (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right)
{
	int n1, n2, n3;
	xp_long_t l1, l2;
	xp_real_t r1, r2;
	xp_awk_val_t* res;

	n1 = xp_awk_valtonum (left, &l1, &r1);
	n2 = xp_awk_valtonum (right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (run, XP_AWK_EOPERAND);

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		xp_long_t v = 1;
		while (l2-- > 0) v *= l1;
		res = xp_awk_makeintval (run, v);
	}
	else if (n3 == 1)
	{
		/*res = xp_awk_makerealval (
			run, pow((xp_real_t)r1,(xp_real_t)l2));*/
		xp_real_t v = 1.0;
		while (l2-- > 0) v *= r1;
		res = xp_awk_makerealval (run, v);
	}
	else if (n3 == 2)
	{
		res = xp_awk_makerealval (
			run, pow((xp_real_t)l1,(xp_real_t)r2));
	}
	else
	{
		xp_assert (n3 == 3);
		res = xp_awk_makerealval (
			run, pow((xp_real_t)r1,(xp_real_t)r2));
	}

	if (res == XP_NULL) PANIC (run, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_concat (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right)
{
	xp_char_t* strl, * strr;
	xp_size_t strl_len, strr_len;
	xp_awk_val_t* res;
	int errnum;

	strl = xp_awk_valtostr (left, &errnum, xp_true, XP_NULL, &strl_len);
	if (strl == XP_NULL) PANIC (run, errnum);

	strr = xp_awk_valtostr (right, &errnum, xp_true, XP_NULL, &strr_len);
	if (strr == XP_NULL) 
	{
		xp_free (strl);
		PANIC (run, errnum);
	}

	res = xp_awk_makestrval2 (strl, strl_len, strr, strr_len);
	if (res == XP_NULL)
	{
		xp_free (strl);
		xp_free (strr);
		PANIC (run, XP_AWK_ENOMEM);
	}

	xp_free (strl);
	xp_free (strr);

	return res;
}

static xp_awk_val_t* __eval_binop_ma (
	xp_awk_run_t* run, xp_awk_nde_t* left, xp_awk_nde_t* right)
{
	xp_awk_val_t* lv, * rv, * res;

	xp_assert (left->next == XP_NULL);
	xp_assert (right->next == XP_NULL);

	lv = __eval_expression (run, left);
	if (lv == XP_NULL) 
	{
		return XP_NULL;
	}

	xp_awk_refupval (lv);

	rv = __eval_expression0 (run, right);
	if (rv == XP_NULL)
	{
		xp_awk_refdownval (run, lv);
		return XP_NULL;
	}

	xp_awk_refupval (rv);

	res = __eval_binop_match0 (run, lv, rv, 1);

	xp_awk_refdownval (run, lv);
	xp_awk_refdownval (run, rv);

	return res;
}

static xp_awk_val_t* __eval_binop_nm (
	xp_awk_run_t* run, xp_awk_nde_t* left, xp_awk_nde_t* right)
{
	xp_awk_val_t* lv, * rv, * res;

	xp_assert (left->next == XP_NULL);
	xp_assert (right->next == XP_NULL);

	lv = __eval_expression (run, left);
	if (lv == XP_NULL) return XP_NULL;

	xp_awk_refupval (lv);

	rv = __eval_expression0 (run, right);
	if (rv == XP_NULL)
	{
		xp_awk_refdownval (run, lv);
		return XP_NULL;
	}

	xp_awk_refupval (rv);

	res = __eval_binop_match0 (run, lv, rv, 0);

	xp_awk_refdownval (run, lv);
	xp_awk_refdownval (run, rv);

	return res;
}

static xp_awk_val_t* __eval_binop_match0 (
	xp_awk_run_t* run, xp_awk_val_t* left, xp_awk_val_t* right, int ret)
{
	xp_awk_val_t* res;
	int n, errnum;
	xp_char_t* str;
	xp_size_t len;
	void* rex_code;

	if (right->type == XP_AWK_VAL_REX)
	{
		rex_code = ((xp_awk_val_rex_t*)right)->code;
	}
	else if (right->type == XP_AWK_VAL_STR)
	{
		rex_code = xp_awk_buildrex ( 
			((xp_awk_val_str_t*)right)->buf,
			((xp_awk_val_str_t*)right)->len, &errnum);
		if (rex_code == XP_NULL)
			PANIC (run, errnum);
	}
	else
	{
		str = xp_awk_valtostr (right, &errnum, xp_true, XP_NULL, &len);
		if (str == XP_NULL) PANIC (run, errnum);

		rex_code = xp_awk_buildrex (str, len, &errnum);
		if (rex_code == XP_NULL)
		{
			xp_free (str);
			PANIC (run, errnum);
		}

		xp_free (str);
	}

	if (left->type == XP_AWK_VAL_STR)
	{
		n = xp_awk_matchrex (
			rex_code,
			((xp_awk_val_str_t*)left)->buf,
			((xp_awk_val_str_t*)left)->len,
			XP_NULL, XP_NULL, &errnum);
		if (n == -1) 
		{
			if (right->type != XP_AWK_VAL_REX) xp_free (rex_code);
			PANIC (run, errnum);
		}

		res = xp_awk_makeintval (run, (n == ret));
		if (res == XP_NULL) 
		{
			if (right->type != XP_AWK_VAL_REX) xp_free (rex_code);
			PANIC (run, XP_AWK_ENOMEM);
		}
	}
	else
	{
		str = xp_awk_valtostr (left, &errnum, xp_true, XP_NULL, &len);
		if (str == XP_NULL) 
		{
			if (right->type != XP_AWK_VAL_REX) xp_free (rex_code);
			PANIC (run, errnum);
		}

		n = xp_awk_matchrex (
			rex_code, str, len, XP_NULL, XP_NULL, &errnum);
		if (n == -1) 
		{
			xp_free (str);
			if (right->type != XP_AWK_VAL_REX) xp_free (rex_code);
			PANIC (run, errnum);
		}

		res = xp_awk_makeintval (run, (n == ret));
		if (res == XP_NULL) 
		{
			xp_free (str);
			if (right->type != XP_AWK_VAL_REX) xp_free (rex_code);
			PANIC (run, XP_AWK_ENOMEM);
		}

		xp_free (str);
	}

	if (right->type != XP_AWK_VAL_REX) xp_free (rex_code);
	return res;
}

static xp_awk_val_t* __eval_unary (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	xp_awk_val_t* left, * res = XP_NULL;
	xp_awk_nde_exp_t* exp = (xp_awk_nde_exp_t*)nde;

	xp_assert (exp->type == XP_AWK_NDE_EXP_UNR);
	xp_assert (exp->left != XP_NULL && exp->right == XP_NULL);

	xp_assert (exp->left->next == XP_NULL);
	left = __eval_expression (run, exp->left);
	if (left == XP_NULL) return XP_NULL;

	xp_awk_refupval (left);

	if (exp->opcode == XP_AWK_UNROP_PLUS) 
	{
		if (left->type == XP_AWK_VAL_INT)
		{
			xp_long_t r = ((xp_awk_val_int_t*)left)->val;
			res = xp_awk_makeintval (run, r);
		}
		else if (left->type == XP_AWK_VAL_REAL)
		{
			xp_real_t r = ((xp_awk_val_real_t*)left)->val;
			res = xp_awk_makerealval (run, r);
		}
		else
		{
			xp_awk_refdownval (run, left);
			PANIC (run, XP_AWK_EOPERAND);
		}
	}
	else if (exp->opcode == XP_AWK_UNROP_MINUS)
	{
		if (left->type == XP_AWK_VAL_INT)
		{
			xp_long_t r = ((xp_awk_val_int_t*)left)->val;
			res = xp_awk_makeintval (run, -r);
		}
		else if (left->type == XP_AWK_VAL_REAL)
		{
			xp_real_t r = ((xp_awk_val_real_t*)left)->val;
			res = xp_awk_makerealval (run, -r);
		}
		else
		{
			xp_awk_refdownval (run, left);
			PANIC (run, XP_AWK_EOPERAND);
		}
	}
	else if (exp->opcode == XP_AWK_UNROP_NOT)
	{
		if (left->type == XP_AWK_VAL_INT)
		{
			xp_long_t r = ((xp_awk_val_int_t*)left)->val;
			res = xp_awk_makeintval (run, !r);
		}
		else if (left->type == XP_AWK_VAL_REAL)
		{
			xp_real_t r = ((xp_awk_val_real_t*)left)->val;
			res = xp_awk_makerealval (run, !r);
		}
		else
		{
			xp_awk_refdownval (run, left);
			PANIC (run, XP_AWK_EOPERAND);
		}
	}
	else if (exp->opcode == XP_AWK_UNROP_BNOT)
	{
		if (left->type == XP_AWK_VAL_INT)
		{
			xp_long_t r = ((xp_awk_val_int_t*)left)->val;
			res = xp_awk_makeintval (run, ~r);
		}
		else
		{
			xp_awk_refdownval (run, left);
			PANIC (run, XP_AWK_EOPERAND);
		}
	}

	if (res == XP_NULL)
	{
		xp_awk_refdownval (run, left);
		PANIC (run, XP_AWK_ENOMEM);
	}

	xp_awk_refdownval (run, left);
	return res;
}

static xp_awk_val_t* __eval_incpre (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	xp_awk_val_t* left, * res;
	xp_awk_nde_exp_t* exp = (xp_awk_nde_exp_t*)nde;

	xp_assert (exp->type == XP_AWK_NDE_EXP_INCPRE);
	xp_assert (exp->left != XP_NULL && exp->right == XP_NULL);

	/* this way of checking if the l-value is assignable is
	 * ugly as it is dependent of the values defined in tree.h.
	 * but let's keep going this way for the time being. */
	if (exp->left->type < XP_AWK_NDE_NAMED ||
	    exp->left->type > XP_AWK_NDE_ARGIDX)
	{
		PANIC (run, XP_AWK_EOPERAND);
	}

	xp_assert (exp->left->next == XP_NULL);
	left = __eval_expression (run, exp->left);
	if (left == XP_NULL) return XP_NULL;

	xp_awk_refupval (left);

	if (exp->opcode == XP_AWK_INCOP_PLUS) 
	{
		if (left->type == XP_AWK_VAL_INT)
		{
			xp_long_t r = ((xp_awk_val_int_t*)left)->val;
			res = xp_awk_makeintval (run, r + 1);
			if (res == XP_NULL) 
			{
				xp_awk_refdownval (run, left);
				PANIC (run, XP_AWK_ENOMEM);
			}
		}
		else if (left->type == XP_AWK_VAL_REAL)
		{
			xp_real_t r = ((xp_awk_val_real_t*)left)->val;
			res = xp_awk_makerealval (run, r + 1.0);
			if (res == XP_NULL) 
			{
				xp_awk_refdownval (run, left);
				PANIC (run, XP_AWK_ENOMEM);
			}
		}
		else
		{
			xp_long_t v1;
			xp_real_t v2;
			int n;

			n = xp_awk_valtonum (left, &v1, &v2);
			if (n == -1)
			{
				xp_awk_refdownval (run, left);
				PANIC (run, XP_AWK_EOPERAND);
			}

			if (n == 0) 
			{
				res = xp_awk_makeintval (run, v1 + 1);
			}
			else /* if (n == 1) */
			{
				xp_assert (n == 1);
				res = xp_awk_makerealval (run, v2 + 1.0);
			}

			if (res == XP_NULL) 
			{
				xp_awk_refdownval (run, left);
				PANIC (run, XP_AWK_ENOMEM);
			}
		}
	}
	else if (exp->opcode == XP_AWK_INCOP_MINUS)
	{
		if (left->type == XP_AWK_VAL_INT)
		{
			xp_long_t r = ((xp_awk_val_int_t*)left)->val;
			res = xp_awk_makeintval (run, r - 1);
			if (res == XP_NULL) 
			{
				xp_awk_refdownval (run, left);
				PANIC (run, XP_AWK_ENOMEM);
			}
		}
		else if (left->type == XP_AWK_VAL_REAL)
		{
			xp_real_t r = ((xp_awk_val_real_t*)left)->val;
			res = xp_awk_makerealval (run, r - 1.0);
			if (res == XP_NULL) 
			{
				xp_awk_refdownval (run, left);
				PANIC (run, XP_AWK_ENOMEM);
			}
		}
		else
		{
			xp_long_t v1;
			xp_real_t v2;
			int n;

			n = xp_awk_valtonum (left, &v1, &v2);
			if (n == -1)
			{
				xp_awk_refdownval (run, left);
				PANIC (run, XP_AWK_EOPERAND);
			}

			if (n == 0) 
			{
				res = xp_awk_makeintval (run, v1 - 1);
			}
			else /* if (n == 1) */
			{
				xp_assert (n == 1);
				res = xp_awk_makerealval (run, v2 - 1.0);
			}

			if (res == XP_NULL) 
			{
				xp_awk_refdownval (run, left);
				PANIC (run, XP_AWK_ENOMEM);
			}
		}
	}
	else
	{
		xp_assert (!"should never happen - invalid opcode");
		xp_awk_refdownval (run, left);
		PANIC (run, XP_AWK_EINTERNAL);
	}

	if (__do_assignment (run, exp->left, res) == XP_NULL)
	{
		xp_awk_refdownval (run, left);
		return XP_NULL;
	}

	xp_awk_refdownval (run, left);
	return res;
}

static xp_awk_val_t* __eval_incpst (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	xp_awk_val_t* left, * res, * res2;
	xp_awk_nde_exp_t* exp = (xp_awk_nde_exp_t*)nde;

	xp_assert (exp->type == XP_AWK_NDE_EXP_INCPST);
	xp_assert (exp->left != XP_NULL && exp->right == XP_NULL);

	/* this way of checking if the l-value is assignable is
	 * ugly as it is dependent of the values defined in tree.h.
	 * but let's keep going this way for the time being. */
	if (exp->left->type < XP_AWK_NDE_NAMED ||
	    exp->left->type > XP_AWK_NDE_ARGIDX)
	{
		PANIC (run, XP_AWK_EOPERAND);
	}

	xp_assert (exp->left->next == XP_NULL);
	left = __eval_expression (run, exp->left);
	if (left == XP_NULL) return XP_NULL;

	xp_awk_refupval (left);

	if (exp->opcode == XP_AWK_INCOP_PLUS) 
	{
		if (left->type == XP_AWK_VAL_INT)
		{
			xp_long_t r = ((xp_awk_val_int_t*)left)->val;
			res = xp_awk_makeintval (run, r);
			if (res == XP_NULL) 
			{
				xp_awk_refdownval (run, left);
				PANIC (run, XP_AWK_ENOMEM);
			}

			res2 = xp_awk_makeintval (run, r + 1);
			if (res2 == XP_NULL)
			{
				xp_awk_refdownval (run, left);
				xp_awk_freeval (run, res, xp_true);
				PANIC (run, XP_AWK_ENOMEM);
			}
		}
		else if (left->type == XP_AWK_VAL_REAL)
		{
			xp_real_t r = ((xp_awk_val_real_t*)left)->val;
			res = xp_awk_makerealval (run, r);
			if (res == XP_NULL) 
			{
				xp_awk_refdownval (run, left);
				PANIC (run, XP_AWK_ENOMEM);
			}

			res2 = xp_awk_makerealval (run, r + 1.0);
			if (res2 == XP_NULL)
			{
				xp_awk_refdownval (run, left);
				xp_awk_freeval (run, res, xp_true);
				PANIC (run, XP_AWK_ENOMEM);
			}
		}
		else
		{
			xp_long_t v1;
			xp_real_t v2;
			int n;

			n = xp_awk_valtonum (left, &v1, &v2);
			if (n == -1)
			{
				xp_awk_refdownval (run, left);
				PANIC (run, XP_AWK_EOPERAND);
			}

			if (n == 0) 
			{
				res = xp_awk_makeintval (run, v1);
				if (res == XP_NULL)
				{
					xp_awk_refdownval (run, left);
					PANIC (run, XP_AWK_ENOMEM);
				}

				res2 = xp_awk_makeintval (run, v1 + 1);
				if (res2 == XP_NULL)
				{
					xp_awk_refdownval (run, left);
					xp_awk_freeval (run, res, xp_true);
					PANIC (run, XP_AWK_ENOMEM);
				}
			}
			else /* if (n == 1) */
			{
				xp_assert (n == 1);
				res = xp_awk_makerealval (run, v2);
				if (res == XP_NULL)
				{
					xp_awk_refdownval (run, left);
					PANIC (run, XP_AWK_ENOMEM);
				}

				res2 = xp_awk_makerealval (run, v2 + 1.0);
				if (res2 == XP_NULL)
				{
					xp_awk_refdownval (run, left);
					xp_awk_freeval (run, res, xp_true);
					PANIC (run, XP_AWK_ENOMEM);
				}
			}
		}
	}
	else if (exp->opcode == XP_AWK_INCOP_MINUS)
	{
		if (left->type == XP_AWK_VAL_INT)
		{
			xp_long_t r = ((xp_awk_val_int_t*)left)->val;
			res = xp_awk_makeintval (run, r);
			if (res == XP_NULL) 
			{
				xp_awk_refdownval (run, left);
				PANIC (run, XP_AWK_ENOMEM);
			}

			res2 = xp_awk_makeintval (run, r - 1);
			if (res2 == XP_NULL)
			{
				xp_awk_refdownval (run, left);
				xp_awk_freeval (run, res, xp_true);
				PANIC (run, XP_AWK_ENOMEM);
			}
		}
		else if (left->type == XP_AWK_VAL_REAL)
		{
			xp_real_t r = ((xp_awk_val_real_t*)left)->val;
			res = xp_awk_makerealval (run, r);
			if (res == XP_NULL) 
			{
				xp_awk_refdownval (run, left);
				PANIC (run, XP_AWK_ENOMEM);
			}

			res2 = xp_awk_makerealval (run, r - 1.0);
			if (res2 == XP_NULL)
			{
				xp_awk_refdownval (run, left);
				xp_awk_freeval (run, res, xp_true);
				PANIC (run, XP_AWK_ENOMEM);
			}
		}
		else
		{
			xp_long_t v1;
			xp_real_t v2;
			int n;

			n = xp_awk_valtonum (left, &v1, &v2);
			if (n == -1)
			{
				xp_awk_refdownval (run, left);
				PANIC (run, XP_AWK_EOPERAND);
			}

			if (n == 0) 
			{
				res = xp_awk_makeintval (run, v1);
				if (res == XP_NULL)
				{
					xp_awk_refdownval (run, left);
					PANIC (run, XP_AWK_ENOMEM);
				}

				res2 = xp_awk_makeintval (run, v1 - 1);
				if (res2 == XP_NULL)
				{
					xp_awk_refdownval (run, left);
					xp_awk_freeval (run, res, xp_true);
					PANIC (run, XP_AWK_ENOMEM);
				}
			}
			else /* if (n == 1) */
			{
				xp_assert (n == 1);
				res = xp_awk_makerealval (run, v2);
				if (res == XP_NULL)
				{
					xp_awk_refdownval (run, left);
					PANIC (run, XP_AWK_ENOMEM);
				}

				res2 = xp_awk_makerealval (run, v2 - 1.0);
				if (res2 == XP_NULL)
				{
					xp_awk_refdownval (run, left);
					xp_awk_freeval (run, res, xp_true);
					PANIC (run, XP_AWK_ENOMEM);
				}
			}
		}
	}
	else
	{
		xp_assert (!"should never happen - invalid opcode");
		xp_awk_refdownval (run, left);
		PANIC (run, XP_AWK_EINTERNAL);
	}

	if (__do_assignment (run, exp->left, res2) == XP_NULL)
	{
		xp_awk_refdownval (run, left);
		return XP_NULL;
	}

	xp_awk_refdownval (run, left);
	return res;
}

static xp_awk_val_t* __eval_cnd (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	xp_awk_val_t* tv, * v;
	xp_awk_nde_cnd_t* cnd = (xp_awk_nde_cnd_t*)nde;

	xp_assert (cnd->test->next == XP_NULL);
	tv = __eval_expression (run, cnd->test);
	if (tv == XP_NULL) return XP_NULL;

	xp_awk_refupval (tv);

	xp_assert (cnd->left->next == XP_NULL &&
	           cnd->right->next == XP_NULL);
	v = (xp_awk_valtobool(tv))?
		__eval_expression (run, cnd->left):
		__eval_expression (run, cnd->right);

	xp_awk_refdownval (run, tv);
	return v;
}

static xp_awk_val_t* __eval_bfn (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	xp_awk_nde_call_t* call = (xp_awk_nde_call_t*)nde;

	/* built-in function */
	if (call->nargs < call->what.bfn.min_args)
	{
		PANIC (run, XP_AWK_ETOOFEWARGS);
	}

	if (call->nargs > call->what.bfn.max_args)
	{
		PANIC (run, XP_AWK_ETOOMANYARGS);
	}

	return __eval_call (run, nde, XP_NULL);
}

static xp_awk_val_t* __eval_afn (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	xp_awk_nde_call_t* call = (xp_awk_nde_call_t*)nde;
	xp_awk_afn_t* afn;
	xp_awk_pair_t* pair;

	pair = xp_awk_map_get (&run->awk->tree.afns, 
		call->what.afn.name, call->what.afn.name_len);
	if (pair == XP_NULL) PANIC (run, XP_AWK_ENOSUCHFUNC);

	afn = (xp_awk_afn_t*)pair->val;
	xp_assert (afn != XP_NULL);

	if (call->nargs > afn->nargs)
	{
		/* TODO: is this correct? what if i want to allow arbitarary numbers of arguments? */
		PANIC (run, XP_AWK_ETOOMANYARGS);
	}

	return __eval_call (run, nde, afn);
}

static xp_awk_val_t* __eval_call (
	xp_awk_run_t* run, xp_awk_nde_t* nde, xp_awk_afn_t* afn)
{
	xp_awk_nde_call_t* call = (xp_awk_nde_call_t*)nde;
	xp_size_t saved_stack_top;
	xp_size_t nargs, i;
	xp_awk_nde_t* p;
	xp_awk_val_t* v;
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

	xp_assert (xp_sizeof(void*) >= xp_sizeof(run->stack_top));
	xp_assert (xp_sizeof(void*) >= xp_sizeof(run->stack_base));

	saved_stack_top = run->stack_top;

/*xp_printf (XP_T("setting up function stack frame stack_top = %ld stack_base = %ld\n"), run->stack_top, run->stack_base); */
	if (__raw_push(run,(void*)run->stack_base) == -1) 
	{
		PANIC (run, XP_AWK_ENOMEM);
	}
	if (__raw_push(run,(void*)saved_stack_top) == -1) 
	{
		__raw_pop (run);
		PANIC (run, XP_AWK_ENOMEM);
	}

	/* secure space for a return value. */
	if (__raw_push(run,xp_awk_val_nil) == -1)
	{
		__raw_pop (run);
		__raw_pop (run);
		PANIC (run, XP_AWK_ENOMEM);
	}

	/* secure space for nargs */
	if (__raw_push(run,xp_awk_val_nil) == -1)
	{
		__raw_pop (run);
		__raw_pop (run);
		__raw_pop (run);
		PANIC (run, XP_AWK_ENOMEM);
	}

	nargs = 0;
	p = call->args;
	while (p != XP_NULL)
	{
		v = __eval_expression(run,p);
		if (v == XP_NULL)
		{
			while (nargs > 0)
			{
/* TODO: test this portion. */
				--nargs;
				xp_awk_refdownval (run, STACK_ARG(run,nargs));
				__raw_pop (run);
			}	

			__raw_pop (run);
			__raw_pop (run);
			__raw_pop (run);
			return XP_NULL;
		}

		if (__raw_push(run,v) == -1) 
		{
			/* ugly - v needs to be freed if it doesn't have
			 * any reference. but its reference has not been 
			 * updated for the successful stack push. so it adds
			 * up a reference and dereferences it*/
			xp_awk_refupval (v);
			xp_awk_refdownval (run, v);

			while (nargs > 0)
			{
/* TODO: test this portion. */
				--nargs;
				xp_awk_refdownval (run, STACK_ARG(run,nargs));
				__raw_pop (run);
			}	

			__raw_pop (run);
			__raw_pop (run);
			__raw_pop (run);
			PANIC (run, XP_AWK_ENOMEM);
		}

		xp_awk_refupval (v);
		nargs++;
		p = p->next;
	}

	xp_assert (nargs == call->nargs);

	if (afn != XP_NULL)
	{
		/* extra step for normal awk functions */

		while (nargs < afn->nargs)
		{
			/* push as many nils as the number of missing actual arguments */
			if (__raw_push(run,xp_awk_val_nil) == -1)
			{
				while (nargs > 0)
				{
					/* TODO: test this portion. */
					--nargs;
					xp_awk_refdownval (run, STACK_ARG(run,nargs));
					__raw_pop (run);
				}	

				__raw_pop (run);
				__raw_pop (run);
				__raw_pop (run);
				PANIC (run, XP_AWK_ENOMEM);
			}

			nargs++;
		}
	}

	run->stack_base = saved_stack_top;
	STACK_NARGS(run) = (void*)nargs;
	
/*xp_printf (XP_T("running function body\n")); */

	if (afn != XP_NULL)
	{
		/* normal awk function */
		xp_assert (afn->body->type == XP_AWK_NDE_BLK);
		n = __run_block(run,(xp_awk_nde_blk_t*)afn->body);
	}
	else
	{
		n = 0;

		/* built-in function */
		xp_assert (call->nargs >= call->what.bfn.min_args &&
		           call->nargs <= call->what.bfn.max_args);

		if (call->what.bfn.handler != XP_NULL)
		{
			n = call->what.bfn.handler (run->awk, run);
		}
	}

/*xp_printf (XP_T("block run complete\n")); */

	/* refdown args in the run.stack */
	nargs = (xp_size_t)STACK_NARGS(run);
/*xp_printf (XP_T("block run complete nargs = %d\n"), nargs); */
	for (i = 0; i < nargs; i++)
	{
		xp_awk_refdownval (run, STACK_ARG(run,i));
	}
/*xp_printf (XP_T("got return value\n")); */

	/* this is the trick mentioned in __run_return.
	 * adjust the reference count of the return value.
	 * the value must not be freed even if the reference count
	 * is decremented to zero because its reference has been incremented 
	 * in __run_return regardless of its reference count. */
	v = STACK_RETVAL(run);
	xp_awk_refdownval_nofree (run, v);

	run->stack_top =  (xp_size_t)run->stack[run->stack_base+1];
	run->stack_base = (xp_size_t)run->stack[run->stack_base+0];

	if (run->exit_level == EXIT_FUNCTION)
	{	
		run->exit_level = EXIT_NONE;
	}

/*xp_printf (XP_T("returning from function stack_top=%ld, stack_base=%ld\n"), run->stack_top, run->stack_base); */
	return (n == -1)? XP_NULL: v;
}

static xp_awk_val_t* __eval_int (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	xp_awk_val_t* val;
	val = xp_awk_makeintval (
		run, ((xp_awk_nde_int_t*)nde)->val);
	if (val == XP_NULL) PANIC (run, XP_AWK_ENOMEM);
	return val;
}

static xp_awk_val_t* __eval_real (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	xp_awk_val_t* val;
	val = xp_awk_makerealval (
		run, ((xp_awk_nde_real_t*)nde)->val);
	if (val == XP_NULL) PANIC (run, XP_AWK_ENOMEM);
	return val;
}

static xp_awk_val_t* __eval_str (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	xp_awk_val_t* val;

	val = xp_awk_makestrval (
		((xp_awk_nde_str_t*)nde)->buf,
		((xp_awk_nde_str_t*)nde)->len);
	if (val == XP_NULL) PANIC (run, XP_AWK_ENOMEM);

	return val;
}

static xp_awk_val_t* __eval_rex (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	xp_awk_val_t* val;

	val = xp_awk_makerexval (
		((xp_awk_nde_rex_t*)nde)->buf,
		((xp_awk_nde_rex_t*)nde)->len,
		((xp_awk_nde_rex_t*)nde)->code);
	if (val == XP_NULL) PANIC (run, XP_AWK_ENOMEM);

	return val;
}

static xp_awk_val_t* __eval_named (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	xp_awk_pair_t* pair;
	xp_awk_nde_var_t* tgt = (xp_awk_nde_var_t*)nde;
		       
	pair = xp_awk_map_get (&run->named, tgt->id.name, tgt->id.name_len);
	return (pair == XP_NULL)? xp_awk_val_nil: pair->val;
}

static xp_awk_val_t* __eval_global (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	xp_awk_nde_var_t* tgt = (xp_awk_nde_var_t*)nde;
	return STACK_GLOBAL(run,tgt->id.idxa);
}

static xp_awk_val_t* __eval_local (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	xp_awk_nde_var_t* tgt = (xp_awk_nde_var_t*)nde;
	return STACK_LOCAL(run,tgt->id.idxa);
}

static xp_awk_val_t* __eval_arg (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	xp_awk_nde_var_t* tgt = (xp_awk_nde_var_t*)nde;
	return STACK_ARG(run,tgt->id.idxa);
}

static xp_awk_val_t* __eval_indexed (
	xp_awk_run_t* run, xp_awk_nde_var_t* nde, xp_awk_val_map_t* map)
{
	xp_awk_val_t* res;
	xp_awk_pair_t* pair;
	xp_char_t* str;
	xp_size_t len;

	/* TODO: should it be an error? should it return nil? */
	if (map->type != XP_AWK_VAL_MAP) 
	{
	        PANIC (run, XP_AWK_ENOTINDEXABLE);
	}

	xp_assert (nde->idx != XP_NULL);

	str = __idxnde_to_str (run, nde->idx, &len);
	if (str == XP_NULL) return XP_NULL;

/* TODO: check this out........ */
	pair = xp_awk_map_get (((xp_awk_val_map_t*)map)->map, str, len);
	res = (pair == XP_NULL)? xp_awk_val_nil: (xp_awk_val_t*)pair->val;

	xp_free (str);
	return res;
}

static xp_awk_val_t* __eval_namedidx (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	xp_awk_nde_var_t* tgt = (xp_awk_nde_var_t*)nde;
	xp_awk_pair_t* pair;

	pair = xp_awk_map_get (&run->named, tgt->id.name, tgt->id.name_len);
	return __eval_indexed (run, tgt, 
		(pair == XP_NULL)? xp_awk_val_nil: pair->val);
}

static xp_awk_val_t* __eval_globalidx (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	xp_awk_nde_var_t* tgt = (xp_awk_nde_var_t*)nde;
	return __eval_indexed (run, tgt, STACK_GLOBAL(run,tgt->id.idxa));
}

static xp_awk_val_t* __eval_localidx (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	xp_awk_nde_var_t* tgt = (xp_awk_nde_var_t*)nde;
	return __eval_indexed (run, tgt, STACK_LOCAL(run,tgt->id.idxa));
}

static xp_awk_val_t* __eval_argidx (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	xp_awk_nde_var_t* tgt = (xp_awk_nde_var_t*)nde;
	return __eval_indexed (run, tgt, STACK_ARG(run,tgt->id.idxa));
}

static xp_awk_val_t* __eval_pos (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	xp_awk_nde_pos_t* pos = (xp_awk_nde_pos_t*)nde;
	xp_awk_val_t* v;
	xp_long_t lv;
	xp_real_t rv;
	int n;

	v = __eval_expression (run, pos->val);
	if (v == XP_NULL) return XP_NULL;

	xp_awk_refupval (v);
	n = xp_awk_valtonum (v, &lv, &rv);
	xp_awk_refdownval (run, v);

	if (n == -1) PANIC (run, XP_AWK_EPOSIDX);
	if (n == 1) lv = (xp_long_t)rv;

	if (lv < 0) PANIC (run, XP_AWK_EPOSIDX);
	if (lv == 0) v = run->inrec.d0;
	else if (lv > 0 && lv <= run->inrec.nflds) 
		v = run->inrec.flds[lv-1].val;
	else v = xp_awk_val_zls; /*xp_awk_val_nil;*/

	return v;
}

static xp_awk_val_t* __eval_getline (xp_awk_run_t* run, xp_awk_nde_t* nde)
{
	xp_awk_nde_getline_t* p;
	xp_awk_val_t* v, * res;
	xp_char_t* in = XP_NULL;
	const xp_char_t* dst;
	xp_str_t buf;
	int errnum, n;

	p = (xp_awk_nde_getline_t*)nde;

	xp_assert ((p->in_type == XP_AWK_IN_PIPE && p->in != XP_NULL) ||
	           (p->in_type == XP_AWK_IN_COPROC && p->in != XP_NULL) ||
		   (p->in_type == XP_AWK_IN_FILE && p->in != XP_NULL) ||
	           (p->in_type == XP_AWK_IN_CONSOLE && p->in == XP_NULL));

	if (p->in != XP_NULL)
	{
		xp_size_t len;

		v = __eval_expression (run, p->in);
		if (v == XP_NULL) return XP_NULL;

		/* TODO: distinction between v->type == XP_AWK_VAL_STR 
		 *       and v->type != XP_AWK_VAL_STR
		 *       if you use the buffer the v directly when
		 *       v->type == XP_AWK_VAL_STR, xp_awk_refdownval(v)
		 *       should not be called immediately below */
		xp_awk_refupval (v);
		in = xp_awk_valtostr (v, &errnum, xp_true, XP_NULL, &len);
		if (in == XP_NULL) 
		{
			xp_awk_refdownval (run, v);
			PANIC (run, errnum);
		}
		xp_awk_refdownval (run, v);

		if (len <= 0) 
		{
			/* the input source name is empty.
			 * make getline return -1 */
			xp_free (in);
			n = -1;
			goto skip_read;
		}

		while (len > 0)
		{
			if (in[--len] == XP_T('\0'))
			{
				/* the input source name contains a null 
				 * character. make getline return -1 */
				xp_free (in);
				n = -1;
				goto skip_read;
			}
		}
	}

	dst = (in == XP_NULL)? XP_T(""): in;

	/* TODO: optimize the line buffer management */
	if (xp_str_open (&buf, DEF_BUF_CAPA) == XP_NULL)
	{
		if (in != XP_NULL) xp_free (in);
		PANIC (run, XP_AWK_ENOMEM);
	}

	n = xp_awk_readextio (run, p->in_type, dst, &buf, &errnum);
	if (in != XP_NULL) xp_free (in);

	if (n < 0) 
	{
		if (errnum != XP_AWK_ENOERR)
		{
			xp_str_close (&buf);
			PANIC (run, errnum);
		}

		/* if errnum == XP_AWK_ENOERR, make getline return -1 */
		n = -1;
	}

	if (n > 0)
	{
		if (p->var == XP_NULL)
		{
			/* set $0 with the input value */
			__clear_record (run, xp_false);

			if (__set_record (run, 
				XP_STR_BUF(&buf), XP_STR_LEN(&buf)) == -1)
			{
				xp_str_close (&buf);
				return XP_NULL;
			}

			xp_str_close (&buf);
		}
		else
		{
			xp_awk_val_t* v;

			v = xp_awk_makestrval (
				XP_STR_BUF(&buf), XP_STR_LEN(&buf));

			xp_str_close (&buf);

			if (v == XP_NULL) PANIC (run, XP_AWK_ENOMEM);
			xp_awk_refupval (v);
	
			if (__do_assignment(run, p->var, v) == XP_NULL)
			{
				xp_awk_refdownval (run, v);
				return XP_NULL;
			}
			xp_awk_refdownval (run, v);
		}
	}
	else
	{
		xp_str_close (&buf);
	}
	
skip_read:
	res =  xp_awk_makeintval (run, n);
	if (res == XP_NULL) PANIC (run, XP_AWK_ENOMEM);

	return res;
}

static int __raw_push (xp_awk_run_t* run, void* val)
{
	if (run->stack_top >= run->stack_limit)
	{
		void** tmp;
		xp_size_t n;
	       
		n = run->stack_limit + STACK_INCREMENT;

#ifndef XP_AWK_NTDDK
		tmp = (void**) xp_realloc (
			run->stack, n * xp_sizeof(void*)); 
		if (tmp == XP_NULL) return -1;
#else
		tmp = (void**) xp_malloc (n * xp_sizeof(void*));
		if (tmp == XP_NULL) return -1;
		if (run->stack != XP_NULL)
		{
			xp_memcpy (tmp, run->stack, 
				run->stack_limit * xp_sizeof(void*)); 
			xp_free (run->stack);
		}
#endif
		run->stack = tmp;
		run->stack_limit = n;
	}

	run->stack[run->stack_top++] = val;
	return 0;
}

static void __raw_pop (xp_awk_run_t* run)
{
	xp_assert (run->stack_top > run->stack_base);
	run->stack_top--;
}

static void __raw_pop_times (xp_awk_run_t* run, xp_size_t times)
{
	while (times > 0)
	{
		--times;
		__raw_pop (run);
	}
}

static int __read_record (xp_awk_run_t* run)
{
	xp_ssize_t n;
	int errnum;

	__clear_record (run, xp_false);

	/*TODO: use RS */
	n = xp_awk_readextio (
		run, XP_AWK_IN_CONSOLE, 
		XP_T(""), &run->inrec.line, &errnum);
	if (n < 0) 
	{
		if (errnum == XP_AWK_ENOERR)
			PANIC_I (run, XP_AWK_ECONINDATA);
		else
			PANIC_I (run, errnum);
	}
	if (n == 0) 
	{
		xp_assert (XP_STR_LEN(&run->inrec.line) == 0);
		return 0;
	}

	if (__set_record (run, 
		XP_STR_BUF(&run->inrec.line), 
		XP_STR_LEN(&run->inrec.line)) == -1) return -1;

	return 1;
}

static int __set_record (xp_awk_run_t* run, const xp_char_t* str, xp_size_t len)
{
	xp_awk_val_t* v;

	v = xp_awk_makestrval (str, len);
	if (v == XP_NULL)
	{
		__clear_record (run, xp_false);
		PANIC_I (run, XP_AWK_ENOMEM);
	}

	xp_assert (run->inrec.d0 == xp_awk_val_nil);
	/* the record should be clear cleared before this function is called
	 * as it doesn't call xp_awk_refdownval on run->inrec.d0 */
	run->inrec.d0 = v;
	xp_awk_refupval (v);

	if (__split_record (run) == -1) 
	{
		__clear_record (run, xp_false);
		return -1;
	}

	return 0; /* success */
}

static int __split_record (xp_awk_run_t* run)
{
	/* TODO: support FS and regular expression */
	/* TODO: set NR after split */

	xp_char_t* p, * tok;
	xp_size_t len, tok_len, nflds;
	xp_awk_val_t* v;
       
	/* inrec should be cleared before __split_record is called */
	xp_assert (run->inrec.nflds == 0);

	/* scan the input record to count the fields */
	p = XP_STR_BUF(&run->inrec.line);
	len = XP_STR_LEN(&run->inrec.line);

	nflds = 0;
	while (p != XP_NULL)
	{
		p = xp_strxtok (p, len, XP_T(" \t"), &tok, &tok_len);

		if (nflds == 0 && p == XP_NULL && tok_len == 0)
		{
			/* no fields */
			return 0;
		}

		xp_assert ((tok != XP_NULL && tok_len > 0) || tok_len == 0);

		nflds++;
		len = XP_STR_LEN(&run->inrec.line) - 
			(p - XP_STR_BUF(&run->inrec.line));
	}

	/* allocate space */
	if (nflds > run->inrec.maxflds)
	{
		void* tmp = xp_malloc (
			xp_sizeof(*run->inrec.flds) * nflds);
		if (tmp == XP_NULL) PANIC_I (run, XP_AWK_ENOMEM);

		if (run->inrec.flds != NULL) 
			xp_free (run->inrec.flds);
		run->inrec.flds = tmp;
		run->inrec.maxflds = nflds;
	}

	/* scan again and split it */
	p = XP_STR_BUF(&run->inrec.line);
	len = XP_STR_LEN(&run->inrec.line);

	while (p != XP_NULL)
	{
		p = xp_strxtok (p, len, XP_T(" \t"), &tok, &tok_len);

		xp_assert ((tok != XP_NULL && tok_len > 0) || tok_len == 0);

		run->inrec.flds[run->inrec.nflds].ptr = tok;
		run->inrec.flds[run->inrec.nflds].len = tok_len;
		run->inrec.flds[run->inrec.nflds].val = 
			xp_awk_makestrval (tok, tok_len);

		if (run->inrec.flds[run->inrec.nflds].val == XP_NULL)
			PANIC_I (run, XP_AWK_ENOMEM);

		xp_awk_refupval (run->inrec.flds[run->inrec.nflds].val);
		run->inrec.nflds++;

		len = XP_STR_LEN(&run->inrec.line) - 
			(p - XP_STR_BUF(&run->inrec.line));
	}

	/* set the number of fields */
	v = xp_awk_makeintval (run, (xp_long_t)nflds);
	if (v == XP_NULL) PANIC_I (run, XP_AWK_ENOMEM);
	xp_awk_setglobal (run, XP_AWK_GLOBAL_NF, v);

	xp_assert (nflds == run->inrec.nflds);
	return 0;
}

static void __clear_record (xp_awk_run_t* run, xp_bool_t noline)
{
	xp_size_t i;

	xp_awk_refdownval (run, run->inrec.d0);
	run->inrec.d0 = xp_awk_val_nil;

	if (run->inrec.nflds > 0)
	{
		xp_assert (run->inrec.flds != XP_NULL);

		for (i = 0; i < run->inrec.nflds; i++) 
		{
			xp_assert (run->inrec.flds[i].val != XP_NULL);
			xp_awk_refdownval (run, run->inrec.flds[i].val);
		}
		run->inrec.nflds = 0;
		xp_awk_setglobal (run, XP_AWK_GLOBAL_NF, xp_awk_val_zero);
	}

	xp_assert (run->inrec.nflds == 0);
	if (!noline) xp_str_clear (&run->inrec.line);
}

static int __recomp_record_fields (xp_awk_run_t* run, 
	xp_size_t lv, xp_char_t* str, xp_size_t len, int* errnum)
{
	xp_awk_val_t* v;
	xp_char_t* ofsp = XP_NULL, * ofs;
	xp_size_t ofs_len;
	xp_size_t max, i, nflds;

	xp_assert (lv > 0);
	max = (lv > run->inrec.nflds)? lv: run->inrec.nflds;

	nflds = run->inrec.nflds;
	if (max > run->inrec.maxflds)
	{
		void* tmp;

#ifndef XP_AWK_NTDDK
		tmp = xp_realloc (
			run->inrec.flds, xp_sizeof(*run->inrec.flds) * max);
		if (tmp == XP_NULL) 
		{
			*errnum = XP_AWK_ENOMEM;
			return -1;
		}
#else
		tmp = xp_malloc (xp_sizeof(*run->inrec.flds) * max);
		if (tmp == XP_NULL)
		{
			*errnum = XP_AWK_ENOMEM;
			return -1;
		}
		if (run->inrec.flds != XP_NULL)
		{
			xp_memcpy (tmp, run->inrec.flds, 
				xp_sizeof(*run->inrec.flds) * run->inrec.maxflds);
			xp_free (run->inrec.flds);
		}
#endif

		run->inrec.flds = tmp;
		run->inrec.maxflds = max;
	}

	lv = lv - 1; /* adjust the value to 0-based index */

	xp_str_clear (&run->inrec.line);

	if (max > 1)
	{
		v = STACK_GLOBAL(run, XP_AWK_GLOBAL_OFS);
		if (v != xp_awk_val_nil)
		{
			ofsp = xp_awk_valtostr (
				v, errnum, xp_true, XP_NULL, &ofs_len);
			if (ofsp == XP_NULL) return -1;

			ofs = ofsp;
		}
		else
		{
			/* OFS not set */
			ofs = XP_T(" ");
			ofs_len = 1;
		}

	}

	for (i = 0; i < max; i++)
	{
		if (i > 0)
		{
			if (xp_str_ncat (
				&run->inrec.line, 
				ofs, ofs_len) == (xp_size_t)-1) 
			{
				if (ofsp != XP_NULL) xp_free (ofsp);
				*errnum = XP_AWK_ENOMEM;
				return -1;
			}
		}

		if (i == lv)
		{
			xp_awk_val_t* tmp;

			run->inrec.flds[i].ptr = 
				XP_STR_BUF(&run->inrec.line) +
				XP_STR_LEN(&run->inrec.line);
			run->inrec.flds[i].len = len;

			if (xp_str_ncat (
				&run->inrec.line, str, len) == (xp_size_t)-1)
			{
				if (ofsp != XP_NULL) xp_free (ofsp);
				*errnum = XP_AWK_ENOMEM;
				return -1;
			}

			tmp = xp_awk_makestrval (str,len);
			if (tmp == XP_NULL) 
			{
				if (ofsp != XP_NULL) xp_free (ofsp);
				*errnum = XP_AWK_ENOMEM;
				return -1;
			}

			if (i < nflds)
				xp_awk_refdownval (run, run->inrec.flds[i].val);
			else run->inrec.nflds++;
			run->inrec.flds[i].val = tmp;
			xp_awk_refupval (tmp);
		}
		else if (i >= nflds)
		{
			run->inrec.flds[i].ptr = 
				XP_STR_BUF(&run->inrec.line) +
				XP_STR_LEN(&run->inrec.line);
			run->inrec.flds[i].len = 0;

			if (xp_str_cat (
				&run->inrec.line, XP_T("")) == (xp_size_t)-1)
			{
				if (ofsp != XP_NULL) xp_free (ofsp);
				*errnum = XP_AWK_ENOMEM;
				return -1;
			}

			/* xp_awk_refdownval should not be called over 
			 * run->inrec.flds[i].val as it is not initialized
			 * to any valid values */
			/*xp_awk_refdownval (run, run->inrec.flds[i].val);*/
			run->inrec.flds[i].val = xp_awk_val_zls;
			xp_awk_refupval (xp_awk_val_zls);
			run->inrec.nflds++;
		}
		else
		{
			xp_awk_val_str_t* tmp;

			tmp = (xp_awk_val_str_t*)run->inrec.flds[i].val;

			run->inrec.flds[i].ptr = 
				XP_STR_BUF(&run->inrec.line) +
				XP_STR_LEN(&run->inrec.line);
			run->inrec.flds[i].len = tmp->len;

			if (xp_str_ncat (&run->inrec.line, 
				tmp->buf, tmp->len) == (xp_size_t)-1)
			{
				if (ofsp != XP_NULL) xp_free (ofsp);
				*errnum = XP_AWK_ENOMEM;
				return -1;
			}
		}
	}

	if (ofsp != XP_NULL) xp_free (ofsp);

	v = STACK_GLOBAL(run, XP_AWK_GLOBAL_NF);
	xp_assert (v->type == XP_AWK_VAL_INT);
	if (((xp_awk_val_int_t*)v)->val != max)
	{
		v = xp_awk_makeintval (run, (xp_long_t)max);
		if (v == XP_NULL) 
		{
			*errnum = XP_AWK_ENOMEM;
			return -1;
		}

		xp_awk_setglobal (run, XP_AWK_GLOBAL_NF, v);
	}

	return 0;
}

static xp_char_t* __idxnde_to_str (
	xp_awk_run_t* run, xp_awk_nde_t* nde, xp_size_t* len)
{
	xp_char_t* str;
	xp_awk_val_t* idx;
	int errnum;

	xp_assert (nde != XP_NULL);

	if (nde->next == XP_NULL)
	{
		/* single node index */
		idx = __eval_expression (run, nde);
		if (idx == XP_NULL) return XP_NULL;

		xp_awk_refupval (idx);

		str = xp_awk_valtostr (idx, &errnum, xp_true, XP_NULL, len);
		if (str == XP_NULL) 
		{
			xp_awk_refdownval (run, idx);
			PANIC (run, errnum);
		}

		xp_awk_refdownval (run, idx);
	}
	else
	{
		/* multidimensional index */
		xp_str_t idxstr;

		if (xp_str_open (&idxstr, DEF_BUF_CAPA) == XP_NULL) 
		{
			PANIC (run, XP_AWK_ENOMEM);
		}

		while (nde != XP_NULL)
		{
			idx = __eval_expression (run, nde);
			if (idx == XP_NULL) 
			{
				xp_str_close (&idxstr);
				return XP_NULL;
			}

			xp_awk_refupval (idx);

/* TODO: configurable index seperator, not just a comma */
			if (XP_STR_LEN(&idxstr) > 0 &&
			    xp_str_cat (&idxstr, XP_T(",")) == (xp_size_t)-1)
			{
				xp_awk_refdownval (run, idx);
				xp_str_close (&idxstr);
				PANIC (run, XP_AWK_ENOMEM);
			}

			if (xp_awk_valtostr (idx, &errnum, 
				xp_false, &idxstr, XP_NULL) == XP_NULL)
			{
				xp_awk_refdownval (run, idx);
				xp_str_close (&idxstr);
				PANIC (run, errnum);
			}

			xp_awk_refdownval (run, idx);
			nde = nde->next;
		}

		str = XP_STR_BUF(&idxstr);
		*len = XP_STR_LEN(&idxstr);
		xp_str_forfeit (&idxstr);
	}

	return str;
}

