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

#if defined(_WIN32) || defined(__DOS__) || defined(__OS2__)
/* UNSUPPORTED YET..  */ 
/* TODO: IMPLEMENT THIS */
#else

#include "httpd.h"
#include "../cmn/mem.h"
#include "../cmn/syscall.h"
#include <qse/cmn/chr.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/hton.h>

#include <qse/cmn/stdio.h>

typedef struct htrd_xtn_t htrd_xtn_t;

struct htrd_xtn_t
{
	qse_httpd_t*        httpd;
	qse_httpd_client_t* client;
};

QSE_IMPLEMENT_COMMON_FUNCTIONS (httpd)

#define DEFAULT_PORT        80
#define DEFAULT_SECURE_PORT 443

enum client_status_t
{
	CLIENT_BAD                       = (1 << 0),
	CLIENT_READY                     = (1 << 1),
	CLIENT_SECURE                    = (1 << 2),

	CLIENT_HANDLE_READ_IN_MUX        = (1 << 3),
	CLIENT_HANDLE_WRITE_IN_MUX       = (1 << 4),
	CLIENT_HANDLE_IN_MUX             = (CLIENT_HANDLE_READ_IN_MUX |
	                                    CLIENT_HANDLE_WRITE_IN_MUX),

	CLIENT_TASK_TRIGGER_READ_IN_MUX  = (1 << 5),
	CLIENT_TASK_TRIGGER_RELAY_IN_MUX = (1 << 6),
	CLIENT_TASK_TRIGGER_WRITE_IN_MUX = (1 << 7),
	CLIENT_TASK_TRIGGER_IN_MUX       = (CLIENT_TASK_TRIGGER_READ_IN_MUX |
	                                    CLIENT_TASK_TRIGGER_RELAY_IN_MUX |
	                                    CLIENT_TASK_TRIGGER_WRITE_IN_MUX)
};

static void free_server_list (
	qse_httpd_t* httpd, qse_httpd_server_t* server);
static int perform_client_task (
	qse_httpd_t* httpd, void* mux, qse_ubi_t handle, int mask, void* cbarg);

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

	return httpd;
}

void qse_httpd_close (qse_httpd_t* httpd)
{
	qse_httpd_fini (httpd);
	QSE_MMGR_FREE (httpd->mmgr, httpd);
}

int qse_httpd_init (qse_httpd_t* httpd, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (httpd, 0, QSE_SIZEOF(*httpd));
	httpd->mmgr = mmgr;
	return 0;
}

