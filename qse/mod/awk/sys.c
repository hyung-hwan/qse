#include <qse/awk/awk.h>
#include <qse/cmn/str.h>

#if defined(_WIN32)
#	include <windows.h>
#	include <process.h>
#elif defined(__OS2__)
#	define INCL_DOSPROCESS
#	define INCL_DOSEXCEPTIONS
#	define INCL_ERRORS
#	include <os2.h>
#elif defined(__DOS__)
#	include <dos.h>
#else
#	include <unistd.h>
#	include <errno.h>
#endif

static int fnc_sleep (qse_awk_rtx_t* run, const qse_cstr_t* fnm)
{
	qse_size_t nargs;
	qse_awk_val_t* a0;
	qse_long_t lv;
	qse_flt_t rv;
	qse_awk_val_t* r;
	int n;

	nargs = qse_awk_rtx_getnargs (run);
	QSE_ASSERT (nargs == 1);

	a0 = qse_awk_rtx_getarg (run, 0);

	n = qse_awk_rtx_valtonum (run, a0, &lv, &rv);
	if (n == -1) return -1;
	if (n == 1) lv = (qse_long_t)rv;

#if defined(_WIN32)
	Sleep ((DWORD)(lv * 1000));
	n = 0;
#elif defined(__OS2__)
	DosSleep ((ULONG)(lv * 1000));
	n = 0;
#else
	n = sleep (lv);	
#endif

	r = qse_awk_rtx_makeintval (run, n);
	if (r == QSE_NULL) return -1;

	qse_awk_rtx_setretval (run, r);
	return 0;
}

int query (qse_awk_t* awk, const qse_char_t* name, qse_awk_mod_info_t* info)
{
	qse_cstr_t ea;

	if (qse_strcmp (name, QSE_T("sleep")) == 0)
	{
		info->type = QSE_AWK_MOD_FNC;
		info->u.f.arg.min = 1;
		info->u.f.arg.max = 1;
		info->u.f.impl = fnc_sleep;
		return 0;
	}

	ea.ptr = name;
	ea.len = qse_strlen(name);
	qse_awk_seterror (awk, QSE_AWK_ENOENT, &ea, QSE_NULL);
	return -1;
}
