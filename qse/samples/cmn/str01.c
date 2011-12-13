#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
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

static int test1 ()
{
	qse_str_t* s1;

	s1 = qse_str_open (QSE_MMGR_GETDFL(), 0, 5);	
	if (s1 == QSE_NULL)
	{
		qse_printf (QSE_T("cannot open a string\n"));
		return -1;
	}

	qse_printf (QSE_T("LEN=%u\n"), 
		(unsigned)qse_str_ncat (s1, QSE_T("i love you this is great"), 24));
	qse_printf (QSE_T("LEN=%u CAPA=%u [%.*s]\n"), 
		(unsigned)QSE_STR_LEN(s1), (unsigned)QSE_STR_CAPA(s1), 
		(int)QSE_STR_LEN(s1), QSE_STR_PTR(s1));

	qse_printf (QSE_T("LEN=%u\n"),
		(unsigned int)qse_str_setlen (s1, QSE_STR_LEN(s1)-1));
	qse_printf (QSE_T("LEN=%u CAPA=%u [%.*s]\n"), 
		(unsigned)QSE_STR_LEN(s1), (unsigned)QSE_STR_CAPA(s1), 
		(int)QSE_STR_LEN(s1), QSE_STR_PTR(s1));

	qse_printf (QSE_T("LEN=%u\n"),
		(unsigned int)qse_str_setlen (s1, QSE_STR_CAPA(s1) + 5));
	qse_printf (QSE_T("LEN=%u CAPA=%u [%.*s]\n"), 
		(unsigned)QSE_STR_LEN(s1), (unsigned)QSE_STR_CAPA(s1), 
		(int)QSE_STR_LEN(s1), QSE_STR_PTR(s1));

	qse_str_close (s1);
	return 0;
}

static int test2 ()
{
	qse_str_t s1;

	qse_str_init (&s1, QSE_MMGR_GETDFL(), 5);

	qse_printf (QSE_T("LEN=%u\n"), 
		(unsigned)qse_str_ncat (&s1, QSE_T("i love you this is great"), 24));
	qse_printf (QSE_T("LEN=%u CAPA=%u [%.*s]\n"), 
		(unsigned)QSE_STR_LEN(&s1), (unsigned)QSE_STR_CAPA(&s1), 
		(int)QSE_STR_LEN(&s1), QSE_STR_PTR(&s1));

	qse_str_fini (&s1);
	return 0;
}

static qse_size_t resize_str_1 (qse_str_t* str, qse_size_t hint)
{
	return QSE_STR_CAPA(str) + 1;
}

static qse_size_t resize_str_2 (qse_str_t* str, qse_size_t hint)
{
	return hint;
}

static qse_size_t resize_str_3 (qse_str_t* str, qse_size_t hint)
{
	return hint * 2 + hint / 2;
}

static qse_size_t resize_str_4 (qse_str_t* str, qse_size_t hint)
{
	return 0;
}

static int test3 ()
{
	qse_str_t s1;
	int i;

	qse_str_init (&s1, QSE_MMGR_GETDFL(), 5);

	for (i = 0; i < 9; i++)
	{
		if (i == 0) qse_str_setsizer (&s1, resize_str_1);
		if (i == 1) 
		{	
			qse_str_setsizer (&s1, resize_str_2);
			qse_str_clear (&s1);
		}
		if (i == 2) qse_str_setsizer (&s1, resize_str_3);
		if (i == 3) qse_str_setsizer (&s1, resize_str_4);
		if (i == 8) qse_str_setsizer (&s1, resize_str_2);

		if (qse_str_ncat (&s1, QSE_T("i love you this is great"), 24) == (qse_size_t)-1)
		{
			qse_printf (QSE_T("cannot add string\n"));
			qse_str_fini (&s1);
			return -1;
		}

		qse_printf (QSE_T("LEN=%u CAPA=%u [%.*s]\n"), 
			(unsigned)QSE_STR_LEN(&s1), (unsigned)QSE_STR_CAPA(&s1), 
			(int)QSE_STR_LEN(&s1), QSE_STR_PTR(&s1));
	}

	qse_str_fini (&s1);
	return 0;
}

static int test4 ()
{
	qse_str_t s1;
	qse_xstr_t out;

	qse_str_init (&s1, QSE_MMGR_GETDFL(), 0);

	if (qse_str_yield (&s1, &out, 0) == -1)
	{
		qse_printf (QSE_T("cannot yield string\n"));
		qse_str_fini (&s1);
		return -1;
	}

	qse_printf (QSE_T("out.ptr=%p LEN=%u [.*s]\n"), 
		out.ptr, (unsigned)out.len, (int)out.len, out.ptr);

	if (qse_str_ncat (&s1, QSE_T("i love you this is great"), 24) == (qse_size_t)-1)
	{
		qse_printf (QSE_T("cannot add string\n"));
		qse_str_fini (&s1);
		return -1;
	}

	if (qse_str_yield (&s1, &out, 0) == -1)
	{
		qse_printf (QSE_T("cannot yield string\n"));
		qse_str_fini (&s1);
		return -1;
	}

	qse_printf (QSE_T("out.ptr=%p LEN=%u, [%.*s]\n"), 
		out.ptr, (unsigned)out.len, (int)out.len, out.ptr);

	qse_str_fini (&s1);

	QSE_MMGR_FREE (QSE_MMGR_GETDFL(), out.ptr);
	return 0;
}

