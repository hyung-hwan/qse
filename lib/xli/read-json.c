/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "xli-prv.h"
#include <qse/cmn/chr.h>

/*
{
	"key1": {
		# comment
		"key11": = "test machine,
		"key1122": [
			1,
 			2,
 			{ 
				"a": 10,
 				"b": 20,
 			}
		]
	}
}
*/

enum
{
	TOK_STATUS_DEINDENT_TEXT = (1 << 0)
};

static int get_token (qse_xli_t* xli);
static int read_list (qse_xli_t* xli, qse_xli_list_t* lv);
static int read_array (qse_xli_t* xli, qse_xli_list_t* lv);

enum
{
	TOK_STATUS_ENABLE_NSTR = (1 << 0)
};

#define GET_CHAR(xli) \
	do { if (qse_xli_getchar(xli) <= -1) return -1; } while(0)

#define GET_CHAR_TO(xli,c) \
	do { \
		if (qse_xli_getchar(xli) <= -1) return -1; \
		c = (xli)->rio.last.c; \
	} while(0)

#define ADD_TOKEN_CHAR(xli,tok,c) \
	do { \
		if (qse_str_ccat((tok)->name,(c)) == (qse_size_t)-1) \
		{ \
			qse_xli_seterrnum (xli, QSE_XLI_ENOMEM, QSE_NULL); \
			return -1; \
		} \
	} while (0)

#define ADD_TOKEN_STR(xli,tok,s,l) \
	do { \
		if (qse_str_ncat((tok)->name,(s),(l)) == (qse_size_t)-1) \
		{ \
			qse_xli_seterrnum (xli, QSE_XLI_ENOMEM, QSE_NULL); \
			return -1; \
		} \
	} while (0)

#if defined(QSE_CHAR_IS_MCHAR)

#	define ADD_TOKEN_UINT32(xli,tok,c) \
	do { \
		if (c <= 0xFF) ADD_TOKEN_CHAR(xli, tok, c); \
		else  \
		{ \
			qse_mchar_t __xbuf[QSE_MBLEN_MAX + 1]; \
			qse_size_t __len, __i; \
			__len = qse_uctoutf8(c, __xbuf, QSE_COUNTOF(__xbuf)); /* use utf8 all the time */ \
			for (__i = 0; __i < __len; __i++) ADD_TOKEN_CHAR(xli, tok, __xbuf[__i]); \
		} \
	} while (0)
#else
#	define ADD_TOKEN_UINT32(xli,tok,c) ADD_TOKEN_CHAR(xli,tok,c);
#endif

#define SET_TOKEN_TYPE(xli,tok,code) \
	do { (tok)->type = (code); } while (0)

#define MATCH(xli,tok_type) ((xli)->tok.type == (tok_type))

typedef struct kwent_t kwent_t;

struct kwent_t 
{ 
	qse_cstr_t name;
	qse_xli_tok_type_t type; 
};

/* note that the keyword must start with @. */
static kwent_t kwtab[] = 
{
	/* keep it sorted by the first field for binary search */
	{ { QSE_T("@include"),     8 }, QSE_XLI_TOK_XINCLUDE },
	{ { QSE_T("false"),        5 }, QSE_XLI_TOK_FALSE    },
	{ { QSE_T("nil"),          3 }, QSE_XLI_TOK_NIL      },
	{ { QSE_T("true"),         4 }, QSE_XLI_TOK_TRUE     }
};

static int skip_spaces (qse_xli_t* xli)
{
	qse_cint_t c = xli->rio.last.c;
	while (QSE_ISSPACE(c)) GET_CHAR_TO (xli, c);
	return 0;
}

static int skip_comment (qse_xli_t* xli, qse_xli_tok_t* tok)
{
	qse_cint_t c = xli->rio.last.c;

	if (c == QSE_T('#'))
	{
		/* skip up to \n */

		qse_str_clear (tok->name);

		do
		{ 
			GET_CHAR_TO (xli, c); 
			if (c == QSE_T('\n') || c == QSE_CHAR_EOF) break;

			if  (xli->opt.trait & QSE_XLI_KEEPTEXT) ADD_TOKEN_CHAR (xli, tok, c);
		}
		while (1);

		if (xli->opt.trait & QSE_XLI_KEEPTEXT)
		{
			qse_xli_text_t* ta;
			ta = qse_xli_inserttext(xli, xli->parlink->list, QSE_NULL, QSE_STR_PTR(tok->name));
			if (!ta) return -1;

			if (xli->tok_status & TOK_STATUS_DEINDENT_TEXT) ta->flags |= QSE_XLI_TEXT_DEINDENT;
		}

		GET_CHAR (xli); /* eat the new line letter */
		return 1; /* comment by # */
	}

	return 0; 
}

