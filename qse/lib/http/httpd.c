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

#include "httpd.h"
#include "../cmn/mem.h"
#include <qse/cmn/chr.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/sio.h>

typedef struct htrd_xtn_t htrd_xtn_t;

struct htrd_xtn_t
{
	qse_httpd_t*        httpd;
	qse_httpd_client_t* client;
};

static void free_server_list (qse_httpd_t* httpd);
static int perform_client_task (
	qse_httpd_t* httpd, void* mux, qse_ubi_t handle, int mask, void* cbarg);

qse_http_version_t qse_http_v11 = { 1, 1 };

qse_httpd_t* qse_httpd_open (qse_mmgr_t* mmgr, qse_size_t xtnsize)
{
	qse_httpd_t* httpd;

	httpd = (qse_httpd_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_httpd_t) + xtnsize);
	if (httpd)
	{

		if (qse_httpd_init (httpd, mmgr) <= -1)
		{
			QSE_MMGR_FREE (mmgr, httpd);
			return QSE_NULL;
		}
		else QSE_MEMSET (QSE_XTN(httpd), 0, xtnsize);
	}

	return httpd;
}

void qse_httpd_close (qse_httpd_t* httpd)
{
	qse_httpd_ecb_t* ecb;

	for (ecb = httpd->ecb; ecb; ecb = ecb->next)
		if (ecb->close) ecb->close (httpd);

	qse_httpd_fini (httpd);
	QSE_MMGR_FREE (httpd->mmgr, httpd);
}

int qse_httpd_init (qse_httpd_t* httpd, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (httpd, 0, QSE_SIZEOF(*httpd));

	httpd->mmgr = mmgr;

	qse_mbscpy (httpd->sname, QSE_MT("QSE-HTTPD " QSE_PACKAGE_VERSION));

	httpd->opt.tmout.sec = 3;
	httpd->opt.idle_limit.sec = 30;

	return 0;
}

void qse_httpd_fini (qse_httpd_t* httpd)
{
	free_server_list (httpd);
}

void qse_httpd_stop (qse_httpd_t* httpd)
{
	httpd->stopreq = 1;
}

void qse_httpd_impede (qse_httpd_t* httpd)
{
	httpd->impedereq = 1;
}

qse_httpd_errnum_t qse_httpd_geterrnum (qse_httpd_t* httpd)
{
	return httpd->errnum;
}

void qse_httpd_seterrnum (qse_httpd_t* httpd, qse_httpd_errnum_t errnum)
{
	httpd->errnum = errnum;
}

qse_mmgr_t* qse_httpd_getmmgr (qse_httpd_t* httpd)
{
	return httpd->mmgr;
}

void* qse_httpd_getxtn (qse_httpd_t* httpd)
{
	return QSE_XTN (httpd);
}

int qse_httpd_getopt (qse_httpd_t* httpd, qse_httpd_opt_t id, void* value)
{
	switch (id)
	{
		case QSE_HTTPD_TRAIT:
			*(int*)value = httpd->opt.trait;
			return 0;

		case QSE_HTTPD_SCB:
			*(qse_httpd_scb_t*)value = httpd->opt.scb;
			return 0;

		case QSE_HTTPD_RCB:
			*(qse_httpd_rcb_t*)value = httpd->opt.rcb;
			return 0;

		case QSE_HTTPD_TMOUT:
			*(qse_ntime_t*)value = httpd->opt.tmout;
			return 0;

		case QSE_HTTPD_IDLELIMIT:
			*(qse_ntime_t*)value = httpd->opt.idle_limit;
			return 0;
	}

	httpd->errnum = QSE_HTTPD_EINVAL;
	return -1;
}

int qse_httpd_setopt (qse_httpd_t* httpd, qse_httpd_opt_t id, const void* value)
{
	switch (id)
	{
		case QSE_HTTPD_TRAIT:
			httpd->opt.trait = *(const int*)value;
			return 0;

		case QSE_HTTPD_SCB:
			httpd->opt.scb = *(qse_httpd_scb_t*)value;
			return 0;

		case QSE_HTTPD_RCB:
			httpd->opt.rcb = *(qse_httpd_rcb_t*)value;
			return 0;

		case QSE_HTTPD_TMOUT:
			httpd->opt.tmout = *(qse_ntime_t*)value;
			return 0;

		case QSE_HTTPD_IDLELIMIT:
			httpd->opt.idle_limit = *(qse_ntime_t*)value;
			return 0;
	}

	httpd->errnum = QSE_HTTPD_EINVAL;
	return -1;
}

/* ----------------------------------------------------------------------- */

qse_httpd_ecb_t* qse_httpd_popecb (qse_httpd_t* httpd)
{
	qse_httpd_ecb_t* top = httpd->ecb;
	if (top) httpd->ecb = top->next;
	return top;
}

void qse_httpd_pushecb (qse_httpd_t* httpd, qse_httpd_ecb_t* ecb)
{
	ecb->next = httpd->ecb;
	httpd->ecb = ecb;
}

/* ----------------------------------------------------------------------- */

QSE_INLINE void* qse_httpd_allocmem (qse_httpd_t* httpd, qse_size_t size)
{
	void* ptr = QSE_MMGR_ALLOC (httpd->mmgr, size);
	if (ptr == QSE_NULL) httpd->errnum = QSE_HTTPD_ENOMEM;
	return ptr;
}

QSE_INLINE void* qse_httpd_callocmem (qse_httpd_t* httpd, qse_size_t size)
{
	void* ptr = QSE_MMGR_ALLOC (httpd->mmgr, size);
	if (ptr == QSE_NULL) httpd->errnum = QSE_HTTPD_ENOMEM;
	else QSE_MEMSET (ptr, 0, size);
	return ptr;
}

