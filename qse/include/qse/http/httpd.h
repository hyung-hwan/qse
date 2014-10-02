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

#ifndef _QSE_HTTP_HTTPD_H_
#define _QSE_HTTP_HTTPD_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/http/htre.h>
#include <qse/http/htrd.h>
#include <qse/cmn/nwad.h>
#include <qse/cmn/time.h>
#include <qse/cmn/tmr.h>
#include <qse/cmn/env.h>


typedef struct qse_httpd_t        qse_httpd_t;
typedef struct qse_httpd_mate_t   qse_httpd_mate_t;
typedef struct qse_httpd_server_t qse_httpd_server_t;
typedef struct qse_httpd_client_t qse_httpd_client_t;
typedef struct qse_httpd_dns_t    qse_httpd_dns_t;
typedef struct qse_httpd_urs_t    qse_httpd_urs_t;

typedef qse_intptr_t qse_httpd_hnd_t;

enum qse_httpd_errnum_t
{
	QSE_HTTPD_ENOERR,
	QSE_HTTPD_EOTHER,
	QSE_HTTPD_ENOIMPL,
	QSE_HTTPD_ESYSERR,
	QSE_HTTPD_EINTERN,

	QSE_HTTPD_ENOMEM,
	QSE_HTTPD_EINVAL,
	QSE_HTTPD_EACCES,
	QSE_HTTPD_ENOENT,
	QSE_HTTPD_EEXIST,
	QSE_HTTPD_EINTR,
	QSE_HTTPD_EPIPE,
	QSE_HTTPD_EAGAIN,

	QSE_HTTPD_ENOSVR,  /* no active servers */
	QSE_HTTPD_ECONN,
	QSE_HTTPD_ENOBUF,  /* no buffer available */
	QSE_HTTPD_EDISCON, /* client disconnnected */
	QSE_HTTPD_EBADREQ, /* bad request */
	QSE_HTTPD_ENODNS,  /* dns service not activated/enabled or no valid dns server specified */
	QSE_HTTPD_ENOURS,  /* urs service not activated/enabled or no valid urs server specified */
	QSE_HTTPD_ETASK
};
typedef enum qse_httpd_errnum_t qse_httpd_errnum_t;

enum qse_httpd_opt_t
{
	QSE_HTTPD_TRAIT,
	QSE_HTTPD_TMOUT,
	QSE_HTTPD_IDLELIMIT,
	QSE_HTTPD_MODPREFIX,
	QSE_HTTPD_MODPOSTFIX,
	QSE_HTTPD_SCB,
	QSE_HTTPD_RCB
};
typedef enum qse_httpd_opt_t qse_httpd_opt_t;

enum qse_httpd_trait_t
{
	QSE_HTTPD_MUTECLIENT   = (1 << 0),
	QSE_HTTPD_CGIERRTONUL  = (1 << 1),
	QSE_HTTPD_CGINOCLOEXEC = (1 << 2),
	QSE_HTTPD_CGINOCHUNKED = (1 << 3),
	QSE_HTTPD_PROXYNOVIA   = (1 << 4),
	QSE_HTTPD_LOGACT       = (1 << 5)
};
typedef enum qse_httpd_trait_t qse_httpd_trait_t;

typedef struct qse_httpd_mod_t qse_httpd_mod_t;

typedef int (*qse_httpd_mod_load_t) (
	qse_httpd_mod_t* mod
);

typedef void (*qse_httpd_mod_unload_t) (
	qse_httpd_mod_t* mod
);

typedef int (*qse_httpd_mod_dns_preresolve_t) (
	qse_httpd_mod_t*    mod,
	qse_httpd_client_t* client,
	const qse_mchar_t*  host,
	qse_nwad_t*         nwad
);

typedef int (*qse_httpd_mod_urs_prerewrite_t) (
	qse_httpd_mod_t*    mod,
	qse_httpd_client_t* client,
	qse_htre_t*         req,
	const qse_mchar_t*  host,
	qse_mchar_t**       url
);

struct qse_httpd_mod_t
{
	/* private */
	qse_httpd_mod_t* next;
	void* handle; /* set to the return value of mod.open() */

	/* module may access these fields for rererence  */
	qse_httpd_t* httpd;
	qse_char_t* name; /* portable module name */
	qse_char_t* fullname; /* name to use when loading module from the system. */

	/* module's entry point may set these items */
	void* ctx; 
	qse_httpd_mod_unload_t unload;
	qse_httpd_mod_dns_preresolve_t dns_preresolve;
	qse_httpd_mod_urs_prerewrite_t urs_prerewrite;

	/* more fields will get added here for expansion in the future. */
};

typedef struct qse_httpd_stat_t qse_httpd_stat_t;
struct qse_httpd_stat_t
{
	int         isdir;
	qse_long_t  dev;
	qse_long_t  ino;
	qse_foff_t  size;
	qse_ntime_t mtime;
};

typedef struct qse_httpd_peer_t qse_httpd_peer_t;
struct qse_httpd_peer_t
{
	qse_nwad_t nwad;
	qse_nwad_t local; /* local side address facing the peer */
	qse_httpd_hnd_t  handle;
};

