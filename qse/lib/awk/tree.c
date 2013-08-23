/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include "awk.h"
#include <qse/cmn/fmt.h>

static const qse_char_t* assop_str[] =
{
	/* this table must match qse_awk_assop_type_t in run.h */
	QSE_T("="),
	QSE_T("+="),
	QSE_T("-="),
	QSE_T("*="),
	QSE_T("/="),
	QSE_T("\\="),
	QSE_T("%="),
	QSE_T("**="), /* exponentation, also ^= */
	QSE_T("%%="),
	QSE_T(">>="),
	QSE_T("<<="),
	QSE_T("&="),
	QSE_T("^^="),
	QSE_T("|=")
};

static const qse_char_t* binop_str[][2] =
{
	{ QSE_T("||"), QSE_T("||") },
	{ QSE_T("&&"), QSE_T("&&") },
	{ QSE_T("in"), QSE_T("in") },

	{ QSE_T("|"),  QSE_T("|") },
	{ QSE_T("^^"), QSE_T("^^") },
	{ QSE_T("&"),  QSE_T("&") },

	{ QSE_T("==="), QSE_T("===") },
	{ QSE_T("!=="), QSE_T("!==") },
	{ QSE_T("=="), QSE_T("==") },
	{ QSE_T("!="), QSE_T("!=") },
	{ QSE_T(">"),  QSE_T(">") },
	{ QSE_T(">="), QSE_T(">=") },
	{ QSE_T("<"),  QSE_T("<") },
	{ QSE_T("<="), QSE_T("<=") },

	{ QSE_T("<<"), QSE_T("<<") },
	{ QSE_T(">>"), QSE_T(">>") },

	{ QSE_T("+"),  QSE_T("+") },
	{ QSE_T("-"),  QSE_T("-") },
	{ QSE_T("*"),  QSE_T("*") },
	{ QSE_T("/"),  QSE_T("/") },
	{ QSE_T("\\"), QSE_T("\\") },
	{ QSE_T("%"),  QSE_T("%") },
	{ QSE_T("**"), QSE_T("**") }, /* exponentation, also ^ */

	{ QSE_T(" "),  QSE_T("%%") }, /* take note of this entry */
	{ QSE_T("~"),  QSE_T("~") },
	{ QSE_T("!~"), QSE_T("!~") }
};

static const qse_char_t* unrop_str[] =
{
	QSE_T("+"),
	QSE_T("-"),
	QSE_T("!"),
	QSE_T("~~"),
	QSE_T("`")
};

static const qse_char_t* incop_str[] =
{
	QSE_T("++"),
	QSE_T("--"),
	QSE_T("++"),
	QSE_T("--")
};

static const qse_char_t* getline_inop_str[] =
{
	QSE_T("|"),
	QSE_T("||"),
	QSE_T("<"),
	QSE_T("")
};

static const qse_char_t* print_outop_str[] =
{
	QSE_T("|"),
	QSE_T("||"),
	QSE_T(">"),
	QSE_T(">>"),
	QSE_T("")
};

#define PUT_SRCSTR(awk,str) QSE_BLOCK (\
	if (qse_awk_putsrcstr (awk, str) == -1) return -1; \
)

#define PUT_NL(awk) QSE_BLOCK (\
	if (awk->opt.trait & QSE_AWK_CRLF) PUT_SRCSTR (awk, QSE_T("\r")); \
	PUT_SRCSTR (awk, QSE_T("\n")); \
)

#define PUT_SRCSTRN(awk,str,len) QSE_BLOCK (\
	if (qse_awk_putsrcstrn (awk, str, len) == -1) return -1; \
)

#define PRINT_TABS(awk,depth) QSE_BLOCK (\
	if (print_tabs(awk,depth) == -1) return -1; \
)

#define PRINT_EXPR(awk,nde) QSE_BLOCK (\
	if (print_expr(awk,nde) == -1) return -1; \
)

#define PRINT_EXPR_LIST(awk,nde) QSE_BLOCK(\
	if (print_expr_list(awk,nde) == -1) return -1; \
)

#define PRINT_STMTS(awk,nde,depth) QSE_BLOCK(\
	if (print_stmts(awk,nde,depth) == -1) return -1; \
)

static int print_tabs (qse_awk_t* awk, int depth);
static int print_expr (qse_awk_t* awk, qse_awk_nde_t* nde);
static int print_expr_list (qse_awk_t* awk, qse_awk_nde_t* tree);
static int print_stmts (qse_awk_t* awk, qse_awk_nde_t* tree, int depth);

static int print_tabs (qse_awk_t* awk, int depth)
{
	while (depth > 0) 
	{
		PUT_SRCSTR (awk, QSE_T("\t"));
		depth--;
	}

	return 0;
}

static int print_printx (qse_awk_t* awk, qse_awk_nde_print_t* px)
{
	qse_cstr_t kw;

	if (px->type == QSE_AWK_NDE_PRINT) 
	{
		qse_awk_getkwname (awk, QSE_AWK_KWID_PRINT, &kw);
		PUT_SRCSTRN (awk, kw.ptr, kw.len);
	}
	else
	{
		qse_awk_getkwname (awk, QSE_AWK_KWID_PRINTF, &kw);
		PUT_SRCSTRN (awk, kw.ptr, kw.len);
	}

	if (px->args != QSE_NULL)
	{
		PUT_SRCSTR (awk, QSE_T(" "));
		PRINT_EXPR_LIST (awk, px->args);
	}

	if (px->out != QSE_NULL)
	{
		PUT_SRCSTR (awk, QSE_T(" "));
		PUT_SRCSTR (awk, print_outop_str[px->out_type]);
		PUT_SRCSTR (awk, QSE_T(" "));
		PRINT_EXPR (awk, px->out);
	}

	return 0;
}

