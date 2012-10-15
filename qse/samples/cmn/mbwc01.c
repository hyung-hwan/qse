#include <qse/cmn/mbwc.h>
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


/* the texts here are all in utf-8. you will see many errors 
 * on a non-utf8 locale */
static int test1 (void)
{
	int i;
	const qse_mchar_t* x[] =
	{
		QSE_MT(""),
		QSE_MT("This is good."),
		QSE_MT("이거슨"),
		QSE_MT("뭐냐이거"),
		QSE_MT("過去一個月"),
		QSE_MT("是成功的建商"),
		QSE_MT("뛰어 올라봐. 멀리멀리 잘난척하기는"),
		QSE_MT("Fly to the universe. eat my shorts.")
	};
	qse_wchar_t buf[5];
	qse_wchar_t buf2[50];

	for (i = 0; i < QSE_COUNTOF(x); i++)
	{
		int n;
		qse_size_t wlen = QSE_COUNTOF(buf), wlen1;
		qse_size_t mlen, mlen1;


		n = qse_mbstowcs (x[i], &mlen, buf, &wlen);

		qse_printf (QSE_T("[%hs]=>"), x[i]);
		qse_mbstowcs (x[i], &mlen1, QSE_NULL, &wlen1);

		/* the string should not be properly null terminated 
		 * if buf is shorter than the necesary buffer size needed for 
		 * full conversion */
		qse_wcsxncpy (buf2, QSE_COUNTOF(buf2), buf, wlen); 

		qse_printf (QSE_T("%s %d chars %d bytes (IF FULL %d chars %d bytes) [%ls]\n"), 
			((n == -3)? QSE_T("INCOMPLETE-SEQ"): (n == -2)? QSE_T("BUFFER-SMALL"): (n == -1)? QSE_T("ILLEGAL-CHAR"): QSE_T("FULL")),
			(int)wlen, (int)mlen, (int)wlen1, (int)mlen1, buf2);
		qse_fflush (QSE_STDOUT);
	}

	qse_printf (QSE_T("-----------------\n"));

	for (i = 0; i < QSE_COUNTOF(x); i++)
	{
		int n;
		qse_size_t wlen = QSE_COUNTOF(buf2), wlen1;
		qse_size_t mlen, mlen1;

		n = qse_mbstowcs (x[i], &mlen, buf2, &wlen);
		qse_mbstowcs (x[i], &mlen1, QSE_NULL, &wlen1);

		qse_printf (QSE_T("%s %d chars %d bytes (IF FULL %d chars %d bytes) [%.*ls]\n"), 
			((n == -3)? QSE_T("INCOMPLETE-SEQ"): (n == -2)? QSE_T("BUFFER-SMALL"): (n == -1)? QSE_T("ILLEGAL-CHAR"): QSE_T("FULL")),
			(int)wlen, (int)mlen, (int)wlen, (int)wlen1, (int)mlen1, buf2);

		qse_fflush (QSE_STDOUT);
	}

	return 0;
}

static int test2 (void)
{
	int i;
	const qse_mchar_t* x[] =
	{
		QSE_MT("\0\0\0"),
		QSE_MT("This is good."),
		QSE_MT("이거슨"),
		QSE_MT("뭐냐이거"),
		QSE_MT("過去一個月"),
		QSE_MT("是成功的建商"),
		QSE_MT("뛰어 올라봐. 멀리멀리 잘난척하기는"),
		QSE_MT("Fly to the universe")
	};

	for (i = 0; i < QSE_COUNTOF(x); i++)
	{
		qse_size_t k = qse_mbslen(x[i]);
		qse_size_t j = 0;
		qse_wchar_t buf[2];
		qse_size_t wlen, mlen;
		int iter = 0;

		if (k == 0) k = 3; /* for x[0] */

		mlen = k;
		qse_mbsntowcsn (&x[i][j], &mlen, QSE_NULL, &wlen);
		qse_printf (QSE_T("expecting %d chars, %d bytes ["), wlen, mlen);

		while (j < k)
		{
			int y;

			wlen = QSE_COUNTOF(buf);
			mlen = k - j;

			y = qse_mbsntowcsn (&x[i][j], &mlen, buf, &wlen);


			if (y <= -1 && y != -2)
			{
				qse_printf (QSE_T("***illegal or incomplete sequence***"));
				break;
			}


			if (wlen > 0 && buf[0] == QSE_T('\0'))
			{
				while (wlen > 0 && buf[--wlen] == QSE_T('\0'))
				{
					qse_printf (QSE_T("\\0"));
				}
			}
			else
			{
				qse_printf (QSE_T("%.*s"), (int)wlen, buf);
			}

			j += mlen;
			iter++;
		}

		qse_printf (QSE_T("] => %d bytes, %d iterations\n"), (int)k, (int)iter);
	}

	return 0;
}

static int test3 (void)
{
	int i;
	const qse_mchar_t* x[] =
	{
		QSE_MT(""),
		QSE_MT("This is good."),
		QSE_MT("이거슨"),
		QSE_MT("뭐냐이거"),
		QSE_MT("過去一個月"),
		QSE_MT("是成功的建商"),
		QSE_MT("뛰어 올라봐. 멀리멀리 잘난척하기는"),
		QSE_MT("Fly to the universe")
	};

	for (i = 0; i < QSE_COUNTOF(x); i++)
	{
		qse_wchar_t* wcs;

		wcs = qse_mbstowcsdup (x[i], QSE_MMGR_GETDFL());
		if (wcs == QSE_NULL)
		{
			qse_printf (QSE_T("[ERROR]"));
		}
		else
		{
			qse_printf (QSE_T("[%ls]\n"), wcs);
			QSE_MMGR_FREE (QSE_MMGR_GETDFL(), wcs);
		}
	}

	return 0;
}

static int test4 (void)
{
	const qse_mchar_t* x[] =
	{
		QSE_MT("\0\0\0"),
		QSE_MT("This is good."),
		QSE_MT("이거슨"),
		QSE_MT("뭐냐이거"),
		QSE_MT("過去一個月"),
		QSE_MT("是成功的建商"),
		QSE_MT("뛰어 올라봐. 멀리멀리 잘난척하기는"),
		QSE_MT("Fly to the universe"),
		QSE_NULL
	};
	qse_wchar_t* wcs;

	wcs = qse_mbsatowcsdup (x, QSE_MMGR_GETDFL());
	if (wcs == QSE_NULL)
	{
		qse_printf (QSE_T("[ERROR]\n"));
	}
	else
	{
		qse_printf (QSE_T("[%ls]\n"), wcs);
		QSE_MMGR_FREE (QSE_MMGR_GETDFL(), wcs);
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
#else
	setlocale (LC_ALL, "");
	qse_setdflcmgrbyid (QSE_CMGR_SLMB);
#endif

	qse_printf (QSE_T("--------------------------------------------------------------------------------\n"));
	qse_printf (QSE_T("Set the environment LANG to a Unicode locale such as UTF-8 if you see the illegal XXXXX errors. If you see such errors in Unicode locales, this program might be buggy. It is normal to see such messages in non-Unicode locales as it uses Unicode data\n"));
	qse_printf (QSE_T("--------------------------------------------------------------------------------\n"));

	R (test1);
	R (test2);
	R (test3);
	R (test4);

	return 0;
}