static qse_xli_tok_type_t classify_ident (qse_xli_t* xli, const qse_cstr_t* name)
{
	/* perform binary search */

	/* declaring left, right, mid to be the int type is ok
	 * because we know kwtab is small enough. */
	int left = 0, right = QSE_COUNTOF(kwtab) - 1, mid;

	while (left <= right)
	{
		int n;
		kwent_t* kwp;

		/*mid = (left + right) / 2;*/
		mid = left + (right - left) / 2;
		kwp = &kwtab[mid];

		/* not affected by case-insensitivity */
		n = qse_strxncmp (kwp->name.ptr, kwp->name.len, name->ptr, name->len);
		if (n > 0) 
		{
			/* if left, right, mid were of qse_size_t,
			 * you would need the following line. 
			if (mid == 0) break;
			 */
			right = mid - 1;
		}
		else if (n < 0) left = mid + 1;
		else return kwp->type;
	}

	return QSE_XLI_TOK_IDENT;
}

static int get_symbols (qse_xli_t* xli, qse_cint_t c, qse_xli_tok_t* tok)
{
	struct ops_t
	{
		const qse_char_t* str;
		qse_size_t len;
		int tid;
	};

	static struct ops_t ops[] = 
	{
		{ QSE_T(","),   1, QSE_XLI_TOK_COMMA       },
		{ QSE_T(":"),   1, QSE_XLI_TOK_COLON       },
		{ QSE_T(";"),   1, QSE_XLI_TOK_SEMICOLON   },
		{ QSE_T("{"),   1, QSE_XLI_TOK_LBRACE      },
		{ QSE_T("}"),   1, QSE_XLI_TOK_RBRACE      },
		{ QSE_T("["),   1, QSE_XLI_TOK_LBRACK      },
		{ QSE_T("]"),   1, QSE_XLI_TOK_RBRACK      },
		{ QSE_NULL,     0, 0,                      }
	};

	struct ops_t* p;
	int idx = 0;

	/* note that the loop below is not generaic enough.
	 * you must keep the operators strings in a particular order */

	for (p = ops; p->str != QSE_NULL; )
	{
		if (p->str[idx] == QSE_T('\0'))
		{
			ADD_TOKEN_STR (xli, tok, p->str, p->len);
			SET_TOKEN_TYPE (xli, tok, p->tid);
			return 1;
		}

		if (c == p->str[idx])
		{
			idx++;
			GET_CHAR_TO (xli, c);
			continue;
		}

		p++;
	}

	return 0;
}

static int end_include (qse_xli_t* xli, int noeof)
{
	int x;
	qse_xli_io_arg_t* cur;

	if (xli->rio.inp == &xli->rio.top) return 0; /* no include */


	/* if it is an included file, close it and
	 * retry to read a character from an outer file */

	x = xli->rio.impl (
		xli, QSE_XLI_IO_CLOSE, 
		xli->rio.inp, QSE_NULL, 0);

	/* if closing has failed, still destroy the
	 * sio structure first as normal and return
	 * the failure below. this way, the caller 
	 * does not call QSE_XLI_SIO_CLOSE on 
	 * xli->rio.inp again. */

	cur = xli->rio.inp;
	xli->rio.inp = xli->rio.inp->prev;

	QSE_ASSERT (cur->name != QSE_NULL);
	qse_xli_freemem (xli, cur);
	/* xli->parse.depth.incl--; */

	if ((xli->opt.trait & QSE_XLI_KEEPFILE) && !noeof && 
	    qse_xli_inserteof (xli, xli->parlink->list, QSE_NULL) == QSE_NULL) return -1;

	if (x != 0)
	{
		/* the failure mentioned above is returned here */
		if (xli->errnum == QSE_XLI_ENOERR)
			qse_xli_seterrnum (xli, QSE_XLI_EIOUSR, QSE_NULL); 
		return -1;
	}

	xli->rio.last = xli->rio.inp->last;
	return 1; /* ended the included file successfully */
}

