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
#include <qse/net/htrd.h>
#include <qse/cmn/nwad.h>
#include <qse/cmn/time.h>

typedef struct qse_httpd_t        qse_httpd_t;
typedef struct qse_httpd_client_t qse_httpd_client_t;

enum qse_httpd_errnum_t
{
	QSE_HTTPD_ENOERR,

	QSE_HTTPD_ENOMEM,
	QSE_HTTPD_EINVAL,
	QSE_HTTPD_EACCES,
	QSE_HTTPD_ENOENT,
	QSE_HTTPD_EEXIST,
	QSE_HTTPD_EINTR,
	QSE_HTTPD_EAGAIN,

	QSE_HTTPD_EIOMUX,
	QSE_HTTPD_EDISCON, /* client disconnnected */
	QSE_HTTPD_EBADREQ, /* bad request */
	QSE_HTTPD_ETASK,

	QSE_HTTPD_EINTERN,
	QSE_HTTPD_ESYSERR,
	QSE_HTTPD_ENOIMPL,

	QSE_HTTPD_EOTHER
};
typedef enum qse_httpd_errnum_t qse_httpd_errnum_t;

enum qse_httpd_option_t
{
	QSE_HTTPD_CGIERRTONUL  = (1 << 0),
	QSE_HTTPD_CGINOCLOEXEC = (1 << 1)
};

typedef struct qse_httpd_stat_t qse_httpd_stat_t;
struct qse_httpd_stat_t
{
	qse_foff_t size;
	qse_ntime_t mtime;
	const qse_mchar_t* mime;
};

typedef struct qse_httpd_server_t qse_httpd_server_t;
struct qse_httpd_server_t
{
	qse_httpd_server_t* next;
	int        active;

	qse_nwad_t nwad;
	int        secure;

	/* set by server.open callback */
	qse_ubi_t  handle;
};

typedef struct qse_httpd_peer_t qse_httpd_peer_t;
struct qse_httpd_peer_t
{
	qse_nwad_t nwad;
	qse_ubi_t  handle;
};

enum qse_httpd_mux_mask_t
{
	QSE_HTTPD_MUX_READ  = (1 << 0),
	QSE_HTTPD_MUX_WRITE = (1 << 1)
};

typedef int (*qse_httpd_muxcb_t) (
	qse_httpd_t* httpd,
	void*        mux,
	qse_ubi_t    handle,
	int          mask, /* ORed of qse_httpd_mux_mask_t */
	void*        cbarg
);

typedef struct qse_httpd_cbs_t qse_httpd_cbs_t;
struct qse_httpd_cbs_t
{
	struct
	{
		int (*open) (qse_httpd_t* httpd, qse_httpd_server_t* server);
		void (*close) (qse_httpd_t* httpd, qse_httpd_server_t* server);
		int (*accept) (qse_httpd_t* httpd, qse_httpd_server_t* server, qse_httpd_client_t* client);
	} server;

	struct
	{
		int (*open) (qse_httpd_t* httpd, qse_httpd_peer_t* peer);
		void (*close) (qse_httpd_t* httpd, qse_httpd_peer_t* peer);
		int (*connected) (qse_httpd_t* httpd, qse_httpd_peer_t* peer);

		qse_ssize_t (*recv) (
			qse_httpd_t* httpd, 
			qse_httpd_peer_t* peer,
			qse_mchar_t* buf, qse_size_t bufsize);

		qse_ssize_t (*send) (
			qse_httpd_t* httpd,
			qse_httpd_peer_t* peer,
			const qse_mchar_t* buf, qse_size_t bufsize);
	} peer;

	struct
	{
		void* (*open)   (qse_httpd_t* httpd);
		void  (*close)  (qse_httpd_t* httpd, void* mux);
		int   (*addhnd) (
			qse_httpd_t* httpd, void* mux, qse_ubi_t handle, 
			int mask, qse_httpd_muxcb_t cbfun, void* cbarg);
		int   (*delhnd) (qse_httpd_t* httpd, void* mux, qse_ubi_t handle);
		int   (*poll)   (qse_httpd_t* httpd, void* mux, qse_ntime_t timeout);

		int (*readable) (
			qse_httpd_t* httpd, qse_ubi_t handle, qse_ntoff_t timeout);
		int (*writable) (
			qse_httpd_t* httpd, qse_ubi_t handle, qse_ntoff_t timeout);
	} mux;

	struct
	{
		int (*executable) (
			qse_httpd_t* httpd, const qse_mchar_t* path);

		int (*stat) (
			qse_httpd_t* httpd, const qse_mchar_t* path, 
			qse_httpd_stat_t* stat);
			