static int print_expr (qse_awk_t* awk, qse_awk_nde_t* nde)
{
	qse_cstr_t kw;
	
	switch (nde->type) 
	{
		case QSE_AWK_NDE_GRP:
		{	
			qse_awk_nde_t* p = ((qse_awk_nde_grp_t*)nde)->body;

			PUT_SRCSTR (awk, QSE_T("("));
			while (p != QSE_NULL) 
			{
				PRINT_EXPR (awk, p);
				if (p->next != QSE_NULL) 
					PUT_SRCSTR (awk, QSE_T(","));
				p = p->next;
			}
			PUT_SRCSTR (awk, QSE_T(")"));
			break;
		}

		case QSE_AWK_NDE_ASS:
		{
			qse_awk_nde_ass_t* px = (qse_awk_nde_ass_t*)nde;

			PRINT_EXPR (awk, px->left);
			PUT_SRCSTR (awk, QSE_T(" "));
			PUT_SRCSTR (awk, assop_str[px->opcode]);
			PUT_SRCSTR (awk, QSE_T(" "));
			PRINT_EXPR (awk, px->right);

			QSE_ASSERT (px->right->next == QSE_NULL);
			break;
		}

		case QSE_AWK_NDE_EXP_BIN:
		{
			qse_awk_nde_exp_t* px = (qse_awk_nde_exp_t*)nde;

			PUT_SRCSTR (awk, QSE_T("("));
			PRINT_EXPR (awk, px->left);
			QSE_ASSERT (px->left->next == QSE_NULL);

			PUT_SRCSTR (awk, QSE_T(" "));
			PUT_SRCSTR (awk, binop_str[px->opcode][(awk->opt.trait & QSE_AWK_BLANKCONCAT)? 0: 1]);
			PUT_SRCSTR (awk, QSE_T(" "));

			if (px->right->type == QSE_AWK_NDE_ASS) 
				PUT_SRCSTR (awk, QSE_T("("));
			PRINT_EXPR (awk, px->right);
			if (px->right->type == QSE_AWK_NDE_ASS) 
				PUT_SRCSTR (awk, QSE_T(")"));
			QSE_ASSERT (px->right->next == QSE_NULL); 
			PUT_SRCSTR (awk, QSE_T(")"));
			break;
		}

		case QSE_AWK_NDE_EXP_UNR:
		{
			qse_awk_nde_exp_t* px = (qse_awk_nde_exp_t*)nde;
			QSE_ASSERT (px->right == QSE_NULL);

			PUT_SRCSTR (awk, QSE_T("("));
			PUT_SRCSTR (awk, unrop_str[px->opcode]);
			PUT_SRCSTR (awk, QSE_T("("));
			PRINT_EXPR (awk, px->left);
			PUT_SRCSTR (awk, QSE_T(")"));
			PUT_SRCSTR (awk, QSE_T(")"));
			break;
		}

		case QSE_AWK_NDE_EXP_INCPRE:
		{
			qse_awk_nde_exp_t* px = (qse_awk_nde_exp_t*)nde;
			QSE_ASSERT (px->right == QSE_NULL);

			PUT_SRCSTR (awk, incop_str[px->opcode]);
			PUT_SRCSTR (awk, QSE_T("("));
			PRINT_EXPR (awk, px->left);
			PUT_SRCSTR (awk, QSE_T(")"));
			break;
		}

		case QSE_AWK_NDE_EXP_INCPST:
		{
			qse_awk_nde_exp_t* px = (qse_awk_nde_exp_t*)nde;
			QSE_ASSERT (px->right == QSE_NULL);

			PUT_SRCSTR (awk, QSE_T("("));
			PRINT_EXPR (awk, px->left);
			PUT_SRCSTR (awk, QSE_T(")"));
			PUT_SRCSTR (awk, incop_str[px->opcode]);
			break;
		}

		case QSE_AWK_NDE_CND:
		{
			qse_awk_nde_cnd_t* px = (qse_awk_nde_cnd_t*)nde;

			PUT_SRCSTR (awk, QSE_T("("));
			PRINT_EXPR (awk, px->test);
			PUT_SRCSTR (awk, QSE_T(")?"));

			PRINT_EXPR (awk, px->left);
			PUT_SRCSTR (awk, QSE_T(":"));
			PRINT_EXPR (awk, px->right);
			break;
		}

		case QSE_AWK_NDE_INT:
		{
			if (((qse_awk_nde_int_t*)nde)->str)
			{
				PUT_SRCSTRN (awk,
					((qse_awk_nde_int_t*)nde)->str,
					((qse_awk_nde_int_t*)nde)->len);
			}
			else
			{
				/* Note that the array sizing fomula is not accurate 
				 * but should be good enoug consiering the followings.
				 *
				 * size minval                               digits  sign
				 *   1  -128                                      3  1
				 *   2  -32768                                    5  1
				 *   4  -2147483648                              10  1
				 *   8  -9223372036854775808                     19  1
				 *  16  -170141183460469231731687303715884105728 39  1
				 */
				qse_char_t buf[QSE_SIZEOF(qse_long_t) * 3 + 2]; 

				qse_fmtintmax (
					buf, QSE_COUNTOF(buf),
					((qse_awk_nde_int_t*)nde)->val,
					10, 
					-1,
					QSE_T('\0'),
					QSE_NULL
				);
				PUT_SRCSTR (awk, buf);
			}
			break;
		}

		case QSE_AWK_NDE_FLT:
		{
			if (((qse_awk_nde_flt_t*)nde)->str)
			{
				PUT_SRCSTRN (awk,
					((qse_awk_nde_flt_t*)nde)->str,
					((qse_awk_nde_flt_t*)nde)->len);
			}
			else
			{
				qse_char_t buf[64];
				qse_awk_sprintflt (
					awk, buf, QSE_COUNTOF(buf), 
					((qse_awk_nde_flt_t*)nde)->val);
				PUT_SRCSTR (awk, buf);
			}
			break;
		}

		case QSE_AWK_NDE_STR:
		{
			qse_char_t* ptr;
			qse_size_t len, i;

			PUT_SRCSTR (awk, QSE_T("\""));

			ptr = ((qse_awk_nde_str_t*)nde)->ptr;
			len = ((qse_awk_nde_str_t*)nde)->len;
			for (i = 0; i < len; i++)
			{
				/* TODO: maybe more de-escaping?? */
				switch (ptr[i])
				{
					case QSE_T('\n'):
						PUT_SRCSTR (awk, QSE_T("\\n"));
						break;
					case QSE_T('\r'):
						PUT_SRCSTR (awk, QSE_T("\\r"));
						break;
					case QSE_T('\t'):
						PUT_SRCSTR (awk, QSE_T("\\t"));
						break;
					case QSE_T('\f'):
						PUT_SRCSTR (awk, QSE_T("\\f"));
						break;
					case QSE_T('\b'):
						PUT_SRCSTR (awk, QSE_T("\\b"));
						break;
					case QSE_T('\v'):
						PUT_SRCSTR (awk, QSE_T("\\v"));
						break;
					case QSE_T('\a'):
						PUT_SRCSTR (awk, QSE_T("\\a"));
						break;
					case QSE_T('\0'):
						PUT_SRCSTR (awk, QSE_T("\\0"));
						break;
					case QSE_T('\"'):
						PUT_SRCSTR (awk, QSE_T("\\\""));
						break;
					case QSE_T('\\'):
						PUT_SRCSTR (awk, QSE_T("\\\\"));
						break;
					default:
						PUT_SRCSTRN (awk, &ptr[i], 1);
						break;
				}
			}
			PUT_SRCSTR (awk, QSE_T("\""));
			break;
		}

		case QSE_AWK_NDE_REX:
		{
			PUT_SRCSTR (awk, QSE_T("/"));
			PUT_SRCSTRN (awk,
				((qse_awk_nde_rex_t*)nde)->str.ptr, 
				((qse_awk_nde_rex_t*)nde)->str.len);
			PUT_SRCSTR (awk, QSE_T("/"));
			break;
		}

		case QSE_AWK_NDE_ARG:
		{
			qse_char_t tmp[QSE_SIZEOF(qse_long_t)*8+2]; 
			qse_size_t n;
			qse_awk_nde_var_t* px = (qse_awk_nde_var_t*)nde;
			QSE_ASSERT (px->id.idxa != (qse_size_t)-1);

			n = qse_awk_longtostr (
				awk,
				px->id.idxa, 
				10, QSE_NULL, tmp, QSE_COUNTOF(tmp)
			);

			PUT_SRCSTR (awk, QSE_T("__p"));
			PUT_SRCSTRN (awk, tmp, n);

			QSE_ASSERT (px->idx == QSE_NULL);
			break;
		}

		case QSE_AWK_NDE_ARGIDX:
		{
			qse_size_t n;
			qse_awk_nde_var_t* px = (qse_awk_nde_var_t*)nde;
			QSE_ASSERT (px->id.idxa != (qse_size_t)-1);
			QSE_ASSERT (px->idx != QSE_NULL);

			PUT_SRCSTR (awk, QSE_T("__p"));
			n = qse_awk_longtostr (
				awk,
				px->id.idxa, 10, QSE_NULL,
				awk->tmp.fmt, QSE_COUNTOF(awk->tmp.fmt)
			);
			PUT_SRCSTRN (awk, awk->tmp.fmt, n);
			PUT_SRCSTR (awk, QSE_T("["));
			PRINT_EXPR_LIST (awk, px->idx);
			PUT_SRCSTR (awk, QSE_T("]"));
			break;
		}

		case QSE_AWK_NDE_NAMED:
		{
			qse_awk_nde_var_t* px = (qse_awk_nde_var_t*)nde;
			QSE_ASSERT (px->id.idxa == (qse_size_t)-1);
			QSE_ASSERT (px->idx == QSE_NULL);

			PUT_SRCSTRN (awk, px->id.name.ptr, px->id.name.len);
			break;
		}

		case QSE_AWK_NDE_NAMEDIDX:
		{
			qse_awk_nde_var_t* px = (qse_awk_nde_var_t*)nde;
			QSE_ASSERT (px->id.idxa == (qse_size_t)-1);
			QSE_ASSERT (px->idx != QSE_NULL);

			PUT_SRCSTRN (awk, px->id.name.ptr, px->id.name.len);
			PUT_SRCSTR (awk, QSE_T("["));
			PRINT_EXPR_LIST (awk, px->idx);
			PUT_SRCSTR (awk, QSE_T("]"));
			break;
		}

		case QSE_AWK_NDE_GBL:
		{
			qse_awk_nde_var_t* px = (qse_awk_nde_var_t*)nde;

			if (px->id.idxa != (qse_size_t)-1) 
			{

				if (!(awk->opt.trait & QSE_AWK_IMPLICIT))
				{
					/* no implicit(named) variable is allowed.
					 * use the actual name */
					PUT_SRCSTRN (awk, px->id.name.ptr, px->id.name.len);
				}
				else if (px->id.idxa < awk->tree.ngbls_base)
				{
					/* static global variables */
					PUT_SRCSTRN (awk, px->id.name.ptr, px->id.name.len);
				}
				else
				{
					qse_char_t tmp[QSE_SIZEOF(qse_long_t)*8+2]; 
					qse_size_t n;

					PUT_SRCSTR (awk, QSE_T("__g"));
					n = qse_awk_longtostr (
						awk,
						px->id.idxa, 
						10, 
						QSE_NULL, 
						tmp, QSE_COUNTOF(tmp)
					);
					PUT_SRCSTRN (awk, tmp, n);
				}
			}
			else 
			{
				PUT_SRCSTRN (awk, px->id.name.ptr, px->id.name.len);
			}
			QSE_ASSERT (px->idx == QSE_NULL);
			break;
		}

		case QSE_AWK_NDE_GBLIDX:
		{
			qse_awk_nde_var_t* px = (qse_awk_nde_var_t*)nde;

			if (px->id.idxa != (qse_size_t)-1) 
			{
				if (!(awk->opt.trait & QSE_AWK_IMPLICIT))
				{
					/* no implicit(named) variable is allowed.
					 * use the actual name */
					PUT_SRCSTRN (awk, px->id.name.ptr, px->id.name.len);
				}
				else if (px->id.idxa < awk->tree.ngbls_base)
				{
					/* static global variables */
					PUT_SRCSTRN (awk, px->id.name.ptr, px->id.name.len);
				}
				else
				{
					qse_char_t tmp[QSE_SIZEOF(qse_long_t)*8+2]; 
					qse_size_t n;

					PUT_SRCSTR (awk, QSE_T("__g"));
					n = qse_awk_longtostr (
						awk,
						px->id.idxa, 
						10, 
						QSE_NULL,
						tmp, QSE_COUNTOF(tmp)
					);
					PUT_SRCSTRN (awk, tmp, n);
				}
				PUT_SRCSTR (awk, QSE_T("["));
			}
			else 
			{
				PUT_SRCSTRN (awk, px->id.name.ptr, px->id.name.len);
				PUT_SRCSTR (awk, QSE_T("["));
			}
			QSE_ASSERT (px->idx != QSE_NULL);
			PRINT_EXPR_LIST (awk, px->idx);
			PUT_SRCSTR (awk, QSE_T("]"));
			break;
		}

		case QSE_AWK_NDE_LCL:
		{
			qse_size_t n;
			qse_awk_nde_var_t* px = (qse_awk_nde_var_t*)nde;

			if (px->id.idxa != (qse_size_t)-1) 
			{
				PUT_SRCSTR (awk, QSE_T("__l"));
				n = qse_awk_longtostr (
					awk,
					px->id.idxa,
					10, 
					QSE_NULL, 
					awk->tmp.fmt,
					QSE_COUNTOF(awk->tmp.fmt)
				);
				PUT_SRCSTRN (awk, awk->tmp.fmt, n);
			}
			else 
			{
				PUT_SRCSTRN (awk, px->id.name.ptr, px->id.name.len);
			}
			QSE_ASSERT (px->idx == QSE_NULL);
			break;
		}

		case QSE_AWK_NDE_LCLIDX:
		{
			qse_size_t n;
			qse_awk_nde_var_t* px = (qse_awk_nde_var_t*)nde;

			if (px->id.idxa != (qse_size_t)-1) 
			{
				PUT_SRCSTR (awk, QSE_T("__l"));
				n = qse_awk_longtostr (
					awk,
					px->id.idxa,
					10,
					QSE_NULL, 
					awk->tmp.fmt,
					QSE_COUNTOF(awk->tmp.fmt)
				);
				PUT_SRCSTRN (awk, awk->tmp.fmt, n);
				PUT_SRCSTR (awk, QSE_T("["));
			}
			else 
			{
				PUT_SRCSTRN (awk, px->id.name.ptr, px->id.name.len);
				PUT_SRCSTR (awk, QSE_T("["));
			}
			QSE_ASSERT (px->idx != QSE_NULL);
			PRINT_EXPR_LIST (awk, px->idx);
			PUT_SRCSTR (awk, QSE_T("]"));
			break;
		}

		case QSE_AWK_NDE_POS:
		{
			PUT_SRCSTR (awk, QSE_T("$"));
			PRINT_EXPR (awk, ((qse_awk_nde_pos_t*)nde)->val);
			break;
		}

		case QSE_AWK_NDE_FNC:
		{
			qse_awk_nde_fncall_t* px = (qse_awk_nde_fncall_t*)nde;
			PUT_SRCSTRN (awk, px->u.fnc.info.name.ptr, px->u.fnc.info.name.len);
			PUT_SRCSTR (awk, QSE_T("("));
			PRINT_EXPR_LIST (awk, px->args);
			PUT_SRCSTR (awk, QSE_T(")"));
			break;
		}

		case QSE_AWK_NDE_FUN:
		{
			qse_awk_nde_fncall_t* px = (qse_awk_nde_fncall_t*)nde;
			PUT_SRCSTRN (awk, px->u.fun.name.ptr, px->u.fun.name.len);
			PUT_SRCSTR (awk, QSE_T("("));
			PRINT_EXPR_LIST (awk, px->args);
			PUT_SRCSTR (awk, QSE_T(")"));
			break;
		}

		case QSE_AWK_NDE_GETLINE:
		{
			qse_awk_nde_getline_t* px = (qse_awk_nde_getline_t*)nde;

			PUT_SRCSTR (awk, QSE_T("("));

			if (px->in != QSE_NULL &&
			    (px->in_type == QSE_AWK_IN_PIPE ||
			     px->in_type == QSE_AWK_IN_RWPIPE))
			{
				PRINT_EXPR (awk, px->in);
				PUT_SRCSTR (awk, QSE_T(" "));
				PUT_SRCSTR (awk, getline_inop_str[px->in_type]);
				PUT_SRCSTR (awk, QSE_T(" "));
			}

			qse_awk_getkwname (awk, QSE_AWK_KWID_GETLINE, &kw);
			PUT_SRCSTRN (awk, kw.ptr, kw.len);
			if (px->var != QSE_NULL)
			{
				PUT_SRCSTR (awk, QSE_T(" "));
				PRINT_EXPR (awk, px->var);
			}

			if (px->in != QSE_NULL &&
			    px->in_type == QSE_AWK_IN_FILE)
			{
				PUT_SRCSTR (awk, QSE_T(" "));
				PUT_SRCSTR (awk, getline_inop_str[px->in_type]);
				PUT_SRCSTR (awk, QSE_T(" "));
				PRINT_EXPR (awk, px->in);
			}	  

			PUT_SRCSTR (awk, QSE_T(")"));
			break;
		}

		case QSE_AWK_NDE_PRINT:
		case QSE_AWK_NDE_PRINTF:
		{
			PUT_SRCSTR (awk, QSE_T("("));
			if (print_printx (awk, (qse_awk_nde_print_t*)nde) <= -1) return -1;
			PUT_SRCSTR (awk, QSE_T(")"));
			break;
		}

		default:
		{
			qse_awk_seterrnum (awk, QSE_AWK_EINTERN, QSE_NULL);
			return -1;
		}
	}

	return 0;
}

