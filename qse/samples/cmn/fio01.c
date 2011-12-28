#include <qse/cmn/fio.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/stdio.h>

#include <locale.h>

#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) return -1; \
	} while (0)

static int test1 (void)
{
	qse_fio_t* fio;
	qse_ssize_t n;
	char x[] = "fio test";
	char x2[] = "fio test2";
	qse_fio_off_t off;
	char buf[1000];

	fio = qse_fio_open (
		QSE_MMGR_GETDFL(),
		0,
		QSE_T("fio1.txt"), 
		QSE_FIO_READ|QSE_FIO_WRITE|QSE_FIO_CREATE|QSE_FIO_TRUNCATE, 
		QSE_FIO_RUSR|QSE_FIO_WUSR|QSE_FIO_RGRP|QSE_FIO_ROTH
	);
	if (fio == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open file\n"));
		return -1;
	}

	n = qse_fio_write (fio, x, qse_mbslen(x));
	qse_printf (QSE_T("written %d bytes\n"), (int)n);


	off = qse_fio_seek (fio, 0, QSE_FIO_CURRENT);
	if (off == (qse_fio_off_t)-1)
	{
		qse_printf (QSE_T("failed to get file offset\n"));
	}
	else
	{
		qse_printf (QSE_T("file offset at %lld\n"), (long long)off);
	}

	off = qse_fio_seek (fio, 0, QSE_FIO_BEGIN);
	if (off == (qse_fio_off_t)-1)
	{
		qse_printf (QSE_T("failed to set file offset\n"));
	}
	else
	{
		qse_printf (QSE_T("moved file offset to %lld\n"), (long long)off);
	}

	n = qse_fio_read (fio, buf, sizeof(buf));
	qse_printf (QSE_T("read %d bytes \n"), (int)n);
	if (n > 0)
	{
	#ifdef QSE_CHAR_IS_MCHAR
	qse_printf (QSE_T("buf => [%.*s]\n"), (int)n, buf);
	#else
	qse_printf (QSE_T("buf => [%.*S]\n"), (int)n, buf);
	#endif
	}

	off = qse_fio_seek (fio, QSE_TYPE_MAX(int) * 3llu, QSE_FIO_BEGIN);
	if (off == (qse_fio_off_t)-1)
	{
		qse_printf (QSE_T("failed to set file offset\n"));
	}
	else
	{
		qse_printf (QSE_T("moved file offset to %lld\n"), (long long)off);
	}

	n = qse_fio_write (fio, x2, qse_mbslen(x2));
	qse_printf (QSE_T("written %d bytes\n"), (int)n);

	if (qse_fio_chmod (fio, QSE_FIO_RUSR|QSE_FIO_RGRP) <= -1)
	{
		qse_printf (QSE_T("failed to change mode\n"));
	}
	else
	{
		qse_printf (QSE_T("changed mode\n"));
	}

	qse_fio_close (fio);

	return 0;
}

