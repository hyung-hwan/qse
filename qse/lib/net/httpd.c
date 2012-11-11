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

#include "httpd.h"
#include "../cmn/mem.h"
#include <qse/cmn/chr.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mbwc.h>

#include <qse/cmn/stdio.h>

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

	httpd = (qse_httpd_t*) QSE_MMGR_ALLOC (
		mmgr, QSE_SIZEOF(*httpd) + xtnsize
	);
	if (httpd == QSE_NULL) return QSE_NULL;

	if (qse_httpd_init (httpd, mmgr) <= -1)
	{
		QSE_MMGR_FREE (httpd->mmgr, httpd);
		return QSE_NULL;
	}

	QSE_MEMSET (httpd + 1, 0, xtnsize);
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

int qse_httpd_getoption (qse_httpd_t* httpd)
{
	return httpd->option;
}

void qse_httpd_setoption (qse_httpd_t* httpd, int option)
{
	httpd->option = option;
}

/* --------------------------------------------------- */

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

/* --------------------------------------------------- */

QSE_INLINE void* qse_httpd_allocmem (qse_httpd_t* httpd, qse_size_t size)
{
	void* ptr = QSE_MMGR_ALLOC (httpd->mmgr, size);
	if (ptr == QSE_NULL) httpd->errnum = QSE_HTTPD_ENOMEM;
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

/* --------------------------------------------------- */

static qse_httpd_task_t* enqueue_task (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_httpd_task_t* pred, const qse_httpd_task_t* task,
	qse_size_t xtnsize)
{
	qse_httpd_task_t* new_task;

/* TODO: limit check 
	if (client->task.count >= httpd->limit.client_task_queue)
	{
		httpd->errnum = QSE_HTTPD_ETASK;
		return -1;
	}
*/
	new_task = (qse_httpd_task_t*)
		qse_httpd_allocmem (httpd, QSE_SIZEOF(*new_task) + xtnsize);
	if (new_task == QSE_NULL) return QSE_NULL;

	QSE_MEMSET (new_task, 0, QSE_SIZEOF(*new_task) + xtnsize);
	*new_task = *task;

	if (new_task->init)
	{
		httpd->errnum = QSE_HTTPD_ENOERR;
		if (new_task->init (httpd, client, new_task) <= -1)
		{
			if (httpd->errnum == QSE_HTTPD_ENOERR) 
				httpd->errnum = QSE_HTTPD_ETASK;
			qse_httpd_freemem (httpd, new_task);
			return QSE_NULL;	
		}
	}

	if (pred)
	{
		new_task->next = pred->next;
		new_task->prev = pred;

		if (pred->next) pred->next->prev = new_task;
		else client->task.tail = new_task;
		pred->next = new_task;
	}
	else
	{
		new_task->next = QSE_NULL;
		new_task->prev = client->task.tail;

		if (client->task.tail) 
			client->task.tail->next = new_task;
		else 
			client->task.head = new_task;
		client->task.tail = new_task;
	}
	client->task.count++;

	return new_task;
}

static QSE_INLINE int dequeue_task (
	qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	qse_httpd_task_t* task;
	qse_size_t i;

	if (client->task.count <= 0) return -1;

	task = client->task.head;

	/* clear task triggers from mux if they are registered */
	for (i = 0; i < QSE_COUNTOF(task->trigger); i++)
	{
		if (client->status & CLIENT_TASK_TRIGGER_IN_MUX(i))
		{
			httpd->scb->mux.delhnd (httpd, httpd->mux, task->trigger[i].handle);
			client->status &= ~CLIENT_TASK_TRIGGER_IN_MUX(i);
		}
	}

	/* --------------------------------------------------- */

	if (task == client->task.tail)
	{
		client->task.head = QSE_NULL;
		client->task.tail = QSE_NULL;
	}
	else
	{
		task->next->prev = QSE_NULL;
		client->task.head = task->next;
	}
	client->task.count--;

	if (task->fini) task->fini (httpd, client, task);
	qse_httpd_freemem (httpd, task);

	return 0;
}

static QSE_INLINE void purge_tasks (
	qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	while (dequeue_task (httpd, client) == 0);
}

/* --------------------------------------------------- */

static int htrd_peek_request (qse_htrd_t* htrd, qse_htre_t* req)
{
	htrd_xtn_t* xtn = (htrd_xtn_t*) qse_htrd_getxtn (htrd);
	return xtn->httpd->rcb->peek_request (xtn->httpd, xtn->client, req);
}

static int htrd_handle_request (qse_htrd_t* htrd, qse_htre_t* req)
{
	htrd_xtn_t* xtn = (htrd_xtn_t*) qse_htrd_getxtn (htrd);
	return xtn->httpd->rcb->handle_request (xtn->httpd, xtn->client, req);
}

static qse_htrd_recbs_t htrd_recbs =
{
	htrd_peek_request,
	htrd_handle_request
};

/* --------------------------------------------------- */

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
	if (httpd->scb->client.accepted == QSE_NULL) 
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

qse_printf (QSE_T("Debug: CLOSING SOCKET %d\n"), client->handle.i);

	if (client->status & CLIENT_HANDLE_IN_MUX)
	{
		httpd->scb->mux.delhnd (httpd, httpd->mux, client->handle);
		client->status &= ~CLIENT_HANDLE_IN_MUX;
	}

	/* note that client.closed is not a counterpart to client.accepted. 
	 * so it is called even if client.close() failed. */
	if (httpd->scb->client.closed)
		httpd->scb->client.closed (httpd, client);
	
	httpd->scb->client.close (httpd, client);

	qse_httpd_freemem (httpd, client);
}

