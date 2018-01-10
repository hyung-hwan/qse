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

#include "xli.h"
#include <qse/cmn/chr.h>

typedef struct arg_data_t arg_data_t;
struct arg_data_t
{
	int org_depth;
};

int qse_xli_flushwstream (qse_xli_t* xli, qse_xli_io_arg_t* arg)
{
	qse_ssize_t n;

	while (arg->b.pos < arg->b.len)
	{
		n = xli->wio.impl (xli, QSE_XLI_IO_WRITE, xli->wio.inp, &arg->b.buf[arg->b.pos], arg->b.len - arg->b.pos);
		if (n <= -1)
		{
			if (xli->errnum == QSE_XLI_ENOERR) 
				qse_xli_seterrnum (xli, QSE_XLI_EIOUSR, QSE_NULL);
			return -1;
		}

		arg->b.pos += n;
	}

	arg->b.pos = 0;
	arg->b.len = 0;
	return 0;
}

int qse_xli_openwstream (qse_xli_t* xli, const qse_char_t* path, int old_depth)
{
	qse_ssize_t n;
	qse_xli_io_arg_t* arg;

	if (path == QSE_NULL)
	{
		/* top-level */
		arg = &xli->wio.top;
	}
	else
	{
		qse_link_t* link;
		qse_size_t plen;
		arg_data_t* arg_data;

		plen = qse_strlen (path);

		link = (qse_link_t*) qse_xli_callocmem (xli, 
			QSE_SIZEOF(*link) + QSE_SIZEOF(qse_char_t) * (plen + 1));
		if (link == QSE_NULL) return -1;

		qse_strcpy ((qse_char_t*)(link + 1), path);
		link->link = xli->wio_names;
		xli->wio_names = link;

		arg = (qse_xli_io_arg_t*) qse_xli_callocmem (xli, QSE_SIZEOF(*arg) + QSE_SIZEOF(*arg_data));
		if (arg == QSE_NULL) return -1;

		arg_data = (arg_data_t*)(arg + 1);
		arg_data->org_depth = old_depth;

		arg->name = (const qse_char_t*)(link + 1);
		arg->prev = xli->wio.inp;
	}

	n = xli->wio.impl (xli, QSE_XLI_IO_OPEN, arg, QSE_NULL, 0);
	if (n <= -1)
	{
		if (xli->errnum == QSE_XLI_ENOERR)
			qse_xli_seterrnum (xli, QSE_XLI_EIOUSR, QSE_NULL); 
		if (arg != &xli->wio.top) 
		{
			qse_xli_freemem (xli, arg);
			/* don't clean up 'link' since it's linked to xli->wio_names */
		}
		return -1;
	}

	xli->wio.inp = arg;
	return 0;
}

int qse_xli_closeactivewstream (qse_xli_t* xli, int* org_depth)
{
	qse_ssize_t n;
	qse_xli_io_arg_t* arg;

	arg = xli->wio.inp;

	qse_xli_flushwstream (xli, arg); /* TODO: do i have to care about the result? */

	n = xli->wio.impl (xli, QSE_XLI_IO_CLOSE, arg, QSE_NULL, 0);
	if (n <= -1)
	{
		if (xli->errnum == QSE_XLI_ENOERR) 
			qse_xli_seterrnum (xli, QSE_XLI_EIOUSR, QSE_NULL);
		return -1;
	}

	xli->wio.inp = arg->prev;
	if (arg == &xli->wio.top)
	{
		if (org_depth) *org_depth = 0;
	}
	else
	{
		if (org_depth) *org_depth = ((arg_data_t*)(arg + 1))->org_depth;
		qse_xli_freemem (xli, arg);
	}

	return 0;
}

static int write_to_current_stream (qse_xli_t* xli, const qse_char_t* ptr, qse_size_t len, int escape)
{
	qse_xli_io_arg_t* arg;
	qse_size_t i;

	arg = xli->wio.inp;

	for (i = 0; i < len; i++)
	{
		if (escape && (ptr[i] == QSE_T('\\') || ptr[i] == QSE_T('\"')))
		{
			if (arg->b.len + 2 > QSE_COUNTOF(arg->b.buf) && qse_xli_flushwstream (xli, arg) <= -1) return -1;
			arg->b.buf[arg->b.len++] = QSE_T('\\');
		}
		else
		{
			if (arg->b.len + 1 > QSE_COUNTOF(arg->b.buf) && qse_xli_flushwstream (xli, arg) <= -1) return -1;
		}
		arg->b.buf[arg->b.len++] = ptr[i];
	}

	return 0;
}

