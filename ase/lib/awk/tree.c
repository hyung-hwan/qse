/*
 * $Id: tree.c 192 2008-06-06 10:33:44Z baconevi $
 *
 * {License}
 */

#include "awk_i.h"

static const ase_char_t* assop_str[] =
{
	ASE_T("="),
	ASE_T("+="),
	ASE_T("-="),
	ASE_T("*="),
	ASE_T("/="),
	ASE_T("//="),
	ASE_T("%="),
	ASE_T("**="),
	ASE_T(">>="),
	ASE_T("<<="),
	ASE_T("&="),
	ASE_T("^="),
	ASE_T("|=")
};

static const ase_char_t* binop_str[][2] =
{
	{ ASE_T("||"), ASE_T("||") },
	{ ASE_T("&&"), ASE_T("&&") },
	{ ASE_T("in"), ASE_T("in") },

	{ ASE_T("|"),  ASE_T("|") },
	{ ASE_T("^"),  ASE_T("^") },
	{ ASE_T("&"),  ASE_T("&") },

	{ ASE_T("=="), ASE_T("==") },
	{ ASE_T("!="), ASE_T("!=") },
	{ ASE_T(">"),  ASE_T(">") },
	{ ASE_T(">="), ASE_T(">=") },
	{ ASE_T("<"),  ASE_T("<") },
	{ ASE_T("<="), ASE_T("<=") },

	{ ASE_T("<<"), ASE_T("<<") },
	{ ASE_T(">>"), ASE_T(">>") },

	{ ASE_T("+"),  ASE_T("+") },
	{ ASE_T("-"),  ASE_T("-") },
	{ ASE_T("*"),  ASE_T("*") },
	{ ASE_T("/"),  ASE_T("/") },
	{ ASE_T("//"), ASE_T("//") },
	{ ASE_T("%"),  ASE_T("%") },
	{ ASE_T("**"), ASE_T("**") },

	{ ASE_T(" "),  ASE_T(".") },
	{ ASE_T("~"),  ASE_T("~") },
	{ ASE_T("!~"), ASE_T("!~") }
};

static const ase_char_t* unrop_str[] =
{
	ASE_T("+"),
	ASE_T("-"),
	ASE_T("!"),
	ASE_T("~")
};

static const ase_char_t* incop_str[] =
{
	ASE_T("++"),
	ASE_T("--"),
	ASE_T("++"),
	ASE_T("--")
};

static const ase_char_t* getline_inop_str[] =
{
	ASE_T("|"),
	ASE_T("|&"),
	ASE_T("<"),
	ASE_T("")
};

static const ase_char_t* print_outop_str[] =
{
	ASE_T("|"),
	ASE_T("|&"),
	ASE_T(">"),
	ASE_T(">>"),
	ASE_T("")
};

#define PUT_SRCSTR(awk,str) \
	do { if (ase_awk_putsrcstr (awk, str) == -1) return -1; } while (0)

#define PUT_NEWLINE(awk) \
	do { \
		if (awk->option & ASE_AWK_CRLF) PUT_SRCSTR (awk, ASE_T("\r")); \
		PUT_SRCSTR (awk, ASE_T("\n")); \
	} while (0)

#define PUT_SRCSTRX(awk,str,len) \
	do { if (ase_awk_putsrcstrx (awk, str, len) == -1) return -1; } while (0)

#define PRINT_TABS(awk,depth) \
	do { if (print_tabs(awk,depth) == -1) return -1; } while (0)

#define PRINT_EXPRESSION(awk,nde) \
	do { if (print_expression(awk,nde) == -1) return -1; } while (0)

#define PRINT_EXPRESSION_LIST(awk,nde) \
	do { if (print_expression_list(awk,nde) == -1) return -1; } while (0)

#define PRINT_STATEMENTS(awk,nde,depth) \
	do { if (print_statements(awk,nde,depth) == -1) return -1; } while (0)

static int print_tabs (ase_awk_t* awk, int depth);
static int print_expression (ase_awk_t* awk, ase_awk_nde_t* nde);
static int print_expression_list (ase_awk_t* awk, ase_awk_nde_t* tree);
static int print_statements (ase_awk_t* awk, ase_awk_nde_t* tree, int depth);