enum qse_httpd_mux_mask_t
{
	QSE_HTTPD_MUX_READ  = (1 << 0),
	QSE_HTTPD_MUX_WRITE = (1 << 1)
};

typedef int (*qse_httpd_muxcb_t) (
	qse_httpd_t* httpd,
	void*        mux,
	qse_httpd_hnd_t    handle,
	int          mask, /* ORed of qse_httpd_mux_mask_t */
	void*        cbarg
);

typedef struct qse_httpd_dirent_t qse_httpd_dirent_t;

struct qse_httpd_dirent_t
{
	const qse_mchar_t* name;
	qse_httpd_stat_t   stat;
};


/* -------------------------------------------------------------------------- */
enum qse_httpd_dns_server_flag_t
{
	QSE_HTTPD_DNS_SERVER_A    = (1 << 0), /* send A query */
	QSE_HTTPD_DNS_SERVER_AAAA = (1 << 1)  /* send AAAA query */
};

typedef enum qse_httpd_dns_server_flag_t qse_httpd_dns_server_flag_t;


typedef struct qse_httpd_dns_server_t qse_httpd_dns_server_t;
struct qse_httpd_dns_server_t
{
	qse_nwad_t  nwad;
	qse_ntime_t tmout;
	int         retries;
	int         flags; /* bitwise-ORed of qse_httpd_dns_server_flag_t enumerators */
};

typedef struct qse_httpd_urs_server_t qse_httpd_urs_server_t;
struct qse_httpd_urs_server_t
{
	qse_nwad_t  nwad;
	qse_ntime_t tmout;
	int         retries;
};

/* -------------------------------------------------------------------------- */

typedef void (*qse_httpd_resolve_t) (
	qse_httpd_t*       httpd,
	const qse_mchar_t* name,
	const qse_nwad_t*  nwad,
	void*              ctx
);

typedef void (*qse_httpd_rewrite_t) (
	qse_httpd_t*       httpd,
	/* optional original URL. if the original URL is not 
	 * avaialble,it can be set to #QSE_NULL */
	const qse_mchar_t* url,     
	/* rewritten URL */
	const qse_mchar_t* new_url,
	/* content data pointer */
	void*              ctx
);


typedef int (*qse_httpd_dns_preresolve_t) (
	qse_httpd_t*        httpd, 
	qse_httpd_client_t* client,
	const qse_mchar_t*  host,
	qse_nwad_t*         nwad
);


typedef int (*qse_httpd_urs_open_t) (
	qse_httpd_t*     httpd, 
	qse_httpd_urs_t* urs
);

typedef void (*qse_httpd_urs_close_t) (
	qse_httpd_t*     httpd,
	qse_httpd_urs_t* urs
);

typedef int (*qse_httpd_urs_recv_t) (
	qse_httpd_t*     httpd,
	qse_httpd_urs_t* urs,
	qse_httpd_hnd_t        handle
);

typedef int (*qse_httpd_urs_send_t) (
	qse_httpd_t*                  httpd,
	qse_httpd_urs_t*              urs, 
	const qse_mchar_t*            url,
	qse_httpd_rewrite_t           rewrite,
	const qse_httpd_urs_server_t* urs_server,
	void*                         ctx
);

/* Success is indicated by a positive return value including 0. 
 * When it's 0, url must point to #QSE_NULL or a null-teminated 
 * string. When it's greater than 0, url must point to a null-terminated
 * string. The null-terminated string is freed with qse_httpd_freemem()
 * when not needed. 
 * 
 * The return value of 0 indicates that the string is the final 
 * rewriting result and no sending to urs-server is required. The URL of 
 * #QSE_NULL , when the return value is 0, indicates that no translation
 * is required. 
 *
 * If the return value is greater than 0, the string is sent to the
 * urs-server for the actual rewriting. 
 * 
 * A negative return value indicates failure. 
 */
typedef int (*qse_httpd_urs_prerewrite_t) (
	qse_httpd_t*        httpd, 
	qse_httpd_client_t* client,
	qse_htre_t*         req,
	const qse_mchar_t*  host,
	qse_mchar_t**       url
);

typedef struct qse_httpd_scb_t qse_httpd_scb_t;
struct qse_httpd_scb_t
{
	struct
	{
		void* (*open) (qse_httpd_t* httpd, const qse_char_t* fullname);
		void (*close) (qse_httpd_t* httpd, void* handle);
		void* (*symbol) (qse_httpd_t* httpd, void* handle, const qse_char_t* name);
	} mod; /* module */

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
		void* (*open)   (qse_httpd_t* httpd, qse_httpd_muxcb_t muxcb);
		void  (*close)  (qse_httpd_t* httpd, void* mux);
		int   (*addhnd) (qse_httpd_t* httpd, void* mux, qse_httpd_hnd_t handle, int mask, void* cbarg);
		int   (*delhnd) (qse_httpd_t* httpd, void* mux, qse_httpd_hnd_t handle);
		int   (*poll)   (qse_httpd_t* httpd, void* mux, const qse_ntime_t* tmout);