QSE_INLINE void* qse_httpd_reallocmem (
	qse_httpd_t* httpd, void* ptr, qse_size_t size)
{
	void* nptr = QSE_MMGR_REALLOC (httpd->mmgr, ptr, size);
	if (nptr == QSE_NULL) httpd->errnum = QSE_HTTPD_ENOMEM;
	return nptr;
}

QSE_INLINE void qse_httpd_freemem (qse_httpd_t* httpd, void* ptr)
{
	QSE_MMGR_FREE (httpd->mmgr, ptr);
}

qse_mchar_t* qse_httpd_strtombsdup (qse_httpd_t* httpd, const qse_char_t* str)
{
	qse_mchar_t* mptr;

#if defined(QSE_CHAR_IS_MCHAR)
	mptr = qse_mbsdup (str, httpd->mmgr);
#else
	mptr = qse_wcstombsdup (str, QSE_NULL, httpd->mmgr);
#endif

	if (mptr == QSE_NULL) httpd->errnum = QSE_HTTPD_ENOMEM;
	return mptr;
}

qse_mchar_t* qse_httpd_strntombsdup (qse_httpd_t* httpd, const qse_char_t* str, qse_size_t len)
{
	qse_mchar_t* mptr;

#if defined(QSE_CHAR_IS_MCHAR)
	mptr = qse_mbsxdup (str, len, httpd->mmgr);
#else
	mptr = qse_wcsntombsdup (str, len, QSE_NULL, httpd->mmgr);
#endif

	if (mptr == QSE_NULL) httpd->errnum = QSE_HTTPD_ENOMEM;
	return mptr;
}

/* ----------------------------------------------------------------------- */

static qse_httpd_real_task_t* enqueue_task (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_httpd_task_t* pred, const qse_httpd_task_t* task,
	qse_size_t xtnsize)
{
	qse_httpd_real_task_t* new_task;
	qse_httpd_real_task_t* real_pred;
	
/* TODO: limit check 
	if (client->task.count >= httpd->limit.client_task_queue)
	{
		httpd->errnum = QSE_HTTPD_ETASK;
		return -1;
	}
*/
	new_task = (qse_httpd_real_task_t*)
		qse_httpd_allocmem (httpd, QSE_SIZEOF(*new_task) + xtnsize);
	if (new_task == QSE_NULL) return QSE_NULL;

	QSE_MEMSET (new_task, 0, QSE_SIZEOF(*new_task) + xtnsize);
	new_task->core = *task;

	if (new_task->core.init)
	{
		httpd->errnum = QSE_HTTPD_ENOERR;
		if (new_task->core.init (httpd, client, (qse_httpd_task_t*)new_task) <= -1)
		{
			if (httpd->errnum == QSE_HTTPD_ENOERR) 
				httpd->errnum = QSE_HTTPD_ETASK;
			qse_httpd_freemem (httpd, new_task);
			return QSE_NULL;	
		}
	}

	real_pred = (qse_httpd_real_task_t*)pred;
	if (pred)
	{
		new_task->next = real_pred->next;
		new_task->prev = real_pred;

		if (real_pred->next) real_pred->next->prev = new_task;
		else client->task.tail = (qse_httpd_task_t*)new_task;
		real_pred->next = new_task;
	}
	else
	{
		new_task->next = QSE_NULL;
		new_task->prev = (qse_httpd_real_task_t*)client->task.tail;

		if (client->task.tail) 
			((qse_httpd_real_task_t*)client->task.tail)->next = new_task;
		else 
			client->task.head = (qse_httpd_task_t*)new_task;
		client->task.tail = (qse_httpd_task_t*)new_task;
	}
	client->task.count++;

	return new_task;
}

static QSE_INLINE int dequeue_task (
	qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	qse_httpd_real_task_t* task;
	qse_size_t i;

	if (client->task.count <= 0) return -1;

	task = (qse_httpd_real_task_t*)client->task.head;

	/* clear task triggers from mux if they are registered */
	for (i = 0; i < QSE_COUNTOF(task->core.trigger); i++)
	{
		if (client->status & CLIENT_TASK_TRIGGER_IN_MUX(i))
		{
			httpd->opt.scb.mux.delhnd (httpd, httpd->mux, task->core.trigger[i].handle);
			client->status &= ~CLIENT_TASK_TRIGGER_IN_MUX(i);
		}
	}

	/* --------------------------------------------------- */

	if (task == (qse_httpd_real_task_t*)client->task.tail)
	{
		client->task.head = QSE_NULL;
		client->task.tail = QSE_NULL;
	}
	else
	{
		task->next->prev = QSE_NULL;
		client->task.head = (qse_httpd_task_t*)task->next;
	}
	client->task.count--;

	if (task->core.fini) task->core.fini (httpd, client, (qse_httpd_task_t*)task);
	qse_httpd_freemem (httpd, task);

	return 0;
}

static QSE_INLINE void purge_tasks (
	qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	while (dequeue_task (httpd, client) == 0);
}

/* ----------------------------------------------------------------------- */

static int htrd_peek_request (qse_htrd_t* htrd, qse_htre_t* req)
{
	htrd_xtn_t* xtn = (htrd_xtn_t*) qse_htrd_getxtn (htrd);
	return xtn->httpd->opt.rcb.peekreq (xtn->httpd, xtn->client, req);
}

static int htrd_poke_request (qse_htrd_t* htrd, qse_htre_t* req)
{
	htrd_xtn_t* xtn = (htrd_xtn_t*) qse_htrd_getxtn (htrd);
	return xtn->httpd->opt.rcb.pokereq (xtn->httpd, xtn->client, req);
}

static qse_htrd_recbs_t htrd_recbs =
{
	QSE_STRUCT_FIELD (peek, htrd_peek_request),
	QSE_STRUCT_FIELD (poke, htrd_poke_request)
};

/* ----------------------------------------------------------------------- */