static int print_expr_list (qse_awk_t* awk, qse_awk_nde_t* tree)
{
	qse_awk_nde_t* p = tree;

	while (p != QSE_NULL) 
	{
		PRINT_EXPR (awk, p);
		p = p->next;
		if (p != QSE_NULL) PUT_SRCSTR (awk, QSE_T(","));
	}

	return 0;
}

static int print_stmt (qse_awk_t* awk, qse_awk_nde_t* p, int depth)
{
	qse_size_t i;
	qse_cstr_t kw;

	switch (p->type) 
	{
		case QSE_AWK_NDE_NULL:
		{
			PRINT_TABS (awk, depth);
			PUT_SRCSTR (awk, QSE_T(";"));
			PUT_NL (awk);
			break;
		}

		case QSE_AWK_NDE_BLK:
		{
			qse_size_t n;
			qse_awk_nde_blk_t* px = (qse_awk_nde_blk_t*)p;

			PRINT_TABS (awk, depth);
			PUT_SRCSTR (awk, QSE_T("{"));
			PUT_NL (awk);

			if (px->nlcls > 0) 
			{
				PRINT_TABS (awk, depth + 1);

				qse_awk_getkwname (awk, QSE_AWK_KWID_XLOCAL, &kw);
				PUT_SRCSTRN (awk, kw.ptr, kw.len);
				PUT_SRCSTR (awk, QSE_T(" "));

				for (i = 0; i < px->nlcls - 1; i++) 
				{
					PUT_SRCSTR (awk, QSE_T("__l"));
					n = qse_awk_longtostr (
						awk,
						i, 
						10,
						QSE_NULL, 
						awk->tmp.fmt,
						QSE_COUNTOF(awk->tmp.fmt)
					);
					PUT_SRCSTRN (awk, awk->tmp.fmt, n);
					PUT_SRCSTR (awk, QSE_T(", "));
				}

				PUT_SRCSTR (awk, QSE_T("__l"));
				n = qse_awk_longtostr (
					awk,
					i, 
					10,
					QSE_NULL, 
					awk->tmp.fmt,
					QSE_COUNTOF(awk->tmp.fmt)
				);
				PUT_SRCSTRN (awk, awk->tmp.fmt, n);
				PUT_SRCSTR (awk, QSE_T(";"));
				PUT_NL (awk);
			}

			PRINT_STMTS (awk, px->body, depth + 1);	
			PRINT_TABS (awk, depth);
			PUT_SRCSTR (awk, QSE_T("}"));
			PUT_NL (awk);
			break;
		}

		case QSE_AWK_NDE_IF: 
		{
			qse_awk_nde_if_t* px = (qse_awk_nde_if_t*)p;

			PRINT_TABS (awk, depth);
			qse_awk_getkwname (awk, QSE_AWK_KWID_IF, &kw);
			PUT_SRCSTRN (awk, kw.ptr, kw.len);
			PUT_SRCSTR (awk, QSE_T(" ("));	
			PRINT_EXPR (awk, px->test);
			PUT_SRCSTR (awk, QSE_T(")"));
			PUT_NL (awk);

			QSE_ASSERT (px->then_part != QSE_NULL);
			if (px->then_part->type == QSE_AWK_NDE_BLK)
				PRINT_STMTS (awk, px->then_part, depth);
			else
				PRINT_STMTS (awk, px->then_part, depth + 1);

			if (px->else_part != QSE_NULL) 
			{
				PRINT_TABS (awk, depth);
				qse_awk_getkwname (awk, QSE_AWK_KWID_ELSE, &kw);
				PUT_SRCSTRN (awk, kw.ptr, kw.len);
				PUT_NL (awk);
				if (px->else_part->type == QSE_AWK_NDE_BLK)
					PRINT_STMTS (awk, px->else_part, depth);
				else
					PRINT_STMTS (awk, px->else_part, depth + 1);
			}
			break;
		}

		case QSE_AWK_NDE_WHILE: 
		{
			qse_awk_nde_while_t* px = (qse_awk_nde_while_t*)p;

			PRINT_TABS (awk, depth);
			qse_awk_getkwname (awk, QSE_AWK_KWID_WHILE, &kw);
			PUT_SRCSTRN (awk, kw.ptr, kw.len);
			PUT_SRCSTR (awk, QSE_T(" ("));	
			PRINT_EXPR (awk, px->test);
			PUT_SRCSTR (awk, QSE_T(")"));
			PUT_NL (awk);
			if (px->body->type == QSE_AWK_NDE_BLK) 
			{
				PRINT_STMTS (awk, px->body, depth);
			}
			else 
			{
				PRINT_STMTS (awk, px->body, depth + 1);
			}
			break;
		}

		case QSE_AWK_NDE_DOWHILE: 
		{
			qse_awk_nde_while_t* px = (qse_awk_nde_while_t*)p;

			PRINT_TABS (awk, depth);
			qse_awk_getkwname (awk, QSE_AWK_KWID_DO, &kw);
			PUT_SRCSTRN (awk, kw.ptr, kw.len);
			PUT_NL (awk);
			if (px->body->type == QSE_AWK_NDE_BLK) 
			{
				PRINT_STMTS (awk, px->body, depth);
			}
			else 
			{
				PRINT_STMTS (awk, px->body, depth + 1);
			}

			PRINT_TABS (awk, depth);
			qse_awk_getkwname (awk, QSE_AWK_KWID_WHILE, &kw);
			PUT_SRCSTRN (awk, kw.ptr, kw.len);
			PUT_SRCSTR (awk, QSE_T(" ("));	
			PRINT_EXPR (awk, px->test);
			PUT_SRCSTR (awk, QSE_T(");"));
			PUT_NL (awk);
			break;
		}

		case QSE_AWK_NDE_FOR:
		{
			qse_awk_nde_for_t* px = (qse_awk_nde_for_t*)p;

			PRINT_TABS (awk, depth);
			qse_awk_getkwname (awk, QSE_AWK_KWID_FOR, &kw);
			PUT_SRCSTRN (awk, kw.ptr, kw.len);
			PUT_SRCSTR (awk, QSE_T(" ("));
			if (px->init != QSE_NULL) 
			{
				PRINT_EXPR (awk, px->init);
			}
			PUT_SRCSTR (awk, QSE_T("; "));
			if (px->test != QSE_NULL) 
			{
				PRINT_EXPR (awk, px->test);
			}
			PUT_SRCSTR (awk, QSE_T("; "));
			if (px->incr != QSE_NULL) 
			{
				PRINT_EXPR (awk, px->incr);
			}
			PUT_SRCSTR (awk, QSE_T(")"));
			PUT_NL (awk);

			if (px->body->type == QSE_AWK_NDE_BLK) 
			{
				PRINT_STMTS (awk, px->body, depth);
			}
			else 
			{
				PRINT_STMTS (awk, px->body, depth + 1);
			}
			break;
		}

		case QSE_AWK_NDE_FOREACH:
		{
			qse_awk_nde_foreach_t* px = (qse_awk_nde_foreach_t*)p;

			PRINT_TABS (awk, depth);
			qse_awk_getkwname (awk, QSE_AWK_KWID_FOR, &kw);
			PUT_SRCSTRN (awk, kw.ptr, kw.len);
			PUT_SRCSTR (awk, QSE_T(" "));
			PRINT_EXPR (awk, px->test);
			PUT_NL (awk);
			if (px->body->type == QSE_AWK_NDE_BLK) 
			{
				PRINT_STMTS (awk, px->body, depth);
			}
			else 
			{
				PRINT_STMTS (awk, px->body, depth + 1);
			}
			break;
		}

		case QSE_AWK_NDE_BREAK:
		{
			PRINT_TABS (awk, depth);
			qse_awk_getkwname (awk, QSE_AWK_KWID_BREAK, &kw);
			PUT_SRCSTRN (awk, kw.ptr, kw.len);
			PUT_SRCSTR (awk, QSE_T(";"));
			PUT_NL (awk);
			break;
		}

		case QSE_AWK_NDE_CONTINUE:
		{
			PRINT_TABS (awk, depth);
			qse_awk_getkwname (awk, QSE_AWK_KWID_CONTINUE, &kw);
			PUT_SRCSTRN (awk, kw.ptr, kw.len);
			PUT_SRCSTR (awk, QSE_T(";"));
			PUT_NL (awk);
			break;
		}

		case QSE_AWK_NDE_RETURN:
		{
			PRINT_TABS (awk, depth);
			if (((qse_awk_nde_return_t*)p)->val == QSE_NULL) 
			{
				qse_awk_getkwname (awk, QSE_AWK_KWID_RETURN, &kw);
				PUT_SRCSTRN (awk, kw.ptr, kw.len);
				PUT_SRCSTR (awk, QSE_T(";"));
				PUT_NL (awk);
			}
			else 
			{
				qse_awk_getkwname (awk, QSE_AWK_KWID_RETURN, &kw);
				PUT_SRCSTRN (awk, kw.ptr, kw.len);
				PUT_SRCSTR (awk, QSE_T(" "));
				QSE_ASSERT (((qse_awk_nde_return_t*)p)->val->next == QSE_NULL);

				PRINT_EXPR (awk, ((qse_awk_nde_return_t*)p)->val);
				PUT_SRCSTR (awk, QSE_T(";"));
				PUT_NL (awk);
			}
			break;
		}

		case QSE_AWK_NDE_EXIT:
		{
			qse_awk_nde_exit_t* px = (qse_awk_nde_exit_t*)p;
			PRINT_TABS (awk, depth);

			if (px->val == QSE_NULL) 
			{
				qse_awk_getkwname (awk, (px->abort? QSE_AWK_KWID_XABORT: QSE_AWK_KWID_EXIT), &kw);
				PUT_SRCSTRN (awk, kw.ptr, kw.len);
				PUT_SRCSTR (awk, QSE_T(";"));
				PUT_NL (awk);
			}
			else 
			{
				qse_awk_getkwname (awk, (px->abort? QSE_AWK_KWID_XABORT: QSE_AWK_KWID_EXIT), &kw);
				PUT_SRCSTRN (awk, kw.ptr, kw.len);
				PUT_SRCSTR (awk, QSE_T(" "));
				QSE_ASSERT (px->val->next == QSE_NULL);
				PRINT_EXPR (awk, px->val);
				PUT_SRCSTR (awk, QSE_T(";"));
				PUT_NL (awk);
			}
			break;
		}

		case QSE_AWK_NDE_NEXT:
		{
			PRINT_TABS (awk, depth);
			qse_awk_getkwname (awk, QSE_AWK_KWID_NEXT, &kw);
			PUT_SRCSTRN (awk, kw.ptr, kw.len);
			PUT_SRCSTR (awk, QSE_T(";"));
			PUT_NL (awk);
			break;
		}

		case QSE_AWK_NDE_NEXTFILE:
		{
			PRINT_TABS (awk, depth);
			if (((qse_awk_nde_nextfile_t*)p)->out)
			{
				qse_awk_getkwname (awk, QSE_AWK_KWID_NEXTOFILE, &kw);
				PUT_SRCSTRN (awk, kw.ptr, kw.len);
			}
			else
			{
				qse_awk_getkwname (awk, QSE_AWK_KWID_NEXTFILE, &kw);
				PUT_SRCSTRN (awk, kw.ptr, kw.len);
			}
			PUT_SRCSTR (awk, QSE_T(";"));
			PUT_NL (awk);
			break;
		}

		case QSE_AWK_NDE_DELETE:
		{
			PRINT_TABS (awk, depth);
			qse_awk_getkwname (awk, QSE_AWK_KWID_DELETE, &kw);
			PUT_SRCSTRN (awk, kw.ptr, kw.len);
			PUT_SRCSTR (awk, QSE_T(" "));
			qse_awk_prnpt (awk, ((qse_awk_nde_delete_t*)p)->var);
			break;
		}

		case QSE_AWK_NDE_RESET:
		{
			PRINT_TABS (awk, depth);
			qse_awk_getkwname (awk, QSE_AWK_KWID_XRESET, &kw);
			PUT_SRCSTRN (awk, kw.ptr, kw.len);
			PUT_SRCSTR (awk, QSE_T(" "));
			qse_awk_prnpt (awk, ((qse_awk_nde_reset_t*)p)->var);
			break;
		}

		case QSE_AWK_NDE_PRINT:
		case QSE_AWK_NDE_PRINTF:
		{
			PRINT_TABS (awk, depth);
			if (print_printx (awk, (qse_awk_nde_print_t*)p) <= -1) return -1;
			PUT_SRCSTR (awk, QSE_T(";"));
			PUT_NL (awk);
			break;
		}

		default:
		{
			PRINT_TABS (awk, depth);
			PRINT_EXPR (awk, p);
			PUT_SRCSTR (awk, QSE_T(";"));
			PUT_NL (awk);
			break;
		}
	}

	return 0;
}

