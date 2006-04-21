/*
 * $Id: run.c,v 1.66 2006-04-21 16:21:27 bacon Exp $
 */

#include <xp/awk/awk_i.h>

/* TODO: remove this dependency...*/
#include <math.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/assert.h>
#include <xp/bas/string.h>
#include <xp/bas/memory.h>
#endif

#define STACK_INCREMENT 512

#define STACK_AT(awk,n) ((awk)->run.stack[(awk)->run.stack_base+(n)])
#define STACK_NARGS(awk) (STACK_AT(awk,3))
#define STACK_ARG(awk,n) STACK_AT(awk,3+1+(n))
#define STACK_LOCAL(awk,n) STACK_AT(awk,3+(xp_size_t)STACK_NARGS(awk)+1+(n))
#define STACK_RETVAL(awk) STACK_AT(awk,2)
#define STACK_GLOBAL(awk,n) ((awk)->run.stack[(n)])
#define STACK_RETVAL_GLOBAL(awk) ((awk)->run.stack[(awk)->tree.nglobals+2])

#define EXIT_NONE      0
#define EXIT_BREAK     1
#define EXIT_CONTINUE  2
#define EXIT_FUNCTION  3
#define EXIT_GLOBAL    4
#define EXIT_ABORT     5

#define PANIC(awk,code) \
	do { (awk)->errnum = (code); return XP_NULL; } while (0)
#define PANIC_I(awk,code) \
	do { (awk)->errnum = (code); return -1; } while (0)

static int __run_block (xp_awk_t* awk, xp_awk_nde_blk_t* nde);
static int __run_statement (xp_awk_t* awk, xp_awk_nde_t* nde);
static int __run_if_statement (xp_awk_t* awk, xp_awk_nde_if_t* nde);
static int __run_while_statement (xp_awk_t* awk, xp_awk_nde_while_t* nde);
static int __run_for_statement (xp_awk_t* awk, xp_awk_nde_for_t* nde);
static int __run_break_statement (xp_awk_t* awk, xp_awk_nde_break_t* nde);
static int __run_continue_statement (xp_awk_t* awk, xp_awk_nde_continue_t* nde);
static int __run_return_statement (xp_awk_t* awk, xp_awk_nde_return_t* nde);
static int __run_exit_statement (xp_awk_t* awk, xp_awk_nde_exit_t* nde);

