/*
 * $Id: htrd.h 223 2008-06-26 06:44:41Z baconevi $
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

#ifndef _QSE_NET_HTTPD_H_
#define _QSE_NET_HTTPD_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/net/htre.h>

typedef struct qse_httpd_t        qse_httpd_t;
typedef struct qse_httpd_client_t qse_httpd_client_t;

enum qse_httpd_errnum_t
{
	QSE_HTTPD_ENOERR,
	QSE_HTTPD_ENOMEM,
	QSE_HTTPD_EINVAL,
	QSE_HTTPD_EINTERN,
	QSE_HTTPD_EIOMUX,
	QSE_HTTPD_ESOCKET,
	QSE_HTTPD_EDISCON, /* client disconnnected */
	QSE_HTTPD_EBADREQ, /* bad request */
	QSE_HTTPD_ETASK,
	QSE_HTTPD_ECOMCBS
};
typedef enum qse_httpd_errnum_t qse_httpd_errnum_t;

typedef struct qse_httpd_cbs_t qse_httpd_cbs_t;
struct qse_httpd_cbs_t
{
	int (*handle_request) (
		qse_httpd_t* httpd, qse_httpd_client_t* client, qse_htre_t* req);
	int (*handle_expect_continue) (
		qse_httpd_t* httpd, qse_httpd_client_t* client, qse_htre_t* req);

	const qse_mchar_t* (*getmimetype) (qse_httpd_t* httpd, const qse_mchar_t* path);
	int (*listdir) (qse_httpd_t* httpd, const qse_mchar_t* path);
};

typedef struct qse_httpd_task_t qse_httpd_task_t;

typedef int (*qse_httpd_task_init_t) (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client,
	qse_httpd_task_t* task
);

typedef void (*qse_httpd_task_fini_t) (	
	qse_httpd_t* httpd,
	qse_httpd_client_t* client,
	qse_httpd_task_t* task
);

typedef int (*qse_httpd_task_main_t) (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client,
	qse_httpd_task_t* task
);

struct qse_httpd_task_t
{
	/* you must not call another entask functions from within initailizer */
	qse_httpd_task_init_t init;
	qse_httpd_task_fini_t fini;
	qse_httpd_task_main_t main;
	void*                 ctx;
};

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (httpd)

/**
 * The qse_httpd_open() function creates a httpd processor.
 */
qse_httpd_t* qse_httpd_open (
	qse_mmgr_t* mmgr,   /**< memory manager */
	qse_size_t  xtnsize /**< extension size in bytes */
);

/**
 * The qse_httpd_close() function destroys a httpd processor.
 */
void qse_httpd_close (
	qse_httpd_t* httpd 
);

const qse_httpd_cbs_t* qse_httpd_getcbs (
	qse_httpd_t* httpd
);

void qse_httpd_setcbs (
	qse_httpd_t*     httpd,
	qse_httpd_cbs_t* cbs
);

/**
 * The qse_httpd_loop() function starts a httpd server loop.
 * If @a threaded is non-zero, it creates a separate output thread.
 * If no thread support is available, it is ignored.
 *
 * @note
 * In the future, the @a threaded parameter will be extended to
 * specify the number of output threads.
 */
int qse_httpd_loop (
	qse_httpd_t* httpd, 
	int          threaded 
);

/**
 * The qse_httpd_stop() function requests to stop qse_httpd_loop()
 */
void qse_httpd_stop (
	qse_httpd_t* httpd
);


int qse_httpd_addlistener (
	qse_httpd_t*      httpd,
	const qse_char_t* uri
);


void qse_httpd_markclientbad (
	qse_httpd_t*        httpd,
	qse_httpd_client_t* client
);

#define qse_httpd_gettaskxtn(httpd,task) ((void*)(task+1))

qse_httpd_task_t* qse_httpd_entask (
	qse_httpd_t*            httpd,
	qse_httpd_client_t*     client,
	const qse_httpd_task_t* pred,
	const qse_httpd_task_t* task,
	qse_size_t              xtnsize
);

qse_httpd_task_t* qse_httpd_entaskdisconnect (
	qse_httpd_t*            httpd,
	qse_httpd_client_t*     client,
	const qse_httpd_task_t* pred
);

qse_httpd_task_t* qse_httpd_entasktext (
	qse_httpd_t*            httpd,
	qse_httpd_client_t*     client,
	const qse_httpd_task_t* pred,
	const qse_mchar_t*      text
);

qse_httpd_task_t* qse_httpd_entaskstatictext (
     qse_httpd_t*            httpd,
	qse_httpd_client_t*     client,
	const qse_httpd_task_t* pred,
	const qse_mchar_t*      text
);

qse_httpd_task_t* qse_httpd_entaskformat (
	qse_httpd_t*            httpd,
	qse_httpd_client_t*     client,
	const qse_httpd_task_t* pred,
	const qse_mchar_t*      fmt,
	...
);

qse_httpd_task_t* qse_httpd_entaskfile (
	qse_httpd_t*            httpd,
	qse_httpd_client_t*     client,
	const qse_httpd_task_t* pred,
	qse_ubi_t               handle,
	qse_foff_t              offset,
	qse_foff_t              size
);

qse_httpd_task_t* qse_httpd_entaskdir (
	qse_httpd_t*            httpd,
	qse_httpd_client_t*     client,
	const qse_httpd_task_t* pred,
	qse_ubi_t               handle
);

qse_httpd_task_t* qse_httpd_entaskpath (
	qse_httpd_t*              httpd,
	qse_httpd_client_t*       client,
	const qse_httpd_task_t*   pred,
	const qse_mchar_t*        name,
	const qse_http_range_t*   range,
	const qse_http_version_t* version,
	int                       keepalive
);

qse_httpd_task_t* qse_httpd_entaskcgi (
	qse_httpd_t*              httpd,
	qse_httpd_client_t*       client,
	const qse_httpd_task_t*   pred,
	const qse_mchar_t*        path,
	const qse_htre_t*         req
);

void* qse_httpd_allocmem (
	qse_httpd_t* httpd, 
	qse_size_t   size
);

void* qse_httpd_reallocmem (
	qse_httpd_t* httpd,
	void*        ptr,
	qse_size_t   size
);

void qse_httpd_freemem (
	qse_httpd_t* httpd,
	void*        ptr
);

#ifdef __cplusplus
}
#endif

#endif
