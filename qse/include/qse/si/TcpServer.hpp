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


QSE_BEGIN_NAMESPACE(QSE)

// The TcpServer class implements a simple block TCP server that start a thread
// for each connection accepted.

class TcpServer: public QSE::Uncopyable 
{
public:
	TcpServer ();
	TcpServer (const SocketAddress& address);
	virtual ~TcpServer () QSE_CPP_NOEXCEPT;

	enum 
	{
		ERR_NONE      = 0,
		ERR_OPEN      = 1,
		ERR_BIND      = 2,
		ERR_LISTEN    = 3,
		ERR_EXCEPTION = 4
	};

	virtual int start (int* err_code = QSE_NULL) QSE_CPP_NOEXCEPT;
	virtual int stop () QSE_CPP_NOEXCEPT;

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

	const SocketAddress& getBindingAddress () const QSE_CPP_NOEXCEPT
	{
		return this->binding_address;
	}
	void setBindingAddress (const SocketAddress& address) QSE_CPP_NOEXCEPT
	{
		QSE_ASSERT (this->server_serving == false);
		this->binding_address = address;
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

	bool getReopenSocketUponError () const QSE_CPP_NOEXCEPT
	{
		return this->reopen_socket_upon_error;
	}

	void setReopenSocketUponError (bool v) QSE_CPP_NOEXCEPT
	{
		this->reopen_socket_upon_error = v;
	}

protected:
	class Listener: public QSE::Socket
	{
		Listener* next_listener;
	};

	class Client: public QSE::Thread 
	{
	public:
		friend class TcpServer;

		Client (TcpServer* server);

		int run ();
		int stop () QSE_CPP_NOEXCEPT;

	private:
		TcpServer* server;
		QSE::Socket  socket;
		SocketAddress address;
	};

	Listener* listener_head;
	Listener* listener_tail;

	SocketAddress binding_address;
	bool          stop_requested;
	bool          server_serving;
	qse_size_t    max_connections;
	qse_size_t    thread_stack_size;
	bool          reopen_socket_upon_error;

	typedef QSE::LinkedList<Client*> ClientList;
	ClientList client_list;

	friend class TcpServer::Client;
	virtual int handle_client (Socket* sock, SocketAddress* addr) = 0;

private:
	void delete_dead_clients () QSE_CPP_NOEXCEPT;
	void delete_all_clients  () QSE_CPP_NOEXCEPT;
	int open_tcp_socket (Socket& socket, int* err_code) QSE_CPP_NOEXCEPT;
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
	~TcpServerL () QSE_CPP_NOEXCEPT 
	{ 
		if (this->__lfunc) delete this->__lfunc; 
	}

	static int call_func (qse_thr_t* thr, void* ctx)
	{
		TcpServerL* t = (TcpServerL*)ctx;
		return t->__lfunc->invoke(t);
	}

	template <typename T>
	int handle_client (Socket* sock, SocketAddress* addr)
	{
		if (this->__state == QSE_THR_RUNNING) return -1;
		if (this->__lfunc) delete this->__lfunc;
		try
		{
			// TODO: are there any ways to achieve this without memory allocation?
			//this->__lfunc = new TCallable<T> (QSE_CPP_RVREF(f));
		// TODO:	this->__lfunc = new TCallable<T> (QSE_CPP_RVREF(f));
		}
		catch (...)
		{
			this->__lfunc = nullptr;
			return -1;
		}
		return this->__lfunc->invoke (sock, addr);
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
};

#endif

QSE_END_NAMESPACE(QSE)

#endif

