#include <qse/cmn/pio.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/env.h>
#include <qse/cmn/sio.h>

#include <locale.h>

#if defined(_WIN32)
#	include <windows.h>
#elif defined(__OS2__)
#	define INCL_DOSPROCESS
#	define INCL_DOSERRORS
#	define INCL_DOSDATETIME
#	include <os2.h>
#else
#	include <unistd.h>
#	include <sys/wait.h>
#endif

#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) return -1; \
	} while (0)

static int pio1 (const qse_char_t* cmd, qse_env_t* env, int oflags, qse_pio_hid_t rhid)
{
	qse_pio_t pio;
	int x;

	if (qse_pio_init (&pio, QSE_MMGR_GETDFL(), cmd, env, oflags) <= -1)
	{
		qse_printf (QSE_T("cannot open program through pipe - %d\n"), 
			(int)qse_pio_geterrnum(&pio));
		return -1;
	}

	while (1)
	{
		qse_byte_t buf[128];
		qse_ssize_t i;

		/*qse_pio_canread (&pio, QSE_PIO_ERR, 1000)*/
		qse_ssize_t n = qse_pio_read (&pio, rhid, buf, QSE_SIZEOF(buf));
		if (n == 0) break;
		if (n <= -1)
		{
			qse_printf (
				QSE_T("qse_pio_read() returned error - %d\n"),
				(int)qse_pio_geterrnum(&pio)
			);
			break;
		}	

		qse_printf (QSE_T("N===> %d buf => ["), (int)n);
		for (i = 0; i < n; i++)
		{
		#ifdef QSE_CHAR_IS_MCHAR
			qse_printf (QSE_T("%c"), buf[i]);
		#else
			qse_printf (QSE_T("%C"), buf[i]);
		#endif
		}	
		qse_printf (QSE_T("]\n"));
	}

	x = qse_pio_wait (&pio);
	qse_printf (QSE_T("qse_pio_wait returns %d\n"), x);
	if (x <= -1)
	{
		qse_printf (QSE_T("error code : %d\n"), (int)qse_pio_geterrnum(&pio));
	}

	qse_pio_fini (&pio);

	return 0;
}

static int pio2 (const qse_char_t* cmd, qse_env_t* env, int oflags, qse_pio_hid_t rhid)
{
	qse_pio_t* pio;
	int x;

	pio = qse_pio_open (
		QSE_MMGR_GETDFL(),
		0,
		cmd,
		env,
		oflags | QSE_PIO_TEXT
	);
	if (pio == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open program through pipe\n"));
		return -1;
	}

	while (1)
	{
		qse_char_t buf[128];
		qse_ssize_t i;

		qse_ssize_t n = qse_pio_read (pio, rhid, buf, QSE_COUNTOF(buf));
		if (n == 0) break;
		if (n < 0)
		{
			qse_printf (
				QSE_T("qse_pio_read() returned error - %d\n"),
				(int)qse_pio_geterrnum(pio)
			);
			break;
		}	

		qse_printf (QSE_T("N===> %d buf => ["), (int)n);
		for (i = 0; i < n; i++)
		{
			qse_printf (QSE_T("%c"), buf[i]);
		}
		qse_printf (QSE_T("]\n"));
	}

	x = qse_pio_wait (pio);
	qse_printf (QSE_T("qse_pio_wait returns %d\n"), x);
	if (x <= -1)
	{
		qse_printf (QSE_T("error code : %d\n"), (int)qse_pio_geterrnum(pio));
	}

	qse_pio_close (pio);

	return 0;
}


static int test1 (void)
{

	return pio1 (
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
		QSE_T("dir /a"), 
#else
		QSE_T("ls -laF"),
#endif
		QSE_NULL,
		QSE_PIO_READOUT|QSE_PIO_WRITEIN|QSE_PIO_SHELL,
		QSE_PIO_OUT
	);
}

static int test2 (void)
{
	return pio1 (
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
		QSE_T("dir /a"), 
#else
		QSE_T("ls -laF"),
#endif
		QSE_NULL,
		QSE_PIO_READERR|QSE_PIO_OUTTOERR|QSE_PIO_WRITEIN|QSE_PIO_SHELL,
		QSE_PIO_ERR
	);
}

static int test3 (void)
{
	return pio1 (
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
		QSE_T("tree.com"), 
#else
		QSE_T("/bin/ls -laF"), 
#endif
		QSE_NULL,
		QSE_PIO_READERR|QSE_PIO_OUTTOERR|QSE_PIO_WRITEIN,
		QSE_PIO_ERR
	);
}

static int test4 (void)
{
	return pio2 (
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
		QSE_T("dir /a"), 
#else
		QSE_T("ls -laF"),
#endif
		QSE_NULL,
		QSE_PIO_READOUT|QSE_PIO_WRITEIN|QSE_PIO_SHELL, 
		QSE_PIO_OUT
	);
}

static int test5 (void)
{
	return pio2 (
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
		QSE_T("dir /a"), 
#else
		QSE_T("ls -laF"), 
#endif
		QSE_NULL,
		QSE_PIO_READERR|QSE_PIO_OUTTOERR|QSE_PIO_WRITEIN|QSE_PIO_SHELL,
		QSE_PIO_ERR
	);
}

static int test6 (void)
{
	return pio2 (
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
		QSE_T("tree.com"), 
#else
		QSE_T("/bin/ls -laF"),
#endif
		QSE_NULL,
		QSE_PIO_READERR|QSE_PIO_OUTTOERR|QSE_PIO_WRITEIN,
		QSE_PIO_ERR
	);
}

