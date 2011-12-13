#include <qse/cmn/sio.h>
#include <locale.h>

#define R(f) \
	do { \
		qse_sio_puts (qse_sio_out,QSE_T("== ")); \
		qse_sio_puts (qse_sio_out,QSE_T(#f)); \
		qse_sio_puts (qse_sio_out,QSE_T(" ==\n")); \
		if (f() == -1) return -1; \
	} while (0)

static int test1 (void)
{
	qse_sio_t* sio;
	int i;
	const qse_wchar_t* x[] =
	{
		L"\uB108 \uBB50\uAC00 \uC798\uB0AC\uC5B4?",
		L"Fly to the universe, kick you ass",
		L"\uB108 \uBB50\uAC00 \uC798\uB0AC\uC5B4?",
		L"Fly to the universe, kick you ass",
		L"Fly to the universe, kick you ass",
		L"\uB108 \uBB50\uAC00 \uC798\uB0AC\uC5B4?"
        };

	sio = qse_sio_open (QSE_NULL, 0, QSE_T("sio.txt"), 
		QSE_SIO_WRITE | QSE_SIO_CREATE | QSE_SIO_TRUNCATE);

	if (sio == QSE_NULL)
	{
		qse_sio_puts (qse_sio_err, QSE_T("cannot open file\n"));
		return -1;
	}


	for (i = 0; i < QSE_COUNTOF(x); i++)
	{
		qse_sio_puts (qse_sio_out, QSE_T("written: ["));
		qse_sio_puts (qse_sio_out, x[i]);
		qse_sio_puts (qse_sio_out, QSE_T("]\n"));

		qse_sio_puts (sio, x[i]);
		qse_sio_putc (sio, QSE_T('\n'));
	}

	qse_sio_close (sio);

	return 0;
}

static int test2 (void)
{
	qse_ssize_t n;
	qse_char_t buf[20];
	
	qse_sio_puts (qse_sio_out, QSE_T("type something...\n"));
	while (1)
	{
		n = qse_sio_gets (qse_sio_in, buf, QSE_COUNTOF(buf));
		if (n == 0) break;
		if (n < 0) 
		{
			qse_sio_puts (qse_sio_err, QSE_T("error ....\n"));
			break;
		}

		qse_sio_puts (qse_sio_out, buf);
	}

	return 0;
}

int main ()
{
	setlocale (LC_ALL, "");

	qse_sio_puts (qse_sio_out, QSE_T("--------------------------------------------------------------------------------\n"));
	qse_sio_puts (qse_sio_out, QSE_T("Set the environment LANG to a Unicode locale such as UTF-8 if you see the illegal XXXXX errors. If you see such errors in Unicode locales, this program might be buggy. It is normal to see such messages in non-Unicode locales as it uses Unicode data\n"));
	qse_sio_puts (qse_sio_out, QSE_T("--------------------------------------------------------------------------------\n"));

	R (test1);
	R (test2);

	return 0;
}