static int begin_include (qse_xli_t* xli)
{
	qse_link_t* link;
	qse_xli_io_arg_t* arg = QSE_NULL;

	link = (qse_link_t*) qse_xli_callocmem (xli, 
		QSE_SIZEOF(*link) + QSE_SIZEOF(qse_char_t) * (QSE_STR_LEN(xli->tok.name) + 1));
	if (link == QSE_NULL) goto oops;

	qse_strncpy ((qse_char_t*)(link + 1), QSE_STR_PTR(xli->tok.name), QSE_STR_LEN(xli->tok.name));
	link->link = xli->rio_names;
	xli->rio_names = link;

	arg = (qse_xli_io_arg_t*) qse_xli_callocmem (xli, QSE_SIZEOF(*arg));
	if (arg == QSE_NULL) goto oops;

	arg->name = (const qse_char_t*)(link + 1);
	arg->line = 1;
	arg->colm = 1;

	/* let the argument's prev point field to the current */
	arg->prev = xli->rio.inp; 

	if (qse_xli_openrstream(xli, arg) <= -1) goto oops;

	/* i update the current pointer after opening is successful */
	xli->rio.inp = arg;
	/* xli->parse.depth.incl++; */

	/* read in the first character in the included file. 
	 * so the next call to get_token() sees the character read
	 * from this file. */
	if (qse_xli_getchar(xli) <= -1 || get_token(xli) <= -1) 
	{
		end_include (xli, 1); 
		/* i don't jump to oops since i've called 
		 * end_include() where xli->rio.inp/arg is freed. */
		return -1;
	}

	if ((xli->opt.trait & QSE_XLI_KEEPFILE) &&
	    qse_xli_insertfile(xli, xli->parlink->list, QSE_NULL, arg->name) == QSE_NULL) 
	{
		end_include (xli, 1);
		return -1;
	}

	return 0;

oops:
	/* i don't need to free 'link' since it's linked to
	 * xli->rio_names that's freed at the beginning of qse_xli_read()
	 * or by qse_xli_fini() */
	if (arg) qse_xli_freemem (xli, arg);
	return -1;
}