static int print_stmts (qse_awk_t* awk, qse_awk_nde_t* tree, int depth)
{
	qse_awk_nde_t* p = tree;

	while (p != QSE_NULL) 
	{
		if (print_stmt (awk, p, depth) == -1) return -1;
		p = p->next;
	}

	return 0;
}

int qse_awk_prnpt (qse_awk_t* awk, qse_awk_nde_t* tree)
{
	return print_stmts (awk, tree, 0);
}

int qse_awk_prnnde (qse_awk_t* awk, qse_awk_nde_t* tree)
{
	return print_stmt (awk, tree, 0);
}

int qse_awk_prnptnpt (qse_awk_t* awk, qse_awk_nde_t* tree)
{
	qse_awk_nde_t* nde = tree;

	while (nde != QSE_NULL)
	{
		if (print_expr (awk, nde) == -1) return -1;
		if (nde->next == QSE_NULL) break;

		PUT_SRCSTR (awk, QSE_T(","));
		nde = nde->next;
	}

	return 0;
}

void qse_awk_clrpt (qse_awk_t* awk, qse_awk_nde_t* tree)
{
	qse_awk_nde_t* p = tree;
	qse_awk_nde_t* next;

	while (p != QSE_NULL) 
	{
		next = p->next;

		switch (p->type) 
		{
			case QSE_AWK_NDE_NULL:
			{
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_BLK:
			{
				qse_awk_clrpt (awk, ((qse_awk_nde_blk_t*)p)->body);
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_IF:
			{
				qse_awk_nde_if_t* px = (qse_awk_nde_if_t*)p;
				qse_awk_clrpt (awk, px->test);
				qse_awk_clrpt (awk, px->then_part);

				if (px->else_part != QSE_NULL)
					qse_awk_clrpt (awk, px->else_part);
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_WHILE:
			case QSE_AWK_NDE_DOWHILE:
			{
				qse_awk_clrpt (awk, ((qse_awk_nde_while_t*)p)->test);
				qse_awk_clrpt (awk, ((qse_awk_nde_while_t*)p)->body);
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_FOR:
			{
				qse_awk_nde_for_t* px = (qse_awk_nde_for_t*)p;

				if (px->init != QSE_NULL)
					qse_awk_clrpt (awk, px->init);
				if (px->test != QSE_NULL)
					qse_awk_clrpt (awk, px->test);
				if (px->incr != QSE_NULL)
					qse_awk_clrpt (awk, px->incr);
				qse_awk_clrpt (awk, px->body);
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_FOREACH:
			{
				qse_awk_clrpt (awk, ((qse_awk_nde_foreach_t*)p)->test);
				qse_awk_clrpt (awk, ((qse_awk_nde_foreach_t*)p)->body);
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_BREAK:
			{
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_CONTINUE:
			{
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_RETURN:
			{
				qse_awk_nde_return_t* px =
					(qse_awk_nde_return_t*)p;
				if (px->val != QSE_NULL) 
					qse_awk_clrpt (awk, px->val);
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_EXIT:
			{
				if (((qse_awk_nde_exit_t*)p)->val != QSE_NULL) 
					qse_awk_clrpt (awk, ((qse_awk_nde_exit_t*)p)->val);
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_NEXT:
			case QSE_AWK_NDE_NEXTFILE:
			{
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_DELETE:
			{
				qse_awk_clrpt (awk, ((qse_awk_nde_delete_t*)p)->var);
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_RESET:
			{
				qse_awk_clrpt (awk, ((qse_awk_nde_reset_t*)p)->var);
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_PRINT:
			case QSE_AWK_NDE_PRINTF:
			{
				qse_awk_nde_print_t* px = 
					(qse_awk_nde_print_t*)p;
				if (px->args != QSE_NULL) 
					qse_awk_clrpt (awk, px->args);
				if (px->out != QSE_NULL) 
					qse_awk_clrpt (awk, px->out);
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_GRP:
			{
				qse_awk_clrpt (awk, ((qse_awk_nde_grp_t*)p)->body);
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_ASS:
			{
				qse_awk_clrpt (awk, ((qse_awk_nde_ass_t*)p)->left);
				qse_awk_clrpt (awk, ((qse_awk_nde_ass_t*)p)->right);
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_EXP_BIN:
			{
				qse_awk_nde_exp_t* px = (qse_awk_nde_exp_t*)p;
				QSE_ASSERT (px->left->next == QSE_NULL);
				QSE_ASSERT (px->right->next == QSE_NULL);

				qse_awk_clrpt (awk, px->left);
				qse_awk_clrpt (awk, px->right);
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_EXP_UNR:
			case QSE_AWK_NDE_EXP_INCPRE:
			case QSE_AWK_NDE_EXP_INCPST:
			{
				qse_awk_nde_exp_t* px = (qse_awk_nde_exp_t*)p;
				QSE_ASSERT (px->right == QSE_NULL);
				qse_awk_clrpt (awk, px->left);
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_CND:
			{
				qse_awk_clrpt (awk, ((qse_awk_nde_cnd_t*)p)->test);
				qse_awk_clrpt (awk, ((qse_awk_nde_cnd_t*)p)->left);
				qse_awk_clrpt (awk, ((qse_awk_nde_cnd_t*)p)->right);
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_INT:
			{
				if (((qse_awk_nde_int_t*)p)->str)
					QSE_AWK_FREE (awk, ((qse_awk_nde_int_t*)p)->str);
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_FLT:
			{
				if (((qse_awk_nde_flt_t*)p)->str)
					QSE_AWK_FREE (awk, ((qse_awk_nde_flt_t*)p)->str);
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_STR:
			{
				QSE_AWK_FREE (awk, ((qse_awk_nde_str_t*)p)->ptr);
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_REX:
			{
				qse_awk_nde_rex_t* rex = (qse_awk_nde_rex_t*)p;
				qse_awk_freerex (awk, rex->code[0], rex->code[1]);
				QSE_AWK_FREE (awk, ((qse_awk_nde_rex_t*)p)->str.ptr);
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_NAMED:
			case QSE_AWK_NDE_GBL:
			case QSE_AWK_NDE_LCL:
			case QSE_AWK_NDE_ARG:
			{
				qse_awk_nde_var_t* px = (qse_awk_nde_var_t*)p;
				QSE_ASSERT (px->idx == QSE_NULL);
				if (px->id.name.ptr) QSE_AWK_FREE (awk, px->id.name.ptr);
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_NAMEDIDX:
			case QSE_AWK_NDE_GBLIDX:
			case QSE_AWK_NDE_LCLIDX:
			case QSE_AWK_NDE_ARGIDX:
			{
				qse_awk_nde_var_t* px = (qse_awk_nde_var_t*)p;
				QSE_ASSERT (px->idx != QSE_NULL);
				qse_awk_clrpt (awk, px->idx);
				if (px->id.name.ptr != QSE_NULL)
					QSE_AWK_FREE (awk, px->id.name.ptr);
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_POS:
			{
				qse_awk_clrpt (awk, ((qse_awk_nde_pos_t*)p)->val);
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_FNC:
			{
				qse_awk_nde_fncall_t* px = (qse_awk_nde_fncall_t*)p;
				/* QSE_AWK_FREE (awk, px->u.fnc); */
				QSE_AWK_FREE (awk, px->u.fnc.info.name.ptr);
				qse_awk_clrpt (awk, px->args);
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_FUN:
			{
				qse_awk_nde_fncall_t* px = (qse_awk_nde_fncall_t*)p;
				QSE_AWK_FREE (awk, px->u.fun.name.ptr);
				qse_awk_clrpt (awk, px->args);
				QSE_AWK_FREE (awk, p);
				break;
			}

			case QSE_AWK_NDE_GETLINE:
			{
				qse_awk_nde_getline_t* px = 
					(qse_awk_nde_getline_t*)p;
				if (px->var != QSE_NULL) 
					qse_awk_clrpt (awk, px->var);
				if (px->in != QSE_NULL) 
					qse_awk_clrpt (awk, px->in);
				QSE_AWK_FREE (awk, p);
				break;
			}

			default:
			{
				QSE_ASSERT (!"should never happen - invalid node type");
				QSE_AWK_FREE (awk, p);
				break;
			}
		}

		p = next;
	}
}