static qse_httpd_client_t* new_client (
	qse_httpd_t* httpd, qse_httpd_client_t* tmpl)
{
	qse_httpd_client_t* client;
	htrd_xtn_t* xtn;

	client = qse_httpd_allocmem (httpd, QSE_SIZEOF(*client));
	if (client == QSE_NULL) return QSE_NULL;

	QSE_MEMSET (client, 0, QSE_SIZEOF(*client));

	client->type = QSE_HTTPD_CLIENT;
	client->htrd = qse_htrd_open (httpd->mmgr, QSE_SIZEOF(*xtn));
	if (client->htrd == QSE_NULL) 
	{
		httpd->errnum = QSE_HTTPD_ENOMEM;
		qse_httpd_freemem (httpd, client);
		return QSE_NULL;
	}

	qse_htrd_setoption (client->htrd, QSE_HTRD_REQUEST | QSE_HTRD_TRAILERS | QSE_HTRD_CANONQPATH);

	/* copy the public fields, 
	 * keep the private fields initialized at 0 */
	client->status = tmpl->status;
	if (httpd->opt.scb.client.accepted == QSE_NULL) 
		client->status |= CLIENT_READY;
	client->handle = tmpl->handle;
	client->handle2 = tmpl->handle2;
	client->remote_addr = tmpl->remote_addr;
	client->local_addr = tmpl->local_addr;
	client->orgdst_addr = tmpl->orgdst_addr;
	client->server = tmpl->server;
	client->initial_ifindex = tmpl->initial_ifindex;

	xtn = (htrd_xtn_t*)qse_htrd_getxtn (client->htrd);	
	xtn->httpd = httpd;
	xtn->client = client;

	qse_htrd_setrecbs (client->htrd, &htrd_recbs);

	return client;
}

static void free_client (
	qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	QSE_ASSERT (client->htrd != QSE_NULL);

	purge_tasks (httpd, client);

	qse_htrd_close (client->htrd);

#if 0
qse_printf (QSE_T("Debug: CLOSING SOCKET %d\n"), client->handle.i);
#endif

	if (client->status & CLIENT_HANDLE_IN_MUX)
	{
		httpd->opt.scb.mux.delhnd (httpd, httpd->mux, client->handle);
		client->status &= ~CLIENT_HANDLE_IN_MUX;
	}

	/* note that client.closed is not a counterpart to client.accepted. 
	 * so it is called even if client.close() failed. */
	if (httpd->opt.scb.client.closed)
		httpd->opt.scb.client.closed (httpd, client);
	
	httpd->opt.scb.client.close (httpd, client);

	qse_httpd_freemem (httpd, client);
}

static void purge_client (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	qse_httpd_client_t* prev;
	qse_httpd_client_t* next;

	prev = client->prev;
	next = client->next;

	if (httpd->opt.trait & QSE_HTTPD_LOGACT)
	{
		qse_httpd_act_t msg;
		msg.code = QSE_HTTPD_PURGE_CLIENT;
		msg.u.client = client;
		httpd->opt.rcb.logact (httpd, &msg);
	}

	free_client (httpd, client);

	if (prev) prev->next = next;
	else httpd->client.list.head = next;
	if (next) next->prev = prev;
	else httpd->client.list.tail = prev;
}

static void purge_client_list (qse_httpd_t* httpd)
{
	while (httpd->client.list.tail)
		purge_client (httpd, httpd->client.list.tail);
}

static void move_client_to_tail (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	if (httpd->client.list.tail != client)
	{
		qse_httpd_client_t* prev;
		qse_httpd_client_t* next;

		prev = client->prev;
		next = client->next;

		if (prev) prev->next = next;
		else httpd->client.list.head = next;
		if (next) next->prev = prev;
		else httpd->client.list.tail = prev;

		client->next = QSE_NULL;
		client->prev = httpd->client.list.tail;
		httpd->client.list.tail->next = client;
		httpd->client.list.tail = client;
	}
}

static int accept_client (
	qse_httpd_t* httpd, void* mux, qse_ubi_t handle, int mask, void* cbarg)
{
	qse_httpd_server_t* server;
	qse_httpd_client_t clibuf;
	qse_httpd_client_t* client;

	if (mask & QSE_HTTPD_MUX_READ)
	{
		server = (qse_httpd_server_t*)cbarg;

		/*QSE_ASSERT (handle == server->handle);*/

		QSE_MEMSET (&clibuf, 0, QSE_SIZEOF(clibuf));

		if (httpd->opt.scb.server.accept (httpd, server, &clibuf) <= -1) 
		{
#if 0
/* TODO: proper logging */
qse_char_t tmp[128];
qse_nwadtostr (&server->dope.nwad, tmp, QSE_COUNTOF(tmp), QSE_NWADTOSTR_ALL);
qse_printf (QSE_T("failed to accept from server [%s] [%d]\n"), tmp, server->handle.i);
#endif

			return -1;
		}

/* TODO: check maximum number of client. if exceed call client.close */

		if (server->dope.flags & QSE_HTTPD_SERVER_SECURE) clibuf.status |= CLIENT_SECURE;
		clibuf.server = server;

		client = new_client (httpd, &clibuf);
		if (client == QSE_NULL)
		{
			httpd->opt.scb.client.close (httpd, &clibuf);
			return -1;
		}

#if 0
qse_printf (QSE_T("MUX ADDHND CLIENT READ %d\n"), client->handle.i);
#endif
		if (httpd->opt.scb.mux.addhnd (
			httpd, mux, client->handle, QSE_HTTPD_MUX_READ, client) <= -1)
		{
			free_client (httpd, client);
			return -1;
		}
		client->status |= CLIENT_HANDLE_READ_IN_MUX;

		qse_gettime (&client->last_active); /* TODO: error check */
		/* link the new client to the tail of the client list. */
		if (httpd->client.list.tail)
		{
			QSE_ASSERT (httpd->client.list.head);
			client->prev = httpd->client.list.tail;
			httpd->client.list.tail->next = client;
			httpd->client.list.tail = client;
		}
		else
		{
			httpd->client.list.head = client;
			httpd->client.list.tail = client;
		}

		if (httpd->opt.trait & QSE_HTTPD_LOGACT)
		{
			qse_httpd_act_t msg;
			msg.code = QSE_HTTPD_ACCEPT_CLIENT;
			msg.u.client = client;
			httpd->opt.rcb.logact (httpd, &msg);
		}
	}
	return 0;
}

