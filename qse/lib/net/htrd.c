/*
 * $Id$
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
    License along with QSE. If not, see <htrd://www.gnu.org/licenses/>.
 */

#include <qse/net/htrd.h>
#include <qse/cmn/chr.h>
#include "../cmn/mem.h"

QSE_IMPLEMENT_COMMON_FUNCTIONS (htrd)

static const qse_mchar_t NUL = QSE_MT('\0');

static QSE_INLINE int is_whspace_octet (qse_mchar_t c)
{
	return c == QSE_MT(' ') || c == QSE_MT('\t') || c == QSE_MT('\r') || c == QSE_MT('\n');
}

static QSE_INLINE int is_space_octet (qse_mchar_t c)
{
	return c == QSE_MT(' ') || c == QSE_MT('\t') || c == QSE_MT('\r');
}

static QSE_INLINE int is_purespace_octet (qse_mchar_t c)
{
	return c == QSE_MT(' ') || c == QSE_MT('\t');
}

static QSE_INLINE int is_upalpha_octet (qse_mchar_t c)
{
	return c >= QSE_MT('A') && c <= QSE_MT('Z');
}

static QSE_INLINE int is_loalpha_octet (qse_mchar_t c)
{
	return c >= QSE_MT('a') && c <= QSE_MT('z');
}

static QSE_INLINE int is_alpha_octet (qse_mchar_t c)
{
	return (c >= QSE_MT('A') && c <= QSE_MT('Z')) ||
	       (c >= QSE_MT('a') && c <= QSE_MT('z'));
}

static QSE_INLINE int is_digit_octet (qse_mchar_t c)
{
	return c >= QSE_MT('0') && c <= QSE_MT('9');
}

static QSE_INLINE int is_xdigit_octet (qse_mchar_t c)
{
	return (c >= QSE_MT('0') && c <= QSE_MT('9')) ||
	       (c >= QSE_MT('A') && c <= QSE_MT('F')) ||
	       (c >= QSE_MT('a') && c <= QSE_MT('f'));
}

static QSE_INLINE int digit_to_num (qse_mchar_t c)
{
	if (c >= QSE_MT('0') && c <= QSE_MT('9')) return c - QSE_MT('0');
	return -1;
}

static QSE_INLINE int xdigit_to_num (qse_mchar_t c)
{
	return QSE_MXDIGITTONUM (c);
}


static QSE_INLINE int push_to_buffer (
	qse_htrd_t* htrd, qse_htob_t* octb,
	const qse_mchar_t* ptr, qse_size_t len)
{
	if (qse_mbs_ncat (octb, ptr, len) == (qse_size_t)-1) 
	{
		htrd->errnum = QSE_HTRD_ENOMEM;
		return -1;
	}
	return 0;
}

struct hdr_cmb_t
{
	struct hdr_cmb_t* next;
};

static QSE_INLINE void clear_combined_headers (qse_htrd_t* htrd)
{
	struct hdr_cmb_t* cmb = (struct hdr_cmb_t*)htrd->fed.chl;	
	
	while (cmb)
	{	
		struct hdr_cmb_t* next = cmb->next;
		QSE_MMGR_FREE (htrd->mmgr, cmb);
		cmb = next;
	}

	htrd->fed.chl = QSE_NULL;
}

static QSE_INLINE void clear_feed (qse_htrd_t* htrd)
{
	/* clear necessary part of the request/response before 
	 * reading the next request/response */
	qse_htre_clear (&htrd->re);

	qse_mbs_clear (&htrd->fed.b.tra);
	qse_mbs_clear (&htrd->fed.b.raw);
	clear_combined_headers (htrd);

	QSE_MEMSET (&htrd->fed.s, 0, QSE_SIZEOF(htrd->fed.s));
}

#define QSE_HTRD_STATE_REQ  1
#define QSE_HTRD_STATE_HDR  2
#define QSE_HTRD_STATE_POST 3

qse_htrd_t* qse_htrd_open (qse_mmgr_t* mmgr, qse_size_t xtnsize)
{
	qse_htrd_t* htrd;

	htrd = (qse_htrd_t*) QSE_MMGR_ALLOC (
		mmgr, QSE_SIZEOF(qse_htrd_t) + xtnsize
	);
	if (htrd == QSE_NULL) return QSE_NULL;

	if (qse_htrd_init (htrd, mmgr) <= -1)
	{
		QSE_MMGR_FREE (htrd->mmgr, htrd);
		return QSE_NULL;
	}

	return htrd;
}

void qse_htrd_close (qse_htrd_t* htrd)
{
	qse_htrd_fini (htrd);
	QSE_MMGR_FREE (htrd->mmgr, htrd);
}