static int test2 (void)
{
	qse_fio_t* fio;
	qse_ssize_t n;
	char x[] = "fio test";
	char x2[] = "fio test2";
	qse_fio_off_t off;
	char buf[1000];
	int i;

	fio = qse_fio_open (
		QSE_MMGR_GETDFL(), 
		0, 
		QSE_T("fio2.txt"), 
		QSE_FIO_CREATE | QSE_FIO_TRUNCATE | QSE_FIO_APPEND, 
		QSE_FIO_RUSR|QSE_FIO_WUSR|QSE_FIO_RGRP|QSE_FIO_ROTH
	);
	if (fio == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open file\n"));
		return -1;
	}

	for (i = 0; i < 2; i++)
	{
		n = qse_fio_write (fio, x, qse_mbslen(x));
		qse_printf (QSE_T("written %d bytes\n"), (int)n);

		off = qse_fio_seek (fio, 0, QSE_FIO_CURRENT);
		if (off == (qse_fio_off_t)-1)
		{
			qse_printf (QSE_T("failed to get file offset\n"));
		}
		else
		{
			qse_printf (QSE_T("file offset at %lld\n"), (long long)off);
		}

		off = qse_fio_seek (fio, 0, QSE_FIO_BEGIN);
		if (off == (qse_fio_off_t)-1)
		{
			qse_printf (QSE_T("failed to set file offset\n"));
		}
		else
		{
			qse_printf (QSE_T("moved file offset to %lld\n"), (long long)off);
		}
	}


	n = qse_fio_read (fio, buf, sizeof(buf));
	qse_printf (QSE_T("read %d bytes \n"), (int)n);
	if (n > 0)
	{
	#ifdef QSE_CHAR_IS_MCHAR
	qse_printf (QSE_T("buf => [%.*s]\n"), (int)n, buf);
	#else
	qse_printf (QSE_T("buf => [%.*S]\n"), (int)n, buf);
	#endif
	}

	off = qse_fio_seek (fio, QSE_TYPE_MAX(int) * 2llu, QSE_FIO_BEGIN);
	if (off == (qse_fio_off_t)-1)
	{
		qse_printf (QSE_T("failed to set file offset\n"));
	}
	else
	{
		qse_printf (QSE_T("moved file offset to %lld\n"), (long long)off);
	}

	n = qse_fio_write (fio, x2, qse_mbslen(x2));
	qse_printf (QSE_T("written %d bytes\n"), (int)n);

	off = qse_fio_seek (fio, 0, QSE_FIO_CURRENT);
	if (off == (qse_fio_off_t)-1)
	{
		qse_printf (QSE_T("failed to get file offset\n"));
	}
	else
	{
		qse_printf (QSE_T("file offset at %lld\n"), (long long)off);
	}

	if (qse_fio_truncate (fio, 20000) == -1)
	{
		qse_printf (QSE_T("failed to truncate the file\n"));
	}

	n = qse_fio_write (fio, x2, qse_mbslen(x2));
	qse_printf (QSE_T("written %d bytes\n"), (int)n);

	off = qse_fio_seek (fio, 0, QSE_FIO_CURRENT);
	if (off == (qse_fio_off_t)-1)
	{
		qse_printf (QSE_T("failed to get file offset\n"));
	}
	else
	{
		qse_printf (QSE_T("file offset at %lld\n"), (long long)off);
	}

	/* on _WIN32, this will fail as this file is opened without 
	 * QSE_FIO_READ. */
	if (qse_fio_chmod (fio, QSE_FIO_RUSR|QSE_FIO_RGRP) <= -1)
	{
		qse_printf (QSE_T("failed to change mode\n"));
	}
	else
	{
		qse_printf (QSE_T("changed mode\n"));
	}

	qse_fio_close (fio);

	return 0;
}

static int test3 (void)
{
	qse_fio_t* fio;
	qse_ssize_t n;
	const qse_char_t* x = QSE_T("\uB108 \uBB50\uAC00 \uC798\uB0AC\uC5B4?");
	qse_fio_off_t off;
	qse_char_t buf[1000];

	fio = qse_fio_open (
		QSE_MMGR_GETDFL(),
		0,
		QSE_T("fio3.txt"), 

		QSE_FIO_TEXT | QSE_FIO_READ | QSE_FIO_WRITE |
		QSE_FIO_CREATE | QSE_FIO_TRUNCATE, 

		QSE_FIO_RUSR|QSE_FIO_WUSR|QSE_FIO_RGRP|QSE_FIO_ROTH
	);
	if (fio == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open file\n"));
		return -1;
	}

	n = qse_fio_write (fio, x, qse_strlen(x));
	qse_printf (QSE_T("written %d chars\n"), (int)n);

	n = qse_fio_flush (fio);
	qse_printf (QSE_T("flushed %d chars\n"), (int)n);

	off = qse_fio_seek (fio, 0, QSE_FIO_BEGIN);
	if (off == (qse_fio_off_t)-1)
	{
		qse_printf (QSE_T("failed to get file offset\n"));
	}


	n = qse_fio_read (fio, buf, QSE_COUNTOF(buf));
	qse_printf (QSE_T("read %d chars\n"), (int)n);
	if (n > 0)
	{
		qse_printf (QSE_T("[%.*s]\n"), (int)n,  buf);
	}

	
	qse_fio_close (fio);

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

	qse_printf (QSE_T("--------------------------------------------------------------------------------\n"));
	qse_printf (QSE_T("Run \"rm -f fio?.txt\" to delete garbages\n"));
	qse_printf (QSE_T("--------------------------------------------------------------------------------\n"));

	return 0;
}
