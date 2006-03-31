/*
 * $Id: run.c,v 1.28 2006-03-31 16:35:37 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifndef __STAND_ALONE
#include <xp/bas/assert.h>
#include <xp/bas/string.h>
#include <xp/bas/memory.h>
#endif

#define STACK_INCREMENT 512

#define STACK_AT(awk,n) ((awk)->run.stack[(awk)->run.stack_base+(n)])
#define STACK_NARGS(awk) ((xp_size_t)STACK_AT(awk,3))
#define STACK_ARG(awk,n) STACK_AT(awk,3+1+(n))
#define STACK_LOCAL(awk,n) STACK_AT(awk,3+STACK_NARGS(awk)+1+(n))
#define STACK_RETVAL(awk) STACK_AT(awk,2)

#define STACK_GLOBAL(awk,n) ((awk)->run.stack[n])

#define EXIT_NONE      0
#define EXIT_BREAK     1
#define EXIT_CONTINUE  2
#define EXIT_FUNCTION  3
#define EXIT_GLOBAL    4

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
static xp_awk_val_t* __eval_assignment (xp_awk_t* awk, xp_awk_nde_ass_t* nde);
static xp_awk_val_t* __eval_binary (xp_awk_t* awk, xp_awk_nde_exp_t* nde);
static xp_awk_val_t* __eval_funccall (xp_awk_t* awk, xp_awk_nde_call_t* nde);

static int __raw_push (xp_awk_t* awk, void* val);
static void __raw_pop (xp_awk_t* awk);

int __printval (xp_awk_pair_t* pair)
{
	xp_printf (XP_TEXT("%s = "), (const xp_char_t*)pair->key);
	xp_awk_printval ((xp_awk_val_t*)pair->val);
	xp_printf (XP_TEXT("\n"));
	return 0;
}

int xp_awk_run (xp_awk_t* awk)
{
	xp_size_t nglobals;

	// TODO: clear run stack/exit_level
	awk->run.exit_level = EXIT_NONE;

	xp_assert (awk->run.stack_base == 0 && awk->run.stack_top == 0);

	/* secure space for global variables */
	nglobals = awk->tree.nglobals;
	while (nglobals > 0)
	{
		--nglobals;
		if (__raw_push(awk,xp_awk_val_nil) == -1)
		{
			// TODO: error handling
			return -1;
		}
	}	

	if (awk->tree.begin != XP_NULL) 
	{
		xp_assert (awk->tree.begin->type == XP_AWK_NDE_BLK);
		if (__run_block (awk, 
			(xp_awk_nde_blk_t*)awk->tree.begin) == -1) return -1;
	}

	if (awk->tree.end != XP_NULL) 
	{
		xp_assert (awk->tree.end->type == XP_AWK_NDE_BLK);
		if (__run_block (awk, 
			(xp_awk_nde_blk_t*)awk->tree.end) == -1) return -1;
	}

xp_printf (XP_TEXT("---------------------------\n"));
xp_awk_map_walk (&awk->run.named, __printval);
	return 0;
}

static int __run_block (xp_awk_t* awk, xp_awk_nde_blk_t* nde)
{
	xp_awk_nde_t* p;
	xp_size_t nlocals;

	xp_assert (nde->type == XP_AWK_NDE_BLK);

	p = nde->body;
	nlocals = nde->nlocals;

//xp_printf (XP_TEXT("securing space for local variables nlocals = %d\n"), nlocals);
	/* secure space for local variables */
	while (nlocals > 0)
	{
		--nlocals;
		if (__raw_push(awk,xp_awk_val_nil) == -1)
		{
			// TODO: proper error handling...
			return -1;
		}

		/* refupval is not required for xp_awk_val_nil */
	}

//xp_printf (XP_TEXT("executing block statements\n"));
	while (p != XP_NULL && awk->run.exit_level == EXIT_NONE) 
	{
		if (__run_statement(awk,p) == -1) return -1;
		p = p->next;
	}

//xp_printf (XP_TEXT("popping off local variables\n"));
	/* pop off local variables */
	nlocals = nde->nlocals;
	while (nlocals > 0)
	{
		--nlocals;
		xp_awk_refdownval (awk, STACK_LOCAL(awk,nlocals));
		__raw_pop (awk);
	}

	return 0;
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
		break;

	case XP_AWK_NDE_NEXTFILE:
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
	if (xp_awk_isvaltrue(test))
	{
		n = __run_statement (awk, nde->then_part);
	}
	else if (nde->else_part != XP_NULL)
	{
		n = __run_statement (awk, nde->else_part);
	}

	xp_awk_refdownval (awk, test); // TODO: is this correct?
	return n;
}

