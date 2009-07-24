#include <qse/cmn/pio.h>
#include <qse/cmn/stdio.h>

#include <string.h>
#include <locale.h>

#ifdef _WIN32
#	include <windows.h>
#endif

#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) return -1; \
	} while (0)

static int pio1 (const qse_char_t* cmd, int oflags, qse_pio_hid_t rhid)
{
	qse_pio_t* pio;
	int x;

	pio = qse_pio_open (
		QSE_NULL,
		0,
		cmd,
		oflags
	);
	if (pio == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open program through pipe\n"));
		return -1;
	}

	while (1)
	{
		qse_byte_t buf[128];
		qse_ssize_t i;

		/*qse_pio_canread (pio, QSE_PIO_ERR, 1000)*/
		qse_ssize_t n = qse_pio_read (pio, buf, sizeof(buf), rhid);
		if (n == 0) break;
		if (n <= -1)
		{
			qse_printf (QSE_T("qse_pio_read() returned error - %s\n"), qse_pio_geterrmsg(pio));
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

	x = qse_pio_wait (pio);
	qse_printf (QSE_T("qse_pio_wait returns %d\n"), x);
	if (x <= -1)
	{
		qse_printf (QSE_T("error code : %d, error string: %s\n"), (int)qse_pio_geterrnum(pio), qse_pio_geterrmsg(pio));
	}

	qse_pio_close (pio);

	return 0;
}

static int pio2 (const qse_char_t* cmd, int oflags, qse_pio_hid_t rhid)
{
	qse_pio_t* pio;
	int x;

	pio = qse_pio_open (
		QSE_NULL,
		0,
		cmd,
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

		qse_ssize_t n = qse_pio_read (pio, buf, QSE_COUNTOF(buf), rhid);
		if (n == 0) break;
		if (n < 0)
		{
			qse_printf (QSE_T("qse_pio_read() returned error - %s\n"), qse_pio_geterrmsg(pio));
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
		qse_printf (QSE_T("error code : %d, error string: %s\n"), (int)qse_pio_geterrnum(pio), qse_pio_geterrmsg(pio));
	}

	qse_pio_close (pio);

	return 0;
}


static int test1 (void)
{

	return pio1 (
#ifdef _WIN32
		QSE_T("sll.exe"), 
#else
		QSE_T("ls -laF"),
#endif
		QSE_PIO_READOUT|QSE_PIO_WRITEIN|QSE_PIO_SHELL,
		QSE_PIO_OUT
	);
}

static int test2 (void)
{
	return pio1 (
#ifdef _WIN32
		QSE_T("sll.exe"), 
#else
		QSE_T("ls -laF"),
#endif
		QSE_PIO_READERR|QSE_PIO_OUTTOERR|QSE_PIO_WRITEIN|QSE_PIO_SHELL,
		QSE_PIO_ERR
	);
}

static int test3 (void)
{
	return pio1 (
#ifdef _WIN32
		QSE_T(".\\sll.exe"), 
#else
		QSE_T("/bin/ls -laF"), 
#endif
		QSE_PIO_READERR|QSE_PIO_OUTTOERR|QSE_PIO_WRITEIN,
		QSE_PIO_ERR
	);
}

static int test4 (void)
{
	return pio2 (
#ifdef _WIN32
		QSE_T("sll.exe"), 
#else
		QSE_T("ls -laF"),
#endif
		QSE_PIO_READOUT|QSE_PIO_WRITEIN|QSE_PIO_SHELL, 
		QSE_PIO_OUT
	);
}

static int test5 (void)
{
	return pio2 (
#ifdef _WIN32
		QSE_T("sll.exe"), 
#else
		QSE_T("ls -laF"), 
#endif
		QSE_PIO_READERR|QSE_PIO_OUTTOERR|QSE_PIO_WRITEIN|QSE_PIO_SHELL,
		QSE_PIO_ERR
	);
}

static int test6 (void)
{
	return pio2 (
#ifdef _WIN32
		QSE_T(".\\sll.exe"), 
#else
		QSE_T("/bin/ls -laF"),
#endif
		QSE_PIO_READERR|QSE_PIO_OUTTOERR|QSE_PIO_WRITEIN,
		QSE_PIO_ERR
	);
}

static int test7 (void)
{
	return pio1 (
#ifdef _WIN32
		QSE_T(".\\sll.exe"), 
#else
		QSE_T("/bin/ls -laF"), 
#endif
		QSE_PIO_READOUT|QSE_PIO_ERRTOOUT|QSE_PIO_WRITEIN,
		QSE_PIO_OUT
	);
}

static int test8 (void)
{
	return pio1 (
#ifdef _WIN32
		QSE_T("sll.exe"), 
#else
		QSE_T("ls -laF"), 
#endif
		QSE_PIO_READOUT|QSE_PIO_WRITEIN|
		QSE_PIO_OUTTONUL|QSE_PIO_ERRTONUL|QSE_PIO_INTONUL,
		QSE_PIO_OUT
	);
}

static int test9 (void)
{
	qse_pio_t* pio;
	int x;

	pio = qse_pio_open (
		QSE_NULL,
		0,
#ifdef _WIN32
		QSE_T(".\\sll.exe"),
#else
		QSE_T("/bin/ls -laF"),
#endif
		QSE_PIO_READOUT|QSE_PIO_READERR|QSE_PIO_WRITEIN
	);
	if (pio == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open program through pipe\n"));
		return -1;
	}

#ifdef _WIN32
	{
		int n = 5;

		qse_printf (QSE_T("sleeping for %d seconds\n"), n);
		Sleep (n * 1000);
		qse_printf (QSE_T("WaitForSingleObject....%d\n"),
			WaitForSingleObject (pio->child, 0));
	}
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
		qse_printf (QSE_T("error code : %d, error string: %s\n"), (int)QSE_PIO_ERRNUM(pio), qse_pio_geterrmsg(pio));
	}

	qse_pio_close (pio);
	return 0;
}

int main ()
{
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

	return 0;
}