		int (*ropen) (
			qse_httpd_t* httpd, const qse_mchar_t* path, 
			qse_ubi_t* handle);
		int (*wopen) (
			qse_httpd_t* httpd, const qse_mchar_t* path, 
			qse_ubi_t* handle);
		void (*close) (qse_httpd_t* httpd, qse_ubi_t handle);

		qse_ssize_t (*read) (
			qse_httpd_t* httpd, qse_ubi_t handle,
			qse_mchar_t* buf, qse_size_t len);
		qse_ssize_t (*write) (
			qse_httpd_t* httpd, qse_ubi_t handle,
			const qse_mchar_t* buf, qse_size_t len);
	} file;

	struct
	{
		void (*close) (
			qse_httpd_t* httpd,
			qse_httpd_client_t* client);

		void (*shutdown) (
			qse_httpd_t* httpd,
			qse_httpd_client_t* client);

		/* action */
		qse_ssize_t (*recv) (
			qse_httpd_t* httpd, 
			qse_httpd_client_t* client,
			qse_mchar_t* buf, qse_size_t bufsize);

		qse_ssize_t (*send) (
			qse_httpd_t* httpd,
			qse_httpd_client_t* client,
			const qse_mchar_t* buf, qse_size_t bufsize);

		qse_ssize_t (*sendfile) (
			qse_httpd_t* httpd,
			qse_httpd_client_t* client,
			qse_ubi_t handle, qse_foff_t* offset, qse_size_t count);

		/* event notification */
		int (*accepted) (
			qse_httpd_t* httpd,
			qse_httpd_client_t* client);  /* optional */
		void (*closed) (
			qse_httpd_t* httpd,
			qse_httpd_client_t* client);  /* optional */
	} client;

	int (*peek_request) (
		qse_httpd_t* httpd, qse_httpd_client_t* client, qse_htre_t* req);
	int (*handle_request) (
		qse_httpd_t* httpd, qse_httpd_client_t* client, qse_htre_t* req);

	int (*listdir) (qse_httpd_t* httpd, const qse_mchar_t* path);
};

typedef struct qse_httpd_task_t qse_httpd_task_t;

typedef int (*qse_httpd_task_init_t) (
	qse_httpd_t*        httpd,
	qse_httpd_client_t* client,
	qse_httpd_task_t*   task
);

typedef void (*qse_httpd_task_fini_t) (	
	qse_httpd_t*        httpd,
	qse_httpd_client_t* client,
	qse_httpd_task_t*   task
);

typedef int (*qse_httpd_task_main_t) (
	qse_httpd_t*        httpd,
	qse_httpd_client_t* client,
	qse_httpd_task_t*   task
);


enum qse_httpd_task_trigger_mask_t
{
	QSE_HTTPD_TASK_TRIGGER_READ      = (1 << 0),
	QSE_HTTPD_TASK_TRIGGER_RELAY     = (1 << 1),
	QSE_HTTPD_TASK_TRIGGER_WRITE     = (1 << 2),
	QSE_HTTPD_TASK_TRIGGER_READABLE  = (1 << 3),
	QSE_HTTPD_TASK_TRIGGER_RELAYABLE = (1 << 4),
	QSE_HTTPD_TASK_TRIGGER_WRITABLE  = (1 << 5)

};

typedef struct qse_httpd_task_trigger_t qse_httpd_task_trigger_t;
struct qse_httpd_task_trigger_t
{
	int       mask; /* QSE_HTTPD_TASK_TRIGGER_READ | QSE_HTTPD_TASK_TRIGGER_WRITE */
	qse_ubi_t handle;
};

#define QSE_HTTPD_TASK_TRIGGER_MAX 3

struct qse_httpd_task_t
{
	/* == PUBLIC  == */

	/* you must not call another entask functions from within 
	 * an initailizer. you can call entask functions from within 
	 * a finalizer and a main function. */
	qse_httpd_task_init_t    init;
	qse_httpd_task_fini_t    fini;
	qse_httpd_task_main_t    main;
	qse_httpd_task_trigger_t trigger[QSE_HTTPD_TASK_TRIGGER_MAX];
	void*                    ctx;

	/* == PRIVATE  == */
	qse_httpd_task_t*     prev;
	qse_httpd_task_t*     next;
};


struct qse_httpd_client_t
{
	/* == PUBLIC  == */

	qse_ubi_t                handle;
	qse_ubi_t                handle2;
	qse_nwad_t               remote_addr;
	qse_nwad_t               local_addr;
	qse_nwad_t               orgdst_addr;

	/* == PRIVATE == */
	qse_htrd_t*              htrd;
	int                      secure;
	int                      status;
	qse_httpd_task_trigger_t trigger[QSE_HTTPD_TASK_TRIGGER_MAX];
	qse_ntime_t              last_active;

	qse_httpd_client_t*      prev;
	qse_httpd_client_t*      next;
 
	qse_httpd_client_t*      bad_next;

