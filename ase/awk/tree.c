/*
 * $Id: tree.c,v 1.83 2006-10-24 04:10:12 bacon Exp $
 */

#include <ase/awk/awk_i.h>

static const ase_char_t* __assop_str[] =
{
	ASE_T("="),
	ASE_T("+="),
	ASE_T("-="),
	ASE_T("*="),
	ASE_T("/="),
	ASE_T("%="),
	ASE_T("**=")
};

static const ase_char_t* __binop_str[] =
{
	ASE_T("||"),
	ASE_T("&&"),
	ASE_T("in"),

	ASE_T("|"),
	ASE_T("^"),
	ASE_T("&"),

	ASE_T("=="),
	ASE_T("!="),
	ASE_T(">"),
	ASE_T(">="),
	ASE_T("<"),
	ASE_T("<="),

	ASE_T("<<"),
	ASE_T(">>"),

	ASE_T("+"),
	ASE_T("-"),
	ASE_T("*"),
	ASE_T("/"),
	ASE_T("%"),
	ASE_T("**"),

	ASE_T(" "),
	ASE_T("~"),
	ASE_T("!~")
};

static const ase_char_t* __unrop_str[] =
{
	ASE_T("+"),
	ASE_T("-"),
	ASE_T("!"),
	ASE_T("~")
};

static const ase_char_t* __incop_str[] =
{
	ASE_T("++"),
	ASE_T("--"),
	ASE_T("++"),
	ASE_T("--")
};

static const ase_char_t* __getline_inop_str[] =
{
	ASE_T("|"),
	ASE_T("|&"),
	ASE_T("<"),
	ASE_T("")
};

static const ase_char_t* __print_outop_str[] =
{
	ASE_T("|"),
	ASE_T("|&"),
	ASE_T(">"),
	ASE_T(">>"),
	ASE_T("")
};

#define PUT_SRCSTR(awk,str) \
	do { if (ase_awk_putsrcstr (awk, str) == -1) return -1; } while (0)

#define PUT_SRCSTRX(awk,str,len) \
	do { if (ase_awk_putsrcstrx (awk, str, len) == -1) return -1; } while (0)

#define PRINT_TABS(awk,depth) \
	do { if (__print_tabs(awk,depth) == -1) return -1; } while (0)

#define PRINT_EXPRESSION(awk,nde) \
	do { if (__print_expression(awk,nde) == -1) return -1; } while (0)

#define PRINT_EXPRESSION_LIST(awk,nde) \
	do { if (__print_expression_list(awk,nde) == -1) return -1; } while (0)

#define PRINT_STATEMENTS(awk,nde,depth) \
	do { if (__print_statements(awk,nde,depth) == -1) return -1; } while (0)

static int __print_tabs (ase_awk_t* awk, int depth);
static int __print_expression (ase_awk_t* awk, ase_awk_nde_t* nde);
static int __print_expression_list (ase_awk_t* awk, ase_awk_nde_t* tree);
static int __print_statements (ase_awk_t* awk, ase_awk_nde_t* tree, int depth);

static int __print_tabs (ase_awk_t* awk, int depth)
{
	while (depth > 0) 
	{
		PUT_SRCSTR (awk, ASE_T("\t"));
		depth--;
	}

	return 0;
}

