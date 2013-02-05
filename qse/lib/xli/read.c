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

#if 0
static int open_stream (qse_xli_t* xli)
{
	qse_ssize_t n;

	xli->errnum = QSE_XLI_ENOERR;
	n = xli->src.fun (xli, QSE_XLI_IO_OPEN, &xli->src.arg, QSE_NULL, 0);
	if (n <= -1)
	{
		if (xli->errnum == QSE_XLI_ENOERR) xli->errnum = QSE_XLI_EIOUSR;
		return -1;
	}

	xli->src.cur = xli->src.buf;
	xli->src.end = xli->src.buf;
	xli->src.cc  = QSE_CHAR_EOF;
	xli->src.loc.line = 1;
	xli->src.loc.colm = 0;

	xli->src.eof = 0;
	return 0;
}

static int close_stream (qse_xli_t* xli)
{
	qse_ssize_t n;

	xli->errnum = QSE_XLI_ENOERR;
	n = xli->src.fun (xli, QSE_XLI_IO_CLOSE, &xli->src.arg, QSE_NULL, 0);
	if (n <= -1)
	{
		if (xli->errnum == QSE_XLI_ENOERR) xli->errnum = QSE_XLI_EIOUSR;
		return -1;
	}

	return 0;
}

static int read_stream (qse_xli_t* xli)
{
	qse_ssize_t n;

	xli->errnum = QSE_XLI_ENOERR;
	n = xli->src.fun (
		xli, QSE_XLI_IO_READ, &xli->src.arg, 
		xli->src.buf, QSE_COUNTOF(xli->src.buf)
	);
	if (n <= -1)
	{
		if (xli->errnum == QSE_XLI_ENOERR) xli->errnum = QSE_XLI_EIOUSR;
		return -1; /* error */
	}

	if (n == 0)
	{
		/* don't change xli->src.cur and xli->src.end.
		 * they remain the same on eof  */
		xli->src.eof = 1;
		return 0; /* eof */
	}

	xli->src.cur = xli->src.buf;
	xli->src.end = xli->src.buf + n;
	return 1; /* read something */
}

#endif

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
			xli->errnum = QSE_XLI_ENOMEM; \
			return -1; \
		} \
	} while (0)

#define ADD_TOKEN_STR(xli,tok,s,l) \
	do { \
		if (qse_str_ncat((tok)->name,(s),(l)) == (qse_size_t)-1) \
		{ \
			xli->errnum = QSE_XLI_ENOMEM; \
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
		xli->errnum = QSE_XLI_ENOERR;
		n = xli->sio.inf (
			xli, QSE_XLI_IO_READ, xli->sio.inp,
			xli->sio.inp->b.buf, QSE_COUNTOF(xli->sio.inp->b.buf)
		);
		if (n <= -1)
		{
			if (xli->errnum == QSE_XLI_ENOERR)
				xli->errnum = QSE_XLI_EIOUSR;
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

		return kwp->type;
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
#if 0
		n = end_include (xli);
		if (n <= -1) return -1;
		if (n >= 1) 
		{
			/*xli->sio.last = xli->sio.inp->last;*/
			/* mark that i'm retrying after end of an included file */
			skip_semicolon_after_include = 1; 
			goto retry;
		}
#endif

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
		qse_cint_t sc = c;

		SET_TOKEN_TYPE (xli, tok, 
			((sc == QSE_T('\''))? TOK_SQSTR: TOK_DQSTR));

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

			if (c == sc)
			{
				/* terminating quote */
				GET_CHAR (xli);
				break;
			}

			ADD_TOKEN_CHAR (xli, tok, c);
		}
		return 0;
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

int qse_xli_read (qse_xli_t* xli, qse_xli_io_impl_t io)
{
	if (io == QSE_NULL)
	{
		xli->errnum = QSE_XLI_EINVAL;
		return -1;
	}

	xli->sio.inf = io;
#if 0
	if (open_stream (xli) <= -1) return -1;

	close_stream (xli);
#endif

	do
	{
		if (get_token (xli) <= -1) return -1;
		if (MATCH (xli, TOK_XINCLUDE))
		{
		}
		else if (MATCH (xli, TOK_IDENT))
		{
		}
		else if (MATCH (xli, TOK_TEXT))
		{
		}
		else
		{
		}
	}
	while (1);

	return 0;
}
