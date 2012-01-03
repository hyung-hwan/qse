#include <qse/cmn/slmb.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/cmn/stdio.h>

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
	int i;
	const qse_mchar_t* x[] =
	{
		"\0",
		"뛰어 올라봐", /* this text is in utf8. so some conversions fail on a non-utf8 locale */
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
			qse_size_t y = qse_slmblen (&x[i][j], k-j);

			if (y == 0) 
			{
				qse_printf (QSE_T("***illegal sequence***"));
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

static int test2 (void)
{
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
		L"\0",
		L"",
		L"Fly to the universe"
	};
	char buf[100];
	int i, j;

	x[1] = unistr;

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
		qse_setdflcmgr (qse_utf8cmgr);
	}
	else
	{
     	sprintf (locale, ".%u", (unsigned int)codepage);
     	setlocale (LC_ALL, locale);
		qse_setdflcmgr (qse_slmbcmgr);
	}
#else
     setlocale (LC_ALL, "");
	qse_setdflcmgr (qse_slmbcmgr);
#endif

	R (test1);
	R (test2);

	return 0;
}
