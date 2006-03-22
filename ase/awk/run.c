/*
 * $Id: run.c,v 1.13 2006-03-22 16:05:49 bacon Exp $
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

static xp_awk_val_t* __eval_expression (xp_awk_t* awk, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_assignment (xp_awk_t* awk, xp_awk_nde_ass_t* nde);
static xp_awk_val_t* __eval_binary (xp_awk_t* awk, xp_awk_nde_exp_t* nde);

static void __refup (xp_awk_val_t* val);
static void __refdown (xp_awk_val_t* val);

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
		if (__run_if_statement(awk,(xp_awk_nde_if_t*)nde) == -1) return -1;	
		break;

	case XP_AWK_NDE_WHILE:
		break;
	case XP_AWK_NDE_DOWHILE:
		break;
	case XP_AWK_NDE_FOR:
		break;

	case XP_AWK_NDE_BREAK:
		break;
	case XP_AWK_NDE_CONTINUE:
		break;

	case XP_AWK_NDE_RETURN:
		break;

	case XP_AWK_NDE_EXIT:
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
	if (xp_awk_isvaltrue(test))
	{
		n = __run_statement (awk, nde->then_part);
	}
	else if (nde->else_part != XP_NULL)
	{
		n = __run_statement (awk, nde->else_part);
	}

	xp_awk_freeval (test); // TODO: is this correct?
	return n;
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
			if (pair == XP_NULL) val = xp_awk_val_nil;
			else val = xp_awk_cloneval (pair->val);
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
		/* xp_awk_val_t* old, * new; */
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
		else name = pair->key;

		if (xp_awk_map_put(&awk->run.named, name, new) == XP_NULL) 
		{
			xp_free (name);
			xp_awk_freeval (new);
			awk->errnum = XP_AWK_ENOMEM;
			return XP_NULL;
		}

		__refup (new);
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

	right = __eval_expression (awk, nde->right);
	if (right == XP_NULL) 
	{
		xp_awk_freeval (left);
		return XP_NULL;
	}

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
	
	xp_awk_freeval(left);
	xp_awk_freeval(right);

	return res;
}

static void __refup (xp_awk_val_t* val)
{
	val->ref++;
}

static void __refdown (xp_awk_val_t* val)
{
	xp_assert (val->ref > 0);

	val->ref--;
	if (val->ref <= 0) xp_awk_freeval(val);
}