static int test11 (void)
{
	qse_char_t buf[1000];
	const qse_char_t* arg3[] = 
	{
		QSE_T("00000"),
		QSE_T("11111"), 
		QSE_T("22222")
	};

	const qse_char_t* arg12[] = 
	{
		QSE_T("00000"), QSE_T("11111"), QSE_T("22222"),
		QSE_T("33333"), QSE_T("44444"), QSE_T("55555"),
		QSE_T("66666"), QSE_T("77777"), QSE_T("88888"),
		QSE_T("99999"), QSE_T("aaaaa"), QSE_T("bbbbb")
	};

	qse_strfcpy (buf, QSE_T("${2}${1}${0}"), arg3);
	qse_printf (QSE_T("buf=[%s]\n"), buf);

	qse_strfcpy (buf, QSE_T("${2}/${1}/${0}"), arg3);
	qse_printf (QSE_T("buf=[%s]\n"), buf);

	qse_strfcpy (buf, QSE_T("/${2}/${1}/${0}/"), arg3);
	qse_printf (QSE_T("buf=[%s]\n"), buf);

	qse_strfcpy (buf, QSE_T("/$${2}/$${1}/$${0}/"), arg3);
	qse_printf (QSE_T("buf=[%s]\n"), buf);

	qse_strfcpy (buf, QSE_T("/${2/${1}/${0}/"), arg3);
	qse_printf (QSE_T("buf=[%s]\n"), buf);

	qse_strfcpy (buf, QSE_T("/$2}/${1}/${0}/"), arg3);
	qse_printf (QSE_T("buf=[%s]\n"), buf);

	qse_strfcpy (buf, QSE_T("/${2}/${1}/${0}/${3}/${4}/${5}/${6}/${7}/${8}/${9}/${10}/${11}/"), arg12);
	qse_printf (QSE_T("buf=[%s]\n"), buf);

	qse_strfcpy (buf, QSE_T("/${2}/${1}/${0}/${0}/${1}/${2}/"), arg3);
	qse_printf (QSE_T("buf=[%s]\n"), buf);

	qse_strfcpy (buf, QSE_T("/${002}/${001}/${000}/"), arg3);
	qse_printf (QSE_T("buf=[%s]\n"), buf);

	return 0;
}

static int test12 (void)
{
	qse_char_t buf[20];
	int i, j;

	const qse_char_t* arg3[] = 
	{
		QSE_T("00000"),
		QSE_T("11111"), 
		QSE_T("22222")
	};

	for (i = 0; i <= QSE_COUNTOF(buf); i++)
	{
		qse_strcpy (buf, QSE_T("AAAAAAAAAAAAAAAAAAA"));	
		qse_strxfcpy (buf, i, QSE_T("${2}${1}${0}"), arg3);
		qse_printf (QSE_T("bufsize=%02d, buf=[%-20s] "), i, buf);

		qse_printf (QSE_T("["));
		for (j = 0; j < QSE_COUNTOF(buf); j++)
		{
			if (buf[j] == QSE_T('\0')) qse_printf (QSE_T("*"));
			else qse_printf (QSE_T("%c"), buf[j]);
		}
		qse_printf (QSE_T("]\n"));
	}
	return 0;
}

qse_char_t* subst (qse_char_t* buf, qse_size_t bsz, const qse_cstr_t* ident, void* ctx)
{
	if (qse_strxcmp (ident->ptr, ident->len, QSE_T("USER")) == 0)
	{
		return buf + qse_strxput (buf, bsz, QSE_T("sam"));	
	}
	else if (qse_strxcmp (ident->ptr, ident->len, QSE_T("GROUP")) == 0)
	{
		return buf + qse_strxput (buf, bsz, QSE_T("coders"));	
	}

	/* don't do anything */
	return buf;
}

