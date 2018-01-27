#include <qse/si/thr.h>
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

struct thr_xtn_t
{
	int stopreq;
};
typedef struct thr_xtn_t thr_xtn_t;

static int thr_func (qse_thr_t* thr)
{
	int i = 0;
	thr_xtn_t* xtn = qse_thr_getxtn(thr);

	while (!xtn->stopreq)
	{
		qse_printf (QSE_T("%d\n"), i);
		i++;
		sleep (1);
	}

	return i;
}

static int test1 (void)
{
	qse_thr_t* thr;

	thr = qse_thr_open (QSE_MMGR_GETDFL(), QSE_SIZEOF(thr_xtn_t));
	if (!thr)
	{
		qse_printf (QSE_T("cannot open thread\n"));
		return -1;
	}

	qse_thr_setstacksize (thr, 64000);

	if (qse_thr_start(thr, thr_func, QSE_NULL, QSE_THR_SIGNALS_BLOCKED) <= -1)
	{
		qse_printf (QSE_T("cannot start thread\n"));
		qse_thr_close (thr);
		return -1;
	}

	while (!g_stopreq)
	{
		if (qse_thr_getstate(thr) == QSE_THR_TERMINATED) break;
		sleep (1);
	}

	if (g_stopreq) 
	{
		/*qse_thr_stop (thr);*/
		thr_xtn_t* xtn = qse_thr_getxtn(thr);
		xtn->stopreq = 1;
	}

	qse_thr_join (thr);

	qse_printf (QSE_T("thread ended with retcode %d\n"), qse_thr_getretcode(thr));
	qse_thr_close (thr);

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
