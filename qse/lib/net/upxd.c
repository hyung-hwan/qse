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
    License along with QSE. If not, see <htrd://www.gnu.org/licenses/>.
 */

#include "upxd.h"
#include <qse/cmn/str.h>

static void disable_all_servers (qse_upxd_t* upxd);
static void free_all_servers (qse_upxd_t* upxd);
static qse_upxd_server_session_t* find_server_session (
	qse_upxd_t* upxd, qse_upxd_server_t* server, qse_nwad_t* from);
static void release_session (
	qse_upxd_t* upxd, qse_upxd_server_session_t* session);


qse_upxd_t* qse_upxd_open (qse_mmgr_t* mmgr, qse_size_t xtnsize)
{
	qse_upxd_t* upxd;

	upxd = (qse_upxd_t*) QSE_MMGR_ALLOC (
		mmgr, QSE_SIZEOF(*upxd) + xtnsize
	);
	if (upxd == QSE_NULL) return QSE_NULL;

	if (qse_upxd_init (upxd, mmgr) <= -1)
	{
		QSE_MMGR_FREE (upxd->mmgr, upxd);
		return QSE_NULL;
	}

	return upxd;
}

void qse_upxd_close (qse_upxd_t* upxd)
{
	qse_upxd_fini (upxd);
	QSE_MMGR_FREE (upxd->mmgr, upxd);
}

int qse_upxd_init (qse_upxd_t* upxd, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (upxd, 0, QSE_SIZEOF(*upxd));
	upxd->mmgr = mmgr;
	return 0;
}

void qse_upxd_fini (qse_upxd_t* upxd)
{
	if (upxd->server.nactive > 0) disable_all_servers (upxd);
	
	if (upxd->mux) 
	{
		upxd->cbs->mux.close (upxd, upxd->mux);
		upxd->mux = QSE_NULL;
	}
	
	free_all_servers (upxd);
}

qse_mmgr_t* qse_upxd_getmmgr (qse_upxd_t* upxd)
{
	return upxd->mmgr;
}

void* qse_upxd_getxtn (qse_upxd_t* upxd)
{
	return QSE_XTN (upxd);
}

qse_upxd_errnum_t qse_upxd_geterrnum (qse_upxd_t* upxd)
{
	return upxd->errnum;
}

void qse_upxd_seterrnum (qse_upxd_t* upxd, qse_upxd_errnum_t errnum)
{
	upxd->errnum = errnum;
}

QSE_INLINE void* qse_upxd_allocmem (qse_upxd_t* upxd, qse_size_t size)
{
	void* ptr = QSE_MMGR_ALLOC (upxd->mmgr, size);
	if (ptr == QSE_NULL) upxd->errnum = QSE_UPXD_ENOMEM;
	return ptr;
}

QSE_INLINE void* qse_upxd_reallocmem (
	qse_upxd_t* upxd, void* ptr, qse_size_t size)
{
	void* nptr = QSE_MMGR_REALLOC (upxd->mmgr, ptr, size);
	if (nptr == QSE_NULL) upxd->errnum = QSE_UPXD_ENOMEM;
	return nptr;
}

QSE_INLINE void qse_upxd_freemem (qse_upxd_t* upxd, void* ptr)
{
	QSE_MMGR_FREE (upxd->mmgr, ptr);
}


static int perform_session_task (
	qse_upxd_t* upxd, void* mux, qse_ubi_t handle, void* cbarg)
{
	qse_upxd_server_session_t* session;
	qse_upxd_server_t* server;
	qse_ssize_t n;

	session = (qse_upxd_server_session_t*)cbarg;
	server = session->inner.server;

	qse_gettime (&session->modified);

	/* this handler should set the 'from' field of server->scok */
	n = upxd->cbs->sock.recv (
		upxd, &session->peer, upxd->rbuf, QSE_SIZEOF(upxd->rbuf));
	if (n <= -1)
	{
		upxd->cbs->session.error (upxd, &session->inner);
		release_session (upxd, session);
		return -1;
	}

	/* TODO: inspect if session->inner.to matches session->sock.from.
	         drop it if they don't match if a certain option (QSE_UPXD_STRICT)
			 is set??? */

	/* send the peer's packet back to the client */ 
	server->local.to = session->inner.client;
	n = upxd->cbs->sock.send (upxd, &server->local, upxd->rbuf, n);
	if (n <= -1)
	{
		upxd->cbs->session.error (upxd, &session->inner);
		release_session (upxd, session);
		return -1;
	}

	return 0;
}