static xp_awk_val_t* __eval_expression (xp_awk_t* awk, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_assignment (xp_awk_t* awk, xp_awk_nde_t* nde);
static xp_awk_val_t* __do_assignment (
	xp_awk_t* awk, xp_awk_nde_var_t* var, xp_awk_val_t* val);
static xp_awk_val_t* __do_assignment_map (
	xp_awk_t* awk, xp_awk_nde_var_t* var, xp_awk_val_t* val);

static xp_awk_val_t* __eval_binary (xp_awk_t* awk, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_binop_lor (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_land (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_bor (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_bxor (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_band (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_eq (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_ne (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_gt (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_ge (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_lt (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_le (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_lshift (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_rshift (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_plus (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_minus (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_mul (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_div (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_mod (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_exp (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_ma (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right);
static xp_awk_val_t* __eval_binop_nm (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right);

static xp_awk_val_t* __eval_unary (xp_awk_t* awk, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_incpre (xp_awk_t* awk, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_incpst (xp_awk_t* awk, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_cnd (xp_awk_t* awk, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_call (xp_awk_t* awk, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_int (xp_awk_t* awk, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_real (xp_awk_t* awk, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_str (xp_awk_t* awk, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_named (xp_awk_t* awk, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_global (xp_awk_t* awk, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_local (xp_awk_t* awk, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_arg (xp_awk_t* awk, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_namedidx (xp_awk_t* awk, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_globalidx (xp_awk_t* awk, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_localidx (xp_awk_t* awk, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_argidx (xp_awk_t* awk, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_pos (xp_awk_t* awk, xp_awk_nde_t* nde);

static int __raw_push (xp_awk_t* awk, void* val);
static void __raw_pop (xp_awk_t* awk);
static void __raw_pop_times (xp_awk_t* awk, xp_size_t times);

static int __val_to_num (xp_awk_val_t* v, xp_long_t* l, xp_real_t* r);
static xp_char_t* __val_to_str (xp_awk_val_t* v, int* errnum);

typedef xp_awk_val_t* (*binop_func_t) (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right);
typedef xp_awk_val_t* (*eval_expr_t) (xp_awk_t* awk, xp_awk_nde_t* nde);

int __printval (xp_awk_pair_t* pair)
{
	xp_printf (XP_TEXT("%s = "), (const xp_char_t*)pair->key);
	xp_awk_printval ((xp_awk_val_t*)pair->val);
	xp_printf (XP_TEXT("\n"));
	return 0;
}

int xp_awk_run (xp_awk_t* awk)
{
	xp_size_t nglobals, nargs, i;
	xp_size_t saved_stack_top;
	xp_awk_val_t* v;
	int n = 0;

	xp_assert (awk->run.stack_base == 0 && awk->run.stack_top == 0);

	/* secure space for global variables */
	saved_stack_top = awk->run.stack_top;

	nglobals = awk->tree.nglobals;
	while (nglobals > 0)
	{
		--nglobals;
		if (__raw_push(awk,xp_awk_val_nil) == -1)
		{
			/* restore the stack_top with the saved value
			 * instead of calling __raw_pop as many times as
			 * the successful __raw_push. it is ok because
			 * the values pushed so fare are all xp_awk_val_nil */
			awk->run.stack_top = saved_stack_top;
			PANIC_I (awk, XP_AWK_ENOMEM);
		}
	}	

	if (awk->opt.run & XP_AWK_RUNMAIN)
	{
		static xp_char_t m_a_i_n[] = 
		{ 
			XP_CHAR('m'), 
			XP_CHAR('a'), 
			XP_CHAR('i'), 
			XP_CHAR('n'), 
			XP_CHAR('\0')
		};
		static xp_awk_nde_call_t nde = 
		{ 
			XP_AWK_NDE_CALL, /* type */
			XP_NULL,         /* next */
			m_a_i_n,         /* name */
			XP_NULL          /* args */
		};

		awk->run.exit_level = EXIT_NONE;

		v = __eval_call(awk,(xp_awk_nde_t*)&nde);
		if (v == XP_NULL) n = -1;
		else
		{
			/* destroy the return value if necessary */
			xp_awk_refupval (v);
			xp_awk_refdownval (awk, v);
		}
	}
	else
	{
		saved_stack_top = awk->run.stack_top;
		if (__raw_push(awk,(void*)awk->run.stack_base) == -1) 
		{
			/* restore the stack top in a cheesy(?) way */
			awk->run.stack_top = saved_stack_top;
			/* pops off global variables in a decent way */	
			__raw_pop_times (awk, awk->tree.nglobals);
			PANIC_I (awk, XP_AWK_ENOMEM);
		}
		if (__raw_push(awk,(void*)saved_stack_top) == -1) 
		{
			awk->run.stack_top = saved_stack_top;
			__raw_pop_times (awk, awk->tree.nglobals);
			PANIC_I (awk, XP_AWK_ENOMEM);
		}
	
		/* secure space for a return value */
		if (__raw_push(awk,xp_awk_val_nil) == -1)
		{
			awk->run.stack_top = saved_stack_top;
			__raw_pop_times (awk, awk->tree.nglobals);
			PANIC_I (awk, XP_AWK_ENOMEM);
		}
	
		/* secure space for nargs */
		if (__raw_push(awk,xp_awk_val_nil) == -1)
		{
			awk->run.stack_top = saved_stack_top;
			__raw_pop_times (awk, awk->tree.nglobals);
			PANIC_I (awk, XP_AWK_ENOMEM);
		}
	
		awk->run.stack_base = saved_stack_top;
	
		/* set nargs to zero */
		nargs = 0;
		STACK_NARGS(awk) = (void*)nargs;
	
		/* stack set up properly. ready to exeucte statement blocks */
		if (n == 0 && awk->tree.begin != XP_NULL) 
		{
			xp_assert (awk->tree.begin->type == XP_AWK_NDE_BLK);

			awk->run.exit_level = EXIT_NONE;

			if (__run_block (awk, 
				(xp_awk_nde_blk_t*)awk->tree.begin) == -1) n = -1;
		}

		while (awk->run.exit_level != EXIT_GLOBAL &&
		       awk->run.exit_level != EXIT_ABORT)
		{
			awk->run.exit_level = EXIT_NONE;

			/*
			 * TODO: execute pattern blocks.
			 */

			break;
		}

		if (n == 0 && awk->tree.end != XP_NULL) 
		{
			xp_assert (awk->tree.end->type == XP_AWK_NDE_BLK);

			awk->run.exit_level = EXIT_NONE;

			if (__run_block (awk, 
				(xp_awk_nde_blk_t*)awk->tree.end) == -1) n = -1;
		}

		/* restore stack */
		nargs = (xp_size_t)STACK_NARGS(awk);
		xp_assert (nargs == 0);
		for (i = 0; i < nargs; i++)
		{
			xp_awk_refdownval (awk, STACK_ARG(awk,i));
		}

		v = STACK_RETVAL(awk);
xp_printf (XP_TEXT("Return Value - "));
xp_awk_printval (v);
xp_printf (XP_TEXT("\n"));
		/* the life of the global return value is over here
		 * unlike the return value of each function */
		/*xp_awk_refdownval_nofree (awk, v);*/
		xp_awk_refdownval (awk, v);

		awk->run.stack_top = 
			(xp_size_t)awk->run.stack[awk->run.stack_base+1];
		awk->run.stack_base = 
			(xp_size_t)awk->run.stack[awk->run.stack_base+0];
	}

	/* pops off the global variables */
	nglobals = awk->tree.nglobals;
	while (nglobals > 0)
	{
		--nglobals;
		xp_awk_refdownval (awk, STACK_GLOBAL(awk,nglobals));
		__raw_pop (awk);
	}

	/* just reset the exit level */
	awk->run.exit_level = EXIT_NONE;

xp_printf (XP_TEXT("-[VARIABLES]------------------------\n"));
xp_awk_map_walk (&awk->run.named, __printval);
xp_printf (XP_TEXT("-[END VARIABLES]--------------------------\n"));

	return n;
}

static void __free_namedval (xp_run_t* awk, void* val)
{
	xp_awk_refdownval (run, val);
}

static int __open_run (xp_awk_run_t* run, xp_awk_run_t* awk)
{
	run->awk = awk;

	run->stack = XP_NULL;
	run->stack_top = 0;
	run->stack_base = 0;
	run->stack_limit = 0;
	run->exit_level = 0;
	run->icache_count = 0;
	run->rcache_count = 0;

	if (xp_awk_map_open (&run->named, 
		awk, 256, __free_namedval) == XP_NULL) return -1;
}

static void __close_run (xp_awk_run_t* run)
{
	/* destroy run stack */
	if (run->stack != XP_NULL)
	{
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
		--run->icache_count;
		xp_awk_freeval (run, 
			run->icache[run->icache_count], xp_false);
	}
	while (run->rcache_count > 0)
	{
		--run->rcache_count;
		xp_awk_freeval (run, 
			run->rcache[run->rcache_count], xp_false);
	}
}

static int __run_block (xp_awk_t* awk, xp_awk_nde_blk_t* nde)
{
	xp_awk_nde_t* p;
	xp_size_t nlocals;
	xp_size_t saved_stack_top;
	int n = 0;

	xp_assert (nde->type == XP_AWK_NDE_BLK);

	p = nde->body;
	nlocals = nde->nlocals;

/*xp_printf (XP_TEXT("securing space for local variables nlocals = %d\n"), nlocals);*/
	saved_stack_top = awk->run.stack_top;

	/* secure space for local variables */
	while (nlocals > 0)
	{
		--nlocals;
		if (__raw_push(awk,xp_awk_val_nil) == -1)
		{
			/* restore stack top */
			awk->run.stack_top = saved_stack_top;
			return -1;
		}

		/* refupval is not required for xp_awk_val_nil */
	}

/*xp_printf (XP_TEXT("executing block statements\n"));*/
	while (p != XP_NULL && awk->run.exit_level == EXIT_NONE) 
	{
/*xp_printf (XP_TEXT("running a statement\n"));*/
		if (__run_statement(awk,p) == -1) 
		{
			n = -1;
			break;
		}
		p = p->next;
	}

/*xp_printf (XP_TEXT("popping off local variables\n"));*/
	/* pop off local variables */
	nlocals = nde->nlocals;
	while (nlocals > 0)
	{
		--nlocals;
		xp_awk_refdownval (awk, STACK_LOCAL(awk,nlocals));
		__raw_pop (awk);
	}

	return n;
}

static int __run_statement (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	switch (nde->type) 
	{
	case XP_AWK_NDE_NULL:
		/* do nothing */
		break;

	case XP_AWK_NDE_BLK:
		if (__run_block(awk,(xp_awk_nde_blk_t*)nde) == -1) return -1;
		break;

	case XP_AWK_NDE_IF:
		if (__run_if_statement (
			awk, (xp_awk_nde_if_t*)nde) == -1) return -1;	
		break;

	case XP_AWK_NDE_WHILE:
	case XP_AWK_NDE_DOWHILE:
		if (__run_while_statement (
			awk, (xp_awk_nde_while_t*)nde) == -1) return -1;
		break;

	case XP_AWK_NDE_FOR:
		if (__run_for_statement (
			awk, (xp_awk_nde_for_t*)nde) == -1) return -1;
		break;

	case XP_AWK_NDE_BREAK:
		if (__run_break_statement(
			awk, (xp_awk_nde_break_t*)nde) == -1) return -1;
		break;

	case XP_AWK_NDE_CONTINUE:
		if (__run_continue_statement (
			awk, (xp_awk_nde_continue_t*)nde) == -1) return -1;
		break;

	case XP_AWK_NDE_RETURN:
		if (__run_return_statement (
			awk, (xp_awk_nde_return_t*)nde) == -1) return -1;
		break;

	case XP_AWK_NDE_EXIT:
		if (__run_exit_statement (
			awk, (xp_awk_nde_exit_t*)nde) == -1) return -1;
		break;

	case XP_AWK_NDE_NEXT:
		/* TODO: */
		break;

	case XP_AWK_NDE_NEXTFILE:
		/* TODO: */
		break;

	default:
		if (__eval_expression(awk,nde) == XP_NULL) return -1;
		break;
	}

	return 0;
}

static int __run_if_statement (xp_awk_t* awk, xp_awk_nde_if_t* nde)
{
	xp_awk_val_t* test;
	int n = 0;

	test = __eval_expression (awk, nde->test);
	if (test == XP_NULL) return -1;

	xp_awk_refupval (test);
	if (xp_awk_boolval(test))
	{
		n = __run_statement (awk, nde->then_part);
	}
	else if (nde->else_part != XP_NULL)
	{
		n = __run_statement (awk, nde->else_part);
	}

	xp_awk_refdownval (awk, test); /* TODO: is this correct?*/
	return n;
}

static int __run_while_statement (xp_awk_t* awk, xp_awk_nde_while_t* nde)
{
	xp_awk_val_t* test;

	if (nde->type == XP_AWK_NDE_WHILE)
	{
		/* TODO: handle run-time abortion... */
		while (1)
		{
			test = __eval_expression (awk, nde->test);
			if (test == XP_NULL) return -1;

			xp_awk_refupval (test);

			if (xp_awk_boolval(test))
			{
				if (__run_statement(awk,nde->body) == -1)
				{
					xp_awk_refdownval (awk, test);
					return -1;
				}
			}
			else
			{
				xp_awk_refdownval (awk, test);
				break;
			}

			xp_awk_refdownval (awk, test);

			if (awk->run.exit_level == EXIT_BREAK)
			{	
				awk->run.exit_level = EXIT_NONE;
				break;
			}
			else if (awk->run.exit_level == EXIT_CONTINUE)
			{
				awk->run.exit_level = EXIT_NONE;
			}
			else if (awk->run.exit_level != EXIT_NONE) break;
		}
	}
	else if (nde->type == XP_AWK_NDE_DOWHILE)
	{
		/* TODO: handle run-time abortion... */
		do
		{
			if (__run_statement(awk,nde->body) == -1) return -1;

			if (awk->run.exit_level == EXIT_BREAK)
			{	
				awk->run.exit_level = EXIT_NONE;
				break;
			}
			else if (awk->run.exit_level == EXIT_CONTINUE)
			{
				awk->run.exit_level = EXIT_NONE;
			}
			else if (awk->run.exit_level != EXIT_NONE) break;

			test = __eval_expression (awk, nde->test);
			if (test == XP_NULL) return -1;

			xp_awk_refupval (test);

			if (!xp_awk_boolval(test))
			{
				xp_awk_refdownval (awk, test);
				break;
			}

			xp_awk_refdownval (awk, test);
		}
		while (1);
	}

	return 0;
}

static int __run_for_statement (xp_awk_t* awk, xp_awk_nde_for_t* nde)
{
	xp_awk_val_t* val;

	if (nde->init != XP_NULL)
	{
		val = __eval_expression(awk,nde->init);
		if (val == XP_NULL) return -1;

		xp_awk_refupval (val);
		xp_awk_refdownval (awk, val);
	}

	while (1)
	{
		if (nde->test != XP_NULL)
		{
			xp_awk_val_t* test;

			test = __eval_expression (awk, nde->test);
			if (test == XP_NULL) return -1;

			xp_awk_refupval (test);
			if (xp_awk_boolval(test))
			{
				if (__run_statement(awk,nde->body) == -1)
				{
					xp_awk_refdownval (awk, test);
					return -1;
				}
			}
			else
			{
				xp_awk_refdownval (awk, test);
				break;
			}

			xp_awk_refdownval (awk, test);
		}	
		else
		{
			if (__run_statement(awk,nde->body) == -1)
			{
				return -1;
			}
		}

		if (awk->run.exit_level == EXIT_BREAK)
		{	
			awk->run.exit_level = EXIT_NONE;
			break;
		}
		else if (awk->run.exit_level == EXIT_CONTINUE)
		{
			awk->run.exit_level = EXIT_NONE;
		}
		else if (awk->run.exit_level != EXIT_NONE) break;

		if (nde->incr != XP_NULL)
		{
			val = __eval_expression(awk,nde->incr);
			if (val == XP_NULL) return -1;

			xp_awk_refupval (val);
			xp_awk_refdownval (awk, val);
		}
	}

	return 0;
}

static int __run_break_statement (xp_awk_t* awk, xp_awk_nde_break_t* nde)
{
	awk->run.exit_level = EXIT_BREAK;
	return 0;
}

static int __run_continue_statement (xp_awk_t* awk, xp_awk_nde_continue_t* nde)
{
	awk->run.exit_level = EXIT_CONTINUE;
	return 0;
}

static int __run_return_statement (xp_awk_t* awk, xp_awk_nde_return_t* nde)
{

	if (nde->val != XP_NULL)
	{
		xp_awk_val_t* val;
/*xp_printf (XP_TEXT("returning....\n"));*/
		val = __eval_expression(awk, nde->val);
		if (val == XP_NULL) return -1;

		xp_awk_refdownval (awk, STACK_RETVAL(awk));
		STACK_RETVAL(awk) = val;

		xp_awk_refupval (val); /* see __eval_call for the trick */
/*xp_printf (XP_TEXT("set return value....\n"));*/
	}
	
	awk->run.exit_level = EXIT_FUNCTION;
	return 0;
}

static int __run_exit_statement (xp_awk_t* awk, xp_awk_nde_exit_t* nde)
{
	if (nde->val != XP_NULL)
	{
		xp_awk_val_t* val;

		val = __eval_expression(awk, nde->val);
		if (val == XP_NULL) return -1;

		xp_awk_refdownval (awk, STACK_RETVAL_GLOBAL(awk));
		STACK_RETVAL_GLOBAL(awk) = val; /* global return value */

		xp_awk_refupval (val);
	}

	awk->run.exit_level = EXIT_GLOBAL;
	return 0;
}

static xp_awk_val_t* __eval_expression (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	static eval_expr_t __eval_func[] =
	{
		/* the order of functions here should match the order
		 * of node types declared in tree.h */
		__eval_assignment,
		__eval_binary,
		__eval_unary,
		__eval_incpre,
		__eval_incpst,
		__eval_cnd,
		__eval_call,
		__eval_int,
		__eval_real,
		__eval_str,
		__eval_named,
		__eval_global,
		__eval_local,
		__eval_arg,
		__eval_namedidx,
		__eval_globalidx,
		__eval_localidx,
		__eval_argidx,
		__eval_pos
	};

	xp_assert (nde->type >= XP_AWK_NDE_ASS &&
		(nde->type - XP_AWK_NDE_ASS) < xp_countof(__eval_func));
	return __eval_func[nde->type-XP_AWK_NDE_ASS] (awk, nde);
}

static xp_awk_val_t* __eval_assignment (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	xp_awk_val_t* val, * res;
	xp_awk_nde_ass_t* ass = (xp_awk_nde_ass_t*)nde;

	xp_assert (ass->left != XP_NULL && ass->right != XP_NULL);

	val = __eval_expression(awk, ass->right);
	if (val == XP_NULL) return XP_NULL;

	xp_awk_refupval (val);

	if (ass->opcode != XP_AWK_ASSOP_NONE)
	{
		xp_awk_val_t* val2, * tmp;

		val2 = __eval_expression (awk, ass->left);
		if (val2 == XP_NULL)
		{
			xp_awk_refdownval (awk, val);
			return XP_NULL;
		}

		xp_awk_refupval (val2);

		if (ass->opcode == XP_AWK_ASSOP_PLUS)
		{
			tmp = __eval_binop_plus (awk, val2, val);
		}
		else if (ass->opcode == XP_AWK_ASSOP_MINUS)
		{
			tmp = __eval_binop_minus (awk, val2, val);
		}
		else if (ass->opcode == XP_AWK_ASSOP_MUL)
		{
			tmp = __eval_binop_mul (awk, val2, val);
		}
		else if (ass->opcode == XP_AWK_ASSOP_DIV)
		{
			tmp = __eval_binop_div (awk, val2, val);
		}
		else if (ass->opcode == XP_AWK_ASSOP_MOD)
		{
			tmp = __eval_binop_mod (awk, val2, val);
		}
		else if (ass->opcode == XP_AWK_ASSOP_EXP)
		{
			tmp = __eval_binop_exp (awk, val2, val);
		}
		else
		{
			xp_assert (!"should never happen - invalid assignment opcode");
			PANIC (awk, XP_AWK_EINTERNAL);
		}

		if (tmp == XP_NULL)
		{
			xp_awk_refdownval (awk, val);
			xp_awk_refdownval (awk, val2);
			return XP_NULL;
		}

		xp_awk_refdownval (awk, val);
		val = tmp;
		xp_awk_refupval (val);
	}

	res = __do_assignment (awk, (xp_awk_nde_var_t*)ass->left, val);
	xp_awk_refdownval (awk, val);

	return res;
}

static xp_awk_val_t* __do_assignment (
	xp_awk_t* awk, xp_awk_nde_var_t* var, xp_awk_val_t* val)
{

	if (val->type == XP_AWK_VAL_MAP)
	{
		/* a map cannot be assigned to a variable */
		PANIC (awk, XP_AWK_ENOTASSIGNABLE);
	}

	if (var->type == XP_AWK_NDE_NAMED) 
	{
		int n;

/* TODO: need to check if the old value is a map?? prevent the assignment? */
		n = xp_awk_map_putx (
			&awk->run.named, var->id.name, val, XP_NULL);
		if (n < 0) PANIC (awk, XP_AWK_ENOMEM);

		xp_awk_refupval (val);
	}
	else if (var->type == XP_AWK_NDE_GLOBAL) 
	{
/* TODO: need to check if the old value is a map?? prevent the assignment? */
		xp_awk_refdownval (awk, STACK_GLOBAL(awk,var->id.idxa));
		STACK_GLOBAL(awk,var->id.idxa) = val;
		xp_awk_refupval (val);
	}
	else if (var->type == XP_AWK_NDE_LOCAL) 
	{
/* TODO: need to check if the old value is a map?? prevent the assignment? */
		xp_awk_refdownval (awk, STACK_LOCAL(awk,var->id.idxa));
		STACK_LOCAL(awk,var->id.idxa) = val;
		xp_awk_refupval (val);
	}
	else if (var->type == XP_AWK_NDE_ARG) 
	{
/* TODO: need to check if the old value is a map?? prevent the assignment? */
		xp_awk_refdownval (awk, STACK_ARG(awk,var->id.idxa));
		STACK_ARG(awk,var->id.idxa) = val;
		xp_awk_refupval (val);
	}
	else if (var->type == XP_AWK_NDE_NAMEDIDX ||
	         var->type == XP_AWK_NDE_GLOBALIDX ||
	         var->type == XP_AWK_NDE_LOCALIDX ||
	         var->type == XP_AWK_NDE_ARGIDX) 
	{
		if (__do_assignment_map(awk,var,val) == XP_NULL)
			return XP_NULL;
	}
	else if (var->type == XP_AWK_NDE_POS)
	{
	        /* TODO: */
	        xp_printf (XP_TEXT("XP_AWK_NDE_POS not implemented\n"));
	        PANIC (awk, XP_AWK_EINTERNAL);
	}
	else
	{
		xp_assert (!"should never happen - invalid variable type");
		PANIC (awk, XP_AWK_EINTERNAL);
	}

	return val;
}

static xp_awk_val_t* __do_assignment_map (
	xp_awk_t* awk, xp_awk_nde_var_t* var, xp_awk_val_t* val)
{
	xp_awk_val_map_t* map;
	xp_awk_nde_t* nde;
	xp_char_t* str;
	int n;

	xp_assert (
		(var->type == XP_AWK_NDE_NAMEDIDX ||
		 var->type == XP_AWK_NDE_GLOBALIDX ||
		 var->type == XP_AWK_NDE_LOCALIDX ||
		 var->type == XP_AWK_NDE_ARGIDX) && var->idx != XP_NULL);

	if (var->type == XP_AWK_NDE_NAMEDIDX)
	{
		xp_awk_pair_t* pair;
		pair = xp_awk_map_get (&awk->run.named, var->id.name);
		map = (pair == XP_NULL)? xp_awk_val_nil: pair->val;
	}
	else
	{
		map = (var->type == XP_AWK_NDE_GLOBALIDX)? 
		      	(xp_awk_val_map_t*)STACK_GLOBAL(awk,var->id.idxa):
		      (var->type == XP_AWK_NDE_LOCALIDX)? 
		      	(xp_awk_val_map_t*)STACK_LOCAL(awk,var->id.idxa):
		      	(xp_awk_val_map_t*)STACK_ARG(awk,var->id.idxa);
	}

	if (map->type == XP_AWK_VAL_NIL)
	{
		/* the map is not initialized yet */
		xp_awk_val_t* tmp;

		tmp = xp_awk_makemapval (awk);
		if (tmp == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);

		if (var->type == XP_AWK_NDE_NAMEDIDX)
		{
			/* doesn't have to decrease the reference count 
			 * of the previous value here as it is done by 
			 * xp_awk_map_put */
			if (xp_awk_map_put (
				&awk->run.named, var->id.name, tmp) == XP_NULL)
			{
				xp_awk_refupval (tmp);
				xp_awk_refdownval (awk, tmp);
				PANIC (awk, XP_AWK_ENOMEM);		
			}
		}
		else if (var->type == XP_AWK_NDE_GLOBALIDX)
		{
			/* decrease the reference count of the previous value.
			 * in fact, this is not necessary as map is always 
			 * xp_awk_val_nil here. */
			xp_awk_refdownval (awk, (xp_awk_val_t*)map);
			STACK_GLOBAL(awk,var->id.idxa) = tmp;
		}
		else if (var->type == XP_AWK_NDE_LOCALIDX)
		{
			xp_awk_refdownval (awk, (xp_awk_val_t*)map);
			STACK_LOCAL(awk,var->id.idxa) = tmp;
		}
		else 
		{
			xp_awk_refdownval (awk, (xp_awk_val_t*)map);
			STACK_ARG(awk,var->id.idxa) = tmp;
		}

		xp_awk_refupval (tmp);
		map = (xp_awk_val_map_t*) tmp;
	}
	else if (map->type != XP_AWK_VAL_MAP)
	{
		PANIC (awk, XP_AWK_ENOTINDEXABLE);
	}

	/* compose a map index */
	nde = var->idx;
/* TODO: while (nde != XP_NULL)   */
	{ 
		xp_awk_val_t* tmp;
		int errnum;

		tmp = __eval_expression (awk, nde);
		if (tmp == XP_NULL) 
		{
			/* TODO: clearing previous idx values... */
			return XP_NULL;
		}

		xp_awk_refupval (tmp);

		str = __val_to_str (tmp, &errnum);
		if (str == XP_NULL) 
		{
			xp_awk_refdownval (awk, tmp);
			PANIC (awk, errnum);
		}

		xp_awk_refdownval (awk, tmp);
		/* TODO: nde = nde->next; */
	}

/*
xp_printf (XP_TEXT("**** index str=>%s, map->ref=%d, map->type=%d\n"), str, map->ref, map->type);
*/
	n = xp_awk_map_putx (map->map, str, val, XP_NULL);
	if (n < 0)
	{
		xp_free (str);
		PANIC (awk, XP_AWK_ENOMEM);
	}

	xp_free (str);
	xp_awk_refupval (val);
	return val;
}

static xp_awk_val_t* __eval_binary (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	static binop_func_t __binop_func[] =
	{
		__eval_binop_lor,
		__eval_binop_land,
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

		__eval_binop_ma,
		__eval_binop_nm
	};
	xp_awk_nde_exp_t* exp = (xp_awk_nde_exp_t*)nde;
	xp_awk_val_t* left, * right, * res;

	xp_assert (exp->type == XP_AWK_NDE_EXP_BIN);

	left = __eval_expression (awk, exp->left);
	if (left == XP_NULL) return XP_NULL;

	xp_awk_refupval (left);

	right = __eval_expression (awk, exp->right);
	if (right == XP_NULL) 
	{
		xp_awk_refdownval (awk, left);
		return XP_NULL;
	}

	xp_awk_refupval (right);

	xp_assert (exp->opcode >= 0 && 
		exp->opcode < xp_countof(__binop_func));

	res = __binop_func[exp->opcode] (awk, left, right);

	xp_awk_refdownval (awk, left);
	xp_awk_refdownval (awk, right);

	return res;
}

static xp_awk_val_t* __eval_binop_lor (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right)
{
	xp_awk_val_t* res = XP_NULL;

	res = xp_awk_makeintval (awk, 
		xp_awk_boolval(left) || xp_awk_boolval(right));
	if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);

	return res;
}

static xp_awk_val_t* __eval_binop_land (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right)
{
	xp_awk_val_t* res = XP_NULL;

	res = xp_awk_makeintval (awk, 
		xp_awk_boolval(left) && xp_awk_boolval(right));
	if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);

	return res;
}

static xp_awk_val_t* __eval_binop_bor (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right)
{
	xp_awk_val_t* res = XP_NULL;

	if (left->type == XP_AWK_VAL_INT &&
	    right->type == XP_AWK_VAL_INT)
	{
		xp_long_t r = 
			((xp_awk_val_int_t*)left)->val | 
			((xp_awk_val_int_t*)right)->val;
		res = xp_awk_makeintval (awk, r);
	}
	else
	{
		PANIC (awk, XP_AWK_EOPERAND);
	}

	if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_bxor (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right)
{
	xp_awk_val_t* res = XP_NULL;

	if (left->type == XP_AWK_VAL_INT &&
	    right->type == XP_AWK_VAL_INT)
	{
		xp_long_t r = 
			((xp_awk_val_int_t*)left)->val ^ 
			((xp_awk_val_int_t*)right)->val;
		res = xp_awk_makeintval (awk, r);
	}
	else
	{
		PANIC (awk, XP_AWK_EOPERAND);
	}

	if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_band (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right)
{
	xp_awk_val_t* res = XP_NULL;

	if (left->type == XP_AWK_VAL_INT &&
	    right->type == XP_AWK_VAL_INT)
	{
		xp_long_t r = 
			((xp_awk_val_int_t*)left)->val &
			((xp_awk_val_int_t*)right)->val;
		res = xp_awk_makeintval (awk, r);
	}
	else
	{
		PANIC (awk, XP_AWK_EOPERAND);
	}

	if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_eq (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right)
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
		PANIC (awk, XP_AWK_EOPERAND);
	}

	res = xp_awk_makeintval (awk, r);
	if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_ne (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right)
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
		PANIC (awk, XP_AWK_EOPERAND);
	}

	res = xp_awk_makeintval (awk, r);
	if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_gt (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right)
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
		PANIC (awk, XP_AWK_EOPERAND);
	}

	res = xp_awk_makeintval (awk, r);
	if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_ge (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right)
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
		PANIC (awk, XP_AWK_EOPERAND);
	}

	res = xp_awk_makeintval (awk, r);
	if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_lt (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right)
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
		PANIC (awk, XP_AWK_EOPERAND);
	}

	res = xp_awk_makeintval (awk, r);
	if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_le (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right)
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
		PANIC (awk, XP_AWK_EOPERAND);
	}

	res = xp_awk_makeintval (awk, r);
	if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_lshift (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right)
{
	int n1, n2, n3;
	xp_long_t l1, l2;
	xp_real_t r1, r2;
	xp_awk_val_t* res;

	n1 = __val_to_num (left, &l1, &r1);
	n2 = __val_to_num (right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (awk, XP_AWK_EOPERAND);

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		if  (l2 == 0) PANIC (awk, XP_AWK_EDIVBYZERO);
		res = xp_awk_makeintval (awk, (xp_long_t)l1 << (xp_long_t)l2);
	}
	else PANIC (awk, XP_AWK_EOPERAND);

	if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_rshift (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right)
{
	int n1, n2, n3;
	xp_long_t l1, l2;
	xp_real_t r1, r2;
	xp_awk_val_t* res;

	n1 = __val_to_num (left, &l1, &r1);
	n2 = __val_to_num (right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (awk, XP_AWK_EOPERAND);

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		if  (l2 == 0) PANIC (awk, XP_AWK_EDIVBYZERO);
		res = xp_awk_makeintval (awk, (xp_long_t)l1 >> (xp_long_t)l2);
	}
	else PANIC (awk, XP_AWK_EOPERAND);

	if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_plus (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right)
{
	int n1, n2, n3;
	xp_long_t l1, l2;
	xp_real_t r1, r2;
	xp_awk_val_t* res;

	n1 = __val_to_num (left, &l1, &r1);
	n2 = __val_to_num (right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (awk, XP_AWK_EOPERAND);
	/*
	n1  n2    n3
	0   0   = 0
	1   0   = 1
	0   1   = 2
	1   1   = 3
	*/
	n3 = n1 + (n2 << 1);
	xp_assert (n3 >= 0 && n3 <= 3);
	res = (n3 == 0)? xp_awk_makeintval(awk,(xp_long_t)l1+(xp_long_t)l2):
	      (n3 == 1)? xp_awk_makerealval(awk,(xp_real_t)r1+(xp_real_t)l2):
	      (n3 == 2)? xp_awk_makerealval(awk,(xp_real_t)l1+(xp_real_t)r2):
	                 xp_awk_makerealval(awk,(xp_real_t)r1+(xp_real_t)r2);

	if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_minus (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right)
{
	int n1, n2, n3;
	xp_long_t l1, l2;
	xp_real_t r1, r2;
	xp_awk_val_t* res;

	n1 = __val_to_num (left, &l1, &r1);
	n2 = __val_to_num (right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (awk, XP_AWK_EOPERAND);

	n3 = n1 + (n2 << 1);
	xp_assert (n3 >= 0 && n3 <= 3);
	res = (n3 == 0)? xp_awk_makeintval(awk,(xp_long_t)l1-(xp_long_t)l2):
	      (n3 == 1)? xp_awk_makerealval(awk,(xp_real_t)r1-(xp_real_t)l2):
	      (n3 == 2)? xp_awk_makerealval(awk,(xp_real_t)l1-(xp_real_t)r2):
	                 xp_awk_makerealval(awk,(xp_real_t)r1-(xp_real_t)r2);

	if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_mul (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right)
{
	int n1, n2, n3;
	xp_long_t l1, l2;
	xp_real_t r1, r2;
	xp_awk_val_t* res;

	n1 = __val_to_num (left, &l1, &r1);
	n2 = __val_to_num (right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (awk, XP_AWK_EOPERAND);

	n3 = n1 + (n2 << 1);
	xp_assert (n3 >= 0 && n3 <= 3);
	res = (n3 == 0)? xp_awk_makeintval(awk,(xp_long_t)l1*(xp_long_t)l2):
	      (n3 == 1)? xp_awk_makerealval(awk,(xp_real_t)r1*(xp_real_t)l2):
	      (n3 == 2)? xp_awk_makerealval(awk,(xp_real_t)l1*(xp_real_t)r2):
	                 xp_awk_makerealval(awk,(xp_real_t)r1*(xp_real_t)r2);

	if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_div (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right)
{
	int n1, n2, n3;
	xp_long_t l1, l2;
	xp_real_t r1, r2;
	xp_awk_val_t* res;

	n1 = __val_to_num (left, &l1, &r1);
	n2 = __val_to_num (right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (awk, XP_AWK_EOPERAND);

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		if  (l2 == 0) PANIC (awk, XP_AWK_EDIVBYZERO);
		res = xp_awk_makeintval (awk, (xp_long_t)l1 / (xp_long_t)l2);
	}
	else if (n3 == 1)
	{
		res = xp_awk_makerealval (awk, (xp_real_t)r1 / (xp_real_t)l2);
	}
	else if (n3 == 2)
	{
		res = xp_awk_makerealval (awk, (xp_real_t)l1 / (xp_real_t)r2);
	}
	else
	{
		xp_assert (n3 == 3);
		res = xp_awk_makerealval (awk, (xp_real_t)r1 / (xp_real_t)r2);
	}

	if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_mod (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right)
{
	int n1, n2, n3;
	xp_long_t l1, l2;
	xp_real_t r1, r2;
	xp_awk_val_t* res;

	n1 = __val_to_num (left, &l1, &r1);
	n2 = __val_to_num (right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (awk, XP_AWK_EOPERAND);

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		if  (l2 == 0) PANIC (awk, XP_AWK_EDIVBYZERO);
		res = xp_awk_makeintval (awk, (xp_long_t)l1 % (xp_long_t)l2);
	}
	else PANIC (awk, XP_AWK_EOPERAND);

	if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_exp (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right)
{
	int n1, n2, n3;
	xp_long_t l1, l2;
	xp_real_t r1, r2;
	xp_awk_val_t* res;

	n1 = __val_to_num (left, &l1, &r1);
	n2 = __val_to_num (right, &l2, &r2);

	if (n1 == -1 || n2 == -1) PANIC (awk, XP_AWK_EOPERAND);

	n3 = n1 + (n2 << 1);
	if (n3 == 0)
	{
		xp_long_t v = 1;
		while (l2-- > 0) v *= l1;
		res = xp_awk_makeintval (awk, v);
	}
	else if (n3 == 1)
	{
		/*res = xp_awk_makerealval (
			awk, pow((xp_real_t)r1,(xp_real_t)l2));*/
		xp_real_t v = 1.0;
		while (l2-- > 0) v *= r1;
		res = xp_awk_makerealval (awk, v);
	}
	else if (n3 == 2)
	{
		res = xp_awk_makerealval (
			awk, pow((xp_real_t)l1,(xp_real_t)r2));
	}
	else
	{
		xp_assert (n3 == 3);
		res = xp_awk_makerealval (
			awk, pow((xp_real_t)r1,(xp_real_t)r2));
	}

	if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	return res;
}

static xp_awk_val_t* __eval_binop_ma (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right)
{
	/* TODO: ... */
	PANIC (awk, XP_AWK_EINTERNAL);
	return XP_NULL;
}

static xp_awk_val_t* __eval_binop_nm (
	xp_awk_t* awk, xp_awk_val_t* left, xp_awk_val_t* right)
{
	/* TODO: ... */
	PANIC (awk, XP_AWK_EINTERNAL);
	return XP_NULL;
}

static xp_awk_val_t* __eval_unary (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	xp_awk_val_t* left, * res = XP_NULL;
	xp_awk_nde_exp_t* exp = (xp_awk_nde_exp_t*)nde;

	xp_assert (exp->type == XP_AWK_NDE_EXP_UNR);
	xp_assert (exp->left != XP_NULL && exp->right == XP_NULL);

	left = __eval_expression (awk, exp->left);
	if (left == XP_NULL) return XP_NULL;

	xp_awk_refupval (left);

	if (exp->opcode == XP_AWK_UNROP_PLUS) 
	{
		if (left->type == XP_AWK_VAL_INT)
		{
			xp_long_t r = ((xp_awk_val_int_t*)left)->val;
			res = xp_awk_makeintval (awk, r);
		}
		else if (left->type == XP_AWK_VAL_REAL)
		{
			xp_real_t r = ((xp_awk_val_real_t*)left)->val;
			res = xp_awk_makerealval (awk, r);
		}
		else
		{
			xp_awk_refdownval (awk, left);
			PANIC (awk, XP_AWK_EOPERAND);
		}
	}
	else if (exp->opcode == XP_AWK_UNROP_MINUS)
	{
		if (left->type == XP_AWK_VAL_INT)
		{
			xp_long_t r = ((xp_awk_val_int_t*)left)->val;
			res = xp_awk_makeintval (awk, -r);
		}
		else if (left->type == XP_AWK_VAL_REAL)
		{
			xp_real_t r = ((xp_awk_val_real_t*)left)->val;
			res = xp_awk_makerealval (awk, -r);
		}
		else
		{
			xp_awk_refdownval (awk, left);
			PANIC (awk, XP_AWK_EOPERAND);
		}
	}
	else if (exp->opcode == XP_AWK_UNROP_NOT)
	{
		if (left->type == XP_AWK_VAL_INT)
		{
			xp_long_t r = ((xp_awk_val_int_t*)left)->val;
			res = xp_awk_makeintval (awk, !r);
		}
		else if (left->type == XP_AWK_VAL_REAL)
		{
			xp_real_t r = ((xp_awk_val_real_t*)left)->val;
			res = xp_awk_makerealval (awk, !r);
		}
		else
		{
			xp_awk_refdownval (awk, left);
			PANIC (awk, XP_AWK_EOPERAND);
		}
	}
	else if (exp->opcode == XP_AWK_UNROP_BNOT)
	{
		if (left->type == XP_AWK_VAL_INT)
		{
			xp_long_t r = ((xp_awk_val_int_t*)left)->val;
			res = xp_awk_makeintval (awk, ~r);
		}
		else
		{
			xp_awk_refdownval (awk, left);
			PANIC (awk, XP_AWK_EOPERAND);
		}
	}

	if (res == XP_NULL)
	{
		xp_awk_refdownval (awk, left);
		PANIC (awk, XP_AWK_ENOMEM);
	}

	xp_awk_refdownval (awk, left);
	return res;
}

static xp_awk_val_t* __eval_incpre (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	xp_awk_val_t* left, * res;
	xp_awk_nde_exp_t* exp = (xp_awk_nde_exp_t*)nde;

	xp_assert (exp->type == XP_AWK_NDE_EXP_INCPRE);
	xp_assert (exp->left != XP_NULL && exp->right == XP_NULL);

	/* ugly. but let's keep going this way for the time being */
	/*if (exp->left->type != XP_AWK_NDE_NAMED && 
	    exp->left->type != XP_AWK_NDE_GLOBAL && 
	    exp->left->type != XP_AWK_NDE_LOCAL && 
	    exp->left->type != XP_AWK_NDE_ARG &&
	    exp->left->type != XP_AWK_NDE_NAMEDIDX &&
	    exp->left->type != XP_AWK_NDE_GLOBALIDX &&
	    exp->left->type != XP_AWK_NDE_LOCALIDX &&
	    exp->left->type != XP_AWK_NDE_ARGIDX) */
	if (exp->left->type < XP_AWK_NDE_NAMED ||
	    exp->left->type > XP_AWK_NDE_ARGIDX)
	{
		PANIC (awk, XP_AWK_EOPERAND);
	}

	left = __eval_expression (awk, exp->left);
	if (left == XP_NULL) return XP_NULL;

	xp_awk_refupval (left);

	if (exp->opcode == XP_AWK_INCOP_PLUS) 
	{
		if (left->type == XP_AWK_VAL_INT)
		{
			xp_long_t r = ((xp_awk_val_int_t*)left)->val;
			res = xp_awk_makeintval (awk, r + 1);
			if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
		}
		else if (left->type == XP_AWK_VAL_REAL)
		{
			xp_real_t r = ((xp_awk_val_real_t*)left)->val;
			res = xp_awk_makerealval (awk, r + 1.0);
			if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
		}
		else
		{
			xp_awk_refdownval (awk, left);
			PANIC (awk, XP_AWK_EOPERAND);
		}
	}
	else if (exp->opcode == XP_AWK_INCOP_MINUS)
	{
		if (left->type == XP_AWK_VAL_INT)
		{
			xp_long_t r = ((xp_awk_val_int_t*)left)->val;
			res = xp_awk_makeintval (awk, r - 1);
			if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
		}
		else if (left->type == XP_AWK_VAL_REAL)
		{
			xp_real_t r = ((xp_awk_val_real_t*)left)->val;
			res = xp_awk_makerealval (awk, r - 1.0);
			if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
		}
		else
		{
			xp_awk_refdownval (awk, left);
			PANIC (awk, XP_AWK_EOPERAND);
		}
	}
	else
	{
		xp_assert (!"should never happen - invalid opcode");
		xp_awk_refdownval (awk, left);
		PANIC (awk, XP_AWK_EINTERNAL);
	}

	if (__do_assignment(awk, (xp_awk_nde_var_t*)exp->left, res) == XP_NULL)
	{
		xp_awk_refdownval (awk, left);
		return XP_NULL;
	}

	xp_awk_refdownval (awk, left);
	return res;
}

static xp_awk_val_t* __eval_incpst (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	xp_awk_val_t* left, * res, * res2;
	xp_awk_nde_exp_t* exp = (xp_awk_nde_exp_t*)nde;

	xp_assert (exp->type == XP_AWK_NDE_EXP_INCPST);
	xp_assert (exp->left != XP_NULL && exp->right == XP_NULL);

	/* ugly. but let's keep going this way for the time being */
	/*if (exp->left->type != XP_AWK_NDE_NAMED && 
	    exp->left->type != XP_AWK_NDE_GLOBAL && 
	    exp->left->type != XP_AWK_NDE_LOCAL && 
	    exp->left->type != XP_AWK_NDE_ARG &&
	    exp->left->type != XP_AWK_NDE_NAMEDIDX &&
	    exp->left->type != XP_AWK_NDE_GLOBALIDX &&
	    exp->left->type != XP_AWK_NDE_LOCALIDX &&
	    exp->left->type != XP_AWK_NDE_ARGIDX) */
	if (exp->left->type < XP_AWK_NDE_NAMED ||
	    exp->left->type > XP_AWK_NDE_ARGIDX)
	{
		PANIC (awk, XP_AWK_EOPERAND);
	}

	left = __eval_expression (awk, exp->left);
	if (left == XP_NULL) return XP_NULL;

	xp_awk_refupval (left);

	if (exp->opcode == XP_AWK_INCOP_PLUS) 
	{
		if (left->type == XP_AWK_VAL_INT)
		{
			xp_long_t r = ((xp_awk_val_int_t*)left)->val;
			res = xp_awk_makeintval (awk, r);
			if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);

			res2 = xp_awk_makeintval (awk, r + 1);
			if (res2 == XP_NULL)
			{
				xp_awk_freeval (awk, res, xp_true);
				PANIC (awk, XP_AWK_ENOMEM);
			}
		}
		else if (left->type == XP_AWK_VAL_REAL)
		{
			xp_real_t r = ((xp_awk_val_real_t*)left)->val;
			res = xp_awk_makerealval (awk, r);
			if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);

			res2 = xp_awk_makerealval (awk, r + 1.0);
			if (res2 == XP_NULL)
			{
				xp_awk_freeval (awk, res, xp_true);
				PANIC (awk, XP_AWK_ENOMEM);
			}
		}
		else
		{
			xp_awk_refdownval (awk, left);
			PANIC (awk, XP_AWK_EOPERAND);
		}
	}
	else if (exp->opcode == XP_AWK_INCOP_MINUS)
	{
		if (left->type == XP_AWK_VAL_INT)
		{
			xp_long_t r = ((xp_awk_val_int_t*)left)->val;
			res = xp_awk_makeintval (awk, r);
			if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);

			res2 = xp_awk_makeintval (awk, r - 1);
			if (res2 == XP_NULL)
			{
				xp_awk_freeval (awk, res, xp_true);
				PANIC (awk, XP_AWK_ENOMEM);
			}
		}
		else if (left->type == XP_AWK_VAL_REAL)
		{
			xp_real_t r = ((xp_awk_val_real_t*)left)->val;
			res = xp_awk_makerealval (awk, r);
			if (res == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);

			res2 = xp_awk_makerealval (awk, r - 1.0);
			if (res2 == XP_NULL)
			{
				xp_awk_freeval (awk, res, xp_true);
				PANIC (awk, XP_AWK_ENOMEM);
			}
		}
		else
		{
			xp_awk_refdownval (awk, left);
			PANIC (awk, XP_AWK_EOPERAND);
		}
	}
	else
	{
		xp_assert (!"should never happen - invalid opcode");
		xp_awk_refdownval (awk, left);
		PANIC (awk, XP_AWK_EINTERNAL);
	}

	if (__do_assignment(awk, (xp_awk_nde_var_t*)exp->left, res2) == XP_NULL)
	{
		xp_awk_refdownval (awk, left);
		return XP_NULL;
	}

	xp_awk_refdownval (awk, left);
	return res;
}

static xp_awk_val_t* __eval_cnd (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	xp_awk_val_t* tv, * v;
	xp_awk_nde_cnd_t* cnd = (xp_awk_nde_cnd_t*)nde;

	tv = __eval_expression (awk, cnd->test);
	if (tv == XP_NULL) return XP_NULL;

	xp_awk_refupval (tv);

	v = (xp_awk_boolval(tv))?
		__eval_expression (awk, cnd->left):
		__eval_expression (awk, cnd->right);

	xp_awk_refdownval (awk, tv);
	return v;
}

static xp_awk_val_t* __eval_call (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	xp_awk_func_t* func;
	xp_awk_pair_t* pair;
	xp_awk_nde_t* p;
	xp_size_t nargs, i;
	xp_awk_val_t* v;
	xp_size_t saved_stack_top;
	xp_awk_nde_call_t* call = (xp_awk_nde_call_t*)nde;
	int n;

/*xp_printf (XP_TEXT(".....__eval_call\n"));*/
	pair = xp_awk_map_get (&awk->tree.funcs, call->name);
	if (pair == XP_NULL) PANIC (awk, XP_AWK_ENOSUCHFUNC);

	func = (xp_awk_func_t*)pair->val;
	xp_assert (func != XP_NULL);

	if (call->nargs > func->nargs)
	{
		/* TODO: is this correct? what if i want to allow arbitarary numbers of arguments? */
		PANIC (awk, XP_AWK_ETOOMANYARGS);
	}

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

	xp_assert (xp_sizeof(void*) >= xp_sizeof(awk->run.stack_top));
	xp_assert (xp_sizeof(void*) >= xp_sizeof(awk->run.stack_base));

	saved_stack_top = awk->run.stack_top;

/*xp_printf (XP_TEXT("setting up function stack frame stack_top = %ld stack_base = %ld\n"), awk->run.stack_top, awk->run.stack_base); */
	if (__raw_push(awk,(void*)awk->run.stack_base) == -1) 
	{
		PANIC (awk, XP_AWK_ENOMEM);
	}
	if (__raw_push(awk,(void*)saved_stack_top) == -1) 
	{
		__raw_pop (awk);
		PANIC (awk, XP_AWK_ENOMEM);
	}

	/* secure space for a return value. */
	if (__raw_push(awk,xp_awk_val_nil) == -1)
	{
		__raw_pop (awk);
		__raw_pop (awk);
		PANIC (awk, XP_AWK_ENOMEM);
	}

	/* secure space for nargs */
	if (__raw_push(awk,xp_awk_val_nil) == -1)
	{
		__raw_pop (awk);
		__raw_pop (awk);
		__raw_pop (awk);
		PANIC (awk, XP_AWK_ENOMEM);
	}

	nargs = 0;
	p = call->args;
	while (p != XP_NULL)
	{
		v = __eval_expression(awk,p);
		if (v == XP_NULL)
		{
			while (nargs > 0)
			{
/* TODO: test this portion. */
				--nargs;
				xp_awk_refdownval (awk, STACK_ARG(awk,nargs));
				__raw_pop (awk);
			}	

			__raw_pop (awk);
			__raw_pop (awk);
			__raw_pop (awk);
			return XP_NULL;
		}

		if (__raw_push(awk,v) == -1) 
		{
			/* ugly - v needs to be freed if it doesn't have
			 * any reference. but its reference has not been 
			 * updated for the successful stack push. so it adds
			 * up a reference and dereferences it*/
			xp_awk_refupval (v);
			xp_awk_refdownval (awk, v);

			while (nargs > 0)
			{
/* TODO: test this portion. */
				--nargs;
				xp_awk_refdownval (awk, STACK_ARG(awk,nargs));
				__raw_pop (awk);
			}	

			__raw_pop (awk);
			__raw_pop (awk);
			__raw_pop (awk);
			PANIC (awk, XP_AWK_ENOMEM);
		}

		xp_awk_refupval (v);
		nargs++;
		p = p->next;
	}

	xp_assert (nargs == call->nargs);

	while (nargs < func->nargs)
	{
		/* push as many nils as the number of missing actual arguments */
		if (__raw_push(awk,xp_awk_val_nil) == -1)
		{
			while (nargs > 0)
			{
/* TODO: test this portion. */
				--nargs;
				xp_awk_refdownval (awk, STACK_ARG(awk,nargs));
				__raw_pop (awk);
			}	

			__raw_pop (awk);
			__raw_pop (awk);
			__raw_pop (awk);
			PANIC (awk, XP_AWK_ENOMEM);
		}

		nargs++;
	}

	awk->run.stack_base = saved_stack_top;
	STACK_NARGS(awk) = (void*)nargs;
	
/*xp_printf (XP_TEXT("running function body\n")); */

	xp_assert (func->body->type == XP_AWK_NDE_BLK);
	n = __run_block(awk,(xp_awk_nde_blk_t*)func->body);

/*xp_printf (XP_TEXT("block run complete\n")); */

	/* refdown args in the run.stack */
	nargs = (xp_size_t)STACK_NARGS(awk);
/*xp_printf (XP_TEXT("block run complete nargs = %d\n"), nargs); */
	for (i = 0; i < nargs; i++)
	{
		xp_awk_refdownval (awk, STACK_ARG(awk,i));
	}
/*xp_printf (XP_TEXT("got return value\n")); */

	/* this is the trick mentioned in __run_return_statement.
	 * adjust the reference count of the return value.
	 * the value must not be freeed event if the reference count
	 * is decremented to zero because its reference is increased in 
	 * __run_return_statement regardless of its reference count. */
	v = STACK_RETVAL(awk);
	xp_awk_refdownval_nofree (awk, v);

	awk->run.stack_top =  (xp_size_t)awk->run.stack[awk->run.stack_base+1];
	awk->run.stack_base = (xp_size_t)awk->run.stack[awk->run.stack_base+0];

	if (awk->run.exit_level == EXIT_FUNCTION)
	{	
		awk->run.exit_level = EXIT_NONE;
	}

/*xp_printf (XP_TEXT("returning from function stack_top=%ld, stack_base=%ld\n"), awk->run.stack_top, awk->run.stack_base); */
	return (n == -1)? XP_NULL: v;
}

static xp_awk_val_t* __eval_int (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	xp_awk_val_t* val;
	val = xp_awk_makeintval (
		awk, ((xp_awk_nde_int_t*)nde)->val);
	if (val == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	return val;
}

static xp_awk_val_t* __eval_real (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	xp_awk_val_t* val;
	val = xp_awk_makerealval (
		awk, ((xp_awk_nde_real_t*)nde)->val);
	if (val == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	return val;
}

static xp_awk_val_t* __eval_str (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	xp_awk_val_t* val;
	val = xp_awk_makestrval (
		((xp_awk_nde_str_t*)nde)->buf,
		((xp_awk_nde_str_t*)nde)->len);
	if (val == XP_NULL) PANIC (awk, XP_AWK_ENOMEM);
	return val;
}

static xp_awk_val_t* __eval_named (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	xp_awk_pair_t* pair;
	xp_awk_nde_var_t* tgt = (xp_awk_nde_var_t*)nde;
		       
	pair = xp_awk_map_get(&awk->run.named,tgt->id.name);
	return (pair == XP_NULL)? xp_awk_val_nil: pair->val;
}

static xp_awk_val_t* __eval_global (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	xp_awk_nde_var_t* tgt = (xp_awk_nde_var_t*)nde;
	return STACK_GLOBAL(awk,tgt->id.idxa);
}

static xp_awk_val_t* __eval_local (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	xp_awk_nde_var_t* tgt = (xp_awk_nde_var_t*)nde;
	return STACK_LOCAL(awk,tgt->id.idxa);
}

static xp_awk_val_t* __eval_arg (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	xp_awk_nde_var_t* tgt = (xp_awk_nde_var_t*)nde;
	return STACK_ARG(awk,tgt->id.idxa);
}

static xp_awk_val_t* __eval_indexed (
	xp_awk_t* awk, xp_awk_nde_var_t* nde, xp_awk_val_map_t* map)
{
	xp_awk_val_t* idx, *res;
	xp_awk_pair_t* pair;
	xp_awk_nde_t* tmp;
	xp_char_t* str;

	/* TODO: should it be an error? should it return nil? */
	if (map->type != XP_AWK_VAL_MAP) 
	{
	        PANIC (awk, XP_AWK_ENOTINDEXABLE);
	}

	xp_assert (nde->idx != XP_NULL);

	tmp = nde->idx;
	/* TODO:
	while (tmp != XP_NULL)
	*/
	{
		int errnum;

		idx = __eval_expression (awk, tmp);
		if (idx == XP_NULL) return XP_NULL;

		xp_awk_refupval (idx);

		str = __val_to_str (idx, &errnum);
		if (str == XP_NULL) 
		{
			xp_awk_refdownval (awk, idx);
			PANIC (awk, errnum);
		}

		xp_awk_refdownval (awk, idx);
		/* TODO:
		tmp = tmp->next;
		 */
	}


/* TODO: check this out........ */
	pair = xp_awk_map_get (((xp_awk_val_map_t*)map)->map, str);
	res = (pair == XP_NULL)? xp_awk_val_nil: (xp_awk_val_t*)pair->val;

	xp_free (str);

	return res;
}

static xp_awk_val_t* __eval_namedidx (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	xp_awk_nde_var_t* tgt = (xp_awk_nde_var_t*)nde;
	xp_awk_pair_t* pair;

	pair = xp_awk_map_get (&awk->run.named, tgt->id.name);
	return __eval_indexed (awk, tgt, 
		(pair == XP_NULL)? xp_awk_val_nil: pair->val);
}

static xp_awk_val_t* __eval_globalidx (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	xp_awk_nde_var_t* tgt = (xp_awk_nde_var_t*)nde;
	return __eval_indexed (awk, tgt, STACK_GLOBAL(awk,tgt->id.idxa));
}

static xp_awk_val_t* __eval_localidx (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	xp_awk_nde_var_t* tgt = (xp_awk_nde_var_t*)nde;
	return __eval_indexed (awk, tgt, STACK_LOCAL(awk,tgt->id.idxa));
}

static xp_awk_val_t* __eval_argidx (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	xp_awk_nde_var_t* tgt = (xp_awk_nde_var_t*)nde;
	return __eval_indexed (awk, tgt, STACK_ARG(awk,tgt->id.idxa));
}

static xp_awk_val_t* __eval_pos (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	/* TODO: */
	xp_printf (XP_TEXT("eval_pos not competed....\n"));
	return XP_NULL;
}

static int __raw_push (xp_awk_t* awk, void* val)
{
	if (awk->run.stack_top >= awk->run.stack_limit)
	{
		void* tmp;
		xp_size_t n;
	       
		n = awk->run.stack_limit + STACK_INCREMENT;
		tmp = (void**)xp_realloc (awk->run.stack, n * xp_sizeof(void*));
		if (tmp == XP_NULL) return -1;

		awk->run.stack = tmp;
		awk->run.stack_limit = n;
	}

	awk->run.stack[awk->run.stack_top++] = val;
	return 0;
}

static void __raw_pop (xp_awk_t* awk)
{
	xp_assert (awk->run.stack_top > awk->run.stack_base);
	awk->run.stack_top--;
}

static void __raw_pop_times (xp_awk_t* awk, xp_size_t times)
{
	while (times > 0)
	{
		--times;
		__raw_pop (awk);
	}
}

static int __val_to_num (xp_awk_val_t* v, xp_long_t* l, xp_real_t* r)
{
	if (v->type == XP_AWK_VAL_NIL) 
	{
		*l = 0;
		return 0;
	}

	if (v->type == XP_AWK_VAL_INT)
	{
		*l = ((xp_awk_val_int_t*)v)->val;
		return 0;
	}

	if (v->type == XP_AWK_VAL_REAL)
	{
		*r = ((xp_awk_val_real_t*)v)->val;
		return 1;
	}

	if (v->type == XP_AWK_VAL_STR)
	{
		const xp_char_t* endptr;

		/* don't care about val->len */
		*l = xp_awk_strtolong (((xp_awk_val_str_t*)v)->buf, 0, &endptr);
	
		if (*endptr == XP_CHAR('.') ||
		    *endptr == XP_CHAR('E') ||
		    *endptr == XP_CHAR('e'))
		{
			*r = xp_awk_strtoreal (((xp_awk_val_str_t*)v)->buf);
			return 1; /* real */
		}
	
		return 0; /* long */
	}

	return -1; /* error */
}

static xp_char_t* __val_to_str (xp_awk_val_t* v, int* errnum)
{
	if (v->type == XP_AWK_VAL_INT)
	{
		xp_char_t* tmp;
		xp_long_t t;
		xp_size_t len = 0;

		t = ((xp_awk_val_int_t*)v)->val; 
		if (t < 0) { t = -t; len++; }
		while (t > 0) { len++; t /= 10; }

		tmp = xp_malloc (len + 1 * xp_sizeof(xp_char_t));
		if (tmp == XP_NULL)
		{
			*errnum = XP_AWK_ENOMEM;
			return XP_NULL;
		}

		t = ((xp_awk_val_int_t*)v)->val; 
		if (t < 0) t = -t;

		tmp[len] = XP_CHAR('\0');
		while (t > 0) 
		{
			tmp[--len] = (xp_char_t)(t % 10) + XP_CHAR('0');
			t /= 10;
		}

		if (v < 0) tmp[--len] = XP_CHAR('-');
		return tmp;
	}

	if (v->type == XP_AWK_VAL_STR) 
	{
		xp_char_t* tmp;
		tmp = xp_strxdup (
			((xp_awk_val_str_t*)v)->buf, 
			((xp_awk_val_str_t*)v)->len);

		if (tmp == XP_NULL) *errnum = XP_AWK_ENOMEM;
		return tmp;
	}

	/* TODO: process more value types */

	*errnum = XP_AWK_EWRONGINDEX;
	return XP_NULL;
}
