#include <qse/cmn/stdio.h>
#include <qse/cmn/sio.h>

#include <locale.h>

#if defined(_WIN32)
#	include <windows.h>
#endif

#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) return -1; \
	} while (0)

static int test1 (void)
{
	const qse_wchar_t unistr[] = 
	{
		/* ugly hack for old compilers that don't support \u */
		/*L"\uB108 \uBB50\uAC00 \uC798\uB0AC\uC5B4?",*/
		0xB108,
		L' ',
		0xBB50,
		0xAC00,
		L' ',
		0xC798,
		0xB0AC,
		0xC5B4,
		L'?',
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

	sio = qse_sio_openstd (QSE_NULL, 0, QSE_SIO_STDOUT, QSE_SIO_READ | QSE_SIO_IGNOREMBWCERR | QSE_SIO_NOAUTOFLUSH);
	if (sio == QSE_NULL) return -1;

	for (i = 0; i < QSE_COUNTOF(x); i++)
	{
		qse_sio_putws (sio, x[i]);
		qse_sio_putws (sio, QSE_WT("\n"));
	}

	qse_sio_close (sio);
	return 0;
}

int main ()
{
#if defined(_WIN32)
	char codepage[100];
	UINT old_cp = GetConsoleOutputCP();
	SetConsoleOutputCP (CP_UTF8);

	/* TODO: on windows this set locale only affects those mbcs fucntions in clib.
	 * it doesn't support utf8 i guess find a working way. the following won't work 
	sprintf (codepage, ".%d", GetACP());
	setlocale (LC_ALL, codepage);
	*/
#else
	setlocale (LC_ALL, "");
#endif

	qse_printf (QSE_T("--------------------------------------------------------------------------------\n"));
	qse_printf (QSE_T("Set the environment LANG to a Unicode locale such as UTF-8 if you see the illegal XXXXX errors. If you see such errors in Unicode locales, this program might be buggy. It is normal to see such messages in non-Unicode locales as it uses Unicode data\n"));
	qse_printf (QSE_T("--------------------------------------------------------------------------------\n"));

	R (test1);

#if defined(_WIN32)
	SetConsoleOutputCP (old_cp);
#endif
	return 0;
}