		int (*readable) (qse_httpd_t* httpd, qse_httpd_hnd_t handle, const qse_ntime_t* tmout);
		int (*writable) (qse_httpd_t* httpd, qse_httpd_hnd_t handle, const qse_ntime_t* tmout);
	} mux;

	struct
	{
		int (*stat) (qse_httpd_t* httpd, const qse_mchar_t* path, qse_httpd_stat_t* stat);
		int (*purge) (qse_httpd_t* httpd, const qse_mchar_t* path);
			
		int (*ropen) (
			qse_httpd_t* httpd, const qse_mchar_t* path, 
			qse_httpd_hnd_t* handle);
		int (*wopen) (
			qse_httpd_t* httpd, const qse_mchar_t* path, 
			qse_httpd_hnd_t* handle);
		void (*close) (qse_httpd_t* httpd, qse_httpd_hnd_t handle);

		qse_ssize_t (*read) (
			qse_httpd_t* httpd, qse_httpd_hnd_t handle,
			qse_mchar_t* buf, qse_size_t len);
		qse_ssize_t (*write) (
			qse_httpd_t* httpd, qse_httpd_hnd_t handle,
			const qse_mchar_t* buf, qse_size_t len);
	} file;

	struct
	{
		int (*stat) (
			qse_httpd_t* httpd, const qse_mchar_t* path, 
			qse_httpd_stat_t* stat);
		int (*make) (qse_httpd_t* httpd, const qse_mchar_t* path);
		int (*purge) (qse_httpd_t* httpd, const qse_mchar_t* path);

		int (*open) (
			qse_httpd_t* httpd, const qse_mchar_t* path, 
			qse_httpd_hnd_t* handle);
		void (*close) (qse_httpd_t* httpd, qse_httpd_hnd_t handle);
		int (*read) (
			qse_httpd_t* httpd, qse_httpd_hnd_t handle,
			qse_httpd_dirent_t* ent);
	} dir;

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
			qse_httpd_hnd_t handle, qse_foff_t* offset, qse_size_t count);

		/* event notification */
		int (*accepted) (
			qse_httpd_t* httpd,
			qse_httpd_client_t* client);  /* optional */
		void (*closed) (
			qse_httpd_t* httpd,
			qse_httpd_client_t* client);  /* optional */
	} client;

	struct
	{
		int (*open) (qse_httpd_t* httpd, qse_httpd_dns_t* dns);
		void (*close) (qse_httpd_t* httpd, qse_httpd_dns_t* dns);
		int (*recv) (qse_httpd_t* httpd, qse_httpd_dns_t* dns, qse_httpd_hnd_t handle);
		int (*send) (qse_httpd_t* httpd, qse_httpd_dns_t* dns,
		             const qse_mchar_t* name, qse_httpd_resolve_t resol,
		             const qse_httpd_dns_server_t* dns_server, void* ctx);

		qse_httpd_dns_preresolve_t preresolve;
	} dns;

	struct
	{
		qse_httpd_urs_open_t open;
		qse_httpd_urs_close_t close;
		qse_httpd_urs_recv_t recv;
		qse_httpd_urs_send_t send;
		qse_httpd_urs_prerewrite_t prerewrite;
	} urs;
};

/* -------------------------------------------------------------------------- */

typedef int (*qse_httpd_peekreq_t) (
	qse_httpd_t*        httpd,
	qse_httpd_client_t* client,
	qse_htre_t*         req
);

typedef int (*qse_httpd_pokereq_t) (
	qse_httpd_t*        httpd,
	qse_httpd_client_t* client,
	qse_htre_t*         req
);

typedef int (*qse_httpd_fmterr_t) (
	qse_httpd_t*        httpd,
	qse_httpd_client_t* client, 
	int                 code,
	qse_mchar_t*        buf,
	int                 bufsz
);

typedef void (*qse_httpd_impede_t) (
	qse_httpd_t* httpd
);

enum qse_httpd_act_code_t 
{
	QSE_HTTPD_CATCH_MERRMSG,
	QSE_HTTPD_CATCH_MWARNMSG,
	QSE_HTTPD_CATCH_MDBGMSG,
	QSE_HTTPD_ACCEPT_CLIENT,
	QSE_HTTPD_PURGE_CLIENT,
	QSE_HTTPD_READERR_CLIENT
};
typedef enum qse_httpd_act_code_t qse_httpd_act_code_t;

struct qse_httpd_act_t
{
	qse_httpd_act_code_t code;
	union
	{
		qse_httpd_client_t* client;
		qse_mchar_t merrmsg[128];
		qse_mchar_t mwarnmsg[128];
		qse_mchar_t mdbgmsg[128];
	} u;
};
typedef struct qse_httpd_act_t qse_httpd_act_t;

typedef void (*qse_httpd_logact_t) (
	qse_httpd_t*            httpd,
	const qse_httpd_act_t*  act
);

typedef struct qse_httpd_rcb_t qse_httpd_rcb_t;
struct qse_httpd_rcb_t
{
	qse_httpd_peekreq_t peekreq;
	qse_httpd_pokereq_t pokereq;
	qse_httpd_fmterr_t fmterr;
	qse_httpd_impede_t impede;
	qse_httpd_logact_t logact;
};

/* -------------------------------------------------------------------------- */

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

enum qse_httpd_task_trigger_flag_t
{
	QSE_HTTPD_TASK_TRIGGER_INACTIVE = (1 << 0)
};

