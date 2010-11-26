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

static QSE_INLINE int is_purespace_octet (qse_byte_t c)
{
	return c == ' ' || c == '\t';
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
		/* TODO: enhance the resizing scheme */
		qse_size_t ncapa = (nsize > (octb)->capa * 2)? nsize: ((octb)->capa * 2);
		
		do
		{
			void* tmp = QSE_MMGR_REALLOC (
				(http)->mmgr, (octb)->data, ncapa * QSE_SIZEOF(*ptr)
			);
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

struct hdr_cmb_t
{
	struct hdr_cmb_t* next;
};

static QSE_INLINE void clear_combined_headers (qse_http_t* http)
{
	struct hdr_cmb_t* cmb = (struct hdr_cmb_t*)http->req.hdr.combined;	
	
	while (cmb)
	{	
		struct hdr_cmb_t* next = cmb->next;
		QSE_MMGR_FREE (http->mmgr, cmb);
		cmb = next;
	}

	http->req.hdr.combined = QSE_NULL;
}

static QSE_INLINE void clear_request (qse_http_t* http)
{
	/* clear necessary part of the request before 
	 * reading the next request */
	QSE_MEMSET (&http->req.state, 0, QSE_SIZEOF(http->req.state));
	QSE_MEMSET (&http->req.attr, 0, QSE_SIZEOF(http->req.attr));
	qse_htb_clear (&http->req.hdr.tab);
	clear_combined_headers (http);
	clear_buffer (http, &http->req.con);
	clear_buffer (http, &http->req.raw);
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

	init_buffer (http, &http->req.raw);
	init_buffer (http, &http->req.con);

	if (qse_htb_init (&http->req.hdr.tab, mmgr, 60, 70, 1, 1) == QSE_NULL) 
	{
		fini_buffer (http, &http->req.raw);
		return QSE_NULL;
	}

	return http;
}

void qse_http_fini (qse_http_t* http)
{
	qse_htb_fini (&http->req.hdr.tab);
	clear_combined_headers (http);
	fini_buffer (http, &http->req.con);
	fini_buffer (http, &http->req.raw);
}

static qse_byte_t* parse_reqline (qse_http_t* http, qse_byte_t* line)
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
	http->req.path.ptr = p; 
	http->req.args.ptr = QSE_NULL;

	tmp = p;
	while (*p != '\0' && !is_space_octet(*p)) 
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
			if (!http->req.args.ptr)
			{
				/* ? must be explicit to be a argument instroducer. 
				 * %3f is just a literal. */
				http->req.path.len = tmp - http->req.path.ptr;
				/**tmp++ = '\0';*/
				http->req.args.ptr = tmp;
				p++;
			}
			else *tmp++ = *p++;
		}
		else *tmp++ = *p++;
	}

	/* the url must be followed by a space */
	if (!is_space_octet(*p)) goto badreq;

	if (http->req.args.ptr)
		http->req.args.len = tmp - http->req.args.ptr;
	else
		http->req.path.len = tmp - http->req.path.ptr;
	/* *tmp = '\0'; */ /* null-terminate the url part */

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

	/* adjust Connection: close for HTTP 1.0 or eariler */
	if (http->req.version.major < 1 || 
	    (http->req.version.major == 1 && http->req.version.minor == 0))
	{
		http->req.attr.connection_close = 1;
	}

qse_printf (QSE_T("parse_reqline ....\n"));
	return ++p;

badreq:
qse_printf (QSE_T("BADREQ\n"));
	http->errnum = QSE_HTTP_EBADREQ;
	return QSE_NULL;
}

void qse_http_clear (qse_http_t* http)
{
	clear_request (http);
}

#define octet_tolower(c) (((c) >= 'A' && (c) <= 'Z') ? ((c) | 0x20) : (c))
#define octet_toupper(c) (((c) >= 'a' && (c) <= 'z') ? ((c) & ~0x20) : (c))