static int __run_while_statement (xp_awk_t* awk, xp_awk_nde_while_t* nde)
{
	xp_awk_val_t* test;

	if (nde->type == XP_AWK_NDE_WHILE)
	{
		while (1)
		{
			test = __eval_expression (awk, nde->test);
			if (test == XP_NULL) return -1;

			xp_awk_refupval (test);

			if (xp_awk_isvaltrue(test))
			{
				// TODO: break.... continue...., global exit, return... run-time abortion...
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
		do
		{
			if (__run_statement(awk,nde->body) == -1)
			{
				// TODO: error handling...
				return -1;
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

			test = __eval_expression (awk, nde->test);
			if (test == XP_NULL) return -1;

			xp_awk_refupval (test);
			if (!xp_awk_isvaltrue(test))
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
	if (nde->init != XP_NULL)
	{
		if (__eval_expression(awk,nde->init) == XP_NULL) return -1;
	}

	while (1)
	{
		if (nde->test != XP_NULL)
		{
			xp_awk_val_t* test;

			test = __eval_expression (awk, nde->test);
			if (test == XP_NULL) return -1;

			xp_awk_refupval (test);
			if (xp_awk_isvaltrue(test))
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
			if (__eval_expression(awk,nde->incr) == XP_NULL) return -1;
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
//xp_printf (XP_TEXT("returning....\n"));
		val = __eval_expression(awk, nde->val);
		if (val == XP_NULL) 
		{
			// TODO: error handling
			return -1;
		}

		STACK_RETVAL(awk) = val;
		xp_awk_refupval (val); /* see run_funccall for the trick */

//xp_printf (XP_TEXT("set return value....\n"));
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
		if (val == XP_NULL) 
		{
			// TODO: error handling
			return -1;
		}

// TODO: check out the line below..
		awk->run.stack[2] = val; /* global return value */
		xp_awk_refupval (val);
	}

	awk->run.exit_level = EXIT_GLOBAL;
	return 0;
}

static xp_awk_val_t* __eval_expression (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	xp_awk_val_t* val;

	switch (nde->type) 
	{
	case XP_AWK_NDE_ASS:
		val = __eval_assignment(awk,(xp_awk_nde_ass_t*)nde);
		break;

	case XP_AWK_NDE_EXP_BIN:
		val = __eval_binary(awk,(xp_awk_nde_exp_t*)nde);
		break;

	case XP_AWK_NDE_EXP_UNR:
		// TODO: .......................
		break;

	case XP_AWK_NDE_STR:
		val = xp_awk_makestrval(
			((xp_awk_nde_str_t*)nde)->buf,
			((xp_awk_nde_str_t*)nde)->len);
		break;

	case XP_AWK_NDE_INT:
		val = xp_awk_makeintval(awk,((xp_awk_nde_int_t*)nde)->val);
		break;

	case XP_AWK_NDE_REAL:
		val = xp_awk_makerealval(awk,((xp_awk_nde_real_t*)nde)->val);
		break;

	case XP_AWK_NDE_NAMED:
		{
			xp_awk_nde_var_t* tgt = (xp_awk_nde_var_t*)nde;
			xp_awk_pair_t* pair;
		       
			pair = xp_awk_map_get(&awk->run.named,tgt->id.name);
			val = (pair == XP_NULL)? xp_awk_val_nil: pair->val;
		}
		break;

	case XP_AWK_NDE_GLOBAL:
		{
			xp_awk_nde_var_t* tgt = (xp_awk_nde_var_t*)nde;
			val = STACK_GLOBAL(awk,tgt->id.idxa);
		}
		break;


	case XP_AWK_NDE_LOCAL:
		{
			xp_awk_nde_var_t* tgt = (xp_awk_nde_var_t*)nde;
			val = STACK_LOCAL(awk,tgt->id.idxa);
		}
		break;

	case XP_AWK_NDE_ARG:
		{
			xp_awk_nde_var_t* tgt = (xp_awk_nde_var_t*)nde;
			val = STACK_ARG(awk,tgt->id.idxa);
		}
		break;

	case XP_AWK_NDE_NAMEDIDX:
		break;
	case XP_AWK_NDE_GLOBALIDX:
		break;
	case XP_AWK_NDE_LOCALIDX:
		break;
	case XP_AWK_NDE_ARGIDX:
		break;

	case XP_AWK_NDE_POS:
		break;

	case XP_AWK_NDE_CALL:
		val = __eval_funccall(awk, (xp_awk_nde_call_t*)nde);
		if (val == XP_NULL) return XP_NULL;
		break;

	default:
		/* somthing wrong. internal error */
		/* TODO: set the error code instead of assertion below */
		xp_assert (XP_TEXT("should never happen") == XP_NULL);
		return XP_NULL;
	}

	return val;
}

static xp_awk_val_t* __eval_assignment (xp_awk_t* awk, xp_awk_nde_ass_t* nde)
{
	xp_awk_val_t* val;
	xp_awk_nde_var_t* tgt;

	xp_assert (nde->left != XP_NULL && nde->right != XP_NULL);

	tgt = (xp_awk_nde_var_t*)nde->left;
	val = __eval_expression(awk, nde->right);
	if (val == XP_NULL) return XP_NULL;

	if (tgt->type == XP_AWK_NDE_NAMED) 
	{
		xp_awk_pair_t* pair;
		xp_char_t* name;

		pair = xp_awk_map_get(&awk->run.named, tgt->id.name);
		if (pair == XP_NULL) 
		{
			name = xp_strdup (tgt->id.name);
			if (name == XP_NULL) 
			{
				xp_awk_freeval (awk, val);
				awk->errnum = XP_AWK_ENOMEM;
				return XP_NULL;
			}
		}
		else 
		{
			name = pair->key;
		}

		if (xp_awk_map_put(&awk->run.named, name, val) == XP_NULL) 
		{
			xp_free (name);
			xp_awk_freeval (awk, val);
			awk->errnum = XP_AWK_ENOMEM;
			return XP_NULL;
		}

		xp_awk_refupval (val);
	}
	else if (tgt->type == XP_AWK_NDE_GLOBAL) 
	{
		xp_awk_refdownval (awk, STACK_GLOBAL(awk,tgt->id.idxa));
		STACK_GLOBAL(awk,tgt->id.idxa) = val;
		xp_awk_refupval (val);
	}
	else if (tgt->type == XP_AWK_NDE_LOCAL) 
	{
		xp_awk_refdownval (awk, STACK_LOCAL(awk,tgt->id.idxa));
		STACK_LOCAL(awk,tgt->id.idxa) = val;
		xp_awk_refupval (val);
	}
	else if (tgt->type == XP_AWK_NDE_ARG) 
	{
		xp_awk_refdownval (awk, STACK_ARG(awk,tgt->id.idxa));
		STACK_ARG(awk,tgt->id.idxa) = val;
		xp_awk_refupval (val);
	}
	else if (tgt->type == XP_AWK_NDE_NAMEDIDX) 
	{
	}
	else if (tgt->type == XP_AWK_NDE_GLOBALIDX) 
	{
	}
	else if (tgt->type == XP_AWK_NDE_LOCALIDX) 
	{
	}
	else if (tgt->type == XP_AWK_NDE_ARGIDX) 
	{
	}
	else
	{
		/* this should never be reached. something wrong */
		// TODO: set errnum ....
		return XP_NULL;
	}

	return val;
}

static xp_awk_val_t* __eval_binary (xp_awk_t* awk, xp_awk_nde_exp_t* nde)
{
	xp_awk_val_t* left, * right, * res;

	xp_assert (nde->type == XP_AWK_NDE_EXP_BIN);

	left = __eval_expression (awk, nde->left);
	if (left == XP_NULL) return XP_NULL;

	xp_awk_refupval (left);

	right = __eval_expression (awk, nde->right);
	if (right == XP_NULL) 
	{
		xp_awk_refdownval (awk, left);
		return XP_NULL;
	}

	xp_awk_refupval (right);

	res = XP_NULL;

// TODO: a lot of things to do....
	if (nde->opcode == XP_AWK_BINOP_PLUS) 
	{
		if (left->type == XP_AWK_VAL_INT &&
		    right->type == XP_AWK_VAL_INT)
		{
			xp_long_t r = 
				((xp_awk_val_int_t*)left)->val + 
				((xp_awk_val_int_t*)right)->val;
			res = xp_awk_makeintval (awk, r);
		}
	}
	else if (nde->opcode == XP_AWK_BINOP_MINUS)
	{
		if (left->type == XP_AWK_VAL_INT &&
		    right->type == XP_AWK_VAL_INT)
		{
			xp_long_t r = 
				((xp_awk_val_int_t*)left)->val - 
				((xp_awk_val_int_t*)right)->val;
			res = xp_awk_makeintval (awk, r);
		}
	}
	else if (nde->opcode == XP_AWK_BINOP_MUL)
	{
		if (left->type == XP_AWK_VAL_INT &&
		    right->type == XP_AWK_VAL_INT)
		{
			xp_long_t r = 
				((xp_awk_val_int_t*)left)->val * 
				((xp_awk_val_int_t*)right)->val;
			res = xp_awk_makeintval (awk, r);
		}
	}
	else if (nde->opcode == XP_AWK_BINOP_DIV)
	{
		if (left->type == XP_AWK_VAL_INT &&
		    right->type == XP_AWK_VAL_INT)
		{
			xp_long_t r = 
				((xp_awk_val_int_t*)left)->val / 
				((xp_awk_val_int_t*)right)->val;
			res = xp_awk_makeintval (awk, r);
		}
	}
	else if (nde->opcode == XP_AWK_BINOP_MOD)
	{
		if (left->type == XP_AWK_VAL_INT &&
		    right->type == XP_AWK_VAL_INT)
		{
			xp_long_t r = 
				((xp_awk_val_int_t*)left)->val %
				((xp_awk_val_int_t*)right)->val;
			res = xp_awk_makeintval (awk, r);
		}
	}
	else if (nde->opcode == XP_AWK_BINOP_BXOR)
	{
		if (left->type == XP_AWK_VAL_INT &&
		    right->type == XP_AWK_VAL_INT)
		{
			xp_long_t r = 
				((xp_awk_val_int_t*)left)->val ^
				((xp_awk_val_int_t*)right)->val;
			res = xp_awk_makeintval (awk, r);
		}
	}


	xp_awk_refdownval (awk, left);
	xp_awk_refdownval (awk, right);

	return res;
}

static xp_awk_val_t* __eval_funccall (xp_awk_t* awk, xp_awk_nde_call_t* nde)
{
	xp_awk_func_t* func;
	xp_awk_pair_t* pair;
	xp_awk_nde_t* p;
	xp_size_t nargs, i;
	xp_awk_val_t* v;
	xp_size_t saved_stack_top;

	pair = xp_awk_map_get (&awk->tree.funcs, nde->name);
	if (pair == XP_NULL) return XP_NULL; /* no such function */

	/* 
	 * ---------------------
	 *  locn                 <- stack top
	 * ---------------------
	 *  ....
	 * ---------------------
	 *  loc0                 local variables are pushed by __run_block
	 * ---------------------
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
	 * ---------------------
	 */

	xp_assert (xp_sizeof(void*) >= xp_sizeof(awk->run.stack_top));
	xp_assert (xp_sizeof(void*) >= xp_sizeof(awk->run.stack_base));

	saved_stack_top = awk->run.stack_top;

//xp_printf (XP_TEXT("setting up function stack frame stack_top = %ld stack_base = %ld\n"), awk->run.stack_top, awk->run.stack_base);
	if (__raw_push(awk,(void*)awk->run.stack_base) == -1) return XP_NULL;
	if (__raw_push(awk,(void*)saved_stack_top) == -1) 
	{
		/* TODO: run.stack recovery */
		return XP_NULL;
	}

	/* secure space for return value. */
	if (__raw_push(awk,xp_awk_val_nil) == -1)
	{
		/* TODO: run.stack recovery */
		return XP_NULL;
	}

	/* secure space for nargs */
	if (__raw_push(awk,xp_awk_val_nil) == -1)
	{
		/* TODO: run.stack recovery */
		return XP_NULL;
	}

	nargs = 0;
	p = nde->args;
	while (p != XP_NULL)
	{
		v = __eval_expression(awk,p);
		if (v == XP_NULL)
		{
			/* TODO: run.stack recovery */
			return XP_NULL;
		}

		if (__raw_push(awk,v) == -1) 
		{
			/* TODO: run.stack recovery */
			return XP_NULL;
		}

		xp_awk_refupval (v);
		nargs++;
		p = p->next;
	}

	awk->run.stack_base = saved_stack_top;
	awk->run.stack[awk->run.stack_base+3] = (void*)nargs;

	func = (xp_awk_func_t*)pair->val;
	xp_assert (func != XP_NULL);

	// TODO: do i need to check if the number of arguments matches the actual arguments...???? this might be the compiler job...
	
//xp_printf (XP_TEXT("running function body\n"));

	xp_assert (func->body->type == XP_AWK_NDE_BLK);
	if (__run_block(awk,(xp_awk_nde_blk_t*)func->body) == -1) 
	{
		// TODO: handle error properly....
		return XP_NULL;
	}

//xp_printf (XP_TEXT("block run complete\n"));

	/* refdown args in the run.stack */
	nargs = STACK_NARGS(awk);
//xp_printf (XP_TEXT("block run complete nargs = %d\n"), nargs);
	for (i = 0; i < nargs; i++)
	{
		xp_awk_refdownval (awk, STACK_ARG(awk,i));
	}
//xp_printf (XP_TEXT("got return value\n"));

	/* adjust the reference count of the return value.
	 * the value must not be freeed event if the reference count
	 * is decremented to zero. */
	v = STACK_RETVAL(awk);
	xp_awk_refdownval_nofree (awk, v);

	awk->run.stack_top =  (xp_size_t)awk->run.stack[awk->run.stack_base+1];
	awk->run.stack_base = (xp_size_t)awk->run.stack[awk->run.stack_base+0];

	if (awk->run.exit_level == EXIT_FUNCTION)
	{	
		awk->run.exit_level = EXIT_NONE;
	}

//xp_printf (XP_TEXT("returning from function stack_top=%ld, stack_base=%ld\n"), awk->run.stack_top, awk->run.stack_base);
	return v;
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

