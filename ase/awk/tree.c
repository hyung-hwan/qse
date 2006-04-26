/*
 * $Id: tree.c,v 1.42 2006-04-26 15:49:33 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/memory.h>
#include <xp/bas/assert.h>
#include <xp/bas/stdio.h>
#endif

static const xp_char_t* __assop_str[] =
{
	XP_TEXT("="),
	XP_TEXT("+="),
	XP_TEXT("-="),
	XP_TEXT("*="),
	XP_TEXT("/="),
	XP_TEXT("%="),
	XP_TEXT("**=")
};

static const xp_char_t* __binop_str[] =
{
	XP_TEXT("||"),
	XP_TEXT("&&"),
	XP_TEXT("|"),
	XP_TEXT("^"),
	XP_TEXT("&"),

	XP_TEXT("=="),
	XP_TEXT("!="),
	XP_TEXT(">"),
	XP_TEXT(">="),
	XP_TEXT("<"),
	XP_TEXT("<="),

	XP_TEXT("<<"),
	XP_TEXT(">>"),

	XP_TEXT("+"),
	XP_TEXT("-"),
	XP_TEXT("*"),
	XP_TEXT("/"),
	XP_TEXT("%"),
	XP_TEXT("**"),

	XP_TEXT("in"),
	XP_TEXT("~"),
	XP_TEXT("!~")
};

static const xp_char_t* __unrop_str[] =
{
	XP_TEXT("+"),
	XP_TEXT("-"),
	XP_TEXT("!"),
	XP_TEXT("~")
};

static const xp_char_t* __incop_str[] =
{
	XP_TEXT("++"),
	XP_TEXT("--"),
	XP_TEXT("++"),
	XP_TEXT("--")
};

static void __print_tabs (int depth);
static int __print_expression (xp_awk_nde_t* nde);
static int __print_expression_list (xp_awk_nde_t* tree);
static void __print_statements (xp_awk_nde_t* tree, int depth);

static void __print_tabs (int depth)
{
	int i;
	for (i = 0; i < depth; i++) xp_printf (XP_TEXT("\t"));
}

static int __print_expression (xp_awk_nde_t* nde)
{
	switch (nde->type) 
	{
	case XP_AWK_NDE_GRP:
		{	
			xp_awk_nde_t* p = ((xp_awk_nde_grp_t*)nde)->body;

			xp_printf (XP_TEXT("("));
			while (p != XP_NULL) {
				__print_expression (p);
				if (p->next != XP_NULL) xp_printf (XP_TEXT(","));
				p = p->next;
			}
			xp_printf (XP_TEXT(")"));
		}
		break;

	case XP_AWK_NDE_ASS:
		if (__print_expression (((xp_awk_nde_ass_t*)nde)->left) == -1) return -1;
		xp_printf (XP_TEXT(" %s "), 
			__assop_str[((xp_awk_nde_exp_t*)nde)->opcode]);
		if (__print_expression (((xp_awk_nde_ass_t*)nde)->right) == -1) return -1;
		xp_assert ((((xp_awk_nde_ass_t*)nde)->right)->next == XP_NULL);
		break;

	case XP_AWK_NDE_EXP_BIN:
		xp_printf (XP_TEXT("("));
		if (__print_expression(((xp_awk_nde_exp_t*)nde)->left) == -1)
			return -1;
		xp_assert ((((xp_awk_nde_exp_t*)nde)->left)->next == XP_NULL);
		xp_printf (XP_TEXT(" %s "), 
			__binop_str[((xp_awk_nde_exp_t*)nde)->opcode]);
		if (((xp_awk_nde_exp_t*)nde)->right->type == XP_AWK_NDE_ASS) 
			xp_printf (XP_TEXT("("));
		if (__print_expression (((xp_awk_nde_exp_t*)nde)->right) == -1)
			return -1;
		if (((xp_awk_nde_exp_t*)nde)->right->type == XP_AWK_NDE_ASS)
			xp_printf (XP_TEXT(")"));
		xp_assert ((((xp_awk_nde_exp_t*)nde)->right)->next == XP_NULL);
		xp_printf (XP_TEXT(")"));
		break;

	case XP_AWK_NDE_EXP_UNR:
		xp_assert (((xp_awk_nde_exp_t*)nde)->right == XP_NULL);

		xp_printf (XP_TEXT("%s("), 
			__unrop_str[((xp_awk_nde_exp_t*)nde)->opcode]);
		if (__print_expression (((xp_awk_nde_exp_t*)nde)->left) == -1)
			return -1;
		xp_printf (XP_TEXT(")"));
		break;

	case XP_AWK_NDE_EXP_INCPRE:
		xp_assert (((xp_awk_nde_exp_t*)nde)->right == XP_NULL);

		xp_printf (XP_TEXT("%s("), 
			__incop_str[((xp_awk_nde_exp_t*)nde)->opcode]);
		if (__print_expression (((xp_awk_nde_exp_t*)nde)->left) == -1)
			return -1;
		xp_printf (XP_TEXT(")"));
		break;

	case XP_AWK_NDE_EXP_INCPST:
		xp_assert (((xp_awk_nde_exp_t*)nde)->right == XP_NULL);

		xp_printf (XP_TEXT("("));
		if (__print_expression (((xp_awk_nde_exp_t*)nde)->left) == -1)
			return -1;
		xp_printf (XP_TEXT(")%s"), 
			__incop_str[((xp_awk_nde_exp_t*)nde)->opcode]);
		break;

	case XP_AWK_NDE_CND:
		xp_printf (XP_TEXT("("));
		if (__print_expression(((xp_awk_nde_cnd_t*)nde)->test) == -1)
			return -1;
		xp_printf (XP_TEXT(")?"));

		if (__print_expression(((xp_awk_nde_cnd_t*)nde)->left) == -1)
			return -1;
		xp_printf (XP_TEXT(":"));
		if (__print_expression(((xp_awk_nde_cnd_t*)nde)->right) == -1)
			return -1;
		break;

	case XP_AWK_NDE_INT:
#if defined(__LCC__)
		xp_printf (XP_TEXT("%lld"), (long long)((xp_awk_nde_int_t*)nde)->val);
#elif defined(__BORLANDC__) || defined(_MSC_VER)
		xp_printf (XP_TEXT("%I64d"), (__int64)((xp_awk_nde_int_t*)nde)->val);
#elif defined(vax) || defined(__vax) || defined(_SCO_DS)
		xp_printf (XP_TEXT("%ld"), (long)((xp_awk_nde_int_t*)nde)->val);
#else
		xp_printf (XP_TEXT("%lld"), (long long)((xp_awk_nde_int_t*)nde)->val);
#endif
		break;

	case XP_AWK_NDE_REAL:
		xp_printf (XP_TEXT("%Lf"), (long double)((xp_awk_nde_real_t*)nde)->val);
		break;

	case XP_AWK_NDE_STR:
		/* TODO: buf, len */
		xp_printf (XP_TEXT("\"%s\""), ((xp_awk_nde_str_t*)nde)->buf);
		break;

	case XP_AWK_NDE_REX:
		/* TODO: buf, len */
		xp_printf (XP_TEXT("/%s/"), ((xp_awk_nde_rex_t*)nde)->buf);
		break;

	case XP_AWK_NDE_ARG:
		xp_assert (((xp_awk_nde_var_t*)nde)->id.idxa != (xp_size_t)-1);
		xp_printf (XP_TEXT("__arg%lu"), 
			(unsigned long)((xp_awk_nde_var_t*)nde)->id.idxa);
		xp_assert (((xp_awk_nde_var_t*)nde)->idx == XP_NULL);
		break;

	case XP_AWK_NDE_ARGIDX:
		xp_assert (((xp_awk_nde_var_t*)nde)->id.idxa != (xp_size_t)-1);
		xp_printf (XP_TEXT("__arg%lu["), 
			(unsigned long)((xp_awk_nde_var_t*)nde)->id.idxa);
		xp_assert (((xp_awk_nde_var_t*)nde)->idx != XP_NULL);
		__print_expression (((xp_awk_nde_var_t*)nde)->idx);
		xp_printf (XP_TEXT("]"));
		break;

	case XP_AWK_NDE_NAMED:
		xp_assert (((xp_awk_nde_var_t*)nde)->id.idxa == (xp_size_t)-1);
		xp_printf (XP_TEXT("%s"), ((xp_awk_nde_var_t*)nde)->id.name);
		xp_assert (((xp_awk_nde_var_t*)nde)->idx == XP_NULL);
		break;

	case XP_AWK_NDE_NAMEDIDX:
		xp_assert (((xp_awk_nde_var_t*)nde)->id.idxa == (xp_size_t)-1);
		xp_printf (XP_TEXT("%s["), ((xp_awk_nde_var_t*)nde)->id.name);
		xp_assert (((xp_awk_nde_var_t*)nde)->idx != XP_NULL);
		__print_expression (((xp_awk_nde_var_t*)nde)->idx);
		xp_printf (XP_TEXT("]"));
		break;

	case XP_AWK_NDE_GLOBAL:
		if (((xp_awk_nde_var_t*)nde)->id.idxa != (xp_size_t)-1) 
		{
			xp_printf (XP_TEXT("__global%lu"), 
				(unsigned long)((xp_awk_nde_var_t*)nde)->id.idxa);
		}
		else 
		{
			xp_printf (XP_TEXT("%s"), ((xp_awk_nde_var_t*)nde)->id.name);
		}
		xp_assert (((xp_awk_nde_var_t*)nde)->idx == XP_NULL);
		break;

	case XP_AWK_NDE_GLOBALIDX:
		if (((xp_awk_nde_var_t*)nde)->id.idxa != (xp_size_t)-1) 
		{
			xp_printf (XP_TEXT("__global%lu["), 
				(unsigned long)((xp_awk_nde_var_t*)nde)->id.idxa);
		}
		else 
		{
			xp_printf (XP_TEXT("%s["), ((xp_awk_nde_var_t*)nde)->id.name);
		}
		xp_assert (((xp_awk_nde_var_t*)nde)->idx != XP_NULL);
		__print_expression (((xp_awk_nde_var_t*)nde)->idx);
		xp_printf (XP_TEXT("]"));
		break;

	case XP_AWK_NDE_LOCAL:
		if (((xp_awk_nde_var_t*)nde)->id.idxa != (xp_size_t)-1) 
		{
			xp_printf (XP_TEXT("__local%lu"), 
				(unsigned long)((xp_awk_nde_var_t*)nde)->id.idxa);
		}
		else 
		{
			xp_printf (XP_TEXT("%s"), ((xp_awk_nde_var_t*)nde)->id.name);
		}
		xp_assert (((xp_awk_nde_var_t*)nde)->idx == XP_NULL);
		break;

	case XP_AWK_NDE_LOCALIDX:
		if (((xp_awk_nde_var_t*)nde)->id.idxa != (xp_size_t)-1) 
		{
			xp_printf (XP_TEXT("__local%lu["), 
				(unsigned long)((xp_awk_nde_var_t*)nde)->id.idxa);
		}
		else 
		{
			xp_printf (XP_TEXT("%s["), ((xp_awk_nde_var_t*)nde)->id.name);
		}
		xp_assert (((xp_awk_nde_var_t*)nde)->idx != XP_NULL);
		__print_expression (((xp_awk_nde_var_t*)nde)->idx);
		xp_printf (XP_TEXT("]"));
		break;

	case XP_AWK_NDE_POS:
		xp_printf (XP_TEXT("$"));
		__print_expression (((xp_awk_nde_pos_t*)nde)->val);
		break;

	case XP_AWK_NDE_CALL:
		xp_printf (XP_TEXT("%s ("), ((xp_awk_nde_call_t*)nde)->name);
		if (__print_expression_list (((xp_awk_nde_call_t*)nde)->args) == -1) return -1;
		xp_printf (XP_TEXT(")"));
		break;

	default:
		return -1;
	}

	return 0;
}