static QSE_INLINE int compare_octets (
     const qse_byte_t* s1, qse_size_t len1,
     const qse_byte_t* s2, qse_size_t len2)
{
	qse_char_t c1, c2;
	const qse_byte_t* end1 = s1 + len1;
	const qse_byte_t* end2 = s2 + len2;

	while (s1 < end1)
	{
		c1 = octet_toupper (*s1);
		if (s2 < end2)
		{
			c2 = octet_toupper (*s2);
			if (c1 > c2) return 1;
			if (c1 < c2) return -1;
		}
		else return 1;
		s1++; s2++;
	}

	return (s2 < end2)? -1: 0;
}

static QSE_INLINE int capture_connection (
	qse_http_t* http, qse_htb_pair_t* pair)
{
	int n;

	n = compare_octets (pair->vptr, pair->vlen, "close", 5);
	if (n == 0)
	{
		http->req.attr.connection_close = 1;
		return 0;
	}

	/* don't care about other values */
	return 0;
}

static QSE_INLINE int capture_content_length (
	qse_http_t* http, qse_htb_pair_t* pair)
{
	qse_size_t len = 0, off = 0, tmp;
	const qse_byte_t* ptr = pair->vptr;

	while (off < pair->vlen)
	{
		int num = digit_to_num (ptr[off]);
		if (num <= -1)
		{
			/* the length contains a non-digit */
			http->errnum = QSE_HTTP_EBADREQ;
			return -1;
		}

		tmp = len * 10 + num;
		if (tmp < len)
		{
			/* the length has overflown */
			http->errnum = QSE_HTTP_EBADREQ;
			return -1;
		}

		len = tmp;
		off++;
	}

	if (off == 0)
	{
		/* no length was provided */
		http->errnum = QSE_HTTP_EBADREQ;
		return -1;
	}

	if (http->req.attr.chunked && len > 0)
	{
		/* content-length is greater than 0 
		 * while transfer-encoding: chunked is specified. */
		http->errnum = QSE_HTTP_EBADREQ;
		return -1;
	}

	http->req.attr.content_length = len;
	return 0;
}

static QSE_INLINE int capture_content_type (
	qse_http_t* http, qse_htb_pair_t* pair)
{
	qse_printf (QSE_T("content type capture => %.*S\n"), (int)pair->vlen, pair->vptr);
	return 0;
}

static QSE_INLINE int capture_host (
	qse_http_t* http, qse_htb_pair_t* pair)
{
	qse_printf (QSE_T("host capture => %.*S\n"), (int)pair->vlen, pair->vptr);
	return 0;
}

static QSE_INLINE int capture_transfer_encoding (
	qse_http_t* http, qse_htb_pair_t* pair)
{
	int n;

	n = compare_octets (pair->vptr, pair->vlen, "chunked", 7);
	if (n == 0)
	{
		if (http->req.attr.content_length > 0)
		{
			/* content-length is greater than 0 
			 * while transfer-encoding: chunked is specified. */
			goto badreq;
		}

		http->req.attr.chunked = 1;
		return 0;
	}

	/* other encoding type not supported yet */
badreq:
	http->errnum = QSE_HTTP_EBADREQ;
	return -1;
}

static QSE_INLINE int capture_key_header (
	qse_http_t* http, qse_htb_pair_t* pair)
{
	static struct
	{
		const qse_byte_t* ptr;
		qse_size_t        len;
		int (*handler) (qse_http_t*, qse_htb_pair_t*);
	} hdrtab[] = 
	{
		{ "Connection",         10, capture_connection },
		{ "Content-Length",     14, capture_content_length },
		{ "Content-Type",       12, capture_content_type },
		{ "Host",               4,  capture_host },
		{ "Transfer-Encoding",  17, capture_transfer_encoding  }
	};

	int n;
	qse_size_t mid, count, base = 0;

	/* perform binary search */
	for (count = QSE_COUNTOF(hdrtab); count > 0; count /= 2)
	{
		mid = base + count / 2;

		n = compare_octets (
			pair->kptr, pair->klen,
			hdrtab[mid].ptr, hdrtab[mid].len
		);

		if (n == 0)
		{
			/* bingo! */
			return hdrtab[mid].handler (http, pair);
		}

		if (n > 0) { base = mid + 1; count--; }
	}

	/* No callback functions were interested in this header field. */
	return 0;
}

