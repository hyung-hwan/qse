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

QSE_BEGIN_NAMESPACE(QSE)

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

int TcpServer::Client::run ()
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

TcpServer::TcpServer (): 
	stop_requested(false), 
	server_serving(false), 
	max_connections(0),
	thread_stack_size (0),
	reopen_socket_upon_error(false)
{
}

TcpServer::TcpServer (const SocketAddress& address): 
	binding_address(address),
	stop_requested(false), 
	server_serving(false), 
	max_connections(0), 
	thread_stack_size (0),
	reopen_socket_upon_error(false)
{
}

TcpServer::~TcpServer () QSE_CPP_NOEXCEPT
{
	// QSE_ASSERT (server_serving == false);
	this->delete_all_clients ();
}

int TcpServer::start (int* err_code) QSE_CPP_NOEXCEPT
{
	return this->start(true, err_code);
}

int TcpServer::open_tcp_socket (Socket& socket, bool winsock_inheritable, int* err_code) QSE_CPP_NOEXCEPT
{
	if (socket.open(this->binding_address.getFamily(), QSE_SOCK_STREAM, 0) <= -1)
	{
		if (err_code) *err_code = ERR_OPEN;
		return -1;
	}

#if defined(_WIN32)
	SetHandleInformation ((HANDLE)socket.handle(), HANDLE_FLAG_INHERIT, (winsock_inheritable? HANDLE_FLAG_INHERIT: 0));
#endif

	//socket.setReuseAddr (true);
	//socket.setReusePort (true);

	if (socket.bind(this->binding_address) <= -1)
	{
		if (err_code) *err_code = ERR_BIND;
		return -1;
	}

	if (socket.listen() <= -1) 
	{
		if (err_code) *err_code = ERR_LISTEN;
		return -1;
	}

	//socket.enableTimeout (1000);
	return 0;
}

int TcpServer::start (bool winsock_inheritable, int* err_code) QSE_CPP_NOEXCEPT
{
	this->server_serving = true;
	if (err_code != QSE_NULL) *err_code = ERR_NONE;

	this->setStopRequested (false);

	Client* client = QSE_NULL;

	try 
	{
		Socket socket;

		if (this->open_tcp_socket(socket, winsock_inheritable, err_code) <= -1)
		{
			this->server_serving = false;
			this->setStopRequested (false);
			return -1;
		}

		while (!this->isStopRequested()) 
		{
			this->delete_dead_clients ();

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
				if (socket.accept(&s, &sa, Socket::T_CLOEXEC) >= 0) s.close();
				continue;
			}

			if (socket.accept(&client->socket, &client->address, Socket::T_CLOEXEC) <= -1) 
			{
				// can't do much if accept fails

				// don't "delete client" here as i want it to be reused
				// in the next iteration after "continue"

				if (this->reopen_socket_upon_error)
				{
					// closing the listeing socket causes the 
					// the pending connection to be dropped.
					// this poses the risk of reopening failure.
					// imagine the case of EMFILE(too many open files).
					// accept() will fail until an open file is closed.
					qse_size_t reopen_count = 0;
					socket.close ();

				reopen:
					if (this->open_tcp_socket (socket, winsock_inheritable, err_code) <= -1)
					{
						if (reopen_count >= 100) 
						{
							qse_ntime_t interval;
							if (reopen_count >= 200) qse_inittime (&interval, 0, 100000000); // 100 milliseconds
							else qse_inittime (&interval, 0, 10000000); // 10 milliseconds
							qse_sleep (&interval);
						}

						if (this->isStopRequested()) break;
						reopen_count++;
						goto reopen;
					}
				}
				continue;
			}

			client->setStackSize (this->thread_stack_size);
		#if defined(_WIN32)
			if (client->start(Thread::DETACHED) == -1) 
		#else
			if (client->start(0) == -1)
		#endif
			{
				delete client; 
				client = QSE_NULL;
				continue;
			}

			this->client_list.append (client);
			client = QSE_NULL;
		}

		this->delete_all_clients ();
		if (client != QSE_NULL) delete client;
	}
	catch (...) 
	{
		this->delete_all_clients ();
		if (client != QSE_NULL) delete client;

		if (err_code) *err_code = ERR_EXCEPTION;
		this->server_serving = false;
		this->setStopRequested (false);

		return -1;
	}

	this->server_serving = false;
	this->setStopRequested (false);
	return 0;
}

int TcpServer::stop () QSE_CPP_NOEXCEPT
{
	if (server_serving) setStopRequested (true);
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
