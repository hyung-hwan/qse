
#include <qse/cmn/tre.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/path.h>
#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/stdio.h>

#include <locale.h>
#if defined(_WIN32)
#	include <windows.h>
#endif


static int test_main (int argc, qse_char_t* argv[])
{
	qse_tre_t tre;
	unsigned int nsubmat;
	qse_tre_match_t* mat = QSE_NULL;

	if (argc != 3)
	{
		qse_printf (QSE_T("USAGE: %s pattern string\n"),
			qse_basename(argv[0]));
		return -1;
	}

	qse_tre_init (&tre, QSE_MMGR_GETDFL());

	if (qse_tre_comp (&tre, argv[1], &nsubmat, QSE_TRE_EXTENDED) <= -1)
	{
		qse_printf (QSE_T("ERROR: Cannot compile pattern [%s] - %s\n"), argv[1], qse_tre_geterrmsg(&tre));
		goto oops;
	}	

	if (nsubmat > 0)
	{
		mat = QSE_MMGR_ALLOC (qse_tre_getmmgr(&tre), QSE_SIZEOF(*mat) * nsubmat);
		if (mat == QSE_NULL)
		{
			qse_printf (QSE_T("ERROR: Cannot allocate submatch array\n"));
			goto oops;
		}
	}

	if (qse_tre_exec(&tre, argv[2], mat, nsubmat,  0) <= -1)
	{
		if (QSE_TRE_ERRNUM(&tre) == QSE_TRE_ENOMATCH) qse_printf (QSE_T("Match: NO\n"));
		else 
		{
			qse_printf (QSE_T("ERROR: Cannot not match pattern - %s\n"), qse_tre_geterrmsg(&tre));
			goto oops;
		}
	}
	else
	{
		unsigned int i;
		qse_printf (QSE_T("Match: YES\n"));

		for (i = 0; i < nsubmat; i++)
		{
			if (mat[i].rm_so == -1) break;
			qse_printf (QSE_T("SUBMATCH[%u] = [%.*s]\n"), i, 
				(int)(mat[i].rm_eo - mat[i].rm_so), &argv[2][mat[i].rm_so]);
		}
	}

	if (mat) QSE_MMGR_FREE (qse_tre_getmmgr(&tre), mat);
	qse_tre_fini (&tre);
	return 0;

oops:
	if (mat) QSE_MMGR_FREE (qse_tre_getmmgr(&tre), mat);
	qse_tre_fini (&tre);
	return -1;
}

int qse_main (int argc, qse_achar_t* argv[])
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
	return qse_runmain (argc, argv, test_main);
}

