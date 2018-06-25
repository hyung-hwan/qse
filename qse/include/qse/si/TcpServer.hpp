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

class TcpServer: public QSE::Uncopyable 
{
public:
	TcpServer ();
	TcpServer (const SocketAddress& address);
	virtual ~TcpServer ();

	enum 
	{
		ERR_NONE      = 0,
		ERR_OPEN      = 1,
		ERR_BIND      = 2,
		ERR_LISTEN    = 3,
		ERR_EXCEPTION = 4
	};

	virtual int start (int* err_code = QSE_NULL);
	virtual int start (bool winsock_inheritable, int* err_code = QSE_NULL);
	virtual int stop ();

	bool isServing () const 
	{ 
		return this->server_serving; 
	}

	bool isStopRequested () const 
	{
		return this->stop_requested;
	}
	void setStopRequested (bool req)
	{
		this->stop_requested = req;
	}

	const SocketAddress& bindingAddress () const
	{
		return this->binding_address;
	}
	void setBindingAddress (const SocketAddress& address)
	{
		QSE_ASSERT (this->server_serving == false);
		this->binding_address = address;
	}

	qse_size_t maxConnections () const 
	{
		return this->max_connections;
	}
	void setMaxConnections (qse_size_t mc) 
	{
		// don't disconnect client connections 
		// establised before maxConn is set.
		// 0 means there's no restriction over 
		// the number of connection.
		this->max_connections = mc;
	}

	qse_size_t clientCount () const 
	{ 
		return this->client_list.getSize(); 
	}
	qse_size_t connectionCount () const
	{
		return this->client_list.getSize(); 
	}

	qse_size_t threadStackSize () const
	{
		return this->thread_stack_size;
	}

	void setThreadStackSize (qse_size_t tss)
	{
		this->thread_stack_size = tss;
	}

	bool shouldReopenSocketUponError () const
	{
		return this->reopen_socket_upon_error;
	}

	void setReopenSocketUponError (bool v)
	{
		this->reopen_socket_upon_error = v;
	}

protected:
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
	void delete_dead_clients ();
	void delete_all_clients  ();
	int open_tcp_socket (Socket& socket, bool winsock_inheritable, int* err_code);
};

QSE_END_NAMESPACE(QSE)

#endif