static int print_tabs (ase_awk_t* awk, int depth)
{
	while (depth > 0) 
	{
		PUT_SRCSTR (awk, ASE_T("\t"));
		depth--;
	}

	return 0;
}

static int print_expression (ase_awk_t* awk, ase_awk_nde_t* nde)
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
			PUT_SRCSTR (awk, assop_str[px->opcode]);
			PUT_SRCSTR (awk, ASE_T(" "));
			PRINT_EXPRESSION (awk, px->right);

			ASE_ASSERT (px->right->next == ASE_NULL);
			break;
		}

		case ASE_AWK_NDE_EXP_BIN:
		{
			ase_awk_nde_exp_t* px = (ase_awk_nde_exp_t*)nde;

			PUT_SRCSTR (awk, ASE_T("("));
			PRINT_EXPRESSION (awk, px->left);
			ASE_ASSERT (px->left->next == ASE_NULL);

			PUT_SRCSTR (awk, ASE_T(" "));
			PUT_SRCSTR (awk, binop_str[px->opcode][(awk->option & ASE_AWK_IMPLICIT)? 0: 1]);
			PUT_SRCSTR (awk, ASE_T(" "));

			if (px->right->type == ASE_AWK_NDE_ASS) 
				PUT_SRCSTR (awk, ASE_T("("));
			PRINT_EXPRESSION (awk, px->right);
			if (px->right->type == ASE_AWK_NDE_ASS) 
				PUT_SRCSTR (awk, ASE_T(")"));
			ASE_ASSERT (px->right->next == ASE_NULL); 
			PUT_SRCSTR (awk, ASE_T(")"));
			break;
		}

		case ASE_AWK_NDE_EXP_UNR:
		{
			ase_awk_nde_exp_t* px = (ase_awk_nde_exp_t*)nde;
			ASE_ASSERT (px->right == ASE_NULL);

			PUT_SRCSTR (awk, ASE_T("("));
			PUT_SRCSTR (awk, unrop_str[px->opcode]);
			PUT_SRCSTR (awk, ASE_T("("));
			PRINT_EXPRESSION (awk, px->left);
			PUT_SRCSTR (awk, ASE_T(")"));
			PUT_SRCSTR (awk, ASE_T(")"));
			break;
		}

		case ASE_AWK_NDE_EXP_INCPRE:
		{
			ase_awk_nde_exp_t* px = (ase_awk_nde_exp_t*)nde;
			ASE_ASSERT (px->right == ASE_NULL);

			PUT_SRCSTR (awk, incop_str[px->opcode]);
			PUT_SRCSTR (awk, ASE_T("("));
			PRINT_EXPRESSION (awk, px->left);
			PUT_SRCSTR (awk, ASE_T(")"));
			break;
		}

		case ASE_AWK_NDE_EXP_INCPST:
		{
			ase_awk_nde_exp_t* px = (ase_awk_nde_exp_t*)nde;
			ASE_ASSERT (px->right == ASE_NULL);

			PUT_SRCSTR (awk, ASE_T("("));
			PRINT_EXPRESSION (awk, px->left);
			PUT_SRCSTR (awk, ASE_T(")"));
			PUT_SRCSTR (awk, incop_str[px->opcode]);
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
				ase_size_t n;

				n = ase_awk_longtostr (
					((ase_awk_nde_int_t*)nde)->val, 10, ASE_NULL, 
					awk->tmp.fmt, ASE_COUNTOF(awk->tmp.fmt));

				PUT_SRCSTRX (awk, awk->tmp.fmt, n);
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
			#if (ASE_SIZEOF_LONG_DOUBLE != 0)
				awk->prmfns.misc.sprintf (
					awk->prmfns.misc.custom_data,
					awk->tmp.fmt, ASE_COUNTOF(awk->tmp.fmt), ASE_T("%Lf"), 
					(long double)((ase_awk_nde_real_t*)nde)->val);
			#elif (ASE_SIZEOF_DOUBLE != 0)
				awk->prmfns.misc.sprintf (
					awk->prmfns.misc.custom_data,
					awk->tmp.fmt, ASE_COUNTOF(awk->tmp.fmt), ASE_T("%f"), 
					(double)((ase_awk_nde_real_t*)nde)->val);
			#else
				#error unsupported floating-point data type
			#endif
				PUT_SRCSTR (awk, awk->tmp.fmt);
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
			ase_char_t* ptr;
			ase_size_t len, i;

			PUT_SRCSTR (awk, ASE_T("\""));

			ptr = ((ase_awk_nde_str_t*)nde)->buf;
			len = ((ase_awk_nde_str_t*)nde)->len;
			for (i = 0; i < len; i++)
			{
				/* TODO: maybe more de-escaping?? */
				if (ptr[i] == ASE_T('\n'))
					PUT_SRCSTR (awk, ASE_T("\\n"));
				else if (ptr[i] == ASE_T('\r'))
					PUT_SRCSTR (awk, ASE_T("\\r"));
				else if (ptr[i] == ASE_T('\f'))
					PUT_SRCSTR (awk, ASE_T("\\f"));
				else if (ptr[i] == ASE_T('\b'))
					PUT_SRCSTR (awk, ASE_T("\\b"));
				else if (ptr[i] == ASE_T('\v'))
					PUT_SRCSTR (awk, ASE_T("\\v"));
				else if (ptr[i] == ASE_T('\a'))
					PUT_SRCSTR (awk, ASE_T("\\a"));
				else if (ptr[i] == ASE_T('\0'))
					PUT_SRCSTR (awk, ASE_T("\\0"));
				else
					PUT_SRCSTRX (awk, &ptr[i], 1);
			}
			PUT_SRCSTR (awk, ASE_T("\""));
			break;
		}

		case ASE_AWK_NDE_REX:
		{
			PUT_SRCSTR (awk, ASE_T("/"));
			PUT_SRCSTRX (awk,
				((ase_awk_nde_rex_t*)nde)->buf, 
				((ase_awk_nde_rex_t*)nde)->len);
			PUT_SRCSTR (awk, ASE_T("/"));
			break;
		}

		case ASE_AWK_NDE_ARG:
		{
			ase_char_t tmp[ASE_SIZEOF(ase_long_t)*8+2]; 
			ase_size_t n;
			ase_awk_nde_var_t* px = (ase_awk_nde_var_t*)nde;
			ASE_ASSERT (px->id.idxa != (ase_size_t)-1);

			n = ase_awk_longtostr (
				px->id.idxa, 10, ASE_NULL, tmp, ASE_COUNTOF(tmp));

			PUT_SRCSTR (awk, ASE_T("__p"));
			PUT_SRCSTRX (awk, tmp, n);

			ASE_ASSERT (px->idx == ASE_NULL);
			break;
		}

		case ASE_AWK_NDE_ARGIDX:
		{
			ase_size_t n;
			ase_awk_nde_var_t* px = (ase_awk_nde_var_t*)nde;
			ASE_ASSERT (px->id.idxa != (ase_size_t)-1);
			ASE_ASSERT (px->idx != ASE_NULL);

			PUT_SRCSTR (awk, ASE_T("__p"));
			n = ase_awk_longtostr (
				px->id.idxa, 10, ASE_NULL,
				awk->tmp.fmt, ASE_COUNTOF(awk->tmp.fmt));
			PUT_SRCSTRX (awk, awk->tmp.fmt, n);
			PUT_SRCSTR (awk, ASE_T("["));
			PRINT_EXPRESSION_LIST (awk, px->idx);
			PUT_SRCSTR (awk, ASE_T("]"));
			break;
		}

		case ASE_AWK_NDE_NAMED:
		{
			ase_awk_nde_var_t* px = (ase_awk_nde_var_t*)nde;
			ASE_ASSERT (px->id.idxa == (ase_size_t)-1);
			ASE_ASSERT (px->idx == ASE_NULL);

			PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
			break;
		}

		case ASE_AWK_NDE_NAMEDIDX:
		{
			ase_awk_nde_var_t* px = (ase_awk_nde_var_t*)nde;
			ASE_ASSERT (px->id.idxa == (ase_size_t)-1);
			ASE_ASSERT (px->idx != ASE_NULL);

			PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
			PUT_SRCSTR (awk, ASE_T("["));
			PRINT_EXPRESSION_LIST (awk, px->idx);
			PUT_SRCSTR (awk, ASE_T("]"));
			break;
		}

		case ASE_AWK_NDE_GLOBAL:
		{
			ase_awk_nde_var_t* px = (ase_awk_nde_var_t*)nde;

			if (px->id.idxa != (ase_size_t)-1) 
			{

				if ((awk->option & ASE_AWK_EXPLICIT) && 
				    !(awk->option & ASE_AWK_IMPLICIT))
				{
					/* no implicit(named) variable is allowed.
					 * use the actual name */
					PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
				}
				else if (px->id.idxa < awk->tree.nbglobals)
				{
					/* static global variables */
					PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
				}
				else
				{
					ase_char_t tmp[ASE_SIZEOF(ase_long_t)*8+2]; 
					ase_size_t n;

					PUT_SRCSTR (awk, ASE_T("__g"));
					n = ase_awk_longtostr (
						px->id.idxa, 10, 
						ASE_NULL, tmp, ASE_COUNTOF(tmp));
					PUT_SRCSTRX (awk, tmp, n);
				}
			}
			else 
			{
				PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
			}
			ASE_ASSERT (px->idx == ASE_NULL);
			break;
		}

		case ASE_AWK_NDE_GLOBALIDX:
		{
			ase_awk_nde_var_t* px = (ase_awk_nde_var_t*)nde;

			if (px->id.idxa != (ase_size_t)-1) 
			{
				if ((awk->option & ASE_AWK_EXPLICIT) && 
				    !(awk->option & ASE_AWK_IMPLICIT))
				{
					/* no implicit(named) variable is allowed.
					 * use the actual name */
					PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
				}
				else if (px->id.idxa < awk->tree.nbglobals)
				{
					/* static global variables */
					PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
				}
				else
				{
					ase_char_t tmp[ASE_SIZEOF(ase_long_t)*8+2]; 
					ase_size_t n;

					PUT_SRCSTR (awk, ASE_T("__g"));
					n = ase_awk_longtostr (
						px->id.idxa, 10, 
						ASE_NULL, tmp, ASE_COUNTOF(tmp));
					PUT_SRCSTRX (awk, tmp, n);
				}
				PUT_SRCSTR (awk, ASE_T("["));
			}
			else 
			{
				PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
				PUT_SRCSTR (awk, ASE_T("["));
			}
			ASE_ASSERT (px->idx != ASE_NULL);
			PRINT_EXPRESSION_LIST (awk, px->idx);
			PUT_SRCSTR (awk, ASE_T("]"));
			break;
		}

		case ASE_AWK_NDE_LOCAL:
		{
			ase_size_t n;
			ase_awk_nde_var_t* px = (ase_awk_nde_var_t*)nde;

			if (px->id.idxa != (ase_size_t)-1) 
			{
				PUT_SRCSTR (awk, ASE_T("__l"));
				n = ase_awk_longtostr (
					px->id.idxa, 10, ASE_NULL, 
					awk->tmp.fmt, ASE_COUNTOF(awk->tmp.fmt));
				PUT_SRCSTRX (awk, awk->tmp.fmt, n);
			}
			else 
			{
				PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
			}
			ASE_ASSERT (px->idx == ASE_NULL);
			break;
		}

		case ASE_AWK_NDE_LOCALIDX:
		{
			ase_size_t n;
			ase_awk_nde_var_t* px = (ase_awk_nde_var_t*)nde;

			if (px->id.idxa != (ase_size_t)-1) 
			{
				PUT_SRCSTR (awk, ASE_T("__l"));
				n = ase_awk_longtostr (
					px->id.idxa, 10, ASE_NULL, 
					awk->tmp.fmt, ASE_COUNTOF(awk->tmp.fmt));
				PUT_SRCSTRX (awk, awk->tmp.fmt, n);
				PUT_SRCSTR (awk, ASE_T("["));
			}
			else 
			{
				PUT_SRCSTRX (awk, px->id.name, px->id.name_len);
				PUT_SRCSTR (awk, ASE_T("["));
			}
			ASE_ASSERT (px->idx != ASE_NULL);
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
				px->what.bfn.name.ptr, px->what.bfn.name.len);
			PUT_SRCSTR (awk, ASE_T(" ("));
			PRINT_EXPRESSION_LIST (awk, px->args);
			PUT_SRCSTR (awk, ASE_T(")"));
			break;
		}

		case ASE_AWK_NDE_AFN:
		{
			ase_awk_nde_call_t* px = (ase_awk_nde_call_t*)nde;
			PUT_SRCSTRX (awk, 
				px->what.afn.name.ptr, px->what.afn.name.len);
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
				PUT_SRCSTR (awk, getline_inop_str[px->in_type]);
				PUT_SRCSTR (awk, ASE_T(" "));
			}

			PUT_SRCSTR (awk, ase_awk_getkw(awk,ASE_T("getline")));
			if (px->var != ASE_NULL)
			{
				PUT_SRCSTR (awk, ASE_T(" "));
				PRINT_EXPRESSION (awk, px->var);
			}

			if (px->in != ASE_NULL &&
			    px->in_type == ASE_AWK_IN_FILE)
			{
				PUT_SRCSTR (awk, ASE_T(" "));
				PUT_SRCSTR (awk, getline_inop_str[px->in_type]);
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

static int print_expression_list (ase_awk_t* awk, ase_awk_nde_t* tree)
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

static int print_statement (ase_awk_t* awk, ase_awk_nde_t* p, int depth)
{
	ase_size_t i;

	switch (p->type) 
	{
		case ASE_AWK_NDE_NULL:
		{
			PRINT_TABS (awk, depth);
			PUT_SRCSTR (awk, ASE_T(";"));
			PUT_NEWLINE (awk);
			break;
		}

		case ASE_AWK_NDE_BLK:
		{
			ase_size_t n;
			ase_awk_nde_blk_t* px = (ase_awk_nde_blk_t*)p;

			PRINT_TABS (awk, depth);
			PUT_SRCSTR (awk, ASE_T("{"));
			PUT_NEWLINE (awk);

			if (px->nlocals > 0) 
			{
				PRINT_TABS (awk, depth + 1);
				PUT_SRCSTR (awk, ase_awk_getkw(awk,ASE_T("local")));
				PUT_SRCSTR (awk, ASE_T(" "));

				for (i = 0; i < px->nlocals - 1; i++) 
				{
					PUT_SRCSTR (awk, ASE_T("__l"));
					n = ase_awk_longtostr (
						i, 10, ASE_NULL, 
						awk->tmp.fmt, ASE_COUNTOF(awk->tmp.fmt));
					PUT_SRCSTRX (awk, awk->tmp.fmt, n);
					PUT_SRCSTR (awk, ASE_T(", "));
				}

				PUT_SRCSTR (awk, ASE_T("__l"));
				n = ase_awk_longtostr (
					i, 10, ASE_NULL, 
					awk->tmp.fmt, ASE_COUNTOF(awk->tmp.fmt));
				PUT_SRCSTRX (awk, awk->tmp.fmt, n);
				PUT_SRCSTR (awk, ASE_T(";"));
				PUT_NEWLINE (awk);
			}

			PRINT_STATEMENTS (awk, px->body, depth + 1);	
			PRINT_TABS (awk, depth);
			PUT_SRCSTR (awk, ASE_T("}"));
			PUT_NEWLINE (awk);
			break;
		}

		case ASE_AWK_NDE_IF: 
		{
			ase_awk_nde_if_t* px = (ase_awk_nde_if_t*)p;

			PRINT_TABS (awk, depth);
			PUT_SRCSTR (awk, ase_awk_getkw(awk,ASE_T("if")));
			PUT_SRCSTR (awk, ASE_T(" ("));	
			PRINT_EXPRESSION (awk, px->test);
			PUT_SRCSTR (awk, ASE_T(")"));
			PUT_NEWLINE (awk);

			ASE_ASSERT (px->then_part != ASE_NULL);
			if (px->then_part->type == ASE_AWK_NDE_BLK)
				PRINT_STATEMENTS (awk, px->then_part, depth);
			else
				PRINT_STATEMENTS (awk, px->then_part, depth + 1);

			if (px->else_part != ASE_NULL) 
			{
				PRINT_TABS (awk, depth);
				PUT_SRCSTR (awk, ase_awk_getkw(awk,ASE_T("else")));
				PUT_NEWLINE (awk);
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
			PUT_SRCSTR (awk, ase_awk_getkw(awk,ASE_T("while")));
			PUT_SRCSTR (awk, ASE_T(" ("));	
			PRINT_EXPRESSION (awk, px->test);
			PUT_SRCSTR (awk, ASE_T(")"));
			PUT_NEWLINE (awk);
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
			PUT_SRCSTR (awk, ase_awk_getkw(awk,ASE_T("do")));
			PUT_NEWLINE (awk);
			if (px->body->type == ASE_AWK_NDE_BLK) 
			{
				PRINT_STATEMENTS (awk, px->body, depth);
			}
			else 
			{
				PRINT_STATEMENTS (awk, px->body, depth + 1);
			}

			PRINT_TABS (awk, depth);
			PUT_SRCSTR (awk, ase_awk_getkw(awk,ASE_T("while")));
			PUT_SRCSTR (awk, ASE_T(" ("));	
			PRINT_EXPRESSION (awk, px->test);
			PUT_SRCSTR (awk, ASE_T(");"));
			PUT_NEWLINE (awk);
			break;
		}

		case ASE_AWK_NDE_FOR:
		{
			ase_awk_nde_for_t* px = (ase_awk_nde_for_t*)p;

			PRINT_TABS (awk, depth);
			PUT_SRCSTR (awk, ase_awk_getkw(awk,ASE_T("for")));
			PUT_SRCSTR (awk, ASE_T(" ("));
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
			PUT_SRCSTR (awk, ASE_T(")"));
			PUT_NEWLINE (awk);

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
			PUT_SRCSTR (awk, ase_awk_getkw(awk,ASE_T("for")));
			PUT_SRCSTR (awk, ASE_T(" "));
			PRINT_EXPRESSION (awk, px->test);
			PUT_NEWLINE (awk);
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
			PUT_SRCSTR (awk, ase_awk_getkw(awk,ASE_T("break")));
			PUT_SRCSTR (awk, ASE_T(";"));
			PUT_NEWLINE (awk);
			break;
		}

		case ASE_AWK_NDE_CONTINUE:
		{
			PRINT_TABS (awk, depth);
			PUT_SRCSTR (awk, ase_awk_getkw(awk,ASE_T("continue")));
			PUT_SRCSTR (awk, ASE_T(";"));
			PUT_NEWLINE (awk);
			break;
		}

		case ASE_AWK_NDE_RETURN:
		{
			PRINT_TABS (awk, depth);
			if (((ase_awk_nde_return_t*)p)->val == ASE_NULL) 
			{
				PUT_SRCSTR (awk, ase_awk_getkw(awk,ASE_T("return")));
				PUT_SRCSTR (awk, ASE_T(";"));
				PUT_NEWLINE (awk);
			}
			else 
			{
				PUT_SRCSTR (awk, ase_awk_getkw(awk,ASE_T("return")));
				PUT_SRCSTR (awk, ASE_T(" "));
				ASE_ASSERT (((ase_awk_nde_return_t*)p)->val->next == ASE_NULL);

				PRINT_EXPRESSION (awk, ((ase_awk_nde_return_t*)p)->val);
				PUT_SRCSTR (awk, ASE_T(";"));
				PUT_NEWLINE (awk);
			}
			break;
		}

		case ASE_AWK_NDE_EXIT:
		{
			ase_awk_nde_exit_t* px = (ase_awk_nde_exit_t*)p;
			PRINT_TABS (awk, depth);

			if (px->val == ASE_NULL) 
			{
				PUT_SRCSTR (awk, ase_awk_getkw(awk,ASE_T("exit")));
				PUT_SRCSTR (awk, ASE_T(";"));
				PUT_NEWLINE (awk);
			}
			else 
			{
				PUT_SRCSTR (awk, ase_awk_getkw(awk,ASE_T("exit")));
				PUT_SRCSTR (awk, ASE_T(" "));
				ASE_ASSERT (px->val->next == ASE_NULL);
				PRINT_EXPRESSION (awk, px->val);
				PUT_SRCSTR (awk, ASE_T(";"));
				PUT_NEWLINE (awk);
			}
			break;
		}

		case ASE_AWK_NDE_NEXT:
		{
			PRINT_TABS (awk, depth);
			PUT_SRCSTR (awk, ase_awk_getkw(awk,ASE_T("next")));
			PUT_SRCSTR (awk, ASE_T(";"));
			PUT_NEWLINE (awk);
			break;
		}

		case ASE_AWK_NDE_NEXTFILE:
		{
			PRINT_TABS (awk, depth);
			if (((ase_awk_nde_nextfile_t*)p)->out)
			{
				PUT_SRCSTR (awk, ase_awk_getkw(awk,ASE_T("nextofile")));
			}
			else
			{
				PUT_SRCSTR (awk, ase_awk_getkw(awk,ASE_T("nextfile")));
			}
			PUT_SRCSTR (awk, ASE_T(";"));
			PUT_NEWLINE (awk);
			break;
		}

		case ASE_AWK_NDE_DELETE:
		{
			PRINT_TABS (awk, depth);
			PUT_SRCSTR (awk, ase_awk_getkw(awk,ASE_T("delete")));
			PUT_SRCSTR (awk, ASE_T(" "));
			ase_awk_prnpt (awk, ((ase_awk_nde_delete_t*)p)->var);
			break;
		}

		case ASE_AWK_NDE_RESET:
		{
			PRINT_TABS (awk, depth);
			PUT_SRCSTR (awk, ase_awk_getkw(awk,ASE_T("reset")));
			PUT_SRCSTR (awk, ASE_T(" "));
			ase_awk_prnpt (awk, ((ase_awk_nde_reset_t*)p)->var);
			break;
		}

		case ASE_AWK_NDE_PRINT:
		case ASE_AWK_NDE_PRINTF:
		{
			ase_awk_nde_print_t* px = (ase_awk_nde_print_t*)p;

			PRINT_TABS (awk, depth);

			if (p->type == ASE_AWK_NDE_PRINT) 
			{
				PUT_SRCSTR (awk, ase_awk_getkw(awk,ASE_T("print")));
			}
			else
			{
				PUT_SRCSTR (awk, ase_awk_getkw(awk,ASE_T("printf")));
			}

			if (px->args != ASE_NULL)
			{
				PUT_SRCSTR (awk, ASE_T(" "));
				PRINT_EXPRESSION_LIST (awk, px->args);
			}

			if (px->out != ASE_NULL)
			{
				PUT_SRCSTR (awk, ASE_T(" "));
				PUT_SRCSTR (awk, print_outop_str[px->out_type]);
				PUT_SRCSTR (awk, ASE_T(" "));
				PRINT_EXPRESSION (awk, px->out);
			}

			PUT_SRCSTR (awk, ASE_T(";"));
			PUT_NEWLINE (awk);
			break;
		}

		default:
		{
			PRINT_TABS (awk, depth);
			PRINT_EXPRESSION (awk, p);
			PUT_SRCSTR (awk, ASE_T(";"));
			PUT_NEWLINE (awk);
		}
	}

	return 0;
}

static int print_statements (ase_awk_t* awk, ase_awk_nde_t* tree, int depth)
{
	ase_awk_nde_t* p = tree;

	while (p != ASE_NULL) 
	{
		if (print_statement (awk, p, depth) == -1) return -1;
		p = p->next;
	}

	return 0;
}

int ase_awk_prnpt (ase_awk_t* awk, ase_awk_nde_t* tree)
{
	return print_statements (awk, tree, 0);
}

int ase_awk_prnnde (ase_awk_t* awk, ase_awk_nde_t* tree)
{
	return print_statement (awk, tree, 0);
}

int ase_awk_prnptnpt (ase_awk_t* awk, ase_awk_nde_t* tree)
{
	ase_awk_nde_t* nde = tree;

	while (nde != ASE_NULL)
	{
		if (print_expression (awk, nde) == -1) return -1;
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

			case ASE_AWK_NDE_RESET:
			{
				ase_awk_clrpt (awk, ((ase_awk_nde_reset_t*)p)->var);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_PRINT:
			case ASE_AWK_NDE_PRINTF:
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
				ASE_ASSERT (px->left->next == ASE_NULL);
				ASE_ASSERT (px->right->next == ASE_NULL);

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
				ASE_ASSERT (px->right == ASE_NULL);
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
				ASE_ASSERT (px->idx == ASE_NULL);
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
				ASE_ASSERT (px->idx != ASE_NULL);
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
				ASE_AWK_FREE (awk, px->what.bfn.name.ptr);
				ase_awk_clrpt (awk, px->args);
				ASE_AWK_FREE (awk, p);
				break;
			}

			case ASE_AWK_NDE_AFN:
			{
				ase_awk_nde_call_t* px = (ase_awk_nde_call_t*)p;
				ASE_AWK_FREE (awk, px->what.afn.name.ptr);
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
				ASE_ASSERT (!"should never happen - invalid node type");
				ASE_AWK_FREE (awk, p);
				break;
			}
		}

		p = next;
	}
}