	qse_httpd_client_t*      prev_tasked;
	qse_httpd_client_t*      next_tasked;

	struct
	{
		int count;
		qse_httpd_task_t* head;
		qse_httpd_task_t* tail;
	} task;
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

qse_httpd_errnum_t qse_httpd_geterrnum (
	qse_httpd_t* httpd
);

void qse_httpd_seterrnum (
	qse_httpd_t*       httpd,
	qse_httpd_errnum_t errnum
);

int qse_httpd_getoption (
	qse_httpd_t* httpd
);

void qse_httpd_setoption (
	qse_httpd_t* httpd,
	int          option
);

/**
 * The qse_httpd_loop() function starts a httpd server loop.
 */
int qse_httpd_loop (
	qse_httpd_t*     httpd, 
	qse_httpd_cbs_t* cbs,
	qse_ntime_t      timeout
);

/**
 * The qse_httpd_stop() function requests to stop qse_httpd_loop()
 */
void qse_httpd_stop (
	qse_httpd_t* httpd
);


int qse_httpd_addserver (
	qse_httpd_t*      httpd,
	const qse_char_t* uri
);

void qse_httpd_discardcontent (
	qse_httpd_t*        httpd,
	qse_htre_t*         req
);

void qse_httpd_completecontent (
	qse_httpd_t*        httpd,
	qse_htre_t*         req
);

#define qse_httpd_gettaskxtn(httpd,task) ((void*)(task+1))

qse_httpd_task_t* qse_httpd_entask (
	qse_httpd_t*            httpd,
	qse_httpd_client_t*     client,
	qse_httpd_task_t*       pred,
	const qse_httpd_task_t* task,
	qse_size_t              xtnsize
);

/* -------------------------------------------- */

qse_httpd_task_t* qse_httpd_entaskdisconnect (
	qse_httpd_t*            httpd,
	qse_httpd_client_t*     client,
	qse_httpd_task_t*       pred
);

qse_httpd_task_t* qse_httpd_entasktext (
	qse_httpd_t*            httpd,
	qse_httpd_client_t*     client,
	qse_httpd_task_t*       pred,
	const qse_mchar_t*      text
);

qse_httpd_task_t* qse_httpd_entaskstatictext (
     qse_httpd_t*            httpd,
	qse_httpd_client_t*     client,
	qse_httpd_task_t*       pred,
	const qse_mchar_t*      text
);

qse_httpd_task_t* qse_httpd_entaskformat (
	qse_httpd_t*            httpd,
	qse_httpd_client_t*     client,
	qse_httpd_task_t*       pred,
	const qse_mchar_t*      fmt,
	...
);

/* -------------------------------------------- */

qse_httpd_task_t* qse_httpd_entaskerror (
     qse_httpd_t*              httpd,
	qse_httpd_client_t*       client,
	qse_httpd_task_t*         pred,
     int                       code, 
	qse_htre_t*               req
);

qse_httpd_task_t* qse_httpd_entaskcontinue (
     qse_httpd_t*              httpd,
	qse_httpd_client_t*       client,
	qse_httpd_task_t*         pred,
	qse_htre_t*               req
);

/**
 * The qse_httpd_entaskauth() function adds a basic authorization task.
 */
qse_httpd_task_t* qse_httpd_entaskauth (
     qse_httpd_t*              httpd,
	qse_httpd_client_t*       client,
	qse_httpd_task_t*         pred,
	const qse_mchar_t*        realm,
	qse_htre_t*               req
);

qse_httpd_task_t* qse_httpd_entaskdir (
	qse_httpd_t*              httpd,
	qse_httpd_client_t*       client,
	qse_httpd_task_t*         pred,
	const qse_mchar_t*        name,
	qse_htre_t*               req
);

qse_httpd_task_t* qse_httpd_entaskfile (
	qse_httpd_t*              httpd,
	qse_httpd_client_t*       client,
	qse_httpd_task_t*         pred,
	const qse_mchar_t*        name,
	qse_htre_t*               req
);

qse_httpd_task_t* qse_httpd_entaskcgi (
	qse_httpd_t*              httpd,
	qse_httpd_client_t*       client,
	qse_httpd_task_t*         pred,
	const qse_mchar_t*        path,
	qse_htre_t*               req
);

qse_httpd_task_t* qse_httpd_entasknph (
	qse_httpd_t*              httpd,
	qse_httpd_client_t*       client,
	qse_httpd_task_t*         pred,
	const qse_mchar_t*        path,
	qse_htre_t*               req
);

qse_httpd_task_t* qse_httpd_entaskproxy (
	qse_httpd_t*            httpd,
	qse_httpd_client_t*     client,
	qse_httpd_task_t*       pred,
	const qse_nwad_t*       nwad,
	qse_htre_t*             req
);

/* -------------------------------------------- */

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