static int get_token_into (qse_xli_t* xli, qse_xli_tok_t* tok)
{
	qse_cint_t c;
	int n;

retry:
	do 
	{
		if (skip_spaces (xli) <= -1) return -1;
		if ((n = skip_comment (xli, tok)) <= -1) return -1; 
	} 
	while (n >= 1);

	qse_str_clear (tok->name);
	tok->loc.file = xli->rio.last.file;
	tok->loc.line = xli->rio.last.line;
	tok->loc.colm = xli->rio.last.colm;

	c = xli->rio.last.c;

	if (c == QSE_CHAR_EOF) 
	{
		n = end_include (xli, 0);
		if (n <= -1) return -1;
		if (n >= 1) 
		{
			/*xli->rio.last = xli->rio.inp->last;*/
			/* mark that i'm retrying after end of an included file */
			goto retry;
		}

		ADD_TOKEN_STR (xli, tok, QSE_T("<EOF>"), 5);
		SET_TOKEN_TYPE (xli, tok, QSE_XLI_TOK_EOF);
	}
	else if (c == QSE_T('@'))
	{
		/* keyword/directive - start with @ */

		qse_xli_tok_type_t type;

		ADD_TOKEN_CHAR (xli, tok, c);
		GET_CHAR_TO (xli, c);

		if (!QSE_ISALPHA(c))
		{
			/* this directive is empty, not followed by a valid word */
			qse_xli_seterror (xli, QSE_XLI_EXKWEM, QSE_NULL, &tok->loc);
			return -1;
		}

		/* expect an identifier starting with an alphabet. the identifier 
		 * forming a keyword/directory is composed of alphabets. */
		do 
		{
			ADD_TOKEN_CHAR (xli, tok, c);
			GET_CHAR_TO (xli, c);
		} 
		while (QSE_ISALPHA(c));

		type = classify_ident(xli, QSE_STR_XSTR(tok->name));
		if (type == QSE_XLI_TOK_IDENT)
		{
			/* this keyword/directive is not recognized */
			qse_xli_seterror (xli, QSE_XLI_EXKWNR, QSE_STR_XSTR(tok->name), &tok->loc);
			return -1;
		}
		SET_TOKEN_TYPE (xli, tok, type);
	}
	else if (c == QSE_T('_') || QSE_ISALPHA(c))
	{
		qse_xli_tok_type_t type;

		do
		{
			ADD_TOKEN_CHAR (xli, tok, c);
			GET_CHAR_TO (xli, c);
		}
		while (c == QSE_T('_') || QSE_ISALNUM (c));

		type = classify_ident(xli, QSE_STR_XSTR(tok->name));
		SET_TOKEN_TYPE (xli, tok, type);
	}
	/* TODO: negative number, floating-point number, etc */
	else if (c == QSE_T('-'))
	{
		GET_CHAR_TO (xli, c);
		if (!QSE_ISDIGIT(c))
		{
			qse_char_t cc = QSE_T('-');
			qse_cstr_t ea;
			ea.ptr = &cc;
			ea.len = 1;
			qse_xli_seterror (xli, QSE_XLI_ELXCHR, &ea, &tok->loc);
			return -1;
		}

		SET_TOKEN_TYPE (xli, tok, QSE_XLI_TOK_NSTR);
		ADD_TOKEN_CHAR (xli, tok, QSE_T('-'));
		do
		{
			ADD_TOKEN_CHAR (xli, tok, c);
			GET_CHAR_TO (xli, c);
		}
		while (QSE_ISDIGIT(c));
	}
	else if (QSE_ISDIGIT(c))
	{
		SET_TOKEN_TYPE (xli, tok, QSE_XLI_TOK_NSTR);
		do
		{
			ADD_TOKEN_CHAR (xli, tok, c);
			GET_CHAR_TO (xli, c);
		}
		while (QSE_ISDIGIT(c));
	}
	else if (c == QSE_T('\''))
	{
		/* single-quoted string - no escaping */
		SET_TOKEN_TYPE (xli, tok, QSE_XLI_TOK_SQSTR);

		while (1)
		{
			GET_CHAR_TO (xli, c);

			if (c == QSE_CHAR_EOF)
			{
				/* the string is not closed */
				qse_xli_seterror (xli, QSE_XLI_ESTRNC, QSE_NULL, &tok->loc);
				return -1;
			}

			if (c == QSE_T('\''))
			{
				/* terminating quote */
				GET_CHAR (xli);
				break;
			}

			ADD_TOKEN_CHAR (xli, tok, c);
		}
	}
	else if (c == QSE_T('\"'))
	{
		/* double-quoted string - support escaping */
		int escaped = 0;
		qse_size_t digit_count = 0;
		qse_uint32_t c_acc = 0;

		SET_TOKEN_TYPE (xli, tok, QSE_XLI_TOK_DQSTR);

		while (1)
		{
			GET_CHAR_TO (xli, c);

			if (c == QSE_CHAR_EOF)
			{
				/* the string is not closed */
				qse_xli_seterror (xli, QSE_XLI_ESTRNC, QSE_NULL, &tok->loc);
				return -1;
			}

			if (!escaped)
			{
			not_escaped:
				if (c == QSE_T('\\')) 
				{
					escaped = 1;
					continue;
				}

				if (c == QSE_T('\"'))
				{
					/* terminating quote */
					GET_CHAR (xli);
					break;
				}

				ADD_TOKEN_CHAR (xli, tok, c);
			}
			else if (escaped == 4)
			{
				if (c >= QSE_T('0') && c <= QSE_T('9'))
				{
					c_acc = c_acc * 16 + c - QSE_T('0');
					digit_count++;
					if (digit_count >= escaped) 
					{
						ADD_TOKEN_UINT32 (xli, tok, c_acc);
						escaped = 0;
					}
				}
				else if (c >= QSE_T('A') && c <= QSE_T('F'))
				{
					c_acc = c_acc * 16 + c - QSE_T('A') + 10;
					digit_count++;
					if (digit_count >= escaped) 
					{
						ADD_TOKEN_UINT32 (xli, tok, c_acc);
						escaped = 0;
					}
				}
				else if (c >= QSE_T('a') && c <= QSE_T('f'))
				{
					c_acc = c_acc * 16 + c - QSE_T('a') + 10;
					digit_count++;
					if (digit_count >= escaped) 
					{
						ADD_TOKEN_UINT32 (xli, tok, c_acc);
						escaped = 0;
					}
				}
				else
				{
					/* not a hexadecimal digit */

					if (digit_count == 0) 
					{
						/* no valid character after the escaper. keep the escaper as it is */
						ADD_TOKEN_CHAR (xli, tok, 'u');
					}
					else ADD_TOKEN_UINT32 (xli, tok, c_acc);

					escaped = 0;
					goto not_escaped;
				}
			}
			else
			{
				if (c == 'u') 
				{
					escaped = 4;
					digit_count = 0;
					c_acc = 0;
					continue;
				}

				if (c == 'b') c = '\b';
				else if (c == 'f') c = '\f';
				else if (c == 'n') c = '\n';
				else if (c == 'r') c = '\r';
				else if (c == 't') c = '\t';

				ADD_TOKEN_CHAR (xli, tok, c);
				escaped = 0;
			}
		}
	}
	else
	{
		n = get_symbols (xli, c, tok);
		if (n <= -1) return -1; /* hard failure */
		if (n == 0)
		{
			/* not handled yet */
			if (c == QSE_T('\0'))
			{
				qse_cstr_t ea;
				ea.ptr = QSE_T("<NUL>");
				ea.len = 5;
				qse_xli_seterror (xli, QSE_XLI_ELXCHR, &ea, &tok->loc);
			}
			else
			{
				qse_char_t cc = (qse_char_t)c;
				qse_cstr_t ea;
				ea.ptr = &cc;
				ea.len = 1;
				qse_xli_seterror (xli, QSE_XLI_ELXCHR, &ea, &tok->loc);
			}
			return -1;
		}
	}

	return 0;
}