static int perform_server_task (
	qse_upxd_t* upxd, void* mux, qse_ubi_t handle, void* cbarg)
{
	qse_upxd_server_t* server;
	qse_upxd_server_session_t* session;
	qse_ssize_t n;

	server = (qse_upxd_server_t*)cbarg;

	/* this handler should set the 'from' field of server->scok */
	n = upxd->cbs->sock.recv (
		upxd, &server->local, upxd->rbuf, QSE_SIZEOF(upxd->rbuf));
	if (n <= -1) return -1;

	/* get the existing session or create a new session based on
	 * server->local->from */
	session = find_server_session (upxd, server, &server->local.from);
	if (session == QSE_NULL)
	{
		qse_upxd_session_t interim;
		QSE_MEMSET (&interim, 0, QSE_SIZEOF(interim));
		interim.client = server->local.from;
		upxd->cbs->session.error (upxd, &interim);
		return -1;
	}

	n = upxd->cbs->sock.send (upxd, &session->peer, upxd->rbuf, n);
	if (n <= -1)
	{
		upxd->cbs->session.error (upxd, &session->inner);
		release_session (upxd, session);
		return -1;
	}

	return 0;
}

static qse_upxd_server_session_t* find_server_session (
	qse_upxd_t* upxd, qse_upxd_server_t* server, qse_nwad_t* from)
{
	qse_upxd_server_session_t* session;

	/* TODO: make it indexable or hashable with 'from'
	 *       don't perform linear search */
	/* find an existing session made for the source address 'from' */
	for (session = server->session.list; session; session = session->next)
	{
		if (QSE_MEMCMP (&session->inner.client, from, QSE_SIZEOF(*from)) == 0)
		{
			qse_gettime (&session->modified);
			return session;
		}
	}

	/* there is no session found for the source address 'from'.
	 * let's create a new session. */
	session = qse_upxd_allocmem (upxd, QSE_SIZEOF(*session));
	if (session == QSE_NULL) return QSE_NULL;

	QSE_MEMSET (session, 0, QSE_SIZEOF(*session));

	if (qse_gettime (&session->created) <= -1)
	{
		qse_upxd_freemem (upxd, session);
		upxd->errnum = QSE_UPXD_ESYSERR;
		return QSE_NULL;
	}
	session->modified = session->created;
	
	session->inner.server = server;
	session->inner.client = *from;

	/* set the default dormancy */
	session->inner.config.dormancy = QSE_UPXD_SESSION_DORMANCY;

	/* call the configurationc callback for configuration data */
	if (upxd->cbs->session.config (upxd, &session->inner) <= -1)
	{
		qse_upxd_freemem (upxd, session);
		return QSE_NULL;
	}

	/* set up the peer socket with the configuration data */ 
	session->peer.bind = session->inner.config.bind;
	session->peer.to = session->inner.config.peer;
	if (session->inner.config.dev[0] != QSE_T('\0'))
		session->peer.dev = session->inner.config.dev;

	if (upxd->cbs->sock.open (upxd, &session->peer) <= -1)
	{
		qse_upxd_freemem (upxd, session);
		return QSE_NULL;
	}

	if (upxd->cbs->mux.addhnd (
		upxd, upxd->mux, session->peer.handle,
		perform_session_task, session) <= -1)
	{
		upxd->cbs->sock.close (upxd, &session->peer);
		qse_upxd_freemem (upxd, session);
		return QSE_NULL;
	}

	/* insert the session into the head of the session list */
	if (server->session.list)
		server->session.list->prev = session;
	session->next = server->session.list;
	server->session.list = session;

	return session;
}

static void release_session (
	qse_upxd_t* upxd, qse_upxd_server_session_t* session)
{
	qse_upxd_server_t* server;

	server = session->inner.server;
	QSE_ASSERT (server != QSE_NULL);

	upxd->cbs->mux.delhnd (upxd, upxd->mux, session->peer.handle);
	upxd->cbs->sock.close (upxd, &session->peer);

	/* remove the session from the session list */
	if (session->next) session->next->prev = session->prev;
	if (session->prev) session->prev->next = session->next;
	else server->session.list = session->next;

	/* destroy the session */
	qse_upxd_freemem (upxd, session);
}