/* ----------------------------------------------------------------------- */

static void deactivate_servers (qse_httpd_t* httpd)
{
	qse_httpd_server_t* server;

	for (server = httpd->server.list.head; server; server = server->next)
	{
		if (server->dope.flags & QSE_HTTPD_SERVER_ACTIVE)
		{
			httpd->opt.scb.mux.delhnd (httpd, httpd->mux, server->handle);
			httpd->opt.scb.server.close (httpd, server);
			server->dope.flags &= ~QSE_HTTPD_SERVER_ACTIVE;
			httpd->server.nactive--;
		}
	}
}

static int activate_servers (qse_httpd_t* httpd)
{
	qse_httpd_server_t* server;

	for (server = httpd->server.list.head; server; server = server->next)
	{
		if (httpd->opt.scb.server.open (httpd, server) <= -1)
		{
			qse_char_t buf[64];
			qse_nwadtostr (&server->dope.nwad, buf, QSE_COUNTOF(buf), QSE_NWADTOSTR_ALL);

/*
			httpd->opt.rcb.log (httpd, 0, QSE_T("cannot activate %s"), buf);
*/
#if 0
qse_printf(QSE_T("cannot activate [%s]\n"), buf);
#endif
			continue;
		}

		if (httpd->opt.scb.mux.addhnd (
			httpd, httpd->mux, server->handle, QSE_HTTPD_MUX_READ, server) <= -1)
		{
			qse_char_t buf[64];
			qse_nwadtostr (&server->dope.nwad, buf, QSE_COUNTOF(buf), QSE_NWADTOSTR_ALL);
/*
			httpd->opt.rcb.log (httpd, 0, QSE_T("cannot activate %s - "), buf);
*/
#if 0
qse_printf(QSE_T("cannot add handle [%s]\n"), buf);
#endif

			httpd->opt.scb.server.close (httpd, server);
			continue;
		}

		server->dope.flags |= QSE_HTTPD_SERVER_ACTIVE;
		httpd->server.nactive++;
	}

	return 0;
}

static void free_server_list (qse_httpd_t* httpd)
{
	qse_httpd_server_t* server;

	server = httpd->server.list.head;

	while (server)
	{
		qse_httpd_server_t* next = server->next;
		qse_httpd_detachserver (httpd, server);
		server = next;
	}

	QSE_ASSERT (httpd->server.navail == 0);
	QSE_ASSERT (httpd->server.list.head == QSE_NULL);
	QSE_ASSERT (httpd->server.list.tail == QSE_NULL);
}

qse_httpd_server_t* qse_httpd_attachserver (
	qse_httpd_t* httpd, const qse_httpd_server_dope_t* dope, qse_size_t xtnsize)
{
	qse_httpd_server_t* server;

	server = qse_httpd_callocmem (httpd, QSE_SIZEOF(*server) + xtnsize);
	if (server == QSE_NULL) return QSE_NULL;

	server->type = QSE_HTTPD_SERVER;
	/* copy the server dope */
	server->dope = *dope;
	/* and correct some fields in case the dope contains invalid stuffs */
	server->dope.flags &= ~QSE_HTTPD_SERVER_ACTIVE;

	/* chain the server to the tail of the list */
	server->prev = httpd->server.list.tail;
	server->next = QSE_NULL;
	if (httpd->server.list.tail)
		httpd->server.list.tail->next = server;
	else
		httpd->server.list.head = server;
	httpd->server.list.tail = server;
	httpd->server.navail++;
	
	return server;
}

void qse_httpd_detachserver (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	qse_httpd_server_t* prev, * next;

	prev = server->prev;
	next = server->next;

	QSE_ASSERT (!(server->dope.flags & QSE_HTTPD_SERVER_ACTIVE));

	if (server->dope.detach) server->dope.detach (httpd, server);

	qse_httpd_freemem (httpd, server);
	httpd->server.navail--;

	if (prev) prev->next = next;
	else httpd->server.list.head = next;
	if (next) next->prev = prev;
	else httpd->server.list.tail = prev;
}

qse_httpd_server_t* qse_httpd_getfirstserver (qse_httpd_t* httpd)
{
	return httpd->server.list.head;
}

qse_httpd_server_t* qse_httpd_getlastserver (qse_httpd_t* httpd)
{
	return httpd->server.list.tail;
}

qse_httpd_server_t* qse_httpd_getnextserver (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	return server->next;
}

qse_httpd_server_t* qse_httpd_getprevserver (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	return server->prev;
}

/* ----------------------------------------------------------------------- */

#if 0
qse_httpd_dns_t* qse_httpd_attachdns (qse_httpd_t* httpd, qse_httpd_dns_dope_t* dns, qse_size_t xtnsize)
{
	qse_httpd_dns_t* dns;

	dns = qse_httpd_callocmem (httpd, QSE_SIZEOF(*dns) + xtnsize);
	if (dns == QSE_NULL) return QSE_NULL;

	dns->type = QSE_HTTPD_SERVER;
	/* copy the dns dope */
	dns->dope = *dope;
	/* and correct some fields in case the dope contains invalid stuffs */
	dns->dope.flags &= ~QSE_HTTPD_SERVER_ACTIVE;

	/* chain the dns to the tail of the list */
	dns->prev = httpd->dns.list.tail;
	dns->next = QSE_NULL;
	if (httpd->dns.list.tail)
		httpd->dns.list.tail->next = dns;
	else
		httpd->dns.list.head = dns;
	httpd->dns.list.tail = dns;
	httpd->dns.navail++;
	
	return dns;
}
#endif