static int get_token (qse_xli_t* xli)
{
	return get_token_into (xli, &xli->tok);
}

static int check_token_for_key_eligibility (qse_xli_t* xli, qse_xli_list_t* parlist)
{
	if (xli->opt.trait & QSE_XLI_KEYNODUP) 
	{
		qse_xli_atom_t* atom;

		/* find any key conflicts in the current scope */
		/* TODO: optimization. no sequential search */
		atom = parlist->tail;
		while (atom)
		{
			if (atom->type == QSE_XLI_PAIR &&
			    xli->opt.strcmp(((qse_xli_pair_t*)atom)->key, QSE_STR_PTR(xli->tok.name)) == 0)
			{
				qse_xli_seterror (xli, QSE_XLI_EEXIST, QSE_STR_XSTR(xli->tok.name), &xli->tok.loc);
				return -1;
			}

			atom = atom->prev;
		}
	}

	return 0;
}

static qse_xli_val_t* __read_value (qse_xli_t* xli)
{
	while (1)
	{
		if (MATCH(xli, QSE_XLI_TOK_XINCLUDE))
		{
			if (get_token(xli) <= -1) return QSE_NULL;

			if (!MATCH(xli,QSE_XLI_TOK_SQSTR) && !MATCH(xli,QSE_XLI_TOK_DQSTR))
			{
				qse_xli_seterror (xli, QSE_XLI_EINCLSTR, QSE_NULL, &xli->tok.loc);
				return QSE_NULL;
			}

			if (begin_include(xli) <= -1) return QSE_NULL;
		}
		else if (/*MATCH(xli, QSE_XLI_TOK_IDENT) || */MATCH(xli, QSE_XLI_TOK_DQSTR) || MATCH(xli, QSE_XLI_TOK_SQSTR) || MATCH(xli, QSE_XLI_TOK_NSTR))
		{
			qse_xli_str_t* sv;

			sv = qse_xli_makestrval(xli, QSE_STR_XSTR(xli->tok.name), QSE_NULL);
			if (!sv) return QSE_NULL;

			if (MATCH(xli, QSE_XLI_TOK_NSTR))
			{
				sv->flags |= QSE_XLI_STR_NSTR;
			}

			return (qse_xli_val_t*)sv;
		}
		else if (MATCH(xli, QSE_XLI_TOK_LBRACE))
		{
			qse_xli_list_t* lv;

			lv = qse_xli_makelistval (xli);
			if (!lv) return QSE_NULL;

			if (get_token(xli) <= -1 || read_list(xli, lv) <= -1)
			{
				qse_xli_freeval (xli, (qse_xli_val_t*)lv);
				return QSE_NULL;
			}

			return (qse_xli_val_t*)lv;
		}
		else if (MATCH(xli, QSE_XLI_TOK_LBRACK))
		{
			qse_xli_list_t* lv;

			lv = qse_xli_makelistval(xli);
			if (!lv) return QSE_NULL;

			if (get_token(xli) <= -1 || read_array(xli, lv) <= -1)
			{
				qse_xli_freeval (xli, (qse_xli_val_t*)lv);
				return QSE_NULL;
			}

			return (qse_xli_val_t*)lv;
		}
		else if (MATCH(xli, QSE_XLI_TOK_NIL))
		{
			return (qse_xli_val_t*)&xli->root->xnil;
		}
		else if (MATCH(xli, QSE_XLI_TOK_TRUE))
		{
			return (qse_xli_val_t*)&xli->root->xtrue;
		}
		else if (MATCH(xli, QSE_XLI_TOK_FALSE))
		{
			return (qse_xli_val_t*)&xli->root->xfalse;
		}
		else if (MATCH(xli, QSE_XLI_TOK_TEXT))
		{
			if (get_token(xli) <= -1) return QSE_NULL;
		}
		else 
		{
			break;
		}
	}

	qse_xli_seterror (xli, QSE_XLI_EVALUE, QSE_STR_XSTR(xli->tok.name), &xli->tok.loc);
	return QSE_NULL;
}