void qse_httpd_fini (qse_httpd_t* httpd)
{
/* TODO */
	free_server_list (httpd, httpd->server.list);
	QSE_ASSERT (httpd->server.navail == 0);
	httpd->server.list = QSE_NULL;
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

int qse_httpd_getoption (qse_httpd_t* httpd)
{
	return httpd->option;
}

void qse_httpd_setoption (qse_httpd_t* httpd, int option)
{
	httpd->option = option;
}

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

	QSE_MEMCPY (new_task, task, QSE_SIZEOF(*new_task));

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

	if (client->task.count <= 0) return -1;

	task = client->task.head;

	/* clear task triggers from mux if they are registered */
	if (client->status & CLIENT_TASK_TRIGGER_READ_IN_MUX)
	{
		httpd->cbs->mux.delhnd (httpd, httpd->mux, task->trigger[0]);
		client->status &= ~CLIENT_TASK_TRIGGER_READ_IN_MUX;
	}
	if (client->status & CLIENT_TASK_TRIGGER_RELAY_IN_MUX)
	{
		httpd->cbs->mux.delhnd (httpd, httpd->mux, task->trigger[1]);
		client->status &= ~CLIENT_TASK_TRIGGER_RELAY_IN_MUX;
	}
	if (client->status & CLIENT_TASK_TRIGGER_WRITE_IN_MUX)
	{
		httpd->cbs->mux.delhnd (httpd, httpd->mux, task->trigger[2]);
		client->status &= ~CLIENT_TASK_TRIGGER_WRITE_IN_MUX;
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
	return xtn->httpd->cbs->peek_request (xtn->httpd, xtn->client, req);
}

static int htrd_handle_request (qse_htrd_t* htrd, qse_htre_t* req)
{
	htrd_xtn_t* xtn = (htrd_xtn_t*) qse_htrd_getxtn (htrd);
	return xtn->httpd->cbs->handle_request (xtn->httpd, xtn->client, req);
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
	int opt;

	client = qse_httpd_allocmem (httpd, QSE_SIZEOF(*client));
	if (client == QSE_NULL) return QSE_NULL;

	QSE_MEMSET (client, 0, QSE_SIZEOF(*client));

	client->htrd = qse_htrd_open (httpd->mmgr, QSE_SIZEOF(*xtn));
	if (client->htrd == QSE_NULL) 
	{
		httpd->errnum = QSE_HTTPD_ENOMEM;
		qse_httpd_freemem (httpd, client);
		return QSE_NULL;
	}

	opt = qse_htrd_getoption (client->htrd);
	opt |= QSE_HTRD_REQUEST;
	opt &= ~QSE_HTRD_RESPONSE;
	qse_htrd_setoption (client->htrd, opt);

	if (httpd->cbs->client.accepted == QSE_NULL) 
		client->status |= CLIENT_READY;

	client->status = tmpl->status;
	client->handle = tmpl->handle;
	client->handle2 = tmpl->handle2;
	client->local_addr = tmpl->local_addr;
	client->remote_addr = tmpl->remote_addr;

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

qse_fprintf (QSE_STDERR, QSE_T("Debug: closing socket %d\n"), client->handle.i);

	if (client->status & CLIENT_HANDLE_IN_MUX)
	{
		httpd->cbs->mux.delhnd (httpd, httpd->mux, client->handle);
		client->status &= ~CLIENT_HANDLE_IN_MUX;
	}

	/* note that client.closed is not a counterpart to client.accepted. 
	 * so it is called even if client.close() failed. */
	if (httpd->cbs->client.closed)
		httpd->cbs->client.closed (httpd, client);
	
	httpd->cbs->client.close (httpd, client);

	qse_httpd_freemem (httpd, client);
}

static void purge_client (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	qse_httpd_client_t* prev;
	qse_httpd_client_t* next;
	qse_httpd_client_t* prev_tasked;
	qse_httpd_client_t* next_tasked;

	prev = client->prev;
	next = client->next;

	prev_tasked = client->prev_tasked;
	next_tasked = client->next_tasked;

	free_client (httpd, client);

	if (prev) prev->next = next;
	else httpd->client.list.head = next;
	if (next) next->prev = prev;
	else httpd->client.list.tail = prev;

	if (prev_tasked) prev_tasked->next_tasked = next_tasked;
	else httpd->client.tasked.head = next_tasked;
	if (next_tasked) next_tasked->prev_tasked = prev_tasked;
	else httpd->client.tasked.tail = prev_tasked;
}

static void purge_client_list (qse_httpd_t* httpd)
{
	while (httpd->client.list.tail)
		purge_client (httpd, httpd->client.list.tail);
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

		if (httpd->cbs->server.accept (httpd, server, &clibuf) <= -1) 
		{
/* TODO: proper logging */
qse_char_t tmp[128];
qse_nwadtostr (&server->nwad, tmp, QSE_COUNTOF(tmp), QSE_NWADTOSTR_ALL);
qse_printf (QSE_T("failed to accept from server %s\n"), tmp);

			return -1;
		}

/* TODO: check maximum number of client. if exceed call client.close */

		if (server->secure) clibuf.status |= CLIENT_SECURE;

		client = new_client (httpd, &clibuf);
		if (client == QSE_NULL)
		{
			httpd->cbs->client.close (httpd, &clibuf);
			return -1;
		}

		if (httpd->cbs->mux.addhnd (
			httpd, mux, client->handle, QSE_HTTPD_MUX_READ, 
			perform_client_task, client) <= -1)
		{
			free_client (httpd, client);
			return -1;
		}
		client->status |= CLIENT_HANDLE_READ_IN_MUX;

		/* link the new client to the back of the client list. */
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
qse_char_t tmp[128], tmp2[128];
qse_nwadtostr (&client->local_addr, tmp, QSE_COUNTOF(tmp), QSE_NWADTOSTR_ALL);
qse_nwadtostr (&client->remote_addr, tmp2, QSE_COUNTOF(tmp2), QSE_NWADTOSTR_ALL);
qse_printf (QSE_T("connection %d accepted %s from %s\n"), client->handle.i, tmp, tmp2);
}
	}
	return 0;
}