static int __print_expression_list (xp_awk_nde_t* tree)
{
	xp_awk_nde_t* p = tree;

	while (p != XP_NULL) 
	{
		if (__print_expression(p) == -1) return -1;
		p = p->next;
		if (p != XP_NULL) xp_printf (XP_TEXT(","));
	}

	return 0;
}

static void __print_statements (xp_awk_nde_t* tree, int depth)
{
	xp_awk_nde_t* p = tree;
	xp_size_t i;

	while (p != XP_NULL) 
	{

		switch (p->type) 
		{
		case XP_AWK_NDE_NULL:
			__print_tabs (depth);
			xp_printf (XP_TEXT(";\n"));
			break;

		case XP_AWK_NDE_BLK:
			__print_tabs (depth);
			xp_printf (XP_TEXT("{\n"));

			if (((xp_awk_nde_blk_t*)p)->nlocals > 0) 
			{
				__print_tabs (depth + 1);
				xp_printf (XP_TEXT("local "));

				for (i = 0; i < ((xp_awk_nde_blk_t*)p)->nlocals - 1; i++) 
				{
					xp_printf (XP_TEXT("__local%lu, "), (unsigned long)i);
				}
				xp_printf (XP_TEXT("__local%lu;\n"), (unsigned long)i);
			}

			__print_statements (((xp_awk_nde_blk_t*)p)->body, depth + 1);	
			__print_tabs (depth);
			xp_printf (XP_TEXT("}\n"));
			break;

		case XP_AWK_NDE_IF: 
			__print_tabs (depth);
			xp_printf (XP_TEXT("if ("));	
			__print_expression (((xp_awk_nde_if_t*)p)->test);
			xp_printf (XP_TEXT(")\n"));

			xp_assert (((xp_awk_nde_if_t*)p)->then_part != XP_NULL);
			if (((xp_awk_nde_if_t*)p)->then_part->type == XP_AWK_NDE_BLK)
				__print_statements (((xp_awk_nde_if_t*)p)->then_part, depth);
			else
				__print_statements (((xp_awk_nde_if_t*)p)->then_part, depth + 1);

			if (((xp_awk_nde_if_t*)p)->else_part != XP_NULL) 
			{
				__print_tabs (depth);
				xp_printf (XP_TEXT("else\n"));	
				if (((xp_awk_nde_if_t*)p)->else_part->type == XP_AWK_NDE_BLK)
					__print_statements (((xp_awk_nde_if_t*)p)->else_part, depth);
				else
					__print_statements (((xp_awk_nde_if_t*)p)->else_part, depth + 1);
			}
			break;
		case XP_AWK_NDE_WHILE: 
			__print_tabs (depth);
			xp_printf (XP_TEXT("while ("));	
			__print_expression (((xp_awk_nde_while_t*)p)->test);
			xp_printf (XP_TEXT(")\n"));
			if (((xp_awk_nde_while_t*)p)->body->type == XP_AWK_NDE_BLK) 
			{
				__print_statements (((xp_awk_nde_while_t*)p)->body, depth);
			}
			else 
			{
				__print_statements (((xp_awk_nde_while_t*)p)->body, depth + 1);
			}
			break;

		case XP_AWK_NDE_DOWHILE: 
			__print_tabs (depth);
			xp_printf (XP_TEXT("do\n"));	
			if (((xp_awk_nde_while_t*)p)->body->type == XP_AWK_NDE_BLK) 
			{
				__print_statements (((xp_awk_nde_while_t*)p)->body, depth);
			}
			else 
			{
				__print_statements (((xp_awk_nde_while_t*)p)->body, depth + 1);
			}

			__print_tabs (depth);
			xp_printf (XP_TEXT("while ("));	
			__print_expression (((xp_awk_nde_while_t*)p)->test);
			xp_printf (XP_TEXT(");\n"));	
			break;

		case XP_AWK_NDE_FOR:
			__print_tabs (depth);
			xp_printf (XP_TEXT("for ("));
			if (((xp_awk_nde_for_t*)p)->init != XP_NULL) 
			{
				__print_expression (((xp_awk_nde_for_t*)p)->init);
			}
			xp_printf (XP_TEXT("; "));
			if (((xp_awk_nde_for_t*)p)->test != XP_NULL) 
			{
				__print_expression (((xp_awk_nde_for_t*)p)->test);
			}
			xp_printf (XP_TEXT("; "));
			if (((xp_awk_nde_for_t*)p)->incr != XP_NULL) 
			{
				__print_expression (((xp_awk_nde_for_t*)p)->incr);
			}
			xp_printf (XP_TEXT(")\n"));

			if (((xp_awk_nde_for_t*)p)->body->type == XP_AWK_NDE_BLK) 
			{
				__print_statements (((xp_awk_nde_for_t*)p)->body, depth);
			}
			else 
			{
				__print_statements (((xp_awk_nde_for_t*)p)->body, depth + 1);
			}
			break;

		case XP_AWK_NDE_BREAK:
			__print_tabs (depth);
			xp_printf (XP_TEXT("break;\n"));
			break;

		case XP_AWK_NDE_CONTINUE:
			__print_tabs (depth);
			xp_printf (XP_TEXT("continue;\n"));
			break;

		case XP_AWK_NDE_RETURN:
			__print_tabs (depth);
			if (((xp_awk_nde_return_t*)p)->val == XP_NULL) 
			{
				xp_printf (XP_TEXT("return;\n"));
			}
			else 
			{
				xp_printf (XP_TEXT("return "));
				xp_assert (((xp_awk_nde_return_t*)p)->val->next == XP_NULL);
				if (__print_expression(((xp_awk_nde_return_t*)p)->val) == 0) 
				{
					xp_printf (XP_TEXT(";\n"));
				}
				else 
				{
					xp_awk_nde_return_t* x = (xp_awk_nde_return_t*)p;
					xp_printf (XP_TEXT("***INTERNAL ERROR: unknown nde type - %d\n"), x->type);
				}
			}
			break;

		case XP_AWK_NDE_EXIT:
			__print_tabs (depth);

			if (((xp_awk_nde_exit_t*)p)->val == XP_NULL) 
			{
				xp_printf (XP_TEXT("exit;\n"));
			}
			else 
			{
				xp_printf (XP_TEXT("exit "));
				xp_assert (((xp_awk_nde_exit_t*)p)->val->next == XP_NULL);
				if (__print_expression(((xp_awk_nde_exit_t*)p)->val) == 0) 
				{
					xp_printf (XP_TEXT(";\n"));
				}
				else 
				{
					xp_awk_nde_exit_t* x = (xp_awk_nde_exit_t*)p;
					xp_printf (XP_TEXT("***INTERNAL ERROR: unknown nde type - %d\n"), x->type);
				}
			}
			break;

		case XP_AWK_NDE_NEXT:
			__print_tabs (depth);
			xp_printf (XP_TEXT("next;\n"));
			break;

		case XP_AWK_NDE_NEXTFILE:
			__print_tabs (depth);
			xp_printf (XP_TEXT("nextfile;\n"));
			break;

		default:
			__print_tabs (depth);
			if (__print_expression(p) == 0) 
			{
				xp_printf (XP_TEXT(";\n"));
			}
			else 
			{
				xp_printf (XP_TEXT("***INTERNAL ERROR: unknown type - %d\n"), p->type);
			}
		}

		p = p->next;
	}
}