struct hdr_cbserter_ctx_t
{
	qse_http_t* http;
	void* vptr;
	qse_size_t vlen;
};

static qse_htb_pair_t* hdr_cbserter (
	qse_htb_t* htb, qse_htb_pair_t* pair, 
	void* kptr, qse_size_t klen, void* ctx)
{
	struct hdr_cbserter_ctx_t* tx = (struct hdr_cbserter_ctx_t*)ctx;

	if (pair == QSE_NULL)
	{
		/* the key is new. let's create a new pair. */
		qse_htb_pair_t* p; 

		p = qse_htb_allocpair (htb, kptr, klen, tx->vptr, tx->vlen);

		if (p == QSE_NULL) tx->http->errnum = QSE_HTTP_ENOMEM;
		else 
		{
			if (capture_key_header (tx->http, p) <= -1)
			{
				/* Destroy the pair created here
				 * as it is not added to the hash table yet */
				qse_htb_freepair (htb, p);
				p = QSE_NULL;
			}
		}

		return p;
	}
	else
	{
		/* the key exists. let's combine values, each separated 
		 * by a comma */
		struct hdr_cmb_t* cmb;
		qse_byte_t* ptr;
		qse_size_t len;

		/* TODO: reduce waste in case the same key appears again.
		 *
		 *  the current implementation is not space nor performance 
		 *  efficient. it allocates a new buffer again whenever it
		 *  encounters the same key. memory is wasted and performance
		 *  is sacrificed. 

		 *  hopefully, a http header does not include a lot of 
		 *  duplicate fields and this implmentation can afford wastage.
		 */

		/* allocate a block to combine the existing value and the new value */
		cmb = (struct hdr_cmb_t*) QSE_MMGR_ALLOC (
			tx->http->mmgr, 
			QSE_SIZEOF(*cmb) + 
			QSE_SIZEOF(qse_byte_t) * (pair->vlen + 1 + tx->vlen + 1)
		);
		if (cmb == QSE_NULL)
		{
			tx->http->errnum = QSE_HTTP_ENOMEM;
			return QSE_NULL;
		}

		/* let 'ptr' point to the actual space for the combined value */
		ptr = (qse_byte_t*)(cmb + 1);
		len = 0;

		/* fill the space with the value */
		QSE_MEMCPY (&ptr[len], pair->vptr, pair->vlen);
		len += pair->vlen;
		ptr[len++] = ',';
		QSE_MEMCPY (&ptr[len], tx->vptr, tx->vlen);
		len += tx->vlen;
		ptr[len] = '\0';

#if 0
TODO:
Not easy to unlink when using a singly linked list...
Change it to doubly linked for this?

		/* let's destroy the old buffer at least */
		if (!(ptr >= tx->http->req.raw.data && ptr < 
		      &tx->http->req.raw.data[tx->http->req.raw.size]))
		{
			/* NOTE the range check in 'if' assumes that raw.data is never
			 * relocated for resizing */

			QSE_MMGR_FREE (
				tx->http->mmgr, 
				((struct hdr_cmb_t*)pair->vptr) - 1
			);
		}
#endif
		
		/* update the value pointer and length */
		pair->vptr = ptr;
		pair->vlen = len;

		/* link the new combined value block */
		cmb->next = tx->http->req.hdr.combined;
		tx->http->req.hdr.combined = cmb;

		if (capture_key_header (tx->http, pair) <= -1) return QSE_NULL;

		return pair;
	}
}

