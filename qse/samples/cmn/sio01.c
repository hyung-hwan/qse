#include <qse/cmn/sio.h>
#include <qse/cmn/fmt.h>
#include <qse/cmn/stdio.h>
#include <locale.h>

#if defined(_WIN32)
#	include <windows.h>
#endif


#define R(f) \
	do { \
		qse_printf (QSE_T("== ")); \
		qse_printf (QSE_T(#f)); \
		qse_printf (QSE_T(" ==\n")); \
		qse_fflush (QSE_STDOUT); \
		if (f() == -1) return -1; \
	} while (0)

static int test1 (void)
{
	qse_sio_t* sio;
	int i;

	const qse_wchar_t unistr[] =
	{
		/*L"\uB108 \uBB50\uAC00 \uC798\uB0AC\uC5B4!",*/
		0xB108,
		L' ',
		0xBB50,
		0xAC00,
		L' ',
		0xC798,
		0xB0AC,
		0xC5B4,
		L'!',
		L'\0'
     };

	const qse_wchar_t* x[] =
	{
		L"",
		L"Fly to the universe, kick you ass",
		L"",
		L"Fly to the universe, kick you ass",
		L"Fly to the universe, kick you ass",
		L""
        };

	x[0] = unistr;
	x[2] = unistr;
	x[5] = unistr;
	sio = qse_sio_open (QSE_NULL, 0, QSE_T("sio.txt"), 
		QSE_SIO_WRITE | QSE_SIO_CREATE | QSE_SIO_TRUNCATE);

	if (sio == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open file\n"));
		return -1;
	}

	for (i = 0; i < QSE_COUNTOF(x); i++)
	{
		qse_printf (QSE_T("written: ["));
		qse_printf (x[i]);
		qse_printf (QSE_T("]\n"));

		qse_sio_putstr (sio, x[i]);
		qse_sio_putc (sio, QSE_T('\n'));
	}

	qse_sio_close (sio);

	return 0;
}

static int test2 (void)
{
	qse_ssize_t n;
	qse_wchar_t buf[20];
	qse_sio_t* in, * out;
	
	in = qse_sio_openstd (QSE_NULL, 0, QSE_SIO_STDIN, QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR);
	out = qse_sio_openstd (QSE_NULL, 0, QSE_SIO_STDOUT, QSE_SIO_WRITE | QSE_SIO_IGNOREMBWCERR);
	
	qse_sio_putstr (out, QSE_T("Type something here:\n"));
	while (1)
	{
		n = qse_sio_getwcs (in, buf, QSE_COUNTOF(buf));
		if (n == 0) break;
		if (n <= -1) 
		{
			qse_char_t buf[32];
			qse_fmtintmax (buf, QSE_COUNTOF(buf), qse_sio_geterrnum(in), 10, -1, QSE_T('\0'), QSE_NULL);
			qse_sio_putstr (out, QSE_T("ERROR .... "));
			qse_sio_putstr (out, buf);
			qse_sio_putstr (out, QSE_T("\n"));
			break;
		}

		qse_sio_putwcs (out, buf);
	}

	qse_sio_close (out);
	qse_sio_close (in);
	return 0;
}

static int test3 (void)
{
	qse_ssize_t n;
	qse_mchar_t buf[20];
	qse_sio_t* in, * out;
	
	in = qse_sio_openstd (QSE_NULL, 0, QSE_SIO_STDIN, QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR);
	out = qse_sio_openstd (QSE_NULL, 0, QSE_SIO_STDOUT, QSE_SIO_WRITE | QSE_SIO_IGNOREMBWCERR);

	qse_sio_putstr (out, QSE_T("Type something here:\n"));
	while (1)
	{
		n = qse_sio_getmbs (in, buf, QSE_COUNTOF(buf));
		if (n == 0) break;
		if (n < 0) 
		{
			qse_char_t buf[32];
			qse_fmtintmax (buf, QSE_COUNTOF(buf), qse_sio_geterrnum(in), 10, -1, QSE_T('\0'), QSE_NULL);
			qse_sio_putstr (out, QSE_T("error .... "));
			qse_sio_putstr (out, buf);
			qse_sio_putstr (out, QSE_T("\n"));
			break;
		}

		qse_sio_putmbs (out, buf);
	}

	qse_sio_close (out);
	qse_sio_close (in);
	return 0;
}


int main ()
{
#if defined(_WIN32)
	UINT old_cp = GetConsoleOutputCP();
	SetConsoleOutputCP (CP_UTF8);
#endif

	setlocale (LC_ALL, "");

	qse_printf (QSE_T("--------------------------------------------------------------------------------\n"));
	qse_printf (QSE_T("Set the environment LANG to a Unicode locale such as UTF-8 if you see the illegal XXXXX errors. If you see such errors in Unicode locales, this program might be buggy. It is normal to see such messages in non-Unicode locales as it uses Unicode data\n"));
	qse_printf (QSE_T("--------------------------------------------------------------------------------\n"));

	R (test1);
	R (test2);
	R (test3);

#if defined(_WIN32)
	SetConsoleOutputCP (old_cp);
#endif
	return 0;
}
