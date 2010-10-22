/*
 * $Id: http.c 341 2008-08-20 10:58:19Z baconevi $
 * 
    Copyright 2006-2009 Chung, Hyung-Hwan.
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

#include <qse/utl/http.h>
#include <qse/cmn/chr.h>
#include "../cmn/mem.h"

QSE_IMPLEMENT_COMMON_FUNCTIONS (http)

static int is_http_space (qse_char_t c)
{
	return QSE_ISSPACE(c) && c != QSE_T('\r') && c != QSE_T('\n');
}

#define is_http_ctl(c) QSE_ISCNTRL(c)

static int is_http_separator (qse_char_t c)
{
	return c == QSE_T('(') ||
	       c == QSE_T(')') ||
	       c == QSE_T('<') ||
	       c == QSE_T('>') ||
	       c == QSE_T('@') ||
	       c == QSE_T(',') ||
	       c == QSE_T(';') ||
	       c == QSE_T(':') ||
	       c == QSE_T('\\') ||
	       c == QSE_T('\"') ||
	       c == QSE_T('/') ||
	       c == QSE_T('[') ||
	       c == QSE_T(']') ||
	       c == QSE_T('?') ||
	       c == QSE_T('=') ||
	       c == QSE_T('{') ||
	       c == QSE_T('}') ||
	       c == QSE_T('\t') ||
	       c == QSE_T(' ');
}

static int is_http_token (qse_char_t c)
{
	return QSE_ISPRINT(c) && !is_http_ctl(c) && !is_http_separator(c);
}

static int digit_to_num (qse_char_t c)
{
	if (c >= QSE_T('0') && c <= QSE_T('9')) return c - QSE_T('0');
	if (c >= QSE_T('A') && c <= QSE_T('Z')) return c - QSE_T('A') + 10;
	if (c >= QSE_T('a') && c <= QSE_T('z')) return c - QSE_T('a') + 10;
	return -1;
}

qse_char_t* qse_parsehttpreq (qse_char_t* buf, qse_http_req_t* req)
{
	qse_char_t* p = buf, * x;

	/* ignore leading spaces */
	while (is_http_space(*p)) p++;

	/* the method should start with an alphabet */
	if (!QSE_ISALPHA(*p)) return QSE_NULL;

	/* scan the method */
	req->method = p; while (QSE_ISALPHA(*p)) p++;

	/* the method should be followed by a space */
	if (!is_http_space(*p)) return QSE_NULL;

	/* null-terminate the method */
	*p++ = QSE_T('\0');

	/* skip spaces */
	while (is_http_space(*p)) p++;

	/* scan the url */
	req->path.ptr = p; 
	req->args.ptr = QSE_NULL;

	x = p;
	while (QSE_ISPRINT(*p) && !QSE_ISSPACE(*p)) 
	{
		if (*p == QSE_T('%') && QSE_ISXDIGIT(*(p+1)) && QSE_ISXDIGIT(*(p+2)))
		{
			*x++ = (digit_to_num(*(p+1)) << 4) + digit_to_num(*(p+2));
			p += 3;
		}
		else if (*p == QSE_T('?') && req->args.ptr == QSE_NULL)
		{
			/* ? must be explicit to be a argument instroducer. 
			 * %3f is just a literal. */
			req->path.len = x - req->path.ptr;
			*x++ = QSE_T('\0');
			req->args.ptr = x;
			p++;
		}
		else *x++ = *p++;
	}

	/* the url should be followed by a space */
	if (!is_http_space(*p)) return QSE_NULL;
	
	/* null-terminate the url and store the length */
	if (req->args.ptr != QSE_NULL)
		req->args.len = x - req->args.ptr;
	else
		req->path.len = x - req->path.ptr;
	*x++ = QSE_T('\0');

	/* path should start with a slash */
	if (req->path.len <= 0 || req->path.ptr[0] != QSE_T('/')) return QSE_NULL;

	/* skip spaces */
	do { p++; } while (is_http_space(*p));

	/* check http version */
	if ((p[0] == QSE_T('H') || p[0] == QSE_T('h')) &&
	    (p[1] == QSE_T('T') || p[1] == QSE_T('t')) &&
	    (p[2] == QSE_T('T') || p[2] == QSE_T('t')) &&
	    (p[3] == QSE_T('P') || p[3] == QSE_T('p')) &&
	    p[4] == QSE_T('/') && p[6] == QSE_T('.'))
	{
		if (!QSE_ISDIGIT(p[5])) return QSE_NULL;
		if (!QSE_ISDIGIT(p[7])) return QSE_NULL;
		req->vers.major = p[5] - QSE_T('0');
		req->vers.minor = p[7] - QSE_T('0');
		p += 8;
	}
	else return QSE_NULL;

	while (QSE_ISSPACE(*p)) 
	{
		if (*p++ == QSE_T('\n')) goto ok;
	}

	/* not terminating with a new line.
	 * maybe garbage after the request line */
	if (*p != QSE_T('\0')) return QSE_NULL;

ok:
	/* returns the next position */
	return p;
}

