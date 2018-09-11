#include <qse/si/TcpServer.hpp>
#include <qse/si/Mutex.hpp>
#include <qse/si/sio.h>
#include <qse/si/os.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/cmn/HeapMmgr.hpp>
#include <qse/si/App.hpp>


#include <locale.h>
#if defined(_WIN32)
#	include <windows.h>
#endif

#include <signal.h>
#include <string.h>


#include <sys/wait.h>
#include <sys/prctl.h>
#include <unistd.h>

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
				qse_printf (QSE_T("requesting to stop server...app %p server %p - pid %d\n"), this, &this->server, (int)getpid());
				this->server.stop();
				break;
		}
	}

	int run ()
	{
		QSE::App::SignalSet signals;
		signals.set (SIGINT);
		signals.set (SIGHUP);
		signals.set (SIGTERM);
		signals.set (SIGUSR1);
		signals.set (SIGUSR2);
		if (this->guardProcess(signals) > 0)
		{
			qse_printf (QSE_T("Stareting server\n"));
			this->server.setThreadStackSize (256000);
			return this->server.start (QSE_T("[::]:9998,0.0.0.0:9998"));
		}
		return -1;
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

	app.subscribeToSignal (SIGINT, true);
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
qse_printf (QSE_T("END OF %d\n"), (int)getpid());

	return n;
}

int main ()
{
#if defined(_WIN32)
 	char locale[100];
	UINT codepage = GetConsoleOutputCP();
	if (codepage == CP_UTF8)
	{
		/*SetConsoleOutputCP (CP_UTF8);*/
		qse_setdflcmgrbyid (QSE_CMGR_UTF8);
	}
	else
	{
		qse_mbsxfmt (locale, QSE_COUNTOF(locale), ".%u", (unsigned int)codepage);
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
