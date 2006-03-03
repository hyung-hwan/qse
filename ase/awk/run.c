/*
 * $Id: run.c,v 1.5 2006-03-03 11:45:45 bacon Exp $
 */

#include <xp/awk/awk.h>

#ifndef __STAND_ALONE
#include <xp/bas/assert.h>
#endif

static int __run_block (xp_awk_t* awk, xp_awk_nde_t* nde);
static int __run_statement (xp_awk_t* awk, xp_awk_nde_t* nde);
static int __run_assignment (xp_awk_t* awk, xp_awk_nde_ass_t* nde);

static xp_awk_val_t* __eval_expr (xp_awk_t* awk, xp_awk_nde_t* nde);

int xp_awk_run (xp_awk_t* awk)
{
	if (awk->tree.begin != XP_NULL) 
	{
		if (__run_block(awk, awk->tree.begin) == -1) return -1;
	}

	if (awk->tree.end != XP_NULL) 
	{
		if (__run_block(awk, awk->tree.end) == -1) return -1;
	}

	return 0;
}

static int __run_block (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	xp_awk_nde_t* p;

	xp_assert (nde->type == XP_AWK_NDE_BLOCK);

	p = nde;

	while (p != XP_NULL) 
	{
		if (__run_statement(awk, p) == -1) return -1;
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

	case XP_AWK_NDE_BLOCK:
		if (__run_block(awk, nde) == -1) return -1;
		break;

#if 0
	case XP_AWK_NDE_IF:
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
#endif

	case XP_AWK_NDE_ASS:
		if (__run_assignment (
			awk, (xp_awk_nde_ass_t*)nde) == -1) return -1;
		break;

#if 0
	case XP_AWK_NDE_NUM:
		break;
#endif

	default:
		/* this should never be reached */
		// TODO: set errnum ....
		return -1;
	}

	return 0;
}

static int __run_assignment (xp_awk_t* awk, xp_awk_nde_ass_t* nde)
{
	if (nde->type == XP_AWK_NDE_NAMED) 
	{
		xp_awk_nde_var_t* left = (xp_awk_nde_var_t*)nde->left;
		xp_awk_val_t* right = __eval_expr (awk, nde->right);

		xp_assert (left != XP_NULL);
		if (right == XP_NULL) return -1;

		if (xp_awk_map_put (
			&awk->run.named, left->id.name, right) == XP_NULL) 
		{
			awk->errnum = XP_AWK_ENOMEM;
			return -1;
		}
	}
	else if (nde->type == XP_AWK_NDE_GLOBAL) 
	{
	}
	else if (nde->type == XP_AWK_NDE_LOCAL) 
	{
	}
	else if (nde->type == XP_AWK_NDE_ARG) 
	{
	}

	else if (nde->type == XP_AWK_NDE_NAMEDIDX) 
	{
	}
	else if (nde->type == XP_AWK_NDE_GLOBALIDX) 
	{
	}
	else if (nde->type == XP_AWK_NDE_LOCALIDX) 
	{
	}
	else if (nde->type == XP_AWK_NDE_ARGIDX) 
	{
	}
	else
	{
		/* this should never be reached */
		// TODO: set errnum ....
		return -1;
	}

	return 0;
}

static xp_awk_val_t* __eval_expr (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	return XP_NULL;
}
