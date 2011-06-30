/*
 * $Id: http.h 223 2008-06-26 06:44:41Z baconevi $
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

#ifndef _QSE_HTTP_HTTP_H_
#define _QSE_HTTP_HTTP_H_

#include <qse/http/htre.h>

typedef struct qse_http_t qse_http_t;

enum qse_http_errnum_t
{
	QSE_HTTP_ENOERR,
	QSE_HTTP_ENOMEM,
	QSE_HTTP_EBADRE,
	QSE_HTTP_EBADHDR,
	QSE_HTTP_EREQCBS
};

typedef enum qse_http_errnum_t qse_http_errnum_t;

enum qse_http_option_t
{
	QSE_HTTP_LEADINGEMPTYLINES = (1 << 0)
};

typedef enum qse_http_option_t qse_http_option_t;

typedef struct qse_http_recbs_t qse_http_recbs_t;

struct qse_http_recbs_t
{
	int (*request)         (qse_http_t* http, qse_htre_t* req);
	int (*response)        (qse_http_t* http, qse_htre_t* res);
	int (*expect_continue) (qse_http_t* http, qse_htre_t* req);
};

struct qse_http_t
{
	QSE_DEFINE_COMMON_FIELDS (http)
	qse_http_errnum_t errnum;
	int option;

	qse_http_recbs_t recbs;

	struct
	{
		struct
		{
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


		/* buffers needed to for processing a request */
		struct
		{
			qse_htob_t raw; /* buffer to hold raw octets */
			qse_htob_t tra; /* buffer for handling trailers */
			qse_htob_t pen; /* buffer for raw octets during pending period */
		} b; 

		/* points to the head of the combined header list */
		void* chl;
	} fed; 

	enum 
	{
		QSE_HTTP_RETYPE_Q,
		QSE_HTTP_RETYPE_S
	} retype;

	qse_htre_t re;
};

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (http)

/**
 * The qse_http_open() function creates a http processor.
 */
qse_http_t* qse_http_open (
	qse_mmgr_t* mmgr,   /**< memory manager */
	qse_size_t  xtnsize /**< extension size in bytes */
);

/**
 * The qse_http_close() function destroys a http processor.
 */
void qse_http_close (
	qse_http_t* http 
);

qse_http_t* qse_http_init (
	qse_http_t* http,
	qse_mmgr_t* mmgr
);

void qse_http_fini (
	qse_http_t* http
);

void qse_http_clear (
	qse_http_t* http
);

int qse_http_getoption (
	qse_http_t* http	
);

void qse_http_setoption (
	qse_http_t* http,
	int         opts
);

const qse_http_recbs_t* qse_http_getrecbs (
	qse_http_t* http
);

void qse_http_setrecbs (
	qse_http_t*             http,
	const qse_http_recbs_t* recbs
);

/**
 * The qse_http_feed() function accepts http request octets and invokes a 
 * callback function if it has processed a proper http request. 
 */
int qse_http_feed (
	qse_http_t*       http, /**< http */
	const qse_htoc_t* req,  /**< request octets */
	qse_size_t        len   /**< number of octets */
);

#ifdef __cplusplus
}
#endif

#endif
