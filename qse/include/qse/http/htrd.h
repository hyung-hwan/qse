/*
 * $Id: htrd.h 223 2008-06-26 06:44:41Z baconevi $
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

#ifndef _QSE_HTTP_HTRD_H_
#define _QSE_HTTP_HTRD_H_

#include <qse/http/http.h>
#include <qse/http/htre.h>

typedef struct qse_htrd_t qse_htrd_t;

enum qse_htrd_errnum_t
{
	QSE_HTRD_ENOERR,
	QSE_HTRD_EOTHER,
	QSE_HTRD_ENOIMPL,
	QSE_HTRD_ESYSERR,
	QSE_HTRD_EINTERN,

	QSE_HTRD_ENOMEM,
	QSE_HTRD_EBADRE,
	QSE_HTRD_EBADHDR,
	QSE_HTRD_ERECBS,
	QSE_HTRD_ECONCB,
	QSE_HTRD_ESUSPENDED
};

typedef enum qse_htrd_errnum_t qse_htrd_errnum_t;

/**
 * The qse_htrd_option_t type defines various options to
 * change the behavior of the qse_htrd_t reader.
 */
enum qse_htrd_option_t
{
	QSE_HTRD_SKIPEMPTYLINES  = (1 << 0), /**< skip leading empty lines before the initial line */
	QSE_HTRD_SKIPINITIALLINE = (1 << 1), /**< skip processing an initial line */
	QSE_HTRD_CANONQPATH      = (1 << 2), /**< canonicalize the query path */
	QSE_HTRD_PEEKONLY        = (1 << 3), /**< trigger a peek callback after headers without processing contents */
	QSE_HTRD_REQUEST         = (1 << 4), /**< parse input as a request */
	QSE_HTRD_RESPONSE        = (1 << 5), /**< parse input as a response */
	QSE_HTRD_TRAILERS        = (1 << 6), /**< store trailers in a separate table */
	QSE_HTRD_STRICT          = (1 << 7)  /**< be more picky */
};

typedef enum qse_htrd_option_t qse_htrd_option_t;

typedef struct qse_htrd_recbs_t qse_htrd_recbs_t;

struct qse_htrd_recbs_t
{
	int  (*peek) (qse_htrd_t* htrd, qse_htre_t* re);
	int  (*poke) (qse_htrd_t* htrd, qse_htre_t* re);
};

struct qse_htrd_t
{
	qse_mmgr_t* mmgr;
	qse_htrd_errnum_t errnum;
	int option;
	int flags;

	const qse_htrd_recbs_t* recbs;

	struct
	{
		struct
		{
			int flags;

			int crlf; /* crlf status */
			qse_size_t plen; /* raw request length excluding crlf */
			qse_size_t need; /* number of octets needed for contents */

			struct
			{
				qse_size_t len;
				qse_size_t count;
				int        phase;
			} chunk;
		} s; /* state */

		/* buffers needed for processing a request */
		struct
		{
			qse_htob_t raw; /* buffer to hold raw octets */
			qse_htob_t tra; /* buffer for handling trailers */
		} b; 
	} fed; 

	qse_htre_t re;
	int        clean;
};

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * The qse_htrd_open() function creates a htrd processor.
 */
QSE_EXPORT qse_htrd_t* qse_htrd_open (
	qse_mmgr_t* mmgr,   /**< memory manager */
	qse_size_t  xtnsize /**< extension size in bytes */
);

/**
 * The qse_htrd_close() function destroys a htrd processor.
 */
QSE_EXPORT void qse_htrd_close (
	qse_htrd_t* htrd 
);

QSE_EXPORT int qse_htrd_init (
	qse_htrd_t* htrd,
	qse_mmgr_t* mmgr
);

QSE_EXPORT void qse_htrd_fini (
	qse_htrd_t* htrd
);

QSE_EXPORT qse_mmgr_t* qse_htrd_getmmgr (
	qse_htrd_t* htrd
); 

QSE_EXPORT void* qse_htrd_getxtn (
	qse_htrd_t* htrd
);

QSE_EXPORT qse_htrd_errnum_t qse_htrd_geterrnum (
	qse_htrd_t* htrd
);

QSE_EXPORT void qse_htrd_clear (
	qse_htrd_t* htrd
);

QSE_EXPORT int qse_htrd_getopt (
	qse_htrd_t* htrd
);

QSE_EXPORT void qse_htrd_setopt (
	qse_htrd_t* htrd,
	int         opts
);

QSE_EXPORT const qse_htrd_recbs_t* qse_htrd_getrecbs (
	qse_htrd_t* htrd
);

QSE_EXPORT void qse_htrd_setrecbs (
	qse_htrd_t*             htrd,
	const qse_htrd_recbs_t* recbs
);

/**
 * The qse_htrd_feed() function accepts htrd request octets and invokes a 
 * callback function if it has processed a proper htrd request. 
 */
QSE_EXPORT int qse_htrd_feed (
	qse_htrd_t*        htrd, /**< htrd */
	const qse_mchar_t* req,  /**< request octets */
	qse_size_t         len   /**< number of octets */
);

/**
 * The qse_htrd_halt() function indicates the end of feeeding
 * if the current response should be processed until the 
 * connection is closed.
 */ 
QSE_EXPORT int qse_htrd_halt (
	qse_htrd_t* htrd
);

QSE_EXPORT void qse_htrd_suspend (
	qse_htrd_t* htrd
);

QSE_EXPORT void qse_htrd_resume (
	qse_htrd_t* htrd
);

QSE_EXPORT void  qse_htrd_dummify (
	qse_htrd_t* htrd
);

QSE_EXPORT void qse_htrd_undummify (
	qse_htrd_t* htrd
);

QSE_EXPORT int qse_htrd_scanqparam (
	qse_htrd_t*        http,
	const qse_mcstr_t* cstr
);

#if defined(__cplusplus)
}
#endif

#endif
