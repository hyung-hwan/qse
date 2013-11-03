/*
 * $Id$
 * 
    Copyright 2006-2012 Chung, Hyung-Hwan.
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

#include <qse/cmn/stdio.h> /* TODO: remove this.. only for debugging at this moment */

struct qse_httpd_t
{
	qse_mmgr_t* mmgr;
	qse_httpd_errnum_t errnum;
	qse_httpd_ecb_t* ecb; /* event callbacks */


	struct
	{
		int trait;
		qse_httpd_scb_t scb; /* system callbacks */
		qse_httpd_rcb_t rcb; /* request callbacks */
		qse_ntime_t tmout; /* poll timeout */
		qse_ntime_t idle_limit; 
	} opt;

	int stopreq: 1;
	int impedereq: 1;

	qse_mchar_t sname[128]; /* server name for the server header */
	qse_mchar_t gtbuf[10][64]; /* GMT time buffers */

	struct
	{
		struct
		{
			qse_httpd_client_t* head;
			qse_httpd_client_t* tail;
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

#define MAX_SEND_SIZE 4096


/* client->status */
#define CLIENT_BAD                    (1 << 0)
#define CLIENT_READY                  (1 << 1)
#define CLIENT_SECURE                 (1 << 2)
#define CLIENT_PENDING                (1 << 3)
#define CLIENT_MUTE                   (1 << 4)
#define CLIENT_MUTE_DELETED           (1 << 5)
#define CLIENT_HANDLE_READ_IN_MUX     (1 << 6)
#define CLIENT_HANDLE_WRITE_IN_MUX    (1 << 7)
#define CLIENT_HANDLE_IN_MUX          (CLIENT_HANDLE_READ_IN_MUX|CLIENT_HANDLE_WRITE_IN_MUX)
#define CLIENT_TASK_TRIGGER_IN_MUX(i) (1 << ((i) + 8))

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

qse_httpd_task_t* qse_httpd_entask_err (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client,
	qse_httpd_task_t* pred,
	int code,
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

#ifdef __cplusplus
}
#endif


#endif
