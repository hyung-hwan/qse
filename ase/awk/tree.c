/*
 * $Id: tree.c,v 1.82 2006-10-22 12:39:30 bacon Exp $
 */

#include <sse/awk/awk_i.h>

static const sse_char_t* __assop_str[] =
{
	SSE_T("="),
	SSE_T("+="),
	SSE_T("-="),
	SSE_T("*="),
	SSE_T("/="),
	SSE_T("%="),
	SSE_T("**=")
};

static const sse_char_t* __binop_str[] =
{
	SSE_T("||"),
	SSE_T("&&"),
	SSE_T("in"),

	SSE_T("|"),
	SSE_T("^"),
	SSE_T("&"),

	SSE_T("=="),
	SSE_T("!="),
	SSE_T(">"),
	SSE_T(">="),
	SSE_T("<"),
	SSE_T("<="),

	SSE_T("<<"),
	SSE_T(">>"),

	SSE_T("+"),
	SSE_T("-"),
	SSE_T("*"),
	SSE_T("/"),
	SSE_T("%"),
	SSE_T("**"),

	SSE_T(" "),
	SSE_T("~"),
	SSE_T("!~")
};

static const sse_char_t* __unrop_str[] =
{
	SSE_T("+"),
	SSE_T("-"),
	SSE_T("!"),
	SSE_T("~")
};

static const sse_char_t* __incop_str[] =
{
	SSE_T("++"),
	SSE_T("--"),
	SSE_T("++"),
	SSE_T("--")
};

static const sse_char_t* __getline_inop_str[] =
{
	SSE_T("|"),
	SSE_T("|&"),
	SSE_T("<"),
	SSE_T("")
};

static const sse_char_t* __print_outop_str[] =
{
	SSE_T("|"),
	SSE_T("|&"),
	SSE_T(">"),
	SSE_T(">>"),
	SSE_T("")
};

#define PUT_SRCSTR(awk,str) \
	do { if (sse_awk_putsrcstr (awk, str) == -1) return -1; } while (0)

#define PUT_SRCSTRX(awk,str,len) \
	do { if (sse_awk_putsrcstrx (awk, str, len) == -1) return -1; } while (0)

#define PRINT_TABS(awk,depth) \
	do { if (__print_tabs(awk,depth) == -1) return -1; } while (0)

#define PRINT_EXPRESSION(awk,nde) \
	do { if (__print_expression(awk,nde) == -1) return -1; } while (0)

#define PRINT_EXPRESSION_LIST(awk,nde) \
	do { if (__print_expression_list(awk,nde) == -1) return -1; } while (0)

#define PRINT_STATEMENTS(awk,nde,depth) \
	do { if (__print_statements(awk,nde,depth) == -1) return -1; } while (0)

static int __print_tabs (sse_awk_t* awk, int depth);
static int __print_expression (sse_awk_t* awk, sse_awk_nde_t* nde);
static int __print_expression_list (sse_awk_t* awk, sse_awk_nde_t* tree);
static int __print_statements (sse_awk_t* awk, sse_awk_nde_t* tree, int depth);

static int __print_tabs (sse_awk_t* awk, int depth)
{
	while (depth > 0) 
	{
		PUT_SRCSTR (awk, SSE_T("\t"));
		depth--;
	}

	return 0;
}

