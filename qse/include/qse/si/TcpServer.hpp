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

#ifndef _QSE_SI_TCPSERVER_CLASS_
#define _QSE_SI_TCPSERVER_CLASS_

#include <qse/si/Socket.hpp>
#include <qse/si/SocketAddress.hpp>
#include <qse/si/Thread.hpp>
#include <qse/si/SpinLock.hpp>
#include <qse/cmn/Mmged.hpp>
#include <qse/Uncopyable.hpp>
#include <qse/cmn/ErrorGrab.hpp>
#include <qse/si/mux.h>

QSE_BEGIN_NAMESPACE(QSE)

// The TcpServer class implements a simple block TCP server that start a thread
// for each connection accepted.

class QSE_EXPORT TcpServer: public Uncopyable, public Mmged, public Types, public ErrorGrab256
{
public:
	TcpServer (Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT;
	virtual ~TcpServer () QSE_CPP_NOEXCEPT;

	virtual int start (const qse_char_t* addrs) QSE_CPP_NOEXCEPT;
	virtual int stop () QSE_CPP_NOEXCEPT;

	bool isServing () const QSE_CPP_NOEXCEPT
	{ 
		return this->_server_serving; 
	}

	bool isStopRequested () const QSE_CPP_NOEXCEPT
	{
		return this->_stop_requested;
	}
	void setStopRequested (bool req) QSE_CPP_NOEXCEPT
	{
		this->_stop_requested = req;
	}

	qse_size_t getMaxConnections () const QSE_CPP_NOEXCEPT
	{
		return this->_max_connections;
	}
	void setMaxConnections (qse_size_t mc) QSE_CPP_NOEXCEPT
	{
		// don't disconnect connection connections 
		// establised before maxConn is set.
		// 0 means there's no restriction over 
		// the number of connection.
		this->_max_connections = mc;
	}

	qse_size_t getConnectionCount () const QSE_CPP_NOEXCEPT
	{
		return this->_connection_list[Connection::LIVE].getSize(); 
	}

	qse_size_t getThreadStackSize () const QSE_CPP_NOEXCEPT
	{
		return this->_thread_stack_size;
	}

	void setThreadStackSize (qse_size_t tss) QSE_CPP_NOEXCEPT
	{
		this->_thread_stack_size = tss;
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

public:
	class Connection: public QSE::Thread 
	{
	public:
		friend class TcpServer;

		enum State
		{
			DEAD = 0,
			LIVE = 1
		};
		Connection (Listener* listener) QSE_CPP_NOEXCEPT: listener(listener), prev_connection(QSE_NULL), next_connection(QSE_NULL), claimed(false), wid(_wid_map_t::WID_INVALID) {}

		int main ();
		int stop () QSE_CPP_NOEXCEPT;

		Listener* getListener() QSE_CPP_NOEXCEPT { return this->listener; }
		const Listener* getListener() const QSE_CPP_NOEXCEPT { return this->listener; }

		TcpServer* getServer() QSE_CPP_NOEXCEPT { return this->listener->server; }
		const TcpServer* getServer() const QSE_CPP_NOEXCEPT { return this->listener->server; }

		Connection* getNextConnection() { return this->next_connection; }
		Connection* getPrevConnection() { return this->prev_connection; }

		qse_size_t getWid() const { return this->wid; }
		const QSE::SocketAddress& getSourceAddress() const { return this->address; }
		const QSE::Socket& getSocket() const { return this->socket; }

		Listener* listener;
		Connection* prev_connection;
		Connection* next_connection;
		bool claimed;
		qse_size_t wid;

		QSE::Socket socket;
		QSE::SocketAddress address;
		QSE::SpinLock csspl; // spin lock for connection stop 
	};

private:
	struct _wid_map_data_t
	{
		int used;
		union
		{
			Connection* connection;
			qse_size_t  next;
		} u;
	};

	struct _wid_map_t
	{
		enum 
		{
			WID_INVALID = (qse_size_t)-1
		};

		_wid_map_t(): ptr(QSE_NULL), capa(0), free_first(WID_INVALID), free_last(WID_INVALID) {}
		_wid_map_data_t* ptr;
		qse_size_t      capa;
		qse_size_t      free_first;
		qse_size_t      free_last;
	};

	struct ListenerList
	{
		ListenerList(): mux(QSE_NULL), head(QSE_NULL), tail(QSE_NULL), count(0)
		{
			this->mux_pipe[0] = -1;
			this->mux_pipe[1] = -1;
		}

		qse_mux_t* mux;
		int mux_pipe[2];
		SpinLock mux_pipe_spl;

		Listener* head;
		Listener* tail;

		qse_size_t count;
	};

	struct ConnectionList
	{
		ConnectionList() QSE_CPP_NOEXCEPT: head(QSE_NULL), tail(QSE_NULL), count(0) {}

		qse_size_t getSize() const { return this->count; }
		Connection* getHead() { return this->head; }
		Connection* getTail() { return this->tail; }

		void append (Connection* connection)
		{
			connection->next_connection = QSE_NULL;
			if (this->count == 0) 
			{
				this->head = this->tail = connection;
				connection->prev_connection = QSE_NULL;
			}
			else 
			{
				connection->prev_connection = this->tail;
				this->tail->next_connection = connection;
				this->tail = connection;
			}

			this->count++;
		}

		void remove (Connection* connection)
		{
			if (connection->next_connection)
				connection->next_connection->prev_connection = connection->prev_connection;
			else
				this->tail = connection->prev_connection;

			if (connection->prev_connection)
				connection->prev_connection->next_connection = connection->next_connection;
			else
				this->head = connection->next_connection;

			connection->prev_connection = QSE_NULL;
			connection->next_connection = QSE_NULL;
			this->count--;
		}

		Connection* head;
		Connection* tail;
		qse_size_t count;
	};

	_wid_map_t     _wid_map; // connection's id map 
	ListenerList   _listener_list;
	ConnectionList _connection_list[2];
	QSE::SpinLock  _connection_list_spl;
	bool           _stop_requested;
	bool           _server_serving;
	qse_size_t     _max_connections;
	qse_size_t     _thread_stack_size;

protected:
	friend class TcpServer::Connection;
	virtual int handle_connection (Connection* connection) = 0;
	virtual void errlogfmt (const qse_char_t* fmt, ...) { /* do nothing. subclasses may override this */ }

private:
	void delete_all_connections (Connection::State state) QSE_CPP_NOEXCEPT;

	int setup_listeners (const qse_char_t* addrs) QSE_CPP_NOEXCEPT;
	void free_all_listeners () QSE_CPP_NOEXCEPT;

	int prepare_to_acquire_wid () QSE_CPP_NOEXCEPT;
	void acquire_wid (Connection* connection) QSE_CPP_NOEXCEPT;
	void release_wid (Connection* connection) QSE_CPP_NOEXCEPT;
	void free__wid_map () QSE_CPP_NOEXCEPT;

	static void dispatch_mux_event (qse_mux_t* mux, const qse_mux_evt_t* evt) QSE_CPP_NOEXCEPT;
};


// functor as a template parameter
template <typename F>
class QSE_EXPORT TcpServerF: public TcpServer
{
public:
	TcpServerF (Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT: TcpServer(mmgr) {}
	TcpServerF (const F& f, Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT: TcpServer(mmgr), __lfunc(f) {}
#if defined(QSE_CPP_ENABLE_CPP11_MOVE)
	TcpServerF (F&& f, Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT: TcpServer(mmgr), __lfunc(QSE_CPP_RVREF(f)) {}
#endif

private:
	F __lfunc;

	int handle_connection (Connection* connection)
	{
		return this->__lfunc(this, connection);
	}
};

// functor + extra data in the functor
template <typename F, typename D>
class QSE_EXPORT TcpServerFD: public TcpServer
{
public:
	TcpServerFD (Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT: TcpServer(mmgr) {}
	TcpServerFD (const F& f, Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT: TcpServer(mmgr), __lfunc(f) {}
	TcpServerFD (const D& d, Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT: TcpServer(mmgr), __lfunc(d) {}
#if defined(QSE_CPP_ENABLE_CPP11_MOVE)
	TcpServerFD (F&& f, Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT: TcpServer(mmgr), __lfunc(QSE_CPP_RVREF(f)) {}
	TcpServerFD (D&& d, Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT: TcpServer(mmgr), __lfunc(QSE_CPP_RVREF(d)) {}
#endif

private:
	F __lfunc;

	int handle_connection (Connection* connection)
	{
		return this->__lfunc(this, connection);
	}
};


#if defined(QSE_LANG_CPP11)

template <typename T>
class TcpServerL;

template <typename RT, typename... ARGS>
class QSE_EXPORT TcpServerL<RT(ARGS...)>: public TcpServer
{
public:
	TcpServerL (Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT: TcpServer(mmgr), __lfunc(nullptr) {}

	template <typename T>
	TcpServerL (T&& f, Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT: TcpServer(mmgr), __lfunc(nullptr)
	{
		try
		{
			// TODO: are there any ways to achieve this without memory allocation?
			this->__lfunc = new(this->getMmgr()) TCallable<T> (QSE_CPP_RVREF(f));
		}
		catch (...)
		{
			// upon failure, i set this->__lfunc to null.
			// this->handle_connection() will return failure for this.
			this->__lfunc = nullptr;
		}
	}

	~TcpServerL () QSE_CPP_NOEXCEPT 
	{ 
		if (this->__lfunc) QSE_CPP_DELETE_WITH_MMGR (this->__lfunc, Callable, this->getMmgr()); //delete this->__lfunc; 
	}

	template <typename T>
	int setConnectionHandler (T&& f) QSE_CPP_NOEXCEPT
	{
		Callable* lf;

		try
		{
			// TODO: are there any ways to achieve this without memory allocation?
			lf = new(this->getMmgr()) TCallable<T> (QSE_CPP_RVREF(f));
		}
		catch (...)
		{
			this->setErrorNumber (E_ENOMEM);
			return -1;
		}

		if (this->__lfunc) QSE_CPP_DELETE_WITH_MMGR (this->__lfunc, Callable, this->getMmgr()); //delete this->__lfunc;
		this->__lfunc = lf;
		return 0;
	}

private:
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

	int handle_connection (Connection* connection)
	{
		if (!this->__lfunc)
		{
			//this->setErrorNumber (TcpServer::E_ENOMEM or E_EINVAL??);
			return -1;
		}

		return this->__lfunc->invoke(connection);
	}
};

#endif

QSE_END_NAMESPACE(QSE)

#endif

