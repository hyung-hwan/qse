/*
 * $Id$
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

#ifndef _QSE_LIB_NET_HTTPD_H_
#define _QSE_LIB_NET_HTTPD_H_

/* private header file for httpd */

#include <qse/net/httpd.h>
#include <qse/net/htrd.h>

#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

typedef struct client_array_t client_array_t;

union sockaddr_t
{
	struct sockaddr_in in4;
	struct sockaddr_in6 in6;
};

typedef union sockaddr_t sockaddr_t;

typedef struct task_queue_node_t task_queue_node_t;
struct task_queue_node_t
{
	task_queue_node_t* next;
	task_queue_node_t* prev;
	qse_httpd_task_t task;
};

struct qse_httpd_client_t
{
	qse_ubi_t               handle;
	int                     bad;
	sockaddr_t              addr;
	qse_htrd_t*             htrd;

	struct
	{
		pthread_mutex_t  mutex;
		struct
		{
			int count;
			task_queue_node_t* head;
			task_queue_node_t* tail;
		} queue;
	} task;
};

struct client_array_t
{
	int                 capa;
	int                 size;
	qse_httpd_client_t* data;
};

typedef struct listener_t listener_t;
struct listener_t
{
	int family;/* AF_INET, AF_INET6 */
	int secure;
	
	qse_char_t* host;
	union
	{
		struct in_addr in4;
		struct in6_addr in6;
	} addr;

	int port;
	int handle;
	listener_t* next;
};

struct qse_httpd_t
{
	QSE_DEFINE_COMMON_FIELDS (httpd)
	qse_httpd_errnum_t errnum;
	qse_httpd_cbs_t* cbs;

	int stopreq;

	struct
	{
		pthread_mutex_t mutex;
		pthread_cond_t  cond;
		client_array_t array;
	} client;

	struct
	{
		pthread_mutex_t mutex;
		listener_t*     list;
		fd_set          set;
		int             max;
	} listener;
};

#ifdef __cplusplus
extern "C" {
#endif

qse_httpd_t* qse_httpd_init (
	qse_httpd_t* httpd,
	qse_mmgr_t*  mmgr
);

void qse_httpd_fini (
	qse_httpd_t* httpd
);

#ifdef __cplusplus
}
#endif


#endif