static int enable_server (qse_upxd_t* upxd, qse_upxd_server_t* server)
{
	QSE_ASSERT (upxd->cbs != QSE_NULL);
	QSE_ASSERT (!(server->flags & QSE_UPXD_SERVER_ENABLED));
	
	if (upxd->cbs->sock.open (upxd, &server->local) <= -1)
	{
		return -1;
	}

	if (upxd->cbs->mux.addhnd (
		upxd, upxd->mux, server->local.handle,
		perform_server_task, server) <= -1)
	{
		upxd->cbs->sock.close (upxd, &server->local);
		return -1;
	}

	server->flags |= QSE_UPXD_SERVER_ENABLED;
	upxd->server.nactive++;
	return 0;
}

static void disable_server (qse_upxd_t* upxd, qse_upxd_server_t* server)
{
	qse_upxd_server_session_t* session;

	QSE_ASSERT (upxd->cbs != QSE_NULL);
	QSE_ASSERT (server->flags & QSE_UPXD_SERVER_ENABLED);

	session = server->session.list;
	while (session)
	{
		qse_upxd_server_session_t* next = session->next;
		release_session (upxd, session);
		session = next;
	}

	upxd->cbs->mux.delhnd (upxd, upxd->mux, server->local.handle);
	upxd->cbs->sock.close (upxd, &server->local);

	server->flags &= ~QSE_UPXD_SERVER_ENABLED;
	upxd->server.nactive--;
}

static void enable_all_servers (qse_upxd_t* upxd)
{
	qse_upxd_server_t* server;

	for (server = upxd->server.list; server; server = server->next)
	{
		if (!(server->flags & QSE_UPXD_SERVER_ENABLED))
		{
			enable_server (upxd, server);
		}
	}
}

static void disable_all_servers (qse_upxd_t* upxd)
{
	qse_upxd_server_t* server;

	server = upxd->server.list;
	while (server)
	{
		if (server->flags & QSE_UPXD_SERVER_ENABLED)
			disable_server (upxd, server);

		server = server->next;
	}
}

static void free_all_servers (qse_upxd_t* upxd)
{
	qse_upxd_server_t* server;
	qse_upxd_server_t* next;

	server = upxd->server.list;
	while (server)
	{
		next = server->next;
		QSE_MMGR_FREE (upxd->mmgr, server);
		server = next;
	}
	upxd->server.list = QSE_NULL;
}

qse_upxd_server_t* qse_upxd_addserver (
	qse_upxd_t* upxd, const qse_nwad_t* nwad, const qse_char_t* dev)
{
	qse_upxd_server_t* server;

	if (dev && qse_strlen(dev) >= QSE_COUNTOF(server->dev))
	{
		upxd->errnum = QSE_UPXD_EINVAL;
		return QSE_NULL;
	}

	server = QSE_MMGR_ALLOC (upxd->mmgr, QSE_SIZEOF(*server));
	if (server == QSE_NULL)
	{
		upxd->errnum = QSE_UPXD_ENOMEM;
		return QSE_NULL;
	}

	QSE_MEMSET (server, 0, QSE_SIZEOF(*server));
	if (dev)
	{
		qse_strxcpy (server->dev, QSE_COUNTOF(server->dev), dev);
		server->local.dev = server->dev;
	}
	server->local.bind = *nwad;

	/* chain it to the head of the list */
	if (upxd->server.list)
		upxd->server.list->prev = server;
	server->next = upxd->server.list;
	upxd->server.list = server;

	return server;
}

void qse_upxd_delserver (
	qse_upxd_t* upxd, qse_upxd_server_t* server)
{
	if (server->flags & QSE_UPXD_SERVER_ENABLED)
		disable_server (upxd, server);

	/* unchain the session from the list */
	if (server->next) server->next->prev = server->prev;
	if (server->prev) server->prev->next = server->next;
	else upxd->server.list = server->next;
}