int qse_htrd_init (qse_htrd_t* htrd, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (htrd, 0, QSE_SIZEOF(*htrd));
	htrd->mmgr = mmgr;
	htrd->option = QSE_HTRD_REQUEST | QSE_HTRD_RESPONSE;

#if 0
	qse_mbs_init (&htrd->tmp.qparam, htrd->mmgr, 0);
#endif
	qse_mbs_init (&htrd->fed.b.raw, htrd->mmgr, 0);
	qse_mbs_init (&htrd->fed.b.tra, htrd->mmgr, 0);

	if (qse_htre_init (&htrd->re, mmgr) <= -1)
	{
		qse_mbs_fini (&htrd->fed.b.tra);
		qse_mbs_fini (&htrd->fed.b.raw);
#if 0
		qse_mbs_fini (&htrd->tmp.qparam);
#endif
		return -1;
	}

	return 0;
}

void qse_htrd_fini (qse_htrd_t* htrd)
{
	qse_htre_fini (&htrd->re);

	clear_combined_headers (htrd);
	qse_mbs_fini (&htrd->fed.b.tra);
	qse_mbs_fini (&htrd->fed.b.raw);
#if 0
	qse_mbs_fini (&htrd->tmp.qparam);
#endif
}

static qse_mchar_t* parse_initial_line (
	qse_htrd_t* htrd, qse_mchar_t* line)
{
	qse_mchar_t* p = line;
	qse_mcstr_t tmp;
	qse_http_method_t mtype;

#if 0
	/* ignore leading spaces excluding crlf */
	while (is_space_octet(*p)) p++;
#endif

	/* the method should start with an alphabet */
	if (!is_upalpha_octet(*p)) goto badre;

	/* get the method name */
	tmp.ptr = p;
	do { p++; } while (is_upalpha_octet(*p));
	tmp.len = p - tmp.ptr;

	htrd->retype = QSE_HTRD_RETYPE_Q;
	if ((htrd->option & QSE_HTRD_REQUEST) &&
	    qse_gethttpmethodtypefromstr (&tmp, &mtype) >= 0)
	{
		qse_htre_setqmethod (&htrd->re, mtype);
	}
	else if ((htrd->option & QSE_HTRD_RESPONSE) &&
	         qse_mbsxcmp (tmp.ptr, tmp.len, QSE_MT("HTTP")) == 0)
	{
		/* it begins with HTTP. it may be a response */
		htrd->retype = QSE_HTRD_RETYPE_S;
	}
	else goto badre;

	if (htrd->retype == QSE_HTRD_RETYPE_S)
	{
		int n, status;

		if (*p == QSE_MT('/') && p[1] != QSE_MT('\0') && p[2] == QSE_MT('.'))
		{
			int q = digit_to_num(p[1]);
			int w = digit_to_num(p[3]);
			if (q >= 0 && w >= 0)
			{
				htrd->re.version.major = q;
				htrd->re.version.minor = w;
				p += 4;
			}
			else goto badre;
		}
		else goto badre;

		/* htrd version must be followed by space */
		if (!is_space_octet(*p)) goto badre;

		/* skip spaces */
		do p++; while (is_space_octet(*p));
		
		n = digit_to_num(*p);
		if (n <= -1) goto badre;

		status = 0;
		do
		{
			status = status * 10 + n;
			p++;
		} 
		while ((n = digit_to_num(*p)) >= 0);

		qse_htre_setsstatus (&htrd->re, status);

		/* i don't treat the following weird messages as bad message
		 *    no status message follows the status code
		 *    no space between the status code and the status message
		 */

		/* skip spaces */
		while (is_space_octet(*p)) p++;
	
		tmp.ptr = p;
		while (*p != QSE_MT('\0') && *p != QSE_MT('\n')) p++;
		tmp.len = p - tmp.ptr;
	
		if (qse_htre_setsmessagefromcstr (&htrd->re, &tmp) <= -1) goto outofmem;

		/* adjust Connection: close for HTTP 1.0 or eariler */
		if (htrd->re.version.major < 1 || 
		    (htrd->re.version.major == 1 && htrd->re.version.minor == 0))
		{
			htrd->re.attr.connection_close = 1;
		}
	}
	else
	{
		qse_mchar_t* out;
		qse_mcstr_t param;

		/* method name must be followed by space */
		if (!is_space_octet(*p)) goto badre;

		/* skip spaces */
		do p++; while (is_space_octet(*p));

		/* process the url part */
		tmp.ptr = p; /* remember the beginning of path*/
		param.ptr = QSE_NULL;
	
		out = p;
		while (*p != QSE_MT('\0') && !is_space_octet(*p)) 
		{
			if (*p == QSE_MT('%') && param.ptr == QSE_NULL)
			{
				/* decode percence-encoded charaters in the 
				 * path part.  if we're in the parameter string
				 * part, we don't decode them. */

				int q = xdigit_to_num(*(p+1));
				int w = xdigit_to_num(*(p+2));
	
				if (q >= 0 && w >= 0)
				{
					int t = (q << 4) + w;
					if (t == 0)
					{
						/* percent enconding contains a null character */
						goto badre;
					}
	
					*out++ = t;
					p += 3;
				}
				else *out++ = *p++;
			}
			else if (*p == QSE_MT('?'))
			{
				if (!param.ptr)
				{
					/* ? must be explicit to be a argument instroducer. 
					 * %3f is just a literal. */
					tmp.len = out - tmp.ptr;
					*out++ = QSE_MT('\0'); /* null-terminate the path part */
					param.ptr = out;
					p++;
				}
				else *out++ = *p++;
			}
			else *out++ = *p++;
		}
	
		/* the url must be followed by a space */
		if (!is_space_octet(*p)) goto badre;
	
		/* null-terminate the url part though we know the length */
		*out = QSE_MT('\0'); 

		if (param.ptr)
		{
			param.len = out - param.ptr;
			if (qse_htre_setqparamfromcstr (&htrd->re, &param) <= -1) goto outofmem;
		}
		else tmp.len = out - tmp.ptr;

		if (qse_htre_setqpathfromcstr (&htrd->re, &tmp) <= -1) goto outofmem;

		/* skip spaces after the url part */
		do { p++; } while (is_space_octet(*p));
	
		/* check protocol version */
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
				htrd->re.version.major = q;
				htrd->re.version.minor = w;
				p += 8;
			}
			else goto badre;
		}
		else goto badre;
	
		/* skip trailing spaces on the line */
		while (is_space_octet(*p)) p++;

		/* adjust Connection: close for HTTP 1.0 or eariler */
		if (htrd->re.version.major < 1 || 
		    (htrd->re.version.major == 1 && htrd->re.version.minor == 0))
		{
			htrd->re.attr.connection_close = 1;
		}
	}
	
	/* if the line does not end with a new line, it is a bad request */
	if (*p != QSE_T('\n')) goto badre;
	
	return ++p;

