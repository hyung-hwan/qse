/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

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
 * It reads key-value pairs under sections as shown below:
 * 
 * [SECTION1]
 * key1 = value1
 * key2 = value2
 * [SECTION2]
 * key1 = value1
 *
 * The above can get translated to the native XLI format shown below:
 *
 * SECTION1 {
 *   key1 = "value1";
 *   key2 = "value2";
 * }
 * SECTION2 {
 *   key1 = "value1";
 * }
 *
 * The ini-format reader doesn't support file inclusion via @include.
 */

enum
{
	TOK_STATUS_SAME_LINE = (1 << 0),
	TOK_STATUS_UPTO_EOL = (1 << 1)
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

#define SET_TOKEN_TYPE(xli,tok,code) \
	do { (tok)->type = (code); } while (0)

#define MATCH(xli,tok_type) ((xli)->tok.type == (tok_type))
#define MATCH(xli,tok_type) ((xli)->tok.type == (tok_type))

static int skip_spaces (qse_xli_t* xli)
{
	qse_cint_t c = xli->rio.last.c;
	if (xli->tok_status & TOK_STATUS_SAME_LINE)
	{
		while (QSE_ISSPACE(c) && c != QSE_T('\n')) GET_CHAR_TO (xli, c);
	}
	else
	{
		while (QSE_ISSPACE(c)) GET_CHAR_TO (xli, c);
	}

	return 0;
}

static int skip_comment (qse_xli_t* xli, qse_xli_tok_t* tok)
{
	qse_cint_t c = xli->rio.last.c;

	if (c == QSE_T(';'))
	{
		/* skip up to \n */
		qse_str_clear (tok->name);

		do
		{ 
			GET_CHAR_TO (xli, c); 
			if (c == QSE_T('\n') || c == QSE_CHAR_EOF) break;

			if (xli->opt.trait & QSE_XLI_KEEPTEXT) ADD_TOKEN_CHAR (xli, tok, c);
		}
		while (1);

		if ((xli->opt.trait & QSE_XLI_KEEPTEXT) && 
		    qse_xli_inserttext (xli,  xli->parlink->list, QSE_NULL, QSE_STR_PTR(tok->name)) == QSE_NULL) return -1;

		GET_CHAR (xli); /* eat the new line letter */
		return 1; /* comment by ; */
	}

	return 0; 
}

static int get_token_into (qse_xli_t* xli, qse_xli_tok_t* tok)
{
	qse_cint_t c;
	int n;

/*retry:*/
	do 
	{
		if (skip_spaces (xli) <= -1) return -1;
		if (!(xli->tok_status & TOK_STATUS_UPTO_EOL))
		{
			if ((n = skip_comment (xli, tok)) <= -1) return -1; 
		}
		else n = 0;
	} 
	while (n >= 1);

	qse_str_clear (tok->name);
	tok->loc.file = xli->rio.last.file;
	tok->loc.line = xli->rio.last.line;
	tok->loc.colm = xli->rio.last.colm;

	c = xli->rio.last.c;

	if (c == QSE_CHAR_EOF) 
	{
		ADD_TOKEN_STR (xli, tok, QSE_T("<EOF>"), 5);
		SET_TOKEN_TYPE (xli, tok, QSE_XLI_TOK_EOF); 
	}
	else if (xli->tok_status & TOK_STATUS_UPTO_EOL)
	{
		qse_size_t xlen = 0;

		SET_TOKEN_TYPE (xli, tok, QSE_XLI_TOK_SQSTR);

		do
		{
			if (c == QSE_CHAR_EOF || c == QSE_T(';')) break;
			if (c == QSE_T('\n')) 
			{
				GET_CHAR (xli);
				break;
			}

			ADD_TOKEN_CHAR (xli, tok, c);
			if (!QSE_ISSPACE(c)) xlen = QSE_STR_LEN(tok->name);

			GET_CHAR_TO (xli, c);
		}
		while (1);

		/* trim away trailing spaces */
		qse_str_setlen (tok->name, xlen);
	}
	else if (c == QSE_T('['))
	{
		/* in the ini-styled format, a tag is used as a section name.
		 * but the kinds of allowed charaters are more limited than
		 * a normal tag in the xli format. */
		SET_TOKEN_TYPE (xli, tok, QSE_XLI_TOK_TAG);

		while (1)
		{
			GET_CHAR_TO (xli, c);

			if (c == QSE_CHAR_EOF || c == QSE_T('\n'))
			{
				/* the string tag is not closed */
				qse_xli_seterror (xli, QSE_XLI_ETAGNC, QSE_NULL, &xli->tok.loc);
				return -1;
			}

			if (c == QSE_T(']'))
			{
				/* terminating quote */
				GET_CHAR (xli);
				break;
			}

			if (!QSE_ISALNUM(c) && c != QSE_T('-') && c != QSE_T('_') && c != QSE_T(':'))
			{
				qse_char_t cc = (qse_char_t)c;
				qse_cstr_t ea;
				ea.ptr = &cc;
				ea.len = 1;
				qse_xli_seterror (xli, QSE_XLI_ETAGCHR, &ea, &tok->loc);
				return -1;
			}

			ADD_TOKEN_CHAR (xli, tok, c);
		}
	}
	else if (c == QSE_T('_') || QSE_ISALPHA(c) || 
	         ((xli->opt.trait & QSE_XLI_LEADDIGIT) && QSE_ISDIGIT(c)))
	{
		int lead_digit = QSE_ISDIGIT(c);
		int all_digits = 1;

		/* a normal identifier can be composed of wider varieties of 
		 * characters than a keyword/directive */
		while (1)
		{
			ADD_TOKEN_CHAR (xli, tok, c);
			GET_CHAR_TO (xli, c);

			if (c == QSE_T('_') || c == QSE_T('-') || 
			    c == QSE_T(':') || c == QSE_T('*') ||
			    c == QSE_T('/') || QSE_ISALPHA(c)) 
			{
				all_digits = 0;
			}
			else if (QSE_ISDIGIT(c)) 
			{
				/* nothing to do */
			}
			else break;
		} 

		if (lead_digit && all_digits)
		{
			/* if an identifier begins with a digit, it must contain a non-digits character */
			qse_xli_seterror (xli, QSE_XLI_EIDENT, QSE_STR_XSTR(tok->name), &tok->loc);
			return -1;
		}

		SET_TOKEN_TYPE (xli, tok, QSE_XLI_TOK_IDENT);
	}
	else if (c == QSE_T('='))
	{
		SET_TOKEN_TYPE (xli, tok, QSE_XLI_TOK_EQ);
		ADD_TOKEN_CHAR (xli, tok, c);
		GET_CHAR (xli);
	}
	else
	{
		if ((xli->tok_status & TOK_STATUS_SAME_LINE) && c == QSE_T('\n'))
		{
			SET_TOKEN_TYPE (xli, tok, QSE_XLI_TOK_NL);
			ADD_TOKEN_STR (xli, tok, QSE_T("<NL>"), 4);
			GET_CHAR (xli);
		}
		/* not handled yet */
		else if (c == QSE_T('\0'))
		{
			qse_cstr_t ea;
			ea.ptr = QSE_T("<NUL>");
			ea.len = 5;
			qse_xli_seterror (xli, QSE_XLI_ELXCHR, &ea, &tok->loc);
			return -1;
		}
		else
		{
			qse_char_t cc = (qse_char_t)c;
			qse_cstr_t ea;
			ea.ptr = &cc;
			ea.len = 1;
			qse_xli_seterror (xli, QSE_XLI_ELXCHR, &ea, &tok->loc);
			return -1;
		}
	}

	return 0;
}

static int get_token (qse_xli_t* xli)
{
	return get_token_into (xli, &xli->tok);
}

static int read_list (qse_xli_t* xli)
{
	qse_xli_pair_t* pair;
	qse_cstr_t key;
	qse_xli_list_t* curlist;

	key.ptr = QSE_NULL;
	key.len = 0;

	while (1)
	{
		if (MATCH(xli, QSE_XLI_TOK_EOF)) break;

		if (MATCH(xli, QSE_XLI_TOK_TAG))
		{
			/* insert a pair with an empty list */
			pair = qse_xli_insertpairwithemptylist (xli, &xli->root->list, QSE_NULL, QSE_STR_PTR(xli->tok.name), QSE_NULL, QSE_NULL);
			if (pair == QSE_NULL) goto oops;

			curlist = (qse_xli_list_t*)pair->val;
			xli->parlink->list = curlist;

			while (1)
			{
				if (get_token (xli) <= -1) goto oops;

				if (MATCH(xli, QSE_XLI_TOK_EOF)) break;
				if (MATCH(xli, QSE_XLI_TOK_TAG)) 
				{
					/* switch to a new tag */
					pair = qse_xli_insertpairwithemptylist (xli, &xli->root->list, QSE_NULL, QSE_STR_PTR(xli->tok.name), QSE_NULL, QSE_NULL);
					if (pair == QSE_NULL) goto oops;

					curlist = (qse_xli_list_t*)pair->val;
					xli->parlink->list = curlist;
					continue;
				}

				if (!MATCH(xli, QSE_XLI_TOK_IDENT))
				{
					qse_xli_seterror (xli, QSE_XLI_EKEY, QSE_STR_XSTR(xli->tok.name), &xli->tok.loc);
					goto oops;
				}

				if (xli->opt.trait & QSE_XLI_KEYNODUP)
				{
					qse_xli_atom_t* atom;

					/* find any key conflicts in the current scope */
					/* TODO: optimization. no sequential search */
					atom = curlist->tail;
					while (atom)
					{
						if (atom->type == QSE_XLI_PAIR &&
						    xli->opt.strcmp(((qse_xli_pair_t*)atom)->key, QSE_STR_PTR(xli->tok.name)) == 0)
						{
							qse_xli_seterror (xli, QSE_XLI_EEXIST, QSE_STR_XSTR(xli->tok.name), &xli->tok.loc);
							goto oops;
						}

						atom = atom->prev;
					}
				}

				QSE_ASSERT (key.ptr == QSE_NULL);
				key.len = QSE_STR_LEN(xli->tok.name);
				key.ptr = qse_strdup (QSE_STR_PTR(xli->tok.name), xli->mmgr);
				if (key.ptr == QSE_NULL) 
				{
					qse_xli_seterrnum (xli, QSE_XLI_ENOMEM, QSE_NULL); 
					goto oops;
				}

				xli->tok_status |= TOK_STATUS_SAME_LINE;
				if (get_token (xli) <= -1) goto oops;

				if (!MATCH(xli, QSE_XLI_TOK_EQ))
				{
					qse_xli_seterror (xli, QSE_XLI_EEQ,  QSE_STR_XSTR(xli->tok.name), &xli->tok.loc);
					goto oops;
				}

				/* read the value */
				xli->tok_status |= TOK_STATUS_UPTO_EOL;
				if (get_token (xli) <= -1) goto oops;

				xli->tok_status &= ~(TOK_STATUS_SAME_LINE | TOK_STATUS_UPTO_EOL);

				if (MATCH(xli, QSE_XLI_TOK_EOF))
				{
					/* empty value */
					qse_cstr_t empty;

					empty.ptr = QSE_T("");
					empty.len = 0;

					pair = qse_xli_insertpairwithstr (xli, curlist, QSE_NULL, key.ptr, QSE_NULL, QSE_NULL, &empty, QSE_NULL);

					QSE_MMGR_FREE (xli->mmgr, key.ptr);
					key.ptr = QSE_NULL;

					if (pair == QSE_NULL) goto oops;
					break;
				}

				if (MATCH(xli, QSE_XLI_TOK_SQSTR))
				{
					/* add a new pair with the initial string segment */
					pair = qse_xli_insertpairwithstr (xli, curlist, QSE_NULL, key.ptr, QSE_NULL, QSE_NULL, QSE_STR_XSTR(xli->tok.name), QSE_NULL);

					QSE_MMGR_FREE (xli->mmgr, key.ptr);
					key.ptr = QSE_NULL;

					if (pair == QSE_NULL) goto oops;
				}
				else
				{
					qse_xli_seterror (xli, QSE_XLI_EVAL, QSE_STR_XSTR(xli->tok.name), &xli->tok.loc);
					goto oops;
				}
			}
		}
		else
		{
			qse_xli_seterror (xli, QSE_XLI_ESECTAG, QSE_STR_XSTR(xli->tok.name), &xli->tok.loc);
			goto oops;
		}
	}

	return 0;

oops:
	if (key.ptr) QSE_MMGR_FREE (xli->mmgr, key.ptr);
	return -1;
}

static int read_root_list (qse_xli_t* xli)
{
	qse_xli_list_link_t* link;

	link = qse_xli_makelistlink (xli, &xli->root->list);
	if (!link) return -1;

	if (qse_xli_getchar (xli) <= -1 || get_token (xli) <= -1 || read_list (xli) <= -1)
	{
		qse_xli_freelistlink (xli, link);
		return -1;
	}

	QSE_ASSERT (link == xli->parlink);
	qse_xli_freelistlink (xli, link);

	return 0;
}

int qse_xli_readini (qse_xli_t* xli, qse_xli_io_impl_t io)
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

	if (!MATCH (xli, QSE_XLI_TOK_EOF))
	{
		qse_xli_seterror (xli, QSE_XLI_ESYNTAX, QSE_NULL, &xli->tok.loc);
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
		QSE_MMGR_FREE (xli->mmgr, xli->rio.inp);
		xli->rio.inp = prev;
	}
	
	qse_xli_closeactiverstream (xli);
	qse_str_clear (xli->tok.name);
	return -1;
}