static int __print_expression (sse_awk_t* awk, sse_awk_nde_t* nde)
{
	switch (nde->type) 
	{
		case SSE_AWK_NDE_GRP:
		{	
			sse_awk_nde_t* p = ((sse_awk_nde_grp_t*)nde)->body;

			PUT_SRCSTR (awk, SSE_T("("));
			while (p != SSE_NULL) 
			{
				PRINT_EXPRESSION (awk, p);
				if (p->next != SSE_NULL) 
					PUT_SRCSTR (awk, SSE_T(","));
				p = p->next;
			}
			PUT_SRCSTR (awk, SSE_T(")"));
			break;
		}

		case SSE_AWK_NDE_ASS:
		{
			sse_awk_nde_ass_t* px = (sse_awk_nde_ass_t*)nde;

			PRINT_EXPRESSION (awk, px->left);
			PUT_SRCSTR (awk, SSE_T(" "));
			PUT_SRCSTR (awk, __assop_str[px->opcode]);
			PUT_SRCSTR (awk, SSE_T(" "));
			PRINT_EXPRESSION (awk, px->right);

			sse_awk_assert (awk, px->right->next == SSE_NULL);
			break;
		}

		case SSE_AWK_NDE_EXP_BIN:
		{
			sse_awk_nde_exp_t* px = (sse_awk_nde_exp_t*)nde;

			PUT_SRCSTR (awk, SSE_T("("));
			PRINT_EXPRESSION (awk, px->left);
			sse_awk_assert (awk, px->left->next == SSE_NULL);

			PUT_SRCSTR (awk, SSE_T(" "));
			PUT_SRCSTR (awk, __binop_str[px->opcode]);
			PUT_SRCSTR (awk, SSE_T(" "));

			if (px->right->type == SSE_AWK_NDE_ASS) 
				PUT_SRCSTR (awk, SSE_T("("));
			PRINT_EXPRESSION (awk, px->right);
			if (px->right->type == SSE_AWK_NDE_ASS) 
				PUT_SRCSTR (awk, SSE_T(")"));
			sse_awk_assert (awk, px->right->next == SSE_NULL); 
			PUT_SRCSTR (awk, SSE_T(")"));
			break;
		}

		case SSE_AWK_NDE_EXP_UNR:
		{
			sse_awk_nde_exp_t* px = (sse_awk_nde_exp_t*)nde;
			sse_awk_assert (awk, px->right == SSE_NULL);

			PUT_SRCSTR (awk, __unrop_str[px->opcode]);
			PUT_SRCSTR (awk, SSE_T("("));
			PRINT_EXPRESSION (awk, px->left);
			PUT_SRCSTR (awk, SSE_T(")"));
			break;
		}

		case SSE_AWK_NDE_EXP_INCPRE:
		{
			sse_awk_nde_exp_t* px = (sse_awk_nde_exp_t*)nde;
			sse_awk_assert (awk, px->right == SSE_NULL);

			PUT_SRCSTR (awk, __incop_str[px->opcode]);
			PUT_SRCSTR (awk, SSE_T("("));
			PRINT_EXPRESSION (awk, px->left);
			PUT_SRCSTR (awk, SSE_T(")"));
			break;
		}

		case SSE_AWK_NDE_EXP_INCPST:
		{
			sse_awk_nde_exp_t* px = (sse_awk_nde_exp_t*)nde;
			sse_awk_assert (awk, px->right == SSE_NULL);

			PUT_SRCSTR (awk, SSE_T("("));
			PRINT_EXPRESSION (awk, px->left);
			PUT_SRCSTR (awk, SSE_T(")"));
			PUT_SRCSTR (awk, __incop_str[px->opcode]);
			break;
		}

		case SSE_AWK_NDE_CND:
		{
			sse_awk_nde_cnd_t* px = (sse_awk_nde_cnd_t*)nde;

			PUT_SRCSTR (awk, SSE_T("("));
			PRINT_EXPRESSION (awk, px->test);
			PUT_SRCSTR (awk, SSE_T(")?"));

			PRINT_EXPRESSION (awk, px->left);
			PUT_SRCSTR (awk, SSE_T(":"));
			PRINT_EXPRESSION (awk, px->right);
			break;
		}

		case SSE_AWK_NDE_INT:
		{
			if (((sse_awk_nde_int_t*)nde)->str == SSE_NULL)
			{
				sse_char_t tmp[sse_sizeof(sse_long_t)*8+2]; 
				sse_size_t n;

				n = sse_awk_longtostr (
					((sse_awk_nde_int_t*)nde)->val,
					10, SSE_NULL, tmp, sse_countof(tmp));

				PUT_SRCSTRX (awk, tmp, n);
			}
			else
			{
				PUT_SRCSTRX (awk,
					((sse_awk_nde_int_t*)nde)->str,
					((sse_awk_nde_int_t*)nde)->len);
			}
			break;
		}

		case SSE_AWK_NDE_REAL:
		{
			if (((sse_awk_nde_real_t*)nde)->str == SSE_NULL)
			{
				sse_char_t tmp[128];
			#if (SSE_SIZEOF_LONG_DOUBLE != 0)
				awk->syscas.sprintf (
					tmp, sse_countof(tmp), SSE_T("%Lf"), 
					(long double)((sse_awk_nde_real_t*)nde)->val);
			#elif (SSE_SIZEOF_DOUBLE != 0)
				awk->syscas.sprintf (
					tmp, sse_countof(tmp), SSE_T("%f"), 
					(double)((sse_awk_nde_real_t*)nde)->val);
			#else
				#error unsupported floating-point data type
			#endif
				PUT_SRCSTR (awk, tmp);
			}
			else
			{
				PUT_SRCSTRX (awk,
					((sse_awk_nde_real_t*)nde)->str,
					((sse_awk_nde_real_t*)nde)->len);
			}
			break;
		}

		case SSE_AWK_NDE_STR:
		{
			/* TODO: ESCAPING */
			PUT_SRCSTR (awk, SSE_T("\""));
			PUT_SRCSTRX (awk,
				((sse_awk_nde_str_t*)nde)->buf, 
				((sse_awk_nde_str_t*)nde)->len);
			PUT_SRCSTR (awk, SSE_T("\""));
			break;
		}

		case SSE_AWK_NDE_REX:
		{
			/* TODO: buf, len */
			PUT_SRCSTR (awk, SSE_T("/"));
			PUT_SRCSTRX (awk,
				((sse_awk_nde_rex_t*)nde)->buf, 
				((sse_awk_nde_rex_t*)nde)->len);
			PUT_SRCSTR (awk, SSE_T("/"));
			break;
		}

		case SSE_AWK_NDE_ARG:
		{
			sse_char_t tmp[sse_sizeof(sse_long_t)*8+2]; 
			sse_size_t n;
			sse_awk_nde_var_t* px = (sse_awk_nde_var_t*)nde;
			sse_awk_assert (awk, px->id.idxa != (sse_size_t)-1);

			n = sse_awk_longtostr (
				px->id.idxa, 10, SSE_NULL, tmp, sse_countof(tmp));

			PUT_SRCSTR (awk, SSE_T("__param"));
			PUT_SRCSTRX (awk, tmp, n);

			sse_awk_assert (awk, px->idx == SSE_NULL);
			break;
		}

		case SSE_AWK_NDE_ARGIDX:
		{
			sse_char_t tmp[sse_sizeof(sse_long_t)*8+2]; 
			sse_size_t n;
			sse_awk_nde_var_t* px = (sse_awk_nde_var_t*)nde;
			sse_awk_assert (awk, px->id.idxa != (sse_size_t)-1);
			sse_awk_assert (awk, px->idx != SSE_NULL);

			PUT_SRCSTR (awk, SSE_T("__param"));
			n = sse_awk_longtostr (
				px->id.idxa, 10, SSE_NULL, tmp, sse_countof(tmp));
			PUT_SRCSTRX (awk, tmp, n);
			PUT_SRCSTR (awk, SSE_T("["));
			PRINT_EXPRESSION_LIST (awk, px->idx);
			PUT_SRCSTR (awk, SSE_T("]"));
			break;
		}

		case SSE_AWK_NDE_NAMED:
		{
			sse_awk_nde_var_t* px = (sse_awk_nde_var_t*)nde;
			sse_awk_assert (awk, px->id.idxa == (sse_size_t)-1);
			sse_awk_assert (awk, px->idx == SSE_NULL);

			PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
			break;
		}

		case SSE_AWK_NDE_NAMEDIDX:
		{
			sse_awk_nde_var_t* px = (sse_awk_nde_var_t*)nde;
			sse_awk_assert (awk, px->id.idxa == (sse_size_t)-1);
			sse_awk_assert (awk, px->idx != SSE_NULL);

			PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
			PUT_SRCSTR (awk, SSE_T("["));
			PRINT_EXPRESSION_LIST (awk, px->idx);
			PUT_SRCSTR (awk, SSE_T("]"));
			break;
		}

		case SSE_AWK_NDE_GLOBAL:
		{
			sse_char_t tmp[sse_sizeof(sse_long_t)*8+2]; 
			sse_size_t n;
			sse_awk_nde_var_t* px = (sse_awk_nde_var_t*)nde;

			if (px->id.idxa != (sse_size_t)-1) 
			{
				PUT_SRCSTR (awk, SSE_T("__global"));
				n = sse_awk_longtostr (
					px->id.idxa, 10, 
					SSE_NULL, tmp, sse_countof(tmp));
				PUT_SRCSTRX (awk, tmp, n);
			}
			else 
			{
				PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
			}
			sse_awk_assert (awk, px->idx == SSE_NULL);
			break;
		}

		case SSE_AWK_NDE_GLOBALIDX:
		{
			sse_char_t tmp[sse_sizeof(sse_long_t)*8+2]; 
			sse_size_t n;
			sse_awk_nde_var_t* px = (sse_awk_nde_var_t*)nde;

			if (px->id.idxa != (sse_size_t)-1) 
			{
				PUT_SRCSTR (awk, SSE_T("__global"));
				n = sse_awk_longtostr (
					px->id.idxa, 10, 
					SSE_NULL, tmp, sse_countof(tmp));
				PUT_SRCSTRX (awk, tmp, n);
				PUT_SRCSTR (awk, SSE_T("["));
			}
			else 
			{
				PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
				PUT_SRCSTR (awk, SSE_T("["));
			}
			sse_awk_assert (awk, px->idx != SSE_NULL);
			PRINT_EXPRESSION_LIST (awk, px->idx);
			PUT_SRCSTR (awk, SSE_T("]"));
			break;
		}

		case SSE_AWK_NDE_LOCAL:
		{
			sse_char_t tmp[sse_sizeof(sse_long_t)*8+2]; 
			sse_size_t n;
			sse_awk_nde_var_t* px = (sse_awk_nde_var_t*)nde;

			if (px->id.idxa != (sse_size_t)-1) 
			{
				PUT_SRCSTR (awk, SSE_T("__local"));
				n = sse_awk_longtostr (
					px->id.idxa, 10, 
					SSE_NULL, tmp, sse_countof(tmp));
				PUT_SRCSTRX (awk, tmp, n);
			}
			else 
			{
				PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
			}
			sse_awk_assert (awk, px->idx == SSE_NULL);
			break;
		}

		case SSE_AWK_NDE_LOCALIDX:
		{
			sse_char_t tmp[sse_sizeof(sse_long_t)*8+2]; 
			sse_size_t n;
			sse_awk_nde_var_t* px = (sse_awk_nde_var_t*)nde;

			if (px->id.idxa != (sse_size_t)-1) 
			{
				PUT_SRCSTR (awk, SSE_T("__local"));
				n = sse_awk_longtostr (
					px->id.idxa, 10, 
					SSE_NULL, tmp, sse_countof(tmp));
				PUT_SRCSTRX (awk, tmp, n);
				PUT_SRCSTR (awk, SSE_T("["));
			}
			else 
			{
				PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
				PUT_SRCSTR (awk, SSE_T("["));
			}
			sse_awk_assert (awk, px->idx != SSE_NULL);
			PRINT_EXPRESSION_LIST (awk, px->idx);
			PUT_SRCSTR (awk, SSE_T("]"));
			break;
		}

		case SSE_AWK_NDE_POS:
		{
			PUT_SRCSTR (awk, SSE_T("$"));
			PRINT_EXPRESSION (awk, ((sse_awk_nde_pos_t*)nde)->val);
			break;
		}

		case SSE_AWK_NDE_BFN:
		{
			sse_awk_nde_call_t* px = (sse_awk_nde_call_t*)nde;
			PUT_SRCSTRX (awk, 
				px->what.bfn.name, px->what.bfn.name_len);
			PUT_SRCSTR (awk, SSE_T(" ("));
			PRINT_EXPRESSION_LIST (awk, px->args);
			PUT_SRCSTR (awk, SSE_T(")"));
			break;
		}

		case SSE_AWK_NDE_AFN:
		{
			/* TODO: use px->what.afn.name_len */
			sse_awk_nde_call_t* px = (sse_awk_nde_call_t*)nde;
			PUT_SRCSTRX (awk, 
				px->what.afn.name, px->what.afn.name_len);
			PUT_SRCSTR (awk, SSE_T(" ("));
			PRINT_EXPRESSION_LIST (awk, px->args);
			PUT_SRCSTR (awk, SSE_T(")"));
			break;
		}

		case SSE_AWK_NDE_GETLINE:
		{
			sse_awk_nde_getline_t* px = (sse_awk_nde_getline_t*)nde;
			if (px->in != SSE_NULL &&
			    (px->in_type == SSE_AWK_IN_PIPE ||
			     px->in_type == SSE_AWK_IN_COPROC))
			{
				PRINT_EXPRESSION (awk, px->in);
				PUT_SRCSTR (awk, SSE_T(" "));
				PUT_SRCSTR (awk, __getline_inop_str[px->in_type]);
				PUT_SRCSTR (awk, SSE_T(" "));
			}

			PUT_SRCSTR (awk, SSE_T("getline"));
			if (px->var != SSE_NULL)
			{
				PUT_SRCSTR (awk, SSE_T(" "));
				PRINT_EXPRESSION (awk, px->var);
			}

			if (px->in != SSE_NULL &&
			    px->in_type == SSE_AWK_IN_FILE)
			{
				PUT_SRCSTR (awk, SSE_T(" "));
				PUT_SRCSTR (awk, __getline_inop_str[px->in_type]);
				PUT_SRCSTR (awk, SSE_T(" "));
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

static int __print_expression_list (sse_awk_t* awk, sse_awk_nde_t* tree)
{
	sse_awk_nde_t* p = tree;

	while (p != SSE_NULL) 
	{
		PRINT_EXPRESSION (awk, p);
		p = p->next;
		if (p != SSE_NULL) PUT_SRCSTR (awk, SSE_T(","));
	}

	return 0;
}

static int __print_statements (sse_awk_t* awk, sse_awk_nde_t* tree, int depth)
{
	sse_awk_nde_t* p = tree;
	sse_size_t i;

	while (p != SSE_NULL) 
	{

		switch (p->type) 
		{
			case SSE_AWK_NDE_NULL:
			{
				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, SSE_T(";\n"));
				break;
			}

			case SSE_AWK_NDE_BLK:
			{
				sse_char_t tmp[sse_sizeof(sse_long_t)*8+2];
				sse_size_t n;
				sse_awk_nde_blk_t* px = (sse_awk_nde_blk_t*)p;

				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, SSE_T("{\n"));

				if (px->nlocals > 0) 
				{
					PRINT_TABS (awk, depth + 1);
					PUT_SRCSTR (awk, SSE_T("local "));

					for (i = 0; i < px->nlocals - 1; i++) 
					{
						PUT_SRCSTR (awk, SSE_T("__local"));
						n = sse_awk_longtostr (
							i, 10, SSE_NULL, tmp, sse_countof(tmp));
						PUT_SRCSTRX (awk, tmp, n);
						PUT_SRCSTR (awk, SSE_T(", "));
					}

					PUT_SRCSTR (awk, SSE_T("__local"));
					n = sse_awk_longtostr (
						i, 10, SSE_NULL, tmp, sse_countof(tmp));
					PUT_SRCSTRX (awk, tmp, n);
					PUT_SRCSTR (awk, SSE_T(";\n"));
				}

				PRINT_STATEMENTS (awk, px->body, depth + 1);	
				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, SSE_T("}\n"));
				break;
			}

			case SSE_AWK_NDE_IF: 
			{
				sse_awk_nde_if_t* px = (sse_awk_nde_if_t*)p;

				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, SSE_T("if ("));	
				PRINT_EXPRESSION (awk, px->test);
				PUT_SRCSTR (awk, SSE_T(")\n"));

				sse_awk_assert (awk, px->then_part != SSE_NULL);
				if (px->then_part->type == SSE_AWK_NDE_BLK)
					PRINT_STATEMENTS (awk, px->then_part, depth);
				else
					PRINT_STATEMENTS (awk, px->then_part, depth + 1);

				if (px->else_part != SSE_NULL) 
				{
					PRINT_TABS (awk, depth);
					PUT_SRCSTR (awk, SSE_T("else\n"));	
					if (px->else_part->type == SSE_AWK_NDE_BLK)
						PRINT_STATEMENTS (awk, px->else_part, depth);
					else
						PRINT_STATEMENTS (awk, px->else_part, depth + 1);
				}
				break;
			}

			case SSE_AWK_NDE_WHILE: 
			{
				sse_awk_nde_while_t* px = (sse_awk_nde_while_t*)p;

				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, SSE_T("while ("));	
				PRINT_EXPRESSION (awk, px->test);
				PUT_SRCSTR (awk, SSE_T(")\n"));
				if (px->body->type == SSE_AWK_NDE_BLK) 
				{
					PRINT_STATEMENTS (awk, px->body, depth);
				}
				else 
				{
					PRINT_STATEMENTS (awk, px->body, depth + 1);
				}
				break;
			}

			case SSE_AWK_NDE_DOWHILE: 
			{
				sse_awk_nde_while_t* px = (sse_awk_nde_while_t*)p;

				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, SSE_T("do\n"));	
				if (px->body->type == SSE_AWK_NDE_BLK) 
				{
					PRINT_STATEMENTS (awk, px->body, depth);
				}
				else 
				{
					PRINT_STATEMENTS (awk, px->body, depth + 1);
				}

				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, SSE_T("while ("));	
				PRINT_EXPRESSION (awk, px->test);
				PUT_SRCSTR (awk, SSE_T(");\n"));	
				break;
			}

			case SSE_AWK_NDE_FOR:
			{
				sse_awk_nde_for_t* px = (sse_awk_nde_for_t*)p;

				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, SSE_T("for ("));
				if (px->init != SSE_NULL) 
				{
					PRINT_EXPRESSION (awk, px->init);
				}
				PUT_SRCSTR (awk, SSE_T("; "));
				if (px->test != SSE_NULL) 
				{
					PRINT_EXPRESSION (awk, px->test);
				}
				PUT_SRCSTR (awk, SSE_T("; "));
				if (px->incr != SSE_NULL) 
				{
					PRINT_EXPRESSION (awk, px->incr);
				}
				PUT_SRCSTR (awk, SSE_T(")\n"));

				if (px->body->type == SSE_AWK_NDE_BLK) 
				{
					PRINT_STATEMENTS (awk, px->body, depth);
				}
				else 
				{
					PRINT_STATEMENTS (awk, px->body, depth + 1);
				}
				break;
			}

			case SSE_AWK_NDE_FOREACH:
			{
				sse_awk_nde_foreach_t* px = (sse_awk_nde_foreach_t*)p;

				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, SSE_T("for "));
				PRINT_EXPRESSION (awk, px->test);
				PUT_SRCSTR (awk, SSE_T("\n"));
				if (px->body->type == SSE_AWK_NDE_BLK) 
				{
					PRINT_STATEMENTS (awk, px->body, depth);
				}
				else 
				{
					PRINT_STATEMENTS (awk, px->body, depth + 1);
				}
				break;
			}

			case SSE_AWK_NDE_BREAK:
			{
				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, SSE_T("break;\n"));
				break;
			}

			case SSE_AWK_NDE_CONTINUE:
			{
				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, SSE_T("continue;\n"));
				break;
			}

			case SSE_AWK_NDE_RETURN:
			{
				PRINT_TABS (awk, depth);
				if (((sse_awk_nde_return_t*)p)->val == SSE_NULL) 
				{
					PUT_SRCSTR (awk, SSE_T("return;\n"));
				}
				else 
				{
					PUT_SRCSTR (awk, SSE_T("return "));
					sse_awk_assert (awk, ((sse_awk_nde_return_t*)p)->val->next == SSE_NULL);

					PRINT_EXPRESSION (awk, ((sse_awk_nde_return_t*)p)->val);
					PUT_SRCSTR (awk, SSE_T(";\n"));
				}
				break;
			}

			case SSE_AWK_NDE_EXIT:
			{
				sse_awk_nde_exit_t* px = (sse_awk_nde_exit_t*)p;
				PRINT_TABS (awk, depth);

				if (px->val == SSE_NULL) 
				{
					PUT_SRCSTR (awk, SSE_T("exit;\n"));
				}
				else 
				{
					PUT_SRCSTR (awk, SSE_T("exit "));
					sse_awk_assert (awk, px->val->next == SSE_NULL);
					PRINT_EXPRESSION (awk, px->val);
					PUT_SRCSTR (awk, SSE_T(";\n"));
				}
				break;
			}

			case SSE_AWK_NDE_NEXT:
			{
				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, SSE_T("next;\n"));
				break;
			}

			case SSE_AWK_NDE_NEXTFILE:
			{
				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, SSE_T("nextfile;\n"));
				break;
			}

			case SSE_AWK_NDE_DELETE:
			{
				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, SSE_T("delete "));
		/* TODO: can't use __print_expression??? */
				sse_awk_prnpt (awk, ((sse_awk_nde_delete_t*)p)->var);
				break;
			}

			case SSE_AWK_NDE_PRINT:
			{
				sse_awk_nde_print_t* px = (sse_awk_nde_print_t*)p;

				PRINT_TABS (awk, depth);

				PUT_SRCSTR (awk, SSE_T("print"));
				if (px->args != SSE_NULL)
				{
					PUT_SRCSTR (awk, SSE_T(" "));
					PRINT_EXPRESSION_LIST (awk, px->args);
				}

				if (px->out != SSE_NULL)
				{
					PUT_SRCSTR (awk, SSE_T(" "));
					PUT_SRCSTR (awk, __print_outop_str[px->out_type]);
					PUT_SRCSTR (awk, SSE_T(" "));
					PRINT_EXPRESSION (awk, px->out);
				}

				PUT_SRCSTR (awk, SSE_T(";\n"));
				break;
			}

			default:
			{
				PRINT_TABS (awk, depth);
				PRINT_EXPRESSION (awk, p);
				PUT_SRCSTR (awk, SSE_T(";\n"));
			}
		}

		p = p->next;
	}

	return 0;
}

