/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EQSERESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <qse/si/TcpServer.hpp>
#include <qse/si/os.h>
#include <qse/cmn/str.h>
#include <qse/cmn/chr.h>
#include "../cmn/mem-prv.h"

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>


#define WID_MAP_ALIGN 128
#define WID_MAX (_wid_map_t::WID_INVALID - 1)

QSE_BEGIN_NAMESPACE(QSE)

#include "../cmn/syserr.h"
IMPLEMENT_SYSERR_TO_ERRNUM (TcpServer::ErrorNumber, TcpServer::)

int TcpServer::Connection::main ()
{
	int n;

	// blockAllSignals is called inside run because 
	// Connection is instantiated in the TcpServer thread.
	// so if it is called in the constructor of Connection, 
	// it would just block signals to the TcpProxy thread.
	this->blockAllSignals (); // don't care about the result.

	try { n = this->listener->server->handle_connection(this); }
	catch (...) { n = -1; }

	TcpServer* server = this->getServer();

	server->logfmt (QSE_LOG_INFO, QSE_T("[h=%d,w=%zu] closing connection\n"), (int)this->socket.getHandle(), this->getWid());

	server->_connection_list_spl.lock ();
	this->csspl.lock ();
	this->socket.close (); 
	this->csspl.unlock ();
	if (!this->claimed)
	{
		server->_connection_list[Connection::LIVE].remove (this);
		server->_connection_list[Connection::DEAD].append (this);
	}
	server->_connection_list_spl.unlock ();

	return n;
}

int TcpServer::Connection::stop () QSE_CPP_NOEXCEPT
{
	// the receiver will be notified of the end of 
	// the connection by the socket's closing.
	// therefore, handle_connection() must return
	// when it detects the end of the connection.
	this->csspl.lock ();
	this->socket.shutdown ();
	this->csspl.unlock ();
	return 0;
}

TcpServer::TcpServer (Mmgr* mmgr) QSE_CPP_NOEXCEPT: 
	Mmged(mmgr),
	_halt_requested(false), 
	_server_serving(false), 
	_max_connections(0),
	_thread_stack_size(0)
{
}

TcpServer::~TcpServer () QSE_CPP_NOEXCEPT
{
	// QSE_ASSERT (this->_server_serving == false);
	this->delete_all_connections (Connection::LIVE);
	this->delete_all_connections (Connection::DEAD);
}

void TcpServer::free_all_listeners () QSE_CPP_NOEXCEPT
{
	Listener* lp;

	while (this->_listener_list.head)
	{
		lp = this->_listener_list.head;
		this->_listener_list.head = lp->next_listener;
		this->_listener_list.count--;

		qse_mux_evt_t evt;
		evt.hnd = lp->getHandle();
		qse_mux_delete (this->_listener_list.mux, &evt);

		this->logfmt (QSE_LOG_INFO, QSE_T("[l=%d] closing listener\n"), (int)evt.hnd);
		lp->close ();

		QSE_CPP_DELETE_WITH_MMGR (lp, Listener, this->getMmgr()); // delete lp
	}

	if (this->_listener_list.mux_pipe[0] >= 0)
	{
		qse_mux_evt_t evt;
		evt.hnd = this->_listener_list.mux_pipe[0];
		qse_mux_delete (this->_listener_list.mux, &evt);

		close (this->_listener_list.mux_pipe[0]);
		this->_listener_list.mux_pipe[0] = -1;
	}

	this->_listener_list.mux_pipe_spl.lock ();
	if (this->_listener_list.mux_pipe[1] >= 0)
	{
		close (this->_listener_list.mux_pipe[1]);
		this->_listener_list.mux_pipe[1] = -1;
	}
	this->_listener_list.mux_pipe_spl.unlock ();

	QSE_ASSERT (this->_listener_list.mux != QSE_NULL);
	qse_mux_close (this->_listener_list.mux);
	this->_listener_list.mux = QSE_NULL;
}

struct mux_xtn_t
{
	bool first_time;
	TcpServer* server;
};


