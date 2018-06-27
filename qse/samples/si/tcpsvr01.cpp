#include <qse/si/TcpServer.hpp>
#include <qse/si/mtx.h>
#include <qse/si/sio.h>
#include <qse/cmn/mem.h>

#include <locale.h>
#if defined(_WIN32)
#	include <windows.h>
#endif

#include <unistd.h>
#include <signal.h>
#include <string.h>


#if defined(QSE_LANG_CPP11)
QSE::TcpServerL<int(QSE::Socket*,QSE::SocketAddress*)>* g_server;
#else

class ClientHandler
{
public:
	int operator() (QSE::Socket* sck, QSE::SocketAddress* addr)
	{
qse_printf (QSE_T("XXXXXXXXXXXXXXXXXXXXXXXXXX\n"));
		return 0;
	}
};

static QSE::TcpServerF<ClientHandler>* g_server;
#endif

static int test1 (void)
{
#if defined(QSE_LANG_CPP11)
	int x, y;

	QSE::TcpServerL<int(QSE::Socket*,QSE::SocketAddress*)> server (
		([&x, &y](QSE::Socket* clisock, QSE::SocketAddress* cliaddr) { 
qse_printf (QSE_T("hello word......\n"));
			return 0;
		})
	);
#else
	QSE::TcpServerF<ClientHandler> server;
#endif

	server.setThreadStackSize (256000);
	g_server = &server;
	server.start (QSE_T("0.0.0.0:9998"));
	g_server = QSE_NULL;
	return 0;
}

static void handle_sigint (int sig, siginfo_t* siginfo, void* ctx)
{
	if (g_server) g_server->stop ();
}

static void set_signal (int sig, void(*handler)(int, siginfo_t*, void*))
{
	struct sigaction sa;
 
	memset (&sa, 0, sizeof(sa));
	/*sa.sa_handler = handler;*/
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = handler;
	sigemptyset (&sa.sa_mask);
 
	sigaction (sig, &sa, NULL);
}
 
static void set_signal_to_default (int sig)
{
	struct sigaction sa;
 
	memset (&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_DFL;
	sa.sa_flags = 0;
	sigemptyset (&sa.sa_mask);
 
	sigaction (sig, &sa, NULL);
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

	set_signal (SIGINT, handle_sigint);

	qse_open_stdsios ();
	test1();
	qse_close_stdsios ();

	set_signal_to_default (SIGINT);

	return 0;
}