struct rpair_t
{
	qse_char_t* key;
	qse_xli_val_t* val;
};
typedef struct rpair_t rpair_t;

static int read_pair (qse_xli_t* xli, rpair_t* pair)
{
	qse_char_t* key = QSE_NULL;
	qse_xli_val_t* val = QSE_NULL;

	if (check_token_for_key_eligibility(xli, xli->parlink->list) <= -1) goto oops;

	key = qse_strdup(QSE_STR_PTR(xli->tok.name), qse_xli_getmmgr(xli));
	if (key == QSE_NULL) 
	{
		qse_xli_seterrnum (xli, QSE_XLI_ENOMEM, QSE_NULL); 
		goto oops;
	}

	if (get_token(xli) <= -1) goto oops;

	if (!MATCH(xli, QSE_XLI_TOK_COLON))
	{
		qse_xli_seterror (xli, QSE_XLI_ECOLON, QSE_STR_XSTR(xli->tok.name), &xli->tok.loc);
		goto oops;
	}

	if (get_token(xli) <= -1) goto oops; 

	val = __read_value(xli);
	if (!val) goto oops;

	pair->key = key;
	pair->val = val;

	return 0;

oops:
	if (val) qse_xli_freeval (xli, val);
	if (key) qse_xli_freemem (xli, key);
	return -1;
}

static int __read_array (qse_xli_t* xli)
{
	qse_xli_val_t* val;
	qse_size_t index = 0;
	qse_char_t key[64];

	if (MATCH(xli, QSE_XLI_TOK_RBRACK)) return 0; /* empty array */

	while (1)
	{
		qse_xli_pair_t* p;
		qse_xli_loc_t ploc;

		val = __read_value(xli);
		if (!val) return -1;

		ploc = xli->tok.loc;

		qse_strxfmt (key, QSE_COUNTOF(key), QSE_T("%zu"), index);
		p = qse_xli_insertpair (xli, xli->parlink->list, QSE_NULL, key, QSE_NULL, QSE_NULL, val);
		if (!p) return -1;
		if (xli->opt.cbs.pair_read) xli->opt.cbs.pair_read (xli, p, &ploc);
		index++;

		if (get_token(xli) <= -1) return -1;

		if (MATCH(xli, QSE_XLI_TOK_RBRACK)) break;
		if (!MATCH(xli, QSE_XLI_TOK_COMMA)) 
		{
			qse_xli_seterror (xli, QSE_XLI_ECOMMA, QSE_STR_XSTR(xli->tok.name), &xli->tok.loc);
			return -1;
		}

		if (get_token(xli) <= -1) return -1;
	}

	return 0;
}

static int read_array (qse_xli_t* xli, qse_xli_list_t* lv)
{
	qse_xli_list_link_t* ll;
	int n;

	ll = qse_xli_makelistlink (xli, lv);
	if (!ll) return -1;

	n = __read_array(xli);

	qse_xli_freelistlink (xli, ll);

	if (n <= -1) return -1;

	if (!MATCH(xli, QSE_XLI_TOK_RBRACK))
	{
		qse_xli_seterror (xli, QSE_XLI_ERBRACK, QSE_STR_XSTR(xli->tok.name), &xli->tok.loc);
		return -1;
	}

	lv->flags |= QSE_XLI_LIST_ARRAYED;
	return 0;
}