/* ----------------------------------------------------------------------- */

static int read_from_client (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	qse_mchar_t buf[4096]; /* TODO: adjust this buffer size */
	qse_ssize_t m;
	
	QSE_ASSERT (httpd->opt.scb.client.recv != QSE_NULL);

reread:
	httpd->errnum = QSE_HTTPD_ENOERR;
	m = httpd->opt.scb.client.recv (httpd, client, buf, QSE_SIZEOF(buf));
	if (m <= -1)
	{
		if (httpd->errnum == QSE_HTTPD_EAGAIN)
		{
			/* nothing to read yet. */
			return 0; /* return ok */
		}
		else if (httpd->errnum == QSE_HTTPD_EINTR)
		{
			goto reread;
		}
		else
		{
			/* TOOD: if (httpd->errnum == QSE_HTTPD_ENOERR) httpd->errnum = QSE_HTTPD_ECALLBACK; */
			if (httpd->opt.trait & QSE_HTTPD_LOGACT)
			{
				qse_httpd_act_t msg;
				msg.code = QSE_HTTPD_READERR_CLIENT;
				msg.u.client = client;
				httpd->opt.rcb.logact (httpd, &msg);
			}
			/* TODO: find a way to disconnect */
			return -1;
		}
	}
	else if (m == 0)
	{
#if 0
qse_printf (QSE_T("Debug: connection closed %d\n"), client->handle.i);
#endif
		/* reading from the client returned 0. this typically
		 * happens when the client closes the connection or
		 * shutdown the writing half of the socket. it's
		 * not really easy to determine one from the other.
		 * if QSE_HTTPD_MUTECLIENT is on, attempt to handle
		 * it as a half-close under a certain condition. */

		if (httpd->opt.trait & QSE_HTTPD_MUTECLIENT &&
		    client->task.head && client->htrd->clean)
		{
			/* there is still more tasks to finish and 
			 * http reader is not waiting for any more feeds.  */
			client->status |= CLIENT_MUTE;
#if 0
qse_printf (QSE_T(">>>>> Marking client %d as MUTE\n"), client->handle.i);
#endif
			return 0;
		}
		else
		{
#if 0
qse_printf (QSE_T(">>>>> Returning failure for client %d\n"), client->handle.i);
#endif
			httpd->errnum = QSE_HTTPD_EDISCON;
			return -1;
		}
	}
	
#if 0
qse_printf (QSE_T("!!!!!FEEDING %d from %d ["), (int)m, (int)client->handle.i);
#if !defined(__WATCOMC__)
{
int i;
for (i = 0; i < m; i++) qse_printf (QSE_T("%hc"), buf[i]);
}
#endif
qse_printf (QSE_T("]\n"));
#endif

	/* qse_htrd_feed() may call the request callback 
	 * multiple times. that's because we don't know 
	 * how many valid requests are included in 'buf'. */ 
	httpd->errnum = QSE_HTTPD_ENOERR;
	if (qse_htrd_feed (client->htrd, buf, m) <= -1)
	{
		if (httpd->errnum == QSE_HTTPD_ENOERR)
		{
			if (client->htrd->errnum == QSE_HTRD_EBADRE || 
				client->htrd->errnum == QSE_HTRD_EBADHDR)
				httpd->errnum = QSE_HTTPD_EBADREQ;
			else httpd->errnum = QSE_HTTPD_ENOMEM; /* TODO: better translate error code */
		}
	
#if 0
qse_printf (QSE_T("Error: http error while processing %d ["), (int)client->handle.i);
{
int i;
for (i = 0; i < m; i++) qse_printf (QSE_T("%hc"), buf[i]);
}
qse_printf (QSE_T("]\n"));
#endif
		return -1;
	}


#if 0
qse_printf (QSE_T("!!!!!FEEDING OK OK OK OK %d from %d\n"), (int)m, (int)client->handle.i);
#endif

	if (client->status & CLIENT_PENDING) 
	{
		/* this CLIENT_PENDING thing is a dirty hack for SSL.
		 * In SSL, data is transmitted in a record. a record can be
		 * as large as 16K bytes since its length field is 2 bytes.
		 * If SSL_read() has record a record but it's given a
		 * smaller buffer than the actuaal record, the next call
		 * to select() won't return. there is no data to read
		 * at the socket layer. SSL_pending() can tell you the
		 * amount of data in the SSL buffer. I try to consume
		 * the pending data if the client.recv handler set CLIENT_PENDING.
		 *
		 * TODO: Investigate if there is any starvation issues.
		 *       What if a single client never stops sending? 
		 */
		goto reread;
	}

	return 0;
}

