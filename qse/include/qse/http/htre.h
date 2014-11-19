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

#ifndef _QSE_HTTP_HTRE_H_
#define _QSE_HTTP_HTRE_H_

#include <qse/http/http.h>
#include <qse/cmn/htb.h>
#include <qse/cmn/str.h>

/* 
 * You should not manipulate an object of the #qse_htre_t 
 * type directly since it's complex. Use #qse_htrd_t to 
 * create an object of the qse_htre_t type.
 */

/* header and contents of request/response */
typedef struct qse_htre_t qse_htre_t;
typedef struct qse_htre_hdrval_t qse_htre_hdrval_t;

enum qse_htre_state_t
{
	QSE_HTRE_DISCARDED = (1 << 0), /** content has been discarded */
	QSE_HTRE_COMPLETED = (1 << 1)  /** complete content has been seen */
};
typedef enum qse_htre_state_t qse_htre_state_t;

typedef int (*qse_htre_concb_t) (
	qse_htre_t*        re,
	const qse_mchar_t* ptr,
	qse_size_t         len,
	void*              ctx
);

struct qse_htre_hdrval_t
{
	const qse_mchar_t* ptr;
	qse_size_t         len;
	qse_htre_hdrval_t* next;
};

struct qse_htre_t 
{
	qse_mmgr_t* mmgr;

	enum
	{
		QSE_HTRE_Q,
		QSE_HTRE_S
	} type;

	/* version */
	qse_http_version_t version;
	const qse_mchar_t* verstr; /* version string include HTTP/ */

	union
	{
		struct 
		{
			struct
			{
				qse_http_method_t type;
				const qse_mchar_t* name;
			} method;
			qse_mcstr_t path;
			qse_mcstr_t param;
		} q;
		struct
		{
			struct
			{
				int val;
				qse_mchar_t* str;
			} code;
			qse_mchar_t* mesg;
		} s;
	} u;

#define QSE_HTRE_ATTR_CHUNKED   (1 << 0)
#define QSE_HTRE_ATTR_LENGTH    (1 << 1)
#define QSE_HTRE_ATTR_KEEPALIVE (1 << 2)
#define QSE_HTRE_ATTR_EXPECT    (1 << 3)
#define QSE_HTRE_ATTR_EXPECT100 (1 << 4)
#define QSE_HTRE_ATTR_PROXIED   (1 << 5)
#define QSE_HTRE_QPATH_PERDEC   (1 << 6) /* the qpath has been percent-decoded */
	int flags;

	/* original query path for a request.
	 * meaningful if QSE_HTRE_QPATH_PERDEC is set in the flags */
	struct
	{
		qse_mchar_t* buf; /* buffer pointer */
		qse_size_t capa; /* buffer capacity */

		qse_mchar_t* ptr;
		qse_size_t len;
	} orgqpath;

	/* special attributes derived from the header */
	struct
	{
		qse_size_t content_length;
		const qse_mchar_t* status; /* for cgi */
	} attr;

	/* header table */
	qse_htb_t hdrtab;
	qse_htb_t trailers;
	
	/* content octets */
	qse_mbs_t content;

	/* content callback */
	qse_htre_concb_t concb;
	void* concb_ctx;

	/* bitwise-ORed of qse_htre_state_t */
	int state;
};

#define qse_htre_getversion(re) (&((re)->version))
#define qse_htre_getmajorversion(re) ((re)->version.major)
#define qse_htre_getminorversion(re) ((re)->version.minor)
#define qse_htre_getverstr(re) ((re)->verstr)

#define qse_htre_getqmethodtype(re) ((re)->u.q.method.type)
#define qse_htre_getqmethodname(re) ((re)->u.q.method.name)

#define qse_htre_getqpath(re) ((re)->u.q.path.ptr)
#define qse_htre_getqparam(re) ((re)->u.q.param.ptr)
#define qse_htre_getorgqpath(re) ((re)->orgqpath.ptr)

#define qse_htre_getscodeval(re) ((re)->u.s.code.val)
#define qse_htre_getscodestr(re) ((re)->u.s.code.str)
#define qse_htre_getsmesg(re) ((re)->u.s.mesg)

#define qse_htre_getcontent(re)     (&(re)->content)
#define qse_htre_getcontentxstr(re) QSE_MBS_XSTR(&(re)->content)
#define qse_htre_getcontentcstr(re) QSE_MBS_CSTR(&(re)->content)
#define qse_htre_getcontentptr(re)  QSE_MBS_PTR(&(re)->content)
#define qse_htre_getcontentlen(re)  QSE_MBS_LEN(&(re)->content)

typedef int (*qse_htre_header_walker_t) (
	qse_htre_t*              re,
	const qse_mchar_t*       key,
	const qse_htre_hdrval_t* val,
	void*                    ctx
);

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT int qse_htre_init (
	qse_htre_t* re,
	qse_mmgr_t* mmgr
);

QSE_EXPORT void qse_htre_fini (
	qse_htre_t* re
);

QSE_EXPORT void qse_htre_clear (
	qse_htre_t* re
);

QSE_EXPORT const qse_htre_hdrval_t* qse_htre_getheaderval (
	const qse_htre_t*  re, 
	const qse_mchar_t* key
);

QSE_EXPORT const qse_htre_hdrval_t* qse_htre_gettrailerval (
	const qse_htre_t*  re, 
	const qse_mchar_t* key
);

QSE_EXPORT int qse_htre_walkheaders (
	qse_htre_t*              re,
	qse_htre_header_walker_t walker,
	void*                    ctx
);

QSE_EXPORT int qse_htre_walktrailers (
	qse_htre_t*              re,
	qse_htre_header_walker_t walker,
	void*                    ctx
);

/**
 * The qse_htre_addcontent() function adds a content semgnet pointed to by
 * @a ptr of @a len bytes to the content buffer. If @a re is already completed
 * or discarded, this function returns 0 without adding the segment to the 
 * content buffer. 
 * @return 1 on success, -1 on failure, 0 if adding is skipped.
 */
QSE_EXPORT int qse_htre_addcontent (
	qse_htre_t*        re,
	const qse_mchar_t* ptr,
	qse_size_t         len
);

QSE_EXPORT void qse_htre_completecontent (
	qse_htre_t*      re
);

QSE_EXPORT void qse_htre_discardcontent (
	qse_htre_t*      re
);

QSE_EXPORT void qse_htre_unsetconcb (
	qse_htre_t*      re
);

QSE_EXPORT void qse_htre_setconcb (
	qse_htre_t*      re,
	qse_htre_concb_t concb, 
	void*            ctx
);

QSE_EXPORT int qse_htre_perdecqpath (
	qse_htre_t*      req
);

#if defined(__cplusplus)
}
#endif

#endif
