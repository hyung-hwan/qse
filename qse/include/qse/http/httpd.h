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

typedef struct qse_httpd_t        qse_httpd_t;
typedef struct qse_httpd_mate_t   qse_httpd_mate_t;
typedef struct qse_httpd_server_t qse_httpd_server_t;
typedef struct qse_httpd_client_t qse_httpd_client_t;
typedef struct qse_httpd_dns_t    qse_httpd_dns_t;

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
	QSE_HTTPD_ENODNS,  /* dns service not activated */
	QSE_HTTPD_ETASK
};
typedef enum qse_httpd_errnum_t qse_httpd_errnum_t;

enum qse_httpd_opt_t
{
	QSE_HTTPD_TRAIT,
	QSE_HTTPD_SCB,
	QSE_HTTPD_RCB,
	QSE_HTTPD_TMOUT,
	QSE_HTTPD_IDLELIMIT
};
typedef enum qse_httpd_opt_t qse_httpd_opt_t;

enum qse_httpd_trait_t
{
	QSE_HTTPD_MUTECLIENT   = (1 << 0),
	QSE_HTTPD_CGIERRTONUL  = (1 << 1),
	QSE_HTTPD_CGINOCLOEXEC = (1 << 2),
	QSE_HTTPD_CGINOCHUNKED = (1 << 3),
	QSE_HTTPD_PROXYNOVIA   = (1 << 4),
	QSE_HTTPD_LOGACT       = (1 << 5),
	QSE_HTTPD_DNSNOA       = (1 << 6),
	QSE_HTTPD_DNSNOAAAA    = (1 << 7),
};
typedef enum qse_httpd_trait_t qse_httpd_trait_t;

typedef struct qse_httpd_stat_t qse_httpd_stat_t;
struct qse_httpd_stat_t
{
	int        isdir;
	qse_long_t dev;
	qse_long_t ino;
	qse_foff_t size;
	qse_ntime_t mtime;
};

typedef struct qse_httpd_peer_t qse_httpd_peer_t;
struct qse_httpd_peer_t
{
	qse_nwad_t nwad;
	qse_nwad_t local; /* local side address facing the peer */
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

typedef struct qse_httpd_dirent_t qse_httpd_dirent_t;

struct qse_httpd_dirent_t
{
	const qse_mchar_t* name;
	qse_httpd_stat_t   stat;
};

typedef void (*qse_httpd_resol_t) (
	qse_httpd_t*       httpd,
	const qse_mchar_t* name,
	const qse_nwad_t*  nwad,
	void*              ctx
);

typedef struct qse_httpd_scb_t qse_httpd_scb_t;
struct qse_httpd_scb_t
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
		void* (*open)   (qse_httpd_t* httpd, qse_httpd_muxcb_t muxcb);
		void  (*close)  (qse_httpd_t* httpd, void* mux);
		int   (*addhnd) (qse_httpd_t* httpd, void* mux, qse_ubi_t handle, int mask, void* cbarg);
		int   (*delhnd) (qse_httpd_t* httpd, void* mux, qse_ubi_t handle);
		int   (*poll)   (qse_httpd_t* httpd, void* mux, const qse_ntime_t* tmout);