qse_byte_t* parse_header_fields (qse_http_t* http, qse_byte_t* line)
{
	qse_byte_t* p = line, * last;
	struct
	{
		qse_byte_t* ptr;
		qse_size_t  len;
	} name, value;

#if 0
	/* ignore leading spaces excluding crlf */
	while (is_space_octet(*p)) p++;
#endif

	QSE_ASSERT (!is_whspace_octet(*p));

	/* check the field name */
	name.ptr = last = p;
	while (*p != '\0' && *p != '\n' && *p != ':')
	{
		if (!is_space_octet(*p++)) last = p;
	}
	name.len = last - name.ptr;

	if (*p != ':') goto badhdr;
	*last = '\0';

	/* skip the colon and spaces after it */
	do { p++; } while (is_space_octet(*p));

	value.ptr = last = p;
	while (*p != '\0' && *p != '\n')
	{
		if (!is_space_octet(*p++)) last = p;
	}

	value.len = last - value.ptr;
	if (*p != '\n') goto badhdr; /* not ending with a new line */

	/* peep at the beginning of the next line to check if it is 
	 * the continuation */
	if (is_purespace_octet (*++p))
	{
		qse_byte_t* cpydst;

		cpydst = p - 1;
		if (*(cpydst-1) == '\r') cpydst--;

		/* process all continued lines */
		do 
		{
			while (*p != '\0' && *p != '\n')
			{
				*cpydst = *p++; 
				if (!is_space_octet(*cpydst++)) last = cpydst;
			} 
	
			value.len = last - value.ptr;
			if (*p != '\n') goto badhdr;

			if (*(cpydst-1) == '\r') cpydst--;
		}
		while (is_purespace_octet(*++p));
	}
	*last = '\0';

	/* insert the new field to the header table */
	{
		struct hdr_cbserter_ctx_t ctx;

		ctx.http = http;
		ctx.vptr = value.ptr;
		ctx.vlen = value.len;

		http->errnum = QSE_HTTP_ENOERR;
		if (qse_htb_cbsert (
			&http->req.hdr.tab, name.ptr, name.len, 
			hdr_cbserter, &ctx) == QSE_NULL)
		{
			if (http->errnum == QSE_HTTP_ENOERR) 
				http->errnum = QSE_HTTP_ENOMEM;
			return QSE_NULL;
		}
	}

	return p;

badhdr:
qse_printf (QSE_T("BADHDR\n"),  name.ptr, value.ptr);
	http->errnum = QSE_HTTP_EBADHDR;
	return QSE_NULL;
}

static qse_htb_walk_t walk (qse_htb_t* htb, qse_htb_pair_t* pair, void* ctx)
{
qse_printf (QSE_T("HEADER OK %d[%S] %d[%S]\n"),  (int)pair->klen, pair->kptr, (int)pair->vlen, pair->vptr);
	return QSE_HTB_WALK_FORWARD;
}

static QSE_INLINE int parse_request (
	qse_http_t* http, const qse_byte_t* req, qse_size_t rlen)
{
	static const qse_byte_t nul = '\0';
	qse_byte_t* p;

	/* add the actual request */
	if (push_to_buffer (http, &http->req.raw, req, rlen) <= -1) return -1;

	/* add the terminating null for easier parsing */
	if (push_to_buffer (http, &http->req.raw, &nul, 1) <= -1) return -1;

	p = http->req.raw.data;

	while (is_whspace_octet(*p)) p++;
	QSE_ASSERT (*p != '\0');

	/* parse the request line */
	p = parse_reqline (http, p);
	if (p == QSE_NULL) return -1;

	/* parse header fields */
	do
	{
		while (is_whspace_octet(*p)) p++;
		if (*p == '\0') break;

		/* TODO: return error if protocol is 0.9.
		 * HTTP/0.9 must not get headers... */

		p = parse_header_fields (http, p);
		if (p == QSE_NULL) return -1;
	}
	while (1);
		
	return 0;
}

/* chunk parsing phases */
#define GET_CHUNK_DONE     0
#define GET_CHUNK_LEN      1
#define GET_CHUNK_DATA     2
#define GET_CHUNK_CRLF     3
#define GET_CHUNK_TRAILERS 4

