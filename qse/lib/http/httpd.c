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


#if !defined(QSE_HTTPD_DEFAULT_MODPREFIX)
#	if defined(_WIN32)
#		define QSE_HTTPD_DEFAULT_MODPREFIX "qsehttpd-"
#	elif defined(__OS2__)
#		define QSE_HTTPD_DEFAULT_MODPREFIX "htd-"
#	elif defined(__DOS__)
#		define QSE_HTTPD_DEFAULT_MODPREFIX "htd-"
#	else
#		define QSE_HTTPD_DEFAULT_MODPREFIX "libqsehttpd-"
#	endif
#endif

#if !defined(QSE_HTTPD_DEFAULT_MODPOSTFIX)
#	define QSE_HTTPD_DEFAULT_MODPOSTFIX ""
#endif

typedef struct htrd_xtn_t htrd_xtn_t;
typedef struct tmr_xtn_t tmr_xtn_t;

struct htrd_xtn_t
{
	qse_httpd_t*        httpd;
	qse_httpd_client_t* client;
};

struct tmr_xtn_t
{
	qse_httpd_t* httpd;
};

static void free_server_list (qse_httpd_t* httpd);
static int perform_client_task (
	qse_httpd_t* httpd, void* mux, qse_httpd_hnd_t handle, int mask, void* cbarg);
static void unload_all_modules (qse_httpd_t* httpd);

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
	qse_httpd_fini (httpd);
	QSE_MMGR_FREE (httpd->mmgr, httpd);
}

int qse_httpd_init (qse_httpd_t* httpd, qse_mmgr_t* mmgr)
{
	tmr_xtn_t* tmr_xtn;

	QSE_MEMSET (httpd, 0, QSE_SIZEOF(*httpd));

	httpd->mmgr = mmgr;
	httpd->tmr = qse_tmr_open (mmgr, QSE_SIZEOF(tmr_xtn_t), 2048);
	if (httpd->tmr == QSE_NULL) return -1;

	tmr_xtn = qse_tmr_getxtn (httpd->tmr);
	QSE_MEMSET (tmr_xtn, 0, QSE_SIZEOF(*tmr_xtn));
	tmr_xtn->httpd = httpd;

	qse_mbscpy (httpd->sname, QSE_MT("QSE-HTTPD " QSE_PACKAGE_VERSION));

	httpd->opt.tmout.sec = 3;
	httpd->opt.idle_limit.sec = 30;

	return 0;
}

void qse_httpd_fini (qse_httpd_t* httpd)
{
	qse_httpd_ecb_t* ecb;

	unload_all_modules (httpd);

	for (ecb = httpd->ecb; ecb; ecb = ecb->next)
		if (ecb->close) ecb->close (httpd);

	free_server_list (httpd);
	qse_tmr_close (httpd->tmr);
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

static int dup_str_opt (qse_httpd_t* httpd, const void* value, qse_cstr_t* tmp)
{
	if (value)
	{
		tmp->ptr = qse_strdup (value, httpd->mmgr);
		if (tmp->ptr == QSE_NULL)
		{
			qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOMEM);
			return -1;
		}
		tmp->len = qse_strlen (tmp->ptr);
	}
	else
	{
		tmp->ptr = QSE_NULL;
		tmp->len = 0;
	}

	return 0;
}