static int write_indentation (qse_xli_t* xli, int depth)
{
	static const qse_char_t tabs[16] = 
	{
		QSE_T('\t'), QSE_T('\t'), QSE_T('\t'), QSE_T('\t'),
		QSE_T('\t'), QSE_T('\t'), QSE_T('\t'), QSE_T('\t'),
		QSE_T('\t'), QSE_T('\t'), QSE_T('\t'), QSE_T('\t'),
		QSE_T('\t'), QSE_T('\t'), QSE_T('\t'), QSE_T('\t')
	};

	if (depth <= QSE_COUNTOF(tabs))
	{
		if (write_to_current_stream (xli, tabs, depth, 0) <= -1) return -1;
	}
	else
	{
		int i;
		if (write_to_current_stream (xli, tabs, QSE_COUNTOF(tabs), 0) <= -1) return -1;
		for (i = QSE_COUNTOF(tabs); i < depth; i++) 
		{
			if (write_to_current_stream (xli, QSE_T("\t"), 1, 0) <= -1) return -1;
		}
	}
	return 0;
}

static int key_needs_quoting (qse_xli_t* xli, const qse_char_t* str, int nstr)
{
	/* this determines if a key or an alias requires quoting for output.
	 * NSTR is not taken into account because it's only allowed as a value */

	/* refer to the tokenization rule in get_token_into() in read.c */
	qse_char_t c;

	c = *str++;
	if (c == QSE_T('\0')) return 1; /* an empty string requires a quoting */

	if (c == QSE_T('_') || QSE_ISALPHA(c) || (!nstr && (xli->opt.trait & QSE_XLI_LEADDIGIT) && QSE_ISDIGIT(c)))
	{
		int lead_digit = QSE_ISDIGIT(c);
		int all_digits = 1;

		/* a normal identifier can be composed of wider varieties of 
		 * characters than a keyword/directive */
		while (1)
		{
			c = *str++;
			if (c == QSE_T('\0')) break;

			if (c == QSE_T('_') || c == QSE_T('-') || c == QSE_T(':') || 
			    c == QSE_T('*') || c == QSE_T('/') || QSE_ISALPHA(c)) 
			{
				all_digits = 0;
			}
			else if (QSE_ISDIGIT(c)) 
			{
				/* nothing to do */
			}
			else 
			{
				/* a disallowed character for an identifier */
				return 1; /* quote it */
			}
		}

		if (lead_digit && all_digits)
		{
			/* if an identifier begins with a digit, it must contain a non-digit character */
			/* in fact, it is not a valid identifer. so quote it */
			return 1;
		}
		else
		{
			/* this must be a normal identifer */
			return 0; /* no quoting needed */
		}
	}
	else if (nstr && QSE_ISDIGIT(c))
	{
		do 
		{
			c = *str++;
			if (c == QSE_T('\0')) return 0; /* it's a numeric string */
		}
		while (QSE_ISDIGIT(c));
	}

	/* quote all the rest */
	return 1;
}