static int test13 (void)
{
	qse_char_t buf[24];
	qse_size_t i, j;

	for (i = 0; i <= QSE_COUNTOF(buf); i++)
	{
		qse_strcpy (buf, QSE_T("AAAAAAAAAAAAAAAAAAAAAAA"));
		qse_strxsubst (buf, i, QSE_T("user=${USER},group=${GROUP}"), subst, QSE_NULL);
		qse_printf (QSE_T("bufsize=%02d, buf=[%-25s] "), i, buf);

		qse_printf (QSE_T("["));
		for (j = 0; j < QSE_COUNTOF(buf); j++)
		{
			if (buf[j] == QSE_T('\0')) qse_printf (QSE_T("*"));
			else qse_printf (QSE_T("%c"), buf[j]);
		}
		qse_printf (QSE_T("]\n"));
	}
	return 0;
}

static int test14 (void)
{
	qse_char_t a1[] = QSE_T("   this is a test string    ");
	qse_char_t a2[] = QSE_T("   this is a test string    ");
	qse_char_t a3[] = QSE_T("   this is a test string    ");

	qse_printf (QSE_T("[%s] =>"), a1);
	qse_printf (QSE_T("[%s]\n"), qse_strtrmx (a1, QSE_STRTRMX_LEFT));

	qse_printf (QSE_T("[%s] =>"), a2);
	qse_printf (QSE_T("[%s]\n"), qse_strtrmx (a2, QSE_STRTRMX_RIGHT));

	qse_printf (QSE_T("[%s] =>"), a3);
	qse_printf (QSE_T("[%s]\n"), 
		qse_strtrmx (a3, QSE_STRTRMX_LEFT|QSE_STRTRMX_RIGHT));

	return 0;
}

static int test15 (void)
{
	qse_char_t a1[] = QSE_T("abcdefghijklmnopqrstuvwxyz");
	qse_str_t x;
	int i;

	qse_str_init (&x, QSE_MMGR_GETDFL(), 5);

	for (i = 1; i < 20; i++)
	{
		qse_str_cpy (&x, a1);
		qse_str_del (&x, 10, i);
		qse_printf (QSE_T("deleleted %d from 10 => %lu [%s]\n"), 
			i,
			(unsigned long)QSE_STR_LEN(&x),
			QSE_STR_PTR(&x));
	}

	qse_str_fini (&x);
	return 0;
}

static int test16 (void)
{
	const qse_char_t* x = QSE_T("this is good");

	qse_printf (QSE_T("[%s]\n"), qse_strpbrk (x, QSE_T("si")));
	qse_printf (QSE_T("[%s]\n"), qse_strrpbrk (x, QSE_T("si")));
	qse_printf (QSE_T("[%s]\n"), qse_strpbrk (x, QSE_T("d")));
	qse_printf (QSE_T("[%s]\n"), qse_strrpbrk (x, QSE_T("d")));
	qse_printf (QSE_T("[%s]\n"), qse_strpbrk (x, QSE_T("t")));
	qse_printf (QSE_T("[%s]\n"), qse_strrpbrk (x, QSE_T("t")));
	return 0;
}

static int test17 (void)
{
	const qse_char_t* ptn[] = 
	{
		QSE_T("*.c"),
		QSE_T("h??lo.???"),
		QSE_T("[a-z]*.cpp")
	};

	const qse_char_t* name[] = 
	{
		QSE_T("hello.c"),
		QSE_T("hello.cpp"),
		QSE_T("heLLo.Cpp"),
		QSE_T("/tmp/hello.c"),
		QSE_T("/tmp/Hello.c")
	};

	int i, j;


	qse_printf (QSE_T("flags => 0\n"));
	for (i = 0; i < QSE_COUNTOF(ptn); i++)
		for (j = 0; j < QSE_COUNTOF(name); j++)
			qse_printf (QSE_T("[%s] [%s] %d\n"), ptn[i], name[j], qse_strfnmat (name[j], ptn[i], 0));

	qse_printf (QSE_T("flags => QSE_STRFNMAT_PATHNAME\n"));
	for (i = 0; i < QSE_COUNTOF(ptn); i++)
		for (j = 0; j < QSE_COUNTOF(name); j++)
			qse_printf (QSE_T("[%s] [%s] %d\n"), ptn[i], name[j], qse_strfnmat (name[j], ptn[i], QSE_STRFNMAT_PATHNAME));

	qse_printf (QSE_T("flags => QSE_STRFNMAT_PATHNAME | QSE_STRFNMAT_IGNORECASE\n"));
	for (i = 0; i < QSE_COUNTOF(ptn); i++)
		for (j = 0; j < QSE_COUNTOF(name); j++)
			qse_printf (QSE_T("[%s] [%s] %d\n"), ptn[i], name[j], qse_strfnmat (name[j], ptn[i], QSE_STRFNMAT_PATHNAME | QSE_STRFNMAT_IGNORECASE));


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

	R (test11);
	R (test12);
	R (test13);
	R (test14);
	R (test15);
	R (test16);
	R (test17);

#if defined(_WIN32)
	SetConsoleOutputCP (old_cp);
#endif
	return 0;
}
