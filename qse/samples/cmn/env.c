#include <qse/cmn/env.h>
#include <qse/cmn/str.h>
#include <qse/cmn/stdio.h>

#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) return -1; \
	} while (0)

static int test1 (void)
{
	qse_env_t* env;
	const qse_char_t* envstr;
	qse_char_t** envarr;

	env = qse_env_open (QSE_NULL, 0, 0);

	qse_env_clear (env);
	qse_env_insert (env, QSE_T("alice"), QSE_T("wonderland"));
	qse_env_insert (env, QSE_T("cool"), QSE_T("mint"));
	qse_env_insert (env, QSE_T("smurf"), QSE_T("happy song"));
	qse_env_insert (env, QSE_T("donkey"), QSE_T("mule"));
	qse_env_insert (env, QSE_T("lily"), QSE_T("rose"));

	qse_env_clear (env);
	qse_env_insert (env, QSE_T("alice"), QSE_T("wonderland"));
	qse_env_insert (env, QSE_T("cool"), QSE_T("mint"));
	qse_env_insert (env, QSE_T("smurf"), QSE_T("happy song"));
	qse_env_insert (env, QSE_T("donkey"), QSE_T("mule"));
	qse_env_insert (env, QSE_T("lily"), QSE_T("rose"));


	qse_env_delete (env, QSE_T("cool"));
	qse_env_insert (env, QSE_T("spider"), QSE_T("man"));

	envstr = qse_env_getstr (env);
	if (envstr)
	{
		while (*envstr != QSE_T('\0'))
		{
			qse_printf (QSE_T("%p [%s]\n"), envstr, envstr);
			envstr += qse_strlen(envstr) + 1;
		}
	}

	qse_printf (QSE_T("=====\n"));
	envarr = qse_env_getarr (env);
	if (envarr)
	{
		while (*envarr)
		{
			qse_printf (QSE_T("%p [%s]\n"), *envarr, *envarr);
			envarr++;
		}
	}

	qse_env_close (env);	
	return 0;
}

static int test2 (void)
{
	qse_env_t* env;
	const qse_char_t* envstr;
	qse_char_t** envarr;

	env = qse_env_open (QSE_NULL, 0, 1);

	qse_printf (QSE_T("DELETING HOME => %s\n"),
		(qse_env_delete (env, QSE_T("HOME")) == 0? 
			QSE_T("SUCCESS"): QSE_T("FAILURE"))
	);
	qse_printf (QSE_T("DELETING wolf => %s\n"),
		(qse_env_delete (env, QSE_T("wolf")) == 0? 
			QSE_T("SUCCESS"): QSE_T("FAILURE"))
	);

	envstr = qse_env_getstr (env);
	if (envstr)
	{
		while (*envstr != QSE_T('\0'))
		{
			qse_printf (QSE_T("%p [%s]\n"), envstr, envstr);
			envstr += qse_strlen(envstr) + 1;
		}
	}


	qse_printf (QSE_T("=====\n"));
	envarr = qse_env_getarr (env);
	if (envarr)
	{
		while (*envarr)
		{
			qse_printf (QSE_T("%p [%s]\n"), *envarr, *envarr);
			envarr++;
		}
	}

	qse_env_close (env);	
	return 0;
}

int main ()
{
	R (test1);
	R (test2);
	return 0;
}
