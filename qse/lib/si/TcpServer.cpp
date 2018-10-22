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


#define WID_MAP_ALIGN 128
#define WID_MAX (wid_map_t::WID_INVALID - 1)

QSE_BEGIN_NAMESPACE(QSE)

#include "../cmn/syserr.h"
IMPLEMENT_SYSERR_TO_ERRNUM (TcpServer::ErrorCode, TcpServer::)

int TcpServer::Worker::main ()
{
	int n;

	// blockAllSignals is called inside run because 
	// Worker is instantiated in the TcpServer thread.
	// so if it is called in the constructor of Worker, 
	// it would just block signals to the TcpProxy thread.
	this->blockAllSignals (); // don't care about the result.

	try { n = this->listener->server->handle_worker(this); }
	catch (...) { n = -1; }

	TcpServer* server = this->getServer();

	server->worker_list_spl.lock ();
	this->csspl.lock ();
	this->socket.close (); 
	this->csspl.unlock ();
	if (!this->claimed)
	{
		server->worker_list[Worker::LIVE].remove (this);
		server->worker_list[Worker::DEAD].append (this);
	}
	server->worker_list_spl.unlock ();

	return n;
}

int TcpServer::Worker::stop () QSE_CPP_NOEXCEPT
{
	// the receiver will be notified of the end of 
	// the connection by the socket's closing.
	// therefore, handle_worker() must return
	// when it detects the end of the connection.
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
	this->delete_all_workers (Worker::LIVE);
	this->delete_all_workers (Worker::DEAD);
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
		server->delete_all_workers(Worker::DEAD);
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

		if (server->max_connections > 0 && server->max_connections <= server->worker_list[Worker::LIVE].getSize()) 
		{
			// too many connections. accept the connection and close it.
			// TODO: logging.
			goto accept_and_drop;
		}

		Worker* worker;

		// allocating the worker object before accept is 
		// a bit awkward. but socket.accept() can be passed
		// the socket field inside the worker object.
		try { worker = new(server->getMmgr()) Worker(lsck); } 
		catch (...) 
		{
			// memory alloc failed. accept the connection and close it.
			// TODO: logging.
			goto accept_and_drop;
		}
		if (server->wid_map.free_first == wid_map_t::WID_INVALID && server->prepare_to_acquire_wid() <= -1)
		{
			QSE_CPP_DELETE_WITH_MMGR (worker, Worker, server->getMmgr());
			// TODO: logging
			goto accept_and_drop;
		}

		if (lsck->accept(&worker->socket, &worker->address, Socket::T_CLOEXEC) <= -1)
		{
			QSE_CPP_DELETE_WITH_MMGR (worker, Worker, server->getMmgr());

			if (server->isStopRequested()) return; /* normal termination requested */

			Socket::ErrorCode lerr = lsck->getErrorCode();
			if (lerr == Socket::E_EINTR || lerr == Socket::E_EAGAIN) return;

			server->setErrorCode (lerr);
			server->stop ();
			return;
		}

		server->worker_list_spl.lock ();
		server->worker_list[Worker::LIVE].append (worker); 
		server->worker_list_spl.unlock ();

		server->acquire_wid (worker);
		worker->setStackSize (server->thread_stack_size);
	#if defined(_WIN32)
		if (worker->start(Thread::DETACHED) <= -1) 
	#else
		if (worker->start(0) <= -1)
	#endif
		{
			// TODO: logging.

			server->worker_list_spl.lock ();
			server->worker_list[Worker::LIVE].remove (worker);
			server->worker_list_spl.unlock ();

			server->release_wid (worker);
			QSE_CPP_DELETE_WITH_MMGR (worker, Worker, server->getMmgr());
			return;
		}

		return;

	accept_and_drop:
		Socket s;
		SocketAddress sa;
		if (lsck->accept(&s, &sa, Socket::T_CLOEXEC) >= 0) s.close();
		// TODO: logging.
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

			n = qse_mux_poll(this->listener_list.mux, QSE_NULL);
			if (n <= -1)
			{
				this->setErrorCode (E_ESYSERR); // TODO: proper error code conversion
				xret = -1;
				break;
			}
		}

		this->delete_all_workers (Worker::LIVE);
		this->delete_all_workers (Worker::DEAD);
	}
	catch (...) 
	{
		this->delete_all_workers (Worker::LIVE);
		this->delete_all_workers (Worker::DEAD);

		this->setErrorCode (E_EEXCEPT);
		this->server_serving = false;
		this->setStopRequested (false);
		this->free_all_listeners ();
		this->free_wid_map ();

		return -1;
	}

	this->server_serving = false;
	this->setStopRequested (false);
	this->free_all_listeners ();
	this->free_wid_map ();

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