void TcpServer::dispatch_mux_event (qse_mux_t* mux, const qse_mux_evt_t* evt) QSE_CPP_NOEXCEPT
{
	mux_xtn_t* mux_xtn = (mux_xtn_t*)qse_mux_getxtn(mux);
	TcpServer* server = mux_xtn->server;

	if (mux_xtn->first_time)
	{
		server->delete_all_connections(Connection::DEAD);
		mux_xtn->first_time = false;
	}

	if (!evt->mask) return;

	if (evt->data == NULL)
	{
		/* just consume data written by TcpServer::halt() */
		char tmp[128];
		while (::read(server->_listener_list.mux_pipe[0], tmp, QSE_SIZEOF(tmp)) > 0) /* nothing */;
	}
	else
	{
		/* the reset should be the listener's socket */
		Listener* lsck = (Listener*)evt->data;

		if (server->_max_connections > 0 && server->_max_connections <= server->_connection_list[Connection::LIVE].getSize()) 
		{
			// too many connections. accept the connection and close it.
			server->logfmt (QSE_LOG_ERROR, QSE_T("[l=%d] - too many connections - %zu\n"), (int)lsck->getHandle(), server->_connection_list[Connection::LIVE].getSize());
			goto accept_and_drop;
		}

		Connection* connection;

		// allocating the connection object before accept is 
		// a bit awkward. but socket.accept() can be passed
		// the socket field inside the connection object.
		try { connection = new(server->getMmgr()) Connection(lsck); } 
		catch (...) 
		{
			// memory alloc failed. accept the connection and close it.
			server->logfmt (QSE_LOG_ERROR, QSE_T("[l=%d] unable to instantiate connection\n"), (int)lsck->getHandle());
			goto accept_and_drop;
		}
		if (server->_wid_map.free_first == _wid_map_t::WID_INVALID && server->prepare_to_acquire_wid() <= -1)
		{
			server->logfmt (QSE_LOG_ERROR, QSE_T("[l=%d] unable to assign id to connection\n"), (int)lsck->getHandle());
			QSE_CPP_DELETE_WITH_MMGR (connection, Connection, server->getMmgr());
			goto accept_and_drop;
		}

		if (lsck->accept(&connection->socket, &connection->address, Socket::T_CLOEXEC) <= -1)
		{
			server->logfmt (QSE_LOG_ERROR, QSE_T("[l=%d] unable to accept connection - %hs\n"), (int)lsck->getHandle(), strerror(errno));
			QSE_CPP_DELETE_WITH_MMGR (connection, Connection, server->getMmgr());

			if (server->isHaltRequested()) return; /* normal termination requested */

			Socket::ErrorNumber lerr = lsck->getErrorNumber();
			if (lerr == Socket::E_EINTR || lerr == Socket::E_EAGAIN) return;

			server->setErrorNumber (lerr);
			server->halt ();
			return;
		}

		server->_connection_list_spl.lock ();
		server->_connection_list[Connection::LIVE].append (connection); 
		server->_connection_list_spl.unlock ();

		server->acquire_wid (connection);
		connection->setStackSize (server->_thread_stack_size);
	#if defined(_WIN32)
		if (connection->start(Thread::DETACHED) <= -1) 
	#else
		if (connection->start(0) <= -1)
	#endif
		{
			qse_char_t addrbuf[128];
			server->logfmt (QSE_LOG_ERROR, QSE_T("[l=%d] unable to start connection for connection from %s\n"), (int)lsck->getHandle(), connection->address.toStrBuf(addrbuf, QSE_COUNTOF(addrbuf)));

			server->_connection_list_spl.lock ();
			server->_connection_list[Connection::LIVE].remove (connection);
			server->_connection_list_spl.unlock ();

			server->release_wid (connection);
			QSE_CPP_DELETE_WITH_MMGR (connection, Connection, server->getMmgr());
		}
		else
		{
			qse_char_t addrbuf[128];
			server->logfmt (QSE_LOG_INFO, QSE_T("[l=%d,h=%d,w=%zu] connection from %js\n"), (int)lsck->getHandle(), (int)connection->socket.getHandle(), connection->getWid(), connection->address.toStrBuf(addrbuf, QSE_COUNTOF(addrbuf)));
		}
		return;

	accept_and_drop:
		Socket s;
		SocketAddress sa;

		if (lsck->accept(&s, &sa, Socket::T_CLOEXEC) >= 0) 
		{
			qse_char_t addrbuf[128];
			server->logfmt (QSE_LOG_ERROR, QSE_T("[l=%d] accepted but dropped connection from %js\n"), (int)lsck->getHandle(), sa.toStrBuf(addrbuf, QSE_COUNTOF(addrbuf)));
			s.close();
		}
	}
}