int qse_httpd_getopt (qse_httpd_t* httpd, qse_httpd_opt_t id, void* value)
{
	switch (id)
	{
		case QSE_HTTPD_TRAIT:
			*(int*)value = httpd->opt.trait;
			return 0;

		case QSE_HTTPD_TMOUT:
			*(qse_ntime_t*)value = httpd->opt.tmout;
			return 0;

		case QSE_HTTPD_IDLELIMIT:
			*(qse_ntime_t*)value = httpd->opt.idle_limit;
			return 0;

		case QSE_HTTPD_MODPREFIX:
		case QSE_HTTPD_MODPOSTFIX:
			*(const qse_char_t**)value = httpd->opt.mod[id - QSE_HTTPD_MODPREFIX].ptr;
			return 0;

		case QSE_HTTPD_SCB:
			*(qse_httpd_scb_t*)value = httpd->opt.scb;
			return 0;

		case QSE_HTTPD_RCB:
			*(qse_httpd_rcb_t*)value = httpd->opt.rcb;
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

		case QSE_HTTPD_TMOUT:
			httpd->opt.tmout = *(qse_ntime_t*)value;
			return 0;

		case QSE_HTTPD_IDLELIMIT:
			httpd->opt.idle_limit = *(qse_ntime_t*)value;
			return 0;

		case QSE_HTTPD_MODPREFIX:
		case QSE_HTTPD_MODPOSTFIX:
		{
			qse_cstr_t tmp;
			int idx;

			if (dup_str_opt (httpd, value, &tmp) <= -1) return -1;

			idx = id - QSE_HTTPD_MODPREFIX;
			if (httpd->opt.mod[idx].ptr) QSE_MMGR_FREE (httpd->mmgr, httpd->opt.mod[idx].ptr);

			httpd->opt.mod[idx] = tmp;
			return 0;
		}

		case QSE_HTTPD_SCB:
			httpd->opt.scb = *(qse_httpd_scb_t*)value;
			return 0;

		case QSE_HTTPD_RCB:
			httpd->opt.rcb = *(qse_httpd_rcb_t*)value;
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

qse_mchar_t* qse_httpd_mbsdup (qse_httpd_t* httpd, const qse_mchar_t* str)
{
	qse_mchar_t* mptr;

	mptr = qse_mbsdup (str, httpd->mmgr);
	if (mptr == QSE_NULL) httpd->errnum = QSE_HTTPD_ENOMEM;

	return mptr;
}

qse_mchar_t* qse_httpd_mbsxdup (qse_httpd_t* httpd, const qse_mchar_t* str, qse_size_t len)
{
	qse_mchar_t* mptr;

	mptr = qse_mbsxdup (str, len, httpd->mmgr);
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

static QSE_INLINE int dequeue_task (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	qse_httpd_real_task_t* task;
	qse_size_t i;

	if (client->task.count <= 0) return -1;

	task = (qse_httpd_real_task_t*)client->task.head;

	/* clear task triggers from mux if they are registered */
	for (i = 0; i < QSE_COUNTOF(task->core.trigger.v); i++)
	{
		if (client->status & QSE_HTTPD_CLIENT_TASK_TRIGGER_RW_IN_MUX(i))
		{
			QSE_ASSERT (task->core.trigger.v[i].handle != client->handle);
			httpd->opt.scb.mux.delhnd (httpd, httpd->mux, task->core.trigger.v[i].handle);
			client->status &= ~QSE_HTTPD_CLIENT_TASK_TRIGGER_RW_IN_MUX(i);
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

static QSE_INLINE void purge_tasks (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	while (dequeue_task (httpd, client) == 0);
}

/* ----------------------------------------------------------------------- */

static QSE_INLINE void unchain_cached_proxy_peer (qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_peer_t* peer)
{
	QSE_ASSERT (peer->flags & QSE_HTTPD_PEER_CACHED);

	if (peer->next) peer->next->prev = peer->prev;
	else client->peer.last = peer->prev;

	if (peer->prev) peer->prev->next = peer->next;
	else client->peer.first = peer->next;
}

static void purge_cached_proxy_peer (qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_peer_t* peer)
{
	unchain_cached_proxy_peer (httpd, client, peer);

#if defined(QSE_HTTPD_DEBUG)
	{
		qse_mchar_t tmp[128];

		qse_nwadtombs (&peer->nwad, tmp, QSE_COUNTOF(tmp), QSE_NWADTOMBS_ALL);
		HTTPD_DBGOUT2 ("Closing cached peer [%hs] - %zd\n", tmp, (qse_size_t)peer->handle);
	}
#endif

	httpd->opt.scb.peer.close (httpd, peer);
	qse_httpd_freemem (httpd, peer);
}

static void purge_cached_proxy_peers (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	while (client->peer.first)
	{
		purge_cached_proxy_peer (httpd, client, client->peer.first);
	}
}

qse_httpd_peer_t* qse_httpd_cacheproxypeer (qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_peer_t* tmpl)
{
	qse_httpd_peer_t* peer;

	if (tmpl->flags & QSE_HTTPD_PEER_CACHED)
	{
		/* If QSE_HTTPD_PEER_CACHED is set, tmpl points to a block allocated
		 * here previously. Link such a block to the cache list */
		peer = tmpl;
	}
	else
	{
		/* Clone the peer object if it's not previsouly allocated here */
		peer = qse_httpd_allocmem (httpd, QSE_SIZEOF(*peer));
		if (peer == QSE_NULL) goto oops;

		QSE_MEMCPY (peer, tmpl, QSE_SIZEOF(*peer));
		peer->flags |= QSE_HTTPD_PEER_CACHED;
	}

	/* place the peer at the back of the peer list of a client */
	if (client->peer.last)
	{
		peer->next = QSE_NULL;
		peer->prev = client->peer.last;
		client->peer.last->next = peer;
		client->peer.last = peer;
	}
	else
	{
		peer->next = QSE_NULL;
		peer->prev = QSE_NULL;
		client->peer.first = peer;
		client->peer.last = peer;
	}

	qse_gettime (&peer->timestamp);
	return peer;

oops:
	if (peer) qse_httpd_freemem (httpd, peer);
	return QSE_NULL;
}

qse_httpd_peer_t* qse_httpd_decacheproxypeer (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	const qse_nwad_t* nwad, const qse_nwad_t* local, int secure)
{
	qse_httpd_peer_t* peer, * next;
	qse_ntime_t now, diff;
	static qse_ntime_t diff_limit = { 5, 0 }; /* TODO: make this configurable */

	qse_gettime (&now);

	peer = client->peer.first;
	while (peer)
	{
		next = peer->next;

		qse_subtime (&now, &peer->timestamp, &diff);
		if (qse_cmptime(&diff, &diff_limit) >= 0)
		{
			/* the entry is too old */
			purge_cached_proxy_peer (httpd, client, peer);
		}
		else if (qse_nwadequal (nwad, &peer->nwad) && qse_nwadequal (local, &peer->local))
		{
			if ((secure && (peer->flags & QSE_HTTPD_PEER_SECURE)) ||
			    (!secure && !(peer->flags & QSE_HTTPD_PEER_SECURE))) 
			{
				unchain_cached_proxy_peer (httpd, client, peer);
				peer->next = QSE_NULL;
				peer->prev = QSE_NULL;
				return peer;
			}
		}

		peer = next;
	}

	return QSE_NULL;
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
static void tmr_idle_update (qse_tmr_t* tmr, qse_tmr_index_t old_index, qse_tmr_index_t new_index, qse_tmr_event_t* evt);
static void tmr_idle_handle (qse_tmr_t* tmr, const qse_ntime_t* now, qse_tmr_event_t* evt);

static void mark_bad_client (qse_httpd_client_t* client)
{
	/* you can call this function multiple times */
	if (!(client->status & QSE_HTTPD_CLIENT_BAD))
	{
		client->status |= QSE_HTTPD_CLIENT_BAD;
		client->bad_next = client->server->httpd->client.bad;
		client->server->httpd->client.bad = client;
	}
}

static qse_httpd_client_t* new_client (qse_httpd_t* httpd, qse_httpd_client_t* tmpl)
{
	qse_httpd_client_t* client = QSE_NULL;
	qse_tmr_event_t idle_event;
	htrd_xtn_t* xtn;

	client = qse_httpd_allocmem (httpd, QSE_SIZEOF(*client));
	if (client == QSE_NULL) goto oops;

	QSE_MEMSET (client, 0, QSE_SIZEOF(*client));
	client->tmr_idle = QSE_TMR_INVALID_INDEX;

	client->type = QSE_HTTPD_CLIENT;
	client->htrd = qse_htrd_open (httpd->mmgr, QSE_SIZEOF(*xtn));
	if (client->htrd == QSE_NULL) 
	{
		httpd->errnum = QSE_HTTPD_ENOMEM;
		goto oops;
	}

	if (httpd->opt.idle_limit.sec > 0 || 
	    (httpd->opt.idle_limit.sec == 0 && httpd->opt.idle_limit.nsec > 0))
	{
		/* idle limit is enabled when the limit is greater than 0.0 */
		QSE_MEMSET (&idle_event, 0, QSE_SIZEOF(idle_event));
		qse_gettime (&idle_event.when);
		qse_addtime (&idle_event.when, &httpd->opt.idle_limit, &idle_event.when);
		idle_event.ctx = client;
		idle_event.handler = tmr_idle_handle;
		idle_event.updater = tmr_idle_update;

		if (qse_httpd_insert_timer_event (httpd, &idle_event, &client->tmr_idle) <= -1) goto oops;
	}

	qse_htrd_setoption (client->htrd, QSE_HTRD_REQUEST | QSE_HTRD_TRAILERS | QSE_HTRD_CANONQPATH);

	/* copy the public fields, 
	 * keep the private fields initialized at 0 */
	client->status = tmpl->status;
	if (httpd->opt.scb.client.accepted == QSE_NULL) 
		client->status |= QSE_HTTPD_CLIENT_READY;
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

oops:
	if (client) 
	{
		if (client->tmr_idle != QSE_TMR_INVALID_INDEX) 
		{
			qse_httpd_remove_timer_event (httpd, client->tmr_idle);
			client->tmr_idle = QSE_TMR_INVALID_INDEX;
		}
		if (client->htrd) qse_htrd_close (client->htrd);
		qse_httpd_freemem (httpd, client);
	}

	return QSE_NULL;
}

static void free_client (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	QSE_ASSERT (client->htrd != QSE_NULL);

	purge_tasks (httpd, client);
	purge_cached_proxy_peers (httpd, client);

	qse_htrd_close (client->htrd);

#if defined(QSE_HTTPD_DEBUG)
	{
		qse_mchar_t tmp[128];
		qse_nwadtombs (&client->remote_addr, tmp, QSE_COUNTOF(tmp), QSE_NWADTOMBS_ALL);
		HTTPD_DBGOUT2 ("Closing client [%hs] - %zd\n", tmp, (qse_size_t)client->handle);
	}
#endif

	if (client->status & QSE_HTTPD_CLIENT_HANDLE_RW_IN_MUX)
	{
		httpd->opt.scb.mux.delhnd (httpd, httpd->mux, client->handle);
		client->status &= ~QSE_HTTPD_CLIENT_HANDLE_RW_IN_MUX;
	}

	if (client->tmr_idle != QSE_TMR_INVALID_INDEX)
	{
		qse_httpd_remove_timer_event (httpd, client->tmr_idle);
		client->tmr_idle = QSE_TMR_INVALID_INDEX;
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

#if defined(QSE_HTTPD_DEBUG)
	{
		qse_mchar_t tmp[128];
		qse_nwadtombs (&client->remote_addr, tmp, QSE_COUNTOF(tmp), QSE_NWADTOMBS_ALL);
		HTTPD_DBGOUT2 ("Purging client [%hs] - %zd\n", tmp, (qse_size_t)client->handle);
	}
#endif

	free_client (httpd, client);

	if (prev) prev->next = next;
	else httpd->client.list.head = next;
	if (next) next->prev = prev;
	else httpd->client.list.tail = prev;

	httpd->client.list.count--;
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

#if 0
static int is_client_allowed (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	qse_httpd_mod_t* mod;

/* TODO: no sequential search. speed up */
	for (mod = httpd->modlist; mod;  mod = mod->next)
	{
		if (mod->new_client) mod->new_client (httpd, client);
	}

	return 0;
}
#endif

static int accept_client (
	qse_httpd_t* httpd, void* mux, qse_httpd_hnd_t handle, int mask, void* cbarg)
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
		#if QSE_HTTPD_DEBUG
			qse_mchar_t tmp[128];
			qse_nwadtombs (&server->dope.nwad, tmp, QSE_COUNTOF(tmp), QSE_NWADTOMBS_ALL);
			HTTPD_DBGOUT2 ("Failed to accept from server [%hs] [%d]\n", tmp, (int)server->handle);
		#endif
			return -1;
		}

/* TODO: check maximum number of client. if exceed call client.close */

		if (server->dope.flags & QSE_HTTPD_SERVER_SECURE) clibuf.status |= QSE_HTTPD_CLIENT_SECURE;
		clibuf.server = server;

#if 0
		if (is_client_allowed (httpd, &clibuf) <= -1)
		{
			httpd->opt.scb.client.close (httpd, &clibuf);
			return -1;
		}
#endif

		client = new_client (httpd, &clibuf);
		if (client == QSE_NULL)
		{
			httpd->opt.scb.client.close (httpd, &clibuf);
			return -1;
		}

		if (httpd->opt.scb.mux.addhnd (httpd, mux, client->handle, QSE_HTTPD_MUX_READ, client) <= -1)
		{
			free_client (httpd, client);
			return -1;
		}
		client->status |= QSE_HTTPD_CLIENT_HANDLE_READ_IN_MUX;

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
		httpd->client.list.count++;

	#if defined(QSE_HTTPD_DEBUG)
		{
			qse_mchar_t tmp1[128], tmp2[128], tmp3[128];
			qse_nwadtombs (&client->local_addr, tmp1, QSE_COUNTOF(tmp1), QSE_NWADTOMBS_ALL);
			qse_nwadtombs (&client->orgdst_addr, tmp2, QSE_COUNTOF(tmp2), QSE_NWADTOMBS_ALL);
			qse_nwadtombs (&client->remote_addr, tmp3, QSE_COUNTOF(tmp3), QSE_NWADTOMBS_ALL);
			HTTPD_DBGOUT3 ("Accepted client %hs(%hs) from %hs\n", tmp1, tmp2, tmp3);
		}
	#endif
	}

	return 0;
}

static void tmr_idle_update (qse_tmr_t* tmr, qse_tmr_index_t old_index, qse_tmr_index_t new_index, qse_tmr_event_t* evt)
{
	qse_httpd_client_t* client = (qse_httpd_client_t*)evt->ctx;
	QSE_ASSERT (client->tmr_idle == old_index);
	client->tmr_idle = new_index;
}

static void tmr_idle_handle (qse_tmr_t* tmr, const qse_ntime_t* now, qse_tmr_event_t* evt)
{
	qse_httpd_client_t* client = (qse_httpd_client_t*)evt->ctx;

	if (qse_cmptime(now, &client->last_active) >= 0)
	{
		qse_ntime_t diff;
		qse_subtime (now, &client->last_active, &diff);
		if (qse_cmptime(&diff, &client->server->httpd->opt.idle_limit) >= 0)
		{
			/* this client is idle */
			HTTPD_DBGOUT1 ("Purging idle client %zd\n", (qse_size_t)client->handle);
			purge_client (client->server->httpd, client);
		}
		else
		{
			qse_tmr_event_t idle_event;

			QSE_ASSERT (client->server->httpd->tmr == tmr);

			/*qse_gettime (&idle_event.when);*/
			QSE_MEMSET (&idle_event, 0, QSE_SIZEOF(idle_event));
			idle_event.when = *now;
			qse_addtime (&idle_event.when, &client->server->httpd->opt.idle_limit, &idle_event.when);
			idle_event.ctx = client;
			idle_event.handler = tmr_idle_handle;
			idle_event.updater = tmr_idle_update;

			/* the timer must have been deleted when this callback is called. */
			QSE_ASSERT (client->tmr_idle == QSE_TMR_INVALID_INDEX); 
			if (qse_httpd_insert_timer_event (client->server->httpd, &idle_event, &client->tmr_idle) <= -1)
			{
				HTTPD_DBGOUT1 ("Cannot update idle timer for client %d. Marking it bad", (int)client->handle);
				mark_bad_client (client);
			}
		}
	}
}

/* ----------------------------------------------------------------------- */

static void deactivate_servers (qse_httpd_t* httpd)
{
	qse_httpd_server_t* server;

	for (server = httpd->server.list.head; server; server = server->next)
	{
		if (server->dope.flags & QSE_HTTPD_SERVER_ACTIVE)
		{
		#if defined(QSE_HTTPD_DEBUG)
			{
				qse_mchar_t tmp[128];
				qse_nwadtombs (&server->dope.nwad, tmp, QSE_COUNTOF(tmp), QSE_NWADTOMBS_ALL);
				HTTPD_DBGOUT2 ("Closing server [%hs] %zd to mux\n", tmp, (qse_size_t)server->handle);
			}
		#endif

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
		QSE_ASSERT (server->httpd == httpd);

		if (httpd->opt.scb.server.open (httpd, server) <= -1)
		{
		#if defined(QSE_HTTPD_DEBUG)
			{
				qse_mchar_t tmp[128];
				qse_nwadtombs (&server->dope.nwad, tmp, QSE_COUNTOF(tmp), QSE_NWADTOMBS_ALL);
				HTTPD_DBGOUT1 ("Cannot open server [%hs]\n", tmp);
			}
		#endif

			continue;
		}
		else
		{
		#if defined(QSE_HTTPD_DEBUG)
			{
				qse_mchar_t tmp[128];
				qse_nwadtombs (&server->dope.nwad, tmp, QSE_COUNTOF(tmp), QSE_NWADTOMBS_ALL);
				HTTPD_DBGOUT2 ("Opened server [%hs] - %zd\n", tmp, (qse_size_t)server->handle);
			}
		#endif
		}

		if (httpd->opt.scb.mux.addhnd (
			httpd, httpd->mux, server->handle, QSE_HTTPD_MUX_READ, server) <= -1)
		{
		#if defined(QSE_HTTPD_DEBUG)
			{
				qse_mchar_t tmp[128];
				qse_nwadtombs (&server->dope.nwad, tmp, QSE_COUNTOF(tmp), QSE_NWADTOMBS_ALL);
				HTTPD_DBGOUT2 ("Cannot add server [%hs] %zd to mux. Closing\n", tmp, (qse_size_t)server->handle);
			}
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

	server->httpd = httpd;
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


int qse_httpd_addhnd (qse_httpd_t* httpd, qse_httpd_hnd_t handle, int mask, qse_httpd_custom_t* mate)
{
	/* qse_httpd_loop() opens the multiplexer. you can call this function from 
	 * preloop/postloop hooks only. but calling it from postloop hooks is
	 * useless. */
	return httpd->opt.scb.mux.addhnd (httpd, httpd->mux, handle, mask, mate);
}

int qse_httpd_delhnd (qse_httpd_t* httpd, qse_httpd_hnd_t handle)
{
	return httpd->opt.scb.mux.delhnd (httpd, httpd->mux, handle);
}

/* ----------------------------------------------------------------------- */

static int activate_dns (qse_httpd_t* httpd)
{
	int i;

	QSE_MEMSET (&httpd->dns, 0, QSE_SIZEOF(httpd->dns));

	if (!httpd->opt.scb.dns.open) 
	{
		httpd->errnum = QSE_HTTPD_ENOIMPL;
		return -1;
	}

	if (httpd->opt.scb.dns.open (httpd, &httpd->dns) <= -1) return -1;

	httpd->dns.type = QSE_HTTPD_DNS;

	for (i = 0; i < httpd->dns.handle_count; i++)
	{
		if (httpd->dns.handle_mask & (1 << i))
		{
			if (httpd->opt.scb.mux.addhnd (httpd, httpd->mux, httpd->dns.handle[i], QSE_HTTPD_MUX_READ, &httpd->dns) <= -1) 
			{
				while (i > 0)
				{
					--i;
					if (httpd->dns.handle_mask & (1 << i))
						httpd->opt.scb.mux.delhnd (httpd, httpd->mux, httpd->dns.handle[i]);
				}
				httpd->opt.scb.dns.close (httpd, &httpd->dns);
				return -1;
			}
		}
	}

	return 0;
}

static void deactivate_dns (qse_httpd_t* httpd)
{
	int i;

	for (i = 0; i < httpd->dns.handle_count; i++)
	{
		if (httpd->dns.handle_mask & (1 << i))
			httpd->opt.scb.mux.delhnd (httpd, httpd->mux, httpd->dns.handle[i]);
	}

	httpd->opt.scb.dns.close (httpd, &httpd->dns);
}
/* ----------------------------------------------------------------------- */

static int activate_urs (qse_httpd_t* httpd)
{
	int i;

	QSE_MEMSET (&httpd->urs, 0, QSE_SIZEOF(httpd->urs));

	if (!httpd->opt.scb.urs.open) 
	{
		httpd->errnum = QSE_HTTPD_ENOIMPL;
		return -1;
	}

	if (httpd->opt.scb.urs.open (httpd, &httpd->urs) <= -1) return -1;

	httpd->urs.type = QSE_HTTPD_URS;

	for (i = 0; i < httpd->urs.handle_count; i++)
	{
		if (httpd->urs.handle_mask & (1 << i))
		{
			if (httpd->opt.scb.mux.addhnd (httpd, httpd->mux, httpd->urs.handle[i], QSE_HTTPD_MUX_READ, &httpd->urs) <= -1) 
			{
				while (i > 0) 
				{
					--i;
					if (httpd->urs.handle_mask & (1 << i))
						httpd->opt.scb.mux.delhnd (httpd, httpd->mux, httpd->urs.handle[i]);
				}
				httpd->opt.scb.urs.close (httpd, &httpd->urs);
				return -1;
			}
		}
	}

	return 0;
}

static void deactivate_urs (qse_httpd_t* httpd)
{
	int i;

	for (i = 0; i < httpd->urs.handle_count; i++)
	{
		if (httpd->urs.handle_mask & (1 << i))
			httpd->opt.scb.mux.delhnd (httpd, httpd->mux, httpd->urs.handle[i]);
	}

	httpd->opt.scb.urs.close (httpd, &httpd->urs);
}

/* ----------------------------------------------------------------------- */

static int read_from_client (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	qse_mchar_t buf[MAX_RECV_SIZE]; /* TODO: adjust this buffer size */
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
qse_printf (QSE_T("Debug: connection closed %d\n"), client->handle);
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
			client->status |= QSE_HTTPD_CLIENT_MUTE;
#if 0
qse_printf (QSE_T(">>>>> Marking client %d as MUTE\n"), client->handle);
#endif
			return 0;
		}
		else
		{
#if 0
qse_printf (QSE_T(">>>>> Returning failure for client %d\n"), client->handle);
#endif
			httpd->errnum = QSE_HTTPD_EDISCON;
			return -1;
		}
	}
	
#if 0
qse_printf (QSE_T("!!!!!FEEDING %d from %d ["), (int)m, (int)client->handle);
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
qse_printf (QSE_T("Error: http error while processing %d ["), (int)client->handle);
{
int i;
for (i = 0; i < m; i++) qse_printf (QSE_T("%hc"), buf[i]);
}
qse_printf (QSE_T("]\n"));
#endif
		return -1;
	}


#if 0
qse_printf (QSE_T("!!!!!FEEDING OK OK OK OK %d from %d\n"), (int)m, (int)client->handle);
#endif

	if (client->status & QSE_HTTPD_CLIENT_PENDING) 
	{
		/* this QSE_HTTPD_CLIENT_PENDING thing is a dirty hack for SSL.
		 * In SSL, data is transmitted in a record. a record can be
		 * as large as 16K bytes since its length field is 2 bytes.
		 * If SSL_read() has a record but it's given a smaller buffer
		 * than the actual record, the next call to select() won't return. 
		 * there is no data to read at the socket layer. SSL_pending() can 
		 * tell you the amount of data in the SSL buffer. I try to consume
		 * the pending data if the client.recv handler has set QSE_HTTPD_CLIENT_PENDING.
		 *
		 * TODO: Investigate if there is any starvation issues.
		 *       What if a single client never stops sending? 
		 */
		goto reread;
	}

	return 0;
}

static void clear_trigger_mask_result (qse_httpd_task_t* task)
{
	qse_size_t i;

	task->trigger.cmask &=  ~(QSE_HTTPD_TASK_TRIGGER_READABLE | 
	                          QSE_HTTPD_TASK_TRIGGER_WRITABLE);
	for (i = 0; i < QSE_COUNTOF(task->trigger.v); i++)
	{
		task->trigger.v[i].mask &= ~(QSE_HTTPD_TASK_TRIGGER_READABLE | 
		                             QSE_HTTPD_TASK_TRIGGER_WRITABLE);
	}
}

static int update_mux_for_current_task (qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	qse_size_t i;

	/* the code here is pretty fragile. there is a high chance
	 * that something can go wrong if the task handler plays
	 * with the trigger field in an unexpected manner. 
	 */

	clear_trigger_mask_result (task);

/*printf ("update_mux_for_current_task..............\n");*/
	if (QSE_MEMCMP (&client->trigger, &task->trigger, QSE_SIZEOF(client->trigger)) != 0 ||
	    ((client->status & QSE_HTTPD_CLIENT_MUTE) && !(client->status & QSE_HTTPD_CLIENT_MUTE_DELETED)))
	{
		/* manipulate muxtiplexer settings if there are trigger changes */

		int has_trigger = 0;
		int expected_client_handle_mux_mask;
		int expected_client_handle_mux_status;

		if ((client->trigger.flags & QSE_HTTPD_TASK_TRIGGER_INACTIVE) != 
		    (task->trigger.flags & QSE_HTTPD_TASK_TRIGGER_INACTIVE))
		{
			if (task->trigger.flags & QSE_HTTPD_TASK_TRIGGER_INACTIVE)
			{
				/* active to inactive */

/*printf ("ACTIVE TO INACTIVE....\n");*/
				for (i = 0; i < QSE_COUNTOF(task->trigger.v); i++)
				{
					if (client->status & QSE_HTTPD_CLIENT_TASK_TRIGGER_RW_IN_MUX(i))
					{
						httpd->opt.scb.mux.delhnd (httpd, httpd->mux, client->trigger.v[i].handle);
						client->status &= ~QSE_HTTPD_CLIENT_TASK_TRIGGER_RW_IN_MUX(i);
					}
				}

				if (client->status & QSE_HTTPD_CLIENT_HANDLE_RW_IN_MUX)
				{
					httpd->opt.scb.mux.delhnd (httpd, httpd->mux, client->handle);
					client->status &= ~QSE_HTTPD_CLIENT_HANDLE_RW_IN_MUX;
				}

				/* save the task trigger information */
				client->trigger = task->trigger;
				return 0;
			}

/*printf ("INACTIVE TO ACTIVE....\n");*/
			/* inactive to active . go on*/
		}
		else 
		{
			if (task->trigger.flags & QSE_HTTPD_TASK_TRIGGER_INACTIVE)
			{
/*printf ("INACTIVE TO INACTIVE....\n");*/
				/* inactive to inactive.
				 * save the trigger as the trigger handle and masks could change */
				client->trigger = task->trigger;
				return 0;
			}

/*printf ("ACTIVE TO ACTIVE....\n");*/
			/* active to active. go on */
		}

		/* delete previous trigger handles */
		for (i = 0; i < QSE_COUNTOF(task->trigger.v); i++)
		{
			if (client->status & QSE_HTTPD_CLIENT_TASK_TRIGGER_RW_IN_MUX(i))
			{
				httpd->opt.scb.mux.delhnd (httpd, httpd->mux, client->trigger.v[i].handle);
				client->status &= ~QSE_HTTPD_CLIENT_TASK_TRIGGER_RW_IN_MUX(i);
			}
		}

		/* add new trigger handles */
		for (i = 0; i < QSE_COUNTOF(task->trigger.v); i++)
		{
			int expected_trigger_mux_mask = 0;
			int expected_trigger_mux_status = 0;

			if (task->trigger.v[i].handle == client->handle) continue; 

			if (task->trigger.v[i].mask & QSE_HTTPD_TASK_TRIGGER_READ) 
			{
				expected_trigger_mux_mask |= QSE_HTTPD_MUX_READ;
				expected_trigger_mux_status |= QSE_HTTPD_CLIENT_TASK_TRIGGER_READ_IN_MUX(i);
			}
			if (task->trigger.v[i].mask & QSE_HTTPD_TASK_TRIGGER_WRITE) 
			{
				expected_trigger_mux_mask |= QSE_HTTPD_MUX_WRITE;
				expected_trigger_mux_status |= QSE_HTTPD_CLIENT_TASK_TRIGGER_WRITE_IN_MUX(i);
			}

			if (expected_trigger_mux_mask)
			{
				has_trigger = 1;
				if (httpd->opt.scb.mux.addhnd (httpd, httpd->mux, task->trigger.v[i].handle, expected_trigger_mux_mask, client) <= -1) return -1;
				client->status |= expected_trigger_mux_status;
			}
		}

		/* client-side handle is registered for both reading and 
		 * writing when the task is executed for the first time.
		 * see update_mux_for_next_task() for this.
		 *
		 * starting from the second call, the client-side handle
		 * is registered for writing if it's explicitly requested.
		 * it's always registered for reading if not for QSE_HTTPD_CLIENT_MUTE.
		 *
		 * this means that QSE_HTTP_TASK_TRIGGER_READ set or clear
		 * in task->trigger.cmask is not honored.
		 */

		expected_client_handle_mux_mask = 0;
		expected_client_handle_mux_status = 0;
		if (task->trigger.cmask & QSE_HTTPD_TASK_TRIGGER_WRITE) 
		{
			expected_client_handle_mux_mask |= QSE_HTTPD_MUX_WRITE;
			expected_client_handle_mux_status |= QSE_HTTPD_CLIENT_HANDLE_WRITE_IN_MUX;
		}

		if (client->status & QSE_HTTPD_CLIENT_MUTE)
		{
			/* reading should be excluded from mux if the client-side has 
			 * been closed */
			client->status |= QSE_HTTPD_CLIENT_MUTE_DELETED;
		}
		else
		{
			if (task->trigger.cmask & QSE_HTTPD_TASK_TRIGGER_READ)
			{
				expected_client_handle_mux_mask |= QSE_HTTPD_MUX_READ;
				expected_client_handle_mux_status |= QSE_HTTPD_CLIENT_HANDLE_READ_IN_MUX; 
			}
		}

		if (!expected_client_handle_mux_mask && !has_trigger)
		{
			/* if there is no trigger and the client handle is to be excluded
			 * from reading and writing, writing should be enabled. */
			expected_client_handle_mux_mask |= QSE_HTTPD_MUX_WRITE;
			expected_client_handle_mux_status |= QSE_HTTPD_CLIENT_HANDLE_WRITE_IN_MUX;
		}

		if ((client->status & QSE_HTTPD_CLIENT_HANDLE_RW_IN_MUX) != expected_client_handle_mux_status)
		{
			httpd->opt.scb.mux.delhnd (httpd, httpd->mux, client->handle);
			client->status &= ~QSE_HTTPD_CLIENT_HANDLE_RW_IN_MUX;

			if (expected_client_handle_mux_mask)
			{
				if (httpd->opt.scb.mux.addhnd (httpd, httpd->mux, client->handle, expected_client_handle_mux_mask, client) <= -1) return -1;
				client->status |= expected_client_handle_mux_status;
			}
		}

		/* save the task trigger information */
		client->trigger = task->trigger;
	}

	return 0;
}

static int update_mux_for_next_task (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	/* A new task should be invoked at least once regardless of the
	 * data availablility(read) on the client side. Arrange to invoke
	 * this task so long as the client-side handle is readable or writable. */

	qse_httpd_task_t* task;
	int expected_mux_mask;
	int expected_mux_status;
	int expected_trigger_cmask;

/*printf ("update_mux_for_next_task\n");*/
	expected_mux_mask = QSE_HTTPD_MUX_READ;
	expected_mux_status = QSE_HTTPD_CLIENT_HANDLE_READ_IN_MUX;
	expected_trigger_cmask = QSE_HTTPD_TASK_TRIGGER_READ;

	task = client->task.head;
	if (task)
	{
		/* there is a pending task. arrange to trigger it as if it is 
		 * just entasked. */
		expected_mux_mask |= QSE_HTTPD_MUX_WRITE;
		expected_mux_status |= QSE_HTTPD_CLIENT_HANDLE_WRITE_IN_MUX;
		expected_trigger_cmask |= QSE_HTTPD_TASK_TRIGGER_WRITE;

		if (client->status & QSE_HTTPD_CLIENT_MUTE)
		{
			/* when client-side has been disconnected, it can't read
			 * from the side any more. so exclude reading */
			expected_mux_mask &= ~QSE_HTTPD_MUX_READ;
			expected_mux_status &= ~QSE_HTTPD_CLIENT_HANDLE_READ_IN_MUX;
			expected_trigger_cmask &= ~QSE_HTTPD_TASK_TRIGGER_READ;
		}
	}
	else 
	{
		/* there is no pending task to invoke. */

		if (client->status & QSE_HTTPD_CLIENT_MUTE)
		{
			/* and this client has closed connection previously.
			 * if not, reading would be the only clue to mux for
			 * invocation. return failure as reading from the client-side
			 * is not possible */
			return -1;
		}
	}

	if ((client->status & QSE_HTTPD_CLIENT_HANDLE_RW_IN_MUX) != expected_mux_status)
	{
		httpd->opt.scb.mux.delhnd (httpd, httpd->mux, client->handle);
		client->status &= ~QSE_HTTPD_CLIENT_HANDLE_RW_IN_MUX;

		QSE_ASSERT (expected_mux_status & QSE_HTTPD_CLIENT_HANDLE_RW_IN_MUX);
		if (httpd->opt.scb.mux.addhnd (httpd, httpd->mux, client->handle, expected_mux_mask, client) <= -1) return -1;
		client->status |= expected_mux_status;
	}

	if (task) task->trigger.cmask = expected_trigger_cmask;
	return 0;
}

static int invoke_client_task (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_httpd_hnd_t handle, int mask)
{
	qse_httpd_task_t* task;
	qse_size_t i;
	int n, trigger_fired;

	if (handle == client->handle && (mask & QSE_HTTPD_MUX_READ)) 
	{
		/* keep reading from the client-side as long as
		 * it's readable. */
		if (!(client->status & QSE_HTTPD_CLIENT_MUTE) && 
		    read_from_client (httpd, client) <= -1) 
		{
			/* return failure on disconnection also in order to
			 * purge the client in perform_client_task().
			 * thus the following line isn't necessary.
			 *if (httpd->errnum == QSE_HTTPD_EDISCON) return 0;*/
			return -1;
		}
	}

	task = client->task.head;
	if (task == QSE_NULL) 
	{
		/* this client doesn't have any task */
		if (client->status & QSE_HTTPD_CLIENT_MUTE)
		{
			/* handle the delayed client disconnection */
			return -1;
		}
		return 0;
	}

	trigger_fired = 0;

	clear_trigger_mask_result (task);
	if (handle == client->handle)
	{
		if (mask & QSE_HTTPD_MUX_READ)
		{
			/*QSE_ASSERT (task->trigger.cmask & QSE_HTTPD_TASK_TRIGGER_READ);*/
			task->trigger.cmask |= QSE_HTTPD_TASK_TRIGGER_READABLE;
			trigger_fired = 1;
		}
		if (mask & QSE_HTTPD_MUX_WRITE)
		{
			/*QSE_ASSERT (task->trigger.cmask & QSE_HTTPD_TASK_TRIGGER_WRITE);*/
			task->trigger.cmask |= QSE_HTTPD_TASK_TRIGGER_WRITABLE;
			trigger_fired = 1;
		}
	}
	else
	{
		for (i = 0; i < QSE_COUNTOF(task->trigger.v); i++)
		{
			if (task->trigger.v[i].handle == handle)
			{
				if (mask & QSE_HTTPD_MUX_READ)
				{
					/*QSE_ASSERT (task->trigger.v[i].mask & QSE_HTTPD_TASK_TRIGGER_READ);*/
					/* the assertion above may be false if a task for the same 
					 * trigger set was called earlier by a qse_mux_poll() call.
					 * and the task has changed some masks.
					 *
					 * for instance, you put handle A and B in to a trigger.
					 * if the task is triggered for A but the task may change
					 * the mask for B. the task may get executed for B by 
					 * the same qse_mux_poll() call. 
					 */

					task->trigger.v[i].mask |= QSE_HTTPD_TASK_TRIGGER_READABLE;
					trigger_fired = 1;
				}
				if (mask & QSE_HTTPD_MUX_WRITE)
				{
					/*QSE_ASSERT (task->trigger.v[i].mask & QSE_HTTPD_TASK_TRIGGER_WRITE);*/
					task->trigger.v[i].mask |= QSE_HTTPD_TASK_TRIGGER_WRITABLE;
					trigger_fired = 1;
				}
			}
		}
	}

	if (trigger_fired && !(task->trigger.cmask & QSE_HTTPD_TASK_TRIGGER_WRITABLE))
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

		/* WRITABLE can be set without WRITE as this is the result of 
		 * the additional writability check. */
		task->trigger.cmask |= QSE_HTTPD_TASK_TRIGGER_WRITABLE;
	}

	n = task->main (httpd, client, task);
	if (n == 0)
	{
		/* the current task is over. remove the task 
		 * from the queue. dequeue_task() clears task triggers
		 * from the mux. so i don't clear them explicitly here */
		dequeue_task (httpd, client); 

		/* update the multiplexer settings */
		n = update_mux_for_next_task (httpd, client);

		/* reset the task trigger remembered */
		QSE_MEMSET (&client->trigger, 0, QSE_SIZEOF(client->trigger));
	}
	else if (n > 0)
	{
		n = update_mux_for_current_task (httpd, client, task);
	}

	return n;
}

static int perform_client_task (
	qse_httpd_t* httpd, void* mux, qse_httpd_hnd_t handle, int mask, void* cbarg)
{
	qse_httpd_client_t* client;

	client = (qse_httpd_client_t*)cbarg;

	if (client->status & QSE_HTTPD_CLIENT_BAD) return 0;

	if (!(client->status & QSE_HTTPD_CLIENT_READY))
	{
		int x;
		x = httpd->opt.scb.client.accepted (httpd, client);
		if (x <= -1) goto oops;
		if (x >= 1) 
		{
			client->status |= QSE_HTTPD_CLIENT_READY;

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
	mark_bad_client (client);
	return -1;
}

static int perform_dns (qse_httpd_t* httpd, void* mux, qse_httpd_hnd_t handle, int mask, void* cbarg)
{
	qse_httpd_dns_t* dns = (qse_httpd_dns_t*)cbarg;

	QSE_ASSERT (mask & QSE_HTTPD_MUX_READ);
	QSE_ASSERT (&httpd->dns == dns);

	return httpd->opt.scb.dns.recv (httpd, dns, handle);
}

static int perform_urs (qse_httpd_t* httpd, void* mux, qse_httpd_hnd_t handle, int mask, void* cbarg)
{
	qse_httpd_urs_t* urs = (qse_httpd_urs_t*)cbarg;

	QSE_ASSERT (mask & QSE_HTTPD_MUX_READ);
	QSE_ASSERT (&httpd->urs == urs);

	return httpd->opt.scb.urs.recv (httpd, urs, handle);
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

	if (client->status & QSE_HTTPD_CLIENT_BAD) return QSE_NULL;

	new_task = enqueue_task (httpd, client, pred, task, xtnsize);
	if (new_task == QSE_NULL) 
	{
		/*purge_client (httpd, client);*/
		mark_bad_client (client);
	}
	else if (new_task->prev == QSE_NULL)
	{
		/* this task is the first task to activate. */

		if (update_mux_for_next_task (httpd, client) <= -1)
		{
			/*purge_client (httpd, client);*/
			mark_bad_client (client);

			/* call dequeue_task() to get new_task removed. this works
			 * because it is the only task. the task finalizer is also 
			 * called when it's dequeued. */
			dequeue_task (httpd, client);
 
			new_task = QSE_NULL;
		}
	}

	return (qse_httpd_task_t*)new_task;
}

static int dispatch_mux (qse_httpd_t* httpd, void* mux, qse_httpd_hnd_t handle, int mask, void* cbarg)
{
	switch (((qse_httpd_mate_t*)cbarg)->type)
	{
		case QSE_HTTPD_SERVER:
			return accept_client (httpd, mux, handle, mask, cbarg);

		case QSE_HTTPD_CLIENT:
			return perform_client_task (httpd, mux, handle, mask, cbarg);

		case QSE_HTTPD_DNS:
			return perform_dns (httpd, mux, handle, mask, cbarg);

		case QSE_HTTPD_URS:
			return perform_urs (httpd, mux, handle, mask, cbarg);

		case QSE_HTTPD_CUSTOM:
			return ((qse_httpd_custom_t*)cbarg)->dispatch (httpd, mux, handle, mask);
	}

	httpd->errnum = QSE_HTTPD_EINTERN;
	return -1;
}

int qse_httpd_loop (qse_httpd_t* httpd)
{
	int xret, count;
	qse_ntime_t tmout;
	qse_httpd_ecb_t* ecb;

	QSE_ASSERTX (httpd->server.list.head != QSE_NULL,
		"Add listeners before calling qse_httpd_loop()");

	QSE_ASSERTX (httpd->client.list.head == QSE_NULL,
		"No client should exist when this loop is started");

	QSE_ASSERT (QSE_TMR_SIZE(httpd->tmr) == 0);

	if (httpd->server.list.head == QSE_NULL) 
	{
		httpd->errnum = QSE_HTTPD_ENOSVR;
		return -1;
	}

	httpd->stopreq = 0;
	httpd->impedereq = 0;
	httpd->dnsactive = 0;

	/* system callbacks and request callbacks must be set before the call to this function */
	QSE_ASSERT (httpd->opt.scb.mux.open && httpd->opt.scb.mux.close && httpd->opt.scb.mux.poll);
	QSE_ASSERT (httpd->opt.rcb.peekreq && httpd->opt.rcb.pokereq);

	/* a server must be attached before the call to this function */
	QSE_ASSERT (httpd->server.navail > 0);

	httpd->mux = httpd->opt.scb.mux.open (httpd, dispatch_mux);
	if (httpd->mux == QSE_NULL) return -1;

	if (activate_dns (httpd) <= -1)
	{
		if (httpd->opt.trait & QSE_HTTPD_LOGACT)
		{
			qse_httpd_act_t msg;
			msg.code = QSE_HTTPD_CATCH_MWARNMSG;
			qse_mbscpy (msg.u.mwarnmsg, QSE_MT("cannot activate dns"));
			httpd->opt.rcb.logact (httpd, &msg);
		}

		HTTPD_DBGOUT0 ("Failed to activate DNS\n");
	}
	else httpd->dnsactive = 1;

	if (activate_urs (httpd) <= -1)
	{
		if (httpd->opt.trait & QSE_HTTPD_LOGACT)
		{
			qse_httpd_act_t msg;
			msg.code = QSE_HTTPD_CATCH_MWARNMSG;
			qse_mbscpy (msg.u.mwarnmsg, QSE_MT("cannot activate urs"));
			httpd->opt.rcb.logact (httpd, &msg);
		}
		HTTPD_DBGOUT0 ("Failed to activate URS\n");
	}
	else httpd->ursactive = 1;

	if (activate_servers (httpd) <= -1) 
	{
		if (httpd->dnsactive) deactivate_dns (httpd);
		if (httpd->ursactive) deactivate_urs (httpd);
		httpd->opt.scb.mux.close (httpd, httpd->mux);
		return -1;
	}
	if (httpd->server.nactive <= 0)
	{
		HTTPD_DBGOUT0 ("No servers are active. aborting\n");

		if (httpd->dnsactive) deactivate_dns (httpd);
		if (httpd->ursactive) deactivate_urs (httpd);
		httpd->opt.scb.mux.close (httpd, httpd->mux);

		httpd->errnum = QSE_HTTPD_ENOSVR;
		return -1;
	}

	/* call preloop hooks */
	for (ecb = httpd->ecb; ecb; ecb = ecb->next)
	{
		if (ecb->preloop) ecb->preloop (httpd);
	}

	xret = 0;
	while (!httpd->stopreq)
	{
		if (qse_tmr_gettmout (httpd->tmr, QSE_NULL, &tmout) <= -1) tmout = httpd->opt.tmout;
		count = httpd->opt.scb.mux.poll (httpd, httpd->mux, &tmout);
		/*HTTPD_DBGOUT4 ("Multiplexer returned %d client count = %d  tmout = %d.%d\n", (int)count, (int)httpd->client.list.count, (int)tmout.sec, (int)tmout.nsec);*/
		if (count <= -1) 
		{
			if (httpd->errnum != QSE_HTTPD_EINTR)
			{
				xret = -1; 
				break;
			}
		}

		qse_tmr_fire (httpd->tmr, QSE_NULL);
		purge_bad_clients (httpd);

		if (httpd->impedereq)
		{
			httpd->impedereq = 0;
			httpd->opt.rcb.impede (httpd);
		}
	}

	for (ecb = httpd->ecb; ecb; ecb = ecb->next)
	{
		if (ecb->postloop) ecb->postloop (httpd);
	}

	purge_client_list (httpd);
	deactivate_servers (httpd);

	if (httpd->ursactive) deactivate_urs (httpd);
	if (httpd->dnsactive) deactivate_dns (httpd);

	httpd->opt.scb.mux.close (httpd, httpd->mux);

	QSE_ASSERT (QSE_TMR_SIZE(httpd->tmr) == 0);
	return xret;
}

/* ----------------------------------------------------------------------- */

void qse_httpd_discardcontent (qse_httpd_t* httpd, qse_htre_t* req)
{
	qse_htre_discardcontent (req);
}

void qse_httpd_completecontent (qse_httpd_t* httpd, qse_htre_t* req)
{
	qse_htre_completecontent (req);
}

/* ----------------------------------------------------------------------- */
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

/* ----------------------------------------------------------------------- */

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

/* ----------------------------------------------------------------------- */

int qse_httpd_resolvename (qse_httpd_t* httpd, const qse_mchar_t* name, qse_httpd_resolve_t resol, const qse_httpd_dns_server_t* dns_server, void* ctx)
{
	if (!httpd->dnsactive) 
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENODNS);
		return -1;
	}

#if defined(QSE_HTTPD_DEBUG)
	{
		qse_mchar_t tmp[128];
		if (dns_server)
			qse_nwadtombs (&dns_server->nwad, tmp, QSE_COUNTOF(tmp), QSE_NWADTOMBS_ALL);
		else
			qse_mbscpy (tmp, QSE_MT("default server"));

		HTTPD_DBGOUT2 ("Sending DNS request [%hs] to [%hs]\n", name, tmp);
	}
#endif

	return httpd->opt.scb.dns.send (httpd, &httpd->dns, name, resol, dns_server, ctx);
}

int qse_httpd_rewriteurl (qse_httpd_t* httpd, const qse_mchar_t* url, qse_httpd_rewrite_t rewrite, const qse_httpd_urs_server_t* urs_server, void* ctx)
{
	if (!httpd->ursactive)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOURS);
		return -1;
	}

	HTTPD_DBGOUT1 ("Sending URS request [%hs]\n", url);
	return httpd->opt.scb.urs.send (httpd, &httpd->urs, url, rewrite, urs_server, ctx);
}

int qse_httpd_activatetasktrigger (qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	int x, org_cmask;

	/* don't do anything if it's active */
	if (!(task->trigger.flags & QSE_HTTPD_TASK_TRIGGER_INACTIVE)) return 0;

	/* when task trigger is inactive, no handle are registered
	 * into mux. update_mux_for_current_task adds the client handle
	 * to mux for reading only if writing is not requested explicitly.
	 * if no data is available for reading, the task can never be
	 * called after activation. so let's request writing here.
	 */
	QSE_ASSERT (!(client->status & QSE_HTTPD_CLIENT_HANDLE_RW_IN_MUX));
	org_cmask = task->trigger.cmask;
	task->trigger.cmask |= QSE_HTTPD_TASK_TRIGGER_WRITE;

	task->trigger.flags &= ~QSE_HTTPD_TASK_TRIGGER_INACTIVE;

	x = update_mux_for_current_task (httpd, client, task);

	task->trigger.cmask = org_cmask;
	return x;
}

int qse_httpd_inactivatetasktrigger (qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	if (task->trigger.flags & QSE_HTTPD_TASK_TRIGGER_INACTIVE) return 0;

	task->trigger.flags |= QSE_HTTPD_TASK_TRIGGER_INACTIVE;
	return update_mux_for_current_task (httpd, client, task);
}


/* ------------------------------------------------------------------- */


static void update_timer_event (qse_tmr_t* tmr, qse_tmr_index_t old_index, qse_tmr_index_t new_index, qse_tmr_event_t* evt)
{
	tmr_xtn_t* tmr_xtn;
	qse_httpd_t* httpd;
	qse_httpd_timer_updater_t updater;

	tmr_xtn = qse_tmr_getxtn (tmr);
	httpd = tmr_xtn->httpd;
	updater = evt->ctx2;
	updater (httpd, old_index, new_index, evt->ctx);
}

static void handle_timer_event (qse_tmr_t* tmr, const qse_ntime_t* now, qse_tmr_event_t* evt)
{
	tmr_xtn_t* tmr_xtn;
	qse_httpd_t* httpd;
	qse_httpd_timer_handler_t handler;

	tmr_xtn = qse_tmr_getxtn (tmr);
	httpd = tmr_xtn->httpd;
	handler = evt->ctx3;
	handler (httpd, now, evt->ctx);
}

int qse_httpd_inserttimerevent (qse_httpd_t* httpd, const qse_httpd_timer_event_t* event, qse_httpd_timer_index_t* index)
{
	qse_tmr_event_t timer_event;
	qse_tmr_index_t timer_index;

	QSE_MEMSET (&timer_event, 0, QSE_SIZEOF(timer_event));
	timer_event.ctx     = event->ctx;
	timer_event.ctx2    = event->updater;
	timer_event.ctx3    = event->handler;
	timer_event.when    = event->when;
	timer_event.updater = update_timer_event;
	timer_event.handler = handle_timer_event;

	timer_index = qse_tmr_insert (httpd->tmr, &timer_event);
	if (timer_index == QSE_TMR_INVALID_INDEX)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOMEM);
		return -1;
	}

	*index = timer_index;
	return 0;
}

void qse_httpd_removetimerevent (qse_httpd_t* httpd, qse_httpd_timer_index_t index)
{
	qse_tmr_remove (httpd->tmr, index);
}

/* qse_httpd_insert_timer_event() is a lighter-weight version of 
 * qse_httpd_inserttimerevent() and intended for internal use only */

int qse_httpd_insert_timer_event (qse_httpd_t* httpd, const qse_tmr_event_t* event, qse_tmr_index_t* index)
{
	qse_tmr_index_t tmp = qse_tmr_insert (httpd->tmr, event);
	if (tmp == QSE_TMR_INVALID_INDEX)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOMEM);
		return -1;
	}

	*index = tmp;
	return 0;
}
void qse_httpd_remove_timer_event (qse_httpd_t* httpd, qse_tmr_index_t index)
{
	qse_tmr_remove (httpd->tmr, index);
}

