/*
 * $Id: http.h 223 2008-06-26 06:44:41Z baconevi $
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

#ifndef _QSE_HTTP_HTTP_H_
#define _QSE_HTTP_HTTP_H_

/** @file
 * This file provides basic data types and functions for the http protocol.
 */

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/str.h>
#include <qse/cmn/htb.h>
#include <qse/cmn/time.h>

/* octet buffer */
typedef qse_mbs_t qse_htob_t;

/* octet string */
typedef qse_mcstr_t qse_htos_t;

/**
 * The qse_http_version_t type defines http version.
 */
struct qse_http_version_t
{
	short major; /**< major version */
	short minor; /**< minor version */
};

typedef struct qse_http_version_t qse_http_version_t;

/**
 * The qse_http_method_t type defines http methods .
 */
enum qse_http_method_t
{
	QSE_HTTP_OTHER,

	/* rfc 2616 */
	QSE_HTTP_HEAD,
	QSE_HTTP_GET,
	QSE_HTTP_POST,
	QSE_HTTP_PUT,
	QSE_HTTP_DELETE,
	QSE_HTTP_OPTIONS,
	QSE_HTTP_TRACE,
	QSE_HTTP_CONNECT

#if 0
	/* rfc 2518 */
	QSE_HTTP_PROPFIND,
	QSE_HTTP_PROPPATCH,
	QSE_HTTP_MKCOL,
	QSE_HTTP_COPY,
	QSE_HTTP_MOVE,
	QSE_HTTP_LOCK,
	QSE_HTTP_UNLOCK,

	/* rfc 3253 */
	QSE_HTTP_VERSION_CONTROL,
	QSE_HTTP_REPORT,
	QSE_HTTP_CHECKOUT,
	QSE_HTTP_CHECKIN,
	QSE_HTTP_UNCHECKOUT,
	QSE_HTTP_MKWORKSPACE,
	QSE_HTTP_UPDATE,
	QSE_HTTP_LABEL,
	QSE_HTTP_MERGE,
	QSE_HTTP_BASELINE_CONTROL,
	QSE_HTTP_MKACTIVITY,
	
	/* microsoft */
	QSE_HTTP_BPROPFIND,
	QSE_HTTP_BPROPPATCH,
	QSE_HTTP_BCOPY,
	QSE_HTTP_BDELETE,
	QSE_HTTP_BMOVE,
	QSE_HTTP_NOTIFY,
	QSE_HTTP_POLL,
	QSE_HTTP_SUBSCRIBE,
	QSE_HTTP_UNSUBSCRIBE,
#endif
};

typedef enum qse_http_method_t qse_http_method_t;

/** 
 * The #qse_http_range_int_t type defines an integer that can represent
 * a range offset. Depening on the size of #qse_foff_t, it is defined to
 * either #qse_foff_t or #qse_ulong_t.
 */
#if defined(QSE_SIZEOF_FOFF_T) && \
    defined(QSE_SIZEOF_ULONG_T) && \
    (QSE_SIZEOF_FOFF_T > QSE_SIZEOF_ULONG_T)
typedef qse_foff_t qse_http_range_int_t;
#else
typedef qse_ulong_t qse_http_range_int_t;
#endif

enum qse_http_range_type_t
{
	QSE_HTTP_RANGE_NONE,
	QSE_HTTP_RANGE_PROPER,
	QSE_HTTP_RANGE_SUFFIX
};
typedef enum qse_http_range_type_t qse_http_range_type_t;
/**
 * The qse_http_range_t type defines a structure that can represent
 * a value for the @b Range: http header. 
 *
 * If type is #QSE_HTTP_RANGE_NONE, this range is not valid.
 * 
 * If type is #QSE_HTTP_RANGE_SUFFIX, 'from' is meaningleass and 'to' indicates 
 * the number of bytes from the back. 
 *  - -500    => last 500 bytes
 *
 * You should adjust a range when the size that this range belongs to is 
 * made known. See this code:
 * @code
 *  range.from = total_size - range.to;
 *  range.to = range.to + range.from - 1;
 * @endcode
 *
 * If type is #QSE_HTTP_RANGE_PROPER, 'from' and 'to' represents a proper range
 * where the value of 0 indicates the first byte. This doesn't require any 
 * adjustment.
 *  - 0-999   => first 1000 bytes
 *  - 99-     => from the 100th bytes to the end.
 */
struct qse_http_range_t
{
	qse_http_range_type_t type; /**< type indicator */
	qse_http_range_int_t from;  /**< starting offset */
	qse_http_range_int_t to;    /**< ending offset */
};
typedef struct qse_http_range_t qse_http_range_t;


enum qse_perenchttpstr_opt_t
{
	QSE_PERENCHTTPSTR_KEEP_SLASH = (1 << 0)
};
typedef enum qse_perenchttpstr_opt_t qse_perenchttpstr_opt_t;

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT int qse_comparehttpversions (
	const qse_http_version_t* v1,
	const qse_http_version_t* v2
);

QSE_EXPORT const qse_mchar_t* qse_httpstatustombs (
	int code
);

QSE_EXPORT const qse_mchar_t* qse_httpmethodtombs (
	qse_http_method_t type
);

QSE_EXPORT qse_http_method_t qse_mbstohttpmethod (
	const qse_mchar_t* name
);

QSE_EXPORT qse_http_method_t qse_mcstrtohttpmethod (
	const qse_mcstr_t* name
);

QSE_EXPORT int qse_parsehttprange (
	const qse_mchar_t* str,
	qse_http_range_t* range
);

QSE_EXPORT int qse_parsehttptime (
	const qse_mchar_t* str,
	qse_ntime_t*       nt
);

QSE_EXPORT qse_mchar_t* qse_fmthttptime (
	const qse_ntime_t* nt,
	qse_mchar_t*       buf,
	qse_size_t         bufsz
);

/**
 * The qse_isperencedhttpstr() function determines if the given string
 * contains a valid percent-encoded sequence.
 */
QSE_EXPORT int qse_isperencedhttpstr (
	const qse_mchar_t* str
);

/**
 * The qse_perdechttpstr() function performs percent-decoding over a string.
 * The caller must ensure that the output buffer \a buf is large enough.
 * If \a ndecs is not #QSE_NULL, it is set to the number of characters
 * decoded.  0 means no characters in the input string required decoding
 * \return the length of the output string.
 */
QSE_EXPORT qse_size_t qse_perdechttpstr (
	const qse_mchar_t* str, 
	qse_mchar_t*       buf,
	qse_size_t*        ndecs
);

/**
 * The qse_perenchttpstr() function performs percent-encoding over a string.
 * The caller must ensure that the output buffer \a buf is large enough.
 * If \a nencs is not #QSE_NULL, it is set to the number of characters
 * encoded.  0 means no characters in the input string required encoding.
 * \return the length of the output string.
 */
QSE_EXPORT qse_size_t qse_perenchttpstr (
	int                opt, /**< 0 or bitwise-OR'ed of #qse_perenchttpstr_opt_t */
	const qse_mchar_t* str, 
	qse_mchar_t*       buf,
	qse_size_t*        nencs
);

QSE_EXPORT qse_mchar_t* qse_perenchttpstrdup (
	int                opt, /**< 0 or bitwise-OR'ed of #qse_perenchttpstr_opt_t */
	const qse_mchar_t* str, 
	qse_mmgr_t*        mmgr
);

#if defined(__cplusplus)
}
#endif


#endif
