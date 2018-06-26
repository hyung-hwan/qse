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

#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

QSE_BEGIN_NAMESPACE(QSE)


#include "../cmn/syserr.h"
IMPLEMENT_SYSERR_TO_ERRNUM (TcpServer::ErrorCode, TcpServer::)

TcpServer::Client::Client (TcpServer* server) 
{
	this->server = server;
}

//
// NOTICE: the guarantee class below could have been placed 
//         inside TCPServer::Client::run () without supporting 
//         old C++ compilers.
//
class guarantee_tcpsocket_close {
public:
	guarantee_tcpsocket_close (Socket* socket): psck (socket) {}
	~guarantee_tcpsocket_close () { psck->shutdown (); psck->close (); }
	Socket* psck;
};

int TcpServer::Client::main ()
{
	// blockAllSignals is called inside run because 
	// Client is instantiated in the TcpServer thread.
	// so if it is called in the constructor of Client, 
	// it would just block signals to the TcpProxy thread.
	this->blockAllSignals (); // don't care about the result.

	guarantee_tcpsocket_close close_socket (&this->socket);
	if (server->handle_client(&this->socket, &this->address) <= -1) return -1;
	return 0;
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
	this->socket.shutdown ();
	this->socket.close ();
	return 0;
}

TcpServer::TcpServer () QSE_CPP_NOEXCEPT: 
	errcode(E_ENOERR),
	stop_requested(false), 
	server_serving(false), 
	max_connections(0),
	thread_stack_size (0)
{
}

TcpServer::~TcpServer () QSE_CPP_NOEXCEPT
{
	// QSE_ASSERT (this->server_serving == false);
	this->delete_all_clients ();
}

void TcpServer::free_all_listeners () QSE_CPP_NOEXCEPT
{
	Listener* lp;
	struct epoll_event dummy_ev;

	while (this->listener.head)
	{
		lp = this->listener.head;
		this->listener.head = lp->next_listener;
		this->listener.count--;

		::epoll_ctl (this->listener.ep_fd, EPOLL_CTL_DEL, lp->getHandle(), &dummy_ev);
		lp->close ();
		delete lp;
	}

	if (this->listener.mux_pipe[0] >= 0)
	{
		::epoll_ctl (this->listener.ep_fd, EPOLL_CTL_DEL, this->listener.mux_pipe[0], &dummy_ev);
		close (this->listener.mux_pipe[0]);
		this->listener.mux_pipe[0] = -1;
	}
	if (this->listener.mux_pipe[1] >= 0)
	{
		close (this->listener.mux_pipe[1]);
		this->listener.mux_pipe[1] = -1;
	}
	QSE_ASSERT (this->listener.ep_fd >= 0);

	::close (this->listener.ep_fd);
	this->listener.ep_fd = -1;
}

int TcpServer::setup_listeners (const qse_char_t* addrs) QSE_CPP_NOEXCEPT
{
	const qse_char_t* addr_ptr, * comma;
	int ep_fd = -1, fcv;
	struct epoll_event ev;
	int pfd[2] = { -1, - 1 };
	SocketAddress sockaddr;

	ep_fd = ::epoll_create(1024);
	if (ep_fd <= -1)
	{
		this->setErrorCode (syserr_to_errnum(errno));
		return -1;
	}

#if defined(O_CLOEXEC)
	fcv = ::fcntl(ep_fd, F_GETFD, 0);
	if (fcv >= 0) ::fcntl(ep_fd, F_SETFD, fcv | O_CLOEXEC);
#endif

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
	ev.events = EPOLLIN | EPOLLHUP | EPOLLERR;
	ev.data.ptr = QSE_NULL;
	if (::epoll_ctl(ep_fd, EPOLL_CTL_ADD, pfd[0], &ev) <= -1)
	{
		this->setErrorCode (syserr_to_errnum(errno));
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
			lsck = new Listener(); 
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
		ev.events = EPOLLIN | EPOLLHUP | EPOLLERR;
		ev.data.ptr = lsck;
		if (::epoll_ctl(ep_fd, EPOLL_CTL_ADD, lsck->getHandle(), &ev) <= -1)
		{
			/* TODO: logging */
			lsck->close ();
			goto next_segment;
		}

		lsck->address = sockaddr;
		lsck->next_listener = this->listener.head;
		this->listener.head = lsck;
		this->listener.count++;

	next_segment:
		if (!comma) break;
		addr_ptr = comma + 1;
	}

	if (!this->listener.head) goto oops;
	
	this->listener.ep_fd = ep_fd;
	this->listener.mux_pipe[0] = pfd[0];
	this->listener.mux_pipe[1] = pfd[1];

	return 0;

oops:
	if (this->listener.head) this->free_all_listeners ();
	if (pfd[0] >= 0) close (pfd[0]);
	if (pfd[1] >= 0) close (pfd[1]);
	if (ep_fd >= 0) close (ep_fd);
	return -1;
}