badre:
	htrd->errnum = QSE_HTRD_EBADRE;
	return QSE_NULL;

outofmem:
	htrd->errnum = QSE_HTRD_ENOMEM;
	return QSE_NULL;

}

void qse_htrd_clear (qse_htrd_t* htrd)
{
	clear_feed (htrd);
}

int qse_htrd_getoption (qse_htrd_t* htrd)
{
	return htrd->option;
}

void qse_htrd_setoption (qse_htrd_t* htrd, int opts)
{
	htrd->option = opts;
}

const qse_htrd_recbs_t* qse_htrd_getrecbs (qse_htrd_t* htrd)
{
	return htrd->recbs;
}

void qse_htrd_setrecbs (qse_htrd_t* htrd, const qse_htrd_recbs_t* recbs)
{
	htrd->recbs = recbs;
}

#define octet_tolower(c) (((c) >= 'A' && (c) <= 'Z') ? ((c) | 0x20) : (c))
#define octet_toupper(c) (((c) >= 'a' && (c) <= 'z') ? ((c) & ~0x20) : (c))

static QSE_INLINE int compare_octets (
     const qse_mchar_t* s1, qse_size_t len1,
     const qse_mchar_t* s2, qse_size_t len2)
{
	qse_char_t c1, c2;
	const qse_mchar_t* end1 = s1 + len1;
	const qse_mchar_t* end2 = s2 + len2;

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
	qse_htrd_t* htrd, qse_htb_pair_t* pair)
{
	int n;

	n = compare_octets (QSE_HTB_VPTR(pair), QSE_HTB_VLEN(pair), "close", 5);
	if (n == 0)
	{
		htrd->re.attr.connection_close = 1;
		return 0;
	}

	n = compare_octets (QSE_HTB_VPTR(pair), QSE_HTB_VLEN(pair), "Keep-Alive", 10);
	if (n == 0)
	{
		htrd->re.attr.connection_close = 0;
		return 0;
	}

	/* don't care about other values */
	return 0;
}

static QSE_INLINE int capture_content_length (
	qse_htrd_t* htrd, qse_htb_pair_t* pair)
{
	qse_size_t len = 0, off = 0, tmp;
	const qse_mchar_t* ptr = QSE_HTB_VPTR(pair);

	while (off < QSE_HTB_VLEN(pair))
	{
		int num = digit_to_num (ptr[off]);
		if (num <= -1)
		{
			/* the length contains a non-digit */
			htrd->errnum = QSE_HTRD_EBADRE;
			return -1;
		}

		tmp = len * 10 + num;
		if (tmp < len)
		{
			/* the length has overflown */
			htrd->errnum = QSE_HTRD_EBADRE;
			return -1;
		}

		len = tmp;
		off++;
	}

	if (off == 0)
	{
		/* no length was provided */
		htrd->errnum = QSE_HTRD_EBADRE;
		return -1;
	}

	if (htrd->re.attr.chunked && len > 0)
	{
		/* content-length is greater than 0 
		 * while transfer-encoding: chunked is specified. */
		htrd->errnum = QSE_HTRD_EBADRE;
		return -1;
	}

	htrd->re.attr.content_length = len;
	htrd->re.attr.content_length_set = 1;
	return 0;
}