static const qse_char_t* strip_enclosing_spaces (const qse_char_t* ptr, qse_size_t* len)
{
	const qse_char_t* end = ptr + *len;
	while (ptr < end)
	{
		if (!QSE_ISSPACE(*ptr)) break;
		ptr++;
	}

	while (end > ptr)
	{
		if (!QSE_ISSPACE(end[-1])) break;
		end--;
	}
	*len = end - ptr;
	return ptr;
}

int TcpServer::setup_listeners (const qse_char_t* addrs) QSE_CPP_NOEXCEPT
{
	const qse_char_t* addr_ptr, * comma;
	qse_mux_t* mux = QSE_NULL;
	qse_mux_evt_t ev;
	int fcv, pfd[2] = { -1, - 1 };
	SocketAddress sockaddr;

	errno = 0;
	mux = qse_mux_open(this->getMmgr(), QSE_SIZEOF(mux_xtn_t), TcpServer::dispatch_mux_event, 1024, QSE_NULL); 
	if (!mux)
	{
		if (errno != 0)
			this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
		else
			this->setErrorNumber (E_ENOMEM);
		return -1;
	}
	mux_xtn_t* mux_xtn = (mux_xtn_t*)qse_mux_getxtn(mux);
	mux_xtn->server = this;
	mux_xtn->first_time = true;

	if (::pipe(pfd) <= -1)
	{
		this->setErrorFmt (syserr_to_errnum(errno), QSE_T("%hs"), strerror(errno));
		goto oops;
	}

#if defined(FD_CLOEXEC)
	fcv = ::fcntl(pfd[0], F_GETFD, 0);
	if (fcv >= 0) ::fcntl(pfd[0], F_SETFD, fcv | FD_CLOEXEC);
	fcv = ::fcntl(pfd[1], F_GETFD, 0);
	if (fcv >= 0) ::fcntl(pfd[1], F_SETFD, fcv | FD_CLOEXEC);
#endif
#if defined(O_NONBLOCK)
	fcv = ::fcntl(pfd[0], F_GETFL, 0);
	if (fcv >= 0) ::fcntl(pfd[0], F_SETFL, fcv | O_NONBLOCK);
	fcv = ::fcntl(pfd[1], F_GETFL, 0);
	if (fcv >= 0) ::fcntl(pfd[1], F_SETFL, fcv | O_NONBLOCK);
#endif

	QSE_MEMSET (&ev, 0, QSE_SIZEOF(ev));
	ev.hnd = pfd[0];
	ev.mask = QSE_MUX_IN;
	ev.data = QSE_NULL;
	if (qse_mux_insert(mux, &ev) <= -1)
	{
		this->setErrorNumber (E_ESYSERR);
		goto oops;
	}

	this->setErrorNumber (E_ENOERR);

	addr_ptr = addrs;
	while (1)
	{
		qse_size_t addr_len;
		Listener* lsck = QSE_NULL;
		Socket sock;

		comma = qse_strchr(addr_ptr, QSE_T(','));
		addr_len = comma? comma - addr_ptr: qse_strlen(addr_ptr);

		addr_ptr = strip_enclosing_spaces(addr_ptr, &addr_len);
		if (sockaddr.set(addr_ptr, addr_len) <= -1)
		{
			this->logfmt (QSE_LOG_ERROR, QSE_T("unrecognized listener address - %.*js\n"), (int)addr_len, addr_ptr);
			goto skip_segment;
		}

		try 
		{ 
			lsck = new(this->getMmgr()) Listener(this); 
		}
		catch (...) 
		{
			this->logfmt (QSE_LOG_ERROR, QSE_T("unable to instantiate listener\n"));
			goto skip_segment;
		}

		if (lsck->open(sockaddr.getFamily(), QSE_SOCK_STREAM, 0, Socket::T_CLOEXEC | Socket::T_NONBLOCK) <= -1)
		{
			int xerrno = errno;
			this->logfmt (QSE_LOG_ERROR, QSE_T("unable to open listener socket on %.*js on %hs\n"), (int)addr_len, addr_ptr, strerror(xerrno));
			this->setErrorFmt (syserr_to_errnum(xerrno), QSE_T("%hs"), strerror(xerrno));
			goto skip_segment;
		}

		lsck->setReuseAddr (1);
		lsck->setReusePort (1);

		if (lsck->bind(sockaddr) <= -1 || lsck->listen() <= -1)
		{
			int xerrno = errno;
			this->logfmt (QSE_LOG_ERROR, QSE_T("[l=%d] unable to bind/listen on %.*js - %hs\n"), (int)lsck->getHandle(), (int)addr_len, addr_ptr, strerror(xerrno));
			this->setErrorFmt (syserr_to_errnum(xerrno), QSE_T("%hs"), strerror(xerrno));
			goto skip_segment;
		}

		QSE_MEMSET (&ev, 0, QSE_SIZEOF(ev));
		ev.hnd = lsck->getHandle();
		ev.mask = QSE_MUX_IN;
		ev.data = lsck;
		if (qse_mux_insert(mux, &ev) <= -1)
		{
			this->logfmt (QSE_LOG_ERROR, QSE_T("[l=%d] unable to register listener on %.*js to multiplexer\n"), (int)ev.hnd, (int)addr_len, addr_ptr);
			goto skip_segment;
		}

		this->logfmt (QSE_LOG_INFO, QSE_T("[l=%d] listening on %.*js\n"), (int)ev.hnd, (int)addr_len, addr_ptr);
		lsck->address = sockaddr;
		lsck->next_listener = this->_listener_list.head;
		this->_listener_list.head = lsck;
		this->_listener_list.count++;
		goto segment_done;  // lsck has been added to the listener list. i must not close and destroy it

	skip_segment:
		if (lsck)
		{
			lsck->close();
			QSE_CPP_DELETE_WITH_MMGR (lsck, Listener, this->getMmgr());
			lsck = QSE_NULL;
		}

	segment_done:
		if (!comma) break;
		addr_ptr = comma + 1;
	}

	if (!this->_listener_list.head) 
	{
		if (this->getErrorNumber() == E_ENOERR)
		{
			this->setErrorFmt (E_EINVAL, QSE_T("unable to create liteners with %js"), addrs);
		}
		else
		{
			const qse_char_t* emb = this->backupErrorMsg();
			this->setErrorFmt (E_EINVAL, QSE_T("unable to create liteners with %js - %js"), addrs, emb);
		}
		goto oops;
	}

	this->_listener_list.mux = mux;
	this->_listener_list.mux_pipe[0] = pfd[0];
	this->_listener_list.mux_pipe[1] = pfd[1];

	return 0;

oops:
	if (this->_listener_list.head) this->free_all_listeners ();
	if (pfd[0] >= 0) close (pfd[0]);
	if (pfd[1] >= 0) close (pfd[1]);
	if (mux) qse_mux_close (mux);
	return -1;
}