void TcpServer::delete_all_workers (Worker::State state) QSE_CPP_NOEXCEPT
{
	Worker* worker;

	while (1)
	{
		this->worker_list_spl.lock();
		worker = this->worker_list[state].getHead();
		if (worker)
		{
			this->worker_list[state].remove (worker);
			worker->claimed = true;
			worker->stop();
		}
		this->worker_list_spl.unlock();
		if (!worker) break;

		worker->join ();

		this->release_wid (worker);
		QSE_CPP_DELETE_WITH_MMGR (worker, Worker, this->getMmgr()); // delete worker
	}
}

int TcpServer::prepare_to_acquire_wid () QSE_CPP_NOEXCEPT
{
	qse_size_t new_capa;
	qse_size_t i, j;
	wid_map_data_t* tmp;

	QSE_ASSERT (this->wid_map.free_first == wid_map_t::WID_INVALID);
	QSE_ASSERT (this->wid_map.free_last == wid_map_t::WID_INVALID);

	new_capa = QSE_ALIGNTO_POW2(this->wid_map.capa + 1, WID_MAP_ALIGN);
	if (new_capa > WID_MAX)
	{
		if (this->wid_map.capa >= WID_MAX)
		{
			this->setErrorCode (E_ENOMEM); // TODO: proper error code
			return -1;
		}

		new_capa = WID_MAX;
	}

	tmp = (wid_map_data_t*)QSE_MMGR_REALLOC(this->getMmgr(), this->wid_map.ptr, QSE_SIZEOF(*tmp) * new_capa);
	if (!tmp) 
	{
		this->setErrorCode (E_ENOMEM);
		return -1;
	}

	this->wid_map.free_first = this->wid_map.capa;
	for (i = this->wid_map.capa, j = this->wid_map.capa + 1; j < new_capa; i++, j++)
	{
		tmp[i].used = 0;
		tmp[i].u.next = j;
	}
	tmp[i].used = 0;
	tmp[i].u.next = wid_map_t::WID_INVALID;
	this->wid_map.free_last = i;

	this->wid_map.ptr = tmp;
	this->wid_map.capa = new_capa;

	return 0;
}

void TcpServer::acquire_wid (Worker* worker) QSE_CPP_NOEXCEPT
{
	qse_size_t wid;

	wid = this->wid_map.free_first;
	worker->wid = wid;

	this->wid_map.free_first = this->wid_map.ptr[wid].u.next;
	if (this->wid_map.free_first == wid_map_t::WID_INVALID) this->wid_map.free_last = wid_map_t::WID_INVALID;

	this->wid_map.ptr[wid].used = 1;
	this->wid_map.ptr[wid].u.worker = worker;
}

void TcpServer::release_wid (Worker* worker) QSE_CPP_NOEXCEPT
{
	qse_size_t wid;

	wid = worker->wid;
	QSE_ASSERT (wid < this->wid_map.capa && wid != wid_map_t::WID_INVALID);

	this->wid_map.ptr[wid].used = 0;
	this->wid_map.ptr[wid].u.next = wid_map_t::WID_INVALID;
	if (this->wid_map.free_last == wid_map_t::WID_INVALID)
	{
		QSE_ASSERT (this->wid_map.free_first <= wid_map_t::WID_INVALID);
		this->wid_map.free_first = wid;
	}
	else
	{
		this->wid_map.ptr[this->wid_map.free_last].u.next = wid;
	}
	this->wid_map.free_last = wid;
	worker->wid = wid_map_t::WID_INVALID;
}

void TcpServer::free_wid_map () QSE_CPP_NOEXCEPT
{
	if (this->wid_map.ptr)
	{
		QSE_MMGR_FREE (this->getMmgr(), this->wid_map.ptr);
		this->wid_map.capa = 0;
		this->wid_map.free_first = wid_map_t::WID_INVALID;
		this->wid_map.free_last = wid_map_t::WID_INVALID;
	}
}

QSE_END_NAMESPACE(QSE)