int sse_awk_prnpt (sse_awk_t* awk, sse_awk_nde_t* tree)
{
	return __print_statements (awk, tree, 0);
}

int sse_awk_prnptnpt (sse_awk_t* awk, sse_awk_nde_t* tree)
{
	sse_awk_nde_t* nde = tree;

	while (nde != SSE_NULL)
	{
		if (__print_expression (awk, nde) == -1) return -1;
		if (nde->next == SSE_NULL) break;

		PUT_SRCSTR (awk, SSE_T(","));
		nde = nde->next;
	}

	return 0;
}

void sse_awk_clrpt (sse_awk_t* awk, sse_awk_nde_t* tree)
{
	sse_awk_nde_t* p = tree;
	sse_awk_nde_t* next;

	while (p != SSE_NULL) 
	{
		next = p->next;

		switch (p->type) 
		{
			case SSE_AWK_NDE_NULL:
			{
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_BLK:
			{
				sse_awk_clrpt (awk, ((sse_awk_nde_blk_t*)p)->body);
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_IF:
			{
				sse_awk_nde_if_t* px = (sse_awk_nde_if_t*)p;
				sse_awk_clrpt (awk, px->test);
				sse_awk_clrpt (awk, px->then_part);

				if (px->else_part != SSE_NULL)
					sse_awk_clrpt (awk, px->else_part);
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_WHILE:
			case SSE_AWK_NDE_DOWHILE:
			{
				sse_awk_clrpt (awk, ((sse_awk_nde_while_t*)p)->test);
				sse_awk_clrpt (awk, ((sse_awk_nde_while_t*)p)->body);
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_FOR:
			{
				sse_awk_nde_for_t* px = (sse_awk_nde_for_t*)p;

				if (px->init != SSE_NULL)
					sse_awk_clrpt (awk, px->init);
				if (px->test != SSE_NULL)
					sse_awk_clrpt (awk, px->test);
				if (px->incr != SSE_NULL)
					sse_awk_clrpt (awk, px->incr);
				sse_awk_clrpt (awk, px->body);
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_FOREACH:
			{
				sse_awk_clrpt (awk, ((sse_awk_nde_foreach_t*)p)->test);
				sse_awk_clrpt (awk, ((sse_awk_nde_foreach_t*)p)->body);
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_BREAK:
			{
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_CONTINUE:
			{
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_RETURN:
			{
				sse_awk_nde_return_t* px =
					(sse_awk_nde_return_t*)p;
				if (px->val != SSE_NULL) 
					sse_awk_clrpt (awk, px->val);
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_EXIT:
			{
				if (((sse_awk_nde_exit_t*)p)->val != SSE_NULL) 
					sse_awk_clrpt (awk, ((sse_awk_nde_exit_t*)p)->val);
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_NEXT:
			case SSE_AWK_NDE_NEXTFILE:
			{
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_DELETE:
			{
				sse_awk_clrpt (awk, ((sse_awk_nde_delete_t*)p)->var);
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_PRINT:
			{
				sse_awk_nde_print_t* px = 
					(sse_awk_nde_print_t*)p;
				if (px->args != SSE_NULL) 
					sse_awk_clrpt (awk, px->args);
				if (px->out != SSE_NULL) 
					sse_awk_clrpt (awk, px->out);
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_GRP:
			{
				sse_awk_clrpt (awk, ((sse_awk_nde_grp_t*)p)->body);
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_ASS:
			{
				sse_awk_clrpt (awk, ((sse_awk_nde_ass_t*)p)->left);
				sse_awk_clrpt (awk, ((sse_awk_nde_ass_t*)p)->right);
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_EXP_BIN:
			{
				sse_awk_nde_exp_t* px = (sse_awk_nde_exp_t*)p;
				sse_awk_assert (awk, px->left->next == SSE_NULL);
				sse_awk_assert (awk, px->right->next == SSE_NULL);

				sse_awk_clrpt (awk, px->left);
				sse_awk_clrpt (awk, px->right);
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_EXP_UNR:
			case SSE_AWK_NDE_EXP_INCPRE:
			case SSE_AWK_NDE_EXP_INCPST:
			{
				sse_awk_nde_exp_t* px = (sse_awk_nde_exp_t*)p;
				sse_awk_assert (awk, px->right == SSE_NULL);
				sse_awk_clrpt (awk, px->left);
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_CND:
			{
				sse_awk_clrpt (awk, ((sse_awk_nde_cnd_t*)p)->test);
				sse_awk_clrpt (awk, ((sse_awk_nde_cnd_t*)p)->left);
				sse_awk_clrpt (awk, ((sse_awk_nde_cnd_t*)p)->right);
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_INT:
			{
				if (((sse_awk_nde_int_t*)p)->str != SSE_NULL)
					SSE_AWK_FREE (awk, ((sse_awk_nde_int_t*)p)->str);
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_REAL:
			{
				if (((sse_awk_nde_real_t*)p)->str != SSE_NULL)
					SSE_AWK_FREE (awk, ((sse_awk_nde_real_t*)p)->str);
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_STR:
			{
				SSE_AWK_FREE (awk, ((sse_awk_nde_str_t*)p)->buf);
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_REX:
			{
				SSE_AWK_FREE (awk, ((sse_awk_nde_rex_t*)p)->buf);
				SSE_AWK_FREE (awk, ((sse_awk_nde_rex_t*)p)->code);
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_NAMED:
			case SSE_AWK_NDE_GLOBAL:
			case SSE_AWK_NDE_LOCAL:
			case SSE_AWK_NDE_ARG:
			{
				sse_awk_nde_var_t* px = (sse_awk_nde_var_t*)p;
				sse_awk_assert (awk, px->idx == SSE_NULL);
				if (px->id.name != SSE_NULL)
					SSE_AWK_FREE (awk, px->id.name);
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_NAMEDIDX:
			case SSE_AWK_NDE_GLOBALIDX:
			case SSE_AWK_NDE_LOCALIDX:
			case SSE_AWK_NDE_ARGIDX:
			{
				sse_awk_nde_var_t* px = (sse_awk_nde_var_t*)p;
				sse_awk_assert (awk, px->idx != SSE_NULL);
				sse_awk_clrpt (awk, px->idx);
				if (px->id.name != SSE_NULL)
					SSE_AWK_FREE (awk, px->id.name);
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_POS:
			{
				sse_awk_clrpt (awk, ((sse_awk_nde_pos_t*)p)->val);
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_BFN:
			{
				sse_awk_nde_call_t* px = (sse_awk_nde_call_t*)p;
				/* SSE_AWK_FREE (awk, px->what.bfn); */
				sse_awk_clrpt (awk, px->args);
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_AFN:
			{
				sse_awk_nde_call_t* px = (sse_awk_nde_call_t*)p;
				SSE_AWK_FREE (awk, px->what.afn.name);
				sse_awk_clrpt (awk, px->args);
				SSE_AWK_FREE (awk, p);
				break;
			}

			case SSE_AWK_NDE_GETLINE:
			{
				sse_awk_nde_getline_t* px = 
					(sse_awk_nde_getline_t*)p;
				if (px->var != SSE_NULL) 
					sse_awk_clrpt (awk, px->var);
				if (px->in != SSE_NULL) 
					sse_awk_clrpt (awk, px->in);
				SSE_AWK_FREE (awk, p);
				break;
			}

			default:
			{
				sse_awk_assert (awk, !"should never happen - invalid node type");
				SSE_AWK_FREE (awk, p);
				break;
			}
		}

		p = next;
	}
}
