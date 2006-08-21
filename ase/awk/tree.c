/*
 * $Id: tree.c,v 1.71 2006-08-21 14:51:32 bacon Exp $
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

	XP_T(" "),
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
	XP_T("<"),
	XP_T("")
};

static const xp_char_t* __print_outop_str[] =
{
	XP_T("|"),
	XP_T("|&"),
	XP_T(">"),
	XP_T(">>"),
	XP_T("")
};

#define PUT_SRCSTR(awk,str) \
	do { if (xp_awk_putsrcstr (awk, str) == -1) return -1; } while (0)

#define PUT_SRCSTRX(awk,str,len) \
	do { if (xp_awk_putsrcstrx (awk, str, len) == -1) return -1; } while (0)

#define PRINT_TABS(awk,depth) \
	do { if (__print_tabs(awk,depth) == -1) return -1; } while (0)

#define PRINT_EXPRESSION(awk,nde) \
	do { if (__print_expression(awk,nde) == -1) return -1; } while (0)

#define PRINT_EXPRESSION_LIST(awk,nde) \
	do { if (__print_expression_list(awk,nde) == -1) return -1; } while (0)

#define PRINT_STATEMENTS(awk,nde,depth) \
	do { if (__print_statements(awk,nde,depth) == -1) return -1; } while (0)

static int __print_tabs (xp_awk_t* awk, int depth);
static int __print_expression (xp_awk_t* awk, xp_awk_nde_t* nde);
static int __print_expression_list (xp_awk_t* awk, xp_awk_nde_t* tree);
static int __print_statements (xp_awk_t* awk, xp_awk_nde_t* tree, int depth);

static int __print_tabs (xp_awk_t* awk, int depth)
{
	while (depth > 0) 
	{
		PUT_SRCSTR (awk, XP_T("\t"));
		depth--;
	}

	return 0;
}

static int __print_expression (xp_awk_t* awk, xp_awk_nde_t* nde)
{
	xp_char_t tmp[128];

	switch (nde->type) 
	{
		case XP_AWK_NDE_GRP:
		{	
			xp_awk_nde_t* p = ((xp_awk_nde_grp_t*)nde)->body;

			PUT_SRCSTR (awk, XP_T("("));
			while (p != XP_NULL) 
			{
				PRINT_EXPRESSION (awk, p);
				if (p->next != XP_NULL) 
					PUT_SRCSTR (awk, XP_T(","));
				p = p->next;
			}
			PUT_SRCSTR (awk, XP_T(")"));
			break;
		}

		case XP_AWK_NDE_ASS:
		{
			xp_awk_nde_ass_t* px = (xp_awk_nde_ass_t*)nde;

			PRINT_EXPRESSION (awk, px->left);
			PUT_SRCSTR (awk, XP_T(" "));
			PUT_SRCSTR (awk, __assop_str[px->opcode]);
			PUT_SRCSTR (awk, XP_T(" "));
			PRINT_EXPRESSION (awk, px->right);

			xp_assert (px->right->next == XP_NULL);
			break;
		}

		case XP_AWK_NDE_EXP_BIN:
		{
			xp_awk_nde_exp_t* px = (xp_awk_nde_exp_t*)nde;

			PUT_SRCSTR (awk, XP_T("("));
			PRINT_EXPRESSION (awk, px->left);
			xp_assert (px->left->next == XP_NULL);

			PUT_SRCSTR (awk, XP_T(" "));
			PUT_SRCSTR (awk, __binop_str[px->opcode]);
			PUT_SRCSTR (awk, XP_T(" "));

			if (px->right->type == XP_AWK_NDE_ASS) 
				PUT_SRCSTR (awk, XP_T("("));
			PRINT_EXPRESSION (awk, px->right);
			if (px->right->type == XP_AWK_NDE_ASS) 
				PUT_SRCSTR (awk, XP_T(")"));
			xp_assert (px->right->next == XP_NULL); 
			PUT_SRCSTR (awk, XP_T(")"));
			break;
		}

		case XP_AWK_NDE_EXP_UNR:
		{
			xp_awk_nde_exp_t* px = (xp_awk_nde_exp_t*)nde;
			xp_assert (px->right == XP_NULL);

			PUT_SRCSTR (awk, __unrop_str[px->opcode]);
			PUT_SRCSTR (awk, XP_T("("));
			PRINT_EXPRESSION (awk, px->left);
			PUT_SRCSTR (awk, XP_T(")"));
			break;
		}

		case XP_AWK_NDE_EXP_INCPRE:
		{
			xp_awk_nde_exp_t* px = (xp_awk_nde_exp_t*)nde;
			xp_assert (px->right == XP_NULL);

			PUT_SRCSTR (awk, __incop_str[px->opcode]);
			PUT_SRCSTR (awk, XP_T("("));
			PRINT_EXPRESSION (awk, px->left);
			PUT_SRCSTR (awk, XP_T(")"));
			break;
		}

		case XP_AWK_NDE_EXP_INCPST:
		{
			xp_awk_nde_exp_t* px = (xp_awk_nde_exp_t*)nde;
			xp_assert (px->right == XP_NULL);

			PUT_SRCSTR (awk, XP_T("("));
			PRINT_EXPRESSION (awk, px->left);
			PUT_SRCSTR (awk, XP_T(")"));
			PUT_SRCSTR (awk, __incop_str[px->opcode]);
			break;
		}

		case XP_AWK_NDE_CND:
		{
			xp_awk_nde_cnd_t* px = (xp_awk_nde_cnd_t*)nde;

			PUT_SRCSTR (awk, XP_T("("));
			PRINT_EXPRESSION (awk, px->test);
			PUT_SRCSTR (awk, XP_T(")?"));

			PRINT_EXPRESSION (awk, px->left);
			PUT_SRCSTR (awk, XP_T(":"));
			PRINT_EXPRESSION (awk, px->right);
			break;
		}

		case XP_AWK_NDE_INT:
		{
		#if defined(__LCC__)
			xp_sprintf (tmp, xp_countof(tmp), XP_T("%lld"), 
				(long long)((xp_awk_nde_int_t*)nde)->val);
		#elif defined(__BORLANDC__) || defined(_MSC_VER)
			xp_sprintf (tmp, xp_countof(tmp), XP_T("%I64d"),
				(__int64)((xp_awk_nde_int_t*)nde)->val);
		#elif defined(vax) || defined(__vax) || defined(_SCO_DS)
			xp_sprintf (tmp, xp_countof(tmp), XP_T("%ld"),
				(long)((xp_awk_nde_int_t*)nde)->val);
		#else
			xp_sprintf (tmp, xp_countof(tmp), XP_T("%lld"),
				(long long)((xp_awk_nde_int_t*)nde)->val);
		#endif

			PUT_SRCSTR (awk, tmp);
			break;
		}

		case XP_AWK_NDE_REAL:
		{
			xp_sprintf (tmp, xp_countof(tmp), XP_T("%Lf"), 
				(long double)((xp_awk_nde_real_t*)nde)->val);
			PUT_SRCSTR (awk, tmp);
			break;
		}

		case XP_AWK_NDE_STR:
		{
			/* TODO: ESCAPING */
			PUT_SRCSTR (awk, XP_T("\""));
			PUT_SRCSTRX (awk,
				((xp_awk_nde_str_t*)nde)->buf, 
				((xp_awk_nde_str_t*)nde)->len);
			PUT_SRCSTR (awk, XP_T("\""));
			break;
		}

		case XP_AWK_NDE_REX:
		{
			/* TODO: buf, len */
			PUT_SRCSTR (awk, XP_T("/"));
			PUT_SRCSTRX (awk,
				((xp_awk_nde_rex_t*)nde)->buf, 
				((xp_awk_nde_rex_t*)nde)->len);
			PUT_SRCSTR (awk, XP_T("/"));
			break;
		}

		case XP_AWK_NDE_ARG:
		{
			xp_awk_nde_var_t* px = (xp_awk_nde_var_t*)nde;
			xp_assert (px->id.idxa != (xp_size_t)-1);

			PUT_SRCSTR (awk, XP_T("__param"));
			xp_sprintf (tmp, xp_countof(tmp),
				XP_T("%lu"), (unsigned long)px->id.idxa);
			PUT_SRCSTR (awk, tmp);

			xp_assert (px->idx == XP_NULL);
			break;
		}

		case XP_AWK_NDE_ARGIDX:
		{
			xp_awk_nde_var_t* px = (xp_awk_nde_var_t*)nde;
			xp_assert (px->id.idxa != (xp_size_t)-1);
			xp_assert (px->idx != XP_NULL);

			PUT_SRCSTR (awk, XP_T("__param"));
			xp_sprintf (tmp, xp_countof(tmp),
				XP_T("%lu"), (unsigned long)px->id.idxa);
			PUT_SRCSTR (awk, tmp);
			PUT_SRCSTR (awk, XP_T("["));
			PRINT_EXPRESSION_LIST (awk, px->idx);
			PUT_SRCSTR (awk, XP_T("]"));
			break;
		}

		case XP_AWK_NDE_NAMED:
		{
			xp_awk_nde_var_t* px = (xp_awk_nde_var_t*)nde;
			xp_assert (px->id.idxa == (xp_size_t)-1);
			xp_assert (px->idx == XP_NULL);

			PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
			break;
		}

		case XP_AWK_NDE_NAMEDIDX:
		{
			xp_awk_nde_var_t* px = (xp_awk_nde_var_t*)nde;
			xp_assert (px->id.idxa == (xp_size_t)-1);
			xp_assert (px->idx != XP_NULL);

			PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
			PUT_SRCSTR (awk, XP_T("["));
			PRINT_EXPRESSION_LIST (awk, px->idx);
			PUT_SRCSTR (awk, XP_T("]"));
			break;
		}

		case XP_AWK_NDE_GLOBAL:
		{
			xp_awk_nde_var_t* px = (xp_awk_nde_var_t*)nde;
			if (px->id.idxa != (xp_size_t)-1) 
			{
				PUT_SRCSTR (awk, XP_T("__global"));
				xp_sprintf (tmp, xp_countof(tmp),
					XP_T("%lu"), (unsigned long)px->id.idxa);
				PUT_SRCSTR (awk, tmp);
			}
			else 
			{
				PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
			}
			xp_assert (px->idx == XP_NULL);
			break;
		}

		case XP_AWK_NDE_GLOBALIDX:
		{
			xp_awk_nde_var_t* px = (xp_awk_nde_var_t*)nde;
			if (px->id.idxa != (xp_size_t)-1) 
			{
				PUT_SRCSTR (awk, XP_T("__global"));
				xp_sprintf (tmp, xp_countof(tmp),
					XP_T("%lu"), (unsigned long)px->id.idxa);
				PUT_SRCSTR (awk, tmp);
				PUT_SRCSTR (awk, XP_T("["));
			}
			else 
			{
				PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
				PUT_SRCSTR (awk, XP_T("["));
			}
			xp_assert (px->idx != XP_NULL);
			PRINT_EXPRESSION_LIST (awk, px->idx);
			PUT_SRCSTR (awk, XP_T("]"));
			break;
		}

		case XP_AWK_NDE_LOCAL:
		{
			xp_awk_nde_var_t* px = (xp_awk_nde_var_t*)nde;
			if (px->id.idxa != (xp_size_t)-1) 
			{
				PUT_SRCSTR (awk, XP_T("__local"));
				xp_sprintf (tmp, xp_countof(tmp),
					XP_T("%lu"), (unsigned long)px->id.idxa);
				PUT_SRCSTR (awk, tmp);
			}
			else 
			{
				PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
			}
			xp_assert (px->idx == XP_NULL);
			break;
		}

		case XP_AWK_NDE_LOCALIDX:
		{
			xp_awk_nde_var_t* px = (xp_awk_nde_var_t*)nde;
			if (px->id.idxa != (xp_size_t)-1) 
			{
				PUT_SRCSTR (awk, XP_T("__local"));
				xp_sprintf (tmp, xp_countof(tmp),
					XP_T("%lu"), (unsigned long)px->id.idxa);
				PUT_SRCSTR (awk, tmp);
				PUT_SRCSTR (awk, XP_T("["));
			}
			else 
			{
				PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
				PUT_SRCSTR (awk, XP_T("["));
			}
			xp_assert (px->idx != XP_NULL);
			PRINT_EXPRESSION_LIST (awk, px->idx);
			PUT_SRCSTR (awk, XP_T("]"));
			break;
		}

		case XP_AWK_NDE_POS:
		{
			PUT_SRCSTR (awk, XP_T("$"));
			PRINT_EXPRESSION (awk, ((xp_awk_nde_pos_t*)nde)->val);
			break;
		}

		case XP_AWK_NDE_BFN:
		{
			xp_awk_nde_call_t* px = (xp_awk_nde_call_t*)nde;
			PUT_SRCSTRX (awk, 
				px->what.bfn.name, px->what.bfn.name_len);
			PUT_SRCSTR (awk, XP_T(" ("));
			PRINT_EXPRESSION_LIST (awk, px->args);
			PUT_SRCSTR (awk, XP_T(")"));
			break;
		}

		case XP_AWK_NDE_AFN:
		{
			/* TODO: use px->what.afn.name_len */
			xp_awk_nde_call_t* px = (xp_awk_nde_call_t*)nde;
			PUT_SRCSTRX (awk, 
				px->what.afn.name, px->what.afn.name_len);
			PUT_SRCSTR (awk, XP_T(" ("));
			PRINT_EXPRESSION_LIST (awk, px->args);
			PUT_SRCSTR (awk, XP_T(")"));
			break;
		}

		case XP_AWK_NDE_GETLINE:
		{
			xp_awk_nde_getline_t* px = (xp_awk_nde_getline_t*)nde;
			if (px->in != XP_NULL &&
			    (px->in_type == XP_AWK_IN_PIPE ||
			     px->in_type == XP_AWK_IN_COPROC))
			{
				PRINT_EXPRESSION (awk, px->in);
				PUT_SRCSTR (awk, XP_T(" "));
				PUT_SRCSTR (awk, __getline_inop_str[px->in_type]);
				PUT_SRCSTR (awk, XP_T(" "));
			}

			PUT_SRCSTR (awk, XP_T("getline"));
			if (px->var != XP_NULL)
			{
				PUT_SRCSTR (awk, XP_T(" "));
				PRINT_EXPRESSION (awk, px->var);
			}

			if (px->in != XP_NULL &&
			    px->in_type == XP_AWK_IN_FILE)
			{
				PUT_SRCSTR (awk, XP_T(" "));
				PUT_SRCSTR (awk, __getline_inop_str[px->in_type]);
				PUT_SRCSTR (awk, XP_T(" "));
				PRINT_EXPRESSION (awk, px->in);
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

static int __print_expression_list (xp_awk_t* awk, xp_awk_nde_t* tree)
{
	xp_awk_nde_t* p = tree;

	while (p != XP_NULL) 
	{
		PRINT_EXPRESSION (awk, p);
		p = p->next;
		if (p != XP_NULL) PUT_SRCSTR (awk, XP_T(","));
	}

	return 0;
}

static int __print_statements (xp_awk_t* awk, xp_awk_nde_t* tree, int depth)
{
	xp_awk_nde_t* p = tree;
	xp_char_t tmp[128];
	xp_size_t i;

	while (p != XP_NULL) 
	{

		switch (p->type) 
		{
			case XP_AWK_NDE_NULL:
			{
				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, XP_T(";\n"));
				break;
			}

			case XP_AWK_NDE_BLK:
			{
				xp_awk_nde_blk_t* px = (xp_awk_nde_blk_t*)p;

				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, XP_T("{\n"));

				if (px->nlocals > 0) 
				{
					PRINT_TABS (awk, depth + 1);
					PUT_SRCSTR (awk, XP_T("local "));

					for (i = 0; i < px->nlocals - 1; i++) 
					{
						PUT_SRCSTR (awk, XP_T("__local"));
						xp_sprintf (tmp, xp_countof(tmp), XP_T("%lu"), (unsigned long)i);
						PUT_SRCSTR (awk, tmp);
						PUT_SRCSTR (awk, XP_T(", "));
					}

					PUT_SRCSTR (awk, XP_T("__local"));
					xp_sprintf (tmp, xp_countof(tmp), XP_T("%lu"), (unsigned long)i);
					PUT_SRCSTR (awk, tmp);
					PUT_SRCSTR (awk, XP_T(";\n"));
				}

				PRINT_STATEMENTS (awk, px->body, depth + 1);	
				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, XP_T("}\n"));
				break;
			}

			case XP_AWK_NDE_IF: 
			{
				xp_awk_nde_if_t* px = (xp_awk_nde_if_t*)p;

				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, XP_T("if ("));	
				PRINT_EXPRESSION (awk, px->test);
				PUT_SRCSTR (awk, XP_T(")\n"));

				xp_assert (px->then_part != XP_NULL);
				if (px->then_part->type == XP_AWK_NDE_BLK)
					PRINT_STATEMENTS (awk, px->then_part, depth);
				else
					PRINT_STATEMENTS (awk, px->then_part, depth + 1);

				if (px->else_part != XP_NULL) 
				{
					PRINT_TABS (awk, depth);
					PUT_SRCSTR (awk, XP_T("else\n"));	
					if (px->else_part->type == XP_AWK_NDE_BLK)
						PRINT_STATEMENTS (awk, px->else_part, depth);
					else
						PRINT_STATEMENTS (awk, px->else_part, depth + 1);
				}
				break;
			}

			case XP_AWK_NDE_WHILE: 
			{
				xp_awk_nde_while_t* px = (xp_awk_nde_while_t*)p;

				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, XP_T("while ("));	
				PRINT_EXPRESSION (awk, px->test);
				PUT_SRCSTR (awk, XP_T(")\n"));
				if (px->body->type == XP_AWK_NDE_BLK) 
				{
					PRINT_STATEMENTS (awk, px->body, depth);
				}
				else 
				{
					PRINT_STATEMENTS (awk, px->body, depth + 1);
				}
				break;
			}

			case XP_AWK_NDE_DOWHILE: 
			{
				xp_awk_nde_while_t* px = (xp_awk_nde_while_t*)p;

				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, XP_T("do\n"));	
				if (px->body->type == XP_AWK_NDE_BLK) 
				{
					PRINT_STATEMENTS (awk, px->body, depth);
				}
				else 
				{
					PRINT_STATEMENTS (awk, px->body, depth + 1);
				}

				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, XP_T("while ("));	
				PRINT_EXPRESSION (awk, px->test);
				PUT_SRCSTR (awk, XP_T(");\n"));	
				break;
			}

			case XP_AWK_NDE_FOR:
			{
				xp_awk_nde_for_t* px = (xp_awk_nde_for_t*)p;

				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, XP_T("for ("));
				if (px->init != XP_NULL) 
				{
					PRINT_EXPRESSION (awk, px->init);
				}
				PUT_SRCSTR (awk, XP_T("; "));
				if (px->test != XP_NULL) 
				{
					PRINT_EXPRESSION (awk, px->test);
				}
				PUT_SRCSTR (awk, XP_T("; "));
				if (px->incr != XP_NULL) 
				{
					PRINT_EXPRESSION (awk, px->incr);
				}
				PUT_SRCSTR (awk, XP_T(")\n"));

				if (px->body->type == XP_AWK_NDE_BLK) 
				{
					PRINT_STATEMENTS (awk, px->body, depth);
				}
				else 
				{
					PRINT_STATEMENTS (awk, px->body, depth + 1);
				}
				break;
			}

			case XP_AWK_NDE_FOREACH:
			{
				xp_awk_nde_foreach_t* px = (xp_awk_nde_foreach_t*)p;

				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, XP_T("for "));
				PRINT_EXPRESSION (awk, px->test);
				PUT_SRCSTR (awk, XP_T("\n"));
				if (px->body->type == XP_AWK_NDE_BLK) 
				{
					PRINT_STATEMENTS (awk, px->body, depth);
				}
				else 
				{
					PRINT_STATEMENTS (awk, px->body, depth + 1);
				}
				break;
			}

			case XP_AWK_NDE_BREAK:
			{
				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, XP_T("break;\n"));
				break;
			}

			case XP_AWK_NDE_CONTINUE:
			{
				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, XP_T("continue;\n"));
				break;
			}

			case XP_AWK_NDE_RETURN:
			{
				PRINT_TABS (awk, depth);
				if (((xp_awk_nde_return_t*)p)->val == XP_NULL) 
				{
					PUT_SRCSTR (awk, XP_T("return;\n"));
				}
				else 
				{
					PUT_SRCSTR (awk, XP_T("return "));
					xp_assert (((xp_awk_nde_return_t*)p)->val->next == XP_NULL);

					PRINT_EXPRESSION (awk, ((xp_awk_nde_return_t*)p)->val);
					PUT_SRCSTR (awk, XP_T(";\n"));
				}
				break;
			}

			case XP_AWK_NDE_EXIT:
			{
				xp_awk_nde_exit_t* px = (xp_awk_nde_exit_t*)p;
				PRINT_TABS (awk, depth);

				if (px->val == XP_NULL) 
				{
					PUT_SRCSTR (awk, XP_T("exit;\n"));
				}
				else 
				{
					PUT_SRCSTR (awk, XP_T("exit "));
					xp_assert (px->val->next == XP_NULL);
					PRINT_EXPRESSION (awk, px->val);
					PUT_SRCSTR (awk, XP_T(";\n"));
				}
				break;
			}

			case XP_AWK_NDE_NEXT:
			{
				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, XP_T("next;\n"));
				break;
			}

			case XP_AWK_NDE_NEXTFILE:
			{
				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, XP_T("nextfile;\n"));
				break;
			}

			case XP_AWK_NDE_DELETE:
			{
				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, XP_T("delete "));
		/* TODO: can't use __print_expression??? */
				xp_awk_prnpt (awk, ((xp_awk_nde_delete_t*)p)->var);
				break;
			}

			case XP_AWK_NDE_PRINT:
			{
				xp_awk_nde_print_t* px = (xp_awk_nde_print_t*)p;

				PRINT_TABS (awk, depth);

				PUT_SRCSTR (awk, XP_T("print"));
				if (px->args != XP_NULL)
				{
					PUT_SRCSTR (awk, XP_T(" "));
					PRINT_EXPRESSION_LIST (awk, px->args);
				}

				if (px->out != XP_NULL)
				{
					PUT_SRCSTR (awk, XP_T(" "));
					PUT_SRCSTR (awk, __print_outop_str[px->out_type]);
					PUT_SRCSTR (awk, XP_T(" "));
					PRINT_EXPRESSION (awk, px->out);
				}

				PUT_SRCSTR (awk, XP_T(";\n"));
				break;
			}

			default:
			{
				PRINT_TABS (awk, depth);
				PRINT_EXPRESSION (awk, p);
				PUT_SRCSTR (awk, XP_T(";\n"));
			}
		}

		p = p->next;
	}

	return 0;
}

int xp_awk_prnpt (xp_awk_t* awk, xp_awk_nde_t* tree)
{
	return __print_statements (awk, tree, 0);
}

int xp_awk_prnptnpt (xp_awk_t* awk, xp_awk_nde_t* tree)
{
	xp_awk_nde_t* nde = tree;

	while (nde != XP_NULL)
	{
		if (__print_expression (awk, nde) == -1) return -1;
		if (nde->next == XP_NULL) break;

		PUT_SRCSTR (awk, XP_T(","));
		nde = nde->next;
	}

	return 0;
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
				xp_free (((xp_awk_nde_rex_t*)p)->code);
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

			case XP_AWK_NDE_AFN:
			{
				xp_awk_nde_call_t* px = (xp_awk_nde_call_t*)p;
				xp_free (px->what.afn.name);
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
