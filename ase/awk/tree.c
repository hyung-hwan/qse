/*
 * $Id: tree.c,v 1.58 2006-06-27 14:18:19 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/memory.h>
#include <xp/bas/assert.h>
#include <xp/bas/stdio.h>
#endif

static const xp_char_t* __assop_str[] =
{
	XP_T("="),
	XP_T("+="),
	XP_T("-="),
	XP_T("*="),
	XP_T("/="),
	XP_T("%="),
	XP_T("**=")
};

static const xp_char_t* __binop_str[] =
{
	XP_T("||"),
	XP_T("&&"),
	XP_T("in"),

	XP_T("|"),
	XP_T("^"),
	XP_T("&"),

	XP_T("=="),
	XP_T("!="),
	XP_T(">"),
	XP_T(">="),
	XP_T("<"),
	XP_T("<="),

	XP_T("<<"),
	XP_T(">>"),

	XP_T("+"),
	XP_T("-"),
	XP_T("*"),
	XP_T("/"),
	XP_T("%"),
	XP_T("**"),

	XP_T("~"),
	XP_T("!~")
};

static const xp_char_t* __unrop_str[] =
{
	XP_T("+"),
	XP_T("-"),
	XP_T("!"),
	XP_T("~")
};

static const xp_char_t* __incop_str[] =
{
	XP_T("++"),
	XP_T("--"),
	XP_T("++"),
	XP_T("--")
};

static const xp_char_t* __getline_inop_str[] =
{
	XP_T("|"),
	XP_T("|&"),
	XP_T("<")
};

static const xp_char_t* __print_outop_str[] =
{
	XP_T("|"),
	XP_T("|&"),
	XP_T(">"),
	XP_T(">>"),
	XP_T("")
};

static void __print_tabs (int depth);
static int __print_expression (xp_awk_nde_t* nde);
static int __print_expression_list (xp_awk_nde_t* tree);
static void __print_statements (xp_awk_nde_t* tree, int depth);

static void __print_tabs (int depth)
{
	int i;
	for (i = 0; i < depth; i++) xp_printf (XP_T("\t"));
}

static int __print_expression (xp_awk_nde_t* nde)
{
	switch (nde->type) 
	{
		case XP_AWK_NDE_GRP:
		{	
			xp_awk_nde_t* p = ((xp_awk_nde_grp_t*)nde)->body;

			xp_printf (XP_T("("));
			while (p != XP_NULL) {
				__print_expression (p);
				if (p->next != XP_NULL) xp_printf (XP_T(","));
				p = p->next;
			}
			xp_printf (XP_T(")"));
			break;
		}

		case XP_AWK_NDE_ASS:
		{
			xp_awk_nde_ass_t* px = (xp_awk_nde_ass_t*)nde;

			if (__print_expression (px->left) == -1) return -1;
			xp_printf (XP_T(" %s "), __assop_str[px->opcode]);
			if (__print_expression (px->right) == -1) return -1;
			xp_assert (px->right->next == XP_NULL);
			break;
		}

		case XP_AWK_NDE_EXP_BIN:
		{
			xp_awk_nde_exp_t* px = (xp_awk_nde_exp_t*)nde;

			xp_printf (XP_T("("));
			if (__print_expression(px->left) == -1) return -1;
			xp_assert (px->left->next == XP_NULL);
			xp_printf (XP_T(" %s "), __binop_str[px->opcode]);
			if (px->right->type == XP_AWK_NDE_ASS) xp_printf (XP_T("("));
			if (__print_expression (px->right) == -1) return -1;
			if (px->right->type == XP_AWK_NDE_ASS) xp_printf (XP_T(")"));
			xp_assert (px->right->next == XP_NULL); 
			xp_printf (XP_T(")"));
			break;
		}

		case XP_AWK_NDE_EXP_UNR:
		{
			xp_awk_nde_exp_t* px = (xp_awk_nde_exp_t*)nde;
			xp_assert (px->right == XP_NULL);

			xp_printf (XP_T("%s("), __unrop_str[px->opcode]);
			if (__print_expression (px->left) == -1) return -1;
			xp_printf (XP_T(")"));
			break;
		}

		case XP_AWK_NDE_EXP_INCPRE:
		{
			xp_awk_nde_exp_t* px = (xp_awk_nde_exp_t*)nde;
			xp_assert (px->right == XP_NULL);

			xp_printf (XP_T("%s("), __incop_str[px->opcode]);
			if (__print_expression (px->left) == -1) return -1;
			xp_printf (XP_T(")"));
			break;
		}

		case XP_AWK_NDE_EXP_INCPST:
		{
			xp_awk_nde_exp_t* px = (xp_awk_nde_exp_t*)nde;
			xp_assert (px->right == XP_NULL);

			xp_printf (XP_T("("));
			if (__print_expression (px->left) == -1) return -1;
			xp_printf (XP_T(")%s"), __incop_str[px->opcode]);
			break;
		}

		case XP_AWK_NDE_CND:
		{
			xp_awk_nde_cnd_t* px = (xp_awk_nde_cnd_t*)nde;

			xp_printf (XP_T("("));
			if (__print_expression(px->test) == -1) return -1;
			xp_printf (XP_T(")?"));

			if (__print_expression(px->left) == -1) return -1;
			xp_printf (XP_T(":"));
			if (__print_expression(px->right) == -1) return -1;
			break;
		}

		case XP_AWK_NDE_INT:
		{
		#if defined(__LCC__)
			xp_printf (XP_T("%lld"), 
				(long long)((xp_awk_nde_int_t*)nde)->val);
		#elif defined(__BORLANDC__) || defined(_MSC_VER)
			xp_printf (XP_T("%I64d"),
				(__int64)((xp_awk_nde_int_t*)nde)->val);
		#elif defined(vax) || defined(__vax) || defined(_SCO_DS)
			xp_printf (XP_T("%ld"),
				(long)((xp_awk_nde_int_t*)nde)->val);
		#else
			xp_printf (XP_T("%lld"),
				(long long)((xp_awk_nde_int_t*)nde)->val);
		#endif
			break;
		}

		case XP_AWK_NDE_REAL:
		{
			xp_printf (XP_T("%Lf"), 
				(long double)((xp_awk_nde_real_t*)nde)->val);
			break;
		}

		case XP_AWK_NDE_STR:
		{
			/* TODO: buf, len */
			xp_printf (XP_T("\"%s\""), ((xp_awk_nde_str_t*)nde)->buf);
			break;
		}

		case XP_AWK_NDE_REX:
		{
			/* TODO: buf, len */
			xp_printf (XP_T("/%s/"), ((xp_awk_nde_rex_t*)nde)->buf);
			break;
		}

		case XP_AWK_NDE_ARG:
		{
			xp_awk_nde_var_t* px = (xp_awk_nde_var_t*)nde;
			xp_assert (px->id.idxa != (xp_size_t)-1);
			xp_printf (XP_T("__arg%lu"), (unsigned long)px->id.idxa);
			xp_assert (px->idx == XP_NULL);
			break;
		}

		case XP_AWK_NDE_ARGIDX:
		{
			xp_awk_nde_var_t* px = (xp_awk_nde_var_t*)nde;
			xp_assert (px->id.idxa != (xp_size_t)-1);
			xp_printf (XP_T("__arg%lu["), (unsigned long)px->id.idxa);
			xp_assert (px->idx != XP_NULL);
			__print_expression_list (px->idx);
			xp_printf (XP_T("]"));
			break;
		}

		case XP_AWK_NDE_NAMED:
		{
			xp_awk_nde_var_t* px = (xp_awk_nde_var_t*)nde;
			xp_assert (px->id.idxa == (xp_size_t)-1);
			xp_printf (XP_T("%s"), px->id.name);
			xp_assert (px->idx == XP_NULL);
			break;
		}

		case XP_AWK_NDE_NAMEDIDX:
		{
			xp_awk_nde_var_t* px = (xp_awk_nde_var_t*)nde;
			xp_assert (px->id.idxa == (xp_size_t)-1);
			xp_printf (XP_T("%s["), px->id.name);
			xp_assert (px->idx != XP_NULL);
			__print_expression_list (px->idx);
			xp_printf (XP_T("]"));
			break;
		}

		case XP_AWK_NDE_GLOBAL:
		{
			xp_awk_nde_var_t* px = (xp_awk_nde_var_t*)nde;
			if (px->id.idxa != (xp_size_t)-1) 
			{
				xp_printf (XP_T("__global%lu"), 
					(unsigned long)px->id.idxa);
			}
			else 
			{
				xp_printf (XP_T("%s"), px->id.name);
			}
			xp_assert (px->idx == XP_NULL);
			break;
		}

		case XP_AWK_NDE_GLOBALIDX:
		{
			xp_awk_nde_var_t* px = (xp_awk_nde_var_t*)nde;
			if (px->id.idxa != (xp_size_t)-1) 
			{
				xp_printf (XP_T("__global%lu["), 
					(unsigned long)px->id.idxa);
			}
			else 
			{
				xp_printf (XP_T("%s["), px->id.name);
			}
			xp_assert (px->idx != XP_NULL);
			__print_expression_list (px->idx);
			xp_printf (XP_T("]"));
			break;
		}

		case XP_AWK_NDE_LOCAL:
		{
			xp_awk_nde_var_t* px = (xp_awk_nde_var_t*)nde;
			if (px->id.idxa != (xp_size_t)-1) 
			{
				xp_printf (XP_T("__local%lu"), 
					(unsigned long)px->id.idxa);
			}
			else 
			{
				xp_printf (XP_T("%s"), px->id.name);
			}
			xp_assert (px->idx == XP_NULL);
			break;
		}

		case XP_AWK_NDE_LOCALIDX:
		{
			xp_awk_nde_var_t* px = (xp_awk_nde_var_t*)nde;
			if (px->id.idxa != (xp_size_t)-1) 
			{
				xp_printf (XP_T("__local%lu["), 
					(unsigned long)px->id.idxa);
			}
			else 
			{
				xp_printf (XP_T("%s["), px->id.name);
			}
			xp_assert (px->idx != XP_NULL);
			__print_expression_list (px->idx);
			xp_printf (XP_T("]"));
			break;
		}

		case XP_AWK_NDE_POS:
		{
			xp_printf (XP_T("$"));
			__print_expression (((xp_awk_nde_pos_t*)nde)->val);
			break;
		}

		case XP_AWK_NDE_BFN:
		{
			xp_awk_nde_call_t* px = (xp_awk_nde_call_t*)nde;
			xp_printf (XP_T("%s ("), px->what.bfn->name);
			if (__print_expression_list (px->args) == -1) return -1;
			xp_printf (XP_T(")"));
			break;
		}

		case XP_AWK_NDE_UFN:
		{
			xp_awk_nde_call_t* px = (xp_awk_nde_call_t*)nde;
			xp_printf (XP_T("%s ("), px->what.name);
			if (__print_expression_list (px->args) == -1) return -1;
			xp_printf (XP_T(")"));
			break;
		}

		case XP_AWK_NDE_GETLINE:
		{
			  /* TODO */
			xp_awk_nde_getline_t* px = (xp_awk_nde_getline_t*)nde;
			if (px->in != XP_NULL &&
			    (px->in_type == XP_AWK_GETLINE_PIPE ||
			     px->in_type == XP_AWK_GETLINE_COPROC))
			{
				__print_expression (px->in);
				xp_printf (XP_T(" %s "), 
					__getline_inop_str[px->in_type]);
			}

			xp_printf (XP_T("getline"));
			if (px->var != XP_NULL)
			{
				xp_printf (XP_T(" "));
				__print_expression (px->var);
			}

			if (px->in != XP_NULL &&
			    px->in_type == XP_AWK_GETLINE_FILE)
			{
				xp_printf (XP_T(" %s "), 
					__getline_inop_str[px->in_type]);
				__print_expression (px->in);
			}	  
			break;
		}

		default:
		{
			return -1;
		}
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
		if (p != XP_NULL) xp_printf (XP_T(","));
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
			{
				__print_tabs (depth);
				xp_printf (XP_T(";\n"));
				break;
			}

			case XP_AWK_NDE_BLK:
			{
				xp_awk_nde_blk_t* px = (xp_awk_nde_blk_t*)p;

				__print_tabs (depth);
				xp_printf (XP_T("{\n"));

				if (px->nlocals > 0) 
				{
					__print_tabs (depth + 1);
					xp_printf (XP_T("local "));

					for (i = 0; i < px->nlocals - 1; i++) 
					{
						xp_printf (XP_T("__local%lu, "), (unsigned long)i);
					}
					xp_printf (XP_T("__local%lu;\n"), (unsigned long)i);
				}

				__print_statements (px->body, depth + 1);	
				__print_tabs (depth);
				xp_printf (XP_T("}\n"));
				break;
			}

			case XP_AWK_NDE_IF: 
			{
				xp_awk_nde_if_t* px = (xp_awk_nde_if_t*)p;

				__print_tabs (depth);
				xp_printf (XP_T("if ("));	
				__print_expression (px->test);
				xp_printf (XP_T(")\n"));

				xp_assert (px->then_part != XP_NULL);
				if (px->then_part->type == XP_AWK_NDE_BLK)
					__print_statements (px->then_part, depth);
				else
					__print_statements (px->then_part, depth + 1);

				if (px->else_part != XP_NULL) 
				{
					__print_tabs (depth);
					xp_printf (XP_T("else\n"));	
					if (px->else_part->type == XP_AWK_NDE_BLK)
						__print_statements (px->else_part, depth);
					else
						__print_statements (px->else_part, depth + 1);
				}
				break;
			}

			case XP_AWK_NDE_WHILE: 
			{
				xp_awk_nde_while_t* px = (xp_awk_nde_while_t*)p;

				__print_tabs (depth);
				xp_printf (XP_T("while ("));	
				__print_expression (px->test);
				xp_printf (XP_T(")\n"));
				if (px->body->type == XP_AWK_NDE_BLK) 
				{
					__print_statements (px->body, depth);
				}
				else 
				{
					__print_statements (px->body, depth + 1);
				}
				break;
			}

			case XP_AWK_NDE_DOWHILE: 
			{
				xp_awk_nde_while_t* px = (xp_awk_nde_while_t*)p;

				__print_tabs (depth);
				xp_printf (XP_T("do\n"));	
				if (px->body->type == XP_AWK_NDE_BLK) 
				{
					__print_statements (px->body, depth);
				}
				else 
				{
					__print_statements (px->body, depth + 1);
				}

				__print_tabs (depth);
				xp_printf (XP_T("while ("));	
				__print_expression (px->test);
				xp_printf (XP_T(");\n"));	
				break;
			}

			case XP_AWK_NDE_FOR:
			{
				xp_awk_nde_for_t* px = (xp_awk_nde_for_t*)p;

				__print_tabs (depth);
				xp_printf (XP_T("for ("));
				if (px->init != XP_NULL) 
				{
					__print_expression (px->init);
				}
				xp_printf (XP_T("; "));
				if (px->test != XP_NULL) 
				{
					__print_expression (px->test);
				}
				xp_printf (XP_T("; "));
				if (px->incr != XP_NULL) 
				{
					__print_expression (px->incr);
				}
				xp_printf (XP_T(")\n"));

				if (px->body->type == XP_AWK_NDE_BLK) 
				{
					__print_statements (px->body, depth);
				}
				else 
				{
					__print_statements (px->body, depth + 1);
				}
				break;
			}

			case XP_AWK_NDE_FOREACH:
			{
				xp_awk_nde_foreach_t* px = (xp_awk_nde_foreach_t*)p;

				__print_tabs (depth);
				xp_printf (XP_T("for "));
				__print_expression (px->test);
				xp_printf (XP_T("\n"));
				if (px->body->type == XP_AWK_NDE_BLK) 
				{
					__print_statements (px->body, depth);
				}
				else 
				{
					__print_statements (px->body, depth + 1);
				}
				break;
			}

			case XP_AWK_NDE_BREAK:
			{
				__print_tabs (depth);
				xp_printf (XP_T("break;\n"));
				break;
			}

			case XP_AWK_NDE_CONTINUE:
			{
				__print_tabs (depth);
				xp_printf (XP_T("continue;\n"));
				break;
			}

			case XP_AWK_NDE_RETURN:
			{
				__print_tabs (depth);
				if (((xp_awk_nde_return_t*)p)->val == XP_NULL) 
				{
					xp_printf (XP_T("return;\n"));
				}
				else 
				{
					xp_printf (XP_T("return "));
					xp_assert (((xp_awk_nde_return_t*)p)->val->next == XP_NULL);
					if (__print_expression(((xp_awk_nde_return_t*)p)->val) == 0) 
					{
						xp_printf (XP_T(";\n"));
					}
					else 
					{
						xp_awk_nde_return_t* x = (xp_awk_nde_return_t*)p;
						xp_printf (XP_T("***INTERNAL ERROR: unknown nde type - %d\n"), x->type);
					}
				}
				break;
			}

			case XP_AWK_NDE_EXIT:
			{
				xp_awk_nde_exit_t* px = (xp_awk_nde_exit_t*)p;
				__print_tabs (depth);

				if (px->val == XP_NULL) 
				{
					xp_printf (XP_T("exit;\n"));
				}
				else 
				{
					xp_printf (XP_T("exit "));
					xp_assert (px->val->next == XP_NULL);
					if (__print_expression(px->val) == 0) 
					{
						xp_printf (XP_T(";\n"));
					}
					else 
					{
						xp_printf (XP_T("***INTERNAL ERROR: unknown nde type - %d\n"), px->type);
					}
				}
				break;
			}

			case XP_AWK_NDE_NEXT:
			{
				__print_tabs (depth);
				xp_printf (XP_T("next;\n"));
				break;
			}

			case XP_AWK_NDE_NEXTFILE:
			{
				__print_tabs (depth);
				xp_printf (XP_T("nextfile;\n"));
				break;
			}

			case XP_AWK_NDE_DELETE:
			{
				__print_tabs (depth);
				xp_printf (XP_T("delete "));
				xp_awk_prnpt (((xp_awk_nde_delete_t*)p)->var);
				break;
			}

			case XP_AWK_NDE_PRINT:
			{
				xp_awk_nde_print_t* px = (xp_awk_nde_print_t*)p;

				__print_tabs (depth);

				xp_printf (XP_T("print"));
				if (px->args != XP_NULL)
				{
					xp_printf (XP_T(" "));
					__print_expression_list (px->args);
				}

				if (px->out != XP_NULL)
				{
					xp_printf (XP_T(" %s "), 
						__print_outop_str[px->out_type]);
					__print_expression (px->out);
				}

				xp_printf (XP_T(";\n"));
				break;
			}

			default:
			{
				__print_tabs (depth);
				if (__print_expression(p) == 0) 
				{
					xp_printf (XP_T(";\n"));
				}
				else 
				{
					xp_printf (XP_T("***INTERNAL ERROR: unknown type - %d\n"), p->type);
				}
			}
		}

		p = p->next;
	}
}