static int __print_expression (ase_awk_t* awk, ase_awk_nde_t* nde)
{
	switch (nde->type) 
	{
		case ASE_AWK_NDE_GRP:
		{	
			ase_awk_nde_t* p = ((ase_awk_nde_grp_t*)nde)->body;

			PUT_SRCSTR (awk, ASE_T("("));
			while (p != ASE_NULL) 
			{
				PRINT_EXPRESSION (awk, p);
				if (p->next != ASE_NULL) 
					PUT_SRCSTR (awk, ASE_T(","));
				p = p->next;
			}
			PUT_SRCSTR (awk, ASE_T(")"));
			break;
		}

		case ASE_AWK_NDE_ASS:
		{
			ase_awk_nde_ass_t* px = (ase_awk_nde_ass_t*)nde;

			PRINT_EXPRESSION (awk, px->left);
			PUT_SRCSTR (awk, ASE_T(" "));
			PUT_SRCSTR (awk, __assop_str[px->opcode]);
			PUT_SRCSTR (awk, ASE_T(" "));
			PRINT_EXPRESSION (awk, px->right);

			ase_awk_assert (awk, px->right->next == ASE_NULL);
			break;
		}

		case ASE_AWK_NDE_EXP_BIN:
		{
			ase_awk_nde_exp_t* px = (ase_awk_nde_exp_t*)nde;

			PUT_SRCSTR (awk, ASE_T("("));
			PRINT_EXPRESSION (awk, px->left);
			ase_awk_assert (awk, px->left->next == ASE_NULL);

			PUT_SRCSTR (awk, ASE_T(" "));
			PUT_SRCSTR (awk, __binop_str[px->opcode]);
			PUT_SRCSTR (awk, ASE_T(" "));

			if (px->right->type == ASE_AWK_NDE_ASS) 
				PUT_SRCSTR (awk, ASE_T("("));
			PRINT_EXPRESSION (awk, px->right);
			if (px->right->type == ASE_AWK_NDE_ASS) 
				PUT_SRCSTR (awk, ASE_T(")"));
			ase_awk_assert (awk, px->right->next == ASE_NULL); 
			PUT_SRCSTR (awk, ASE_T(")"));
			break;
		}

		case ASE_AWK_NDE_EXP_UNR:
		{
			ase_awk_nde_exp_t* px = (ase_awk_nde_exp_t*)nde;
			ase_awk_assert (awk, px->right == ASE_NULL);

			PUT_SRCSTR (awk, __unrop_str[px->opcode]);
			PUT_SRCSTR (awk, ASE_T("("));
			PRINT_EXPRESSION (awk, px->left);
			PUT_SRCSTR (awk, ASE_T(")"));
			break;
		}

		case ASE_AWK_NDE_EXP_INCPRE:
		{
			ase_awk_nde_exp_t* px = (ase_awk_nde_exp_t*)nde;
			ase_awk_assert (awk, px->right == ASE_NULL);

			PUT_SRCSTR (awk, __incop_str[px->opcode]);
			PUT_SRCSTR (awk, ASE_T("("));
			PRINT_EXPRESSION (awk, px->left);
			PUT_SRCSTR (awk, ASE_T(")"));
			break;
		}

		case ASE_AWK_NDE_EXP_INCPST:
		{
			ase_awk_nde_exp_t* px = (ase_awk_nde_exp_t*)nde;
			ase_awk_assert (awk, px->right == ASE_NULL);

			PUT_SRCSTR (awk, ASE_T("("));
			PRINT_EXPRESSION (awk, px->left);
			PUT_SRCSTR (awk, ASE_T(")"));
			PUT_SRCSTR (awk, __incop_str[px->opcode]);
			break;
		}

		case ASE_AWK_NDE_CND:
		{
			ase_awk_nde_cnd_t* px = (ase_awk_nde_cnd_t*)nde;

			PUT_SRCSTR (awk, ASE_T("("));
			PRINT_EXPRESSION (awk, px->test);
			PUT_SRCSTR (awk, ASE_T(")?"));

			PRINT_EXPRESSION (awk, px->left);
			PUT_SRCSTR (awk, ASE_T(":"));
			PRINT_EXPRESSION (awk, px->right);
			break;
		}

		case ASE_AWK_NDE_INT:
		{
			if (((ase_awk_nde_int_t*)nde)->str == ASE_NULL)
			{
				ase_char_t tmp[ase_sizeof(ase_long_t)*8+2]; 
				ase_size_t n;

				n = ase_awk_longtostr (
					((ase_awk_nde_int_t*)nde)->val,
					10, ASE_NULL, tmp, ase_countof(tmp));

				PUT_SRCSTRX (awk, tmp, n);
			}
			else
			{
				PUT_SRCSTRX (awk,
					((ase_awk_nde_int_t*)nde)->str,
					((ase_awk_nde_int_t*)nde)->len);
			}
			break;
		}

		case ASE_AWK_NDE_REAL:
		{
			if (((ase_awk_nde_real_t*)nde)->str == ASE_NULL)
			{
				ase_char_t tmp[128];
			#if (ASE_SIZEOF_LONG_DOUBLE != 0)
				awk->syscas.sprintf (
					tmp, ase_countof(tmp), ASE_T("%Lf"), 
					(long double)((ase_awk_nde_real_t*)nde)->val);
			#elif (ASE_SIZEOF_DOUBLE != 0)
				awk->syscas.sprintf (
					tmp, ase_countof(tmp), ASE_T("%f"), 
					(double)((ase_awk_nde_real_t*)nde)->val);
			#else
				#error unsupported floating-point data type
			#endif
				PUT_SRCSTR (awk, tmp);
			}
			else
			{
				PUT_SRCSTRX (awk,
					((ase_awk_nde_real_t*)nde)->str,
					((ase_awk_nde_real_t*)nde)->len);
			}
			break;
		}

		case ASE_AWK_NDE_STR:
		{
			/* TODO: ESCAPING */
			PUT_SRCSTR (awk, ASE_T("\""));
			PUT_SRCSTRX (awk,
				((ase_awk_nde_str_t*)nde)->buf, 
				((ase_awk_nde_str_t*)nde)->len);
			PUT_SRCSTR (awk, ASE_T("\""));
			break;
		}

		case ASE_AWK_NDE_REX:
		{
			/* TODO: buf, len */
			PUT_SRCSTR (awk, ASE_T("/"));
			PUT_SRCSTRX (awk,
				((ase_awk_nde_rex_t*)nde)->buf, 
				((ase_awk_nde_rex_t*)nde)->len);
			PUT_SRCSTR (awk, ASE_T("/"));
			break;
		}

		case ASE_AWK_NDE_ARG:
		{
			ase_char_t tmp[ase_sizeof(ase_long_t)*8+2]; 
			ase_size_t n;
			ase_awk_nde_var_t* px = (ase_awk_nde_var_t*)nde;
			ase_awk_assert (awk, px->id.idxa != (ase_size_t)-1);

			n = ase_awk_longtostr (
				px->id.idxa, 10, ASE_NULL, tmp, ase_countof(tmp));

			PUT_SRCSTR (awk, ASE_T("__param"));
			PUT_SRCSTRX (awk, tmp, n);

			ase_awk_assert (awk, px->idx == ASE_NULL);
			break;
		}

		case ASE_AWK_NDE_ARGIDX:
		{
			ase_char_t tmp[ase_sizeof(ase_long_t)*8+2]; 
			ase_size_t n;
			ase_awk_nde_var_t* px = (ase_awk_nde_var_t*)nde;
			ase_awk_assert (awk, px->id.idxa != (ase_size_t)-1);
			ase_awk_assert (awk, px->idx != ASE_NULL);

			PUT_SRCSTR (awk, ASE_T("__param"));
			n = ase_awk_longtostr (
				px->id.idxa, 10, ASE_NULL, tmp, ase_countof(tmp));
			PUT_SRCSTRX (awk, tmp, n);
			PUT_SRCSTR (awk, ASE_T("["));
			PRINT_EXPRESSION_LIST (awk, px->idx);
			PUT_SRCSTR (awk, ASE_T("]"));
			break;
		}

		case ASE_AWK_NDE_NAMED:
		{
			ase_awk_nde_var_t* px = (ase_awk_nde_var_t*)nde;
			ase_awk_assert (awk, px->id.idxa == (ase_size_t)-1);
			ase_awk_assert (awk, px->idx == ASE_NULL);

			PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
			break;
		}

		case ASE_AWK_NDE_NAMEDIDX:
		{
			ase_awk_nde_var_t* px = (ase_awk_nde_var_t*)nde;
			ase_awk_assert (awk, px->id.idxa == (ase_size_t)-1);
			ase_awk_assert (awk, px->idx != ASE_NULL);

			PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
			PUT_SRCSTR (awk, ASE_T("["));
			PRINT_EXPRESSION_LIST (awk, px->idx);
			PUT_SRCSTR (awk, ASE_T("]"));
			break;
		}

		case ASE_AWK_NDE_GLOBAL:
		{
			ase_char_t tmp[ase_sizeof(ase_long_t)*8+2]; 
			ase_size_t n;
			ase_awk_nde_var_t* px = (ase_awk_nde_var_t*)nde;

			if (px->id.idxa != (ase_size_t)-1) 
			{
				PUT_SRCSTR (awk, ASE_T("__global"));
				n = ase_awk_longtostr (
					px->id.idxa, 10, 
					ASE_NULL, tmp, ase_countof(tmp));
				PUT_SRCSTRX (awk, tmp, n);
			}
			else 
			{
				PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
			}
			ase_awk_assert (awk, px->idx == ASE_NULL);
			break;
		}

		case ASE_AWK_NDE_GLOBALIDX:
		{
			ase_char_t tmp[ase_sizeof(ase_long_t)*8+2]; 
			ase_size_t n;
			ase_awk_nde_var_t* px = (ase_awk_nde_var_t*)nde;

			if (px->id.idxa != (ase_size_t)-1) 
			{
				PUT_SRCSTR (awk, ASE_T("__global"));
				n = ase_awk_longtostr (
					px->id.idxa, 10, 
					ASE_NULL, tmp, ase_countof(tmp));
				PUT_SRCSTRX (awk, tmp, n);
				PUT_SRCSTR (awk, ASE_T("["));
			}
			else 
			{
				PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
				PUT_SRCSTR (awk, ASE_T("["));
			}
			ase_awk_assert (awk, px->idx != ASE_NULL);
			PRINT_EXPRESSION_LIST (awk, px->idx);
			PUT_SRCSTR (awk, ASE_T("]"));
			break;
		}

		case ASE_AWK_NDE_LOCAL:
		{
			ase_char_t tmp[ase_sizeof(ase_long_t)*8+2]; 
			ase_size_t n;
			ase_awk_nde_var_t* px = (ase_awk_nde_var_t*)nde;

			if (px->id.idxa != (ase_size_t)-1) 
			{
				PUT_SRCSTR (awk, ASE_T("__local"));
				n = ase_awk_longtostr (
					px->id.idxa, 10, 
					ASE_NULL, tmp, ase_countof(tmp));
				PUT_SRCSTRX (awk, tmp, n);
			}
			else 
			{
				PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
			}
			ase_awk_assert (awk, px->idx == ASE_NULL);
			break;
		}

		case ASE_AWK_NDE_LOCALIDX:
		{
			ase_char_t tmp[ase_sizeof(ase_long_t)*8+2]; 
			ase_size_t n;
			ase_awk_nde_var_t* px = (ase_awk_nde_var_t*)nde;

			if (px->id.idxa != (ase_size_t)-1) 
			{
				PUT_SRCSTR (awk, ASE_T("__local"));
				n = ase_awk_longtostr (
					px->id.idxa, 10, 
					ASE_NULL, tmp, ase_countof(tmp));
				PUT_SRCSTRX (awk, tmp, n);
				PUT_SRCSTR (awk, ASE_T("["));
			}
			else 
			{
				PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
				PUT_SRCSTR (awk, ASE_T("["));
			}
			ase_awk_assert (awk, px->idx != ASE_NULL);
			PRINT_EXPRESSION_LIST (awk, px->idx);
			PUT_SRCSTR (awk, ASE_T("]"));
			break;
		}

		case ASE_AWK_NDE_POS:
		{
			PUT_SRCSTR (awk, ASE_T("$"));
			PRINT_EXPRESSION (awk, ((ase_awk_nde_pos_t*)nde)->val);
			break;
		}

		case ASE_AWK_NDE_BFN:
		{
			ase_awk_nde_call_t* px = (ase_awk_nde_call_t*)nde;
			PUT_SRCSTRX (awk, 
				px->what.bfn.name, px->what.bfn.name_len);
			PUT_SRCSTR (awk, ASE_T(" ("));
			PRINT_EXPRESSION_LIST (awk, px->args);
			PUT_SRCSTR (awk, ASE_T(")"));
			break;
		}

		case ASE_AWK_NDE_AFN:
		{
			/* TODO: use px->what.afn.name_len */
			ase_awk_nde_call_t* px = (ase_awk_nde_call_t*)nde;
			PUT_SRCSTRX (awk, 
				px->what.afn.name, px->what.afn.name_len);
			PUT_SRCSTR (awk, ASE_T(" ("));
			PRINT_EXPRESSION_LIST (awk, px->args);
			PUT_SRCSTR (awk, ASE_T(")"));
			break;
		}

		case ASE_AWK_NDE_GETLINE:
		{
			ase_awk_nde_getline_t* px = (ase_awk_nde_getline_t*)nde;
			if (px->in != ASE_NULL &&
			    (px->in_type == ASE_AWK_IN_PIPE ||
			     px->in_type == ASE_AWK_IN_COPROC))
			{
				PRINT_EXPRESSION (awk, px->in);
				PUT_SRCSTR (awk, ASE_T(" "));
				PUT_SRCSTR (awk, __getline_inop_str[px->in_type]);
				PUT_SRCSTR (awk, ASE_T(" "));
			}

			PUT_SRCSTR (awk, ASE_T("getline"));
			if (px->var != ASE_NULL)
			{
				PUT_SRCSTR (awk, ASE_T(" "));
				PRINT_EXPRESSION (awk, px->var);
			}

			if (px->in != ASE_NULL &&
			    px->in_type == ASE_AWK_IN_FILE)
			{
				PUT_SRCSTR (awk, ASE_T(" "));
				PUT_SRCSTR (awk, __getline_inop_str[px->in_type]);
				PUT_SRCSTR (awk, ASE_T(" "));
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

static int __print_expression_list (ase_awk_t* awk, ase_awk_nde_t* tree)
{
	ase_awk_nde_t* p = tree;

	while (p != ASE_NULL) 
	{
		PRINT_EXPRESSION (awk, p);
		p = p->next;
		if (p != ASE_NULL) PUT_SRCSTR (awk, ASE_T(","));
	}

	return 0;
}

static int __print_statements (ase_awk_t* awk, ase_awk_nde_t* tree, int depth)
{
	ase_awk_nde_t* p = tree;
	ase_size_t i;

	while (p != ASE_NULL) 
	{

		switch (p->type) 
		{
			case ASE_AWK_NDE_NULL:
			{
				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, ASE_T(";\n"));
				break;
			}

			case ASE_AWK_NDE_BLK:
			{
				ase_char_t tmp[ase_sizeof(ase_long_t)*8+2];
				ase_size_t n;
				ase_awk_nde_blk_t* px = (ase_awk_nde_blk_t*)p;

				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, ASE_T("{\n"));

				if (px->nlocals > 0) 
				{
					PRINT_TABS (awk, depth + 1);
					PUT_SRCSTR (awk, ASE_T("local "));

					for (i = 0; i < px->nlocals - 1; i++) 
					{
						PUT_SRCSTR (awk, ASE_T("__local"));
						n = ase_awk_longtostr (
							i, 10, ASE_NULL, tmp, ase_countof(tmp));
						PUT_SRCSTRX (awk, tmp, n);
						PUT_SRCSTR (awk, ASE_T(", "));
					}

					PUT_SRCSTR (awk, ASE_T("__local"));
					n = ase_awk_longtostr (
						i, 10, ASE_NULL, tmp, ase_countof(tmp));
					PUT_SRCSTRX (awk, tmp, n);
					PUT_SRCSTR (awk, ASE_T(";\n"));
				}

				PRINT_STATEMENTS (awk, px->body, depth + 1);	
				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, ASE_T("}\n"));
				break;
			}

			case ASE_AWK_NDE_IF: 
			{
				ase_awk_nde_if_t* px = (ase_awk_nde_if_t*)p;

				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, ASE_T("if ("));	
				PRINT_EXPRESSION (awk, px->test);
				PUT_SRCSTR (awk, ASE_T(")\n"));

				ase_awk_assert (awk, px->then_part != ASE_NULL);
				if (px->then_part->type == ASE_AWK_NDE_BLK)
					PRINT_STATEMENTS (awk, px->then_part, depth);
				else
					PRINT_STATEMENTS (awk, px->then_part, depth + 1);

				if (px->else_part != ASE_NULL) 
				{
					PRINT_TABS (awk, depth);
					PUT_SRCSTR (awk, ASE_T("else\n"));	
					if (px->else_part->type == ASE_AWK_NDE_BLK)
						PRINT_STATEMENTS (awk, px->else_part, depth);
					else
						PRINT_STATEMENTS (awk, px->else_part, depth + 1);
				}
				break;
			}

			case ASE_AWK_NDE_WHILE: 
			{
				ase_awk_nde_while_t* px = (ase_awk_nde_while_t*)p;

				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, ASE_T("while ("));	
				PRINT_EXPRESSION (awk, px->test);
				PUT_SRCSTR (awk, ASE_T(")\n"));
				if (px->body->type == ASE_AWK_NDE_BLK) 
				{
					PRINT_STATEMENTS (awk, px->body, depth);
				}
				else 
				{
					PRINT_STATEMENTS (awk, px->body, depth + 1);
				}
				break;
			}

			case ASE_AWK_NDE_DOWHILE: 
			{
				ase_awk_nde_while_t* px = (ase_awk_nde_while_t*)p;

				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, ASE_T("do\n"));	
				if (px->body->type == ASE_AWK_NDE_BLK) 
				{
					PRINT_STATEMENTS (awk, px->body, depth);
				}
				else 
				{
					PRINT_STATEMENTS (awk, px->body, depth + 1);
				}

				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, ASE_T("while ("));	
				PRINT_EXPRESSION (awk, px->test);
				PUT_SRCSTR (awk, ASE_T(");\n"));	
				break;
			}

			case ASE_AWK_NDE_FOR:
			{
				ase_awk_nde_for_t* px = (ase_awk_nde_for_t*)p;

				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, ASE_T("for ("));
				if (px->init != ASE_NULL) 
				{
					PRINT_EXPRESSION (awk, px->init);
				}
				PUT_SRCSTR (awk, ASE_T("; "));
				if (px->test != ASE_NULL) 
				{
					PRINT_EXPRESSION (awk, px->test);
				}
				PUT_SRCSTR (awk, ASE_T("; "));
				if (px->incr != ASE_NULL) 
				{
					PRINT_EXPRESSION (awk, px->incr);
				}
				PUT_SRCSTR (awk, ASE_T(")\n"));

				if (px->body->type == ASE_AWK_NDE_BLK) 
				{
					PRINT_STATEMENTS (awk, px->body, depth);
				}
				else 
				{
					PRINT_STATEMENTS (awk, px->body, depth + 1);
				}
				break;
			}

			case ASE_AWK_NDE_FOREACH:
			{
				ase_awk_nde_foreach_t* px = (ase_awk_nde_foreach_t*)p;

				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, ASE_T("for "));
				PRINT_EXPRESSION (awk, px->test);
				PUT_SRCSTR (awk, ASE_T("\n"));
				if (px->body->type == ASE_AWK_NDE_BLK) 
				{
					PRINT_STATEMENTS (awk, px->body, depth);
				}
				else 
				{
					PRINT_STATEMENTS (awk, px->body, depth + 1);
				}
				break;
			}

			case ASE_AWK_NDE_BREAK:
			{
				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, ASE_T("break;\n"));
				break;
			}

			case ASE_AWK_NDE_CONTINUE:
			{
				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, ASE_T("continue;\n"));
				break;
			}

			case ASE_AWK_NDE_RETURN:
			{
				PRINT_TABS (awk, depth);
				if (((ase_awk_nde_return_t*)p)->val == ASE_NULL) 
				{
					PUT_SRCSTR (awk, ASE_T("return;\n"));
				}
				else 
				{
					PUT_SRCSTR (awk, ASE_T("return "));
					ase_awk_assert (awk, ((ase_awk_nde_return_t*)p)->val->next == ASE_NULL);

					PRINT_EXPRESSION (awk, ((ase_awk_nde_return_t*)p)->val);
					PUT_SRCSTR (awk, ASE_T(";\n"));
				}
				break;
			}

			case ASE_AWK_NDE_EXIT:
			{
				ase_awk_nde_exit_t* px = (ase_awk_nde_exit_t*)p;
				PRINT_TABS (awk, depth);

				if (px->val == ASE_NULL) 
				{
					PUT_SRCSTR (awk, ASE_T("exit;\n"));
				}
				else 
				{
					PUT_SRCSTR (awk, ASE_T("exit "));
					ase_awk_assert (awk, px->val->next == ASE_NULL);
					PRINT_EXPRESSION (awk, px->val);
					PUT_SRCSTR (awk, ASE_T(";\n"));
				}
				break;
			}

			case ASE_AWK_NDE_NEXT:
			{
				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, ASE_T("next;\n"));
				break;
			}

			case ASE_AWK_NDE_NEXTFILE:
			{
				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, ASE_T("nextfile;\n"));
				break;
			}

			case ASE_AWK_NDE_DELETE:
			{
				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, ASE_T("delete "));
		/* TODO: can't use __print_expression??? */
				ase_awk_prnpt (awk, ((ase_awk_nde_delete_t*)p)->var);
				break;
			}

			case ASE_AWK_NDE_PRINT:
			{
				ase_awk_nde_print_t* px = (ase_awk_nde_print_t*)p;

				PRINT_TABS (awk, depth);

				PUT_SRCSTR (awk, ASE_T("print"));
				if (px->args != ASE_NULL)
				{
					PUT_SRCSTR (awk, ASE_T(" "));
					PRINT_EXPRESSION_LIST (awk, px->args);
				}

				if (px->out != ASE_NULL)
				{
					PUT_SRCSTR (awk, ASE_T(" "));
					PUT_SRCSTR (awk, __print_outop_str[px->out_type]);
					PUT_SRCSTR (awk, ASE_T(" "));
					PRINT_EXPRESSION (awk, px->out);
				}

				PUT_SRCSTR (awk, ASE_T(";\n"));
				break;
			}

			default:
			{
				PRINT_TABS (awk, depth);
				PRINT_EXPRESSION (awk, p);
				PUT_SRCSTR (awk, ASE_T(";\n"));
			}
		}

		p = p->next;
	}

	return 0;
}