void* qse_upxd_getserverctx (
	qse_upxd_t* upxd, qse_upxd_server_t* server)
{
	return server->ctx;
}

void qse_upxd_setserverctx (
	qse_upxd_t* upxd, qse_upxd_server_t* server, void* ctx)
{
	server->ctx = ctx;
}

qse_upxd_cbs_t* qse_upxd_getcbs (qse_upxd_t* upxd)
{
	return upxd->cbs;
}

void qse_upxd_setcbs (qse_upxd_t* upxd, qse_upxd_cbs_t* cbs)
{
	upxd->cbs = cbs;
}

static QSE_INLINE void purge_idle_sessions_in_server (
	qse_upxd_t* upxd, qse_upxd_server_t* server)
{
	qse_upxd_server_session_t* session;
	qse_upxd_server_session_t* next;
	qse_ntime_t now;

	qse_gettime (&now);

	session = server->session.list;
	while (session)
	{
		next = session->next;

		if (session->inner.config.dormancy > 0 &&
		    now > session->modified &&
		    now - session->modified > session->inner.config.dormancy)
		{
			release_session (upxd, session);
		}

		session = next;
	}
}

static void purge_idle_sessions (qse_upxd_t* upxd)
{
	qse_upxd_server_t* server;

	for (server = upxd->server.list; server; server = server->next)
	{
		if (server->flags & QSE_UPXD_SERVER_ENABLED)
		{
			purge_idle_sessions_in_server (upxd, server);
		}
	}
}

int qse_upxd_loop (qse_upxd_t* upxd, qse_ntime_t timeout)
{
	int retv = -1;

	QSE_ASSERTX (upxd->cbs != QSE_NULL,
		"Call qse_upxd_setcbs() before calling qse_upxd_loop()");

	if (upxd->cbs == QSE_NULL)
	{
		upxd->errnum = QSE_UPXD_EINVAL;
		goto oops;
	}

	if (upxd->mux)
	{
		/* close the mutiplexer if it's open */
		upxd->cbs->mux.close (upxd, upxd->mux);
		upxd->mux = QSE_NULL;
	}

	upxd->stopreq = 0;
	upxd->mux = upxd->cbs->	mux.open (upxd);
	if (upxd->mux == QSE_NULL) goto oops;

	enable_all_servers (upxd);
	
	while (!upxd->stopreq)
	{
		int count;

		count = upxd->cbs->mux.poll (upxd, upxd->mux, timeout);
		if (count <= -1)
		{
			/* TODO: anything? */
		}
			
		purge_idle_sessions (upxd);
		enable_all_servers (upxd);
	}

	retv = 0;

oops:
	if (upxd->server.nactive > 0) disable_all_servers (upxd);
	if (upxd->mux) 
	{
		upxd->cbs->mux.close (upxd, upxd->mux);
		upxd->mux = QSE_NULL;
	}
	return retv;
}

void qse_upxd_stop (qse_upxd_t* upxd)
{
	upxd->stopreq = 1;
}

int qse_upxd_enableserver (qse_upxd_t* upxd, qse_upxd_server_t* server)
{
	if (server->flags & QSE_UPXD_SERVER_ENABLED)
	{
		upxd->errnum = QSE_UPXD_EINVAL;
		return -1;
	}

	return enable_server (upxd, server);
}

int qse_upxd_disableserver (qse_upxd_t* upxd, qse_upxd_server_t* server)
{
	if (!(server->flags & QSE_UPXD_SERVER_ENABLED))
	{
		upxd->errnum = QSE_UPXD_EINVAL;
		return -1;
	}

	disable_server (upxd, server);
	return 0;
}

int qse_upxd_poll (qse_upxd_t* upxd, qse_ntime_t timeout)
{
	int ret;

	QSE_ASSERTX (upxd->cbs != QSE_NULL,
		"Call qse_upxd_setcbs() before calling qse_upxd_loop()");

	if (upxd->mux == QSE_NULL)
	{
		upxd->mux = upxd->cbs->	mux.open (upxd);
		if (upxd->mux == QSE_NULL) return -1;
	}
	
	ret = upxd->cbs->mux.poll (upxd, upxd->mux, timeout);
	purge_idle_sessions (upxd);
	
	return ret;
}
