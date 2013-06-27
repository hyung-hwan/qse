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

static int flush (qse_xli_t* xli, qse_xli_io_arg_t* arg)
{
	qse_ssize_t n;

/* TODO: flush all */
	n = xli->wio.impl (xli, QSE_XLI_IO_WRITE, xli->wio.inp, arg->b.buf, arg->b.len);
	if (n <= -1)
	{
		if (xli->errnum == QSE_XLI_ENOERR) 
			qse_xli_seterrnum (xli, QSE_XLI_EIOUSR, QSE_NULL);
		return -1;
	}

	arg->b.pos += n;
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
		qse_size_t plen;

		plen = qse_strlen (path);

		arg = (qse_xli_io_arg_t*) qse_xli_callocmem (xli, QSE_SIZEOF(*arg) + (plen + 1) * QSE_SIZEOF(*path));
		if (arg == QSE_NULL) return -1;

		qse_strcpy ((qse_char_t*)(arg + 1), path);
		arg->name = (const qse_char_t*)(arg + 1);
		arg->prev = xli->wio.inp;
	}

	n = xli->wio.impl (xli, QSE_XLI_IO_OPEN, arg, QSE_NULL, 0);
	if (n <= -1)
	{
		if (xli->errnum == QSE_XLI_ENOERR)
			qse_xli_seterrnum (xli, QSE_XLI_EIOUSR, QSE_NULL); 
		if (arg != &xli->wio.top) qse_xli_freemem (xli, arg);
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
	/*if (org_depth) *org_depth = ...*/
	qse_xli_freemem (xli, arg);
	return 0;
}

static int write_to_current_stream (qse_xli_t* xli, const qse_char_t* ptr, qse_size_t len, int escape)
{
	qse_xli_io_arg_t* arg;
	qse_size_t i;

/* TODO: buffering or escaping... */
	arg = xli->wio.inp;

	for (i = 0; i < len; i++)
	{
		if (escape && (ptr[i] == QSE_T('\\') || ptr[i] == QSE_T('\"')))
		{
			arg->b.buf[arg->b.len++] = QSE_T('\\');
		}
		arg->b.buf[arg->b.len++] = ptr[i];
		if (arg->b.len >= QSE_COUNTOF(arg->b.buf)) flush (xli, arg);
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
				int i;
				qse_xli_pair_t* pair = (qse_xli_pair_t*)curatom;
				
				for (i = 0; i < depth; i++) 
				{
					if (write_to_current_stream (xli, QSE_T("\t"), 1, 0) <= -1) return -1;
				}

				if (write_to_current_stream (xli, pair->key, qse_strlen(pair->key), 0) <= -1) return -1;

				if (pair->name) 
				{
					if (write_to_current_stream (xli, QSE_T(" \""), 2, 0) <= -1 ||
					    write_to_current_stream (xli, pair->name, qse_strlen(pair->name), 1) <= -1 ||
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
						if (write_to_current_stream (xli, QSE_T(" = \""), 4, 0) <= -1 ||
						    write_to_current_stream (xli, str->ptr, str->len, 1) <= -1 ||
						    write_to_current_stream (xli, QSE_T("\";"), 2, 0) <= -1) return -1;
						break;	
					}

					case QSE_XLI_LIST:
					{
						if (write_to_current_stream (xli, QSE_T(" {\n"), 3, 0) <= -1 ||
						    write_list (xli, pair->val, ++depth) <= -1 ||
						    write_to_current_stream (xli, QSE_T("}\n"), 2, 0) <= -1) return -1;
						break;
					}
				}
				break;
			}

			case QSE_XLI_TEXT:
			{
				const qse_char_t* str = ((qse_xli_text_t*)curatom)->ptr;
				if (write_to_current_stream (xli, QSE_T("#"), 1, 0) <= -1 ||
				    write_to_current_stream (xli, str, qse_strlen(str), 0) <= -1 ||
				    write_to_current_stream (xli, QSE_T("\n"), 1, 0) <= -1) return -1;
				break;
			}

			case QSE_XLI_FILE:
			{
				const qse_char_t* path = ((qse_xli_file_t*)curatom)->path;

				if (write_to_current_stream (xli, QSE_T("@include \""), 10, 0) <= -1 ||
				    write_to_current_stream (xli, path, qse_strlen(path), 1) <= -1 ||
				    write_to_current_stream (xli, QSE_T("\""), 1, 0) <= -1) return -1;
				
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

int qse_xli_write (qse_xli_t* xli, qse_xli_io_impl_t io)
{
	int n;

#if 0
	if (io == QSE_NULL)
	{
		qse_xli_seterrnum (xli, QSE_XLI_EINVAL, QSE_NULL);
		return -1;
	}
#endif

	QSE_MEMSET (&xli->wio, 0, QSE_SIZEOF(xli->wio));
	xli->wio.impl = io;
	xli->wio.inp = &xli->wio.top;
	/*qse_xli_clearwionames (xli);*/

	if (open_new_stream (xli, QSE_NULL, 0) <= -1) return -1;

	n = write_list (xli, &xli->root, 0);
	QSE_ASSERT (xli->wio.inp == &xli->wio.arg);

	while (xli->wio.inp) close_current_stream (xli, QSE_NULL);
	return n;
}