static void insert_client_to_tasked_list (
	qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	QSE_ASSERT (client->prev_tasked == QSE_NULL);
	QSE_ASSERT (client->next_tasked == QSE_NULL);

	if (httpd->client.tasked.tail)
	{
		QSE_ASSERT (httpd->client.tasked.head);
		client->prev_tasked = httpd->client.tasked.tail;
		httpd->client.tasked.tail->next_tasked = client;
		httpd->client.tasked.tail = client;
	}
	else
	{
		httpd->client.tasked.head = client;
		httpd->client.tasked.tail = client;
	}
}

static void delete_client_from_tasked_list (
	qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	qse_httpd_client_t* prev_tasked;
	qse_httpd_client_t* next_tasked;

	prev_tasked = client->prev_tasked;
	next_tasked = client->next_tasked;

	if (prev_tasked) prev_tasked->next_tasked = next_tasked;
	else httpd->client.tasked.head = next_tasked;
	if (next_tasked) next_tasked->prev_tasked = prev_tasked;
	else httpd->client.tasked.tail = prev_tasked;

	client->prev_tasked = QSE_NULL;
	client->next_tasked = QSE_NULL;
}

/* --------------------------------------------------- */

static void deactivate_servers (qse_httpd_t* httpd)
{
	qse_httpd_server_t* server;

	for (server = httpd->server.list; server; server = server->next)
	{
		if (server->active)
		{
			httpd->cbs->mux.delhnd (httpd, httpd->mux, server->handle);
			httpd->cbs->server.close (httpd, server);
			server->active = 0;
			httpd->server.nactive--;
		}
	}
}

static int activate_servers (qse_httpd_t* httpd)
{
	qse_httpd_server_t* server;

	for (server = httpd->server.list; server; server = server->next)
	{
		if (httpd->cbs->server.open (httpd, server) <= -1)
		{
qse_printf (QSE_T("FAILED TO ACTIVATE SERVER....\n"));
			continue;
		}

		if (httpd->cbs->mux.addhnd (
			httpd, httpd->mux, server->handle, QSE_HTTPD_MUX_READ, 
			accept_client, server) <= -1)
		{
qse_printf (QSE_T("FAILED TO ADD SERVER HANDLE TO MUX....\n"));
			httpd->cbs->server.close (httpd, server);
			continue;
		}

		server->active = 1;
		httpd->server.nactive++;
	}

	return 0;
}

static void free_server_list (qse_httpd_t* httpd, qse_httpd_server_t* server)
{
	while (server)
	{
		qse_httpd_server_t* next = server->next;

		httpd->cbs->server.close (httpd, server);
		qse_httpd_freemem (httpd, server);
		httpd->server.navail--;

		server = next;
	}
}

