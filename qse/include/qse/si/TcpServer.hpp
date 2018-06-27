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

#ifndef _QSE_SI_TCPSERVER_CLASS_
#define _QSE_SI_TCPSERVER_CLASS_

#include <qse/si/Socket.hpp>
#include <qse/si/SocketAddress.hpp>
#include <qse/si/Thread.hpp>
#include <qse/cmn/LinkedList.hpp>
#include <qse/Uncopyable.hpp>
#include <qse/si/mtx.h>

QSE_BEGIN_NAMESPACE(QSE)

// The TcpServer class implements a simple block TCP server that start a thread
// for each connection accepted.

class TcpServer: public Uncopyable, public Types
{
public:
	TcpServer () QSE_CPP_NOEXCEPT;
	virtual ~TcpServer () QSE_CPP_NOEXCEPT;

	virtual int start (const qse_char_t* addrs) QSE_CPP_NOEXCEPT;
	virtual int stop () QSE_CPP_NOEXCEPT;

	ErrorCode getErrorCode () const QSE_CPP_NOEXCEPT { return this->errcode; }
	void setErrorCode (ErrorCode errcode) QSE_CPP_NOEXCEPT { this->errcode = errcode; }

	bool isServing () const QSE_CPP_NOEXCEPT
	{ 
		return this->server_serving; 
	}

	bool isStopRequested () const QSE_CPP_NOEXCEPT
	{
		return this->stop_requested;
	}
	void setStopRequested (bool req) QSE_CPP_NOEXCEPT
	{
		this->stop_requested = req;
	}

	qse_size_t getMaxConnections () const QSE_CPP_NOEXCEPT
	{
		return this->max_connections;
	}
	void setMaxConnections (qse_size_t mc) QSE_CPP_NOEXCEPT
	{
		// don't disconnect client connections 
		// establised before maxConn is set.
		// 0 means there's no restriction over 
		// the number of connection.
		this->max_connections = mc;
	}

	qse_size_t getClientCount () const QSE_CPP_NOEXCEPT
	{ 
		return this->client_list.getSize(); 
	}
	qse_size_t getConnectionCount () const QSE_CPP_NOEXCEPT
	{
		return this->client_list.getSize(); 
	}

	qse_size_t getThreadStackSize () const QSE_CPP_NOEXCEPT
	{
		return this->thread_stack_size;
	}

	void setThreadStackSize (qse_size_t tss) QSE_CPP_NOEXCEPT
	{
		this->thread_stack_size = tss;
	}

protected:
	class Listener: public QSE::Socket
	{
	public:
		Listener(TcpServer* server) QSE_CPP_NOEXCEPT : server(server), next_listener(QSE_NULL) {}

		TcpServer* server;
		SocketAddress address;
		Listener* next_listener;
	};

	class Client: public QSE::Thread 
	{
	public:
		friend class TcpServer;

		Client (Listener* listener) QSE_CPP_NOEXCEPT : listener(listener) {}
		~Client ();

		int main ();
		int stop () QSE_CPP_NOEXCEPT;

		Listener* getListener() QSE_CPP_NOEXCEPT { return this->listener; }
		const Listener* getListener() const QSE_CPP_NOEXCEPT { return this->listener; }

		TcpServer* getServer() QSE_CPP_NOEXCEPT { return this->listener->server; }
		const TcpServer* getServer() const QSE_CPP_NOEXCEPT { return this->listener->server; }

	private:
		Listener* listener;
		QSE::Socket socket;
		SocketAddress address;

		qse_mtx_t* csmtx; /* mutex for client stop */
	};

	struct ListenerList
	{
		ListenerList(): ep_fd(-1), head(QSE_NULL), tail(QSE_NULL), count(0)
		{
			this->mux_pipe[0] = -1;
			this->mux_pipe[1] = -1;
		}

		int ep_fd;
		int mux_pipe[2];

		Listener* head;
		Listener* tail;

		qse_size_t count;
	} listener;

	ErrorCode errcode;
	bool          stop_requested;
	bool          server_serving;
	qse_size_t    max_connections;
	qse_size_t    thread_stack_size;

	typedef QSE::LinkedList<Client*> ClientList;
	ClientList client_list;

	friend class TcpServer::Client;
	virtual int handle_client (Socket* sock, SocketAddress* addr) = 0;

private:
	void delete_dead_clients () QSE_CPP_NOEXCEPT;
	void delete_all_clients  () QSE_CPP_NOEXCEPT;

	int setup_listeners (const qse_char_t* addrs) QSE_CPP_NOEXCEPT;
	void free_all_listeners () QSE_CPP_NOEXCEPT;
};


// functor as a template parameter
template <typename F>
class TcpServerF: public TcpServer
{
public:
	TcpServerF () QSE_CPP_NOEXCEPT {}
	TcpServerF (const F& f) QSE_CPP_NOEXCEPT: __lfunc(f) {}
#if defined(QSE_CPP_ENABLE_CPP11_MOVE)
	TcpServerF (F&& f) QSE_CPP_NOEXCEPT: __lfunc(QSE_CPP_RVREF(f)) {}
#endif

protected:
	F __lfunc;

	int handle_client (Socket* sock, SocketAddress* addr)
	{
		return this->__lfunc(sock, addr);
	}
};


#if defined(QSE_LANG_CPP11)

template <typename T>
class TcpServerL;

template <typename RT, typename... ARGS>
class TcpServerL<RT(ARGS...)>: public TcpServer
{
public:
	TcpServerL () QSE_CPP_NOEXCEPT: __lfunc(nullptr) {}

	template <typename T>
	TcpServerL (T&& f) QSE_CPP_NOEXCEPT: __lfunc(nullptr)
	{
		try
		{
			// TODO: are there any ways to achieve this without memory allocation?
			this->__lfunc = new TCallable<T> (QSE_CPP_RVREF(f));
		}
		catch (...)
		{
			// upon failure, i set this->__lfunc to null.
			// this->handle_client() will return failure for this.
			this->__lfunc = nullptr;
		}
	}

	~TcpServerL () QSE_CPP_NOEXCEPT 
	{ 
		if (this->__lfunc) delete this->__lfunc; 
	}

	template <typename T>
	int setClientHandler (T&& f) QSE_CPP_NOEXCEPT
	{
		Callable* lf;

		try
		{
			// TODO: are there any ways to achieve this without memory allocation?
			lf = new TCallable<T> (QSE_CPP_RVREF(f));
		}
		catch (...)
		{
			return -1;
		}

		if (this->__lfunc) delete this->__lfunc;
		this->__lfunc = lf;
		return 0;
	}

protected:
	class Callable
	{
	public:
		virtual ~Callable () QSE_CPP_NOEXCEPT {};
		virtual RT invoke (ARGS... args) = 0;
	};

	template <typename T>
	class TCallable: public Callable
	{
	public:
		TCallable (const T& t) QSE_CPP_NOEXCEPT: t(t) { }
		~TCallable () QSE_CPP_NOEXCEPT {}
		RT invoke (ARGS... args) { return this->t(args ...); }

	private:
		T t;
	};

	Callable* __lfunc;


	int handle_client (Socket* sock, SocketAddress* addr)
	{
		if (!this->__lfunc)
		{
			//this->setErrorCode (TcpServer::E_ENOMEM or E_EINVAL??);
			return -1;
		}

		return this->__lfunc->invoke(sock, addr);
	}
};

#endif

QSE_END_NAMESPACE(QSE)

#endif

