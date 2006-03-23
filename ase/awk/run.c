/*
 * $Id: run.c,v 1.15 2006-03-23 15:36:20 bacon Exp $
 */

#include <xp/awk/awk.h>

#ifndef __STAND_ALONE
#include <xp/bas/assert.h>
#include <xp/bas/string.h>
#include <xp/bas/memory.h>
#endif

static int __activate_block (xp_awk_t* awk, xp_awk_nde_blk_t* nde);
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

int __printval (xp_awk_pair_t* pair)
{
	xp_printf (XP_TEXT("%s = "), (const xp_char_t*)pair->key);
	xp_awk_printval ((xp_awk_val_t*)pair->val);
	xp_printf (XP_TEXT("\n"));
	return 0;
}

int xp_awk_run (xp_awk_t* awk)
{
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

static int __activate_block (xp_awk_t* awk, xp_awk_nde_blk_t* nde)
{
	/*
	if (nde->nlocals == 0 && awk->run.top_frame != XP_NULL) {
	}
	*/
	return -1;
}

static int __run_block (xp_awk_t* awk, xp_awk_nde_blk_t* nde)
{
	xp_awk_nde_t* p;

	xp_assert (nde->type == XP_AWK_NDE_BLK);

	p = nde->body;

	while (p != XP_NULL) 
	{
		if (__run_statement(awk,p) == -1) return -1;
		p = p->next;
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
	int n;

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

	xp_awk_refdownval (test); // TODO: is this correct?
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
					xp_awk_refdownval (test);
					return -1;
				}
			}
			else
			{
				xp_awk_refdownval (test);
				break;
			}

			xp_awk_refdownval (test);
		}
	}
	else if (nde->type == XP_AWK_NDE_DOWHILE)
	{
		do
		{
			test = __eval_expression (awk, nde->test);
			if (test == XP_NULL) return -1;

			xp_awk_refupval (test);

			if (xp_awk_isvaltrue(test))
			{
				// TODO: break.... continue...., global exit, return... run-time abortion...
				if (__run_statement(awk,nde->body) == -1)
				{
					xp_awk_refdownval (test);
					return -1;
				}
			}
			else
			{
				xp_awk_refdownval (test);
				break;
			}

			xp_awk_refdownval (test);
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
					xp_awk_refdownval (test);
					return -1;
				}
			}
			else
			{
				xp_awk_refdownval (test);
				break;
			}

			xp_awk_refdownval (test);
		}	
		else
		{
			if (__run_statement(awk,nde->body) == -1)
			{
				return -1;
			}
		}

		if (nde->incr != XP_NULL)
		{
			if (__eval_expression(awk,nde->incr) == XP_NULL) return -1;
		}
	}

	return 0;
}

static int __run_break_statement (xp_awk_t* awk, xp_awk_nde_break_t* nde)
{
	/* TODO: set runtime error number. 
	 * first of all this should not happen aas the compiler detects this. */
	/*
	if (awk->loop_depth <= 0) return -1; 
	awk->run.exec_break = 1;
	*/
	return 0;
}

static int __run_continue_statement (xp_awk_t* awk, xp_awk_nde_continue_t* nde)
{
	/* TODO: set runtime error number. 
	 * first of all this should not happen aas the compiler detects this. */
	/*
	if (awk->loop_depth <= 0) return -1; 
	awk->run.exec_continue = 1;
	*/
	return 0;
}

static int __run_return_statement (xp_awk_t* awk, xp_awk_nde_return_t* nde)
{
	return 0;
}

static int __run_exit_statement (xp_awk_t* awk, xp_awk_nde_exit_t* nde)
{
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
		val = xp_awk_makeintval(((xp_awk_nde_int_t*)nde)->val);
		break;

	/* TODO:
	case XP_AWK_NDE_REAL:
		val = xp_awk_makerealval(((xp_awk_nde_real_t*)nde)->val);
		break;
	*/

	case XP_AWK_NDE_ARG:

	case XP_AWK_NDE_ARGIDX:

	case XP_AWK_NDE_NAMED:
		{
			xp_awk_nde_var_t* tgt = (xp_awk_nde_var_t*)nde;
			xp_awk_pair_t* pair;
		       
			pair = xp_awk_map_get(&awk->run.named,tgt->id.name);
			/*
			if (pair == XP_NULL) val = xp_awk_val_nil;
			else val = xp_awk_cloneval (pair->val);
			*/
			val = (pair == XP_NULL)? xp_awk_val_nil: pair->val;
		}
		break;

	case XP_AWK_NDE_NAMEDIDX:

	case XP_AWK_NDE_GLOBAL:

	case XP_AWK_NDE_GLOBALIDX:

	case XP_AWK_NDE_LOCAL:

	case XP_AWK_NDE_LOCALIDX:

	case XP_AWK_NDE_POS:

	case XP_AWK_NDE_CALL:
		break;

	default:
		/* somthing wrong */
		return XP_NULL;
	}

	return val;
}

static xp_awk_val_t* __eval_assignment (xp_awk_t* awk, xp_awk_nde_ass_t* nde)
{
	xp_awk_val_t* v;
	xp_awk_nde_var_t* tgt;

	tgt = (xp_awk_nde_var_t*)nde->left;

	if (tgt->type == XP_AWK_NDE_NAMED) 
	{
		xp_awk_pair_t* pair;
		xp_awk_val_t* new;
		xp_char_t* name;

		new = __eval_expression(awk, nde->right);

		xp_assert (tgt != XP_NULL);
		if (new == XP_NULL) return XP_NULL;

		pair = xp_awk_map_get(&awk->run.named, tgt->id.name);
		if (pair == XP_NULL) 
		{
			name = xp_strdup (tgt->id.name);
			if (name == XP_NULL) 
			{
				xp_awk_freeval(new);
				awk->errnum = XP_AWK_ENOMEM;
				return XP_NULL;
			}
		}
		else 
		{
			name = pair->key;
		}

		if (xp_awk_map_put(&awk->run.named, name, new) == XP_NULL) 
		{
			xp_free (name);
			xp_awk_freeval (new);
			awk->errnum = XP_AWK_ENOMEM;
			return XP_NULL;
		}

		xp_awk_refupval (new);
		v = new;
	}
	else if (tgt->type == XP_AWK_NDE_GLOBAL) 
	{
	}
	else if (tgt->type == XP_AWK_NDE_LOCAL) 
	{
	}
	else if (tgt->type == XP_AWK_NDE_ARG) 
	{
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

	return v;
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
		xp_awk_refdownval (left);
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
			res = xp_awk_makeintval (r);
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
			res = xp_awk_makeintval (r);
		}
	}
	
	xp_awk_refdownval(left);
	xp_awk_refdownval(right);

	return res;
}