static int update_mux_for_current_task (qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	qse_size_t i;

	/* the code here is pretty fragile. there is a high chance
	 * that something can go wrong if the task handler plays
	 * with the trigger field in an unexpected manner. 
	 */
	for (i = 0; i < QSE_COUNTOF(task->trigger); i++)
	{
		task->trigger[i].mask &= ~(QSE_HTTPD_TASK_TRIGGER_READABLE | 
		                           QSE_HTTPD_TASK_TRIGGER_WRITABLE);
	}

	if (QSE_MEMCMP (client->trigger, task->trigger, QSE_SIZEOF(client->trigger)) != 0 || 
		((client->status & CLIENT_MUTE) && !(client->status & CLIENT_MUTE_DELETED)))
	{
		/* manipulate muxtiplexer settings if there are trigger changes */

		int has_trigger;
		int trigger_mux_mask;
		int client_handle_mux_mask;
		int client_handle_mux_status;

		/* delete previous trigger handles */
		for (i = 0; i < QSE_COUNTOF(task->trigger); i++)
		{
			if (client->status & CLIENT_TASK_TRIGGER_IN_MUX(i))
			{
				httpd->opt.scb.mux.delhnd (httpd, httpd->mux, client->trigger[i].handle);
				client->status &= ~CLIENT_TASK_TRIGGER_IN_MUX(i);
			}
		}

		has_trigger = 0;
		client_handle_mux_mask = 0;
		client_handle_mux_status = 0;
		if (client->status & CLIENT_MUTE)
		{
			client->status |= CLIENT_MUTE_DELETED;
		}
		else
		{
			client_handle_mux_mask |= QSE_HTTPD_MUX_READ;
			client_handle_mux_status |= CLIENT_HANDLE_READ_IN_MUX; 
		}

		/* add new trigger handles */
		for (i = 0; i < QSE_COUNTOF(task->trigger); i++)
		{
			trigger_mux_mask = 0;
			if (task->trigger[i].mask & QSE_HTTPD_TASK_TRIGGER_READ) 
			{
				if (task->trigger[i].handle.i != client->handle.i ||
					!(client->status & CLIENT_MUTE))
				{
					trigger_mux_mask |= QSE_HTTPD_MUX_READ;
				}
			}
			if (task->trigger[i].mask & QSE_HTTPD_TASK_TRIGGER_WRITE) 
				trigger_mux_mask |= QSE_HTTPD_MUX_WRITE;

			if (trigger_mux_mask)
			{
				has_trigger = 1;

				if (task->trigger[i].handle.i == client->handle.i) /* TODO: no direct comparsion */
				{
					/* if the client handle is included in the trigger,
					 * delay its manipulation until the loop is over.
					 * instead, just remember what mask is requested */
					client_handle_mux_mask |= trigger_mux_mask;
				}
				else
				{
					if (httpd->opt.scb.mux.addhnd (
						httpd, httpd->mux, task->trigger[i].handle,
						trigger_mux_mask, client) <= -1) 
					{
						return -1;
					}
					client->status |= CLIENT_TASK_TRIGGER_IN_MUX(i);
				}
			}
		}

		if (client_handle_mux_mask)
		{
			/* if the client handle is included in the trigger
			 * and writing is requested, arrange writing to be
			 * enabled */
			if (client_handle_mux_mask & QSE_HTTPD_MUX_WRITE)
				client_handle_mux_status |= CLIENT_HANDLE_WRITE_IN_MUX;
		}
		else if (!has_trigger)
		{
			/* if there is no trigger, writing should be enabled */
			client_handle_mux_status |= CLIENT_HANDLE_WRITE_IN_MUX;
			client_handle_mux_mask |= QSE_HTTPD_MUX_WRITE;
		}

		if ((client->status & CLIENT_HANDLE_IN_MUX) != 
			(client_handle_mux_status & CLIENT_HANDLE_IN_MUX))
		{
			httpd->opt.scb.mux.delhnd (httpd, httpd->mux, client->handle);
			client->status &= ~CLIENT_HANDLE_IN_MUX;

			if (client_handle_mux_mask)
			{
				if (httpd->opt.scb.mux.addhnd (
					httpd, httpd->mux, client->handle,
					client_handle_mux_mask, client) <= -1) 
				{
					return -1;
				}
				client->status |= client_handle_mux_status;
			}
		}

		QSE_MEMCPY (client->trigger, task->trigger, QSE_SIZEOF(client->trigger));
	}
	return 0;
}

static int update_mux_for_next_task (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	int mux_mask;
	int mux_status;

	mux_mask = QSE_HTTPD_MUX_READ;
	mux_status = CLIENT_HANDLE_READ_IN_MUX;
	if (client->task.head) 
	{
		/* there is a pending task. arrange to
		 * trigger it as if it is just entasked */
		mux_mask |= QSE_HTTPD_MUX_WRITE;
		mux_status |= CLIENT_HANDLE_WRITE_IN_MUX;

		if (client->status & CLIENT_MUTE)
		{
			mux_mask &= ~QSE_HTTPD_MUX_READ;
			mux_status &= ~CLIENT_HANDLE_READ_IN_MUX;
		}
	}
	else 
	{
		if (client->status & CLIENT_MUTE)
		{
			/* no more task. but this client
			 * has closed connection previously */
			return -1;
		}
	}

	if ((client->status & CLIENT_HANDLE_IN_MUX) != 
		(mux_status & CLIENT_HANDLE_IN_MUX))
	{
		httpd->opt.scb.mux.delhnd (httpd, httpd->mux, client->handle);
		client->status &= ~CLIENT_HANDLE_IN_MUX;

		if (mux_status)
		{
			if (httpd->opt.scb.mux.addhnd (
				httpd, httpd->mux, client->handle, mux_mask, client) <= -1) 
			{
				return -1;
			}
			client->status |= mux_status;
		}
	}

	QSE_MEMSET (client->trigger, 0, QSE_SIZEOF(client->trigger));
	return 0;
}