int TcpServer::execute (const qse_char_t* addrs) QSE_CPP_NOEXCEPT
{
	int xret = 0;

	this->_server_serving = true;
	this->setHaltRequested (false);

	try 
	{
		if (this->setup_listeners(addrs) <= -1)
		{
			this->_server_serving = false;
			this->setHaltRequested (false);
			return -1;
		}

		mux_xtn_t* mux_xtn = (mux_xtn_t*)qse_mux_getxtn(this->_listener_list.mux);

		while (!this->isHaltRequested()) 
		{
			int n;

			mux_xtn->first_time = true;

			n = qse_mux_poll(this->_listener_list.mux, QSE_NULL);
			if (n <= -1)
			{
				qse_mux_errnum_t merr = qse_mux_geterrnum(this->_listener_list.mux);
				if (merr != QSE_MUX_EINTR)
				{
					this->setErrorNumber (E_ESYSERR); // TODO: proper error code conversion
					xret = -1;
					break;
				}
			}
		}

		this->delete_all_connections (Connection::LIVE);
		this->delete_all_connections (Connection::DEAD);
	}
	catch (...) 
	{
		this->delete_all_connections (Connection::LIVE);
		this->delete_all_connections (Connection::DEAD);

		this->setErrorNumber (E_EEXCEPT);
		this->_server_serving = false;
		this->setHaltRequested (false);
		this->free_all_listeners ();
		this->free_wid_map ();

		return -1;
	}

	this->_server_serving = false;
	this->setHaltRequested (false);
	this->free_all_listeners ();
	this->free_wid_map ();

	return xret;
}

int TcpServer::halt () QSE_CPP_NOEXCEPT
{
	if (this->_server_serving) 
	{
		// set halt request before writing "Q" to avoid race condition.
		// after qse_mux_poll() detects activity for "Q" written,
		// it loops over to another qse_mux_poll(). the stop request
		// test is done in between. if this looping is faster than
		// setting stop request after "Q" writing, the second qse_mux_poll()
		// doesn't see it set to true yet.
		this->setHaltRequested (true);

		this->_listener_list.mux_pipe_spl.lock ();
		if (this->_listener_list.mux_pipe[1] >= 0)
		{
			::write (this->_listener_list.mux_pipe[1], "Q", 1);
		}
		this->_listener_list.mux_pipe_spl.unlock ();
	}
	return 0;
}

