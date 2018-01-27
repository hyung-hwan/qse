#include <qse/si/Thread.hpp>
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

static int g_stopreq = 0;
static qse_mtx_t* g_prmtx = QSE_NULL;

class MyThread: public QSE::Thread
{
public:
	MyThread(): stopreq(0) {}

	int main ()
	{
		int i = 0;

		while (!this->stopreq)
		{
			qse_mtx_lock (g_prmtx, QSE_NULL);
			qse_printf (QSE_T("m %p -> %d\n"), this, i);
			qse_mtx_unlock (g_prmtx);
			i++;
			sleep (1);
		}

		return i;
	}

	int stopreq;
};

class Functor
{
public:
	int operator() (QSE::Thread* thr)
	{
		int i = 0;
		int* stopreqptr = (int*)thr->getContext();

		while (!*stopreqptr)
		{
			qse_mtx_lock (g_prmtx, QSE_NULL);
			qse_printf (QSE_T("fc %p -> %d\n"), this, i);
			qse_mtx_unlock (g_prmtx);
			i++;
			sleep (1);
		}

		return i;
	}
};

static int test1 (void)
{
	MyThread thr1;
	QSE::ThreadR thr2;

	int localstopreq = 0;
	g_prmtx = qse_mtx_open (QSE_MMGR_GETDFL(), 0);

	thr1.setStackSize (64000);
	thr2.setStackSize (64000);
	

	auto lambda = [](QSE::Thread* thr)->int 
	{ 
		int i = 0;
		int* stopreqptr = (int*)thr->getContext();

		while (!*stopreqptr)
		{
			qse_mtx_lock (g_prmtx, QSE_NULL);
			qse_printf (QSE_T("l %p -> %d\n"), thr, i);
			qse_mtx_unlock (g_prmtx);
			i++;
			sleep (1);
		}

		return i;
	};

	auto lambda_with_capture = [&localstopreq](QSE::Thread* thr)->int
	{ 
		int i = 0;

		while (!localstopreq)
		{
			qse_mtx_lock (g_prmtx, QSE_NULL);
			qse_printf (QSE_T("lc %p -> %d\n"), thr, i);
			qse_mtx_unlock (g_prmtx);
			i++;
			sleep (1);
		}

		return i;
	};

	if (thr1.start(QSE::Thread::SIGNALS_BLOCKED) <= -1)
	{
		qse_printf (QSE_T("cannot start thread1\n"));
		return -1;
	}

	// the lambda expression with no capture can be passed as a function pointer
	// as long as the signature matches QSE::Thread::ThreadRoutine.
	thr2.setContext (&localstopreq);
	if (thr2.start(lambda, QSE::Thread::SIGNALS_BLOCKED) <= -1)
	{
		qse_printf (QSE_T("cannot start thread2\n"));
		return -1;
	}

	//QSE::LambdaThread thr3;
	QSE::ThreadC<decltype(lambda)> thr3 (lambda);
	thr3.setStackSize (64000);
	thr3.setContext (&localstopreq);
	if (thr3.start(QSE::Thread::SIGNALS_BLOCKED) <= -1)
	{
		qse_printf (QSE_T("cannot start thread3\n"));
		return -1;
	}

	// turn a lambda with capture to a thread
	QSE::ThreadC<decltype(lambda_with_capture)> thr4 (lambda_with_capture);
	thr4.setStackSize (64000);
	if (thr4.start(QSE::Thread::SIGNALS_BLOCKED) <= -1)
	{
		qse_printf (QSE_T("cannot start thread4\n"));
		return -1;
	}

	// turn a functor to a thread
	QSE::ThreadC<Functor> thr5;
	thr5.setStackSize (64000);
	thr5.setContext (&localstopreq);
	if (thr5.start(QSE::Thread::SIGNALS_BLOCKED) <= -1)
	{
		qse_printf (QSE_T("cannot start thread4\n"));
		return -1;
	}

	while (!g_stopreq)
	{
		if (thr1.getState() == QSE::Thread::TERMINATED && 
		    thr2.getState() == QSE::Thread::TERMINATED &&
		    thr3.getState() == QSE::Thread::TERMINATED &&
		    thr4.getState() == QSE::Thread::TERMINATED &&
		    thr5.getState() == QSE::Thread::TERMINATED) break;
		sleep (1);
	}

	if (g_stopreq) 
	{
		localstopreq = 1;
		thr1.stopreq = 1;
	}

	thr1.join ();
	thr2.join ();
	thr3.join ();
	thr4.join ();
	thr5.join ();

	qse_printf (QSE_T("thread1 ended with retcode %d\n"), thr1.getReturnCode());
	qse_printf (QSE_T("thread2 ended with retcode %d\n"), thr2.getReturnCode());
	qse_printf (QSE_T("thread3 ended with retcode %d\n"), thr3.getReturnCode());
	qse_printf (QSE_T("thread4 ended with retcode %d\n"), thr4.getReturnCode());
	qse_printf (QSE_T("thread5 ended with retcode %d\n"), thr5.getReturnCode());

	qse_mtx_close (g_prmtx);
	return 0;
}



static void handle_sigint (int sig, siginfo_t* siginfo, void* ctx)
{
	g_stopreq = 1;
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