static int invoke_client_task (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_ubi_t handle, int mask)
{
	qse_httpd_task_t* task;
	qse_size_t i;
	int n, trigger_fired, client_handle_writable;

/* TODO: handle comparison callback ... */
	if (handle.i == client->handle.i && (mask & QSE_HTTPD_MUX_READ)) /* TODO: no direct comparision */
	{
		if (!(client->status & CLIENT_MUTE) && 
		    read_from_client (httpd, client) <= -1) 
		{
			/* return failure on disconnection also in order to
			 * purge the client in perform_client_task().
			 * thus the following line isn't necessary.
			 *if (httpd->errnum == QSE_HTTPD_EDISCON) return 0;*/
			return -1;
		}
	}

	/* this client doesn't have any task */
	task = client->task.head;
	if (task == QSE_NULL) 
	{
		if (client->status & CLIENT_MUTE)
		{
			/* handle this delayed client disconnection */
			return -1;
		}

		return 0;
	}

	trigger_fired = 0;
	client_handle_writable = 0;

	for (i = 0; i < QSE_COUNTOF(task->trigger); i++)
	{
		task->trigger[i].mask &= ~(QSE_HTTPD_TASK_TRIGGER_READABLE | 
		                           QSE_HTTPD_TASK_TRIGGER_WRITABLE);

		if (task->trigger[i].handle.i == handle.i) /* TODO: no direct comparision */
		{
			if (mask & QSE_HTTPD_MUX_READ)
			{
				QSE_ASSERT (task->trigger[i].mask & QSE_HTTPD_TASK_TRIGGER_READ);
				trigger_fired = 1;
				task->trigger[i].mask |= QSE_HTTPD_TASK_TRIGGER_READABLE;
			}
			if (mask & QSE_HTTPD_MUX_WRITE)
			{
				QSE_ASSERT (task->trigger[i].mask & QSE_HTTPD_TASK_TRIGGER_WRITE);
				trigger_fired = 1;
				task->trigger[i].mask |= QSE_HTTPD_TASK_TRIGGER_WRITABLE;
				if (handle.i == client->handle.i) client_handle_writable = 1; /* TODO: no direct comparison */
			}
		}
	}
	if (trigger_fired && !client_handle_writable)
	{
		/* the task is invoked for triggers. 
		 * check if the client handle is writable */
		qse_ntime_t tmout;
		tmout.sec = 0;
		tmout.nsec = 0;
		if (httpd->opt.scb.mux.writable (httpd, client->handle, &tmout) <= 0) 
		{
			/* it is not writable yet. so just skip 
			 * performing the actual task */
			return 0;
		}
	}

	n = task->main (httpd, client, task);
	if (n <= -1) 
	{
		/* task error */
		return -1;
	}
	else if (n == 0)
	{
		/* the current task is over. remove the task 
		 * from the queue. dequeue_task() clears task triggers
		 * from the mux. so i don't clear them explicitly here */
		dequeue_task (httpd, client); 
		return update_mux_for_next_task (httpd, client);
	}
	else 
	{
		return update_mux_for_current_task (httpd, client, task);
	}
}

static int perform_client_task (
	qse_httpd_t* httpd, void* mux, qse_ubi_t handle, int mask, void* cbarg)
{
	qse_httpd_client_t* client;

	client = (qse_httpd_client_t*)cbarg;

	if (client->status & CLIENT_BAD) return 0;

	if (!(client->status & CLIENT_READY))
	{
		int x;
		x = httpd->opt.scb.client.accepted (httpd, client);
		if (x <= -1) goto oops;
		if (x >= 1) 
		{
			client->status |= CLIENT_READY;

			qse_gettime (&client->last_active);
			move_client_to_tail (httpd, client);
		}
	}
	else
	{
		/* locate an active client to the tail of the client list */

		qse_gettime (&client->last_active); /* TODO: error check??? */
		move_client_to_tail (httpd, client);

		if (invoke_client_task (httpd, client, handle, mask) <= -1) goto oops;
	}

	return 0;

oops:
	/*purge_client (httpd, client);*/
	client->status |= CLIENT_BAD;
	client->bad_next = httpd->client.bad;
	httpd->client.bad = client;
	return -1;
}

static void purge_bad_clients (qse_httpd_t* httpd)
{
	qse_httpd_client_t* client;

	while (httpd->client.bad)
	{
		client = httpd->client.bad;
		httpd->client.bad = client->bad_next;
		purge_client (httpd, client);
	}
}

static void purge_idle_clients (qse_httpd_t* httpd)
{
	qse_httpd_client_t* client;
	qse_httpd_client_t* next_client;
	qse_ntime_t now;

	qse_gettime (&now);

	client = httpd->client.list.head;
	while (client)
	{
		next_client = client->next;

		/* TODO: check the nsec part */
		if (now.sec <= client->last_active.sec) break;
		if (now.sec - client->last_active.sec < httpd->opt.idle_limit.sec) break; 

		purge_client (httpd, client);
		client = next_client;
	}

/* TODO: */
}

void* qse_httpd_gettaskxtn (qse_httpd_t* httpd, qse_httpd_task_t* task)
{
	return (void*)((qse_httpd_real_task_t*)task + 1);
}

qse_httpd_task_t* qse_httpd_entask (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_httpd_task_t* pred, const qse_httpd_task_t* task,
	qse_size_t xtnsize)
{
	qse_httpd_real_task_t* new_task;

	if (client->status & CLIENT_BAD) return QSE_NULL;

	new_task = enqueue_task (httpd, client, pred, task, xtnsize);
	if (new_task == QSE_NULL) 
	{
		/*purge_client (httpd, client);*/
		client->status |= CLIENT_BAD;
	}
	else if (new_task->prev == QSE_NULL)
	{
		/* arrange to invokde this task so long as 
		 * the client-side handle is writable. */
		QSE_ASSERT (client->status & CLIENT_HANDLE_IN_MUX);
		httpd->opt.scb.mux.delhnd (httpd, httpd->mux, client->handle);
		client->status &= ~CLIENT_HANDLE_IN_MUX;

#if 0
printf ("MUX ADDHND CLIENT RW(ENTASK) %d\n", client->handle.i);
#endif
		if (httpd->opt.scb.mux.addhnd (
			httpd, httpd->mux, client->handle, 
			QSE_HTTPD_MUX_READ | QSE_HTTPD_MUX_WRITE, 
			client) <= -1)
		{
			/*purge_client (httpd, client);*/
			client->status |= CLIENT_BAD;
			new_task = QSE_NULL;
		}
		client->status |= CLIENT_HANDLE_IN_MUX; /* READ | WRITE */
	}

	return (qse_httpd_task_t*)new_task;
}