static int test7 (void)
{
	return pio1 (
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
		QSE_T("tree.com"), 
#else
		QSE_T("/bin/ls -laF"), 
#endif
		QSE_NULL,
		QSE_PIO_READOUT|QSE_PIO_ERRTOOUT|QSE_PIO_WRITEIN,
		QSE_PIO_OUT
	);
}

static int test8 (void)
{
	return pio1 (
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
		QSE_T("tree.com"), 
#else
		QSE_T("/bin/ls -laF"), 
#endif
		QSE_NULL,
		QSE_PIO_READOUT|QSE_PIO_WRITEIN|
		QSE_PIO_OUTTONUL|QSE_PIO_ERRTONUL|QSE_PIO_INTONUL,
		QSE_PIO_OUT
	);
}

static int test9 (void)
{
	return pio1 (
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
		(const qse_char_t*)"tree.com", 
#else
		(const qse_char_t*)"/bin/ls -laF", 
#endif
		QSE_NULL,
		QSE_PIO_MBSCMD|QSE_PIO_READOUT|QSE_PIO_WRITEIN,
		QSE_PIO_OUT
	);
}

static int test10 (void)
{
	return pio1 (
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
		(const qse_char_t*)"dir /a",
#else
		(const qse_char_t*)"ls -laF", 
#endif
		QSE_NULL,
		QSE_PIO_MBSCMD|QSE_PIO_READOUT|QSE_PIO_WRITEIN|QSE_PIO_SHELL,
		QSE_PIO_OUT
	);
}

static int test11 (void)
{
	return pio1 (
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
		(const qse_char_t*)"dir /a",
#else
		(const qse_char_t*)"ls -laF", 
#endif
		QSE_NULL,
		QSE_PIO_MBSCMD|QSE_PIO_READOUT|QSE_PIO_WRITEIN|QSE_PIO_DROPERR|QSE_PIO_INTONUL|QSE_PIO_SHELL,
		QSE_PIO_OUT
	);
}


static int test12 (void)
{
	qse_env_t* env;
	int n;
	
	env = qse_env_open (QSE_MMGR_GETDFL(), 0, 0);
	if (env == QSE_NULL) return -1;

	qse_env_insert (env, QSE_T("PATH"), QSE_NULL);
	qse_env_insert (env, QSE_T("HELLO"), QSE_T("WORLD"));

	n = pio1 (
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
		QSE_T("set"),
#else
		QSE_T("printenv"),
#endif
		env,
		QSE_PIO_READOUT|QSE_PIO_WRITEIN|QSE_PIO_SHELL,
		QSE_PIO_OUT
	);

	qse_env_close (env);
	return n;
}

static int test13 (void)
{
	qse_pio_t* pio;
	int x;

	pio = qse_pio_open (
		QSE_MMGR_GETDFL(),
		0,
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
		QSE_T("tree.com"),
#else
		QSE_T("/bin/ls -laF"),
#endif
		QSE_NULL,
		QSE_PIO_READOUT|QSE_PIO_READERR|QSE_PIO_WRITEIN
	);
	if (pio == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open program through pipe\n"));
		return -1;
	}

#if defined(_WIN32)
	{
		int n = 5;

		qse_printf (QSE_T("sleeping for %d seconds\n"), n);
		Sleep (n * 1000);
		qse_printf (QSE_T("WaitForSingleObject....%d\n"),
			(int)WaitForSingleObject (pio->child, INFINITE));
	}
#elif defined(__OS2__)
	{
		int n = 5;
		RESULTCODES result;
		PID pid;

		qse_printf (QSE_T("sleeping for %d seconds\n"), n);
		DosSleep (n * 1000);

		/* it doesn't seem to proceed if the pipe is not read out.
		 * maybe the OS2 pipe buffer is too smally?? */
		while (1)
		{
			qse_mchar_t buf[100];
			qse_ssize_t x = qse_pio_read (pio, QSE_PIO_OUT, buf, QSE_SIZEOF(buf));
			if (x <= 0) break;
		}

		qse_printf (QSE_T("DosWaitChild....%d\n"),
			(int)DosWaitChild (DCWA_PROCESS, DCWW_WAIT, &result, &pid, pio->child));
	}

#elif defined(__DOS__)

	#error NOT SUPPORTED

#else
	{
		int status;
		int n = 5;

		qse_printf (QSE_T("sleeping for %d seconds\n"), n);
		sleep (n);
		qse_printf (QSE_T("waitpid...%d\n"),  (int)waitpid (-1, &status, 0));
	}
#endif

	x = qse_pio_wait (pio);
	qse_printf (QSE_T("qse_pio_wait returns %d\n"), x);
	if (x == -1)
	{
		qse_printf (QSE_T("error code : %d\n"), (int)QSE_PIO_ERRNUM(pio));
	}

	qse_pio_close (pio);
	return 0;
}

int main ()
{
	qse_openstdsios ();
	setlocale (LC_ALL, "");

	qse_printf (QSE_T("--------------------------------------------------------------------------------\n"));
	qse_printf (QSE_T("Set the environment LANG to a Unicode locale such as UTF-8 if you see the illegal XXXXX errors. If you see such errors in Unicode locales, this program might be buggy. It is normal to see such messages in non-Unicode locales as it uses Unicode data\n"));
	qse_printf (QSE_T("--------------------------------------------------------------------------------\n"));

	R (test1);
	R (test2);
	R (test3);
	R (test4);
	R (test5);
	R (test6);
	R (test7);
	R (test8);
	R (test9);
	R (test10);
	R (test11);
	R (test12);
	R (test13);
	qse_closestdsios ();
	return 0;
}
