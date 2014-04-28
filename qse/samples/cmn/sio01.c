#include <qse/cmn/sio.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/fmt.h>


#include <locale.h>
#if defined(_WIN32)
#	include <windows.h>
#endif

static qse_sio_t* g_out;

#define R(f) \
	do { \
		qse_sio_putstr (g_out,QSE_T("== ")); \
		qse_sio_putstr (g_out,QSE_T(#f)); \
		qse_sio_putstr (g_out,QSE_T(" ==\n")); \
		qse_sio_flush (g_out); \
		if (f() == -1) return -1; \
	} while (0)

static int test1 (void)
{
     const qse_wchar_t unistr[] =
     {
		0x00A2,

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

          L'\n',
          L'\0'
     };

	qse_sio_t* out;
	
	out = qse_sio_openstd (
		QSE_MMGR_GETDFL(), 0, QSE_SIO_STDOUT, 
		QSE_SIO_WRITE | QSE_SIO_IGNOREMBWCERR);
	qse_sio_putwcs (out, unistr);
	qse_sio_putwcsf (out, QSE_WT ("[%.*s] [%.*s]\n"), (int)qse_wcslen(unistr) - 1, unistr, (int)qse_wcslen(unistr) - 1, unistr);
	qse_sio_close (out);
	return 0;
}

static int test2 (void)
{
     const qse_wchar_t unistr[] =
     {
		0x00A2,

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

          L'\n',
          L'\0'
     };

	qse_mchar_t mbsbuf[100];
	qse_sio_t* out;
	qse_size_t wlen, mlen;
	
	out = qse_sio_openstd (
		QSE_MMGR_GETDFL(), 0, QSE_SIO_STDOUT, 
		QSE_SIO_WRITE | QSE_SIO_IGNOREMBWCERR);

	mlen = QSE_COUNTOF(mbsbuf);
	qse_wcstombs (unistr, &wlen, mbsbuf, &mlen);

	qse_sio_putmbs (out, mbsbuf);

	qse_sio_putmbsf (out, QSE_MT ("[%.*s] [%.*s]\n"), (int)qse_mbslen(mbsbuf) - 1, mbsbuf, (int)qse_mbslen(mbsbuf) - 1, mbsbuf);
	qse_sio_close (out);
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
	R (test1);
	R (test2);
	qse_sio_close (g_out);

	return 0;
}