static int dispatch_mux (
	qse_httpd_t* httpd, void* mux, qse_ubi_t handle, int mask, void* cbarg)
{
	return ((qse_httpd_mate_t*)cbarg)->type == QSE_HTTPD_SERVER?
		accept_client (httpd, mux, handle, mask, cbarg):
		perform_client_task (httpd, mux, handle, mask, cbarg);
}

int qse_httpd_loop (qse_httpd_t* httpd)
{
	int xret;

	QSE_ASSERTX (httpd->server.list.head != QSE_NULL,
		"Add listeners before calling qse_httpd_loop()");

	QSE_ASSERTX (httpd->client.list.head == QSE_NULL,
		"No client should exist when this loop is started");

	if (httpd->server.list.head == QSE_NULL) 
	{
		httpd->errnum = QSE_HTTPD_EINVAL;
		return -1;
	}

	httpd->stopreq = 0;
	httpd->impedereq = 0;

	/* system callbacks and request callbacks must be set before the call to this function */
	QSE_ASSERT (httpd->opt.scb.mux.open && httpd->opt.scb.mux.close && httpd->opt.scb.mux.poll);
	QSE_ASSERT (httpd->opt.rcb.peekreq && httpd->opt.rcb.pokereq);

	/* a server must be attached before the call to this function */
	QSE_ASSERT (httpd->server.navail > 0);

	httpd->mux = httpd->opt.scb.mux.open (httpd, dispatch_mux);
	if (httpd->mux == QSE_NULL) return -1;

	if (activate_servers (httpd) <= -1) 
	{
		httpd->opt.scb.mux.close (httpd, httpd->mux);
		return -1;
	}
	if (httpd->server.nactive <= 0)
	{
		httpd->errnum = QSE_HTTPD_ENOSVR;
		httpd->opt.scb.mux.close (httpd, httpd->mux);
		return -1;
	}

	xret = 0;

	while (!httpd->stopreq)
	{
		int count;

		count = httpd->opt.scb.mux.poll (httpd, httpd->mux, &httpd->opt.tmout);
		if (count <= -1) 
		{
			if (httpd->errnum != QSE_HTTPD_EINTR)
			{
				xret = -1; 
				break;
			}
		}

		purge_bad_clients (httpd);
		purge_idle_clients (httpd);

		if (httpd->impedereq)
		{
			httpd->impedereq = 0;
			httpd->opt.rcb.impede (httpd);
		}
	}

	purge_client_list (httpd);
	deactivate_servers (httpd);
	httpd->opt.scb.mux.close (httpd, httpd->mux);
	return xret;
}

/* --------------------------------------------------- */

void qse_httpd_discardcontent (qse_httpd_t* httpd, qse_htre_t* req)
{
	qse_htre_discardcontent (req);
}

void qse_httpd_completecontent (qse_httpd_t* httpd, qse_htre_t* req)
{
	qse_htre_completecontent (req);
}

/* --------------------------------------------------- */

void qse_httpd_setname (qse_httpd_t* httpd, const qse_mchar_t* name)
{
	qse_mbsxcpy (httpd->sname, QSE_COUNTOF(httpd->sname), name);
}

const qse_mchar_t* qse_httpd_getname (qse_httpd_t* httpd)
{
	return httpd->sname;
}

const qse_mchar_t* qse_httpd_fmtgmtimetobb (
	qse_httpd_t* httpd, const qse_ntime_t* nt, int idx)
{
	qse_ntime_t now;

	QSE_ASSERT (idx >= 0 && idx < QSE_COUNTOF(httpd->gtbuf));

	if (nt == QSE_NULL) 
	{
		if (qse_gettime(&now) <= -1) 
		{
			now.sec = 0;
			now.nsec = 0;
		}
		nt = &now;
	}

	qse_fmthttptime (nt, httpd->gtbuf[idx], QSE_COUNTOF(httpd->gtbuf[idx]));
	return httpd->gtbuf[idx];
}


/* --------------------------------------------------- */

qse_mchar_t* qse_httpd_escapehtml (qse_httpd_t* httpd, const qse_mchar_t* str)
{
	qse_mchar_t* ptr, * buf;
	qse_size_t reqlen = 0;

	for (ptr = (qse_mchar_t*)str; *ptr != QSE_MT('\0'); ptr++) 
	{
		switch (*ptr)
		{
			case QSE_MT('<'):
			case QSE_MT('>'):
				reqlen += 4;
				break;

			case QSE_MT('&'):
				reqlen += 5;
				break;

			default: 
				reqlen++;
				break;
		}
	}

	if (ptr - str == reqlen) return (qse_mchar_t*)str; /* no escaping is needed */

	buf = qse_httpd_allocmem (httpd, QSE_SIZEOF(*buf) * (reqlen + 1));
	if (buf == QSE_NULL) return QSE_NULL;

	ptr = buf;
	while (*str != QSE_MT('\0'))
	{
		switch (*str)
		{
			case QSE_MT('<'):
				*ptr++ = QSE_MT('&');
				*ptr++ = QSE_MT('l');
				*ptr++ = QSE_MT('t');
				*ptr++ = QSE_MT(';');
				break;

			case QSE_MT('>'):
				*ptr++ = QSE_MT('&');
				*ptr++ = QSE_MT('g');
				*ptr++ = QSE_MT('t');
				*ptr++ = QSE_MT(';');
				break;

			case QSE_MT('&'):
				*ptr++ = QSE_MT('&');
				*ptr++ = QSE_MT('a');
				*ptr++ = QSE_MT('m');
				*ptr++ = QSE_MT('p');
				*ptr++ = QSE_MT(';');
				break;

			default: 
				*ptr++ = *str;
				break;
		}
		str++;
	}
	*ptr = QSE_MT('\0');
	return buf;
}
