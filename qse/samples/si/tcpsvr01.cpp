#include <qse/si/TcpServer.hpp>
#include <qse/si/Mutex.hpp>
#include <qse/si/sio.h>
#include <qse/si/os.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/HeapMmgr.hpp>
#include <qse/si/App.hpp>


#include <locale.h>
#if defined(_WIN32)
#	include <windows.h>
#endif

#include <unistd.h>
#include <signal.h>
#include <string.h>

class ClientHandler
{
public:
	int operator() (QSE::TcpServer* server, QSE::TcpServer::Worker* worker)
	{
		qse_char_t addrbuf[128];
		qse_uint8_t bb[256];
		qse_ssize_t n;

		worker->address.toStrBuf(addrbuf, QSE_COUNTOF(addrbuf));
		qse_printf (QSE_T("hello word..from %s -> wid %zu\n"), addrbuf, worker->getWid());

		while (!server->isStopRequested())
		{
			if ((n = worker->socket.receive(bb, QSE_COUNTOF(bb))) <= 0) 
			{
				qse_printf (QSE_T("%zd bytes received from %s\n"), n, addrbuf);
				break;
			}
			worker->socket.send (bb, n);
		}

		qse_printf (QSE_T("byte to %s -> wid %zu\n"), addrbuf, worker->getWid());
		return 0;
	}
};

#if defined(QSE_LANG_CPP11)
static QSE::TcpServerL<int(QSE::TcpServer::Worker*)>* g_server;
#else
static QSE::TcpServerF<ClientHandler>* g_server;
#endif


class MyApp: public QSE::App
{
public:
	MyApp(QSE::Mmgr* mmgr): App(mmgr), server(mmgr) {}

	void on_signal (int sig)
	{
		switch (sig)
		{
			case SIGINT:
			case SIGTERM:
			case SIGHUP:
				qse_printf (QSE_T("requesting to stop server...app %p server %p\n"), this, &this->server);
				this->server.stop();
				break;
		}
	}

	int run ()
	{
		this->server.setThreadStackSize (256000);
		return this->server.start (QSE_T("[::]:9998,0.0.0.0:9998"));
	}

protected:
	QSE::TcpServerF<ClientHandler> server;
};

static int test1()
{
	QSE::HeapMmgr heap_mmgr (QSE::Mmgr::getDFL(), 30000);
	MyApp app (&heap_mmgr);

MyApp app2 (&heap_mmgr);
MyApp app3 (&heap_mmgr);
MyApp app4 (&heap_mmgr);

	app.subscribeToSignal (SIGINT, true)	;
	app.subscribeToSignal (SIGTERM, true);

app4.subscribeToSignal (SIGINT, true); 
app3.subscribeToSignal (SIGINT, true);
app2.subscribeToSignal (SIGINT, true);



	int n = app.run();
	app.subscribeToSignal (SIGTERM, false);
	app.subscribeToSignal (SIGINT, false);

app4.unsubscribeFromSignal (SIGINT); 
app3.unsubscribeFromSignal (SIGINT);
app2.unsubscribeFromSignal (SIGINT);
	return n;
}

int main ()
{
#if defined(_WIN32)
 	char locale[100];
	UINT codepage = GetConsoleOutputCP();
	if (codepage == CP_UTF8)
	{
		/*SetConsoleOUtputCP (CP_UTF8);*/
		qse_setdflcmgrbyid (QSE_CMGR_UTF8);
	}
	else
	{
		sprintf (locale, ".%u", (unsigned int)codepage);
		setlocale (LC_ALL, locale);
		/*qse_setdflcmgrbyid (QSE_CMGR_SLMB);*/
	}
#else
	setlocale (LC_ALL, "");
	/*qse_setdflcmgrbyid (QSE_CMGR_SLMB);*/
#endif

	qse_open_stdsios ();
	test1();
	qse_close_stdsios ();

	return 0;
}


#if 0 ////////////////////////

static int test1 (void)
{
	QSE::HeapMmgr heap_mmgr (QSE::Mmgr::getDFL(), 30000);

#if defined(QSE_LANG_CPP11)
	QSE::TcpServerL<int(QSE::TcpServer::Worker*)> server (

		// workload by lambda
		([&server](QSE::TcpServer::Worker* worker) {
			qse_char_t addrbuf[128];
			qse_uint8_t bb[256];
			qse_ssize_t n;

			worker->address.toStrBuf(addrbuf, QSE_COUNTOF(addrbuf));
			g_prt_mutex.lock();
			qse_printf (QSE_T("hello word..from %s -> wid %zu\n"), addrbuf, worker->getWid());
			g_prt_mutex.unlock();

			while (!server.isStopRequested())
			{
				if ((n = worker->socket.receive(bb, QSE_COUNTOF(bb))) <= 0) 
				{
					g_prt_mutex.lock();
					qse_printf (QSE_T("%zd bytes received from %s\n"), n, addrbuf);
					g_prt_mutex.unlock();
					break;
				}
				worker->socket.send (bb, n);
			}

			g_prt_mutex.lock();
			qse_printf (QSE_T("byte to %s -> wid %zu\n"), addrbuf, worker->getWid());
			g_prt_mutex.unlock();
			return 0;
		}),

		&heap_mmgr
	);
#else
	QSE::TcpServerF<ClientHandler> server (&heap_mmgr);
#endif

	server.setThreadStackSize (256000);
	g_server = &server;
	//server.start (QSE_T("0.0.0.0:9998"));
	server.start (QSE_T("[::]:9998,0.0.0.0:9998"));
	//server.start (QSE_T("[fe80::1c4:a90d:a0f0:d52%wlan0]:9998,0.0.0.0:9998"));
	g_server = QSE_NULL;
	return 0;
}

static void handle_sigint (int sig)
{
	if (g_server) g_server->stop ();
}
 
int main ()
{
#if defined(_WIN32)
 	char locale[100];
	UINT codepage = GetConsoleOutputCP();
	if (codepage == CP_UTF8)
	{
		/*SetConsoleOUtputCP (CP_UTF8);*/
		qse_setdflcmgrbyid (QSE_CMGR_UTF8);
	}
	else
	{
		sprintf (locale, ".%u", (unsigned int)codepage);
		setlocale (LC_ALL, locale);
		/*qse_setdflcmgrbyid (QSE_CMGR_SLMB);*/
	}
#else
	setlocale (LC_ALL, "");
	/*qse_setdflcmgrbyid (QSE_CMGR_SLMB);*/
#endif

	qse_open_stdsios ();

	//QSE::App::setSignalHandler (SIGINT, handle_sigint);
	test1();
	//QSE::App::unsetSignalHandler (SIGINT);

	qse_close_stdsios ();

	return 0;
}

#endif ////////////////////////
