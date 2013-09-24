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

typedef struct arg_data_t arg_data_t;
struct arg_data_t
{
	int org_depth;
};

static int flush (qse_xli_t* xli, qse_xli_io_arg_t* arg)
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

static int open_new_stream (qse_xli_t* xli, const qse_char_t* path, int old_depth)
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

static int close_current_stream (qse_xli_t* xli, int* org_depth)
{
	qse_ssize_t n;
	qse_xli_io_arg_t* arg;

	arg = xli->wio.inp;

	flush (xli, arg); /* TODO: do i have to care about the result? */

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
			if (arg->b.len + 2 > QSE_COUNTOF(arg->b.buf) && flush (xli, arg) <= -1) return -1;
			arg->b.buf[arg->b.len++] = QSE_T('\\');
		}
		else
		{
			if (arg->b.len + 1 > QSE_COUNTOF(arg->b.buf) && flush (xli, arg) <= -1) return -1;
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

static int write_list (qse_xli_t* xli, qse_xli_list_t* list, int depth)
{
	qse_xli_atom_t* curatom;

	for (curatom = list->head; curatom; curatom = curatom->next)
	{
		switch (curatom->type)
		{
			case QSE_XLI_PAIR:
			{
				qse_xli_pair_t* pair = (qse_xli_pair_t*)curatom;
				
				if (write_indentation (xli, depth) <= -1 ||
				    write_to_current_stream (xli, pair->key, qse_strlen(pair->key), 0) <= -1) return -1;

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
						
							if (write_to_current_stream (xli, QSE_T("\""), 1, 0) <= -1 ||
							    write_to_current_stream (xli, str->ptr, str->len, 1) <= -1 ||
							    write_to_current_stream (xli, QSE_T("\""), 1, 0) <= -1) return -1;
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
				
				if (open_new_stream (xli, ((qse_xli_file_t*)curatom)->path, depth) <= -1) return -1;
				depth = 0;
				break;
			}

			case QSE_XLI_EOF:
				if (close_current_stream (xli, &depth) <= -1) return -1;
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

int qse_xli_write (qse_xli_t* xli, qse_xli_io_impl_t io)
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
	if (open_new_stream (xli, QSE_NULL, 0) <= -1) return -1;

	/* begin writing the root list */
	n = write_list (xli, &xli->root->list, 0);
	
	/* close all open streams. there should be only the
	 * top-level stream here if there occurred no errors */
	while (xli->wio.inp) close_current_stream (xli, QSE_NULL);

	return n;
}

