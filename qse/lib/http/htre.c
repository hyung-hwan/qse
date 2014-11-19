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

#include <qse/http/htre.h>
#include "../cmn/mem.h"

static void free_hdrval (qse_htb_t* htb, void* vptr, qse_size_t vlen)
{
	qse_htre_hdrval_t* val;
	qse_htre_hdrval_t* tmp;

	val = vptr;
	while (val)
	{
		tmp = val;
		val = val->next;
		QSE_MMGR_FREE (htb->mmgr, tmp);
	}
}

int qse_htre_init (qse_htre_t* re, qse_mmgr_t* mmgr)
{
	static qse_htb_style_t style =
	{
		{
			QSE_HTB_COPIER_DEFAULT,
			QSE_HTB_COPIER_DEFAULT
		},
		{
			QSE_HTB_FREEER_DEFAULT,
			free_hdrval
		},
		QSE_HTB_COMPER_DEFAULT,
		QSE_HTB_KEEPER_DEFAULT,
		QSE_HTB_SIZER_DEFAULT,
		QSE_HTB_HASHER_DEFAULT
	};

	QSE_MEMSET (re, 0, QSE_SIZEOF(*re));
	re->mmgr = mmgr;

	if (qse_htb_init (&re->hdrtab, mmgr, 60, 70, 1, 1) <= -1) return -1;
	if (qse_htb_init (&re->trailers, mmgr, 20, 70, 1, 1) <= -1) return -1;

	qse_htb_setstyle (&re->hdrtab, &style);
	qse_htb_setstyle (&re->trailers, &style);

	qse_mbs_init (&re->content, mmgr, 0);
#if 0
	qse_mbs_init (&re->iniline, mmgr, 0);
#endif

	return 0;
}

void qse_htre_fini (qse_htre_t* re)
{
#if 0
	qse_mbs_fini (&re->iniline);
#endif
	qse_mbs_fini (&re->content);
	qse_htb_fini (&re->trailers);
	qse_htb_fini (&re->hdrtab);

	if (re->orgqpath.buf) 
		QSE_MMGR_FREE (re->mmgr, re->orgqpath.buf);
}

void qse_htre_clear (qse_htre_t* re)
{
	if (!(re->state & QSE_HTRE_COMPLETED) && 
	    !(re->state & QSE_HTRE_DISCARDED))
	{
		if (re->concb)
		{
			re->concb (re, QSE_NULL, 0, re->concb_ctx); /* indicate end of content */
			qse_htre_unsetconcb (re);
		}
	}

	re->state = 0;
	re->flags = 0;

	re->orgqpath.ptr = QSE_NULL;
	re->orgqpath.len = 0;

	QSE_MEMSET (&re->version, 0, QSE_SIZEOF(re->version));
	QSE_MEMSET (&re->attr, 0, QSE_SIZEOF(re->attr));

	qse_htb_clear (&re->hdrtab);
	qse_htb_clear (&re->trailers);

	qse_mbs_clear (&re->content);
#if 0 
	qse_mbs_clear (&re->iniline);
#endif
}

const qse_htre_hdrval_t* qse_htre_getheaderval (
	const qse_htre_t* re, const qse_mchar_t* name)
{
	qse_htb_pair_t* pair;
	pair = qse_htb_search (&re->hdrtab, name, qse_mbslen(name));
	if (pair == QSE_NULL) return QSE_NULL;
	return QSE_HTB_VPTR(pair);
}

const qse_htre_hdrval_t* qse_htre_gettrailerval (
	const qse_htre_t* re, const qse_mchar_t* name)
{
	qse_htb_pair_t* pair;
	pair = qse_htb_search (&re->trailers, name, qse_mbslen(name));
	if (pair == QSE_NULL) return QSE_NULL;
	return QSE_HTB_VPTR(pair);
}

struct header_walker_ctx_t
{
	qse_htre_t* re;
	qse_htre_header_walker_t walker;
	void* ctx;
	int ret;
};

static qse_htb_walk_t walk_headers (
	qse_htb_t* htb, qse_htb_pair_t* pair, void* ctx)
{
	struct header_walker_ctx_t* hwctx = (struct header_walker_ctx_t*)ctx;
	if (hwctx->walker (hwctx->re, QSE_HTB_KPTR(pair), QSE_HTB_VPTR(pair), hwctx->ctx) <= -1) 
	{
		hwctx->ret = -1;
		return QSE_HTB_WALK_STOP;
	}
	return QSE_HTB_WALK_FORWARD;
}

int qse_htre_walkheaders (
	qse_htre_t* re, qse_htre_header_walker_t walker, void* ctx)
{
	struct header_walker_ctx_t hwctx;
	hwctx.re = re;
	hwctx.walker = walker;
	hwctx.ctx = ctx;
	hwctx.ret = 0;
	qse_htb_walk (&re->hdrtab, walk_headers, &hwctx);
	return hwctx.ret;
}

