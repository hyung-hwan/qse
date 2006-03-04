/*
 * $Id: run.c,v 1.6 2006-03-04 15:54:37 bacon Exp $
 */

#include <xp/awk/awk.h>

#ifndef __STAND_ALONE
#include <xp/bas/assert.h>
#endif

static int __run_block (xp_awk_t* awk, xp_awk_nde_t* nde);
static int __run_statement (xp_awk_t* awk, xp_awk_nde_t* nde);

static xp_awk_val_t* __eval_expression (xp_awk_t* awk, xp_awk_nde_t* nde);
static xp_awk_val_t* __eval_assignment (xp_awk_t* awk, xp_awk_nde_ass_t* nde);

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

	default:
		if (__eval_expression(awk,nde) == XP_NULL) return -1;
		break;
	}

	return 0;
}

static xp_awk_val_t* __eval_expression (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	xp_awk_val_t* val;

	switch (nde->type) {
	case XP_AWK_NDE_ASS:
		val = __eval_assignment(awk,(xp_awk_nde_ass_t*)nde);
		break;

	case XP_AWK_NDE_EXP_BIN:

	case XP_AWK_NDE_EXP_UNR:

	case XP_AWK_NDE_STR:
		break;

	case XP_AWK_NDE_NUM:
		// TODO: int, real...
		val = xp_awk_makeintval();
		break;

	case XP_AWK_NDE_ARG:

	case XP_AWK_NDE_ARGIDX:

	case XP_AWK_NDE_NAMED:

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

	if (nde->type == XP_AWK_NDE_NAMED) 
	{
		xp_awk_nde_var_t* tgt;
		xp_awk_val_t* old, * new;

		tgt = (xp_awk_nde_var_t*)nde->left;
		new = __eval_expression (awk, nde->right);

		xp_assert (tgt != XP_NULL);
		if (new == XP_NULL) return XP_NULL;

		old = (xp_awk_val_t*) xp_awk_map_get (&awk->run.named, tgt->id.name);

		if (xp_awk_map_put (
			&awk->run.named, tgt->id.name, new) == XP_NULL) 
		{
			xp_awk_freeval (new);
			awk->errnum = XP_AWK_ENOMEM;
			return XP_NULL;
		}
		else if (old != XP_NULL) 
		{
			/* free the old value that has been assigned to the variable */
			xp_awk_freeval (old);
		}

		v = new;
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
		/* this should never be reached. something wrong */
		// TODO: set errnum ....
		return XP_NULL;
	}

	return v;
}