static int __read_list (qse_xli_t* xli)
{
	while (1)
	{
		if (MATCH(xli, QSE_XLI_TOK_XINCLUDE))
		{
			if (get_token(xli) <= -1) return -1;

			if (!MATCH(xli,QSE_XLI_TOK_SQSTR) && !MATCH(xli,QSE_XLI_TOK_DQSTR))
			{
				qse_xli_seterror (xli, QSE_XLI_EINCLSTR, QSE_NULL, &xli->tok.loc);
				return -1;
			}

			if (begin_include (xli) <= -1) return -1;
		}
		else if (/*MATCH(xli, QSE_XLI_TOK_IDENT) ||*/  MATCH(xli, QSE_XLI_TOK_DQSTR) || MATCH(xli, QSE_XLI_TOK_SQSTR))
		{
			rpair_t rpair;
			qse_xli_pair_t* p;
			qse_xli_loc_t ploc;

			ploc = xli->tok.loc;
			if (read_pair(xli, &rpair) <= -1) return -1;

			p = qse_xli_insertpair(xli, xli->parlink->list, QSE_NULL, rpair.key, QSE_NULL, QSE_NULL, rpair.val);
			if (!p)
			{
				qse_xli_freemem (xli, rpair.key);
				qse_xli_freeval (xli, rpair.val);
				return -1;
			}

			if (xli->opt.cbs.pair_read) xli->opt.cbs.pair_read (xli, p, &ploc);

			/* clear the duplicated key. the key is also duplicated in qse_xli_insertpair(). don't need it */
			qse_xli_freemem (xli, rpair.key);

			if (get_token(xli) <= -1) return -1;

			if (MATCH(xli, QSE_XLI_TOK_COMMA))
			{
				if (get_token(xli) <= -1) return -1;
				continue;
			}

			break;
		}
		else if (MATCH(xli, QSE_XLI_TOK_TEXT))
		{
			if (get_token(xli) <= -1) return -1;
		}
		else 
		{
			break;
		}
	}

	return 0;
}

static int read_list (qse_xli_t* xli, qse_xli_list_t* lv)
{
	qse_xli_list_link_t* ll;
	int n;

	ll = qse_xli_makelistlink (xli, lv);
	if (!ll) return -1;

	n = __read_list(xli);

	qse_xli_freelistlink (xli, ll);

	if (n <= -1) return -1;

	if (!MATCH(xli, QSE_XLI_TOK_RBRACE))
	{
		qse_xli_seterror (xli, QSE_XLI_ERBRACE, QSE_STR_XSTR(xli->tok.name), &xli->tok.loc);
		return -1;
	}

	return 0;
}

static int read_root_list (qse_xli_t* xli)
{
	qse_xli_list_link_t* link = QSE_NULL;

	link = qse_xli_makelistlink (xli, &xli->root->list);
	if (!link) goto oops;

	xli->tok_status |= TOK_STATUS_DEINDENT_TEXT;
	if (qse_xli_getchar(xli) <= -1 || get_token(xli) <= -1) goto oops;

	while (1)
	{
		/* NOTE: i don't support @include outside the outermost {} or [] */
		if (MATCH(xli, QSE_XLI_TOK_LBRACK))
		{
			qse_xli_text_t* ta;
			xli->root->list.flags |= QSE_XLI_LIST_ARRAYED; 
			ta = qse_xli_inserttext(xli, xli->parlink->list, QSE_NULL, QSE_STR_PTR(xli->tok.name));
			if (!ta) goto oops;
			ta->flags |= QSE_XLI_TEXT_VERBATIM | QSE_XLI_TEXT_DEINDENT | QSE_XLI_TEXT_ARRAYED_LIST_OPENER;
			xli->tok_status &= ~TOK_STATUS_DEINDENT_TEXT;
			if (get_token(xli) <= -1) goto oops;
			break;
		}
		else if (MATCH(xli, QSE_XLI_TOK_LBRACE))
		{
			qse_xli_text_t* ta;
			ta = qse_xli_inserttext(xli, xli->parlink->list, QSE_NULL, QSE_STR_PTR(xli->tok.name));
			if (!ta) goto oops;
			ta->flags |= QSE_XLI_TEXT_VERBATIM | QSE_XLI_TEXT_DEINDENT | QSE_XLI_TEXT_LIST_OPENER;
			xli->tok_status &= ~TOK_STATUS_DEINDENT_TEXT;
			if (get_token(xli) <= -1) goto oops;
			break;
		}
		else
		{
			qse_xli_seterror (xli, QSE_XLI_ELBRAC, QSE_STR_XSTR(xli->tok.name), &xli->tok.loc);
			goto oops;
		}
	}

	if (__read_list(xli) <= -1) goto oops;

	while (1)
	{
		/* NOTE: i don't support @include outside the outermost {} or [] */
		if (MATCH(xli, QSE_XLI_TOK_RBRACK))
		{
			qse_xli_text_t* ta;
			if (!(xli->root->list.flags & QSE_XLI_LIST_ARRAYED)) goto oops_rbrac;
			ta = qse_xli_inserttext(xli, xli->parlink->list, QSE_NULL, QSE_STR_PTR(xli->tok.name));
			if (!ta) goto oops;
			ta->flags |= QSE_XLI_TEXT_VERBATIM | QSE_XLI_TEXT_DEINDENT | QSE_XLI_TEXT_ARRAYED_LIST_CLOSER;
			xli->tok_status |= TOK_STATUS_DEINDENT_TEXT;
			if (get_token(xli) <= -1) goto oops;
			break;
		}
		else if (MATCH(xli, QSE_XLI_TOK_RBRACE))
		{
			qse_xli_text_t* ta;
			if (xli->root->list.flags & QSE_XLI_LIST_ARRAYED) goto oops_rbrac;
			ta = qse_xli_inserttext(xli, xli->parlink->list, QSE_NULL, QSE_STR_PTR(xli->tok.name));
			if (!ta) goto oops;
			ta->flags |= QSE_XLI_TEXT_VERBATIM | QSE_XLI_TEXT_DEINDENT | QSE_XLI_TEXT_LIST_CLOSER;
			xli->tok_status |= TOK_STATUS_DEINDENT_TEXT;
			if (get_token(xli) <= -1) goto oops;
			break;
		}
		else
		{
		oops_rbrac:
			qse_xli_seterror (xli,
				((xli->root->list.flags & QSE_XLI_LIST_ARRAYED)? QSE_XLI_ERBRACK: QSE_XLI_ERBRACE),
				QSE_STR_XSTR(xli->tok.name), &xli->tok.loc
			);
			goto oops;
		}
	}

	QSE_ASSERT (link == xli->parlink);
	qse_xli_freelistlink (xli, link);

	return 0;

oops:
	if (link) qse_xli_freelistlink (xli, link);
	return -1;
}