/* ----------------------------------------------------------------------- */

static void unload_all_modules (qse_httpd_t* httpd)
{
	qse_httpd_mod_t* mod;

	while (httpd->modlist)
	{
		mod = httpd->modlist;
		httpd->modlist = mod->next;

		if (mod->unload) mod->unload (mod);
		httpd->opt.scb.mod.close (httpd, mod->handle  );
		qse_httpd_freemem (httpd, mod);
	}
}

qse_httpd_mod_t* qse_httpd_loadmod (qse_httpd_t* httpd, const qse_char_t* name)
{
	qse_httpd_mod_t* mod;
	qse_size_t name_len, prefix_len, postfix_len, fullname_len;
	const qse_char_t* prefix, * postfix;
	qse_char_t* entry_point_name;
	qse_httpd_mod_load_t load;

/* TODO: no singly linked list speed up */

	name_len = qse_strlen(name);

	if (httpd->opt.mod[0].len > 0)
	{
		prefix = httpd->opt.mod[0].ptr;
		prefix_len = httpd->opt.mod[0].len;
	}
	else
	{
		prefix = QSE_T(QSE_HTTPD_DEFAULT_MODPREFIX);
		prefix_len = qse_strlen(prefix);
	}

	if (httpd->opt.mod[1].len > 0)
	{
		postfix = httpd->opt.mod[1].ptr;
		postfix_len = httpd->opt.mod[1].len;
	}
	else
	{
		postfix = QSE_T(QSE_HTTPD_DEFAULT_MODPOSTFIX);
		postfix_len = qse_strlen(postfix);
	}

	/* 
	 * +15: length of _qse_httpd_mod_
	 * +2: _\0
	 */
	fullname_len = prefix_len + name_len + postfix_len;
	mod = qse_httpd_callocmem (httpd, QSE_SIZEOF(*mod) + (name_len + 1 + fullname_len + 1 + 15 + name_len + 2) * QSE_SIZEOF(qse_char_t));
	if (mod == QSE_NULL) return QSE_NULL;

	mod->httpd = httpd;
	mod->name = (qse_char_t*)(mod + 1);
	mod->fullname = mod->name + name_len + 1;
	entry_point_name = mod->fullname + fullname_len + 1;
	qse_strcpy (mod->name, name);
	qse_strjoin (mod->fullname, prefix, name, postfix, QSE_NULL);
	qse_strjoin (entry_point_name, QSE_T("_qse_httpd_mod_"), name, QSE_NULL);

	mod->handle = httpd->opt.scb.mod.open (httpd, mod->fullname);
	if (!mod->handle)
	{
		qse_httpd_freemem (httpd, mod);
		return QSE_NULL;
	}

	/* attempt qse_httpd_mod_xxx */
	load = httpd->opt.scb.mod.symbol (httpd, mod->handle, &entry_point_name[1]);
	if (!load)
	{
		/* attempt _qse_awk_mod_xxx */
		load = httpd->opt.scb.mod.symbol (httpd, mod->handle, &entry_point_name[0]);
		if (!load)
		{
			/* attempt qse_awk_mod_xxx_ */
			entry_point_name[15 + name_len] = QSE_T('_');
			entry_point_name[15 + name_len + 1] = QSE_T('\0');
			load = httpd->opt.scb.mod.symbol (httpd, mod->handle, &entry_point_name[1]);
		}
	}

	if (!load || load (mod) <= -1)
	{
		httpd->opt.scb.mod.close (httpd, mod->handle);
		qse_httpd_freemem (httpd, mod);
		return QSE_NULL;
	}

	/* link the loaded module to the module list */
	mod->next = httpd->modlist;
	httpd->modlist = mod;

	return mod;
}

int qse_httpd_configmod (qse_httpd_t* httpd, qse_httpd_mod_t* mod, const qse_char_t* key, const qse_char_t* value)
{
	QSE_ASSERT (httpd == mod->httpd);

	if (mod->config)
	{
		return mod->config (mod, key, value);
	}
	else
	{
		/* not allowed to set the module configuration 
		 * without the 'config' handler */
		httpd->errnum = QSE_HTTPD_EACCES; 
		return -1;
	}
}

qse_httpd_mod_t* qse_httpd_findmod (qse_httpd_t* httpd, const qse_char_t* name)
{
	qse_httpd_mod_t* mod;

/* TODO: no sequential search. speed up */
	for (mod = httpd->modlist; mod;  mod = mod->next)
	{
		if (qse_strcmp (mod->name, name) == 0) return mod;
	}

	return QSE_NULL;
}