enum qse_httpd_task_trigger_mask_t
{
	QSE_HTTPD_TASK_TRIGGER_READ      = (1 << 0),
	QSE_HTTPD_TASK_TRIGGER_WRITE     = (1 << 1),
	QSE_HTTPD_TASK_TRIGGER_READABLE  = (1 << 2),
	QSE_HTTPD_TASK_TRIGGER_WRITABLE  = (1 << 3)
};

#define QSE_HTTPD_TASK_TRIGGER_MAX 3

typedef struct qse_httpd_task_trigger_t qse_httpd_task_trigger_t;
struct qse_httpd_task_trigger_t
{
	int flags; /**< [IN] bitwise-ORed of #qse_httpd_task_trigger_flag_t enumerators*/
	int cmask; /* client mask - QSE_HTTPD_TASK_TRIGGER_READ | QSE_HTTPD_TASK_TRIGGER_WRITE */
	struct
	{
		int       mask; /* QSE_HTTPD_TASK_TRIGGER_READ | QSE_HTTPD_TASK_TRIGGER_WRITE */
		qse_httpd_hnd_t handle;
	} v[QSE_HTTPD_TASK_TRIGGER_MAX];
};

struct qse_httpd_task_t
{
	/* you must not call another entask functions from within 
	 * an initailizer. you can call entask functions from within 
	 * a finalizer and a main function. */
	qse_httpd_task_init_t    init;  /**< [IN] initializer */
	qse_httpd_task_fini_t    fini;  /**< [IN] finalizer */
	qse_httpd_task_main_t    main;  /**< [IN] main task function */
	qse_httpd_task_trigger_t trigger;
	void*                    ctx;   /**< [IN OUT] user-defined data */
};

enum qse_httpd_mate_type_t 
{
	QSE_HTTPD_SERVER,
	QSE_HTTPD_CLIENT,
	QSE_HTTPD_DNS,
	QSE_HTTPD_URS
};
typedef enum qse_httpd_mate_type_t  qse_httpd_mate_type_t;

/* it contains header fields common between 
 * qse_httpd_cleint_t and qse_httpd_server_t. */
#define QSE_HTTPD_MATE_HDR \
	qse_httpd_mate_type_t type

struct qse_httpd_mate_t
{
	/* == PRIVATE == */
	QSE_HTTPD_MATE_HDR;
};

struct qse_httpd_client_t
{
	/* == PRIVATE == */
	QSE_HTTPD_MATE_HDR;

	/* == PUBLIC  == */
	qse_httpd_hnd_t          handle;
	qse_httpd_hnd_t          handle2;
	qse_nwad_t               remote_addr;
	qse_nwad_t               local_addr;
	qse_nwad_t               orgdst_addr;
	qse_httpd_server_t*      server;
	int                      initial_ifindex;

	/* == PRIVATE == */
	qse_htrd_t*              htrd;
	int                      status;
	qse_httpd_task_trigger_t trigger;
	qse_ntime_t              last_active;

	qse_tmr_index_t          tmr_idle;

	qse_httpd_client_t*      prev;
	qse_httpd_client_t*      next;
 
	qse_httpd_client_t*      bad_next;

	struct
	{
		int count;
		qse_httpd_task_t* head;
		qse_httpd_task_t* tail;
	} task;
};

/* client->status */
#define QSE_HTTPD_CLIENT_BAD                    (1 << 0)
#define QSE_HTTPD_CLIENT_READY                  (1 << 1)
#define QSE_HTTPD_CLIENT_INTERCEPTED            (1 << 2)
#define QSE_HTTPD_CLIENT_SECURE                 (1 << 3)
#define QSE_HTTPD_CLIENT_PENDING                (1 << 4)
#define QSE_HTTPD_CLIENT_MUTE                   (1 << 5)
#define QSE_HTTPD_CLIENT_MUTE_DELETED           (1 << 6)
#define QSE_HTTPD_CLIENT_PROTOCOL_SWITCHED      (1 << 7) /* 101 Switching Protocols has been received */
#define QSE_HTTPD_CLIENT_HANDLE_READ_IN_MUX     (1 << 8)
#define QSE_HTTPD_CLIENT_HANDLE_WRITE_IN_MUX    (1 << 9)
#define QSE_HTTPD_CLIENT_HANDLE_RW_IN_MUX       (QSE_HTTPD_CLIENT_HANDLE_READ_IN_MUX | QSE_HTTPD_CLIENT_HANDLE_WRITE_IN_MUX)

/* 'i' must be between 0 and QSE_HTTPD_TASK_TRIGGER_MAX - 1.
 * Be careful about potential overflown when QSE_HTTPD_TASK_TRIGGER_MAX is too large. */
#define QSE_HTTPD_CLIENT_TASK_TRIGGER_READ_IN_MUX(i) (1 << ((i) + 10))
#define QSE_HTTPD_CLIENT_TASK_TRIGGER_WRITE_IN_MUX(i) (1 << ((i) + 10  + QSE_HTTPD_TASK_TRIGGER_MAX))
#define QSE_HTTPD_CLIENT_TASK_TRIGGER_RW_IN_MUX(i) (QSE_HTTPD_CLIENT_TASK_TRIGGER_READ_IN_MUX(i) | QSE_HTTPD_CLIENT_TASK_TRIGGER_WRITE_IN_MUX(i))

