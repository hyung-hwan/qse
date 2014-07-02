/*
 * $Id: http.h 223 2008-06-26 06:44:41Z baconevi $
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
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
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
typedef qse_mxstr_t qse_htos_t;

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

#ifdef __cplusplus
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
	const qse_mxstr_t* name
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

/* percent-decode a string */
QSE_EXPORT qse_size_t qse_perdechttpstr (
	const qse_mchar_t* str, 
	qse_mchar_t*       buf
);


/* percent-encode a string */
QSE_EXPORT qse_size_t qse_perenchttpstr (
	const qse_mchar_t* str, 
	qse_mchar_t*       buf
);

QSE_EXPORT qse_mchar_t* qse_perenchttpstrdup (
	const qse_mchar_t* str, 
	qse_mmgr_t*        mmgr
);

#ifdef __cplusplus
}
#endif


#endif