		int (*readable) (
			qse_httpd_t* httpd, qse_ubi_t handle, const qse_ntime_t* tmout);
		int (*writable) (
			qse_httpd_t* httpd, qse_ubi_t handle, const qse_ntime_t* tmout);
	} mux;

	struct
	{
		int (*stat) (
			qse_httpd_t* httpd, const qse_mchar_t* path, 
			qse_httpd_stat_t* stat);
		int (*purge) (qse_httpd_t* httpd, const qse_mchar_t* path);
			
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
		int (*stat) (
			qse_httpd_t* httpd, const qse_mchar_t* path, 
			qse_httpd_stat_t* stat);
		int (*make) (qse_httpd_t* httpd, const qse_mchar_t* path);
		int (*purge) (qse_httpd_t* httpd, const qse_mchar_t* path);

		int (*open) (
			qse_httpd_t* httpd, const qse_mchar_t* path, 
			qse_ubi_t* handle);
		void (*close) (qse_httpd_t* httpd, qse_ubi_t handle);
		int (*read) (
			qse_httpd_t* httpd, qse_ubi_t handle,
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
			qse_ubi_t handle, qse_foff_t* offset, qse_size_t count);

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
		int (*recv) (qse_httpd_t* httpd, qse_httpd_dns_t* dns);
		int (*send) (qse_httpd_t* httpd, qse_httpd_dns_t* dns, const qse_mchar_t* name, qse_httpd_resol_t resol, void* ctx);
	} dns;
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
		qse_ubi_t handle;
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
	QSE_HTTPD_DNS
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
	qse_ubi_t                handle;
	qse_ubi_t                handle2;
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

	qse_size_t               tmr_idle;
	qse_size_t               tmr_dns;

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

enum qse_httpd_server_flag_t
{
	QSE_HTTPD_SERVER_ACTIVE     = (1 << 0),
	QSE_HTTPD_SERVER_SECURE     = (1 << 1),
	QSE_HTTPD_SERVER_BINDTONWIF = (1 << 2)
};

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
	qse_ubi_t  handle;

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
	qse_ubi_t handle;
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
	QSE_HTTPD_RSRC_ERR,
	QSE_HTTPD_RSRC_FILE,
	QSE_HTTPD_RSRC_PROXY,
	QSE_HTTPD_RSRC_RELOC,
	QSE_HTTPD_RSRC_REDIR,
	QSE_HTTPD_RSRC_TEXT
};
typedef enum qse_httpd_rsrc_type_t qse_httpd_rsrc_type_t;

enum qse_httpd_rsrc_flag_t
{
	QSE_HTTPD_RSRC_100_CONTINUE = (1 << 0)
};

typedef struct qse_httpd_rsrc_cgi_t qse_httpd_rsrc_cgi_t;
struct qse_httpd_rsrc_cgi_t
{
	const qse_mchar_t* path;
	const qse_mchar_t* script;
	const qse_mchar_t* suffix;
	const qse_mchar_t* root;
	const qse_mchar_t* shebang; 
	int nph;
};

enum qse_httpd_rsrc_proxy_flag_t
{
	QSE_HTTPD_RSRC_PROXY_RAW     = (1 << 0),
	QSE_HTTPD_RSRC_PROXY_DST_STR = (1 << 1)
};

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
		const qse_mchar_t* str;
	} dst; /* remote destination address to connect to */

	const qse_mchar_t* pseudonym; /* pseudonym to use in Via: */
};

typedef struct qse_httpd_rsrc_dir_t qse_httpd_rsrc_dir_t;
struct qse_httpd_rsrc_dir_t
{
	const qse_mchar_t* path;
	const qse_mchar_t* head;
	const qse_mchar_t* foot;
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
		} err;

		struct
		{
			const qse_mchar_t* path;
			const qse_mchar_t* mime;
		} file;

		qse_httpd_rsrc_proxy_t proxy;

		struct
		{
			const qse_mchar_t* dst;
		} reloc;

		struct
		{
			const qse_mchar_t* dst;
		} redir;

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
	qse_httpd_t*              httpd,
	qse_httpd_client_t*       client,
	qse_httpd_task_t*         pred,
	const qse_mchar_t*        dst,
	qse_htre_t*               req
);

QSE_EXPORT qse_httpd_task_t* qse_httpd_entaskredir (
	qse_httpd_t*              httpd,
	qse_httpd_client_t*       client,
	qse_httpd_task_t*         pred,
	const qse_mchar_t*        dst,
	qse_htre_t*               req
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


QSE_EXPORT int qse_httpd_resolname (
	qse_httpd_t*       httpd,
	const qse_mchar_t* name,
	qse_httpd_resol_t  resol,
	void*              ctx
);


#ifdef __cplusplus
}
#endif

#endif
