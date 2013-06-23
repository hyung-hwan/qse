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

static int close_current_stream (qse_xli_t* xli)
{
	qse_ssize_t n;

	n = xli->wio.impl (xli, QSE_XLI_IO_CLOSE, xli->wio.inp, QSE_NULL, 0);
	if (n <= -1)
	{
		if (xli->errnum == QSE_XLI_ENOERR) 
			qse_xli_seterrnum (xli, QSE_XLI_EIOUSR, QSE_NULL);
		return -1;
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
				
				for (i = 0; i < depth; i++) qse_printf (QSE_T("\t"));
				qse_printf (QSE_T("%s"), pair->key);
				if (pair->name) qse_printf (QSE_T(" \"%s\""), pair->name);

				switch (pair->val->type)
				{
					case QSE_XLI_NIL:
						qse_printf (QSE_T(";\n"));
						break;

					case QSE_XLI_STR:
					{
						qse_xli_str_t* str = (qse_xli_str_t*)pair->val;
						qse_printf (QSE_T(" = \"%.*s\";\n"), (int)str->len, str->ptr);
						break;	
					}

					case QSE_XLI_LIST:
					{
						qse_printf (QSE_T("{\n"));
						if (write_list (xli, pair->val, ++depth) <= -1)
						{
						}
						qse_printf (QSE_T("}\n"));
						break;
					}
				}
				break;
			}

			case QSE_XLI_TEXT:
				qse_printf (QSE_T("# %s\n"), ((qse_xli_text_t*)curatom)->ptr);
				break;

			case QSE_XLI_FILE:
				/* TODO filename escaping.... */
				qse_printf (QSE_T("@include \"%s\";\n"),((qse_xli_file_t*)curatom)->path);

				/* TODO: open a new stream */
				break;

			case QSE_XLI_EOF:
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
	xli->wio.arg.line = 1;
	xli->wio.arg.colm = 1;
	xli->wio.inp = &xli->wio.arg;
	/*qse_xli_clearwionames (xli);*/

#if 0
	n = xli->wio.impl (xli, QSE_XLI_IO_OPEN, xli->wio.inp, QSE_NULL, 0);
	if (n <= -1)
	{
		if (xli->errnum == QSE_XLI_ENOERR)
			qse_xli_seterrnum (xli, QSE_XLI_EIOUSR, QSE_NULL); 
		return -1;
	}
#endif

	n = write_list (xli, &xli->root, 0);
	QSE_ASSERT (xli->wio.inp == &xli->wio.arg);
#if 0
	close_current_stream (xli);
#endif
	return n;
}




