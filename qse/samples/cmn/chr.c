#include <qse/cmn/mem.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/str.h>
#include <qse/cmn/stdio.h>

#include <locale.h>

#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) return -1; \
	} while (0)

static int test1 (void)
{
	qse_char_t c;

	for (c = QSE_T('a'); c <= QSE_T('z'); c++)
	{
		qse_printf (QSE_T("%c => %c\n"), c, QSE_TOUPPER(c));
	}

	return 0;
}

static int test2 (void)
{
	int i;
	const qse_mchar_t* x[] =
	{
		"\0",
		"뛰어 올라봐",
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
			qse_size_t y = qse_mblen (&x[i][j], k-j);

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
				qse_size_t y2 = qse_mbtowc (&x[i][j], y, &wc);
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
	const qse_wchar_t* x[] =
	{
		L"\0",
		L"\uB108 \uBB50\uAC00 \uC798\uB0AC\uC5B4?",
		L"Fly to the universe"
	};
	char buf[100];
	int i, j;

	for (i = 0; i < QSE_COUNTOF(x); i++)
	{
		int len = qse_wcslen (x[i]);
		if (len == 0) len++; /* for x[0] */

		qse_printf (QSE_T("["));
		for (j = 0; j < len; j++)
		{
			qse_size_t n = qse_wctomb (x[i][j], buf, sizeof(buf));

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
				#ifdef QSE_CHAR_IS_MCHAR
					qse_printf (QSE_T("%.*s"), (int)n, buf);
				#else
					qse_printf (QSE_T("%.*S"), (int)n, buf);
				#endif
				}
			}
		}
		qse_printf (QSE_T("] => %d chars\n"), (int)len);
	}
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

	return 0;
}
