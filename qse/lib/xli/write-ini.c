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

static int write_to_current_stream (qse_xli_t* xli, const qse_char_t* ptr, qse_size_t len)
{
	qse_xli_io_arg_t* arg;
	qse_size_t i;

	arg = xli->wio.inp;

	for (i = 0; i < len; i++)
	{
		if (arg->b.len + 1 > QSE_COUNTOF(arg->b.buf) && qse_xli_flushwstream (xli, arg) <= -1) return -1;
		arg->b.buf[arg->b.len++] = ptr[i];
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

				if (pair->tag)
				{
					/* the tag can't be written. so ignore it */
					/* do nothing */
				}

				if (depth <= 0)
				{
					if (write_to_current_stream (xli, QSE_T("["), 1) <= -1 ||
					    write_to_current_stream (xli, pair->key, qse_strlen(pair->key)) <= -1 ||
					    write_to_current_stream (xli, QSE_T("]\n"), 2) <= -1) return -1;
				}
				else
				{
					if (write_to_current_stream (xli, pair->key, qse_strlen(pair->key)) <= -1) return -1;
				}

				if (pair->alias) 
				{
					/* no alias is supported. so ignore it */
					/* do nothing */
				}

				switch (pair->val->type)
				{
					case QSE_XLI_NIL:
						if (write_to_current_stream (xli, QSE_T("\n"), 1) <= -1) return -1;
						break;

					case QSE_XLI_TRUE:
						if (write_to_current_stream (xli, QSE_T("true\n"), 5) <= -1) return -1;
						break;

					case QSE_XLI_FALSE:
						if (write_to_current_stream (xli, QSE_T("false\n"), 6) <= -1) return -1;
						break;

					case QSE_XLI_STR:
					{
						qse_xli_str_t* str = (qse_xli_str_t*)pair->val;

						if (depth > 0)
						{
							/* key = value is not supported at the top level */

							if (write_to_current_stream (xli, QSE_T("="), 1) <= -1) return -1;
							while (1)
							{
								if (str->tag)
								{
									/* no string tag is supported. ignore it */
									/* do nothing */
								}
							
								if (write_to_current_stream (xli, str->ptr, str->len) <= -1) return -1;

							#if 0
								if (!str->next) break;

								if (write_to_current_stream (xli, QSE_T(", "), 2) <= -1) return -1;
								str = str->next;
							#else
								/* no multi-segment string is supported. ignore it */
								break;
							#endif
							}
						}

						if (write_to_current_stream (xli, QSE_T("\n"), 1) <= -1) return -1;
						break;
					}

					case QSE_XLI_LIST:
						if (depth < 1)
						{
							if (write_list(xli, (qse_xli_list_t*)pair->val, depth + 1) <= -1 ||
							    write_to_current_stream (xli, QSE_T("\n"), 1) <= -1) return -1;
						}
						else
						{
							/* the ini format doesn't support deep nesting */
							if (write_to_current_stream (xli, QSE_T("={}\n"), 4) <= -1) return -1;
						}
						break;
				}
				break;
			}

			case QSE_XLI_TEXT:
			{
				const qse_char_t* str = ((qse_xli_text_t*)curatom)->ptr;

				if (write_to_current_stream(xli, QSE_T(";"), 1) <= -1 ||
				    write_to_current_stream(xli, str, qse_strlen(str)) <= -1 ||
				    write_to_current_stream(xli, QSE_T("\n"), 1) <= -1) return -1;
				break;
			}

			case QSE_XLI_FILE:
				/* no file inclusion is supported by the ini-format. ignore it */
				break;

			case QSE_XLI_EOF:
				/* no file inclusion is supported by the ini-format. so no EOF. ignore it */
				break;
		}
	}

	return 0;
}

int qse_xli_writeini (qse_xli_t* xli, qse_xli_list_t* root_list, qse_xli_io_impl_t io)
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