enum qse_httpd_server_flag_t
{
	QSE_HTTPD_SERVER_ACTIVE     = (1 << 0),
	QSE_HTTPD_SERVER_SECURE     = (1 << 1),
	QSE_HTTPD_SERVER_BINDTONWIF = (1 << 2)
};
typedef enum qse_httpd_server_flag_t qse_httpd_server_flag_t;

typedef void (*qse_httpd_server_detach_t) (
	qse_httpd_t*        httpd,
	qse_httpd_server_t* server
);

typedef struct qse_httpd_server_dope_t qse_httpd_server_dope_t;
struct qse_httpd_server_dope_t
{
	int          flags; /* bitwise-ORed of qse_httpd_server_flag_t */
	qse_nwad_t   nwad; /* binding address */
	unsigned int nwif; /* interface number to bind to */
	qse_httpd_server_detach_t detach; /* executed when the server is detached */
};

struct qse_httpd_server_t
{
	/* == PRIVATE == */
	QSE_HTTPD_MATE_HDR;

	/* provided by a user for attaching */
	qse_httpd_server_dope_t dope;

	/* set by server.open callback */
	qse_httpd_hnd_t  handle;

	/* private  */
	qse_httpd_t*          httpd;
	qse_httpd_server_t*   next;
	qse_httpd_server_t*   prev;
};


struct qse_httpd_dns_t
{
	/* == PRIVATE == */
	QSE_HTTPD_MATE_HDR;

	/* == PUBLIC == */
	qse_httpd_hnd_t handle[5];

	/* the number of effective slots in the handle array */
	int handle_count; 
	
	/* handle validity mask. for example, if handle[2] is valid, (1 << 2) must be set. */
	unsigned long handle_mask; 

	void* ctx;
};

struct qse_httpd_urs_t
{
	/* PRIVATE == */
	QSE_HTTPD_MATE_HDR;

	/* == PUBLIC == */

	/* urs.open() can set the followings */
	qse_httpd_hnd_t handle[5]; 
	int handle_count;
	unsigned long handle_mask;
	void* ctx;
};

/* -------------------------------------------------------------------------- */

/* ensure to define qse_httpd_fncptr_t to the same as 
 * qse_pio_fncptr_t in <qse/cmn/pio.h> */
typedef int (*qse_httpd_fncptr_t) (void* ctx, qse_env_char_t** envir);

typedef struct qse_httpd_fnc_t qse_httpd_fnc_t;
struct qse_httpd_fnc_t
{
	qse_httpd_fncptr_t ptr;
	void* ctx;
};

/* -------------------------------------------------------------------------- */

/**
 * The qse_httpd_rsrc_type_t defines the resource type than can 
 * be entasked with qse_httpd_entaskrsrc().
 */
enum qse_httpd_rsrc_type_t
{
	QSE_HTTPD_RSRC_AUTH,
	QSE_HTTPD_RSRC_CGI,
	QSE_HTTPD_RSRC_DIR,
	QSE_HTTPD_RSRC_ERROR,
	QSE_HTTPD_RSRC_FILE,
	QSE_HTTPD_RSRC_PROXY,
	QSE_HTTPD_RSRC_RELOC,
	QSE_HTTPD_RSRC_TEXT
};
typedef enum qse_httpd_rsrc_type_t qse_httpd_rsrc_type_t;

enum qse_httpd_rsrc_flag_t
{
	QSE_HTTPD_RSRC_100_CONTINUE = (1 << 0)
};
typedef enum qse_httpd_rsrc_flag_t qse_httpd_rsrc_flag_t;

enum qse_httpd_rsrc_cgi_flag_t
{
	/* non-parsed header */
	QSE_HTTPD_RSRC_CGI_NPH = (1 << 0), 

	/* 'path' points to qse_httpd_fncptr_t && 'shebang' points to its context */
	QSE_HTTPD_RSRC_CGI_FNC = (1 << 1)
};
typedef enum qse_httpd_rsrc_cgi_flag_t qse_httpd_rsrc_cgi_flag_t;


typedef struct qse_httpd_rsrc_cgi_t qse_httpd_rsrc_cgi_t;
struct qse_httpd_rsrc_cgi_t
{
	/* bitwised-ORed of #qse_httpd_rsrc_cgi_flag_t */
	int flags;

	/* script path resolved against file system */
	const qse_mchar_t* path; 

	/* script path as in qpath */
	const qse_mchar_t* script; 

	/* trailing part of qpath excluding the script path.
	 * for a qpath of /tmp/abc.cgi/a/b/c, if /tmp/abc.cgi is a script path,
	 * /a/b/c forms the suffix.*/
	const qse_mchar_t* suffix; 

	const qse_mchar_t* root;

	const qse_mchar_t* shebang; 
};