void xp_awk_prnpt (xp_awk_nde_t* tree)
{
	__print_statements (tree, 0);
}

void xp_awk_clrpt (xp_awk_nde_t* tree)
{
	xp_awk_nde_t* p = tree;
	xp_awk_nde_t* next;

	while (p != XP_NULL) 
	{
		next = p->next;

		switch (p->type) 
		{
		case XP_AWK_NDE_NULL:
			xp_free (p);
			break;

		case XP_AWK_NDE_BLK:
			xp_awk_clrpt (((xp_awk_nde_blk_t*)p)->body);
			xp_free (p);
			break;

		case XP_AWK_NDE_IF:
			xp_awk_clrpt (((xp_awk_nde_if_t*)p)->test);
			xp_awk_clrpt (((xp_awk_nde_if_t*)p)->then_part);

			if (((xp_awk_nde_if_t*)p)->else_part != XP_NULL)
				xp_awk_clrpt (((xp_awk_nde_if_t*)p)->else_part);
			xp_free (p);
			break;

		case XP_AWK_NDE_WHILE:
		case XP_AWK_NDE_DOWHILE:
			xp_awk_clrpt (((xp_awk_nde_while_t*)p)->test);
			xp_awk_clrpt (((xp_awk_nde_while_t*)p)->body);
			xp_free (p);
			break;

		case XP_AWK_NDE_FOR:
			if (((xp_awk_nde_for_t*)p)->init != XP_NULL)
				xp_awk_clrpt (((xp_awk_nde_for_t*)p)->init);
			if (((xp_awk_nde_for_t*)p)->test != XP_NULL)
				xp_awk_clrpt (((xp_awk_nde_for_t*)p)->test);
			if (((xp_awk_nde_for_t*)p)->incr != XP_NULL)
				xp_awk_clrpt (((xp_awk_nde_for_t*)p)->incr);
			xp_awk_clrpt (((xp_awk_nde_for_t*)p)->body);
			xp_free (p);
			break;

		case XP_AWK_NDE_BREAK:
			xp_free (p);
			break;
		case XP_AWK_NDE_CONTINUE:
			xp_free (p);
			break;

		case XP_AWK_NDE_NEXT:
		case XP_AWK_NDE_NEXTFILE:
			xp_free (p);
			break;
		
		case XP_AWK_NDE_RETURN:
			if (((xp_awk_nde_return_t*)p)->val != XP_NULL) 
				xp_awk_clrpt (((xp_awk_nde_return_t*)p)->val);
			xp_free (p);
			break;

		case XP_AWK_NDE_EXIT:
			if (((xp_awk_nde_exit_t*)p)->val != XP_NULL) 
				xp_awk_clrpt (((xp_awk_nde_exit_t*)p)->val);
			xp_free (p);
			break;

		case XP_AWK_NDE_GRP:
			xp_awk_clrpt (((xp_awk_nde_grp_t*)p)->body);
			xp_free (p);
			break;

		case XP_AWK_NDE_ASS:
			xp_awk_clrpt (((xp_awk_nde_ass_t*)p)->left);
			xp_awk_clrpt (((xp_awk_nde_ass_t*)p)->right);
			xp_free (p);
			break;

		case XP_AWK_NDE_EXP_BIN:
			xp_assert ((((xp_awk_nde_exp_t*)p)->left)->next == XP_NULL);
			xp_assert ((((xp_awk_nde_exp_t*)p)->right)->next == XP_NULL);

			xp_awk_clrpt (((xp_awk_nde_exp_t*)p)->left);
			xp_awk_clrpt (((xp_awk_nde_exp_t*)p)->right);
			xp_free (p);
			break;

		case XP_AWK_NDE_EXP_UNR:
		case XP_AWK_NDE_EXP_INCPRE:
		case XP_AWK_NDE_EXP_INCPST:
			xp_assert (((xp_awk_nde_exp_t*)p)->right == XP_NULL);
			xp_awk_clrpt (((xp_awk_nde_exp_t*)p)->left);
			xp_free (p);
			break;

		case XP_AWK_NDE_CND:
			xp_awk_clrpt (((xp_awk_nde_cnd_t*)p)->test);
			xp_awk_clrpt (((xp_awk_nde_cnd_t*)p)->left);
			xp_awk_clrpt (((xp_awk_nde_cnd_t*)p)->right);
			xp_free (p);
			break;

		case XP_AWK_NDE_INT:
			xp_free (p);
			break;

		case XP_AWK_NDE_REAL:
			xp_free (p);
			break;

		case XP_AWK_NDE_STR:
			xp_free (((xp_awk_nde_str_t*)p)->buf);
			xp_free (p);
			break;

		case XP_AWK_NDE_REX:
			xp_free (((xp_awk_nde_rex_t*)p)->buf);
			xp_free (p);
			break;

		case XP_AWK_NDE_NAMED:
			xp_assert (((xp_awk_nde_var_t*)p)->id.name != XP_NULL);
		case XP_AWK_NDE_GLOBAL:
		case XP_AWK_NDE_LOCAL:
		case XP_AWK_NDE_ARG:
			xp_assert (((xp_awk_nde_var_t*)p)->idx == XP_NULL);
			if (((xp_awk_nde_var_t*)p)->id.name != XP_NULL)
				xp_free (((xp_awk_nde_var_t*)p)->id.name);
			xp_free (p);
			break;

		case XP_AWK_NDE_NAMEDIDX:
			xp_assert (((xp_awk_nde_var_t*)p)->id.name != XP_NULL);
		case XP_AWK_NDE_GLOBALIDX:
		case XP_AWK_NDE_LOCALIDX:
		case XP_AWK_NDE_ARGIDX:
			xp_assert (((xp_awk_nde_var_t*)p)->idx != XP_NULL);
			xp_awk_clrpt (((xp_awk_nde_var_t*)p)->idx);
			if (((xp_awk_nde_var_t*)p)->id.name != XP_NULL)
				xp_free (((xp_awk_nde_var_t*)p)->id.name);
			xp_free (p);
			break;

		case XP_AWK_NDE_POS:
			xp_assert (((xp_awk_nde_pos_t*)p)->val != XP_NULL);
			xp_awk_clrpt (((xp_awk_nde_pos_t*)p)->val);
			xp_free (p);
			break;

		case XP_AWK_NDE_CALL:
			xp_free (((xp_awk_nde_call_t*)p)->name);
			xp_awk_clrpt (((xp_awk_nde_call_t*)p)->args);
			xp_free (p);
			break;

		default:
			xp_assert (!"should never happen - invalid node type");
			xp_free (p);
			break;
		}

		p = next;
	}
}
