/*
 * $Id: http.c 341 2008-08-20 10:58:19Z baconevi $
 * 
    Copyright 2006-2011 Chung, Hyung-Hwan.
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

#include <qse/http/http.h>
#include <qse/cmn/chr.h>
#include "../cmn/mem.h"

QSE_IMPLEMENT_COMMON_FUNCTIONS (http)

static const qse_byte_t NUL = '\0';

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
	struct hdr_cmb_t* cmb = (struct hdr_cmb_t*)http->reqx.chl;	
	
	while (cmb)
	{	
		struct hdr_cmb_t* next = cmb->next;
		QSE_MMGR_FREE (http->mmgr, cmb);
		cmb = next;
	}

	http->reqx.chl = QSE_NULL;
}

static QSE_INLINE void clear_request (qse_http_t* http)
{
	/* clear necessary part of the request before 
	 * reading the next request */
	QSE_MEMSET (&http->req.attr, 0, QSE_SIZEOF(http->req.attr));

	qse_htb_clear (&http->req.hdrtab);
	clear_combined_headers (http);

	clear_buffer (http, &http->req.con);
	clear_buffer (http, &http->reqx.b.tra);
	clear_buffer (http, &http->reqx.b.raw);

	QSE_MEMSET (&http->reqx.s, 0, QSE_SIZEOF(http->reqx.s));
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

	init_buffer (http, &http->reqx.b.raw);
	init_buffer (http, &http->reqx.b.tra);
	init_buffer (http, &http->reqx.b.pen);
	init_buffer (http, &http->req.con);

	if (qse_htb_init (&http->req.hdrtab, mmgr, 60, 70, 1, 1) == QSE_NULL) 
	{
		fini_buffer (http, &http->req.con);
		fini_buffer (http, &http->reqx.b.pen);
		fini_buffer (http, &http->reqx.b.tra);
		fini_buffer (http, &http->reqx.b.raw);
		return QSE_NULL;
	}

	return http;
}