static const qse_byte_t* getchunklen (qse_http_t* http, const qse_byte_t* ptr, qse_size_t len)
{
	const qse_byte_t* end = ptr + len;

	/* this function must be called in the GET_CHUNK_LEN context */
	QSE_ASSERT (http->req.state.chunk.phase == GET_CHUNK_LEN);

//qse_printf (QSE_T("CALLING getchunklen [%d]\n"), *ptr);
	if (http->req.state.chunk.count <= 0)
	{
		/* skip leading spaces if the first character of
		 * the chunk length has not been read yet */
		while (ptr < end && is_space_octet(*ptr)) ptr++;
	}

	while (ptr < end)
	{
		int n = xdigit_to_num (*ptr);
		if (n <= -1) break;

		http->req.state.chunk.len = http->req.state.chunk.len * 16 + n;
		http->req.state.chunk.count++;
		ptr++;
	}

	/* skip trailing spaces if the length has been read */
	while (ptr < end && is_space_octet(*ptr)) ptr++;

	if (ptr < end)
	{
		if (*ptr == '\n') 
		{
			/* the chunk length line ended properly */

			if (http->req.state.chunk.count <= 0)
			{
				/* empty line - no more chunk */
//qse_printf (QSE_T("empty line chunk done....\n"));
				http->req.state.chunk.phase = GET_CHUNK_DONE;
			}
			else if (http->req.state.chunk.len <= 0)
			{
				/* length explicity specified to 0
				   get trailing headers .... */
				/*TODO: => http->req.state.chunk.phase = GET_CHUNK_TRAILERS;*/
				http->req.state.chunk.phase = GET_CHUNK_DATA;
			}
			else
			{
				/* ready to read the chunk data... */
				http->req.state.chunk.phase = GET_CHUNK_DATA;
//qse_printf (QSE_T("SWITCH TO GET_CHUNK_DATA....\n"));
			}

			http->req.state.need = http->req.state.chunk.len;
			ptr++;
		}
		else
		{
//qse_printf (QSE_T("XXXXXXXXXXXXXXXXXxxx [%c]\n"), *ptr);
			http->errnum = QSE_HTTP_EBADREQ;
			return QSE_NULL;
		}
	}

	return ptr;
}