int qse_htre_walktrailers (
	qse_htre_t* re, qse_htre_header_walker_t walker, void* ctx)
{
	struct header_walker_ctx_t hwctx;
	hwctx.re = re;
	hwctx.walker = walker;
	hwctx.ctx = ctx;
	hwctx.ret = 0;
	qse_htb_walk (&re->trailers, walk_headers, &hwctx);
	return hwctx.ret;
}

int qse_htre_addcontent (
	qse_htre_t* re, const qse_mchar_t* ptr, qse_size_t len)
{
	/* see comments in qse_htre_discardcontent() */

	if (re->state & (QSE_HTRE_COMPLETED | QSE_HTRE_DISCARDED)) return 0; /* skipped */

	if (re->concb) 
	{
		/* if the callback is set, the content goes to the callback. */
		if (re->concb (re, ptr, len, re->concb_ctx) <= -1) return -1;
	}
	else
	{
		/* if the callback is not set, the contents goes to the internal buffer */
		if (qse_mbs_ncat (&re->content, ptr, len) == (qse_size_t)-1) return -1;
	}

	return 1; /* added successfully */
}

void qse_htre_completecontent (qse_htre_t* re)
{
	/* see comments in qse_htre_discardcontent() */

	if (!(re->state & QSE_HTRE_COMPLETED) && 
	    !(re->state & QSE_HTRE_DISCARDED))
	{
		re->state |= QSE_HTRE_COMPLETED;
		if (re->concb)
		{
			/* indicate end of content */
			re->concb (re, QSE_NULL, 0, re->concb_ctx); 
		}
	}
}

void qse_htre_discardcontent (qse_htre_t* re)
{
	/* you can't discard this if it's completed.
	 * you can't complete this if it's discarded 
	 * you can't add contents to this if it's completed or discarded
	 */

	if (!(re->state & QSE_HTRE_COMPLETED) &&
	    !(re->state & QSE_HTRE_DISCARDED))
	{
		re->state |= QSE_HTRE_DISCARDED;

		/* qse_htre_addcontent()...
		 * qse_thre_setconcb()...
		 * qse_htre_discardcontent()... <-- POINT A.
		 *
		 * at point A, the content must contain something
		 * and concb is also set. for simplicity, 
		 * clear the content buffer and invoke the callback 
		 *
		 * likewise, you may produce many weird combinations
		 * of these functions. however, these functions are
		 * designed to serve a certain usage pattern not including
		 * weird combinations.
		 */
		qse_mbs_clear (&re->content);
		if (re->concb)
		{
			/* indicate end of content */
			re->concb (re, QSE_NULL, 0, re->concb_ctx); 
		}
	}
}

void qse_htre_unsetconcb (qse_htre_t* re)
{
	re->concb = QSE_NULL;
	re->concb_ctx = QSE_NULL;
}

void qse_htre_setconcb (qse_htre_t* re, qse_htre_concb_t concb, void* ctx)
{
	re->concb = concb;
	re->concb_ctx = ctx;
}

int qse_htre_perdecqpath (qse_htre_t* re)
{
	qse_size_t dec_count;

	/* percent decode the query path*/

	if (re->type != QSE_HTRE_Q || (re->flags & QSE_HTRE_QPATH_PERDEC)) return -1;

	QSE_ASSERT (re->orgqpath.len <= 0);
	QSE_ASSERT (re->orgqpath.ptr == QSE_NULL);

	if (qse_isperencedhttpstr(re->u.q.path.ptr))
	{
		/* the string is percent-encoded. keep the original request
		 * in a separately allocated buffer */

		if (re->orgqpath.buf && re->u.q.path.len <= re->orgqpath.capa)
		{
			re->orgqpath.len = qse_mbscpy (re->orgqpath.buf, re->u.q.path.ptr);
			re->orgqpath.ptr = re->orgqpath.buf;
		}
		else
		{
			if (re->orgqpath.buf)
			{
				QSE_MMGR_FREE (re->mmgr, re->orgqpath.buf);
				re->orgqpath.capa = 0;
			}

			re->orgqpath.buf = qse_mbsxdup (re->u.q.path.ptr, re->u.q.path.len, re->mmgr);
			if (!re->orgqpath.buf) return -1;
			re->orgqpath.capa = re->u.q.path.len;

			re->orgqpath.ptr = re->orgqpath.buf;
			re->orgqpath.len = re->orgqpath.capa;

			/* orgqpath.buf and orgqpath.ptr are the same here. the caller
			 * is free to change orgqpath.ptr to point to a differnt position
			 * in the buffer. */
		}
	}

	re->u.q.path.len = qse_perdechttpstr (re->u.q.path.ptr, re->u.q.path.ptr, &dec_count);
	if (dec_count > 0) 
	{
		/* this assertion is to ensure that qse_isperencedhttpstr() 
		 * returned true when dec_count is greater than 0 */
		QSE_ASSERT (re->orgqpath.buf != QSE_NULL);
		QSE_ASSERT (re->orgqpath.ptr != QSE_NULL);
		re->flags |= QSE_HTRE_QPATH_PERDEC;
	}

	return 0;
}
