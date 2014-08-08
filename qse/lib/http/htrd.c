/*
 * $Id$
 * 
    Copyright 2006-2014 Chung, Hyung-Hwan.
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

#include <qse/http/htrd.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/path.h>
#include "../cmn/mem.h"

static const qse_mchar_t NUL = QSE_MT('\0');

#define CONSUME_UNTIL_CLOSE (1 << 0)

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

static QSE_INLINE int push_content (
	qse_htrd_t* htrd, const qse_mchar_t* ptr, qse_size_t len)
{
	QSE_ASSERT (len > 0);

	if (qse_htre_addcontent (&htrd->re, ptr, len) <= -1) 
	{
		htrd->errnum = QSE_HTRD_ENOMEM;
		return -1;
	}

	/* qse_htre_addcontent() returns 1 on full success and 0 if adding is 
	 * skipped. i treat both as success */
	return 0;
}

static QSE_INLINE void clear_feed (qse_htrd_t* htrd)
{
	/* clear necessary part of the request/response before 
	 * reading the next request/response */
	htrd->clean = 1;
	qse_htre_clear (&htrd->re);

	qse_mbs_clear (&htrd->fed.b.tra);
	qse_mbs_clear (&htrd->fed.b.raw);

	QSE_MEMSET (&htrd->fed.s, 0, QSE_SIZEOF(htrd->fed.s));
}

qse_htrd_t* qse_htrd_open (qse_mmgr_t* mmgr, qse_size_t xtnsize)
{
	qse_htrd_t* htrd;

	htrd = (qse_htrd_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_htrd_t) + xtnsize);
	if (htrd)
	{
		if (qse_htrd_init (htrd, mmgr) <= -1)
		{
			QSE_MMGR_FREE (mmgr, htrd);
			return QSE_NULL;
		}
		else QSE_MEMSET (QSE_XTN(htrd), 0, xtnsize);
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

	htrd->clean = 1;
	return 0;
}

void qse_htrd_fini (qse_htrd_t* htrd)
{
	qse_htre_fini (&htrd->re);

	qse_mbs_fini (&htrd->fed.b.tra);
	qse_mbs_fini (&htrd->fed.b.raw);
#if 0
	qse_mbs_fini (&htrd->tmp.qparam);
#endif
}

