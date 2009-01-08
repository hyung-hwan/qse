#include <qse/cmn/pio.h>
#include <qse/utl/stdio.h>

#include <string.h>
#include <locale.h>

#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) return -1; \
	} while (0)

static int test1 (void)
{
	qse_pio_t* pio;

	pio = qse_pio_open (
		QSE_NULL,
		0,
		QSE_T("ls -laF"),
		QSE_PIO_READOUT|QSE_PIO_WRITEIN|QSE_PIO_SHELL
	);
	if (pio == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open program through pipe\n"));
		return -1;
	}

	qse_pio_write (pio, "xxxxxxxxxxxx\n", 13, QSE_PIO_IN);
	qse_pio_write (pio, QSE_NULL, 0, QSE_PIO_IN);
	while (1)
	{
		qse_byte_t buf[128];

		/*qse_pio_canread (pio, QSE_PIO_ERR, 1000)*/
		qse_ssize_t n = qse_pio_read (pio, buf, sizeof(buf), QSE_PIO_OUT);
		if (n == 0) break;
		if (n < 0)
		{
			qse_printf (QSE_T("qse_pio_read() returned error\n"));
			break;
		}	

		#ifdef QSE_CHAR_IS_MCHAR
		qse_printf (QSE_T("buf => [%.*s]\n"), (int)n. buf);
		#else
		qse_printf (QSE_T("buf => [%.*S]\n"), (int)n, buf);
		#endif

	}

	qse_pio_close (pio);

	return 0;
}

#if 0
static int test1 (void)
{
	qse_pio_t* pio;

	pio = qse_pio_open (
		QSE_NULL,
		0,
		QSE_T("ls -laF"),
		QSE_PIO_READ | QSE_PIO_WRITE | QSE_PIO_READ_ERR
	);
	if (pio == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open program through pipe\n"));
		return -1;
	}


qse_pio_open (RW, "/bin/ls -laF", R, W, E)
	qse_pio_write ("xxx");

	qse_pio_read (buf, QSE_COUNOF(buf), QSE_PIO_OUT);
	qse_pio_read (buf, QSE_COUNOF(buf), QSE_PIO_ERR);
	qse_pio_write ("xxxx", 4, QSE_PIO_IN | QSE_PIO_EOF);

	qse_pio_close (pio);
	return 0;
}
#endif

int main ()
{
	setlocale (LC_ALL, "");

	qse_printf (QSE_T("--------------------------------------------------------------------------------\n"));
	qse_printf (QSE_T("Set the environment LANG to a Unicode locale such as UTF-8 if you see the illegal XXXXX errors. If you see such errors in Unicode locales, this program might be buggy. It is normal to see such messages in non-Unicode locales as it uses Unicode data\n"));
	qse_printf (QSE_T("--------------------------------------------------------------------------------\n"));

	R (test1);

	return 0;
}