qse_char_t* qse_parsehttphdr (qse_char_t* buf, qse_http_hdr_t* hdr)
{
	qse_char_t* p = buf, * last;

	/* ignore leading spaces including CR and NL */
	while (QSE_ISSPACE(*p)) p++;

	if (*p == QSE_T('\0')) 
	{
		/* no more header line */
		QSE_MEMSET (hdr, 0, QSE_SIZEOF(*hdr));
		return p;
	}

	if (!is_http_token(*p)) return QSE_NULL;

	hdr->name.ptr = p;
	do { p++; } while (is_http_token(*p));

	last = p;
	hdr->name.len = last - hdr->name.ptr;

	while (is_http_space(*p)) p++;
	if (*p != QSE_T(':')) return QSE_NULL;

	*last = QSE_T('\0');

	do { p++; } while (is_http_space(*p));

	hdr->value.ptr = last = p;
	while (QSE_ISPRINT(*p))
	{
		if (!QSE_ISSPACE(*p++)) last = p;
	}
	hdr->value.len = last - hdr->value.ptr;

	while (QSE_ISSPACE(*p)) 
	{
		if (*p++ == QSE_T('\n')) goto ok;
	}

	/* not terminating with a new line.
	 * maybe garbage after the header line */
	if (*p != QSE_T('\0')) return QSE_NULL;

ok:
	*last = QSE_T('\0');
	return p;
}

static QSE_INLINE void init_buffer (qse_http_t* http, qse_http_buf_t* buf)
{
	buf->size = 0;
	buf->capa = 0;
	buf->data = QSE_NULL;
}

static QSE_INLINE void fini_buffer (qse_http_t* http, qse_http_buf_t* buf)
{
	if (buf->data) 
	{
		QSE_MMGR_FREE (http->mmgr, buf->data);
		buf->capa = 0;
		buf->size = 0;
		buf->data = QSE_NULL;
	}
}

static QSE_INLINE_ALWAYS void clear_buffer (qse_http_t* http, qse_http_buf_t* buf)
{
	buf->size = 0;
}

static QSE_INLINE int push_to_buffer (
	qse_http_t* http, qse_http_buf_t* buf, 
	const qse_char_t* ptr, qse_size_t len)
{
	qse_size_t nsize = (buf)->size + len; 
	const qse_char_t* end = ptr + len;

	if (nsize > (buf)->capa) 
	{ 
		qse_size_t ncapa = (nsize > (buf)->capa * 2)? nsize: ((buf)->capa * 2);
		
		do
		{
			void* tmp = QSE_MMGR_REALLOC ((http)->mmgr, (buf)->data, ncapa * QSE_SIZEOF(*ptr));
			if (tmp)
			{
				(buf)->capa = ncapa;
				(buf)->data = tmp;
				break;
			}

			if (ncapa <= nsize)
			{
				(http)->errnum = QSE_HTTP_ENOMEM;
				return -1;
			}

			/* retry with a smaller size */
			ncapa--;
		}
		while (1);
	}

	while (ptr < end) (buf)->data[(buf)->size++] = *ptr++;

	return 0;
}

#define QSE_HTTP_STATE_REQ  1
#define QSE_HTTP_STATE_HDR  2
#define QSE_HTTP_STATE_POST 3

qse_http_t* qse_http_open (qse_mmgr_t* mmgr, qse_size_t xtnsize)
{
	qse_http_t* http;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	http = (qse_http_t*) QSE_MMGR_ALLOC (
		mmgr, QSE_SIZEOF(qse_http_t) + xtnsize
	);
	if (http == QSE_NULL) return QSE_NULL;

	if (qse_http_init (http, mmgr) == QSE_NULL)
	{
		QSE_MMGR_FREE (http->mmgr, http);
		return QSE_NULL;
	}

	return http;
}

void qse_http_close (qse_http_t* http)
{
	qse_http_fini (http);
	QSE_MMGR_FREE (http->mmgr, http);
}

qse_http_t* qse_http_init (qse_http_t* http, qse_mmgr_t* mmgr)
{
	if (mmgr == QSE_NULL) mmgr = QSE_MMGR_GETDFL();

	QSE_MEMSET (http, 0, QSE_SIZEOF(*http));
	http->mmgr = mmgr;

	init_buffer (http, &http->state.buf);
	http->state.no = QSE_HTTP_STATE_REQ;
	return http;
}

void qse_http_fini (qse_http_t* http)
{
	fini_buffer (http, &http->state.buf);
}

/* feed the percent encoded string */
int qse_http_feed (qse_http_t* http, const qse_char_t* ptr, qse_size_t len)
{
	const qse_char_t* end = ptr + len;
	const qse_char_t* blk = ptr;

	while (ptr < end)
	{
		if (*ptr++ == '\n')
		{
			if (push_to_buffer (http, &http->state.buf, blk, ptr - blk) <= -1) return -1;

			blk = ptr; /* let ptr point to the next character to '\n' */

			if (http->state.no == QSE_HTTP_STATE_REQ)
			{
				/*
				if (parse_http_req (http, &http->state.buf) <= -1)
				{
					return -1;
				}
				*/
			}
			else 
			{
				/*
				if (parse_http_hdr (http, &http->state.buf) <= -1)
				{
					return -1;
				}
				*/
			}

qse_printf (QSE_T("[%.*s]\n"), (int)http->state.buf.size, http->state.buf.data);
			clear_buffer (http, &http->state.buf);
		}
	}


	/* enbuffer the unfinished data */
	if (push_to_buffer (http, &http->state.buf, blk, ptr - blk) <= -1) return -1;
qse_printf (QSE_T("UNFINISHED [%.*s]\n"), (int)http->state.buf.size, http->state.buf.data);

	return 0;
}
