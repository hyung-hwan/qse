#include <qse/si/TcpServer.hpp>
#include <qse/si/Mutex.hpp>
#include <qse/si/sio.h>
#include <qse/si/os.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/cmn/HeapMmgr.hpp>
#include <qse/sttp/Sttp.hpp>
#include <qse/si/App.hpp>

#include <locale.h>
#if defined(_WIN32)
#	include <windows.h>
#endif

#include <unistd.h>
#include <signal.h>
#include <string.h>

class Proto: public QSE::Sttp
{
public:
	Proto (QSE::Socket* sck): sck(sck) {}

	int handle_command (const QSE::SttpCmd& cmd)
	{
		qse_printf (QSE_T("command -> %js\n"), cmd.name.getData());
		int n = this->sendCmd(QSE_T("OK"), 1, cmd.name.getData());
		if (cmd.name == QSE_T("quit"))
		{
			sck->shutdown ();
		}
		return n;
	}

	int write_bytes (const qse_uint8_t* data, qse_size_t len)
	{
		int n = this->sck->sendx(data, len, QSE_NULL);
		if (QSE_UNLIKELY(n <= -1)) this->setErrorFmt(E_ESYSERR, QSE_T("%js"), sck->getErrorMsg());
		return n;
	}

protected:
	QSE::Socket* sck;
};

#if defined(QSE_LANG_CPP11)
QSE::TcpServerL<int(QSE::TcpServer::Connection*)>* g_server;
#else

class ClientHandler
{
public:
	int operator() (QSE::TcpServer* server, QSE::TcpServer::Connection* connection)
	{
		qse_char_t addrbuf[128];
		qse_uint8_t bb[256];
		qse_ssize_t n;

		connection->address.toStrBuf(addrbuf, QSE_COUNTOF(addrbuf));
		qse_printf (QSE_T("hello word..from %s[%zu]\n"), addrbuf, connection->getWid());

		Proto proto (&connection->socket);

		while (!server->isHaltRequested())
		{
			if ((n = connection->socket.receive(bb, QSE_COUNTOF(bb))) <= 0)
			{
				if (n <= -1)
					qse_printf (QSE_T("%s[%zu] -> got error\n"), addrbuf, connection->getWid());
				break;
			}

			if (proto.feed(bb, n, QSE_NULL) <= -1)
			{
				qse_printf (QSE_T("%s[%zu] -> protocol error - %js\n"), addrbuf, connection->getWid(), proto.getErrorMsg());
				break;
			}
		}

		qse_printf (QSE_T("byte to %s -> wid %zu\n"), addrbuf, connection->getWid());
		return 0;
	}
};

static QSE::TcpServerF<ClientHandler>* g_server;
#endif


static int test1 (void)
{
	QSE::HeapMmgr heap_mmgr (30000, QSE::Mmgr::getDFL());

#if defined(QSE_LANG_CPP11)
	QSE::TcpServerL<int(QSE::TcpServer::Connection*)> server (

		// workload by lambda
		([&server](QSE::TcpServer::Connection* connection) {
			qse_char_t addrbuf[128];
			qse_uint8_t bb[256];
			qse_ssize_t n;

			connection->address.toStrBuf(addrbuf, QSE_COUNTOF(addrbuf));
			qse_printf (QSE_T("hello word..from %s --> wid %zu\n"), addrbuf, connection->getWid());

			Proto proto (&connection->socket);

			while (!server.isHaltRequested())
			{
				if ((n = connection->socket.receive(bb, QSE_COUNTOF(bb))) <= 0)
				{
					if (n <= -1)
						qse_printf (QSE_T("%s[%zu] -> got error\n"), addrbuf, connection->getWid());
					break;
				}

				if (proto.feed(bb, n, QSE_NULL) <= -1)
				{
					qse_printf (QSE_T("%s[%zu] -> protocol error - %js\n"), addrbuf, connection->getWid(), proto.getErrorMsg());
					break;
				}
			}

			qse_printf (QSE_T("byte to %s -> wid %zu\n"), addrbuf, connection->getWid());
			return 0;
		}),

		&heap_mmgr

	);
#else
	QSE::TcpServerF<ClientHandler> server (&heap_mmgr);
#endif

	server.setThreadStackSize (256000);
	g_server = &server;
	//server.execute (QSE_T("0.0.0.0:9998"));
	server.execute (QSE_T("[::]:9998,0.0.0.0:9998"));
	//server.execute (QSE_T("[fe80::1c4:a90d:a0f0:d52%wlan0]:9998,0.0.0.0:9998"));
	g_server = QSE_NULL;
	return 0;
}

static void handle_sigint (int sig)
{
	if (g_server) g_server->halt ();
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

	QSE::App::setSignalHandler (SIGINT, handle_sigint);
	test1();
	QSE::App::unsetSignalHandler (SIGINT);

	qse_close_stdsios ();

	return 0;
}