static qse_httpd_server_t* parse_server_uri (
	qse_httpd_t* httpd, const qse_char_t* uri)
{
	qse_httpd_server_t* server;
	qse_uint16_t default_port;
	qse_cstr_t tmp;

	server = qse_httpd_allocmem (httpd, QSE_SIZEOF(*server));
	if (server == QSE_NULL) goto oops; /* alloc set error number. */

	QSE_MEMSET (server, 0, QSE_SIZEOF(*server));

	/* check the protocol part */
	tmp.ptr = uri;
	while (*uri != QSE_T(':')) 
	{
		if (*uri == QSE_T('\0'))
		{
			httpd->errnum = QSE_HTTPD_EINVAL;
			goto oops;
		}
		uri++;
	}
	tmp.len = uri - tmp.ptr;
	if (qse_strxcmp (tmp.ptr, tmp.len, QSE_T("http")) == 0) 
	{
		server->secure = 0;
		default_port = DEFAULT_PORT;
	}
	else if (qse_strxcmp (tmp.ptr, tmp.len, QSE_T("https")) == 0) 
	{
		server->secure = 1;
		default_port = DEFAULT_SECURE_PORT;
	}
	else goto oops;
	
	uri++; /* skip : */ 
	if (*uri != QSE_T('/')) 
	{
		httpd->errnum = QSE_HTTPD_EINVAL;
		goto oops;
	}
	uri++; /* skip / */
	if (*uri != QSE_T('/')) 
	{
		httpd->errnum = QSE_HTTPD_EINVAL;
		goto oops;
	}
	uri++; /* skip / */

	if (qse_strtonwad (uri, &server->nwad) <= -1)
	{
		httpd->errnum = QSE_HTTPD_EINVAL;
		goto oops;
	}

	if (server->nwad.type == QSE_NWAD_IN4)
	{
		if (server->nwad.u.in4.port == 0) 
			server->nwad.u.in4.port = qse_hton16(default_port);
	}
	else if (server->nwad.type == QSE_NWAD_IN6)
	{
		if (server->nwad.u.in6.port == 0) 
			server->nwad.u.in6.port = qse_hton16(default_port);
	}

	return server;

oops:
	if (server) qse_httpd_freemem (httpd, server);
	return QSE_NULL;
}


int qse_httpd_addserver (qse_httpd_t* httpd, const qse_char_t* uri)
{
	qse_httpd_server_t* server;

	server = parse_server_uri (httpd, uri);
	if (server == QSE_NULL) return -1;

	server->next = httpd->server.list;
	httpd->server.list = server;
	httpd->server.navail++;
	
	return 0;
}

/* --------------------------------------------------- */

static int read_from_client (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	qse_mchar_t buf[2048]; /* TODO: adjust this buffer size */
	qse_ssize_t m;
	
	QSE_ASSERT (httpd->cbs->client.recv != QSE_NULL);

reread:
	httpd->errnum = QSE_HTTPD_ENOERR;
	m = httpd->cbs->client.recv (httpd, client, buf, QSE_SIZEOF(buf));
	if (m <= -1)
	{
		if (httpd->errnum == QSE_HTTPD_EAGAIN)
		{
			/* nothing to read yet. */
qse_fprintf (QSE_STDERR, QSE_T("Warning: Nothing to read from a client %d\n"), client->handle.i);
			return 0; /* return ok */
		}
		else if (httpd->errnum == QSE_HTTPD_EINTR)
		{
			goto reread;
		}
		else
		{
			/* TOOD: if (httpd->errnum == QSE_HTTPD_ENOERR) httpd->errnum = QSE_HTTPD_ECALLBACK; */
qse_fprintf (QSE_STDERR, QSE_T("Error: failed to read from a client %d\n"), client->handle.i);
	/* TODO: find a way to disconnect */
			return -1;
		}
	}
	else if (m == 0)
	{
		httpd->errnum = QSE_HTTPD_EDISCON;
qse_fprintf (QSE_STDERR, QSE_T("Debug: connection closed %d\n"), client->handle.i);
		return -1;
	}
	
	/* feed may have called the request callback multiple times... 
 	 * that's because we don't know how many valid requests
	 * are included in 'buf' */ 
qse_fprintf (QSE_STDERR, QSE_T("Debug: read from a client %d\n"), client->handle.i);
	
	httpd->errnum = QSE_HTTPD_ENOERR;
qse_printf (QSE_T("!!!!!FEEDING [%.*hs]\n"), (int)m, buf);
	if (qse_htrd_feed (client->htrd, buf, m) <= -1)
	{
		if (httpd->errnum == QSE_HTTPD_ENOERR)
		{
			if (client->htrd->errnum == QSE_HTRD_EBADRE || 
			    client->htrd->errnum == QSE_HTRD_EBADHDR)
				httpd->errnum = QSE_HTTPD_EBADREQ;
			else httpd->errnum = QSE_HTTPD_ENOMEM; /* TODO: better translate error code */
		}
	
qse_fprintf (QSE_STDERR, QSE_T("Error: http error while processing \n"));
		return -1;
	}

	return 0;
}