static void purge_client (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	qse_httpd_client_t* prev;
	qse_httpd_client_t* next;

	prev = client->prev;
	next = client->next;

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

		if (httpd->scb->server.accept (httpd, server, &clibuf) <= -1) 
		{
/* TODO: proper logging */
qse_char_t tmp[128];
qse_nwadtostr (&server->nwad, tmp, QSE_COUNTOF(tmp), QSE_NWADTOSTR_ALL);
qse_printf (QSE_T("failed to accept from server %s\n"), tmp);

			return -1;
		}

/* TODO: check maximum number of client. if exceed call client.close */

		if (server->flags & QSE_HTTPD_SERVER_SECURE) clibuf.status |= CLIENT_SECURE;
		clibuf.server = server;

		client = new_client (httpd, &clibuf);
		if (client == QSE_NULL)
		{
			httpd->scb->client.close (httpd, &clibuf);
			return -1;
		}

qse_printf (QSE_T("MUX ADDHND CLIENT READ %d\n"), client->handle.i);
		if (httpd->scb->mux.addhnd (
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

{
/* TODO: proper logging */
qse_char_t tmp[128], tmp2[128], tmp3[128];
qse_nwadtostr (&client->local_addr, tmp, QSE_COUNTOF(tmp), QSE_NWADTOSTR_ALL);
qse_nwadtostr (&client->orgdst_addr, tmp2, QSE_COUNTOF(tmp2), QSE_NWADTOSTR_ALL);
qse_nwadtostr (&client->remote_addr, tmp3, QSE_COUNTOF(tmp3), QSE_NWADTOSTR_ALL);
qse_printf (QSE_T("connection %d accepted %s(%s from %s\n"), client->handle.i, tmp, tmp2, tmp3);
}
	}
	return 0;
}

/* --------------------------------------------------- */

static void deactivate_servers (qse_httpd_t* httpd)
{
	qse_httpd_server_t* server;

	for (server = httpd->server.list.head; server; server = server->next)
	{
		if (server->flags & QSE_HTTPD_SERVER_ACTIVE)
		{
			httpd->scb->mux.delhnd (httpd, httpd->mux, server->handle);
			httpd->scb->server.close (httpd, server);
			server->flags &= ~QSE_HTTPD_SERVER_ACTIVE;
			httpd->server.nactive--;
		}
	}
}

static int activate_servers (qse_httpd_t* httpd)
{
	qse_httpd_server_t* server;

	for (server = httpd->server.list.head; server; server = server->next)
	{
		if (httpd->scb->server.open (httpd, server) <= -1)
		{
qse_char_t buf[64];
qse_nwadtostr (&server->nwad, buf, QSE_COUNTOF(buf), QSE_NWADTOSTR_ALL);
qse_printf (QSE_T("FAILED TO ACTIVATE SERVER....[%s]\n"), buf);
			continue;
		}

qse_printf (QSE_T("MUX ADDHND SERVER %d\n"), server->handle.i);
		if (httpd->scb->mux.addhnd (
			httpd, httpd->mux, server->handle, QSE_HTTPD_MUX_READ, server) <= -1)
		{
qse_printf (QSE_T("FAILED TO ADD SERVER HANDLE TO MUX....\n"));
			httpd->scb->server.close (httpd, server);
			continue;
		}

		server->flags |= QSE_HTTPD_SERVER_ACTIVE;
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
	qse_httpd_t* httpd, const qse_httpd_server_t* tmpl, 
	qse_httpd_server_predetach_t predetach, qse_size_t xtnsize)
{
	qse_httpd_server_t* server;

	server = qse_httpd_allocmem (httpd, QSE_SIZEOF(*server) + xtnsize);
	if (server == QSE_NULL) return QSE_NULL;

	QSE_MEMCPY (server, tmpl, QSE_SIZEOF(*server)); 
	QSE_MEMSET (server + 1, 0, xtnsize);

	server->type = QSE_HTTPD_SERVER;
	server->flags &= ~QSE_HTTPD_SERVER_ACTIVE;
	server->predetach = predetach;

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

	QSE_ASSERT (!(server->flags & QSE_HTTPD_SERVER_ACTIVE));

	if (server->predetach) server->predetach (httpd, server);

	qse_httpd_freemem (httpd, server);
	httpd->server.navail--;

	if (prev) prev->next = next;
	else httpd->server.list.head = next;
	if (next) next->prev = prev;
	else httpd->server.list.tail = prev;
}

/* --------------------------------------------------- */

static int read_from_client (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	qse_mchar_t buf[4096]; /* TODO: adjust this buffer size */
	qse_ssize_t m;
	
	QSE_ASSERT (httpd->scb->client.recv != QSE_NULL);

reread:
	httpd->errnum = QSE_HTTPD_ENOERR;
	m = httpd->scb->client.recv (httpd, client, buf, QSE_SIZEOF(buf));
	if (m <= -1)
	{
		if (httpd->errnum == QSE_HTTPD_EAGAIN)
		{
			/* nothing to read yet. */
qse_printf (QSE_T("Warning: Nothing to read from a client %d\n"), client->handle.i);
			return 0; /* return ok */
		}
		else if (httpd->errnum == QSE_HTTPD_EINTR)
		{
			goto reread;
		}
		else
		{
			/* TOOD: if (httpd->errnum == QSE_HTTPD_ENOERR) httpd->errnum = QSE_HTTPD_ECALLBACK; */
qse_printf (QSE_T("Error: failed to read from a client %d\n"), client->handle.i);
	/* TODO: find a way to disconnect */
			return -1;
		}
	}
	else if (m == 0)
	{
qse_printf (QSE_T("Debug: connection closed %d\n"), client->handle.i);
		/* reading from the client returned 0. this typically
		 * happens when the client closes the connection or
		 * shutdown the writing half of the socket. it's
		 * not really easy to determine one from the other.
		 * if QSE_HTTPD_MUTECLIENT is on, attempt to handle
		 * it as a half-close under a certain condition. */

		if (httpd->option & QSE_HTTPD_MUTECLIENT &&
		    client->task.head && client->htrd->clean)
		{
			/* there is still more tasks to finish and 
			 * http reader is not waiting for any more feeds.  */
			client->status |= CLIENT_MUTE;
qse_printf (QSE_T(">>>>> Marking client %d as MUTE\n"), client->handle.i);
			return 0;
		}
		else
		{
qse_printf (QSE_T(">>>>> Returning failure for client %d\n"), client->handle.i);
			httpd->errnum = QSE_HTTPD_EDISCON;
			return -1;
		}
	}
	
qse_printf (QSE_T("!!!!!FEEDING %d from %d ["), (int)m, (int)client->handle.i);
{
int i;
for (i = 0; i < m; i++) qse_printf (QSE_T("%hc"), buf[i]);
}
qse_printf (QSE_T("]\n"));

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
	
qse_printf (QSE_T("Error: http error while processing %d ["), (int)client->handle.i);
{
int i;
for (i = 0; i < m; i++) qse_printf (QSE_T("%hc"), buf[i]);
}
qse_printf (QSE_T("]\n"));


		return -1;
	}
qse_printf (QSE_T("!!!!!FEEDING OK OK OK OK %d from %d\n"), (int)m, (int)client->handle.i);

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

static int invoke_client_task (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_ubi_t handle, int mask)
{
	qse_httpd_task_t* task;
	qse_size_t i;
	int n, trigger_fired, client_handle_writable;

/* TODO: handle comparison callback ... */
qse_printf (QSE_T("INVOKE CLIENT TASK..........\n"));
	if (handle.i == client->handle.i && (mask & QSE_HTTPD_MUX_READ)) /* TODO: no direct comparision */
	{
		if (!(client->status & CLIENT_MUTE) && 
		    read_from_client (httpd, client) <= -1) 
		{
			/* return failure on disconnection also in order to
			 * purge the client in perform_client_task().
			 * thus the following line isn't necessary.
			 *if (httpd->errnum == QSE_HTTPD_EDISCON) return 0;*/
qse_printf (QSE_T("ERROR: read from client [%d] failed...\n"), (int)handle.i);
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
qse_printf (QSE_T("ERROR: mute client got no more task [%d] failed...\n"), (int)client->handle.i);
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
			if (task->trigger[i].mask & QSE_HTTPD_TASK_TRIGGER_READ)
			{
				trigger_fired = 1;
				task->trigger[i].mask |= QSE_HTTPD_TASK_TRIGGER_READABLE;
			}
			if (task->trigger[i].mask & QSE_HTTPD_TASK_TRIGGER_WRITE)
			{
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
		if (httpd->scb->mux.writable (httpd, client->handle, &tmout) <= 0) 
		{
			/* it is not writable yet. so just skip 
			 * performing the actual task */
			return 0;
		}
	}

	n = task->main (httpd, client, task);
qse_printf (QSE_T("task returend %d\n"), n);
	if (n <= -1) return -1;
	else if (n == 0)
	{
		int mux_mask;
		int mux_status;

		/* the current task is over. remove the task 
		 * from the queue. dequeue_task() clears task triggers
		 * from the mux. so i don't clear them explicitly here */

		dequeue_task (httpd, client); 
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
qse_printf (QSE_T("REMOVING XXXXX FROM READING....\n"));
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
qse_printf (QSE_T("REMOVING XXXXX FROM READING NO MORE TASK....\n"));
				return -1;
			}
		}

		if ((client->status & CLIENT_HANDLE_IN_MUX) != 
		    (mux_status & CLIENT_HANDLE_IN_MUX))
		{
			httpd->scb->mux.delhnd (httpd, httpd->mux, client->handle);
			client->status &= ~CLIENT_HANDLE_IN_MUX;

			if (mux_status)
			{
				if (httpd->scb->mux.addhnd (
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
	else 
	{
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
					httpd->scb->mux.delhnd (httpd, httpd->mux, client->trigger[i].handle);
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
						if (httpd->scb->mux.addhnd (
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
				httpd->scb->mux.delhnd (httpd, httpd->mux, client->handle);
				client->status &= ~CLIENT_HANDLE_IN_MUX;
	
				if (client_handle_mux_mask)
				{
					if (httpd->scb->mux.addhnd (
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
		x = httpd->scb->client.accepted (httpd, client);
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

		if (invoke_client_task (httpd, client, handle, mask) <= -1) 
		{
qse_printf (QSE_T("OOPS AFTER CLIENT TASK BAD XXXXXXXXXXXXXX [%d]\n"), (int)handle.i);
			goto oops;
		}
	}

	return 0;

oops:
qse_printf (QSE_T("MARKING BAD XXXXXXXXXXXXXX [%d]\n"), (int)handle.i);
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
qse_printf (QSE_T("PURGING BAD CLIENT XXXXXXXXXXXXXX\n"));
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
		if (now.sec <= client->last_active.sec) break;
		if (now.sec - client->last_active.sec < 30) break; /* TODO: make this time configurable... */

qse_printf (QSE_T("PURGING IDLE CLIENT XXXXXXXXXXXXXX %d\n"), (int)(now.sec - client->last_active.sec));
		purge_client (httpd, client);
		client = next_client;
	}

/* TODO: */
}

qse_httpd_task_t* qse_httpd_entask (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_httpd_task_t* pred, const qse_httpd_task_t* task,
	qse_size_t xtnsize)
{
	qse_httpd_task_t* new_task;

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
		httpd->scb->mux.delhnd (httpd, httpd->mux, client->handle);
		client->status &= ~CLIENT_HANDLE_IN_MUX;

qse_printf (QSE_T("MUX ADDHND CLIENT RW(ENTASK) %d\n"), client->handle.i);
		if (httpd->scb->mux.addhnd (
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

	return new_task;
}

static int dispatch_mux (
	qse_httpd_t* httpd, void* mux, qse_ubi_t handle, int mask, void* cbarg)
{
	return ((qse_httpd_server_t*)cbarg)->type == QSE_HTTPD_SERVER?
		accept_client (httpd, mux, handle, mask, cbarg):
		perform_client_task (httpd, mux, handle, mask, cbarg);
}

int qse_httpd_loop (qse_httpd_t* httpd, qse_httpd_scb_t* scb, qse_httpd_rcb_t* rcb, const qse_ntime_t* tmout)
{
	QSE_ASSERTX (httpd->server.list.head != QSE_NULL,
		"Add listeners before calling qse_httpd_loop()");	

	QSE_ASSERTX (httpd->client.list.head == QSE_NULL,
		"No client should exist when this loop is started");

	if (scb == QSE_NULL || rcb == QSE_NULL || 
	    httpd->server.list.head == QSE_NULL) 
	{
		httpd->errnum = QSE_HTTPD_EINVAL;
		return -1;
	}

	httpd->stopreq = 0;
	httpd->scb = scb;
	httpd->rcb = rcb;

	QSE_ASSERT (httpd->server.navail > 0);

	httpd->mux = httpd->scb->mux.open (httpd, dispatch_mux);
	if (httpd->mux == QSE_NULL)
	{
qse_printf (QSE_T("can't open mux....\n"));
		return -1;
	}

	if (activate_servers (httpd) <= -1) 
	{
		httpd->scb->mux.close (httpd, httpd->mux);
		return -1;
	}
	if (httpd->server.nactive <= 0)
	{
qse_printf (QSE_T("no servers are active....\n"));
		httpd->scb->mux.close (httpd, httpd->mux);
		return -1;
	}

	while (!httpd->stopreq)
	{
		int count;

		count = httpd->scb->mux.poll (httpd, httpd->mux, tmout);
		if (count <= -1)
		{
			httpd->errnum = QSE_HTTPD_EIOMUX;
/* TODO: call user callback for this multiplexer error */
			/*if (errno == EINTR) continue;*/
qse_fprintf (QSE_STDERR, QSE_T("Error: mux returned failure\n"));
			/* break; */
		}

		purge_bad_clients (httpd);
		purge_idle_clients (httpd);
	}

	purge_client_list (httpd);
	deactivate_servers (httpd);
	httpd->scb->mux.close (httpd, httpd->mux);
	return 0;
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

qse_mchar_t* qse_httpd_getname (qse_httpd_t* httpd)
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
