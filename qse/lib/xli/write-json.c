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

static int write_to_current_stream(qse_xli_t* xli, const qse_char_t* ptr, qse_size_t len, int escape)
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
		if (write_to_current_stream(xli, tabs, depth, 0) <= -1) return -1;
	}
	else
	{
		int i;
		if (write_to_current_stream(xli, tabs, QSE_COUNTOF(tabs), 0) <= -1) return -1;
		for (i = QSE_COUNTOF(tabs); i < depth; i++) 
		{
			if (write_to_current_stream(xli, QSE_T("\t"), 1, 0) <= -1) return -1;
		}
	}
	return 0;
}

static int need_comma (qse_xli_atom_t* start)
{
	qse_xli_atom_t* cur;
	for (cur = start; cur; cur = cur->next)
	{
		if (cur->type == QSE_XLI_PAIR) return 1;
		if (cur->type == QSE_XLI_EOF) return 0; /* if EOF encountered in the included file, i don't want a comma at the end of the file */
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

				if (write_indentation(xli, depth) <= -1) return -1;

				if (!(list->flags & QSE_XLI_LIST_ARRAYED))
				{
					if (pair->tag)
					{
						/* ignore tag in the json format */
					}

					if (write_to_current_stream(xli, QSE_T("\""), 1, 0) <= -1 ||
					    write_to_current_stream(xli, pair->key, qse_strlen(pair->key), 1) <= -1 ||
					    write_to_current_stream(xli, QSE_T("\""), 1, 0) <= -1) return -1;

					if (pair->alias) 
					{
						/* ignore alias in the json format */
					}

					if (write_to_current_stream(xli, QSE_T(": "), 2, 0) <= -1) return -1;
				}

				switch (pair->val->type)
				{
					case QSE_XLI_NIL:
						if (write_to_current_stream(xli, QSE_T("nil"), 5, 0) <= -1) return -1;
						break;

					case QSE_XLI_TRUE:
						if (write_to_current_stream(xli, QSE_T("true"), 6, 0) <= -1) return -1;
						break;

					case QSE_XLI_FALSE:
						if (write_to_current_stream(xli, QSE_T("false"), 7, 0) <= -1) return -1;
						break;

					case QSE_XLI_STR:
					{
						qse_xli_str_t* str = (qse_xli_str_t*)pair->val;

						/* ignore the string tag(str->tag) in the json format.
						 * concatenate multi-segmented string into 1 seperated by a comma */
						int quote_needed;

						/* if the string value is a non-numeric string or
						 * it is multi-segmented, quoting is needed */
						quote_needed = !(str->flags & QSE_XLI_STR_NSTR) || str->next;

						if (quote_needed && write_to_current_stream(xli, QSE_T("\""), 1, 0) <= -1) return -1;
						while (1)
						{
							if (write_to_current_stream(xli, str->ptr, str->len, quote_needed) <= -1) return -1;
							if (!str->next) break;
							if (write_to_current_stream(xli, QSE_T(","), 1, 0) <= -1) return -1;
							str = str->next;
						}

						if (quote_needed && write_to_current_stream(xli, QSE_T("\""), 1, 0) <= -1) return -1;
						break;
					}

					case QSE_XLI_LIST:
					{
						static qse_char_t* obrac[] =
						{
							QSE_T("{\n"),
							QSE_T("[\n")
						};
						static qse_char_t* cbrac[] =
						{
							QSE_T("}"),
							QSE_T("]")
						};
						qse_xli_list_t* lv = (qse_xli_list_t*)pair->val;
						if (write_to_current_stream(xli, obrac[lv->flags & QSE_XLI_LIST_ARRAYED], 2, 0) <= -1 ||
						    write_list (xli, (qse_xli_list_t*)pair->val, depth + 1) <= -1 ||
						    write_indentation (xli, depth) <= -1 ||
						    write_to_current_stream(xli, cbrac[lv->flags & QSE_XLI_LIST_ARRAYED], 1, 0) <= -1) return -1;
						break;
					}
				}

				if (need_comma(curatom->next) && write_to_current_stream(xli, QSE_T(","), 1, 0) <= -1) return -1;
				if (write_to_current_stream(xli, QSE_T("\n"), 1, 0) <= -1) return -1;
				break;
			}

			case QSE_XLI_TEXT:
			{
				qse_xli_text_t* ta;
				const qse_char_t* str;

				ta = (qse_xli_text_t*)curatom;
				str = ta->ptr;

				if (write_indentation(xli, depth - !!(ta->flags & QSE_XLI_TEXT_DEINDENT)) <= -1) return -1;

				if ((!(ta->flags & QSE_XLI_TEXT_VERBATIM) && write_to_current_stream(xli, QSE_T("#"), 1, 0) <= -1) ||
				    write_to_current_stream(xli, str, qse_strlen(str), 0) <= -1 ||
				    write_to_current_stream(xli, QSE_T("\n"), 1, 0) <= -1) return -1;
				break;
			}

			case QSE_XLI_FILE:
			{
				const qse_char_t* path = ((qse_xli_file_t*)curatom)->path;

				if (write_indentation(xli, depth) <= -1) return -1;

				if (write_to_current_stream(xli, QSE_T("@include \""), 10, 0) <= -1 ||
				    write_to_current_stream(xli, path, qse_strlen(path), 1) <= -1 ||
				    write_to_current_stream(xli, QSE_T("\""), 1, 0) <= -1) return -1;

				if (qse_xli_openwstream (xli, ((qse_xli_file_t*)curatom)->path, depth) <= -1) return -1;
				depth = 0;
				break;
			}

			case QSE_XLI_EOF:
				if (qse_xli_closeactivewstream(xli, &depth) <= -1) return -1;

				if (need_comma(curatom->next) && write_to_current_stream(xli, QSE_T(","), 1, 0) <= -1) return -1;
				if (write_to_current_stream(xli, QSE_T("\n"), 1, 0) <= -1) return -1;
				break;
		}
	}

	return 0;
}


static int have_opening_marker (qse_xli_t* xli, qse_xli_list_t* list)
{
	qse_xli_atom_t* curatom;

	for (curatom = list->head; curatom; curatom = curatom->next)
	{
		qse_xli_text_t* ta;
		if (curatom->type != QSE_XLI_TEXT) break;
		ta = (qse_xli_text_t*)curatom;
		if (ta->flags & (QSE_XLI_TEXT_ARRAYED_LIST_OPENER | QSE_XLI_TEXT_LIST_OPENER)) return 1;
	}

	return 0;
}

int qse_xli_writejson (qse_xli_t* xli, qse_xli_list_t* root_list, qse_xli_io_impl_t io)
{
	int n, marker;

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

	if (!root_list) root_list = &xli->root->list;

	marker = have_opening_marker(xli, root_list);
	if (!marker)
	{
		/* if the data has been loaded from a different format like xli or ini,
		 * there are no opening and closing markers. so emit them manually */
		if (write_to_current_stream(xli, ((root_list->flags & QSE_XLI_LIST_ARRAYED)? QSE_T("[\n"): QSE_T("{\n")), 2, 0) <= -1) return -1;
	}

	/* begin writing the root list */
	n = write_list (xli, root_list, 1);

	if (!marker)
	{
		if (write_to_current_stream(xli, ((root_list->flags & QSE_XLI_LIST_ARRAYED)? QSE_T("]\n"): QSE_T("}\n")), 2, 0) <= -1) return -1;
	}

	/* close all open streams. there should be only the
	 * top-level stream here if there occurred no errors */
	while (xli->wio.inp) qse_xli_closeactivewstream (xli, QSE_NULL);

	return n;
}