void xp_awk_prnpt (xp_awk_nde_t* tree)
{
	__print_statements (tree, 0);
}

void xp_awk_prnptnpt (xp_awk_nde_t* tree)
{
	xp_awk_nde_t* nde = tree;

	while (nde != XP_NULL)
	{
		__print_expression (nde);
		if (nde->next == XP_NULL) break;

		xp_printf (XP_T(","));
		nde = nde->next;
	}
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
			{
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_BLK:
			{
				xp_awk_clrpt (((xp_awk_nde_blk_t*)p)->body);
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_IF:
			{
				xp_awk_nde_if_t* px = (xp_awk_nde_if_t*)p;
				xp_awk_clrpt (px->test);
				xp_awk_clrpt (px->then_part);

				if (px->else_part != XP_NULL)
					xp_awk_clrpt (px->else_part);
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_WHILE:
			case XP_AWK_NDE_DOWHILE:
			{
				xp_awk_clrpt (((xp_awk_nde_while_t*)p)->test);
				xp_awk_clrpt (((xp_awk_nde_while_t*)p)->body);
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_FOR:
			{
				xp_awk_nde_for_t* px = (xp_awk_nde_for_t*)p;

				if (px->init != XP_NULL)
					xp_awk_clrpt (px->init);
				if (px->test != XP_NULL)
					xp_awk_clrpt (px->test);
				if (px->incr != XP_NULL)
					xp_awk_clrpt (px->incr);
				xp_awk_clrpt (px->body);
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_FOREACH:
			{
				xp_awk_clrpt (((xp_awk_nde_foreach_t*)p)->test);
				xp_awk_clrpt (((xp_awk_nde_foreach_t*)p)->body);
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_BREAK:
			{
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_CONTINUE:
			{
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_RETURN:
			{
				xp_awk_nde_return_t* px =
					(xp_awk_nde_return_t*)p;
				if (px->val != XP_NULL) xp_awk_clrpt (px->val);
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_EXIT:
			{
				if (((xp_awk_nde_exit_t*)p)->val != XP_NULL) 
					xp_awk_clrpt (((xp_awk_nde_exit_t*)p)->val);
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_NEXT:
			case XP_AWK_NDE_NEXTFILE:
			{
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_DELETE:
			{
				xp_awk_clrpt (((xp_awk_nde_delete_t*)p)->var);
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_PRINT:
			{
				xp_awk_nde_print_t* px = 
					(xp_awk_nde_print_t*)p;
				if (px->args != XP_NULL) xp_awk_clrpt (px->args);
				if (px->out != XP_NULL) xp_awk_clrpt (px->out);
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_GRP:
			{
				xp_awk_clrpt (((xp_awk_nde_grp_t*)p)->body);
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_ASS:
			{
				xp_awk_clrpt (((xp_awk_nde_ass_t*)p)->left);
				xp_awk_clrpt (((xp_awk_nde_ass_t*)p)->right);
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_EXP_BIN:
			{
				xp_awk_nde_exp_t* px = (xp_awk_nde_exp_t*)p;
				xp_assert (px->left->next == XP_NULL);
				xp_assert (px->right->next == XP_NULL);

				xp_awk_clrpt (px->left);
				xp_awk_clrpt (px->right);
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_EXP_UNR:
			case XP_AWK_NDE_EXP_INCPRE:
			case XP_AWK_NDE_EXP_INCPST:
			{
				xp_awk_nde_exp_t* px = (xp_awk_nde_exp_t*)p;
				xp_assert (px->right == XP_NULL);
				xp_awk_clrpt (px->left);
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_CND:
			{
				xp_awk_clrpt (((xp_awk_nde_cnd_t*)p)->test);
				xp_awk_clrpt (((xp_awk_nde_cnd_t*)p)->left);
				xp_awk_clrpt (((xp_awk_nde_cnd_t*)p)->right);
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_INT:
			{
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_REAL:
			{
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_STR:
			{
				xp_free (((xp_awk_nde_str_t*)p)->buf);
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_REX:
			{
				xp_free (((xp_awk_nde_rex_t*)p)->buf);
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_NAMED:
			case XP_AWK_NDE_GLOBAL:
			case XP_AWK_NDE_LOCAL:
			case XP_AWK_NDE_ARG:
			{
				xp_awk_nde_var_t* px = (xp_awk_nde_var_t*)p;
				xp_assert (px->idx == XP_NULL);
				if (px->id.name != XP_NULL)
					xp_free (px->id.name);
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_NAMEDIDX:
			case XP_AWK_NDE_GLOBALIDX:
			case XP_AWK_NDE_LOCALIDX:
			case XP_AWK_NDE_ARGIDX:
			{
				xp_awk_nde_var_t* px = (xp_awk_nde_var_t*)p;
				xp_assert (px->idx != XP_NULL);
				xp_awk_clrpt (px->idx);
				if (px->id.name != XP_NULL)
					xp_free (px->id.name);
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_POS:
			{
				xp_awk_clrpt (((xp_awk_nde_pos_t*)p)->val);
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_BFN:
			{
				xp_awk_nde_call_t* px = (xp_awk_nde_call_t*)p;
				/* xp_free (px->what.bfn); */
				xp_awk_clrpt (px->args);
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_UFN:
			{
				xp_awk_nde_call_t* px = (xp_awk_nde_call_t*)p;
				xp_free (px->what.name);
				xp_awk_clrpt (px->args);
				xp_free (p);
				break;
			}

			case XP_AWK_NDE_GETLINE:
			{
				xp_awk_nde_getline_t* px = 
					(xp_awk_nde_getline_t*)p;
				if (px->var != XP_NULL) xp_awk_clrpt (px->var);
				if (px->in != XP_NULL) xp_awk_clrpt (px->in);
				xp_free (p);
				break;
			}

			default:
			{
				xp_assert (!"should never happen - invalid node type");
				xp_free (p);
				break;
			}
		}

		p = next;
	}
}
