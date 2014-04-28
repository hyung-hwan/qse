#include <qse/cmn/sio.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/mem.h>

#include <locale.h>

#if defined(_WIN32)
#	include <windows.h>
#endif

static qse_sio_t* g_out;

#define R(f) \
	do { \
		qse_sio_putstrf (g_out, QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) return -1; \
	} while (0)

static int test1 (void)
{
	const qse_wchar_t unistr[] = 
	{
		/* ugly hack for old compilers that don't support \u */
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

	const qse_wchar_t unistr2[] = 
	{
		/* this include an illegal unicode character.
		 * a strict converter should return an error so a question mark 
		 * should be printed for such a character */
		0xFFFF53C0u,
		0xFFFF4912u,
		0xBA00u,
		0xFFFF1234u,
		L'\0'
	};
	const qse_wchar_t* x[] =
	{
		L"",
		L"",
		L"",
		L"Fly to the universe, kick your ass"
	};
	int i;
	qse_sio_t* sio;

	x[1] = unistr;
	x[2] = unistr2;

	sio = qse_sio_openstd (QSE_MMGR_GETDFL(), 0, 
		QSE_SIO_STDOUT, QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR | QSE_SIO_NOAUTOFLUSH);
	if (sio == QSE_NULL) return -1;

	for (i = 0; i < QSE_COUNTOF(x); i++)
	{
		qse_sio_putwcs (sio, x[i]);
		qse_sio_putwc (sio, QSE_WT('\n'));
	}

	qse_sio_close (sio);
	return 0;
}

static int test2 (void)
{
	/* this file is in utf8, the following strings may not be shown properly
	 * if your locale/codepage is not utf8 */
	const qse_mchar_t* x[] =
	{
		QSE_MT("\0\0\0"),
		QSE_MT("이거슨"),
		QSE_MT("뭐냐이거"),
		QSE_MT("過去一個月"),
		QSE_MT("是成功的建商"),
		QSE_MT("뛰어 올라봐. 멀리멀리 잘난척하기는"),
		QSE_MT("Fly to the universe")
	};
	int i;
	qse_sio_t* sio;

	sio = qse_sio_openstd (QSE_MMGR_GETDFL(), 0, QSE_SIO_STDOUT, QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR | QSE_SIO_NOAUTOFLUSH);
	if (sio == QSE_NULL) return -1;

	for (i = 0; i < QSE_COUNTOF(x); i++)
	{
		qse_sio_putmbs (sio, x[i]);
		qse_sio_putmb (sio, QSE_MT('\n'));
	}

	qse_sio_close (sio);
	return 0;
}

static int test3 (void)
{
	/* this file is in utf8, the following strings may not be shown properly
	 * if your locale/codepage is not utf8 */
	const qse_mchar_t* x[] =
	{
		QSE_MT("\0\0\0"),
		QSE_MT("이거슨"),
		QSE_MT("뭐냐이거"),
		QSE_MT("過去一個月"),
		QSE_MT("是成功的建商"),
		QSE_MT("뛰어 올라봐. 멀리멀리 잘난척하기는"),
		QSE_MT("Fly to the universe")
	};
	int i;
	qse_sio_t* sio;

	sio = qse_sio_openstd (QSE_MMGR_GETDFL(), 0, 
		QSE_SIO_STDOUT, QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR | QSE_SIO_NOAUTOFLUSH);
	if (sio == QSE_NULL) return -1;

	for (i = 0; i < QSE_COUNTOF(x); i++)
	{
		qse_sio_putmbsn (sio, x[i], qse_mbslen(x[i]));
		qse_sio_putmb (sio, QSE_MT('\n'));
	}

	qse_sio_close (sio);
	return 0;
}

static int test4 (void)
{
	qse_sio_t* sio;
	qse_sio_pos_t pos;

	sio = qse_sio_open (QSE_MMGR_GETDFL(), 0, QSE_T("sio03.txt"), 
		QSE_SIO_WRITE | QSE_SIO_READ | QSE_SIO_TRUNCATE | QSE_SIO_CREATE);
	if (sio == QSE_NULL) return -1;

	qse_sio_putstr (sio, QSE_T("이거 좋다. this is good"));
	qse_sio_getpos (sio, &pos);
	qse_sio_putstrf (g_out, QSE_T("position = %lld\n"), (long long int)pos);

	qse_sio_setpos (sio, pos - 2);
	qse_sio_putstrf (sio, QSE_T("wonderful"));

	qse_sio_getpos (sio, &pos);
	qse_sio_truncate (sio, pos - 2);

	pos = 0;
	qse_sio_seek (sio, &pos, QSE_SIO_BEGIN);
	qse_sio_putstr (sio, QSE_T("오홍"));

	qse_sio_putstrf (g_out, QSE_T("position returned by seek = %lld\n"), (long long int)pos);
	qse_sio_getpos (sio, &pos);
	qse_sio_putstrf (g_out, QSE_T("position returned by getpos = %lld\n"), (long long int)pos);

	qse_sio_close (sio);
	return 0;
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
		qse_setdflcmgrbyid (QSE_CMGR_SLMB);
	}
#else
	setlocale (LC_ALL, "");
	qse_setdflcmgrbyid (QSE_CMGR_SLMB);
#endif

	g_out = qse_sio_openstd (QSE_MMGR_GETDFL(), 0, QSE_SIO_STDOUT, QSE_SIO_WRITE | QSE_SIO_IGNOREMBWCERR);

	qse_sio_putstr (g_out, QSE_T("--------------------------------------------------------------------------------\n"));
	qse_sio_putstr (g_out, QSE_T("Set the environment LANG to a Unicode locale such as UTF-8 if you see the illegal XXXXX errors. If you see such errors in Unicode locales, this program might be buggy. It is normal to see such messages in non-Unicode locales as it uses Unicode data\n"));
	qse_sio_putstr (g_out, QSE_T("--------------------------------------------------------------------------------\n"));

	R (test1);
	R (test2);
	R (test3);
	R (test4);

	qse_sio_close (g_out);

	return 0;
}