int qse_xli_readjson (qse_xli_t* xli, qse_xli_io_impl_t io)
{
	if (!io)
	{
		qse_xli_seterrnum (xli, QSE_XLI_EINVAL, QSE_NULL); 
		return -1;
	}

	QSE_MEMSET (&xli->rio, 0, QSE_SIZEOF(xli->rio));
	xli->rio.impl = io;
	xli->rio.top.line = 1;
	xli->rio.top.colm = 1;
	xli->rio.inp = &xli->rio.top;

	xli->tok_status = 0;

	qse_xli_seterrnum (xli, QSE_XLI_ENOERR, QSE_NULL); 
	qse_xli_clearrionames (xli);

	QSE_ASSERT (QSE_STR_LEN(xli->dotted_curkey) == 0);

	if (qse_xli_openrstream (xli, xli->rio.inp) <= -1) return -1;
	/* the input stream is open now */

	if (read_root_list (xli) <= -1) goto oops;

	QSE_ASSERT (xli->parlink == QSE_NULL);

	if (!MATCH (xli, QSE_XLI_TOK_EOF))
	{
		
		if (MATCH(xli, QSE_XLI_TOK_LBRACE) || MATCH(xli, QSE_XLI_TOK_LBRACK) || 
		    MATCH(xli, QSE_XLI_TOK_DQSTR) || MATCH(xli, QSE_XLI_TOK_SQSTR))
		{
			qse_xli_seterror (xli, QSE_XLI_ECOMMA, QSE_STR_XSTR(xli->tok.name), &xli->tok.loc);
		}
		else
		{
			qse_xli_seterror (xli, QSE_XLI_ESYNTAX, QSE_NULL, &xli->tok.loc);
		}
		goto oops;
	}

	QSE_ASSERT (xli->rio.inp == &xli->rio.top);
	qse_xli_closeactiverstream (xli);
	qse_str_clear (xli->tok.name);
	return 0;

oops:
	/* an error occurred and control has reached here
	 * probably, some included files might not have been 
	 * closed. close them */
	while (xli->rio.inp != &xli->rio.top)
	{
		qse_xli_io_arg_t* prev;

		/* nothing much to do about a close error */
		qse_xli_closeactiverstream (xli);

		prev = xli->rio.inp->prev;
		QSE_ASSERT (xli->rio.inp->name != QSE_NULL);
		qse_xli_freemem (xli, xli->rio.inp);
		xli->rio.inp = prev;
	}
	
	qse_xli_closeactiverstream (xli);
	qse_str_clear (xli->tok.name);
	return -1;
}
