#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/cmn/sio.h>
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
	const qse_wchar_t unistr[] = 
	{
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
	const qse_wchar_t* x[] =
	{
		L"",
		L"",
		L"Fly to the universe, kick your ass"
	};
	qse_mchar_t buf[10];
	qse_mchar_t buf2[100];
	int i;

	x[1] = unistr;

	for (i = 0; i < QSE_COUNTOF(x); i++)
	{
		int n;
		qse_size_t wlen, mlen;

		n = qse_wcstombs (x[i], &wlen, QSE_NULL, &mlen);

		qse_printf (QSE_T("[%ls] n=%d, wcslen()=%d, wlen=%d, mlen=%d\n"), 
			x[i], (int)n, (int)qse_wcslen(x[i]), (int)wlen, (int)mlen);
	}
	qse_printf (QSE_T("-----------------\n"));

	for (i = 0; i < QSE_COUNTOF(x); i++)
	{
		int n;
		qse_size_t wlen, mlen;

		mlen = QSE_COUNTOF(buf) - 1;

		qse_mbsset (buf, QSE_MT('A'), QSE_COUNTOF(buf));
		n = qse_wcstombs (x[i], &wlen, buf, &mlen);

		QSE_ASSERT (buf[QSE_COUNTOF(buf)-1] == QSE_MT('A'));
		buf[QSE_COUNTOF(buf)-1] = QSE_MT('\0');

		qse_printf (QSE_T("%s chars=%d bytes=%d [%hs]\n"), 
			((n == -2)? QSE_T("BUFFER-SMALL"): (n == -1)? QSE_T("ILLEGAL-CHAR"): QSE_T("FULL")),
			(int)wlen, (int)mlen, buf);
	}

	qse_printf (QSE_T("-----------------\n"));
	for (i = 0; i < QSE_COUNTOF(x); i++)
	{
		int n;
		qse_size_t wlen, mlen;

		mlen = QSE_COUNTOF(buf2);
		n = qse_wcstombs (x[i], &wlen, buf2, &mlen);

		qse_printf (QSE_T("%s chars=%d bytes=%d [%hs]\n"), 
			((n == -2)? QSE_T("BUFFER-SMALL"): (n == -1)? QSE_T("ILLEGAL-CHAR"): QSE_T("FULL")),
			(int)wlen, (int)mlen, buf2);
	}

	qse_printf (QSE_T("-----------------\n"));
	for (i = 0; i < QSE_COUNTOF(x); i++)
	{
		int n;
		const qse_wchar_t* p = x[i];

		while (*p)
		{
			qse_size_t wlen, mlen;

			mlen = QSE_COUNTOF(buf) - 1;
			n = qse_wcstombs (p, &wlen, buf, &mlen);

			qse_printf (QSE_T("%s chars=%d bytes=%d [%hs]\n"), 
				((n == -2)? QSE_T("BUFFER-SMALL"): (n == -1)? QSE_T("ILLEGAL-CHAR"): QSE_T("FULL")),
				(int)wlen, (int)mlen, buf);
			p += wlen;
		}
	}

	return 0;
}

static int test2 (void)
{
	const qse_wchar_t unistr[] = 
	{
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
	const qse_wchar_t* x[] =
	{
		L"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
		L"",
		L"Fly to the universe, kick your ass"
	};
	qse_mchar_t buf[9];
	int i, j;

	x[1] = unistr;

	for (i = 0; i < QSE_COUNTOF(x); i++)
	{
		qse_size_t len, wlen, mlen, iter;
		int n;

		len = qse_wcslen(x[i]);
		if (len == 0) len = 20; /* for x[0] */

		qse_printf (QSE_T("Converting %d wide-character - "), (int)len);

		wlen = len;
		n = qse_wcsntombsn (&x[i][j], &wlen, QSE_NULL, &mlen);
		if (n == -1)
		{
			qse_printf (QSE_T("***illegal character[mlen=%d/wlen=%d]*** ["), (int)mlen, (int)wlen);
		}
		else
		{
			/* -2 is never returned if i don't provide the multibyte buffer */
			qse_printf (QSE_T("%d multibyte characters to be produced from %d wide characters ["), (int)mlen, (int)wlen);
		}

		for (j = 0, iter = 0; j < len; j += wlen)
		{
			iter = iter + 1;
			wlen = len - j;
			mlen = QSE_COUNTOF(buf);
			n = qse_wcsntombsn (&x[i][j], &wlen, buf, &mlen);

			if (n == -1)
			{
				qse_printf (QSE_T("***illegal character*** wlen=%d/mlen=%d/n=%d"), (int)(len-j), (int)mlen, (int)n);
				break;
			}
			#if 0
			else if (n == -2)
			{
				qse_printf (QSE_T("***buffer not large*** wlen=%d/mlen=%d/n=%d"), (int)(len-j), (int)mlen, (int)n);
				break;
			}
			#endif

			if (mlen > 0 && buf[0] == QSE_T('\0'))
			{
				while (mlen > 0 && buf[--mlen] == QSE_T('\0'))
				{
					qse_printf (QSE_T("\\0"));
				}
			}
			else
			{
			#if defined(QSE_CHAR_IS_MCHAR) || defined(_WIN32)
				qse_printf (QSE_T("%.*hs"), (int)mlen, buf);
			#else
				/* at least on linux and macosx, wprintf seems 
				 * to taks preceision as the number of wide 
				 * characters with %s. */
				/* THIS HACK SHOULD BE REMOVED ONCE I IMPLEMENTE
				 * MY OWN qse_printf */
				qse_printf (QSE_T("%.*hs"), (int)wlen, buf);
			#endif
			}
		}

		qse_printf (QSE_T("] => %d iterations\n"), (int)iter);
	}
	return 0;
}

static int test3 (void)
{
	const qse_wchar_t unistr[] = 
	{
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
	const qse_wchar_t* x[] =
	{
		L"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
		L"",
		L"Fly to the universe, kick your ass"
	};
	int i;

	x[1] = unistr;

	for (i = 0; i < QSE_COUNTOF(x); i++)
	{
		qse_mchar_t* m = qse_wcstombsdup (x[i], QSE_MMGR_GETDFL());
		if (m == QSE_NULL)
		{
			qse_printf (QSE_T("[ERROR]\n"), m);
		}
		else
		{
			qse_printf (QSE_T("[%hs]\n"), m);
		}	

		QSE_MMGR_FREE (QSE_MMGR_GETDFL(), m);
	}
	return 0;
}

static int test4 (void)
{
	const qse_wchar_t unistr[] = 
	{
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
	const qse_wchar_t* x[] =
	{
		L"Good morning angels",
		L"",
		L"Fly to the universe, kick your ass",
		QSE_NULL
	};
	qse_mchar_t* m;

	x[1] = unistr;

	m = qse_wcsatombsdup (x, QSE_MMGR_GETDFL());
	if (m == QSE_NULL)
	{
		qse_printf (QSE_T("[ERROR]\n"), m);
	}
	else
	{
		qse_printf (QSE_T("[%hs]\n"), m);
	}	

	QSE_MMGR_FREE (QSE_MMGR_GETDFL(), m);
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
	R (test2);
	R (test3);
	R (test4);

#if defined(_WIN32)
	SetConsoleOutputCP (old_cp);
#endif
	return 0;
}