static QSE_INLINE int capture_expect (
	qse_htrd_t* htrd, qse_htb_pair_t* pair)
{
	int n;

	n = compare_octets (QSE_HTB_VPTR(pair), QSE_HTB_VLEN(pair), "100-continue", 12);
	if (n == 0)
	{
		
		htrd->re.attr.expect_continue = 1;
		return 0;
	}

	/* don't care about other values */
	return 0;
}

static QSE_INLINE int capture_transfer_encoding (
	qse_htrd_t* htrd, qse_htb_pair_t* pair)
{
	int n;

	n = compare_octets (QSE_HTB_VPTR(pair), QSE_HTB_VLEN(pair), "chunked", 7);
	if (n == 0)
	{
		/* if (htrd->re.attr.content_length > 0) */
		if (htrd->re.attr.content_length_set)
		{
			/* both content-length and 'transfer-encoding: chunked' are specified. */
			goto badre;
		}

		htrd->re.attr.chunked = 1;
		return 0;
	}

	/* other encoding type not supported yet */
badre:
	htrd->errnum = QSE_HTRD_EBADRE;
	return -1;
}

static QSE_INLINE int capture_key_header (
	qse_htrd_t* htrd, qse_htb_pair_t* pair)
{
	static struct
	{
		const qse_mchar_t* ptr;
		qse_size_t        len;
		int (*handler) (qse_htrd_t*, qse_htb_pair_t*);
	} hdrtab[] = 
	{
		{ "Connection",         10, capture_connection },
		{ "Content-Length",     14, capture_content_length },
		{ "Expect",             6,  capture_expect },
		{ "Transfer-Encoding",  17, capture_transfer_encoding  }
	};

	int n;
	qse_size_t mid, count, base = 0;

	/* perform binary search */
	for (count = QSE_COUNTOF(hdrtab); count > 0; count /= 2)
	{
		mid = base + count / 2;

		n = compare_octets (
			QSE_HTB_KPTR(pair), QSE_HTB_KLEN(pair),
			hdrtab[mid].ptr, hdrtab[mid].len
		);

		if (n == 0)
		{
			/* bingo! */
			return hdrtab[mid].handler (htrd, pair);
		}

		if (n > 0) { base = mid + 1; count--; }
	}

	/* No callback functions were interested in this header field. */
	return 0;
}

struct hdr_cbserter_ctx_t
{
	qse_htrd_t* htrd;
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

		if (p == QSE_NULL) tx->htrd->errnum = QSE_HTRD_ENOMEM;
		else 
		{
			if (capture_key_header (tx->htrd, p) <= -1)
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
		qse_mchar_t* ptr;
		qse_size_t len;

		/* TODO: reduce waste in case the same key appears again.
		 *
		 *  the current implementation is not space nor performance 
		 *  efficient. it allocates a new buffer again whenever it
		 *  encounters the same key. memory is wasted and performance
		 *  is sacrificed. 

		 *  hopefully, a htrd header does not include a lot of 
		 *  duplicate fields and this implmentation can afford wastage.
		 */

		/* allocate a block to combine the existing value and the new value */
		cmb = (struct hdr_cmb_t*) QSE_MMGR_ALLOC (
			tx->htrd->mmgr, 
			QSE_SIZEOF(*cmb) + 
			QSE_SIZEOF(qse_mchar_t) * (QSE_HTB_VLEN(pair) + 1 + tx->vlen + 1)
		);
		if (cmb == QSE_NULL)
		{
			tx->htrd->errnum = QSE_HTRD_ENOMEM;
			return QSE_NULL;
		}

		/* let 'ptr' point to the actual space for the combined value */
		ptr = (qse_mchar_t*)(cmb + 1);
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
		if (!(ptr >= tx->htrd->fed.b.raw.data && ptr < 
		      &tx->htrd->fed.b.raw.data[tx->htrd->fed.b.raw.size]))
		{
			/* NOTE the range check in 'if' assumes that raw.data is never
			 * relocated for resizing */

			QSE_MMGR_FREE (
				tx->htrd->mmgr, 
				((struct hdr_cmb_t*)QSE_HTB_VPTR(pair)) - 1
			);
		}
#endif
		
		/* update the value pointer and length */
		QSE_HTB_VPTR(pair) = ptr;
		QSE_HTB_VLEN(pair) = len;

		/* link the new combined value block */
		cmb->next = tx->htrd->fed.chl;
		tx->htrd->fed.chl = cmb;

		if (capture_key_header (tx->htrd, pair) <= -1) return QSE_NULL;

		return pair;
	}
}

