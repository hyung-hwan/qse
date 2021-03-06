#include <qse/si/Thread.hpp>
#include <qse/si/Condition.hpp>
#include <qse/si/mtx.h>
#include <qse/si/sio.h>
#include <qse/si/os.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/cmn/HeapMmgr.hpp>


#include <locale.h>
#if defined(_WIN32)
#	include <windows.h>
#endif

#include <signal.h>
#include <string.h>

static int g_stopreq = 0;
static qse_ntime_t sleep_interval = { 1, 0 };

QSE::HeapMmgr g_heap_mmgr (30000, QSE::Mmgr::getDFL());

struct rq_data_t
{
	rq_data_t(): stop(0) {}
	int stop;
	QSE::Mutex mtx;
	QSE::Condition cnd;
};

class Waiter
{
public:
	int operator() (QSE::Thread* thr)
	{
		int i = 0;
		rq_data_t* rqdata = (rq_data_t*)thr->getContext();

		while (1)
		{
		#if 0
			rqdata->mtx.lock ();
		#else
			while (!rqdata->mtx.trylock())
			{
				qse_printf (QSE_T("[%p] -> retrying to lock\n"), this, i);
			}
		#endif

			if (rqdata->stop)
			{
				rqdata->mtx.unlock ();
				break;
			}
			rqdata->cnd.wait(rqdata->mtx);
			rqdata->mtx.unlock ();

			qse_printf (QSE_T("[%p] -> loop %d\n"), this, i);
			i++;
		}

		qse_printf (QSE_T("[%p] -> exiting\n"), this);
		return i;
	}
};

static int test1 (void)
{
	rq_data_t rqdata;
	QSE::ThreadF<Waiter> thr[3];

	for (int i = 0; i < 3; i++)
	{
		thr[i].setStackSize (64000);
		thr[i].setContext (&rqdata);
		if (thr[i].start(QSE::Thread::SIGNALS_BLOCKED) <= -1)
		{
			qse_printf (QSE_T("cannot start thread%d\n"), i);
			return -1;
		}
	}

	while (1)
	{
		if (g_stopreq) 
		{
			qse_printf (QSE_T("broadcasting stop ---> 1\n"));

			rqdata.mtx.lock ();
			rqdata.stop = 1;
			rqdata.mtx.unlock ();
			rqdata.cnd.broadcast ();
		}

		int nterm = 0;
		for (int i = 0; i < 3; i++)
		{
			if (thr[i].getState() == QSE::Thread::TERMINATED) nterm++;
		}
		if (nterm == 3) break;

		qse_printf (QSE_T("signalling ....(nterm = %d)\n"), nterm);

		rqdata.cnd.signal ();
		qse_sleep (&sleep_interval);
	}


	for (int i = 0; i < 3; i++)
	{
		thr[i].join();
	}

	for (int i = 0; i < 3; i++)
	{
		qse_printf (QSE_T("thread%d ended with retcode %d\n"), i, thr[i].getReturnCode());
	}

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

	set_signal (SIGINT, handle_sigint);

	qse_open_stdsios ();
	test1();
	qse_close_stdsios ();

	set_signal_to_default (SIGINT);

	return 0;
}