void TcpServer::delete_all_connections (Connection::State state) QSE_CPP_NOEXCEPT
{
	Connection* connection;

	while (1)
	{
		this->_connection_list_spl.lock();
		connection = this->_connection_list[state].getHead();
		if (connection)
		{
			this->_connection_list[state].remove (connection);
			connection->claimed = true;
			connection->stop();
		}
		this->_connection_list_spl.unlock();
		if (!connection) break;

		connection->join ();

		this->release_wid (connection);
		QSE_CPP_DELETE_WITH_MMGR (connection, Connection, this->getMmgr()); // delete connection
	}
}

int TcpServer::prepare_to_acquire_wid () QSE_CPP_NOEXCEPT
{
	qse_size_t new_capa;
	qse_size_t i, j;
	_wid_map_data_t* tmp;

	QSE_ASSERT (this->_wid_map.free_first == _wid_map_t::WID_INVALID);
	QSE_ASSERT (this->_wid_map.free_last == _wid_map_t::WID_INVALID);

	new_capa = QSE_ALIGNTO_POW2(this->_wid_map.capa + 1, WID_MAP_ALIGN);
	if (new_capa > WID_MAX)
	{
		if (this->_wid_map.capa >= WID_MAX)
		{
			this->setErrorNumber (E_ENOMEM); // TODO: proper error code
			return -1;
		}

		new_capa = WID_MAX;
	}

	tmp = (_wid_map_data_t*)this->getMmgr()->reallocate(this->_wid_map.ptr, QSE_SIZEOF(*tmp) * new_capa, false);
	if (!tmp) 
	{
		this->setErrorNumber (E_ENOMEM);
		return -1;
	}

	this->_wid_map.free_first = this->_wid_map.capa;
	for (i = this->_wid_map.capa, j = this->_wid_map.capa + 1; j < new_capa; i++, j++)
	{
		tmp[i].used = 0;
		tmp[i].u.next = j;
	}
	tmp[i].used = 0;
	tmp[i].u.next = _wid_map_t::WID_INVALID;
	this->_wid_map.free_last = i;

	this->_wid_map.ptr = tmp;
	this->_wid_map.capa = new_capa;

	return 0;
}

void TcpServer::acquire_wid (Connection* connection) QSE_CPP_NOEXCEPT
{
	qse_size_t wid;

	wid = this->_wid_map.free_first;
	connection->wid = wid;

	this->_wid_map.free_first = this->_wid_map.ptr[wid].u.next;
	if (this->_wid_map.free_first == _wid_map_t::WID_INVALID) this->_wid_map.free_last = _wid_map_t::WID_INVALID;

	this->_wid_map.ptr[wid].used = 1;
	this->_wid_map.ptr[wid].u.connection = connection;
}

void TcpServer::release_wid (Connection* connection) QSE_CPP_NOEXCEPT
{
	qse_size_t wid;

	wid = connection->wid;
	QSE_ASSERT (wid < this->_wid_map.capa && wid != _wid_map_t::WID_INVALID);

	this->_wid_map.ptr[wid].used = 0;
	this->_wid_map.ptr[wid].u.next = _wid_map_t::WID_INVALID;
	if (this->_wid_map.free_last == _wid_map_t::WID_INVALID)
	{
		QSE_ASSERT (this->_wid_map.free_first <= _wid_map_t::WID_INVALID);
		this->_wid_map.free_first = wid;
	}
	else
	{
		this->_wid_map.ptr[this->_wid_map.free_last].u.next = wid;
	}
	this->_wid_map.free_last = wid;
	connection->wid = _wid_map_t::WID_INVALID;
}

void TcpServer::free_wid_map () QSE_CPP_NOEXCEPT
{
	if (this->_wid_map.ptr)
	{
		this->getMmgr()->dispose (this->_wid_map.ptr);
		this->_wid_map.capa = 0;
		this->_wid_map.free_first = _wid_map_t::WID_INVALID;
		this->_wid_map.free_last = _wid_map_t::WID_INVALID;
	}
}

QSE_END_NAMESPACE(QSE)