enum qse_httpd_rsrc_proxy_flag_t
{
	QSE_HTTPD_RSRC_PROXY_RAW             = (1 << 0), /* raw proxying. set this for CONNECT */
	QSE_HTTPD_RSRC_PROXY_TRANSPARENT     = (1 << 1),
	QSE_HTTPD_RSRC_PROXY_DST_STR         = (1 << 2), /* destination is an unresovled string pointed to by dst.str */
	QSE_HTTPD_RSRC_PROXY_ENABLE_DNS      = (1 << 3), /* dns service enabled (udp) */
	QSE_HTTPD_RSRC_PROXY_ENABLE_URS      = (1 << 4), /* url rewriting enabled (udp) */
	QSE_HTTPD_RSRC_PROXY_DNS_SERVER      = (1 << 5), /* dns address specified */
	QSE_HTTPD_RSRC_PROXY_URS_SERVER      = (1 << 6), /* urs address specified */
};
typedef enum qse_httpd_rsrc_proxy_flag_t qse_httpd_rsrc_proxy_flag_t;

typedef struct qse_httpd_rsrc_proxy_t qse_httpd_rsrc_proxy_t;
struct qse_httpd_rsrc_proxy_t
{
	int flags; /* bitwise-ORed of qse_httpd_rsrc_proxy_flag_t enumerators */

	union
	{
		qse_nwad_t nwad; 
	} src; /* local binding address */
	union
	{
		qse_nwad_t nwad;

		/* turn QSE_HTTPD_RSRC_PROXY_DST_STR on in flags and set the
		 * destination host name to the str field. if the bit is not set
		 * nwad field is used. */
		const qse_mchar_t* str; 
	} dst; /* remote destination address to connect to */

	qse_httpd_dns_server_t dns_server;
	qse_httpd_urs_server_t urs_server;
	qse_httpd_mod_t* dns_preresolve_mod;
	qse_httpd_mod_t* urs_prerewrite_mod;

	/* optional pseudonym to use for Via: */
	const qse_mchar_t* pseudonym;

	/* optional host name. it's preferred over the Host header in some 
	 * contexts. */
	const qse_mchar_t* host;
};

typedef struct qse_httpd_rsrc_dir_t qse_httpd_rsrc_dir_t;
struct qse_httpd_rsrc_dir_t
{
	const qse_mchar_t* path;
	const qse_mchar_t* head;
	const qse_mchar_t* foot;
};

enum qse_httpd_rsrc_reloc_flag_t
{
	QSE_HTTPD_RSRC_RELOC_PERMANENT = (1 << 0),
	QSE_HTTPD_RSRC_RELOC_KEEPMETHOD = (1 << 1),
	QSE_HTTPD_RSRC_RELOC_APPENDSLASH = (1 << 2)
};
typedef enum qse_httpd_rsrc_reloc_flag_t qse_httpd_rsrc_reloc_flag_t;

typedef struct qse_httpd_rsrc_reloc_t qse_httpd_rsrc_reloc_t;
struct qse_httpd_rsrc_reloc_t
{
	int flags;
	const qse_mchar_t* dst;
};

typedef struct qse_httpd_rsrc_t qse_httpd_rsrc_t;
struct qse_httpd_rsrc_t
{
	qse_httpd_rsrc_type_t type;
	int flags; /**< bitwised-ORed of #qse_httpd_rsrc_flag_t */
	union 
	{
		struct
		{
			const qse_mchar_t* realm;
		} auth;

		qse_httpd_rsrc_cgi_t cgi;
		qse_httpd_rsrc_dir_t dir;

		struct
		{
			int code;
		} error;

		struct
		{
			const qse_mchar_t* path;
			const qse_mchar_t* mime;
		} file;

		qse_httpd_rsrc_proxy_t proxy;

		qse_httpd_rsrc_reloc_t reloc;

		struct
		{
			const qse_mchar_t* ptr;
			const qse_mchar_t* mime;
		} text;
	} u;
};

/**
 * The qse_httpd_ecb_close_t type defines the callback function
 * called when an httpd object is closed.
 */
typedef void (*qse_httpd_ecb_close_t) (
	qse_httpd_t* httpd  /**< httpd */
);

/**
 * The qse_httpd_ecb_t type defines an event callback set.
 * You can register a callback function set with
 * qse_httpd_pushecb().  The callback functions in the registered
 * set are called in the reverse order of registration.
 */
typedef struct qse_httpd_ecb_t qse_httpd_ecb_t;
struct qse_httpd_ecb_t
{
	/**
	 * called by qse_httpd_close().
	 */
	qse_httpd_ecb_close_t close;

	/* internal use only. don't touch this field */
	qse_httpd_ecb_t* next;
};



