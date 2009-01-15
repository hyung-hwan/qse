#include <qse/cmn/pcp.h>
#include <qse/utl/stdio.h>

#include <string.h>
#include <locale.h>

#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) return -1; \
	} while (0)

static int pcp1 (const qse_char_t* cmd, int oflags, qse_pcp_hid_t rhid)
{
	qse_pcp_t* pcp;
	int x;

	pcp = qse_pcp_open (
		QSE_NULL,
		0,
		cmd,
		oflags
	);
	if (pcp == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open program through pipe\n"));
		return -1;
	}

	while (1)
	{
		qse_byte_t buf[128];

		/*qse_pcp_canread (pcp, QSE_PCP_ERR, 1000)*/
		qse_ssize_t n = qse_pcp_read (pcp, buf, sizeof(buf), rhid);
		if (n == 0) break;
		if (n < 0)
		{
			qse_printf (QSE_T("qse_pcp_read() returned error - %s\n"), qse_pcp_geterrstr(pcp));
			break;
		}	

		qse_printf (QSE_T("N===> %d\n"), (int)n);
		#ifdef QSE_CHAR_IS_MCHAR
		qse_printf (QSE_T("buf => [%.*s]\n"), (int)n, buf);
		#else
		qse_printf (QSE_T("buf => [%.*S]\n"), (int)n, buf);
		#endif

	}

	x = qse_pcp_wait (pcp, 0);
	qse_printf (QSE_T("qse_pcp_wait returns %d\n"), x);
	if (x == -1)
	{
		qse_printf (QSE_T("error code : %d, error string: %s\n"), (int)qse_pcp_geterrnum(pcp), qse_pcp_geterrstr(pcp));
	}

	qse_pcp_close (pcp);

	return 0;
}

static int pcp2 (const qse_char_t* cmd, int oflags, qse_pcp_hid_t rhid)
{
	qse_pcp_t* pcp;
	int x;

	pcp = qse_pcp_open (
		QSE_NULL,
		0,
		cmd,
		oflags | QSE_PCP_TEXT
	);
	if (pcp == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open program through pipe\n"));
		return -1;
	}

	while (1)
	{
		qse_char_t buf[128];

		qse_ssize_t n = qse_pcp_read (pcp, buf, QSE_COUNTOF(buf), rhid);
		if (n == 0) break;
		if (n < 0)
		{
			qse_printf (QSE_T("qse_pcp_read() returned error - %s\n"), qse_pcp_geterrstr(pcp));
			break;
		}	

		qse_printf (QSE_T("N===> %d\n"), (int)n);
		qse_printf (QSE_T("buf => [%.*s]\n"), (int)n, buf);
	}

	x = qse_pcp_wait (pcp, 0);
	qse_printf (QSE_T("qse_pcp_wait returns %d\n"), x);
	if (x == -1)
	{
		qse_printf (QSE_T("error code : %d, error string: %s\n"), (int)qse_pcp_geterrnum(pcp), qse_pcp_geterrstr(pcp));
	}

	qse_pcp_close (pcp);

	return 0;
}


static int test1 (void)
{
	return pcp1 (QSE_T("ls -laF"), QSE_PCP_READOUT|QSE_PCP_WRITEIN|QSE_PCP_SHELL, QSE_PCP_OUT);
}

static int test2 (void)
{
	return pcp1 (QSE_T("ls -laF"), QSE_PCP_READERR|QSE_PCP_OUTTOERR|QSE_PCP_WRITEIN|QSE_PCP_SHELL, QSE_PCP_ERR);
}

static int test3 (void)
{
	return pcp1 (QSE_T("/bin/ls -laF"), QSE_PCP_READERR|QSE_PCP_OUTTOERR|QSE_PCP_WRITEIN, QSE_PCP_ERR);
}

static int test4 (void)
{
	return pcp2 (QSE_T("ls -laF"), QSE_PCP_READOUT|QSE_PCP_WRITEIN|QSE_PCP_SHELL, QSE_PCP_OUT);
}

static int test5 (void)
{
	return pcp2 (QSE_T("ls -laF"), QSE_PCP_READERR|QSE_PCP_OUTTOERR|QSE_PCP_WRITEIN|QSE_PCP_SHELL, QSE_PCP_ERR);
}

static int test6 (void)
{
	return pcp2 (QSE_T("/bin/ls -laF"), QSE_PCP_READERR|QSE_PCP_OUTTOERR|QSE_PCP_WRITEIN, QSE_PCP_ERR);
}

static int test7 (void)
{
	qse_pcp_t* pcp;
	int x;

	pcp = qse_pcp_open (
		QSE_NULL,
		0,
		QSE_T("/bin/ls -laF"),
		QSE_PCP_READOUT|QSE_PCP_ERRTOOUT|QSE_PCP_WRITEIN
	);
	if (pcp == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open program through pipe\n"));
		return -1;
	}

	while (1)
	{
		qse_byte_t buf[128];

		/*qse_pcp_canread (pcp, QSE_PCP_ERR, 1000)*/
		qse_ssize_t n = qse_pcp_read (pcp, buf, sizeof(buf), QSE_PCP_OUT);
		if (n == 0) break;
		if (n < 0)
		{
			qse_printf (QSE_T("qse_pcp_read() returned error - %s\n"), qse_pcp_geterrstr(pcp));
			break;
		}	
	}

	x = qse_pcp_wait (pcp, 0);
	qse_printf (QSE_T("qse_pcp_wait returns %d\n"), x);
	if (x == -1)
	{
		qse_printf (QSE_T("error code : %d, error string: %s\n"), (int)QSE_PCP_ERRNUM(pcp), qse_pcp_geterrstr(pcp));
	}

	qse_pcp_close (pcp);
	return 0;
}

static int test8 (void)
{
	qse_pcp_t* pcp;
	int x;

	pcp = qse_pcp_open (
		QSE_NULL,
		0,
		QSE_T("/bin/ls -laF"),
		QSE_PCP_READOUT|QSE_PCP_READERR|QSE_PCP_WRITEIN
	);
	if (pcp == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open program through pipe\n"));
		return -1;
	}

	{
		int status;
		int n = 5;

		qse_printf (QSE_T("sleeping for %d seconds\n"), n);
		sleep (n);
		qse_printf (QSE_T("waitpid...%d\n"),  (int)waitpid (-1, &status, 0));
	}

	x = qse_pcp_wait (pcp, 0);
	qse_printf (QSE_T("qse_pcp_wait returns %d\n"), x);
	if (x == -1)
	{
		qse_printf (QSE_T("error code : %d, error string: %s\n"), (int)QSE_PCP_ERRNUM(pcp), qse_pcp_geterrstr(pcp));
	}

	qse_pcp_close (pcp);
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

	return 0;
}