void qse_http_fini (qse_http_t* http)
{
	qse_htb_fini (&http->req.hdrtab);
	clear_combined_headers (http);
	fini_buffer (http, &http->req.con);
	fini_buffer (http, &http->reqx.b.pen);
	fini_buffer (http, &http->reqx.b.tra);
	fini_buffer (http, &http->reqx.b.raw);
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
				*tmp++ = '\0';
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
	/* null-terminate the url part though we record the length */
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

int qse_http_getoption (qse_http_t* http)
{
	return http->option;
}

void qse_http_setoption (qse_http_t* http, int opts)
{
	http->option = opts;
}

const qse_http_reqcbs_t* qse_http_getreqcbs (qse_http_t* http)
{
	return http->reqcbs;
}

void qse_http_setreqcbs (qse_http_t* http, const qse_http_reqcbs_t* reqcbs)
{
	http->reqcbs = reqcbs;
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

	n = compare_octets (QSE_HTB_VPTR(pair), QSE_HTB_VLEN(pair), "close", 5);
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
	const qse_byte_t* ptr = QSE_HTB_VPTR(pair);

	while (off < QSE_HTB_VLEN(pair))
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
	qse_printf (QSE_T("content type capture => %.*S\n"), (int)QSE_HTB_VLEN(pair), QSE_HTB_VPTR(pair));
	return 0;
}

static QSE_INLINE int capture_host (
	qse_http_t* http, qse_htb_pair_t* pair)
{
	qse_printf (QSE_T("host capture => %.*S\n"), (int)QSE_HTB_VLEN(pair), QSE_HTB_VPTR(pair));
	return 0;
}

static QSE_INLINE int capture_transfer_encoding (
	qse_http_t* http, qse_htb_pair_t* pair)
{
	int n;

	n = compare_octets (QSE_HTB_VPTR(pair), QSE_HTB_VLEN(pair), "chunked", 7);
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
			QSE_HTB_VPTR(pair), QSE_HTB_VLEN(pair),
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
	void*       vptr;
	qse_size_t  vlen;
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
			QSE_SIZEOF(qse_byte_t) * (QSE_HTB_VLEN(pair) + 1 + tx->vlen + 1)
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
		QSE_MEMCPY (&ptr[len], QSE_HTB_VPTR(pair), QSE_HTB_VLEN(pair));
		len += QSE_HTB_VLEN(pair);
		ptr[len++] = ',';
		QSE_MEMCPY (&ptr[len], tx->vptr, tx->vlen);
		len += tx->vlen;
		ptr[len] = '\0';

#if 0
TODO:
Not easy to unlink when using a singly linked list...
Change it to doubly linked for this?

		/* let's destroy the old buffer at least */
		if (!(ptr >= tx->http->reqx.b.raw.data && ptr < 
		      &tx->http->reqx.b.raw.data[tx->http->reqx.b.raw.size]))
		{
			/* NOTE the range check in 'if' assumes that raw.data is never
			 * relocated for resizing */

			QSE_MMGR_FREE (
				tx->http->mmgr, 
				((struct hdr_cmb_t*)QSE_HTB_VPTR(pair)) - 1
			);
		}
#endif
		
		/* update the value pointer and length */
		QSE_HTB_VPTR(pair) = ptr;
		QSE_HTB_VLEN(pair) = len;

		/* link the new combined value block */
		cmb->next = tx->http->reqx.chl;
		tx->http->reqx.chl = cmb;

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
			&http->req.hdrtab, name.ptr, name.len, 
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
qse_printf (QSE_T("HEADER OK %d[%S] %d[%S]\n"),  (int)QSE_HTB_KLEN(pair), QSE_HTB_KPTR(pair), (int)QSE_HTB_VLEN(pair), QSE_HTB_VPTR(pair));
	return QSE_HTB_WALK_FORWARD;
}

static QSE_INLINE int parse_request (
	qse_http_t* http, const qse_byte_t* req, qse_size_t rlen)
{
	qse_byte_t* p;

	/* add the actual request */
	if (push_to_buffer (http, &http->reqx.b.raw, req, rlen) <= -1) return -1;

	/* add the terminating null for easier parsing */
	if (push_to_buffer (http, &http->reqx.b.raw, &NUL, 1) <= -1) return -1;

	p = http->reqx.b.raw.data;

	if (http->option & QSE_HTTP_LEADINGEMPTYLINES)
		while (is_whspace_octet(*p)) p++;
	else
		while (is_space_octet(*p)) p++;
	
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
	QSE_ASSERT (http->reqx.s.chunk.phase == GET_CHUNK_LEN);

//qse_printf (QSE_T("CALLING getchunklen [%d]\n"), *ptr);
	if (http->reqx.s.chunk.count <= 0)
	{
		/* skip leading spaces if the first character of
		 * the chunk length has not been read yet */
		while (ptr < end && is_space_octet(*ptr)) ptr++;
	}

	while (ptr < end)
	{
		int n = xdigit_to_num (*ptr);
		if (n <= -1) break;

		http->reqx.s.chunk.len = http->reqx.s.chunk.len * 16 + n;
		http->reqx.s.chunk.count++;
		ptr++;
	}

	/* skip trailing spaces if the length has been read */
	while (ptr < end && is_space_octet(*ptr)) ptr++;

	if (ptr < end)
	{
		if (*ptr == '\n') 
		{
			/* the chunk length line ended properly */

			if (http->reqx.s.chunk.count <= 0)
			{
				/* empty line - no more chunk */
//qse_printf (QSE_T("empty line chunk done....\n"));
				http->reqx.s.chunk.phase = GET_CHUNK_DONE;
			}
			else if (http->reqx.s.chunk.len <= 0)
			{
				/* length explicity specified to 0
				   get trailing headers .... */
				http->reqx.s.chunk.phase = GET_CHUNK_TRAILERS;
//qse_printf (QSE_T("SWITCH TO GET_CHUNK_TRAILERS....\n"));
			}
			else
			{
				/* ready to read the chunk data... */
				http->reqx.s.chunk.phase = GET_CHUNK_DATA;
//qse_printf (QSE_T("SWITCH TO GET_CHUNK_DATA....\n"));
			}

			http->reqx.s.need = http->reqx.s.chunk.len;
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

static const qse_byte_t* get_trailing_headers (
	qse_http_t* http, const qse_byte_t* req, const qse_byte_t* end)
{
	const qse_byte_t* ptr = req;

	while (ptr < end)
	{
		register qse_byte_t b = *ptr++;

		switch (b)
		{
			case '\0':
				/* guarantee that the request does not contain a null 
				 * character */
				http->errnum = QSE_HTTP_EBADREQ;
				return -1;

			case '\n':
				if (http->reqx.s.crlf <= 1) 
				{
					http->reqx.s.crlf = 2;
					break;
				}
				else
				{
					qse_byte_t* p;
	
					QSE_ASSERT (http->reqx.s.crlf <= 3);
					http->reqx.s.crlf = 0;
	
					if (push_to_buffer (
						http, &http->reqx.b.tra, req, ptr - req) <= -1)
						return QSE_NULL;
					if (push_to_buffer (
						http, &http->reqx.b.tra, &NUL, 1) <= -1) 
						return QSE_NULL;
	
					p = http->reqx.b.tra.data;
	
					do
					{
						while (is_whspace_octet(*p)) p++;
						if (*p == '\0') break;
	
						/* TODO: return error if protocol is 0.9.
						 * HTTP/0.9 must not get headers... */
	
						p = parse_header_fields (http, p);
						if (p == QSE_NULL) return QSE_NULL;
					}
					while (1);

					http->reqx.s.chunk.phase = GET_CHUNK_DONE;
					goto done;
				}

			case '\r':
				if (http->reqx.s.crlf == 0 || http->reqx.s.crlf == 2) 
					http->reqx.s.crlf++;
				else http->reqx.s.crlf = 1;
				break;

			default:
				/* mark that neither CR nor LF was seen */
				http->reqx.s.crlf = 0;
		}
	}

	if (push_to_buffer (http, &http->reqx.b.tra, req, ptr - req) <= -1) 
		return QSE_NULL;

done:
	return ptr;
}


/* feed the percent encoded string */
int qse_http_feed (qse_http_t* http, const qse_byte_t* req, qse_size_t len)
{
	const qse_byte_t* end = req + len;
	const qse_byte_t* ptr = req;

#if 0
	if (http->reqx.pending)
	{
	}	
#endif

	/* does this goto drop code maintainability? */
	if (http->reqx.s.need > 0) 
	{
		/* we're in need of as many octets as http->reqx.s.need 
		 * for contents body. make a proper jump to resume
		 * content handling */
		goto content_resume;
	}

	switch (http->reqx.s.chunk.phase)
	{
		case GET_CHUNK_LEN:
			goto dechunk_resume;

		case GET_CHUNK_DATA:
			/* this won't be reached as http->reqx.s.need 
			 * is greater than 0 if GET_CHUNK_DATA is true */
			goto content_resume;

		case GET_CHUNK_CRLF:
			goto dechunk_crlf;

		case GET_CHUNK_TRAILERS:
			goto dechunk_get_trailers;
	}

	while (ptr < end)
	{
		register qse_byte_t b = *ptr++;

		if (http->option & QSE_HTTP_LEADINGEMPTYLINES &&
		    http->reqx.s.plen <= 0 && is_whspace_octet(b)) 
		{
			/* let's drop leading whitespaces across multiple
			 * lines */
			req++;
			continue;
		}

		switch (b)
		{
			case '\0':
				/* guarantee that the request does not contain
				 * a null character */
				http->errnum = QSE_HTTP_EBADREQ;
				return -1;

			case '\n':
				if (http->reqx.s.crlf <= 1) 
				{
					/* http->reqx.s.crlf == 0
					 *   => CR was not seen
					 * http->reqx.s.crlf == 1
					 *   => CR was seen 
					 * whatever the current case is, 
					 * mark the first LF is seen here.
					 */
					http->reqx.s.crlf = 2;
				}
				else
				{
					/* http->reqx.s.crlf == 2
					 *   => no 2nd CR before LF
					 * http->reqx.s.crlf == 3
					 *   => 2nd CR before LF
					 */
					/* we got a complete request. */
					QSE_ASSERT (http->reqx.s.crlf <= 3);
	
					/* reset the crlf state */
					http->reqx.s.crlf = 0;
					/* reset the raw request length */
					http->reqx.s.plen = 0;
	
					if (parse_request (http, req, ptr - req) <= -1)
						return -1;
	
					if (http->req.attr.chunked)
					{
						/* transfer-encoding: chunked */
						QSE_ASSERT (http->req.attr.content_length <= 0);
	
					dechunk_start:
						http->reqx.s.chunk.phase = GET_CHUNK_LEN;
						http->reqx.s.chunk.len = 0;
						http->reqx.s.chunk.count = 0;
	
					dechunk_resume:
						ptr = getchunklen (http, ptr, end - ptr);
						if (ptr == QSE_NULL) return -1;
	
						if (http->reqx.s.chunk.phase == GET_CHUNK_LEN)
						{
							/* still in the GET_CHUNK_LEN state.
							 * the length has been partially read. */
							goto feedme_more;
						}
						else if (http->reqx.s.chunk.phase == GET_CHUNK_TRAILERS)
						{
						dechunk_get_trailers:
							ptr = get_trailing_headers (http, ptr, end);
							if (ptr == QSE_NULL) return -1;
	
							if (http->reqx.s.chunk.phase == GET_CHUNK_TRAILERS)
							{
								/* still in the same state.
								 * the trailers have not been processed fully */
								goto feedme_more;
							}
						}
					}
					else
					{
						/* we need to read as many octets as
						 * Content-Length */
						http->reqx.s.need = http->req.attr.content_length;
					}

					if (http->reqx.s.need > 0)
					{
						/* content-length or chunked data length 
						 * specified */

						qse_size_t avail;
	
					content_resume:
						avail = end - ptr;
	
						if (avail < http->reqx.s.need)
						{
							/* the data is not as large as needed */
							if (push_to_buffer (http, &http->req.con, ptr, avail) <= -1) return -1;
							http->reqx.s.need -= avail;
							/* we didn't get a complete content yet */
							goto feedme_more; 
						}
						else 
						{
							/* we got all or more than needed */
							if (push_to_buffer (
								http, &http->req.con, ptr, 
								http->reqx.s.need) <= -1) return -1;
							ptr += http->reqx.s.need;
							http->reqx.s.need = 0;
						}
					}
	
					if (http->reqx.s.chunk.phase == GET_CHUNK_DATA)
					{
						QSE_ASSERT (http->reqx.s.need == 0);
						http->reqx.s.chunk.phase = GET_CHUNK_CRLF;
	
					dechunk_crlf:
						while (ptr < end && is_space_octet(*ptr)) ptr++;
						if (ptr < end)
						{
							if (*ptr == '\n') 
							{
								/* end of chunk data. */
								ptr++;
	
								/* more octets still available. 
								 * let it decode the next chunk 
								 */
								if (ptr < end) goto dechunk_start; 
							
								/* no more octets available after 
								 * chunk data. the chunk state variables
								 * need to be reset when a jump is made
								 * to dechunk_resume upon the next call
								 */
								http->reqx.s.chunk.phase = GET_CHUNK_LEN;
								http->reqx.s.chunk.len = 0;
								http->reqx.s.chunk.count = 0;

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

					QSE_ASSERTX (http->reqcbs->request != QSE_NULL,
						"set request callbacks before feeding");
					http->errnum = QSE_HTTP_ENOERR;
					if (http->reqcbs->request (http, &http->req) <= -1)
					{
						if (http->errnum == QSE_HTTP_ENOERR)
							http->errnum = QSE_HTTP_EREQCBS;	

						/* need to clear request on error? 
						clear_request (http); */
						return -1;
					}

#if 0
qse_htb_walk (&http->req.hdrtab, walk, QSE_NULL);
if (http->req.con.size > 0)
{
	qse_printf (QSE_T("content = [%.*S]\n"), (int)http->req.con.size, http->req.con.data);
}
/* TODO: do the main job here... before the raw buffer is cleared out... */
#endif

					clear_request (http);

					/* let ptr point to the next character to LF or 
					 * the optional contents */
					req = ptr; 
				}
				break;

			case '\r':
				if (http->reqx.s.crlf == 0 || http->reqx.s.crlf == 2) 
					http->reqx.s.crlf++;
				else http->reqx.s.crlf = 1;
				break;

			default:
				/* increment length of a request in raw 
				 * excluding crlf */
				http->reqx.s.plen++; 
				/* mark that neither CR nor LF was seen */
				http->reqx.s.crlf = 0;
		}
	}

	if (ptr > req)
	{
		/* enbuffer the incomplete request */
		if (push_to_buffer (http, &http->reqx.b.raw, req, ptr - req) <= -1) return -1;
	}

feedme_more:
	return 0;
}

int qse_http_addtext (qse_http_t* http, const qse_byte_t* ptr, qse_size_t len)
{
}

int qse_http_addresource (qse_http_t* http, const void* ptr, qse_size_t len)
{
}

int qse_http_addheader (
	qse_http_t* http, const qse_byte_t* key, const qse_byte_t* val)
{
}

int qse_http_emit (qse_http_t* http)
{
}
