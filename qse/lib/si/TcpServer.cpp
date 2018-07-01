/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

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
#include "../cmn/mem-prv.h"

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>


QSE_BEGIN_NAMESPACE(QSE)

#include "../cmn/syserr.h"
IMPLEMENT_SYSERR_TO_ERRNUM (TcpServer::ErrorCode, TcpServer::)

int TcpServer::Client::main ()
{
	int n;

	// blockAllSignals is called inside run because 
	// Client is instantiated in the TcpServer thread.
	// so if it is called in the constructor of Client, 
	// it would just block signals to the TcpProxy thread.
	this->blockAllSignals (); // don't care about the result.

	try { n = this->listener->server->handle_client(&this->socket, &this->address); }
	catch (...) { n = -1; }

	this->csspl.lock ();
	this->socket.close (); 
	this->csspl.unlock ();

	TcpServer* server = this->getServer();

	server->client_list_spl.lock ();
	if (!this->claimed)
	{
		server->client_list[Client::LIVE].remove (this);
		server->client_list[Client::DEAD].append (this);
	}
	server->client_list_spl.unlock ();

	return n;
}

int TcpServer::Client::stop () QSE_CPP_NOEXCEPT
{
	// the receiver will be notified of the end of 
	// the connection by the socket's closing.
	// therefore, handle_client() must return
	// when it detects the end of the connection.
	//
	// TODO: must think of a better way to do this 
	//       as it might not be thread-safe.
	//       but it is still ok because Client::stop() 
	//       is rarely called.
	this->csspl.lock ();
	this->socket.shutdown ();
	this->csspl.unlock ();
	return 0;
}

TcpServer::TcpServer (Mmgr* mmgr) QSE_CPP_NOEXCEPT: 
	Mmged(mmgr),
	errcode(E_ENOERR),
	stop_requested(false), 
	server_serving(false), 
	max_connections(0),
	thread_stack_size(0)
{
}

TcpServer::~TcpServer () QSE_CPP_NOEXCEPT
{
	// QSE_ASSERT (this->server_serving == false);
	this->delete_all_clients (Client::LIVE);
	this->delete_all_clients (Client::DEAD);
}