/* feed the percent encoded string */
int qse_http_feed (qse_http_t* http, const qse_byte_t* ptr, qse_size_t len)
{
	const qse_byte_t* end = ptr + len;
	const qse_byte_t* req = ptr;

	/* does this goto drop code maintainability? */
	if (http->req.state.need > 0) goto content_resume;
	switch (http->req.state.chunk.phase)
	{
		case GET_CHUNK_LEN:
			goto dechunk_resume;

		case GET_CHUNK_DATA:
			/* this won't be reached as http->req.state.need 
			 * is greater than 0 if GET_CHUNK_DATA is true */
			goto content_resume;

		case GET_CHUNK_CRLF:
			goto dechunk_crlf;

		/*
		case GET_CHUNK_TRAILERS:
			goto ....
		*/
	}

	while (ptr < end)
	{
		register qse_byte_t b = *ptr++;

		if (http->req.state.plen <= 0 && is_whspace_octet(b)) 
		{
			/* let's drop leading whitespaces across multiple
			 * lines */
			req++;
			continue;
		}

		if (b == '\n')
		{
			if (http->req.state.crlf <= 1) 
			{
				/* http->req.state.crlf == 0, CR was not seen
				 * http->req.state.crlf == 1, CR was seen 
				 * whatever the current case is, mark the 
				 * first LF is seen here.
				 */
				http->req.state.crlf = 2;
			}
			else
			{
				/* http->req.state.crlf == 2, no 2nd CR before LF
				 * http->req.state.crlf == 3, 2nd CR before LF
				 */

				/* we got a complete request. */
				QSE_ASSERT (http->req.state.crlf <= 3);

				/* reset the crlf state */
				http->req.state.crlf = 0;
				/* reset the raw request length */
				http->req.state.plen = 0;

				if (parse_request (http, req, ptr - req) <= -1)
					return -1;

				if (http->req.attr.chunked)
				{
					/* transfer-encoding: chunked */
					QSE_ASSERT (http->req.attr.content_length <= 0);

				dechunk_start:
					http->req.state.chunk.phase = GET_CHUNK_LEN;
					http->req.state.chunk.len = 0;
					http->req.state.chunk.count = 0;

				dechunk_resume:
					ptr = getchunklen (http, ptr, end - ptr);
					if (ptr == QSE_NULL) return -1;

					if (http->req.state.chunk.phase == GET_CHUNK_LEN)
					{
						/* still in the GET_CHUNK_LEN state.
						 * the length has been partially read. */
						goto feedme_more;
					}
				}
				else
				{
					/* we need to read as many octets as Content-Length */
					http->req.state.need = http->req.attr.content_length;
				}

				if (http->req.state.need > 0)
				{
					/* content-length or chunked data length specified */

					qse_size_t avail;

				content_resume:
					avail = end - ptr;

					if (avail < http->req.state.need)
					{
						/* the data is not as large as needed */
						if (push_to_buffer (http, &http->req.con, ptr, avail) <= -1) return -1;
						http->req.state.need -= avail;
						/* we didn't get a complete content yet */
						goto feedme_more; 
					}
					else 
					{
						/* we are given all needed or more than needed */
						if (push_to_buffer (http, &http->req.con, ptr, http->req.state.need) <= -1) return -1;
						ptr += http->req.state.need;
						http->req.state.need = 0;
					}
				}

				if (http->req.state.chunk.phase == GET_CHUNK_DATA)
				{
					QSE_ASSERT (http->req.state.need == 0);
					http->req.state.chunk.phase = GET_CHUNK_CRLF;

				dechunk_crlf:
					while (ptr < end && is_space_octet(*ptr)) ptr++;
					if (ptr < end)
					{
						if (*ptr == '\n') 
						{
							/* end of chunk data. */
							ptr++;

							/* more octets still available. 
							 * let it decode the next chunk */
							if (ptr < end) goto dechunk_start; 
						
							/* no more octets available after chunk data.
							 * the chunk state variables need to be
							 * reset when a jump is made to dechunk_resume
							 * upon the next call */
							http->req.state.chunk.phase = GET_CHUNK_LEN;
							http->req.state.chunk.len = 0;
							http->req.state.chunk.count = 0;

							goto feedme_more;
						}
						else
						{
							/* redundant character ... */
							http->errnum = QSE_HTTP_EBADREQ;
							return -1;
						}
					}
					else
					{
						/* data not enough */
						goto feedme_more;
					}
				}

qse_htb_walk (&http->req.hdr.tab, walk, QSE_NULL);
if (http->req.con.size > 0)
{
	qse_printf (QSE_T("content = [%.*S]\n"), (int)http->req.con.size, http->req.con.data);
}
/* TODO: do the main job here... before the raw buffer is cleared out... */

				clear_request (http);

				/* let ptr point to the next character to LF or the optional contents */
				req = ptr; 
			}
		}
		else if (b == '\r')
		{
			if (http->req.state.crlf == 0 || http->req.state.crlf == 2) 
				http->req.state.crlf++;
			else http->req.state.crlf = 1;
		}
		else if (b == '\0')
		{
			/* guarantee that the request does not contain a null 
			 * character */
			http->errnum = QSE_HTTP_EBADREQ;
			return -1;
		}
		else
		{
			/* increment length of a request in raw 
			 * excluding crlf */
			http->req.state.plen++; 
			/* mark that neither CR nor LF was seen */
			http->req.state.crlf = 0;
		}
	}

	if (ptr > req)
	{
		/* enbuffer the incomplete request */
		if (push_to_buffer (http, &http->req.raw, req, ptr - req) <= -1) return -1;
	}

feedme_more:
	return 0;
}