int ase_awk_prnpt (ase_awk_t* awk, ase_awk_nde_t* tree)
{
	return __print_statements (awk, tree, 0);
}

int ase_awk_prnptnpt (ase_awk_t* awk, ase_awk_nde_t* tree)
{
	ase_awk_nde_t* nde = tree;

	while (nde != ASE_NULL)
	{
		if (__print_expression (awk, nde) == -1) return -1;
		if (nde->next == ASE_NULL) break;

		PUT_SRCSTR (awk, ASE_T(","));
		nde = nde->next;
	}

	return 0;
}

void ase_awk_clrpt (ase_awk_t* awk, ase_awk_nde_t* tree)
{
	ase_awk_nde_t* p = tree;
	ase_awk_nde_t* next;

	while (p != ASE_NULL) 
	{
		next = p->next;

		switch (p->type) 
		{
			case ASE_AWK_NDE_NULL:
			{
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_BLK:
			{
				ase_awk_clrpt (awk, ((ase_awk_nde_blk_t*)p)->body);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_IF:
			{
				ase_awk_nde_if_t* px = (ase_awk_nde_if_t*)p;
				ase_awk_clrpt (awk, px->test);
				ase_awk_clrpt (awk, px->then_part);

				if (px->else_part != ASE_NULL)
					ase_awk_clrpt (awk, px->else_part);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_WHILE:
			case ASE_AWK_NDE_DOWHILE:
			{
				ase_awk_clrpt (awk, ((ase_awk_nde_while_t*)p)->test);
				ase_awk_clrpt (awk, ((ase_awk_nde_while_t*)p)->body);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_FOR:
			{
				ase_awk_nde_for_t* px = (ase_awk_nde_for_t*)p;

				if (px->init != ASE_NULL)
					ase_awk_clrpt (awk, px->init);
				if (px->test != ASE_NULL)
					ase_awk_clrpt (awk, px->test);
				if (px->incr != ASE_NULL)
					ase_awk_clrpt (awk, px->incr);
				ase_awk_clrpt (awk, px->body);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_FOREACH:
			{
				ase_awk_clrpt (awk, ((ase_awk_nde_foreach_t*)p)->test);
				ase_awk_clrpt (awk, ((ase_awk_nde_foreach_t*)p)->body);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_BREAK:
			{
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_CONTINUE:
			{
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_RETURN:
			{
				ase_awk_nde_return_t* px =
					(ase_awk_nde_return_t*)p;
				if (px->val != ASE_NULL) 
					ase_awk_clrpt (awk, px->val);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_EXIT:
			{
				if (((ase_awk_nde_exit_t*)p)->val != ASE_NULL) 
					ase_awk_clrpt (awk, ((ase_awk_nde_exit_t*)p)->val);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_NEXT:
			case ASE_AWK_NDE_NEXTFILE:
			{
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_DELETE:
			{
				ase_awk_clrpt (awk, ((ase_awk_nde_delete_t*)p)->var);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_PRINT:
			{
				ase_awk_nde_print_t* px = 
					(ase_awk_nde_print_t*)p;
				if (px->args != ASE_NULL) 
					ase_awk_clrpt (awk, px->args);
				if (px->out != ASE_NULL) 
					ase_awk_clrpt (awk, px->out);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_GRP:
			{
				ase_awk_clrpt (awk, ((ase_awk_nde_grp_t*)p)->body);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_ASS:
			{
				ase_awk_clrpt (awk, ((ase_awk_nde_ass_t*)p)->left);
				ase_awk_clrpt (awk, ((ase_awk_nde_ass_t*)p)->right);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_EXP_BIN:
			{
				ase_awk_nde_exp_t* px = (ase_awk_nde_exp_t*)p;
				ase_awk_assert (awk, px->left->next == ASE_NULL);
				ase_awk_assert (awk, px->right->next == ASE_NULL);

				ase_awk_clrpt (awk, px->left);
				ase_awk_clrpt (awk, px->right);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_EXP_UNR:
			case ASE_AWK_NDE_EXP_INCPRE:
			case ASE_AWK_NDE_EXP_INCPST:
			{
				ase_awk_nde_exp_t* px = (ase_awk_nde_exp_t*)p;
				ase_awk_assert (awk, px->right == ASE_NULL);
				ase_awk_clrpt (awk, px->left);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_CND:
			{
				ase_awk_clrpt (awk, ((ase_awk_nde_cnd_t*)p)->test);
				ase_awk_clrpt (awk, ((ase_awk_nde_cnd_t*)p)->left);
				ase_awk_clrpt (awk, ((ase_awk_nde_cnd_t*)p)->right);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_INT:
			{
				if (((ase_awk_nde_int_t*)p)->str != ASE_NULL)
					ASE_AWK_FREE (awk, ((ase_awk_nde_int_t*)p)->str);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_REAL:
			{
				if (((ase_awk_nde_real_t*)p)->str != ASE_NULL)
					ASE_AWK_FREE (awk, ((ase_awk_nde_real_t*)p)->str);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_STR:
			{
				ASE_AWK_FREE (awk, ((ase_awk_nde_str_t*)p)->buf);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_REX:
			{
				ASE_AWK_FREE (awk, ((ase_awk_nde_rex_t*)p)->buf);
				ASE_AWK_FREE (awk, ((ase_awk_nde_rex_t*)p)->code);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_NAMED:
			case ASE_AWK_NDE_GLOBAL:
			case ASE_AWK_NDE_LOCAL:
			case ASE_AWK_NDE_ARG:
			{
				ase_awk_nde_var_t* px = (ase_awk_nde_var_t*)p;
				ase_awk_assert (awk, px->idx == ASE_NULL);
				if (px->id.name != ASE_NULL)
					ASE_AWK_FREE (awk, px->id.name);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_NAMEDIDX:
			case ASE_AWK_NDE_GLOBALIDX:
			case ASE_AWK_NDE_LOCALIDX:
			case ASE_AWK_NDE_ARGIDX:
			{
				ase_awk_nde_var_t* px = (ase_awk_nde_var_t*)p;
				ase_awk_assert (awk, px->idx != ASE_NULL);
				ase_awk_clrpt (awk, px->idx);
				if (px->id.name != ASE_NULL)
					ASE_AWK_FREE (awk, px->id.name);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_POS:
			{
				ase_awk_clrpt (awk, ((ase_awk_nde_pos_t*)p)->val);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_BFN:
			{
				ase_awk_nde_call_t* px = (ase_awk_nde_call_t*)p;
				/* ASE_AWK_FREE (awk, px->what.bfn); */
				ase_awk_clrpt (awk, px->args);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_AFN:
			{
				ase_awk_nde_call_t* px = (ase_awk_nde_call_t*)p;
				ASE_AWK_FREE (awk, px->what.afn.name);
				ase_awk_clrpt (awk, px->args);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_GETLINE:
			{
				ase_awk_nde_getline_t* px = 
					(ase_awk_nde_getline_t*)p;
				if (px->var != ASE_NULL) 
					ase_awk_clrpt (awk, px->var);
				if (px->in != ASE_NULL) 
					ase_awk_clrpt (awk, px->in);
				ASE_AWK_FREE (awk, p);
				break;
			}

			default:
			{
				ase_awk_assert (awk, !"should never happen - invalid node type");
				ASE_AWK_FREE (awk, p);
				break;
			}
		}

		p = next;
	}
}