static qse_mchar_t* parse_initial_line (qse_htrd_t* htrd, qse_mchar_t* line)
{
	qse_mchar_t* p = line;
	qse_mcstr_t tmp;

#if 0
	/* ignore leading spaces excluding crlf */
	while (is_space_octet(*p)) p++;
#endif

	/* the method should start with an alphabet */
	if (!is_alpha_octet(*p)) goto badre;

	/* get the method name */
	tmp.ptr = p;
	do { p++; } while (is_alpha_octet(*p));
	tmp.len = p - tmp.ptr;

	htrd->re.type = QSE_HTRE_Q;
	if (htrd->option & QSE_HTRD_REQUEST)
	{
		/* method name must be followed by space */
		if (!is_space_octet(*p)) goto badre;

		*p = QSE_MT('\0'); /* null-terminate the method name */

		htrd->re.u.q.method.type = qse_mcstrtohttpmethod (&tmp);
		htrd->re.u.q.method.name = tmp.ptr;
	}
	else if ((htrd->option & QSE_HTRD_RESPONSE) &&
	         qse_mbsxcmp (tmp.ptr, tmp.len, QSE_MT("HTTP")) == 0)
	{
		/* it begins with HTTP. it may be a response */
		htrd->re.type = QSE_HTRE_S;
	}
	else goto badre;

	if (htrd->re.type == QSE_HTRE_S)
	{
		/* response */
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

		/* version must be followed by space */
		if (!is_space_octet(*p)) goto badre;

		*p = QSE_MT('\0'); /* null-terminate version string */
		htrd->re.verstr = tmp.ptr;

		/* skip spaces */
		do p++; while (is_space_octet(*p));
		
		n = digit_to_num(*p);
		if (n <= -1) goto badre;

		tmp.ptr = p;
		status = 0;
		do
		{
			status = status * 10 + n;
			p++;
		} 
		while ((n = digit_to_num(*p)) >= 0);

		if (!is_space_octet(*p)) goto badre;
		*p = QSE_MT('\0'); /* null-terminate the status code */

		htrd->re.u.s.code.val = status;
		htrd->re.u.s.code.str = tmp.ptr;

		/* i don't treat the following weird messages as bad message:
		 *    no status message follows the status code
		 */

		/* skip spaces */
		do p++; while (is_space_octet(*p));
	
		tmp.ptr = p;
		tmp.len = 0;
		while (*p != QSE_MT('\0') && *p != QSE_MT('\n')) 
		{
			if (!is_space_octet(*p)) tmp.len = p - tmp.ptr + 1;
			p++;
		}
	
		/* if the line does not end with a new line, it is a bad request */
		if (*p != QSE_T('\n')) goto badre;

		/* null-terminate the message */
		((qse_mchar_t*)tmp.ptr)[tmp.len] = QSE_MT('\0');
		htrd->re.u.s.mesg = tmp.ptr;
	}
	else
	{
#if 0
		qse_mchar_t* out;
#endif
		qse_mcstr_t param;

		/* skip spaces */
		do p++; while (is_space_octet(*p));

		/* process the url part */
		tmp.ptr = p; /* remember the beginning of path*/
		param.ptr = QSE_NULL;
#if 0
		out = p;
		while (*p != QSE_MT('\0') && !is_space_octet(*p)) 
		{
			if (*p == QSE_MT('%') && param.ptr == QSE_NULL)
			{
				/* decode percent-encoded charaters in the 
				 * path part. if we're in the parameter string
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
				if (param.ptr == QSE_NULL)
				{
					/* ? must be explicit to be an argument instroducer. 
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
			htrd->re.u.q.path = tmp.ptr;
			htrd->re.u.q.param = param.ptr;
		}
		else 
		{
			tmp.len = out - tmp.ptr;
			htrd->re.u.q.path = tmp.ptr;
			htrd->re.u.q.param = QSE_NULL;
		}
#else
		while (*p != QSE_MT('\0') && !is_space_octet(*p)) 
		{
			if (*p == QSE_MT('?') && param.ptr == QSE_NULL)
			{
				*p++ = QSE_MT('\0'); /* null-terminate the path part */
				param.ptr = p;
			}
			else p++;
		}

		/* the url must be followed by a space */
		if (!is_space_octet(*p)) goto badre;
		*p = QSE_MT('\0');  /* null-terminate the path or param part */

		if (param.ptr)
		{
			htrd->re.u.q.path = tmp.ptr;
			htrd->re.u.q.param = param.ptr;
		}
		else
		{
			htrd->re.u.q.path = tmp.ptr;
			htrd->re.u.q.param = QSE_NULL;
		}
#endif

		if (htrd->option & QSE_HTRD_CANONQPATH)
		{
			qse_mchar_t* qpath = htrd->re.u.q.path;

			/* if the url begins with xxx://,
			 * skip xxx:/ and canonicalize from the second slash */
			while (is_alpha_octet(*qpath)) qpath++;
			if (qse_mbszcmp (qpath, QSE_MT("://"), 3) == 0)
				qpath = qpath + 2; /* set the position to the second / in :// */
			else
				qpath = htrd->re.u.q.path;

			qse_canonmbspath (qpath, qpath, 0);
		}
	
		/* skip spaces after the url part */
		do { p++; } while (is_space_octet(*p));
	
		tmp.ptr = p;
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
	
		tmp.len = p - tmp.ptr;

		/* skip trailing spaces on the line */
		while (is_space_octet(*p)) p++;

		/* if the line does not end with a new line, it is a bad request */
		if (*p != QSE_T('\n')) goto badre;

		((qse_mchar_t*)tmp.ptr)[tmp.len] = QSE_MT('\0');
		htrd->re.verstr = tmp.ptr;
	}
	
	/* adjust Connection: Keep-Alive for HTTP 1.1 or later.
	 * this is initial. it can be adjusted further in capture_connection(). */
	if (htrd->re.version.major > 1 || 
	    (htrd->re.version.major == 1 && htrd->re.version.minor >= 1))
	{
		htrd->re.attr.flags |= QSE_HTRE_ATTR_KEEPALIVE;
	}

	return ++p;

badre:
	htrd->errnum = QSE_HTRD_EBADRE;
	return QSE_NULL;
}