void TcpServer::free_all_listeners () QSE_CPP_NOEXCEPT
{
	Listener* lp;

	while (this->listener_list.head)
	{
		lp = this->listener_list.head;
		this->listener_list.head = lp->next_listener;
		this->listener_list.count--;

		qse_mux_evt_t evt;
		evt.hnd = lp->getHandle();
		qse_mux_delete (this->listener_list.mux, &evt);

		lp->close ();

		QSE_CPP_DELETE_WITH_MMGR(lp, Listener, this->getMmgr()); // delete lp
	}

	if (this->listener_list.mux_pipe[0] >= 0)
	{
		qse_mux_evt_t evt;
		evt.hnd = this->listener_list.mux_pipe[0];
		qse_mux_delete (this->listener_list.mux, &evt);

		close (this->listener_list.mux_pipe[0]);
		this->listener_list.mux_pipe[0] = -1;
	}

	this->listener_list.mux_pipe_spl.lock ();
	if (this->listener_list.mux_pipe[1] >= 0)
	{
		close (this->listener_list.mux_pipe[1]);
		this->listener_list.mux_pipe[1] = -1;
	}
	this->listener_list.mux_pipe_spl.unlock ();

	QSE_ASSERT (this->listener_list.mux != QSE_NULL);
	qse_mux_close (this->listener_list.mux);
	this->listener_list.mux = QSE_NULL;
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
		server->delete_all_clients(Client::DEAD);
		mux_xtn->first_time = false;
	}

	if (!evt->mask) return;

	if (evt->data == NULL)
	{
		/* just consume data written by TcpServer::stop() */
		char tmp[128];
		while (::read(server->listener_list.mux_pipe[0], tmp, QSE_SIZEOF(tmp)) > 0) /* nothing */;
	}
	else
	{
		/* the reset should be the listener's socket */
		Listener* lsck = (Listener*)evt->data;

		if (server->max_connections > 0 && server->max_connections <= server->client_list[Client::LIVE].getSize()) 
		{
			// too many connections. accept the connection and close it.

			Socket s;
			SocketAddress sa;
			if (lsck->accept(&s, &sa, Socket::T_CLOEXEC) >= 0) s.close();
			// TODO: logging.
			return;
		}

		Client* client;

		// allocating the client object before accept is 
		// a bit awkward. but socket.accept() can be passed
		// the socket field inside the client object.
		try { client = new(server->getMmgr()) Client(lsck); } 
		catch (...) 
		{
			// memory alloc failed. accept the connection and close it.

			Socket s;
			SocketAddress sa;
			if (lsck->accept(&s, &sa, Socket::T_CLOEXEC) >= 0) s.close();
			// TODO: logging.
			return;
		}

		if (lsck->accept(&client->socket, &client->address, Socket::T_CLOEXEC) <= -1)
		{
			QSE_CPP_DELETE_WITH_MMGR (client, Client, server->getMmgr());

			if (server->isStopRequested()) return; /* normal termination requested */

			Socket::ErrorCode lerr = lsck->getErrorCode();
			if (lerr == Socket::E_EINTR || lerr == Socket::E_EAGAIN) return;

			server->setErrorCode (lerr);
			server->stop ();
			return;
		}

		server->client_list_spl.lock ();
		server->client_list[Client::LIVE].append (client); 
		server->client_list_spl.unlock ();

		client->setStackSize (server->thread_stack_size);
	#if defined(_WIN32)
		if (client->start(Thread::DETACHED) <= -1) 
	#else
		if (client->start(0) <= -1)
	#endif
		{
			// TODO: logging.

			server->client_list_spl.lock ();
			server->client_list[Client::LIVE].remove (client);
			server->client_list_spl.unlock ();
			QSE_CPP_DELETE_WITH_MMGR (client, Client, server->getMmgr());
			return;
		}
	}
}

int TcpServer::setup_listeners (const qse_char_t* addrs) QSE_CPP_NOEXCEPT
{
	const qse_char_t* addr_ptr, * comma;
	qse_mux_t* mux = QSE_NULL;
	qse_mux_evt_t ev;
	int fcv, pfd[2] = { -1, - 1 };
	SocketAddress sockaddr;

	mux = qse_mux_open(this->getMmgr(), QSE_SIZEOF(mux_xtn_t), TcpServer::dispatch_mux_event, 1024, QSE_NULL); 
	if (!mux)
	{
		this->setErrorCode (syserr_to_errnum(errno));
		return -1;
	}
	mux_xtn_t* mux_xtn = (mux_xtn_t*)qse_mux_getxtn(mux);
	mux_xtn->server = this;
	mux_xtn->first_time = true;

	if (::pipe(pfd) <= -1)
	{
		this->setErrorCode (syserr_to_errnum(errno));
		goto oops;
	}

#if defined(O_CLOEXEC)
	fcv = ::fcntl(pfd[0], F_GETFD, 0);
	if (fcv >= 0) ::fcntl(pfd[0], F_SETFD, fcv | O_CLOEXEC);
	fcv = ::fcntl(pfd[1], F_GETFD, 0);
	if (fcv >= 0) ::fcntl(pfd[1], F_SETFD, fcv | O_CLOEXEC);
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
		this->setErrorCode (E_ESYSERR);
		goto oops;
	}

	addr_ptr = addrs;
	while (1)
	{
		qse_size_t addr_len;
		Listener* lsck;
		Socket sock;

		comma = qse_strchr(addr_ptr, QSE_T(','));
		addr_len = comma? comma - addr_ptr: qse_strlen(addr_ptr);
		/* [NOTE] no whitespaces are allowed before and after a comma */

		if (sockaddr.set(addr_ptr, addr_len) <= -1)
		{
			/* TOOD: logging */
			goto next_segment;
		}

		try 
		{ 
			lsck = new(this->getMmgr()) Listener(this); 
		}
		catch (...) 
		{
			/* TODO: logging... */
			goto next_segment;
		}

		if (lsck->open(sockaddr.getFamily(), QSE_SOCK_STREAM, 0, Socket::T_CLOEXEC | Socket::T_NONBLOCK) <= -1)
		{
			/* TODO: logging */
			goto next_segment;
		}

		lsck->setReuseAddr (1);
		lsck->setReusePort (1);

		if (lsck->bind(sockaddr) <= -1 || lsck->listen() <= -1)
		{
			/* TODO: logging */
			lsck->close ();
			goto next_segment;
		}

		QSE_MEMSET (&ev, 0, QSE_SIZEOF(ev));
		ev.hnd = lsck->getHandle();
		ev.mask = QSE_MUX_IN;
		ev.data = lsck;
		if (qse_mux_insert(mux, &ev) <= -1)
		{
			/* TODO: logging */
			lsck->close ();
			goto next_segment;
		}

		lsck->address = sockaddr;
		lsck->next_listener = this->listener_list.head;
		this->listener_list.head = lsck;
		this->listener_list.count++;

	next_segment:
		if (!comma) break;
		addr_ptr = comma + 1;
	}

	if (!this->listener_list.head) goto oops;
	
	this->listener_list.mux = mux;
	this->listener_list.mux_pipe[0] = pfd[0];
	this->listener_list.mux_pipe[1] = pfd[1];

	return 0;

