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

#ifndef _QSE_LIB_NET_HTTPD_H_
#define _QSE_LIB_NET_HTTPD_H_

/* private header file for httpd */

#include <qse/net/httpd.h>

struct qse_httpd_t
{
	QSE_DEFINE_COMMON_FIELDS (httpd)
	qse_httpd_errnum_t errnum;
	qse_httpd_cbs_t* cbs;

	int option;
	int stopreq;

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
		qse_httpd_server_t* list;
		qse_size_t          navail;
		qse_size_t          nactive;
	} server;

	void* mux;
};


#define MAX_SEND_SIZE 4096

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

qse_httpd_task_t* qse_httpd_entask_error (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client,
     qse_httpd_task_t* pred,
	int code,
     const qse_http_version_t* version,
	int keepalive
);

#ifdef __cplusplus
}
#endif


#endif