void qse_htrd_clear (qse_htrd_t* htrd)
{
	clear_feed (htrd);
}

qse_mmgr_t* qse_htrd_getmmgr (qse_htrd_t* htrd)
{
	return htrd->mmgr;
}

void* qse_htrd_getxtn (qse_htrd_t* htrd)
{
	return QSE_XTN (htrd);
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

static int capture_connection (qse_htrd_t* htrd, qse_htb_pair_t* pair)
{
	int n;
	qse_htre_hdrval_t* val;

	val = QSE_HTB_VPTR(pair);
	while (val->next) val = val->next;

	n = qse_mbscmp (val->ptr, QSE_MT("close"));
	if (n == 0)
	{
		htrd->re.attr.flags &= ~QSE_HTRE_ATTR_KEEPALIVE;
		return 0;
	}

	n = qse_mbscmp (val->ptr, QSE_MT("keep-alive"));
	if (n == 0)
	{
		htrd->re.attr.flags |= QSE_HTRE_ATTR_KEEPALIVE;
		return 0;
	}

	/* Basically i don't care about other values.
	 * but for HTTP 1.0, other values will set connection to 'close'.
	 *
	 * Other values include even Keep-Alive specified multiple times.
	 *  Connection: Keep-Alive
	 *  Connection: Keep-Alive
	 * For the second Keep-Alive, this function sees 'Keep-Alive,Keep-Alive'
	 * That's because values of the same keys are concatenated.
	 */
	if (htrd->re.version.major < 1  || 
	    (htrd->re.version.major == 1 && htrd->re.version.minor <= 0))
	{
		htrd->re.attr.flags &= ~QSE_HTRE_ATTR_KEEPALIVE;
	}
	return 0;
}

static int capture_content_length (qse_htrd_t* htrd, qse_htb_pair_t* pair)
{
	qse_size_t len = 0, off = 0, tmp;
	const qse_mchar_t* ptr;
	qse_htre_hdrval_t* val;

	/* get the last content_length */
	val = QSE_HTB_VPTR(pair);
	while (val->next) val = val->next;

	ptr = val->ptr;
	while (off < val->len)
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

	if ((htrd->re.attr.flags & QSE_HTRE_ATTR_CHUNKED) && len > 0)
	{
		/* content-length is greater than 0 
		 * while transfer-encoding: chunked is specified. */
		htrd->errnum = QSE_HTRD_EBADRE;
		return -1;
	}

	htrd->re.attr.flags |= QSE_HTRE_ATTR_LENGTH;
	htrd->re.attr.content_length = len;
	return 0;
}

static int capture_expect (qse_htrd_t* htrd, qse_htb_pair_t* pair)
{
	qse_htre_hdrval_t* val;

	/* Expect is included */
	htrd->re.attr.flags |= QSE_HTRE_ATTR_EXPECT; 

	val = QSE_HTB_VPTR(pair);
	while (val) 
	{	
		/* Expect: 100-continue is included */
		if (qse_mbscasecmp (val->ptr, QSE_MT("100-continue")) == 0)
			htrd->re.attr.flags |= QSE_HTRE_ATTR_EXPECT100; 
		val = val->next;
	}

	return 0;
}

static int capture_status (qse_htrd_t* htrd, qse_htb_pair_t* pair)
{
	qse_htre_hdrval_t* val;

	val = QSE_HTB_VPTR(pair);
	while (val->next) val = val->next;

	htrd->re.attr.status = val->ptr;
	return 0;
}

static int capture_transfer_encoding (qse_htrd_t* htrd, qse_htb_pair_t* pair)
{
	int n;
	qse_htre_hdrval_t* val;

	val = QSE_HTB_VPTR(pair);
	while (val->next) val = val->next;

	n = qse_mbscasecmp (val->ptr, QSE_MT("chunked"));
	if (n == 0)
	{
		/* if (htrd->re.attr.content_length > 0) */
		if (htrd->re.attr.flags & QSE_HTRE_ATTR_LENGTH)
		{
			/* both content-length and 'transfer-encoding: chunked' are specified. */
			goto badre;
		}

		htrd->re.attr.flags |= QSE_HTRE_ATTR_CHUNKED;
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
		{ "Status",             6,  capture_status },
		{ "Transfer-Encoding",  17, capture_transfer_encoding  }
	};

	int n;
	qse_size_t mid, count, base = 0;

	/* perform binary search */
	for (count = QSE_COUNTOF(hdrtab); count > 0; count /= 2)
	{
		mid = base + count / 2;

		n = qse_mbsxncasecmp (
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
		qse_htre_hdrval_t *val;

		val = QSE_MMGR_ALLOC (htb->mmgr, QSE_SIZEOF(*val));
		if (val == QSE_NULL)
		{
			tx->htrd->errnum = QSE_HTRD_ENOMEM;
			return QSE_NULL;
		}

		QSE_MEMSET (val, 0, QSE_SIZEOF(*val));
		val->ptr = tx->vptr;
		val->len = tx->vlen;
		val->next = QSE_NULL;

		p = qse_htb_allocpair (htb, kptr, klen, val, 0);
		if (p == QSE_NULL) 
		{
			QSE_MMGR_FREE (htb->mmgr, val);
			tx->htrd->errnum = QSE_HTRD_ENOMEM;
		}
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
		/* RFC2616 
		 * Multiple message-header fields with the same field-name 
		 * MAY be present in a message if and only if the entire 
		 * field-value for that header field is defined as a 
		 * comma-separated list [i.e., #(values)]. It MUST be possible 
		 * to combine the multiple header fields into one 
		 * "field-name: field-value" pair, without changing the semantics
		 * of the message, by appending each subsequent field-value 
		 * to the first, each separated by a comma. The order in which 
		 * header fields with the same field-name are received is therefore
		 * significant to the interpretation of the combined field value,
		 * and thus a proxy MUST NOT change the order of these field values 
		 * when a message is forwarded. 

		 * RFC6265 defines the syntax for Set-Cookie and Cookie.
		 * this seems to be conflicting with RFC2616.
		 * 
		 * Origin servers SHOULD NOT fold multiple Set-Cookie header fields 
		 * into a single header field. The usual mechanism for folding HTTP 
		 * headers fields (i.e., as defined in [RFC2616]) might change the 
		 * semantics of the Set-Cookie header field because the %x2C (",")
		 * character is used by Set-Cookie in a way that conflicts with 
		 * such folding.
		 * 	
		 * So i just maintain the list of valuea for a key instead of
		 * folding them.
		 */

		qse_htre_hdrval_t* val;
		qse_htre_hdrval_t* tmp;

		val = (qse_htre_hdrval_t*) QSE_MMGR_ALLOC (
			tx->htrd->mmgr, QSE_SIZEOF(*val));
		if (val == QSE_NULL)
		{
			tx->htrd->errnum = QSE_HTRD_ENOMEM;
			return QSE_NULL;
		}

		QSE_MEMSET (val, 0, QSE_SIZEOF(*val));
		val->ptr = tx->vptr;
		val->len = tx->vlen;
		val->next = QSE_NULL;

/* TODO: doubly linked list for speed-up??? */
		tmp = QSE_HTB_VPTR(pair);
		QSE_ASSERT (tmp != QSE_NULL);

		/* find the tail */
		while (tmp->next) tmp = tmp->next;
		/* append it to the list*/
		tmp->next = val; 

		if (capture_key_header (tx->htrd, pair) <= -1) return QSE_NULL;
		return pair;
	}
}

qse_mchar_t* parse_header_field (
	qse_htrd_t* htrd, qse_mchar_t* line, qse_htb_t* tab)
{
	qse_mchar_t* p = line, * last;
	struct
	{
		qse_mchar_t* ptr;
		qse_size_t   len;
	} name, value;

#if 0
	/* ignore leading spaces excluding crlf */
	while (is_space_octet(*p)) p++;
#endif

	QSE_ASSERT (!is_whspace_octet(*p));

	/* check the field name */
	name.ptr = last = p;
	while (*p != QSE_MT('\0') && *p != QSE_MT('\n') && *p != QSE_MT(':'))
	{
		if (!is_space_octet(*p++)) last = p;
	}
	name.len = last - name.ptr;

	if (*p != QSE_MT(':')) 
	{
		if (!(htrd->option & QSE_HTRD_STRICT))
		{
			while (is_space_octet(*p)) p++;
			if (*p == QSE_MT('\n')) 
			{
				/* ignore a line without a colon */
				p++;
				return p;		
			}
		}
		goto badhdr;
	}
	*last = '\0';

	/* skip the colon and spaces after it */
	do { p++; } while (is_space_octet(*p));

	value.ptr = last = p;
	while (*p != QSE_MT('\0') && *p != QSE_MT('\n'))
	{
		if (!is_space_octet(*p++)) last = p;
	}

	value.len = last - value.ptr;
	if (*p != QSE_MT('\n')) goto badhdr; /* not ending with a new line */

	/* peep at the beginning of the next line to check if it is 
	 * the continuation */
	if (is_purespace_octet (*++p))
	{
		/* RFC: HTTP/1.0 headers may be folded onto multiple lines if 
		 * each continuation line begins with a space or horizontal tab. 
		 * All linear whitespace, including folding, has the same semantics 
		 * as SP. */
		qse_mchar_t* cpydst;

		cpydst = p - 1;
		if (*(cpydst-1) == QSE_MT('\r')) cpydst--;

		/* process all continued lines */
		do 
		{
			while (*p != QSE_MT('\0') && *p != QSE_MT('\n'))
			{
				*cpydst = *p++; 
				if (!is_space_octet(*cpydst++)) last = cpydst;
			} 
	
			value.len = last - value.ptr;
			if (*p != QSE_MT('\n')) goto badhdr;

			if (*(cpydst-1) == QSE_MT('\r')) cpydst--;
		}
		while (is_purespace_octet(*++p));
	}
	*last = QSE_MT('\0');

	/* insert the new field to the header table */
	{
		struct hdr_cbserter_ctx_t ctx;

		ctx.htrd = htrd;
		ctx.vptr = value.ptr;
		ctx.vlen = value.len;

		htrd->errnum = QSE_HTRD_ENOERR;
		if (qse_htb_cbsert (
			tab, name.ptr, name.len, 
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

#if 0
	if (htrd->option & QSE_HTRD_SKIPEMPTYLINES)
		while (is_whspace_octet(*p)) p++;
	else
#endif
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

		p = parse_header_field (htrd, p, &htrd->re.hdrtab);
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
				htrd->fed.s.chunk.phase = GET_CHUNK_DONE;
			}
			else if (htrd->fed.s.chunk.len <= 0)
			{
				/* length explicity specified to 0
				   get trailing headers .... */
				htrd->fed.s.chunk.phase = GET_CHUNK_TRAILERS;
			}
			else
			{
				/* ready to read the chunk data... */
				htrd->fed.s.chunk.phase = GET_CHUNK_DATA;
			}

			htrd->fed.s.need = htrd->fed.s.chunk.len;
			ptr++;
		}
		else
		{
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
	
						p = parse_header_field (
							htrd, p, 
							((htrd->option & QSE_HTRD_TRAILERS)? &htrd->re.trailers: &htrd->re.hdrtab)
						);
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
				break;
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
	int header_completed_during_this_feed = 0;
	qse_size_t avail;

	QSE_ASSERT (len > 0);

	if (htrd->option & QSE_HTRD_DUMMY)
	{
		/* treat everything as contents.
		 * i don't care about headers or whatsoever. */
		return push_content (htrd, req, len);
	}

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

	htrd->clean = 0; /* mark that htrd is in need of some data */

	while (ptr < end)
	{
		register qse_mchar_t b = *ptr++;

#if 0
		if (htrd->option & QSE_HTRD_SKIPEMPTYLINES &&
		    htrd->fed.s.plen <= 0 && is_whspace_octet(b)) 
		{
			/* let's drop leading whitespaces across multiple
			 * lines */
			req++;
			continue;
		}
#endif

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

					/* we got a complete request header. */
					QSE_ASSERT (htrd->fed.s.crlf <= 3);
	
					/* reset the crlf state */
					htrd->fed.s.crlf = 0;
					/* reset the raw request length */
					htrd->fed.s.plen = 0;

					if (parse_initial_line_and_headers (htrd, req, ptr - req) <= -1) return -1;

					/* compelete request header is received */
					header_completed_during_this_feed = 1;
					if (htrd->option & QSE_HTRD_PEEKONLY)
					{
						/* when QSE_HTRD_PEEKONLY is set,
						 * the peek callback is invoked once
						 * a complete header is seen. the caller
						 * should not feed more data by calling
						 * this function again once the callback is
						 * invoked. the trailing data is appended
						 * to the content buffer.
						 *
						 * NOTE: if the current feed that completed 
						 *  the header contains the next request, 
						 *  the next request is treated as if it 
						 *  belongs to the current request.
						 *
						 * In priciple, this option was added for
						 * reading CGI outputs. So it comes with
						 * awkwardity described above.
						 */
						if (ptr < end && push_content (htrd, ptr, end - ptr) <= -1) return -1;

						/* i don't really know if it is really completed 
						 * with content. QSE_HTRD_PEEKONLY is not compatible
						 * with the completed state. anyway, let me complete
						 * it. */
						qse_htre_completecontent (&htrd->re);

						/* this jump is only to invoke the peek 
						 * callback. this function should not be fed
						 * more. */
						goto feedme_more; 
					}

					/* carry on processing content body fed together with the header */
					if (htrd->re.attr.flags & QSE_HTRE_ATTR_CHUNKED)
					{
						/* transfer-encoding: chunked */
						QSE_ASSERT (!(htrd->re.attr.flags & QSE_HTRE_ATTR_LENGTH));

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
							/* this state is reached after the
							 * last chunk length 0 is read. The next
							 * empty line immediately completes 
							 * a content body. so i need to adjust
							 * this crlf status to 2 as if a trailing
							 * header line has been read. */
							htrd->fed.s.crlf = 2;

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
						if ((htrd->option & QSE_HTRD_RESPONSE) && 
						    !(htrd->re.attr.flags & QSE_HTRE_ATTR_LENGTH) &&
						    !(htrd->re.attr.flags & QSE_HTRE_ATTR_KEEPALIVE))
						{
							/* for a response, no content-length and 
							 * no chunk are specified and 'connection' 
							 * is to close. i must read until the 
							 * connection is closed. however, there isn't 
							 * any good way to know when to stop from 
							 * within this function. so the caller
							 * can call qse_htrd_halt() for this. */

							/* set this to the maximum in a type safe way
							 * assuming it's unsigned. the problem of
							 * the current implementation is that 
							 * it can't receive more than  */
							htrd->fed.s.need = 0;
							htrd->fed.s.need = ~htrd->fed.s.need; 
							htrd->fed.s.flags |= CONSUME_UNTIL_CLOSE;
						}
						else
						{
							htrd->fed.s.need = htrd->re.attr.content_length;
						}
				
					}

					if (htrd->fed.s.need > 0)
					{
						/* content-length or chunked data length 
						 * specified */
					content_resume:
						avail = end - ptr;
						if (avail <= 0)
						{
							/* we didn't get a complete content yet */

							/* avail can be 0 if data fed ends with
							 * a chunk length withtout actual data. 
							 * so i check if avail is greater than 0
							 * in order not to push empty content. */
							goto feedme_more; 
						}
						else if (avail < htrd->fed.s.need)
						{
							/* the data is not as large as needed */
							if (push_content (htrd, ptr, avail) <= -1) return -1;

							if (!(htrd->fed.s.flags & CONSUME_UNTIL_CLOSE)) 
							{
								/* i don't decrement htrd->fed.s.need
								 * if i should read until connection is closed.
								 * well, unless set your own callback,
								 * push_content() above will fail 
								 * if too much has been received already */
								htrd->fed.s.need -= avail;
							}

							/* we didn't get a complete content yet */
							goto feedme_more; 
						}
						else 
						{
							/* we got all or more than needed */
							if (push_content (htrd, ptr, htrd->fed.s.need) <= -1) return -1;
							ptr += htrd->fed.s.need;
							if (!(htrd->fed.s.flags & CONSUME_UNTIL_CLOSE)) 
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

					/* the content has been received fully */
					qse_htre_completecontent (&htrd->re);

					if (header_completed_during_this_feed && htrd->recbs->peek)
					{
						/* the peek handler has not been executed.
						 * this can happen if this function is fed with
						 * at least the ending part of a complete header
						 * plus complete content body and the header 
						 * of the next request. */
						int n;
						htrd->errnum = QSE_HTRD_ENOERR;	
						n = htrd->recbs->peek (htrd, &htrd->re);
						if (n <= -1)
						{
							if (htrd->errnum == QSE_HTRD_ENOERR)
								htrd->errnum = QSE_HTRD_ERECBS;	
							/* need to clear request on error? 
							clear_feed (htrd); */
							return -1;
						}

						header_completed_during_this_feed = 0;
					}

					if (htrd->recbs->poke)
					{
						int n;
						htrd->errnum = QSE_HTRD_ENOERR;
						n = htrd->recbs->poke (htrd, &htrd->re);
						if (n <= -1)
						{
							if (htrd->errnum == QSE_HTRD_ENOERR)
								htrd->errnum = QSE_HTRD_ERECBS;	
							/* need to clear request on error? 
							clear_feed (htrd); */
							return -1;
						}
					}

#if 0
qse_printf (QSE_T("CONTENT_LENGTH %d, RAW HEADER LENGTH %d\n"), 
	(int)QSE_MBS_LEN(&htrd->re.content),
	(int)QSE_MBS_LEN(&htrd->fed.b.raw));
#endif

					clear_feed (htrd);
					if (ptr >= end) return 0; /* no more feeds to handle */

					if (htrd->option & QSE_HTRD_DUMMY)
					{
						/* once the mode changes to RAW in a callback,
						 * left-over is pused as contents */
						if (ptr < end)
							return push_content (htrd, ptr, end - ptr);
						else
							return 0;
					}


					/* let ptr point to the next character to LF or 
					 * the optional contents */
					req = ptr; 

					/* since there are more to handle, i mark that
					 * htrd is in need of some data. this may
					 * not be really compatible with SKIPEMPTYLINES. 
					 * SHOULD I simply remove the option? */
					htrd->clean = 0; 
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
	if (header_completed_during_this_feed && htrd->recbs->peek)
	{
		int n;
		htrd->errnum = QSE_HTRD_ENOERR;	
		n = htrd->recbs->peek (htrd, &htrd->re);
		if (n <= -1)
		{
			if (htrd->errnum == QSE_HTRD_ENOERR)
				htrd->errnum = QSE_HTRD_ERECBS;	
			/* need to clear request on error? 
			clear_feed (htrd); */
			return -1;
		}
	}

	return 0;
}

int qse_htrd_halt (qse_htrd_t* htrd)
{
	if (htrd->fed.s.flags & CONSUME_UNTIL_CLOSE || !htrd->clean)
	{
		qse_htre_completecontent (&htrd->re);

		if (htrd->recbs->poke)
		{
			int n;
			htrd->errnum = QSE_HTRD_ENOERR;
			n = htrd->recbs->poke (htrd, &htrd->re);
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
	}

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