qse_mchar_t* parse_header_fields (qse_htrd_t* htrd, qse_mchar_t* line)
{
	qse_mchar_t* p = line, * last;
	struct
	{
		qse_mchar_t* ptr;
		qse_size_t      len;
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
		qse_mchar_t* cpydst;

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

		ctx.htrd = htrd;
		ctx.vptr = value.ptr;
		ctx.vlen = value.len;

		htrd->errnum = QSE_HTRD_ENOERR;
		if (qse_htb_cbsert (
			&htrd->re.hdrtab, name.ptr, name.len, 
			hdr_cbserter, &ctx) == QSE_NULL)
		{
			if (htrd->errnum == QSE_HTRD_ENOERR) 
				htrd->errnum = QSE_HTRD_ENOMEM;
			return QSE_NULL;
		}
	}

	return p;

badhdr:
	htrd->errnum = QSE_HTRD_EBADHDR;
	return QSE_NULL;
}

static QSE_INLINE int parse_initial_line_and_headers (
	qse_htrd_t* htrd, const qse_mchar_t* req, qse_size_t rlen)
{
	qse_mchar_t* p;

	/* add the actual request */
	if (push_to_buffer (htrd, &htrd->fed.b.raw, req, rlen) <= -1) return -1;

	/* add the terminating null for easier parsing */
	if (push_to_buffer (htrd, &htrd->fed.b.raw, &NUL, 1) <= -1) return -1;

	p = QSE_MBS_PTR(&htrd->fed.b.raw);

	if (htrd->option & QSE_HTRD_SKIPEMPTYLINES)
		while (is_whspace_octet(*p)) p++;
	else
		while (is_space_octet(*p)) p++;
	
	QSE_ASSERT (*p != '\0');

	/* parse the initial line */
	if (!(htrd->option & QSE_HTRD_SKIPINITIALLINE))
	{
		p = parse_initial_line (htrd, p);
		if (p == QSE_NULL) return -1;
	}

	/* parse header fields */
	do
	{
		while (is_whspace_octet(*p)) p++;
		if (*p == '\0') break;

		/* TODO: return error if protocol is 0.9.
		 * HTTP/0.9 must not get headers... */

		p = parse_header_fields (htrd, p);
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

static const qse_mchar_t* getchunklen (qse_htrd_t* htrd, const qse_mchar_t* ptr, qse_size_t len)
{
	const qse_mchar_t* end = ptr + len;

	/* this function must be called in the GET_CHUNK_LEN context */
	QSE_ASSERT (htrd->fed.s.chunk.phase == GET_CHUNK_LEN);

/*qse_printf (QSE_T("CALLING getchunklen [%d]\n"), *ptr);*/
	if (htrd->fed.s.chunk.count <= 0)
	{
		/* skip leading spaces if the first character of
		 * the chunk length has not been read yet */
		while (ptr < end && is_space_octet(*ptr)) ptr++;
	}

	while (ptr < end)
	{
		int n = xdigit_to_num (*ptr);
		if (n <= -1) break;

		htrd->fed.s.chunk.len = htrd->fed.s.chunk.len * 16 + n;
		htrd->fed.s.chunk.count++;
		ptr++;
	}

	/* skip trailing spaces if the length has been read */
	while (ptr < end && is_space_octet(*ptr)) ptr++;

	if (ptr < end)
	{
		if (*ptr == '\n') 
		{
			/* the chunk length line ended properly */

			if (htrd->fed.s.chunk.count <= 0)
			{
				/* empty line - no more chunk */
/*qse_printf (QSE_T("empty line chunk done....\n"));*/
				htrd->fed.s.chunk.phase = GET_CHUNK_DONE;
			}
			else if (htrd->fed.s.chunk.len <= 0)
			{
				/* length explicity specified to 0
				   get trailing headers .... */
				htrd->fed.s.chunk.phase = GET_CHUNK_TRAILERS;
/*qse_printf (QSE_T("SWITCH TO GET_CHUNK_TRAILERS....\n"));*/
			}
			else
			{
				/* ready to read the chunk data... */
				htrd->fed.s.chunk.phase = GET_CHUNK_DATA;
/*qse_printf (QSE_T("SWITCH TO GET_CHUNK_DATA....\n"));*/
			}

			htrd->fed.s.need = htrd->fed.s.chunk.len;
			ptr++;
		}
		else
		{
/*qse_printf (QSE_T("XXXXXXXXXXXXXXXXXxxx [%c]\n"), *ptr);*/
			htrd->errnum = QSE_HTRD_EBADRE;
			return QSE_NULL;
		}
	}

	return ptr;
}

static const qse_mchar_t* get_trailing_headers (
	qse_htrd_t* htrd, const qse_mchar_t* req, const qse_mchar_t* end)
{
	const qse_mchar_t* ptr = req;

	while (ptr < end)
	{
		register qse_mchar_t b = *ptr++;

		switch (b)
		{
			case '\0':
				/* guarantee that the request does not contain a null 
				 * character */
				htrd->errnum = QSE_HTRD_EBADRE;
				return QSE_NULL;

			case '\n':
				if (htrd->fed.s.crlf <= 1) 
				{
					htrd->fed.s.crlf = 2;
					break;
				}
				else
				{
					qse_mchar_t* p;
	
					QSE_ASSERT (htrd->fed.s.crlf <= 3);
					htrd->fed.s.crlf = 0;
	
					if (push_to_buffer (
						htrd, &htrd->fed.b.tra, req, ptr - req) <= -1)
						return QSE_NULL;
					if (push_to_buffer (
						htrd, &htrd->fed.b.tra, &NUL, 1) <= -1) 
						return QSE_NULL;
	
					p = QSE_MBS_PTR(&htrd->fed.b.tra);
	
					do
					{
						while (is_whspace_octet(*p)) p++;
						if (*p == '\0') break;
	
						/* TODO: return error if protocol is 0.9.
						 * HTTP/0.9 must not get headers... */
	
						p = parse_header_fields (htrd, p);
						if (p == QSE_NULL) return QSE_NULL;
					}
					while (1);

					htrd->fed.s.chunk.phase = GET_CHUNK_DONE;
					goto done;
				}

			case '\r':
				if (htrd->fed.s.crlf == 0 || htrd->fed.s.crlf == 2) 
					htrd->fed.s.crlf++;
				else htrd->fed.s.crlf = 1;
				break;

			default:
				/* mark that neither CR nor LF was seen */
				htrd->fed.s.crlf = 0;
		}
	}

	if (push_to_buffer (htrd, &htrd->fed.b.tra, req, ptr - req) <= -1) 
		return QSE_NULL;

done:
	return ptr;
}

/* feed the percent encoded string */
int qse_htrd_feed (qse_htrd_t* htrd, const qse_mchar_t* req, qse_size_t len)
{
	const qse_mchar_t* end = req + len;
	const qse_mchar_t* ptr = req;

	/* does this goto drop code maintainability? */
	if (htrd->fed.s.need > 0) 
	{
		/* we're in need of as many octets as htrd->fed.s.need 
		 * for contents body. make a proper jump to resume
		 * content handling */
		goto content_resume;
	}

	switch (htrd->fed.s.chunk.phase)
	{
		case GET_CHUNK_LEN:
			goto dechunk_resume;

		case GET_CHUNK_DATA:
			/* this won't be reached as htrd->fed.s.need 
			 * is greater than 0 if GET_CHUNK_DATA is true */
			goto content_resume;

		case GET_CHUNK_CRLF:
			goto dechunk_crlf;

		case GET_CHUNK_TRAILERS:
			goto dechunk_get_trailers;
	}

	while (ptr < end)
	{
		register qse_mchar_t b = *ptr++;

		if (htrd->option & QSE_HTRD_SKIPEMPTYLINES &&
		    htrd->fed.s.plen <= 0 && is_whspace_octet(b)) 
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
				htrd->errnum = QSE_HTRD_EBADRE;
				return -1;

			case '\n':
			{
				if (htrd->fed.s.crlf <= 1) 
				{
					/* htrd->fed.s.crlf == 0
					 *   => CR was not seen
					 * htrd->fed.s.crlf == 1
					 *   => CR was seen 
					 * whatever the current case is, 
					 * mark the first LF is seen here.
					 */
					htrd->fed.s.crlf = 2;
				}
				else
				{
					/* htrd->fed.s.crlf == 2
					 *   => no 2nd CR before LF
					 * htrd->fed.s.crlf == 3
					 *   => 2nd CR before LF
					 */

					/* we got a complete request. */
					QSE_ASSERT (htrd->fed.s.crlf <= 3);
	
					/* reset the crlf state */
					htrd->fed.s.crlf = 0;
					/* reset the raw request length */
					htrd->fed.s.plen = 0;
	
					if (parse_initial_line_and_headers (htrd, req, ptr - req) <= -1)
						return -1;

					if (htrd->option & QSE_HTRD_HURRIED)
					{
						int n;

						/* it pushes any trailing data into the content in this mode.
						 * so the handler knows if there is contents fed to this reader. */
						if (push_to_buffer (htrd, &htrd->re.content, ptr, end - ptr) <= -1) 
							return -1;
					

						htrd->re.attr.hurried = 1;
						htrd->errnum = QSE_HTRD_ENOERR;	
						if (htrd->retype == QSE_HTRD_RETYPE_S)
						{
							QSE_ASSERTX (
								htrd->recbs->response != QSE_NULL,
								"set response callback before feeding"
							);
							n = htrd->recbs->response (htrd, &htrd->re);
						}
						else
						{
							QSE_ASSERTX (
								htrd->recbs->request != QSE_NULL,
								"set request callback before feeding"
							);
							n = htrd->recbs->request (htrd, &htrd->re);
						}

						/* qse_mbs_clear (&htrd->re.content); */

						if (n <= -1)
						{
							if (htrd->errnum == QSE_HTRD_ENOERR)
								htrd->errnum = QSE_HTRD_ERECBS;	
	
							/* need to clear request on error? 
							clear_feed (htrd); */
							return -1;
						}

						/* if QSE_HTRD_HURRIED is set, we do not handle expect_continue */
						/* if QSE_HTRD_HURRIED is set, we handle a single request only */

						return 0;
					}

					if (htrd->retype == QSE_HTRD_RETYPE_Q && 
					    htrd->re.attr.expect_continue && 
					    htrd->recbs->expect_continue && ptr >= end)
					{
						int n;

						/* the 'ptr >= end' check is for not executing the 
						 * callback if the message body has been seen already.
						 * however, this is not perfect as it is based on
						 * the current feed. what if it has been received but
						 * not fed here?
						 */ 

						htrd->re.attr.hurried = 0;
						htrd->errnum = QSE_HTRD_ENOERR;	
						n = htrd->recbs->expect_continue (htrd, &htrd->re);

						if (n <= -1)
						{
							if (htrd->errnum == QSE_HTRD_ENOERR)
								htrd->errnum = QSE_HTRD_ERECBS;	

							/* need to clear request on error? 
							clear_feed (htrd); */
							return -1;
						}

						/* the expect_continue handler can set discard to non-zero 
						 * to force discard the contents body 
						htrd->re.discard = 1;
						 */
					}
	
					if (htrd->re.attr.chunked)
					{
						/* transfer-encoding: chunked */
						QSE_ASSERT (!htrd->re.attr.content_length_set);
	
					dechunk_start:
						htrd->fed.s.chunk.phase = GET_CHUNK_LEN;
						htrd->fed.s.chunk.len = 0;
						htrd->fed.s.chunk.count = 0;
	
					dechunk_resume:
						ptr = getchunklen (htrd, ptr, end - ptr);
						if (ptr == QSE_NULL) return -1;
	
						if (htrd->fed.s.chunk.phase == GET_CHUNK_LEN)
						{
							/* still in the GET_CHUNK_LEN state.
							 * the length has been partially read. */
							goto feedme_more;
						}
						else if (htrd->fed.s.chunk.phase == GET_CHUNK_TRAILERS)
						{
						dechunk_get_trailers:
							ptr = get_trailing_headers (htrd, ptr, end);
							if (ptr == QSE_NULL) return -1;
	
							if (htrd->fed.s.chunk.phase == GET_CHUNK_TRAILERS)
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
						htrd->fed.s.need = htrd->re.attr.content_length;
					}

					if (htrd->fed.s.need > 0)
					{
						/* content-length or chunked data length 
						 * specified */

						qse_size_t avail;
	
					content_resume:
						avail = end - ptr;
	
						if (avail < htrd->fed.s.need)
						{
							/* the data is not as large as needed */
							if (push_to_buffer (htrd, &htrd->re.content, ptr, avail) <= -1) return -1;
							htrd->fed.s.need -= avail;
							/* we didn't get a complete content yet */
							goto feedme_more; 
						}
						else 
						{
							/* we got all or more than needed */
							if (push_to_buffer (
								htrd, &htrd->re.content, ptr, 
								htrd->fed.s.need) <= -1) return -1;
							ptr += htrd->fed.s.need;
							htrd->fed.s.need = 0;
						}
					}
	
					if (htrd->fed.s.chunk.phase == GET_CHUNK_DATA)
					{
						QSE_ASSERT (htrd->fed.s.need == 0);
						htrd->fed.s.chunk.phase = GET_CHUNK_CRLF;
	
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
								htrd->fed.s.chunk.phase = GET_CHUNK_LEN;
								htrd->fed.s.chunk.len = 0;
								htrd->fed.s.chunk.count = 0;

								goto feedme_more;
							}
							else
							{
								/* redundant character ... */
								htrd->errnum = QSE_HTRD_EBADRE;
								return -1;
							}
						}
						else
						{
							/* data not enough */
							goto feedme_more;
						}
					}

					if (!htrd->re.discard)
					{
						int n;

						htrd->re.attr.hurried = 0;
						htrd->errnum = QSE_HTRD_ENOERR;

						if (htrd->retype == QSE_HTRD_RETYPE_S)
						{
							QSE_ASSERTX (
								htrd->recbs->response != QSE_NULL,
								"set response callback before feeding"
							);
	
							n = htrd->recbs->response (htrd, &htrd->re);
						}
						else
						{
							QSE_ASSERTX (
								htrd->recbs->request != QSE_NULL,
								"set request callback before feeding"
							);
	
							n = htrd->recbs->request (htrd, &htrd->re);
						}
		
						if (n <= -1)
						{
							if (htrd->errnum == QSE_HTRD_ENOERR)
								htrd->errnum = QSE_HTRD_ERECBS;	
	
							/* need to clear request on error? 
							clear_feed (htrd); */
							return -1;
						}
					}

					clear_feed (htrd);

					/* let ptr point to the next character to LF or 
					 * the optional contents */
					req = ptr; 
				}
				break;
			}

			case '\r':
				if (htrd->fed.s.crlf == 0 || htrd->fed.s.crlf == 2) 
					htrd->fed.s.crlf++;
				else htrd->fed.s.crlf = 1;
				break;

			default:
				/* increment length of a request in raw 
				 * excluding crlf */
				htrd->fed.s.plen++; 
				/* mark that neither CR nor LF was seen */
				htrd->fed.s.crlf = 0;
		}
	}

	if (ptr > req)
	{
		/* enbuffer the incomplete request */
		if (push_to_buffer (htrd, &htrd->fed.b.raw, req, ptr - req) <= -1) return -1;
	}

feedme_more:
	return 0;
}

