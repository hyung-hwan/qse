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
#include <qse/si/SpinLock.hpp>
#include <qse/cmn/Mmged.hpp>
#include <qse/Uncopyable.hpp>
#include <qse/si/mux.h>

QSE_BEGIN_NAMESPACE(QSE)

// The TcpServer class implements a simple block TCP server that start a thread
// for each connection accepted.

class QSE_EXPORT TcpServer: public Uncopyable, public Mmged, public Types
{
public:
	TcpServer (Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT;
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
		// don't disconnect worker connections 
		// establised before maxConn is set.
		// 0 means there's no restriction over 
		// the number of connection.
		this->max_connections = mc;
	}

	qse_size_t getWorkerCount () const QSE_CPP_NOEXCEPT
	{ 
		return this->worker_list[Worker::LIVE].getSize(); 
	}
	qse_size_t getConnectionCount () const QSE_CPP_NOEXCEPT
	{
		return this->worker_list[Worker::LIVE].getSize(); 
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

public:
	class Worker: public QSE::Thread 
	{
	public:
		friend class TcpServer;

		enum State
		{
			DEAD = 0,
			LIVE = 1
		};
		Worker (Listener* listener) QSE_CPP_NOEXCEPT: listener(listener), prev_worker(QSE_NULL), next_worker(QSE_NULL), claimed(false), wid(wid_map_t::WID_INVALID) {}

		int main ();
		int stop () QSE_CPP_NOEXCEPT;

		Listener* getListener() QSE_CPP_NOEXCEPT { return this->listener; }
		const Listener* getListener() const QSE_CPP_NOEXCEPT { return this->listener; }

		TcpServer* getServer() QSE_CPP_NOEXCEPT { return this->listener->server; }
		const TcpServer* getServer() const QSE_CPP_NOEXCEPT { return this->listener->server; }


		Worker* getNextWorker() { return this->next_worker; }
		Worker* getPrevWorker() { return this->prev_worker; }

		qse_size_t getWid() const { return this->wid; }

		Listener* listener;
		Worker* prev_worker;
		Worker* next_worker;
		bool claimed;
		qse_size_t wid;

		QSE::Socket socket;
		QSE::SocketAddress address;
		QSE::SpinLock csspl; // spin lock for worker stop 
	};

protected:
	struct wid_map_data_t
	{
		int used;
		union
		{
				Worker*     worker;
				qse_size_t  next;
		} u;
	};

	struct wid_map_t
	{
		enum 
		{
			WID_INVALID = (qse_size_t)-1
		};

		wid_map_t(): ptr(QSE_NULL), capa(0), free_first(WID_INVALID), free_last(WID_INVALID) {}
		wid_map_data_t* ptr;
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

	struct WorkerList
	{
		WorkerList() QSE_CPP_NOEXCEPT: head(QSE_NULL), tail(QSE_NULL), count(0) {}

		qse_size_t getSize() const { return this->count; }
		Worker* getHead() { return this->head; }
		Worker* getTail() { return this->tail; }

		void append (Worker* worker)
		{
			worker->next_worker = QSE_NULL;
			if (this->count == 0) 
			{
				this->head = this->tail = worker;
				worker->prev_worker = QSE_NULL;
			}
			else 
			{
				worker->prev_worker = this->tail;
				this->tail->next_worker = worker;
				this->tail = worker;
			}

			this->count++;
		}

		void remove (Worker* worker)
		{
			if (worker->next_worker)
				worker->next_worker->prev_worker = worker->prev_worker;
			else
				this->tail = worker->prev_worker;

			if (worker->prev_worker)
				worker->prev_worker->next_worker = worker->next_worker;
			else
				this->head = worker->next_worker;

			worker->prev_worker = QSE_NULL;
			worker->next_worker = QSE_NULL;
			this->count--;

		}

		Worker* head;
		Worker* tail;
		qse_size_t count;
	};

	wid_map_t wid_map; // worker's id map 
	ListenerList listener_list;
	WorkerList worker_list[2];
	QSE::SpinLock worker_list_spl;

	ErrorCode     errcode;
	bool          stop_requested;
	bool          server_serving;
	qse_size_t    max_connections;
	qse_size_t    thread_stack_size;

	friend class TcpServer::Worker;
	virtual int handle_worker (Worker* worker) = 0;


private:
	void delete_all_workers (Worker::State state) QSE_CPP_NOEXCEPT;

	int setup_listeners (const qse_char_t* addrs) QSE_CPP_NOEXCEPT;
	void free_all_listeners () QSE_CPP_NOEXCEPT;

	int prepare_to_acquire_wid () QSE_CPP_NOEXCEPT;
	void acquire_wid (Worker* worker) QSE_CPP_NOEXCEPT;
	void release_wid (Worker* worker) QSE_CPP_NOEXCEPT;
	void free_wid_map () QSE_CPP_NOEXCEPT;

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

protected:
	F __lfunc;

	int handle_worker (Worker* worker)
	{
		return this->__lfunc(this, worker);
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
			// this->handle_worker() will return failure for this.
			this->__lfunc = nullptr;
		}
	}

	~TcpServerL () QSE_CPP_NOEXCEPT 
	{ 
		if (this->__lfunc) QSE_CPP_DELETE_WITH_MMGR (this->__lfunc, Callable, this->getMmgr()); //delete this->__lfunc; 
	}

	template <typename T>
	int setWorkerHandler (T&& f) QSE_CPP_NOEXCEPT
	{
		Callable* lf;

		try
		{
			// TODO: are there any ways to achieve this without memory allocation?
			lf = new(this->getMmgr()) TCallable<T> (QSE_CPP_RVREF(f));
		}
		catch (...)
		{
			this->setErrorCode (E_NOERR);
			return -1;
		}

		if (this->__lfunc) QSE_CPP_DELETE_WITH_MMGR (this->__lfunc, Callable, this->getMmgr()); //delete this->__lfunc;
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

	int handle_worker (Worker* worker)
	{
		if (!this->__lfunc)
		{
			//this->setErrorCode (TcpServer::E_ENOMEM or E_EINVAL??);
			return -1;
		}

		return this->__lfunc->invoke(worker);
	}
};

#endif

QSE_END_NAMESPACE(QSE)

#endif