#ifdef __cplusplus
extern "C" {
#endif

/**
 * The qse_httpd_open() function creates a httpd processor.
 */
QSE_EXPORT qse_httpd_t* qse_httpd_open (
	qse_mmgr_t* mmgr,   /**< memory manager */
	qse_size_t  xtnsize /**< extension size in bytes */
);

/**
 * The qse_httpd_close() function destroys a httpd processor.
 */
QSE_EXPORT void qse_httpd_close (
	qse_httpd_t* httpd 
);

QSE_EXPORT qse_mmgr_t* qse_httpd_getmmgr (
	qse_httpd_t* httpd
); 

QSE_EXPORT void* qse_httpd_getxtn (
	qse_httpd_t* httpd
);

QSE_EXPORT qse_httpd_errnum_t qse_httpd_geterrnum (
	qse_httpd_t* httpd
);

QSE_EXPORT void qse_httpd_seterrnum (
	qse_httpd_t*       httpd,
	qse_httpd_errnum_t errnum
);

QSE_EXPORT int qse_httpd_getopt (
	qse_httpd_t*    httpd,
	qse_httpd_opt_t id,
	void*           value
);

QSE_EXPORT int qse_httpd_setopt (
	qse_httpd_t*    httpd,
	qse_httpd_opt_t id,
	const void*     value
);

/**
 * The qse_httpd_popecb() function pops an httpd event callback set
 * and returns the pointer to it. If no callback set can be popped,
 * it returns #QSE_NULL.
 */
QSE_EXPORT qse_httpd_ecb_t* qse_httpd_popecb (
	qse_httpd_t* httpd /**< httpd */
);

/**
 * The qse_httpd_pushecb() function register a runtime callback set.
 */
QSE_EXPORT void qse_httpd_pushecb (
	qse_httpd_t*     httpd, /**< httpd */
	qse_httpd_ecb_t* ecb  /**< callback set */
);

/**
 * The qse_httpd_loop() function starts a httpd server loop.
 */
QSE_EXPORT int qse_httpd_loop (
	qse_httpd_t* httpd
);

/**
 * The qse_httpd_stop() function requests to stop qse_httpd_loop()
 */
QSE_EXPORT void qse_httpd_stop (
	qse_httpd_t* httpd
);

QSE_EXPORT void qse_httpd_impede (
	qse_httpd_t* httpd
);

#define qse_httpd_getserverxtn(httpd,server) ((void*)(server+1))

QSE_EXPORT qse_httpd_server_t* qse_httpd_attachserver (
	qse_httpd_t*                   httpd,
	const qse_httpd_server_dope_t* dope,
	qse_size_t                     xtnsize
);

QSE_EXPORT void qse_httpd_detachserver (
	qse_httpd_t*        httpd,
	qse_httpd_server_t* server
);

QSE_EXPORT qse_httpd_server_t* qse_httpd_getfirstserver (
	qse_httpd_t* httpd
);

QSE_EXPORT qse_httpd_server_t* qse_httpd_getlastserver (
	qse_httpd_t* httpd
);

QSE_EXPORT qse_httpd_server_t* qse_httpd_getnextserver (
	qse_httpd_t*        httpd,
	qse_httpd_server_t* server
);

QSE_EXPORT qse_httpd_server_t* qse_httpd_getprevserver (
	qse_httpd_t*        httpd, 
	qse_httpd_server_t* server
);

QSE_EXPORT void qse_httpd_discardcontent (
	qse_httpd_t*        httpd,
	qse_htre_t*         req
);

QSE_EXPORT void qse_httpd_completecontent (
	qse_httpd_t*        httpd,
	qse_htre_t*         req
);


/**
 * The qse_httpd_setname() function changes the string
 * to be used as the value for the server header. 
 */
QSE_EXPORT void qse_httpd_setname (
	qse_httpd_t*       httpd,
	const qse_mchar_t* name
);


/**
 * The qse_httpd_getname() function returns the
 * pointer to the string used as the value for the server
 * header.
 */
QSE_EXPORT const qse_mchar_t* qse_httpd_getname (
	qse_httpd_t* httpd
);

/**
 * The qse_httpd_fmtgmtimetobb() function converts a numeric time @a nt
 * to a string and stores it in a built-in buffer. 
 * If @a nt is QSE_NULL, the current time is used.
 */
QSE_EXPORT const qse_mchar_t* qse_httpd_fmtgmtimetobb (
	qse_httpd_t*       httpd,
	const qse_ntime_t* nt,
	int                idx
);

QSE_EXPORT void* qse_httpd_gettaskxtn (
	qse_httpd_t*            httpd,
	qse_httpd_task_t*       task
);

QSE_EXPORT qse_httpd_task_t* qse_httpd_entask (
	qse_httpd_t*            httpd,
	qse_httpd_client_t*     client,
	qse_httpd_task_t*       pred,
	const qse_httpd_task_t* task,
	qse_size_t              xtnsize
);

/* -------------------------------------------- */

QSE_EXPORT qse_httpd_task_t* qse_httpd_entaskdisconnect (
	qse_httpd_t*            httpd,
	qse_httpd_client_t*     client,
	qse_httpd_task_t*       pred
);


QSE_EXPORT qse_httpd_task_t* qse_httpd_entaskformat (
	qse_httpd_t*            httpd,
	qse_httpd_client_t*     client,
	qse_httpd_task_t*       pred,
	const qse_mchar_t*      fmt,
	...
);

/* -------------------------------------------- */

QSE_EXPORT qse_httpd_task_t* qse_httpd_entasktext (
	qse_httpd_t*            httpd,
	qse_httpd_client_t*     client,
	qse_httpd_task_t*       pred,
	const qse_mchar_t*      text,
	const qse_mchar_t*      mime,
	qse_htre_t*             req
);

QSE_EXPORT qse_httpd_task_t* qse_httpd_entaskerr (
	qse_httpd_t*              httpd,
	qse_httpd_client_t*       client,
	qse_httpd_task_t*         pred,
	int                       code, 
	qse_htre_t*               req
);

QSE_EXPORT qse_httpd_task_t* qse_httpd_entaskcontinue (
	qse_httpd_t*              httpd,
	qse_httpd_client_t*       client,
	qse_httpd_task_t*         pred,
	qse_htre_t*               req
);

/**
 * The qse_httpd_entaskauth() function adds a basic authorization task.
 */
QSE_EXPORT qse_httpd_task_t* qse_httpd_entaskauth (
	qse_httpd_t*              httpd,
	qse_httpd_client_t*       client,
	qse_httpd_task_t*         pred,
	const qse_mchar_t*        realm,
	qse_htre_t*               req
);

QSE_EXPORT qse_httpd_task_t* qse_httpd_entaskreloc (
	qse_httpd_t*                  httpd,
	qse_httpd_client_t*           client,
	qse_httpd_task_t*             pred,
	const qse_httpd_rsrc_reloc_t* reloc,
	qse_htre_t*                   req
);

QSE_EXPORT qse_httpd_task_t* qse_httpd_entasknomod (
	qse_httpd_t*              httpd,
	qse_httpd_client_t*       client,
	qse_httpd_task_t*         pred,
	qse_htre_t*               req
);

QSE_EXPORT qse_httpd_task_t* qse_httpd_entaskallow (
	qse_httpd_t*              httpd,
	qse_httpd_client_t*       client,
	qse_httpd_task_t*         pred,
	const qse_mchar_t*        allow,
	qse_htre_t*               req
);

QSE_EXPORT qse_httpd_task_t* qse_httpd_entaskdir (
	qse_httpd_t*              httpd,
	qse_httpd_client_t*       client,
	qse_httpd_task_t*         pred,
	qse_httpd_rsrc_dir_t*     dir,
	qse_htre_t*               req
);

QSE_EXPORT qse_httpd_task_t* qse_httpd_entaskfile (
	qse_httpd_t*              httpd,
	qse_httpd_client_t*       client,
	qse_httpd_task_t*         pred,
	const qse_mchar_t*        name,
	const qse_mchar_t*        mime,
	qse_htre_t*               req
);

QSE_EXPORT qse_httpd_task_t* qse_httpd_entaskcgi (
	qse_httpd_t*                    httpd,
	qse_httpd_client_t*             client,
	qse_httpd_task_t*               pred,
	const qse_httpd_rsrc_cgi_t*     cgi,
	qse_htre_t*                     req
);

QSE_EXPORT qse_httpd_task_t* qse_httpd_entaskproxy (
	qse_httpd_t*                  httpd,
	qse_httpd_client_t*           client,
	qse_httpd_task_t*             pred,
	const qse_httpd_rsrc_proxy_t* proxy,
	qse_htre_t*                   req
);

/* -------------------------------------------- */

QSE_EXPORT qse_httpd_task_t* qse_httpd_entaskrsrc (
	qse_httpd_t*            httpd,
	qse_httpd_client_t*     client,
	qse_httpd_task_t*       pred,
	qse_httpd_rsrc_t*       rsrc,
	qse_htre_t*             req
);

/* -------------------------------------------- */

QSE_EXPORT void* qse_httpd_allocmem (
	qse_httpd_t* httpd, 
	qse_size_t   size
);

QSE_EXPORT void* qse_httpd_callocmem (
	qse_httpd_t* httpd, 
	qse_size_t   size
);

QSE_EXPORT void* qse_httpd_reallocmem (
	qse_httpd_t* httpd,
	void*        ptr,
	qse_size_t   size
);

QSE_EXPORT void qse_httpd_freemem (
	qse_httpd_t* httpd,
	void*        ptr
);

QSE_EXPORT qse_mchar_t* qse_httpd_strtombsdup (
	qse_httpd_t* httpd, 
	const qse_char_t*  str
);

QSE_EXPORT qse_mchar_t* qse_httpd_strntombsdup (
	qse_httpd_t*       httpd, 
	const qse_char_t*  str,
	qse_size_t         len
);

QSE_EXPORT qse_mchar_t* qse_httpd_escapehtml (
	qse_httpd_t*        httpd, 
	const qse_mchar_t*  str
);

QSE_EXPORT int qse_httpd_resolvename (
	qse_httpd_t*                  httpd,
	const qse_mchar_t*            name,
	qse_httpd_resolve_t           resol,
	const qse_httpd_dns_server_t* dns_server,
	void*                         ctx
);

QSE_EXPORT int qse_httpd_rewriteurl (
	qse_httpd_t*                  httpd,
	const qse_mchar_t*            url,
	qse_httpd_rewrite_t           rewrite,
	const qse_httpd_urs_server_t* urs_server,
	void*                         ctx
);

QSE_EXPORT int qse_httpd_loadmod (
	qse_httpd_t*      httpd,
	const qse_char_t* name
);

QSE_EXPORT qse_httpd_mod_t* qse_httpd_findmod (
	qse_httpd_t*      httpd,
	const qse_char_t* name
);

#ifdef __cplusplus
}
#endif

#endif
