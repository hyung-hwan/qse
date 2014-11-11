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
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _QSE_LIB_HTTP_HTTPD_H_
#define _QSE_LIB_HTTP_HTTPD_H_

/* private header file for httpd */

#include <qse/http/httpd.h>

#if defined(NDEBUG)
	/* To include debugging messages while NDEBUG is set,
	 * you can define QSE_HTTPD_DEBUG externally in CFLAGS or something.
	 */
#else
	/* debugging mode */
#	define QSE_HTTPD_DEBUG 1
#endif

#if defined(QSE_HTTPD_DEBUG)
#	include <qse/cmn/sio.h>
#	include <qse/cmn/path.h>
#	define HTTPD_DBGOUT0(fmt) qse_putmbsf("%06d %-20hs " fmt, (int)__LINE__, qse_mbsbasename(__FILE__))
#	define HTTPD_DBGOUT1(fmt,a1) qse_putmbsf("%06d %-20hs " fmt, (int)__LINE__, qse_mbsbasename(__FILE__), (a1))
#	define HTTPD_DBGOUT2(fmt,a1,a2) qse_putmbsf("%06d %-20hs " fmt, (int)__LINE__, qse_mbsbasename(__FILE__), (a1), (a2))
#	define HTTPD_DBGOUT3(fmt,a1,a2,a3) qse_putmbsf("%06d %-20hs " fmt, (int)__LINE__, qse_mbsbasename(__FILE__), (a1), (a2), (a3))
#	define HTTPD_DBGOUT4(fmt,a1,a2,a3,a4) qse_putmbsf("%06d %-20hs " fmt, (int)__LINE__, qse_mbsbasename(__FILE__), (a1), (a2), (a3), (a4))
#	define HTTPD_DBGOUT5(fmt,a1,a2,a3,a4,a5) qse_putmbsf("%06d %-20hs " fmt, (int)__LINE__, qse_mbsbasename(__FILE__), (a1), (a2), (a3), (a4), (a5))
#else
#	define HTTPD_DBGOUT0(fmt)
#	define HTTPD_DBGOUT1(fmt,a1)
#	define HTTPD_DBGOUT2(fmt,a1,a2)
#	define HTTPD_DBGOUT3(fmt,a1,a2,a3)
#	define HTTPD_DBGOUT4(fmt,a1,a2,a3,a4)
#	define HTTPD_DBGOUT5(fmt,a1,a2,a3,a4,a5)
#endif

#define QSE_HTTPD_DEFAULT_PORT        80
#define QSE_HTTPD_DEFAULT_SECURE_PORT 443


struct qse_httpd_t
{
	qse_mmgr_t* mmgr;
	qse_httpd_errnum_t errnum;
	qse_httpd_ecb_t* ecb; /* event callbacks */
	qse_tmr_t* tmr;

	struct
	{
		int trait;
		qse_ntime_t tmout; /* poll timeout */
		qse_ntime_t idle_limit; 
		qse_cstr_t mod[2]; /* module prefix and postfix */
		qse_httpd_scb_t scb; /* system callbacks */
		qse_httpd_rcb_t rcb; /* request callbacks */
	} opt;

	int stopreq: 1;
	int impedereq: 1;
	int dnsactive: 1;
	int ursactive: 1;

	qse_mchar_t sname[128]; /* server name for the server header */
	qse_mchar_t gtbuf[10][64]; /* GMT time buffers */

	qse_httpd_mod_t* modlist;

	struct
	{
		struct
		{
			qse_httpd_client_t* head;
			qse_httpd_client_t* tail;
			qse_size_t count;
		} list;

		struct
		{
			qse_httpd_client_t* head;
			qse_httpd_client_t* tail;
		} tasked;

		qse_httpd_client_t* bad;
	} client;

	struct
	{
		struct
		{
			qse_httpd_server_t* head;
			qse_httpd_server_t* tail;
		} list;
		qse_size_t          navail;
		qse_size_t          nactive;
	} server;

	void* mux;
	qse_httpd_dns_t dns;
	qse_httpd_urs_t urs;
};

/* qse_httpd_real_task_t is a private type to hide some private fields
 * from being exposed by qse_httpd_task_t. 
 */
typedef struct qse_httpd_real_task_t qse_httpd_real_task_t;

struct qse_httpd_real_task_t
{
	qse_httpd_task_t core;
	qse_httpd_real_task_t* prev;
	qse_httpd_real_task_t* next;
};

#define MAX_SEND_SIZE (4096 * 4)
#define MAX_RECV_SIZE (4096 * 2)

#define MAX_NWAD_TEXT_SIZE 96

#ifdef __cplusplus
extern "C" {
#endif

extern qse_http_version_t qse_http_v11;

int qse_httpd_init (
	qse_httpd_t* httpd,
	qse_mmgr_t*  mmgr
);

void qse_httpd_fini (
	qse_httpd_t* httpd
);

qse_httpd_task_t* qse_httpd_entask_status (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client, 
	qse_httpd_task_t* pred,
	int code,
	void* extra,
	qse_http_method_t method,
	const qse_http_version_t* version, 
	int keepalive
);

qse_httpd_task_t* qse_httpd_entask_nomod (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client,
	qse_httpd_task_t* pred,
	qse_http_method_t method,
	const qse_http_version_t* version,
	int keepalive
);

int qse_httpd_activatetasktrigger (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client,
	qse_httpd_task_t* task
);

int qse_httpd_inactivatetasktrigger (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client,
	qse_httpd_task_t* task
);

int qse_httpd_insert_timer_event (
	qse_httpd_t*           httpd,
	const qse_tmr_event_t* event,
	qse_tmr_index_t*       index
);

void qse_httpd_remove_timer_event (
	qse_httpd_t*     httpd,
	qse_tmr_index_t  index
);

qse_httpd_peer_t* qse_httpd_cacheproxypeer (
	qse_httpd_t*        httpd,
	qse_httpd_client_t* client,
	qse_httpd_peer_t*   tmpl
);

qse_httpd_peer_t* qse_httpd_decacheproxypeer (
	qse_httpd_t*        httpd,
	qse_httpd_client_t* client, 
	const qse_nwad_t*   nwad,
	const qse_nwad_t*   local,
	int                 secure
);

#ifdef __cplusplus
}
#endif


#endif