oops:
	if (this->listener_list.head) this->free_all_listeners ();
	if (pfd[0] >= 0) close (pfd[0]);
	if (pfd[1] >= 0) close (pfd[1]);
	if (mux) qse_mux_close (mux);
	return -1;
}

int TcpServer::start (const qse_char_t* addrs) QSE_CPP_NOEXCEPT
{
	int xret = 0;

	this->server_serving = true;
	this->setStopRequested (false);

	try 
	{
		if (this->setup_listeners(addrs) <= -1)
		{
			this->server_serving = false;
			this->setStopRequested (false);
			return -1;
		}

		mux_xtn_t* mux_xtn = (mux_xtn_t*)qse_mux_getxtn(this->listener_list.mux);

		while (!this->isStopRequested()) 
		{
			int n;

			mux_xtn->first_time = true;

			n = qse_mux_poll (this->listener_list.mux, QSE_NULL);
			if (n <= -1)
			{
				this->setErrorCode (E_ESYSERR); // TODO: proper error code conversion
				xret = -1;
				break;
			}
		}

		this->delete_all_clients (Client::LIVE);
		this->delete_all_clients (Client::DEAD);
	}
	catch (...) 
	{
		this->delete_all_clients (Client::LIVE);
		this->delete_all_clients (Client::DEAD);

		this->setErrorCode (E_EEXCEPT);
		this->server_serving = false;
		this->setStopRequested (false);
		this->free_all_listeners ();

		return -1;
	}

	this->server_serving = false;
	this->setStopRequested (false);
	this->free_all_listeners ();

	return xret;
}

int TcpServer::stop () QSE_CPP_NOEXCEPT
{
	if (this->server_serving) 
	{
		this->listener_list.mux_pipe_spl.lock ();
		if (this->listener_list.mux_pipe[1] >= 0)
		{
			::write (this->listener_list.mux_pipe[1], "Q", 1);
		}
		this->listener_list.mux_pipe_spl.unlock ();
		this->setStopRequested (true);
	}
	return 0;
}

void TcpServer::delete_all_clients (Client::State state) QSE_CPP_NOEXCEPT
{
	Client* np;

	while (1)
	{
		this->client_list_spl.lock();
		np = this->client_list[state].getHead();
		if (np)
		{
			this->client_list[state].remove (np);
			np->claimed = true;
		}
		this->client_list_spl.unlock();
		if (!np) break;

		np->stop();
		np->join ();
		QSE_CPP_DELETE_WITH_MMGR (np, Client, this->getMmgr()); // delete np
	}
}

QSE_END_NAMESPACE(QSE)
