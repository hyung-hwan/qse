#include <qse/cmn/slmb.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/cmn/stdio.h>

#include <locale.h>
#include <wchar.h>

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
	qse_printf (QSE_T("slmblenmax=%d\n"), (int)qse_slmblenmax());
	return 0;
}

static int test2 (void)
{
	int i;
	const qse_mchar_t* x[] =
	{
		"\0",
		"뛰어 올라봐", /* this text is in utf8. so some conversions fail on a non-utf8 locale */
#if defined(QSE_ENDIAN_BIG)
		/* this text is in cp949. 뛰어올라봐 */
		"\xD9\xB6\xEE\xBE \xC3\xBF\xF3\xB6\xC1\xBA", 
#elif defined(QSE_ENDIAN_LITTLE)
		/* this text is in cp949. 뛰어올라봐 */
		"\xB6\xD9\xBE\xEE \xBF\xC3\xB6\xF3\xBA\xC1", 
#else
#	error ENDIAN UNKNOWN
#endif
		"Fly to the universe"
	};

	for (i = 0; i < QSE_COUNTOF(x); i++)
	{
		qse_size_t k = qse_mbslen(x[i]);
		qse_size_t j = 0;
		qse_wchar_t wc;

		if (k == 0) k++; /* for x[0] */

		qse_printf (QSE_T("["));
		while (j < k)
		{
			mbstate_t state;
			qse_size_t y, ym, ymr;

			y = qse_slmblen (&x[i][j], k-j);
			ym = mblen (&x[i][j], k-j);

			memset (&state, 0, sizeof(state));
			ymr = mbrlen (&x[i][j], k-j, &state);

			if (ym != ymr) 
			{
				/* if this line is shown, the c runtime library is buggy.
				 * note that i assume we handle stateless encoding only
				 * since the state is initlized to 0 above all the time */
				qse_printf (QSE_T("***buggy clib [mblen=%d],[mbrlen=%d]***"), (int)ym, (int)ymr); 
			}

			if (y == 0) 
			{
				qse_printf (QSE_T("***illegal sequence[y=%d][ym=%d][ymr=%d]***"), (int)y, (int)ym, (int)ymr);
				break;
			}
			else if (y > k-j) 
			{
				qse_printf (QSE_T("***incomplete sequence***"));
				break;
			}
			else
			{
				qse_size_t y2 = qse_slmbtoslwc (&x[i][j], y, &wc);
				if (y2 != y)
				{
					qse_printf (QSE_T("***y(%d) != y2(%d). something is wrong*** "), (int)y, (int)y2);
					break;
				}

				if (wc == L'\0')
				{
					qse_printf (QSE_T("'\\0'"));
				}
				else
				{
				#ifdef QSE_CHAR_IS_MCHAR
					qse_printf (QSE_T("%C"), wc);
				#else
					qse_printf (QSE_T("%c"), wc);
				#endif
				}
				j += y;
			}
		}

		qse_printf (QSE_T("] => %d bytes\n"), (int)k);
	}

	return 0;
}

static int test3 (void)
{
	const qse_wchar_t unistr_kr[] =
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

	const qse_wchar_t unistr_cn[] =
	{
		/* 智慧手機帶頭 */
		/* \u667A\u6167\u624B\u6A5F\u5E36\u982D */
		0x667A,
		0x6167,
		0x624B,
		0x6A5F,
		0x5E36,
		0x982D,
		L'\0'
	}; 

	const qse_wchar_t* x[] =
	{
		L"\0",
		L"",
		L"",
		L"Fly to the universe"
	};
	char buf[100];
	int i, j;

	x[1] = unistr_kr;
	x[2] = unistr_cn;

	for (i = 0; i < QSE_COUNTOF(x); i++)
	{
		int nbytes = 0;
		int len = qse_wcslen (x[i]);
		if (len == 0) len++; /* for x[0] */

		qse_printf (QSE_T("["));
		for (j = 0; j < len; j++)
		{
			qse_size_t n = qse_slwctoslmb (x[i][j], buf, sizeof(buf) - 1);

			if (n == 0)
			{
				qse_printf (QSE_T("***illegal character***"));
			}
			else if (n > sizeof(buf))
			{
				qse_printf (QSE_T("***buffer too small***"));
			}
			else
			{
				if (n == 1 && buf[0] == '\0')
				{
					qse_printf (QSE_T("'\\0'"));
				}
				else
				{
					buf[n] = QSE_MT('\0');
					qse_printf (QSE_T("%hs"), buf);
				}
				nbytes += n;
			}
		}
		qse_printf (QSE_T("] => %d chars, %d bytes\n"), (int)len, (int)nbytes);
	}
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

#if 0
	{
		WORD LangID = MAKELANGID(LANG_CHINESE, SUBLANG_DEFAULT);
		SetThreadLocale(MAKELCID(LangID, SORT_DEFAULT));
		/* SetThreadUILanguage(), SetThreadPreferredUILanguage(). */
	}
#endif

#else
     setlocale (LC_ALL, "");
	qse_setdflcmgrbyid (QSE_CMGR_SLMB);
#endif

	R (test1);
	R (test2);
	R (test3);

	return 0;
}
