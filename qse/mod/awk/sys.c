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

static int fnc_fork (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
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
	qse_awk_val_t* retv;

	pid = fork ();
	retv = qse_awk_rtx_makeintval (rtx, pid);
	if (retv == QSE_NULL) return -1;
	qse_awk_rtx_setretval (rtx, retv);
	return 0;
#endif
}

static int fnc_wait (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_long_t lv;
	qse_awk_val_t* retv;
	int n;

/* TODO: handle more parameters */

	n = qse_awk_rtx_valtolong (rtx, qse_awk_rtx_getarg (rtx, 0), &lv);
	if (n <= -1) return -1;

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

	retv = qse_awk_rtx_makeintval (rtx, n);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_sleep (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_long_t lv;
	qse_awk_val_t* retv;
	int n;

	n = qse_awk_rtx_valtolong (
		rtx, qse_awk_rtx_getarg (rtx, 0), &lv);
	if (n <= -1) return -1;

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

	retv = qse_awk_rtx_makeintval (rtx, n);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

typedef struct fnctab_t fnctab_t;
struct fnctab_t
{
	const qse_char_t* name;
	qse_awk_mod_sym_fnc_t info;
};

typedef struct inttab_t inttab_t;
struct inttab_t
{
	const qse_char_t* name;
	qse_awk_mod_sym_int_t info;
};

static fnctab_t fnctab[] =
{
	{ QSE_T("fork"),    { { 0, 0 }, fnc_fork } },
	{ QSE_T("sleep"),   { { 1, 1 }, fnc_sleep  } },
	{ QSE_T("wait"),    { { 1, 1 }, fnc_wait  } }
};

static inttab_t inttab[] =
{
	{ QSE_T("WNOHANG"), { WNOHANG } }
};

static int query (qse_awk_mod_t* mod, qse_awk_t* awk, const qse_char_t* name, qse_awk_mod_sym_t* sym)
{
	qse_cstr_t ea;
	int i;

/* TODO: binary search or something better */
	for (i = 0; i < QSE_COUNTOF(fnctab); i++)
	{
		if (qse_strcmp (fnctab[i].name, name) == 0)
		{
			sym->type = QSE_AWK_MOD_FNC;
			sym->u.fnc = fnctab[i].info;
			return 0;
		}
	}

	for (i = 0; i < QSE_COUNTOF(inttab); i++)
	{
		if (qse_strcmp (inttab[i].name, name) == 0)
		{
			sym->type = QSE_AWK_MOD_INT;
			sym->u.in = inttab[i].info;
			return 0;
		}
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

