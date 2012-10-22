#include <qse/awk/std.h>
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
#	include <sys/wait.h>
#	include <errno.h>
#endif

static int fnc_fork (qse_awk_rtx_t* rtx, const qse_cstr_t* fnm)
{
#if defined(_WIN32)
	qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOIMPL, QSE_NULL);
	return -1;
	
#elif defined(__OS2__)
	qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOIMPL, QSE_NULL);
	return -1;
	
#elif defined(__DOS__)
	qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOIMPL, QSE_NULL);
	return -1;

#else
	pid_t pid;
	qse_awk_val_t* r;

	pid = fork ();
	r = qse_awk_rtx_makeintval (rtx, pid);
	if (r == QSE_NULL) return -1;
	qse_awk_rtx_setretval (rtx, r);
	return 0;
#endif
}

static int fnc_wait (qse_awk_rtx_t* rtx, const qse_cstr_t* fnm)
{
	qse_size_t nargs;
	qse_awk_val_t* a0;
	qse_long_t lv;
	qse_flt_t rv;
	qse_awk_val_t* r;
	int n;

	nargs = qse_awk_rtx_getnargs (rtx);
	QSE_ASSERT (nargs == 1);

/* TODO: handel more parameters */

	a0 = qse_awk_rtx_getarg (rtx, 0);

	n = qse_awk_rtx_valtonum (rtx, a0, &lv, &rv);
	if (n == -1) return -1;
	if (n == 1) lv = (qse_long_t)rv;

#if defined(_WIN32)
	qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOIMPL, QSE_NULL);
	return -1;
	
#elif defined(__OS2__)
	qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOIMPL, QSE_NULL);
	return -1;
	
#elif defined(__DOS__)
	qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOIMPL, QSE_NULL);
	return -1;

#else
	n = waitpid (lv, QSE_NULL, 0);
#endif

	r = qse_awk_rtx_makeintval (rtx, n);
	if (r == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, r);
	return 0;
}

static int fnc_sleep (qse_awk_rtx_t* rtx, const qse_cstr_t* fnm)
{
	qse_size_t nargs;
	qse_awk_val_t* a0;
	qse_long_t lv;
	qse_flt_t rv;
	qse_awk_val_t* r;
	int n;

	nargs = qse_awk_rtx_getnargs (rtx);
	QSE_ASSERT (nargs == 1);

	a0 = qse_awk_rtx_getarg (rtx, 0);

	n = qse_awk_rtx_valtonum (rtx, a0, &lv, &rv);
	if (n == -1) return -1;
	if (n == 1) lv = (qse_long_t)rv;

#if defined(_WIN32)
	Sleep ((DWORD)(lv * 1000));
	n = 0;
#elif defined(__OS2__)
	DosSleep ((ULONG)(lv * 1000));
	n = 0;
#elif defined(__DOS__)
	n = sleep (lv);	
#else
	n = sleep (lv);	
#endif

	r = qse_awk_rtx_makeintval (rtx, n);
	if (r == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, r);
	return 0;
}

static int query (qse_awk_mod_t* mod, qse_awk_t* awk, const qse_char_t* name, qse_awk_mod_sym_t* sym)
{
	qse_cstr_t ea;

/* TODO: tabulation and binary search or something better */

	if (qse_strcmp (name, QSE_T("fork")) == 0)
	{
		sym->type = QSE_AWK_MOD_FNC;
		sym->u.f.arg.min = 0;
		sym->u.f.arg.max = 0;
		sym->u.f.impl = fnc_fork;
		return 0;
	}
	else if (qse_strcmp (name, QSE_T("wait")) == 0)
	{
		sym->type = QSE_AWK_MOD_FNC;
		sym->u.f.arg.min = 1; /* TODO: accept more parameters.. */
		sym->u.f.arg.max = 1;
		sym->u.f.impl = fnc_wait;
		return 0;
	}
/*
	else if (qse_strcmp (name, QSE_T("WNOHANG")) == 0)
	{
		sym->type = QSE_AWK_MOD_INTCON;
		sym->u.c.ivalue = WNOHANG;
		return 0;
	}
*/
	else if (qse_strcmp (name, QSE_T("sleep")) == 0)
	{
		sym->type = QSE_AWK_MOD_FNC;
		sym->u.f.arg.min = 1;
		sym->u.f.arg.max = 1;
		sym->u.f.impl = fnc_sleep;
		return 0;
	}

	ea.ptr = name;
	ea.len = qse_strlen(name);
	qse_awk_seterror (awk, QSE_AWK_ENOENT, &ea, QSE_NULL);
	return -1;
}

/* TODO: proper resource management */

int init (qse_awk_mod_t* mod, qse_awk_rtx_t* rtx)
{
	return 0;
}

void fini (qse_awk_mod_t* mod, qse_awk_rtx_t* rtx)
{
	/* TODO: 
	for (each pid for rtx) kill (pid, SIGKILL);
	for (each pid for rtx) waitpid (pid, QSE_NULL, 0);
	*/
}

static void unload (qse_awk_mod_t* mod, qse_awk_t* awk)
{
	/* TODO: anything */
}

int load (qse_awk_mod_t* mod, qse_awk_t* awk)
{
	mod->query = query;
	mod->unload = unload;

	mod->init = init;
	mod->fini = fini;
	/*
	mod->ctx...
	 */

	return 0;
}

