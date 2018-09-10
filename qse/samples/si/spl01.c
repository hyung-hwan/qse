#include <qse/si/spl.h>
#include <qse/si/thr.h>
#include <qse/si/sio.h>
#include <qse/cmn/mem.h>

#include <locale.h>
#if defined(_WIN32)
#	include <windows.h>
#endif

#include <signal.h>
#include <string.h>

static int g_stopreq = 0;
static qse_ntime_t sleep_interval = { 1, 0 };

struct thr_xtn_t
{
	int stopreq;
	qse_char_t name[32];
	qse_spl_t* spl;
};
typedef struct thr_xtn_t thr_xtn_t;

static int thr_func (qse_thr_t* thr, void* ctx)
{
	int i = 0;
	thr_xtn_t* xtn = qse_thr_getxtn(thr);

	while (!xtn->stopreq)
	{
		qse_spl_lock (xtn->spl);
		qse_printf (QSE_T("%s: [% 16d] [% 16d] [% 16d]\n"), xtn->name, i, i, i);
		qse_spl_unlock (xtn->spl);
		i++;
		/*qse_sleep (&sleep_interval);*/
	}

	return i;
}

static int test1 (void)
{
	qse_thr_t* thr1, * thr2;
	qse_spl_t spl;
	thr_xtn_t* xtn1, * xtn2;

	qse_spl_init (&spl);

	thr1 = qse_thr_open (QSE_MMGR_GETDFL(), QSE_SIZEOF(thr_xtn_t));
	if (!thr1)
	{
		qse_printf (QSE_T("cannot open thread1\n"));
		return -1;
	}

	thr2 = qse_thr_open (QSE_MMGR_GETDFL(), QSE_SIZEOF(thr_xtn_t));
	if (!thr2)
	{
		qse_printf (QSE_T("cannot open thread2\n"));
		return -1;
	}

	xtn1 = qse_thr_getxtn(thr1);
	xtn2 = qse_thr_getxtn(thr2);

	qse_strcpy (xtn1->name, QSE_T("Thr-X"));
	qse_strcpy (xtn2->name, QSE_T("Thr-Y"));
	xtn1->spl = &spl;
	xtn2->spl = &spl;

	qse_thr_setstacksize (thr1, 64000);
	qse_thr_setstacksize (thr2, 64000);

	if (qse_thr_start(thr1, thr_func, QSE_NULL, QSE_THR_SIGNALS_BLOCKED) <= -1)
	{
		qse_printf (QSE_T("cannot start thread1\n"));
		qse_thr_close (thr1);
		return -1;
	}

	if (qse_thr_start(thr2, thr_func, QSE_NULL, QSE_THR_SIGNALS_BLOCKED) <= -1)
	{
		qse_printf (QSE_T("cannot start thread1\n"));
		qse_thr_close (thr1);
		return -1;
	}

	while (!g_stopreq)
	{
		if (qse_thr_getstate(thr1) == QSE_THR_TERMINATED &&
		    qse_thr_getstate(thr2) == QSE_THR_TERMINATED) break;
		sleep (1);
	}

	if (g_stopreq) 
	{
		xtn1->stopreq = 1;
		xtn2->stopreq = 1;
	}

	qse_thr_join (thr1);
	qse_thr_join (thr2);

	qse_printf (QSE_T("thread1 ended with retcode %d\n"), qse_thr_getretcode(thr1));
	qse_printf (QSE_T("thread2 ended with retcode %d\n"), qse_thr_getretcode(thr2));
	qse_thr_close (thr1);
	qse_thr_close (thr2);

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

	qse_open_stdsios_with_flags (QSE_SIO_LINEBREAK); /* to disable mutex protection on stdout & stderr */
	test1();
	qse_close_stdsios ();

	set_signal_to_default (SIGINT);

	return 0;
}
