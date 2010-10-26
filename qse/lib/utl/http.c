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

static QSE_INLINE int is_http_space (qse_char_t c)
{
	return QSE_ISSPACE(c) && c != QSE_T('\r') && c != QSE_T('\n');
}

#define is_http_ctl(c) QSE_ISCNTRL(c)

static QSE_INLINE int is_http_separator (qse_char_t c)
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

static QSE_INLINE int is_http_token (qse_char_t c)
{
	return QSE_ISPRINT(c) && !is_http_ctl(c) && !is_http_separator(c);
}

static QSE_INLINE int dig_to_num (qse_char_t c)
{
	if (c >= QSE_T('0') && c <= QSE_T('9')) return c - QSE_T('0');
	if (c >= QSE_T('A') && c <= QSE_T('Z')) return c - QSE_T('A') + 10;
	if (c >= QSE_T('a') && c <= QSE_T('z')) return c - QSE_T('a') + 10;
	return -1;
}

qse_char_t* qse_parsehttpreq (qse_char_t* octb, qse_http_req_t* req)
{
	qse_char_t* p = octb, * x;

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
			*x++ = (dig_to_num(*(p+1)) << 4) + dig_to_num(*(p+2));
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

qse_char_t* qse_parsehttphdr (qse_char_t* octb, qse_http_hdr_t* hdr)
{
	qse_char_t* p = octb, * last;

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

static QSE_INLINE int is_whspace_octet (qse_byte_t c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static QSE_INLINE int is_space_octet (qse_byte_t c)
{
	return c == ' ' || c == '\t' || c == '\r';
}

static QSE_INLINE int is_upalpha_octet (qse_byte_t c)
{
	return c >= 'A' && c <= 'Z';
}

static QSE_INLINE int is_loalpha_octet (qse_byte_t c)
{
	return c >= 'a' && c <= 'z';
}

static QSE_INLINE int is_alpha_octet (qse_byte_t c)
{
	return (c >= 'A' && c <= 'Z') ||
	       (c >= 'a' && c <= 'z');
}

static QSE_INLINE int is_digit_octet (qse_byte_t c)
{
	return c >= '0' && c <= '9';
}

static QSE_INLINE int is_xdigit_octet (qse_byte_t c)
{
	return (c >= '0' && c <= '9') ||
	       (c >= 'A' && c <= 'F') ||
	       (c >= 'a' && c <= 'f');
}

static QSE_INLINE int digit_to_num (qse_byte_t c)
{
	if (c >= '0' && c <= '9') return c - '0';
	return -1;
}

static QSE_INLINE int xdigit_to_num (qse_byte_t c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
	if (c >= 'a' && c <= 'z') return c - 'a' + 10;
	return -1;
}

static QSE_INLINE void init_buffer (qse_http_t* http, qse_http_octb_t* octb)
{
	octb->size = 0;
	octb->capa = 0;
	octb->data = QSE_NULL;
}

static QSE_INLINE void fini_buffer (qse_http_t* http, qse_http_octb_t* octb)
{
	if (octb->data) 
	{
		QSE_MMGR_FREE (http->mmgr, octb->data);
		octb->capa = 0;
		octb->size = 0;
		octb->data = QSE_NULL;
	}
}

static QSE_INLINE_ALWAYS void clear_buffer (qse_http_t* http, qse_http_octb_t* octb)
{
	octb->size = 0;
}

static QSE_INLINE int push_to_buffer (
	qse_http_t* http, qse_http_octb_t* octb, 
	const qse_byte_t* ptr, qse_size_t len)
{
	qse_size_t nsize = (octb)->size + len; 
	const qse_byte_t* end = ptr + len;

	if (nsize > (octb)->capa) 
	{ 
		qse_size_t ncapa = (nsize > (octb)->capa * 2)? nsize: ((octb)->capa * 2);
		
		do
		{
			void* tmp = QSE_MMGR_REALLOC ((http)->mmgr, (octb)->data, ncapa * QSE_SIZEOF(*ptr));
			if (tmp)
			{
				(octb)->capa = ncapa;
				(octb)->data = tmp;
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

	while (ptr < end) (octb)->data[(octb)->size++] = *ptr++;

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

	/*http->state.pending = 0;*/
	http->state.crlf = 0;
	http->state.plen = 0;
	init_buffer (http, &http->req.raw);
	return http;
}

void qse_http_fini (qse_http_t* http)
{
	fini_buffer (http, &http->req.raw);
}

static qse_byte_t* parse_http_req (qse_http_t* http, qse_byte_t* line)
{
	qse_byte_t* p = line;
	qse_byte_t* tmp;
	qse_size_t tmplen;

#if 0
	/* ignore leading spaces excluding crlf */
	while (is_space_octet(*p)) p++;
#endif

	/* the method should start with an alphabet */
	if (!is_upalpha_octet(*p)) goto badreq;

	/* get the method name */
	tmp = p;
	do { p++; } while (is_upalpha_octet(*p));
	tmplen = p - tmp;

	/* test the method name */
	if (tmplen == 3)
	{
		/* GET */
		if (tmp[0] == 'G' && tmp[1] == 'E' && tmp[2] == 'T')
			http->req.method = QSE_HTTP_REQ_GET;
		else goto badreq;
	}
	else if (tmplen == 4)
	{
		/* POST, HEAD */
		if (tmp[0] == 'P' && tmp[1] == 'O' && tmp[2] == 'S' && tmp[3] == 'T')
			http->req.method = QSE_HTTP_REQ_POST;
		else if (tmp[0] == 'H' && tmp[1] == 'E' && tmp[2] == 'A' && tmp[3] == 'D')
			http->req.method = QSE_HTTP_REQ_HEAD;
		/* TODO: more methods */
		else goto badreq;
	}
	else goto badreq;

	/* skip spaces */
	while (is_space_octet(*p)) p++;

	/* process the url part */
	http->req.path = p; 
	http->req.args = QSE_NULL;

	tmp = p;
	while (!is_space_octet(*p)) 
	{
		if (*p == '%')
		{
			int q = xdigit_to_num(*(p+1));
			int w = xdigit_to_num(*(p+2));

			if (q >= 0 && w >= 0)
			{
				int t = (q << 4) + w;
				if (t == 0)
				{
					/* percent enconding contains a null character */
					goto badreq;
				}

				*tmp++ = t;
				p += 3;
			}
			else *tmp++ = *p++;
		}
		else if (*p == '?')
		{
			if (!http->req.args)
			{
				/* ? must be explicit to be a argument instroducer. 
				 * %3f is just a literal. */
				*tmp++ = '\0';
				http->req.args = tmp;
				p++;
			}
			else *tmp++ = *p++;
		}
		else *tmp++ = *p++;
	}

	/* the url must be followed by a space */
	if (!is_space_octet(*p)) goto badreq;

	/* null-terminate the url part */
	*tmp = '\0';

	/* skip spaces after the url part */
	do { p++; } while (is_space_octet(*p));

	/* check http version */
	if ((p[0] == 'H' || p[0] == 'h') &&
	    (p[1] == 'T' || p[1] == 't') &&
	    (p[2] == 'T' || p[2] == 't') &&
	    (p[3] == 'P' || p[3] == 'p') &&
	    p[4] == '/' && p[6] == '.')
	{
		int q = digit_to_num(p[5]);
		int w = digit_to_num(p[7]);
		if (q >= 0 && w >= 0)
		{
			http->req.version.major = q;
			http->req.version.minor = w;
			p += 8;
		}
		else goto badreq;
	}
	else goto badreq;

	/* skip trailing spaces on the line */
	while (is_space_octet(*p)) p++;

	/* if the line does not end with a new line, it is a bad request */
	if (*p != QSE_T('\n')) goto badreq;

qse_printf (QSE_T("parse_http_req ....\n"));
	return ++p;

badreq:
	http->errnum = QSE_HTTP_EBADREQ;
	return QSE_NULL;
}

qse_byte_t* parse_http_header (qse_http_t* http, qse_byte_t* line)
{
	qse_byte_t* p = line, * mark;
	struct
	{
		qse_byte_t* ptr;
		qse_size_t  len;
	} name, value;

#if 0
	/* ignore leading spaces excluding crlf */
	while (is_space_octet(*p)) p++;
#endif

	name.ptr = p;
	while (!is_whspace_octet(*p) && *p != ':') p++;
	name.len = p - name.ptr;

	mark = p; while (is_space_octet(*p)) p++;
	if (*p != ':') goto badhdr;
	*mark = '\0';

	/* skip the colon and spaces after it */
	do { p++; } while (is_space_octet(*p));

	value.ptr = p;
	do { p++; } while (!is_whspace_octet(*p));
	value.len = p - value.ptr;

	/* skip trailing spaces on the line */
	mark = p; while (is_space_octet(*p)) p++;
	if (*p != '\n') goto badhdr; /* not ending with a new line */
	*mark = '\0';


qse_printf (QSE_T("<<%S>> => <<%S>>\n"),  name.ptr, value.ptr);
	return p;

badhdr:
qse_printf (QSE_T("BADHDR\n"),  name.ptr, value.ptr);
	http->errnum = QSE_HTTP_EBADHDR;
	return QSE_NULL;
}


/* feed the percent encoded string */
int qse_http_feed (qse_http_t* http, const qse_byte_t* ptr, qse_size_t len)
{
	const qse_byte_t* end = ptr + len;
	const qse_byte_t* req = ptr;

	static const qse_byte_t nul = '\0';

	while (ptr < end)
	{
		qse_byte_t b = *ptr++;

		if (http->state.plen <= 0 && is_whspace_octet(b)) 
		{
			/* let's drop leading whitespaces across multiple
			 * lines */
			req++;
			continue;
		}

		if (b == '\n')
		{
			if (http->state.crlf <= 1) http->state.crlf = 2;
			else
			{
				qse_byte_t* p;

				QSE_ASSERT (http->state.crlf <= 3);

				/* got a complete request */
				http->state.crlf = 0;
				http->state.plen = 0;

				/* add the actual request */
				if (push_to_buffer (http, &http->req.raw, req, ptr-req) <= -1) return -1;

				/* add the terminating null for easier parsing */
				if (push_to_buffer (http, &http->req.raw, &nul, 1) <= -1) return -1;

				p = http->req.raw.data;

				while (is_whspace_octet(*p)) p++;
				QSE_ASSERT (*p != '\0');
				p = parse_http_req (http, p);
				if (p == QSE_NULL) return -1;
			
				do
				{
					while (is_whspace_octet(*p)) p++;
					if (*p == '\0') break;

					p = parse_http_header (http, p);
					if (p == QSE_NULL) return -1;
				}
				while (1);
			
				clear_buffer (http, &http->req.raw);
				req = ptr; /* let ptr point to the next character to '\n' */
			}
		}
		else if (b == '\r')
		{
			if (http->state.crlf == 0 || http->state.crlf == 2) 
				http->state.crlf++;
			else http->state.crlf = 1;
		}
		else if (b == '\0')
		{
			/* guarantee that the request does not contain a null character */
			http->errnum = QSE_HTTP_EBADREQ;
			return -1;
		}
		else
		{
			http->state.plen++;
			http->state.crlf = 0;
		}
	}

	if (ptr > req)
	{
		/* enbuffer the incomplete request */
		if (push_to_buffer (http, &http->req.raw, req, ptr - req) <= -1) return -1;
	}

#if 0
	while (ptr < end)
	{
		if (*ptr++ == '\n')
		{
			qse_size_t reqlen = ptr - req;
			int blank;

			if (http->state.pending > 0)
			{
				QSE_ASSERT (http->req.raw.size > 0);
				blank = (reqlen + http->state.pending == 2 && 
				         http->req.raw.data[http->req.raw.size-1] == '\r');
				http->state.pending = 0;
			}
			else
			{
				blank = (reqlen == 1 || (reqlen == 2 && req[0] == '\r'));
			}

			if (push_to_buffer (
				http, &http->req.raw, req, reqlen) <= -1) return -1;

			if (blank)
			{
				/* blank line - we got a complete request. 
				 * we didn't process the optinal message body yet, though */

				/* DO SOMETHIGN ... */
qse_printf  (QSE_T("[[[%.*S]]]]\n"), (int)http->req.raw.size, http->req.raw.data), 

				clear_buffer (http, &http->req.raw);
			}

			req = ptr; /* let ptr point to the next character to '\n' */
		}
	}

	if (ptr > req)
	{
		/* enbuffer the unfinished data */
		if (push_to_buffer (http, &http->req.raw, req, ptr - req) <= -1) return -1;
		http->state.pending = ptr - req;
	}
#endif
	
	return 0;
}