static int write_list (qse_xli_t* xli, qse_xli_list_t* list, int depth)
{
	qse_xli_atom_t* curatom;

	for (curatom = list->head; curatom; curatom = curatom->next)
	{
		int quote_key = 0;

		switch (curatom->type)
		{
			case QSE_XLI_PAIR:
			{
				qse_xli_pair_t* pair = (qse_xli_pair_t*)curatom;
				
				if (write_indentation (xli, depth) <= -1) return -1;

				if (pair->tag)
				{
					if (write_to_current_stream (xli, QSE_T("["), 1, 0) <= -1 ||
					    write_to_current_stream (xli, pair->tag, qse_strlen(pair->tag), 0) <= -1 || 
					    write_to_current_stream (xli, QSE_T("]"), 1, 0) <= -1) return -1;
				}

				if (key_needs_quoting(xli, pair->key, 0)) quote_key = 1;

				if ((quote_key && write_to_current_stream (xli, QSE_T("\""), 1, 0) <= -1) || 
				    write_to_current_stream (xli, pair->key, qse_strlen(pair->key), 0) <= -1 ||
				    (quote_key && write_to_current_stream (xli, QSE_T("\""), 1, 0) <= -1)) return -1;

				if (pair->alias) 
				{
					if (write_to_current_stream (xli, QSE_T(" \""), 2, 0) <= -1 ||
					    write_to_current_stream (xli, pair->alias, qse_strlen(pair->alias), 1) <= -1 ||
					    write_to_current_stream (xli, QSE_T("\""), 1, 0) <= -1) return -1;
				}

				switch (pair->val->type)
				{
					case QSE_XLI_NIL:
						if (write_to_current_stream (xli, QSE_T(";\n"), 2, 0) <= -1) return -1;
						break;

					case QSE_XLI_TRUE:
						if (write_to_current_stream (xli, QSE_T(" = true;\n"), 9, 0) <= -1) return -1;
						break;

					case QSE_XLI_FALSE:
						if (write_to_current_stream (xli, QSE_T(" = false;\n"), 10, 0) <= -1) return -1;
						break;

					case QSE_XLI_STR:
					{
						qse_xli_str_t* str = (qse_xli_str_t*)pair->val;

						if (write_to_current_stream (xli, QSE_T(" = "), 3, 0) <= -1) return -1;
						while (1)
						{
							if (str->tag)
							{
								if (write_to_current_stream (xli, QSE_T("["), 1, 0) <= -1 ||
								    write_to_current_stream (xli, str->tag, qse_strlen(str->tag), 0) <= -1 || 
								    write_to_current_stream (xli, QSE_T("]"), 1, 0) <= -1) return -1;
							}

							if ((!(str->flags & QSE_XLI_STR_NSTR) && 
							     write_to_current_stream (xli, QSE_T("\""), 1, 0) <= -1) ||
							    write_to_current_stream (xli, str->ptr, str->len, 1) <= -1 ||
							    (!(str->flags & QSE_XLI_STR_NSTR) && 
							     write_to_current_stream (xli, QSE_T("\""), 1, 0) <= -1)) return -1;
							if (!str->next) break;

							if (write_to_current_stream (xli, QSE_T(", "), 2, 0) <= -1) return -1;
							str = str->next;
						}
						if (write_to_current_stream (xli, QSE_T(";\n"), 2, 0) <= -1) return -1;
						break;
					}

					case QSE_XLI_LIST:
					{
						if (write_to_current_stream (xli, QSE_T(" {\n"), 3, 0) <= -1 ||
						    write_list (xli, (qse_xli_list_t*)pair->val, depth + 1) <= -1 ||
						    write_indentation (xli, depth) <= -1 ||
						    write_to_current_stream (xli, QSE_T("}\n"), 2, 0) <= -1) return -1;
						break;
					}
				}
				break;
			}

			case QSE_XLI_TEXT:
			{
				int i;
				const qse_char_t* str = ((qse_xli_text_t*)curatom)->ptr;

				for (i = 0; i < depth; i++) 
				{
					if (write_to_current_stream (xli, QSE_T("\t"), 1, 0) <= -1) return -1;
				}

				if (write_to_current_stream (xli, QSE_T("#"), 1, 0) <= -1 ||
				    write_to_current_stream (xli, str, qse_strlen(str), 0) <= -1 ||
				    write_to_current_stream (xli, QSE_T("\n"), 1, 0) <= -1) return -1;
				break;
			}

			case QSE_XLI_FILE:
			{
				int i;
				const qse_char_t* path = ((qse_xli_file_t*)curatom)->path;

				for (i = 0; i < depth; i++) 
				{
					if (write_to_current_stream (xli, QSE_T("\t"), 1, 0) <= -1) return -1;
				}

				if (write_to_current_stream (xli, QSE_T("@include \""), 10, 0) <= -1 ||
				    write_to_current_stream (xli, path, qse_strlen(path), 1) <= -1 ||
				    write_to_current_stream (xli, QSE_T("\";\n"), 3, 0) <= -1) return -1;

				if (qse_xli_openwstream(xli, ((qse_xli_file_t*)curatom)->path, depth) <= -1) return -1;
				depth = 0;
				break;
			}

			case QSE_XLI_EOF:
				if (qse_xli_closeactivewstream (xli, &depth) <= -1) return -1;
				break;
		}
	}

	return 0;
}

void qse_xli_clearwionames (qse_xli_t* xli)
{
	qse_link_t* cur;
	while (xli->wio_names)
	{
		cur = xli->wio_names;
		xli->wio_names = cur->link;
		QSE_MMGR_FREE (xli->mmgr, cur);
	}
}

int qse_xli_write (qse_xli_t* xli, qse_xli_list_t* root_list, qse_xli_io_impl_t io)
{
	int n;

	if (io == QSE_NULL)
	{
		qse_xli_seterrnum (xli, QSE_XLI_EINVAL, QSE_NULL);
		return -1;
	}

	QSE_MEMSET (&xli->wio, 0, QSE_SIZEOF(xli->wio));
	xli->wio.impl = io;
	xli->wio.inp = &xli->wio.top;

	qse_xli_seterrnum (xli, QSE_XLI_ENOERR, QSE_NULL);
	qse_xli_clearwionames (xli);

	/* open the top level stream */
	if (qse_xli_openwstream (xli, QSE_NULL, 0) <= -1) return -1;

	/* begin writing the root list */
	n = write_list (xli, (root_list? root_list: &xli->root->list), 0);

	/* close all open streams. there should be only the
	 * top-level stream here if there occurred no errors */
	while (xli->wio.inp) qse_xli_closeactivewstream (xli, QSE_NULL);

	return n;
}