int TcpServer::start (const qse_char_t* addrs) QSE_CPP_NOEXCEPT
{
	struct epoll_event ev_buf[128];
	int xret = 0;

	this->server_serving = true;
	this->setStopRequested (false);

	Client* client = QSE_NULL;

	try 
	{
		Socket socket;

		if (this->setup_listeners(addrs) <= -1)
		{
			this->server_serving = false;
			this->setStopRequested (false);
			return -1;
		}

		while (!this->isStopRequested()) 
		{
			int n;

			n = ::epoll_wait (this->listener.ep_fd, ev_buf, QSE_COUNTOF(ev_buf), -1);
			this->delete_dead_clients ();
			if (n <= -1)
			{
				if (this->isStopRequested()) break;
				if (errno == EINTR) continue;

				this->setErrorCode (syserr_to_errnum(errno));
				xret = -1;
				break;
			}


			while (n > 0)
			{
				struct epoll_event* evp;

				--n;

				evp = &ev_buf[n];
				if (!evp->events /*& (POLLIN | POLLHUP | POLLERR) */) continue;

				if (evp->data.ptr == NULL)
				{
					char tmp[128];
					while (::read(this->listener.mux_pipe[0], tmp, QSE_SIZEOF(tmp)) > 0) /* nothing */;
				}
				else
				{
					/* the reset should be the listener's socket */
					Listener* lsck = (Listener*)evp->data.ptr;


					if (this->max_connections > 0 && this->max_connections <= this->client_list.getSize()) 
					{
						// too many connections. accept the connection and close it.
						Socket s;
						SocketAddress sa;
						if (socket.accept(&s, &sa, Socket::T_CLOEXEC) >= 0) s.close();
						continue;
					}

					if (client == QSE_NULL)
					{
						// allocating the client object before accept is 
						// a bit awkward. but socket.accept() can be passed
						// the socket field inside the client object.
						try { client = new Client (this); } 
						catch (...) { }
					}
					if (client == QSE_NULL) 
					{
						// memory alloc failed. accept the connection and close it.
						Socket s;
						SocketAddress sa;
						if (lsck->accept(&s, &sa, Socket::T_CLOEXEC) >= 0) s.close();
						continue;
					}

					if (lsck->accept(&client->socket, &client->address, Socket::T_CLOEXEC) <= -1)
					{
						if (this->isStopRequested()) break; /* normal termination requested */

						Socket::ErrorCode lerr = lsck->getErrorCode();
						if (lerr == Socket::E_EINTR || lerr == Socket::E_EAGAIN) continue;

						this->setErrorCode (lerr);
						xret = -1;
						break;
					}

					client->setStackSize (this->thread_stack_size);
				#if defined(_WIN32)
					if (client->start(Thread::DETACHED) <= -1) 
				#else
					if (client->start(0) <= -1)
				#endif
					{
						delete client; 
						client = QSE_NULL;
						continue;
					}

					this->client_list.append (client);
					client = QSE_NULL;
				}
			}
		}

		this->delete_all_clients ();
		if (client != QSE_NULL) delete client;
	}
	catch (...) 
	{
		this->delete_all_clients ();
		if (client != QSE_NULL) delete client;

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
// TODO: mutex
		if (this->listener.mux_pipe[1] >= 0)
		{
			::write (this->listener.mux_pipe[1], "Q", 1);
		}
// TODO: mutex
		this->setStopRequested (true);
	}
	return 0;
}

void TcpServer::delete_dead_clients () QSE_CPP_NOEXCEPT
{
	ClientList::Node* np, * np2;
	
	np = this->client_list.getHeadNode();
	while (np) 
	{
		Client* p = np->value;
		QSE_ASSERT (p != QSE_NULL);

		if (p->getState() != Thread::RUNNING)
		{
		#if !defined(_WIN32)
			p->join ();
		#endif

			delete p;
			np2 = np; np = np->getNextNode();
			this->client_list.remove (np2);
			continue;
		}

		np = np->getNextNode();
	}
}

void TcpServer::delete_all_clients () QSE_CPP_NOEXCEPT
{
	ClientList::Node* np, * np2;
	Client* p;

	for (np = this->client_list.getHeadNode(); np; np = np->getNextNode()) 
	{
		p = np->value;
		if (p->getState() == Thread::RUNNING) p->stop();
	}

	np = this->client_list.getHeadNode();
	while (np != QSE_NULL) 
	{
		p = np->value;
		QSE_ASSERT (p != QSE_NULL);

	#if defined(_WIN32)
		while (p->state() == Thread::RUNNING) qse_sleep (300);
	#else
		p->join ();
	#endif
		delete p;
		np2 = np; np = np->getNextNode();
		this->client_list.remove (np2);
	}
}

QSE_END_NAMESPACE(QSE)