static int invoke_client_task (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_ubi_t handle, int mask)
{
	qse_httpd_task_t* task;
	int n;

/* TODO: handle comparison callback ... */
	if (handle.i == client->handle.i && (mask & QSE_HTTPD_MUX_READ))
	{
		if (read_from_client (httpd, client) <= -1) 
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
	if (task == QSE_NULL) return 0;

	task->trigger_mask &=
		~(QSE_HTTPD_TASK_TRIGGER_READABLE |
		  QSE_HTTPD_TASK_TRIGGER_RELAYABLE |
		  QSE_HTTPD_TASK_TRIGGER_WRITABLE);

qse_printf (QSE_T("handle.i %d mask %d\n"), handle.i, mask);
	if (task->trigger_mask & QSE_HTTPD_TASK_TRIGGER_READ)
	{
		if ((mask & QSE_HTTPD_MUX_READ) && task->trigger[0].i == handle.i)
			task->trigger_mask |= QSE_HTTPD_TASK_TRIGGER_READABLE;
	}
	if (task->trigger_mask & QSE_HTTPD_TASK_TRIGGER_RELAY)
	{
		if ((mask & QSE_HTTPD_MUX_READ) && task->trigger[1].i == handle.i)
			task->trigger_mask |= QSE_HTTPD_TASK_TRIGGER_RELAYABLE;
	}
	if (task->trigger_mask & QSE_HTTPD_TASK_TRIGGER_WRITE)
	{
		if ((mask & QSE_HTTPD_MUX_WRITE) && task->trigger[2].i == handle.i)
			task->trigger_mask |= QSE_HTTPD_TASK_TRIGGER_WRITABLE;
	}

	if (task->trigger_mask & (QSE_HTTPD_TASK_TRIGGER_READABLE | 
	                          QSE_HTTPD_TASK_TRIGGER_RELAYABLE | 
	                          QSE_HTTPD_TASK_TRIGGER_WRITABLE))
	{
		/* the task is invoked for triggers. 
		 * check if the client handle is writable */
		if (httpd->cbs->mux.writable (httpd, client->handle, 0) <= 0) 
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

		/* the current task is over. remove remove the task 
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
		}

		QSE_ASSERT (client->status & CLIENT_HANDLE_IN_MUX);

		httpd->cbs->mux.delhnd (httpd, httpd->mux, client->handle);
		client->status &= ~CLIENT_HANDLE_IN_MUX;

		if (httpd->cbs->mux.addhnd (
			httpd, httpd->mux, client->handle, 
			mux_mask, perform_client_task, client) <= -1) return -1;
		client->status |= mux_status;

		return 0;
	}
	else
	{
/* TODO: if there are no changes in the triggers, do do delhnd/addhnd() */
		static int mux_status[] = 
		{ 
			CLIENT_TASK_TRIGGER_READ_IN_MUX,
			CLIENT_TASK_TRIGGER_RELAY_IN_MUX,
			CLIENT_TASK_TRIGGER_WRITE_IN_MUX 
		};

		static int trigger_mask[] =
		{
			QSE_HTTPD_TASK_TRIGGER_READ,
			QSE_HTTPD_TASK_TRIGGER_RELAY,
			QSE_HTTPD_TASK_TRIGGER_WRITE
		};

		static int mux_mask[] =
		{
			QSE_HTTPD_MUX_READ,
			QSE_HTTPD_MUX_READ,
			QSE_HTTPD_MUX_WRITE
		};
	
		int i;

		for (i = 0; i < 3; i++)
		{
			if (client->status & mux_status[i])
			{
				httpd->cbs->mux.delhnd (httpd, httpd->mux, task->trigger[i]);
				client->status &= ~mux_status[i];
			}
			if (task->trigger_mask & trigger_mask[i])
			{
				if (client->handle.i != task->trigger[i].i)
				{
					if (httpd->cbs->mux.addhnd (
						httpd, httpd->mux, task->trigger[i],
						mux_mask[i], perform_client_task, client) <= -1) return -1;
					client->status |= mux_status[i];
				}
			}
		}

		QSE_ASSERT (client->status & CLIENT_HANDLE_READ_IN_MUX);

		if ((client->status & CLIENT_TASK_TRIGGER_IN_MUX) &&
		    (client->status & CLIENT_HANDLE_WRITE_IN_MUX))
		{
			httpd->cbs->mux.delhnd (httpd, httpd->mux, client->handle);
			client->status &= ~CLIENT_HANDLE_IN_MUX;

			if (httpd->cbs->mux.addhnd (
				httpd, httpd->mux, client->handle, 
				QSE_HTTPD_MUX_READ, perform_client_task, client) <= -1) return -1;
			client->status |= CLIENT_HANDLE_READ_IN_MUX;
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
		x = httpd->cbs->client.accepted (httpd, client);
		if (x <= -1) goto oops;
		if (x >= 1) client->status |= CLIENT_READY;
	}
	else
	{
		if (invoke_client_task (httpd, client, handle, mask) <= -1) goto oops;
	}

	return 0;

oops:
qse_printf (QSE_T("MARKING BAD XXXXXXXXXXXXXX\n"));
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

qse_httpd_task_t* qse_httpd_entask (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_httpd_task_t* pred, const qse_httpd_task_t* task,
	qse_size_t xtnsize)
{
	qse_httpd_task_t* new_task;

	new_task = enqueue_task (httpd, client, pred, task, xtnsize);
	if (new_task == QSE_NULL) purge_client (httpd, client);
	else if (new_task->prev == QSE_NULL)
	{
		/* this new task is the first task for a client */
		/*insert_client_to_tasked_list (httpd, client);*/

		/* arrange to invokde this task so long as 
		 * the client-side handle is writable. */
		QSE_ASSERT (client->status & CLIENT_HANDLE_IN_MUX);
		httpd->cbs->mux.delhnd (httpd, httpd->mux, client->handle);
		client->status &= ~CLIENT_HANDLE_IN_MUX;

		if (httpd->cbs->mux.addhnd (
			httpd, httpd->mux, client->handle, 
			QSE_HTTPD_MUX_READ | QSE_HTTPD_MUX_WRITE, 
			perform_client_task, client) <= -1)
		{
			purge_client (httpd, client);
			new_task = QSE_NULL;
		}
		client->status |= CLIENT_HANDLE_IN_MUX; /* READ | WRITE */
	}

	return new_task;
}

int qse_httpd_loop (qse_httpd_t* httpd, qse_httpd_cbs_t* cbs, qse_ntime_t timeout)
{
	httpd->stopreq = 0;
	httpd->cbs = cbs;

	QSE_ASSERTX (httpd->server.list != QSE_NULL,
		"Add listeners before calling qse_httpd_loop()");	

	QSE_ASSERTX (httpd->client.list.head == QSE_NULL,
		"No client should exist when this loop is started");

	QSE_ASSERTX (httpd->cbs != QSE_NULL,
		"Set httpd callbacks before calling qse_httpd_loop()");	

	if (httpd->server.list == QSE_NULL)
	{
		/* no listener specified */
		httpd->errnum = QSE_HTTPD_EINVAL;
		return -1;
	}

	QSE_ASSERT (httpd->server.navail > 0);

	httpd->mux = httpd->cbs->mux.open (httpd);
	if (httpd->mux == QSE_NULL)
	{
qse_printf (QSE_T("can't open mux....\n"));
		return -1;
	}

	if (activate_servers (httpd) <= -1) 
	{
		httpd->cbs->mux.close (httpd, httpd->mux);
		return -1;
	}
	if (httpd->server.nactive <= 0)
	{
qse_printf (QSE_T("no servers are active....\n"));
		httpd->cbs->mux.close (httpd, httpd->mux);
		return -1;
	}

	while (!httpd->stopreq)
	{
		int count;

		count = httpd->cbs->mux.poll (httpd, httpd->mux, timeout);
		if (count <= -1)
		{
			httpd->errnum = QSE_HTTPD_EIOMUX;
/* TODO: call user callback for this multiplexer error */
			/*if (errno == EINTR) continue;*/
qse_fprintf (QSE_STDERR, QSE_T("Error: mux returned failure\n"));
			/* break; */
		}

		purge_bad_clients (httpd);
	}

	purge_client_list (httpd);
	deactivate_servers (httpd);
	httpd->cbs->mux.close (httpd, httpd->mux);
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

#endif
