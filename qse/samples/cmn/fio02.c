#include <qse/cmn/fio.h>
#include <qse/cmn/stdio.h>

#include <string.h>
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
	int i;
	qse_char_t file[] = QSE_T("fio02-XXXX");

	fio = qse_fio_open (
		QSE_NULL, 
		0, 
		file,
          QSE_FIO_CREATE | QSE_FIO_EXCLUSIVE | QSE_FIO_TEMPORARY | QSE_FIO_READ | QSE_FIO_WRITE,
		QSE_FIO_RUSR|QSE_FIO_WUSR|QSE_FIO_RGRP|QSE_FIO_ROTH
	);
	if (fio == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open file - last file tried [%s]\n"), file);
		return -1;
	}

	qse_printf (QSE_T("file opened: [%s]\n"), file);

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
			qse_printf (QSE_T("file offset at %ld\n"), (long)off);
		}

		off = qse_fio_seek (fio, 0, QSE_FIO_BEGIN);
		if (off == (qse_fio_off_t)-1)
		{
			qse_printf (QSE_T("failed to set file offset\n"));
		}
		else
		{
			qse_printf (QSE_T("moved file offset to %ld\n"), (long)off);
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

	return 0;
}