#if 0
int qse_htrd_scanqparam (qse_htrd_t* htrd, const qse_mcstr_t* cstr)
{
	qse_mcstr_t key, val;
	const qse_mchar_t* p, * end;
	qse_mchar_t* out;

	if (cstr == QSE_NULL) cstr = qse_htre_getqparamcstr(&htrd->re);

	p = cstr->ptr;
	if (p == QSE_NULL) return 0; /* no param string to scan */

	end = p + cstr->len;

	/* a key and a value pair including two terminating null 
	 * can't exceed the the qparamstrlen + 2. only +1 below as there is
	 * one more space for an internal terminating null */
	qse_mbs_setlen (&htrd->tmp.qparam, cstr->len + 1);

	/* let out point to the beginning of the qparam buffer.
	 * the loop below emits percent-decode key and value to this buffer. */
	out = QSE_MBS_PTR(&htrd->tmp.qparam);

	key.ptr = out; key.len = 0;
	val.ptr = QSE_NULL; val.len = 0;

	do
	{
		if (p >= end || *p == '&' || *p == ';')
		{
			QSE_ASSERT (key.ptr != QSE_NULL);

			*out++ = '\0'; 
			if (val.ptr == QSE_NULL) 
			{
				if (key.len == 0) 
				{
					/* both key and value are empty.
					 * we don't need to do anything */
					goto next_octet;
				}

				val.ptr = out;
				*out++ = '\0'; 
				QSE_ASSERT (val.len == 0);
			}

			QSE_ASSERTX (
				htrd->recbs->qparamstr != QSE_NULL,
				"set request parameter string callback before scanning"
			);

			htrd->errnum = QSE_HTRD_ENOERR;
			if (htrd->recbs->qparamstr (htrd, &key, &val) <= -1) 
			{
				if (htrd->errnum == QSE_HTRD_ENOERR)
					htrd->errnum = QSE_HTRD_ERECBS;	
				return -1;
			}

		next_octet:
			if (p >= end) break;
			p++;

			out = QSE_MBS_PTR(&htrd->tmp.qparam);
			key.ptr = out; key.len = 0;
			val.ptr = QSE_NULL; val.len = 0;
		}
		else if (*p == '=')
		{
			*out++ = '\0'; p++;

			val.ptr = out;
			/*val.len = 0; */
		}
		else
		{
			if (*p == '%' && p + 2 <= end)
			{
				int q = xdigit_to_num(*(p+1));
				if (q >= 0)
				{
					int w = xdigit_to_num(*(p+2));
					if (w >= 0)
					{
						/* unlike the path part, we don't care if it 
						 * contains a null character */
						*out++ = ((q << 4) + w);
						p += 3;
						goto next;
					}
				}
			}

			*out++ = *p++;

		next:
			if (val.ptr) val.len++;
			else key.len++;
		}
	}
	while (1);

	qse_mbs_clear (&htrd->tmp.qparam);
	return 0;
}
#endif
