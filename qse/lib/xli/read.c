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

#include "xli.h"
#include <qse/cmn/chr.h>

static int get_char (qse_xli_t* xli);
static int get_token (qse_xli_t* xli);
static int read_list (qse_xli_t* xli, qse_xli_list_t* list);

static int close_stream (qse_xli_t* xli)
{
	qse_ssize_t n;

	n = xli->sio.inf (xli, QSE_XLI_IO_CLOSE, xli->sio.inp, QSE_NULL, 0);
	if (n <= -1)
	{
		if (xli->errnum == QSE_XLI_ENOERR) 
			qse_xli_seterrnum (xli, QSE_XLI_EIOUSR, QSE_NULL);
		return -1;
	}

	return 0;
}

enum tok_t
{
	TOK_EOF,
	TOK_XINCLUDE,
	TOK_SEMICOLON,
	TOK_LBRACE,
	TOK_RBRACE,
	TOK_EQ,
	TOK_DQSTR,
	TOK_SQSTR,
	TOK_IDENT,
	TOK_TEXT,

	__TOKEN_COUNT__
};

#define GET_CHAR(xli) \
	do { if (get_char(xli) <= -1) return -1; } while(0)

#define GET_CHAR_TO(xli,c) \
	do { \
		if (get_char(xli) <= -1) return -1; \
		c = (xli)->sio.last.c; \
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

#define SET_TOKEN_TYPE(xli,tok,code) \
	do { (tok)->type = (code); } while (0)

#define MATCH(xli,tok_type) ((xli)->tok.type == (tok_type))

typedef struct kwent_t kwent_t;

struct kwent_t 
{ 
	qse_cstr_t name;
	int type; 
};

static kwent_t kwtab[] = 
{
	/* keep it sorted by the first field for binary search */
	{ { QSE_T("@include"),     8 }, TOK_XINCLUDE }
};

static int get_char (qse_xli_t* xli)
{
	qse_ssize_t n;

	if (xli->sio.inp->b.pos >= xli->sio.inp->b.len)
	{
		n = xli->sio.inf (
			xli, QSE_XLI_IO_READ, xli->sio.inp,
			xli->sio.inp->b.buf, QSE_COUNTOF(xli->sio.inp->b.buf)
		);
		if (n <= -1)
		{
			if (xli->errnum == QSE_XLI_ENOERR)
				qse_xli_seterrnum (xli, QSE_XLI_EIOUSR, QSE_NULL); 
			return -1;
		}

		if (n == 0)
		{
			xli->sio.last.c = QSE_CHAR_EOF;
			xli->sio.last.line = xli->sio.inp->line;
			xli->sio.last.colm = xli->sio.inp->colm;
			xli->sio.last.file = xli->sio.inp->name;
			return 0;
		}

		xli->sio.inp->b.pos = 0;
		xli->sio.inp->b.len = n;	
	}

	if (xli->sio.inp->last.c == QSE_T('\n'))
	{
		/* if the previous charater was a newline,
		 * increment the line counter and reset column to 1.
		 * incrementing it line number here instead of
		 * updating inp->last causes the line number for
		 * TOK_EOF to be the same line as the last newline. */
		xli->sio.inp->line++;
		xli->sio.inp->colm = 1;
	}
	
	xli->sio.inp->last.c = xli->sio.inp->b.buf[xli->sio.inp->b.pos++];
	xli->sio.inp->last.line = xli->sio.inp->line;
	xli->sio.inp->last.colm = xli->sio.inp->colm++;
	xli->sio.inp->last.file = xli->sio.inp->name;

	xli->sio.last = xli->sio.inp->last;
	return 0;
}

static int skip_spaces (qse_xli_t* xli)
{
	qse_cint_t c = xli->sio.last.c;
	while (QSE_ISSPACE(c)) GET_CHAR_TO (xli, c);
	return 0;
}

static int skip_comment (qse_xli_t* xli)
{
	qse_cint_t c = xli->sio.last.c;

	if (c == QSE_T('#'))
	{
		
		/* skip up to \n */
		/* TODO: support a different line terminator */
		do { GET_CHAR_TO (xli, c); }
		while (c != QSE_T('\n') && c != QSE_CHAR_EOF);
		GET_CHAR (xli); /* eat the new line letter */
		return 1; /* comment by # */
	}

	return 0; 
}

static int classify_ident (qse_xli_t* xli, const qse_cstr_t* name)
{
	/* perform binary search */

	/* declaring left, right, mid to be the int type is ok
	 * because we know kwtab is small enough. */
	int left = 0, right = QSE_COUNTOF(kwtab) - 1, mid;

	while (left <= right)
	{
		int n;
		kwent_t* kwp;

		mid = (left + right) / 2;	
		kwp = &kwtab[mid];

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

	return TOK_IDENT;
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
		{ QSE_T("="),   1, TOK_EQ          },
		{ QSE_T(";"),   1, TOK_SEMICOLON   },
		{ QSE_T("{"),   1, TOK_LBRACE      },
		{ QSE_T("}"),   1, TOK_RBRACE      },
		{ QSE_NULL,     0, 0,              }
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

static int end_include (qse_xli_t* xli)
{
	int x;
	qse_xli_io_arg_t* cur;

	if (xli->sio.inp == &xli->sio.arg) return 0; /* no include */

	/* if it is an included file, close it and
	 * retry to read a character from an outer file */

	x = xli->sio.inf (
		xli, QSE_XLI_IO_CLOSE, 
		xli->sio.inp, QSE_NULL, 0);

	/* if closing has failed, still destroy the
	 * sio structure first as normal and return
	 * the failure below. this way, the caller 
	 * does not call QSE_XLI_SIO_CLOSE on 
	 * xli->sio.inp again. */

	cur = xli->sio.inp;
	xli->sio.inp = xli->sio.inp->next;

	QSE_ASSERT (cur->name != QSE_NULL);
	QSE_MMGR_FREE (xli->mmgr, cur);
	/* xli->parse.depth.incl--; */

	if (x != 0)
	{
		/* the failure mentioned above is returned here */
		if (xli->errnum == QSE_XLI_ENOERR)
			qse_xli_seterrnum (xli, QSE_XLI_EIOUSR, QSE_NULL); 
		return -1;
	}

	xli->sio.last = xli->sio.inp->last;
	return 1; /* ended the included file successfully */
}

static int begin_include (qse_xli_t* xli)
{
	qse_ssize_t op;
	qse_xli_io_arg_t* arg = QSE_NULL;
	qse_htb_pair_t* pair = QSE_NULL;

	/* store the file name to xli->sio_names */
	pair = qse_htb_ensert (
		xli->sio_names, 
		QSE_STR_PTR(xli->tok.name),
		QSE_STR_LEN(xli->tok.name) + 1, /* to include '\0' */
		QSE_NULL, 0
	);
	if (pair == QSE_NULL)
	{
#if 0
		SETERR_LOC (xli, QSE_XLI_ENOMEM, &xli->ptok.loc);
#endif
		goto oops;
	}

	/*QSE_HTB_VPTR(pair) = QSE_HTB_KPTR(pair);
	QSE_HTB_VLEN(pair) = QSE_HTB_KLEN(pair);*/

	arg = (qse_xli_io_arg_t*) qse_xli_callocmem (xli, QSE_SIZEOF(*arg));
	if (arg == QSE_NULL)
	{
#if 0
		ADJERR_LOC (xli, &xli->ptok.loc);
#endif
		goto oops;
	}

	arg->flags = QSE_XLI_IO_INCLUDED;
	arg->name = QSE_HTB_KPTR(pair);
	arg->line = 1;
	arg->colm = 1;

	op = xli->sio.inf (xli, QSE_XLI_IO_OPEN, arg, QSE_NULL, 0);
	if (op <= -1)
	{
		if (xli->errnum == QSE_XLI_ENOERR)
			qse_xli_seterrnum (xli, QSE_XLI_EIOUSR, QSE_NULL); 
		goto oops;
	}

	arg->next = xli->sio.inp;
	xli->sio.inp = arg;
	/* xli->parse.depth.incl++; */

	/* read in the first character in the included file. 
	 * so the next call to get_token() sees the character read
	 * from this file. */
	if (get_char (xli) <= -1 || get_token (xli) <= -1) 
	{
		end_include (xli); 
		/* i don't jump to oops since i've called 
		 * end_include() where xli->sio.inp/arg is freed. */
		return -1;
	}

	return 0;

oops:
	if (arg) QSE_MMGR_FREE (xli->mmgr, arg);
	return -1;
}


static int get_token_into (qse_xli_t* xli, qse_xli_tok_t* tok)
{
	qse_cint_t c;
	int n;
	int skip_semicolon_after_include = 0;

retry:
	do 
	{
		if (skip_spaces(xli) <= -1) return -1;
		if ((n = skip_comment(xli)) <= -1) return -1; 
	} 
	while (n >= 1);

	qse_str_clear (tok->name);
	tok->loc.file = xli->sio.last.file;
	tok->loc.line = xli->sio.last.line;
	tok->loc.colm = xli->sio.last.colm;

	c = xli->sio.last.c;

	if (c == QSE_CHAR_EOF) 
	{
		n = end_include (xli);
		if (n <= -1) return -1;
		if (n >= 1) 
		{
			/*xli->sio.last = xli->sio.inp->last;*/
			/* mark that i'm retrying after end of an included file */
			skip_semicolon_after_include = 1; 
			goto retry;
		}

		ADD_TOKEN_STR (xli, tok, QSE_T("<EOF>"), 5);
		SET_TOKEN_TYPE (xli, tok, TOK_EOF);
	}	
	else if (c == QSE_T('@'))
	{
		int type;

		ADD_TOKEN_CHAR (xli, tok, c);
		GET_CHAR_TO (xli, c);

		if (c != QSE_T('_') && !QSE_ISALPHA (c))
		{
			/* this directive is empty, 
			 * not followed by a valid word */
#if  0
			SETERR_LOC (xli, QSE_XLI_EXKWEM, &(xli)->tok.loc);
#endif
			return -1;
		}

		/* expect normal identifier starting with an alphabet */
		do 
		{
			ADD_TOKEN_CHAR (xli, tok, c);
			GET_CHAR_TO (xli, c);
		} 
		while (c == QSE_T('_') || c == QSE_T('-') || QSE_ISALNUM (c));

		type = classify_ident (xli, QSE_STR_CSTR(tok->name));
		if (type == TOK_IDENT)
		{
			/* this directive is not recognized */
#if 0
			SETERR_TOK (xli, QSE_XLI_EXKWNR);
#endif
			return -1;
		}
		SET_TOKEN_TYPE (xli, tok, type);
	}
	else if (c == QSE_T('_') || QSE_ISALPHA (c))
	{
		int type;

		/* identifier */
		do 
		{
			ADD_TOKEN_CHAR (xli, tok, c);
			GET_CHAR_TO (xli, c);
		} 
		while (c == QSE_T('_') || c == QSE_T('-') || QSE_ISALNUM (c));

		type = classify_ident (xli, QSE_STR_CSTR(tok->name));
		SET_TOKEN_TYPE (xli, tok, type);
	}
	else if (c == QSE_T('\'') || c == QSE_T('\"'))
	{
		/* single-quoted string - no escaping */
		qse_cint_t cc = c;

		SET_TOKEN_TYPE (xli, tok, ((cc == QSE_T('\''))? TOK_SQSTR: TOK_DQSTR));

		while (1)
		{
			GET_CHAR_TO (xli, c);

			if (c == QSE_CHAR_EOF)
			{
				/* the string is not closed */
#if 0
				SETERR_TOK (xli, QSE_XLI_ESTRNC);
#endif
				return -1;
			}

			if (c == cc)
			{
				/* terminating quote */
				GET_CHAR (xli);
				break;
			}

			ADD_TOKEN_CHAR (xli, tok, c);
		}
	}
	else
	{
		n = get_symbols (xli, c, tok);
		if (n <= -1) return -1;
		if (n == 0)
		{
			/* not handled yet */
			if (c == QSE_T('\0'))
			{
#if 0
				SETERR_ARG_LOC (
					xli, QSE_XLI_ELXCHR,
					QSE_T("<NUL>"), 5, &tok->loc);
#endif
			}
			else
			{
#if 0
				qse_char_t cc = (qse_char_t)c;
				SETERR_ARG_LOC (xli, QSE_XLI_ELXCHR, &cc, 1, &tok->loc);
#endif
			}
			return -1;
		}

		if (skip_semicolon_after_include && tok->type == TOK_SEMICOLON)
		{
			/* this handles the optional semicolon after the 
			 * included file named as in @include "file-name"; */
			skip_semicolon_after_include = 0;
			goto retry;
		}
	}

	if (skip_semicolon_after_include)
	{
		/* semiclon has not been skipped yet */
#if 0
		qse_xli_seterror (xli, QSE_XLI_ESCOLON, QSE_STR_CSTR(tok->name), &tok->loc);
#endif
		return -1;
	}

	return 0;
}

static int get_token (qse_xli_t* xli)
{
	return get_token_into (xli, &xli->tok);
}

static int read_pair (qse_xli_t* xli, qse_xli_list_t* list)
{
	qse_char_t* key = QSE_NULL;
	qse_char_t* name = QSE_NULL;
	qse_xli_pair_t* pair;

	if (xli->opt.trait & QSE_XLI_NODUPKEY)
	{
		qse_xli_atom_t* atom;

		/* find any key conflicts in the current scope */
		atom = list->tail;
		while (atom)
		{
			if (atom->type == QSE_XLI_PAIR &&
			    qse_strcmp (((qse_xli_pair_t*)atom)->key, QSE_STR_PTR(xli->tok.name)) == 0)
			{
				qse_xli_seterror (xli, QSE_XLI_EEXIST, QSE_STR_CSTR(xli->tok.name), &xli->tok.loc);
				goto oops;
			}

			atom = atom->prev;
		}
	}

	key = qse_strdup (QSE_STR_PTR(xli->tok.name), xli->mmgr);
	if (key == QSE_NULL) 
	{
		qse_xli_seterrnum (xli, QSE_XLI_ENOMEM, QSE_NULL); 
		goto oops;
	}

	if (get_token (xli) <= -1) goto oops;

	if  (xli->opt.trait & QSE_XLI_NAMEDKEY)
	{
		/* the name part must be unique for the same key(s) */
		if (MATCH (xli, TOK_SQSTR) || MATCH(xli, TOK_DQSTR))
		{
			qse_xli_atom_t* atom;

			atom = list->tail;
			while (atom)
			{
				if (atom->type == QSE_XLI_PAIR &&
				    ((qse_xli_pair_t*)atom)->name && 
				    qse_strcmp (((qse_xli_pair_t*)atom)->key, key) == 0 &&
				    qse_strcmp (((qse_xli_pair_t*)atom)->name, QSE_STR_PTR(xli->tok.name)) == 0)
				{
					qse_xli_seterror (xli, QSE_XLI_EEXIST, QSE_STR_CSTR(xli->tok.name), &xli->tok.loc);
					goto oops;
				}
				atom = atom->prev;
			}

			name = qse_strdup (QSE_STR_PTR(xli->tok.name), xli->mmgr);
			if (name == QSE_NULL) 
			{
				qse_xli_seterrnum (xli, QSE_XLI_ENOMEM, QSE_NULL); 
				goto oops;
			}

			if (get_token (xli) <= -1) goto oops;
		}
	}

	if (MATCH (xli, TOK_EQ))
	{
		if (get_token (xli) <= -1) goto oops;

		if (MATCH (xli, TOK_SQSTR) || MATCH (xli, TOK_DQSTR))
		{
			pair = qse_xli_insertpairwithstr (
				xli, list, QSE_NULL, key, name, 
				QSE_STR_PTR(xli->tok.name), MATCH (xli, TOK_SQSTR));
			if (pair == QSE_NULL) goto oops;

			if (get_token (xli) <= -1) goto oops;

			/* semicolon is mandatory for a string */
			if (!MATCH (xli, TOK_SEMICOLON))
			{
				qse_xli_seterror (xli, QSE_XLI_ESCOLON, QSE_STR_CSTR(xli->tok.name), &xli->tok.loc);
				goto oops;
			}

			if (get_token (xli) <= -1) goto oops;
		}
		else
		{
			qse_xli_seterror (xli, QSE_XLI_EPAVAL, QSE_STR_CSTR(xli->tok.name), &xli->tok.loc);
			goto oops;	
		}
	}
	else if (MATCH (xli, TOK_LBRACE))
	{
		if (get_token (xli) <= -1) goto oops;

		/* insert a pair with an empty list */
		pair = qse_xli_insertpairwithemptylist (xli, list, QSE_NULL, key, name);
		if (pair == QSE_NULL) goto oops;
	
		if (read_list (xli, (qse_xli_list_t*)pair->val) <= -1) goto oops;
		
		if (!MATCH (xli, TOK_RBRACE))
		{
			qse_xli_seterror (xli, QSE_XLI_ERBRCE, QSE_STR_CSTR(xli->tok.name), &xli->tok.loc);
			goto oops;
		}

		if (get_token (xli) <= -1) goto oops;

		/* semicolon is optional for a list */
		if (MATCH (xli, TOK_SEMICOLON))
		{
			/* skip the semicolon */
			if (get_token (xli) <= -1) goto oops;
		}
	}
	else
	{
		qse_xli_seterror (xli, QSE_XLI_ELBREQ, QSE_STR_CSTR(xli->tok.name), &xli->tok.loc);
		goto oops;	
	}

	QSE_MMGR_FREE (xli->mmgr, name);
	QSE_MMGR_FREE (xli->mmgr, key);
	return 0;
	
oops:
	if (name) QSE_MMGR_FREE (xli->mmgr, name);
	if (key) QSE_MMGR_FREE (xli->mmgr, key);
	return -1;
}

static int read_list (qse_xli_t* xli, qse_xli_list_t* list)
{
	while (1)
	{
		if (MATCH (xli, TOK_XINCLUDE))
		{
			if (get_token(xli) <= -1) goto oops;

			if (!MATCH(xli,TOK_SQSTR) && !MATCH(xli,TOK_DQSTR))
			{
#if 0
				SETERR_LOC (xli, QSE_XLI_EINCLSTR, &xli->ptok.loc);
#endif
				return -1;
			}

			if (begin_include (xli) <= -1) goto oops;
		}
		else if (MATCH (xli, TOK_IDENT))
		{
			if (read_pair (xli, list) <= -1) goto oops;
		}
		else if (MATCH (xli, TOK_TEXT))
		{
			if (get_token(xli) <= -1) goto oops;
		}
		else break;
	}

	return 0;

oops:
	return -1;
}

int qse_xli_read (qse_xli_t* xli, qse_xli_io_impl_t io)
{
	qse_ssize_t n;

	if (io == QSE_NULL)
	{
		qse_xli_seterrnum (xli, QSE_XLI_EINVAL, QSE_NULL); 
		return -1;
	}

	QSE_MEMSET (&xli->sio, 0, QSE_SIZEOF(xli->sio));
	xli->sio.inf = io;
	xli->sio.arg.line = 1;
	xli->sio.arg.colm = 1;
	xli->sio.inp = &xli->sio.arg;
	qse_htb_clear (xli->sio_names);

	n = xli->sio.inf (xli, QSE_XLI_IO_OPEN, xli->sio.inp, QSE_NULL, 0);
	if (n <= -1)
	{
		if (xli->errnum == QSE_XLI_ENOERR)
			qse_xli_seterrnum (xli, QSE_XLI_EIOUSR, QSE_NULL); 
		return -1;
	}
	/* the input stream is open now */

	if (get_char (xli) <= -1 || get_token (xli) <= -1) goto oops;
	if (read_list (xli, &xli->root) <= -1) goto oops;

	if (!MATCH (xli, TOK_EOF))
	{
/* TODO: set erro code */
qse_printf (QSE_T("NOT ENDING WITH EOF... %s\n"), QSE_STR_PTR(xli->tok.name));
		goto oops;
	}

	QSE_ASSERT (xli->sio.inp == &xli->sio.arg);
	close_stream (xli);
	return 0;

oops:
	/* an error occurred and control has reached here
	 * probably, some included files might not have been 
	 * closed. close them */
	while (xli->sio.inp != &xli->sio.arg)
	{
		qse_xli_io_arg_t* next;

		/* nothing much to do about a close error */
		close_stream (xli);

		next = xli->sio.inp->next;
		QSE_ASSERT (xli->sio.inp->name != QSE_NULL);
		QSE_MMGR_FREE (xli->mmgr, xli->sio.inp);
		xli->sio.inp = next;
	}
	
	close_stream (xli);
	return -1;
}
