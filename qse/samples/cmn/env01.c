#include <qse/cmn/env.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/cmn/sio.h>

#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) return -1; \
	} while (0)

static void dump (qse_env_t* env)
{
	const qse_env_char_t* envstr;
	qse_env_char_t** envarr;

	envstr = qse_env_getstr (env);
#if (defined(QSE_ENV_CHAR_IS_WCHAR) && defined(QSE_CHAR_IS_WCHAR)) || \
    (defined(QSE_ENV_CHAR_IS_MCHAR) && defined(QSE_CHAR_IS_MCHAR)) 
	while (*envstr != QSE_T('\0'))
	{
		qse_printf (QSE_T("%p [%s]\n"), envstr, envstr);
		envstr += qse_strlen(envstr) + 1;
	}
#elif defined(QSE_ENV_CHAR_IS_WCHAR) 
	while (*envstr != QSE_WT('\0'))
	{
		qse_printf (QSE_T("%p [%S]\n"), envstr, envstr);
		envstr += qse_wcslen(envstr) + 1;
	}
#else
	while (*envstr != QSE_MT('\0'))
	{
		qse_printf (QSE_T("%p [%S]\n"), envstr, envstr);
		envstr += qse_mbslen(envstr) + 1;
	}
#endif

	qse_printf (QSE_T("-------------\n"));
	envarr = qse_env_getarr (env);
	while (*envarr)
	{
#if (defined(QSE_ENV_CHAR_IS_WCHAR) && defined(QSE_CHAR_IS_WCHAR)) || \
    (defined(QSE_ENV_CHAR_IS_MCHAR) && defined(QSE_CHAR_IS_MCHAR)) 
		qse_printf (QSE_T("%p [%s]\n"), *envarr, *envarr);
#else
		qse_printf (QSE_T("%p [%S]\n"), *envarr, *envarr);
#endif
		envarr++;
	}
}

static int test1 (void)
{
	qse_env_t* env;

	env = qse_env_open (QSE_MMGR_GETDFL(), 0, 0);

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

	dump (env);

	qse_env_close (env);	
	return 0;
}

static int test2 (void)
{
	qse_env_t* env;

	env = qse_env_open (QSE_MMGR_GETDFL(), 0, 1);

	qse_printf (QSE_T("DELETING HOME => %s\n"),
		(qse_env_delete (env, QSE_T("HOME")) == 0? 
			QSE_T("SUCCESS"): QSE_T("FAILURE"))
	);
	qse_printf (QSE_T("DELETING wolf => %s\n"),
		(qse_env_delete (env, QSE_T("wolf")) == 0? 
			QSE_T("SUCCESS"): QSE_T("FAILURE"))
	);

	dump (env);

	qse_env_close (env);	
	return 0;
}

static int test3 (void)
{
	qse_env_t* env;

	env = qse_env_open (QSE_MMGR_GETDFL(), 0, 0);

	qse_printf (QSE_T("appending to PATH => %d\n"), qse_env_append (env, QSE_T("xxx"))); /* this should fail as there's no item in the environment */
	qse_printf (QSE_T("inserting PATH => %d\n"), qse_env_insert (env, QSE_T("PATH"), QSE_NULL));
	qse_printf (QSE_T("appending to PATH => %d\n"), qse_env_append (env, QSE_T(":/usr/xxx/bin:/tmp/bin:/var/lib/qse/bin/sbin/test/bin")));
	qse_printf (QSE_T("appending to PATH => %d\n"), qse_env_append (env, QSE_T("")));

	qse_printf (QSE_T("inserting HOME => %d\n"), qse_env_insertmbs (env, QSE_MT("HOME"), QSE_NULL));
	qse_printf (QSE_T("inserting USER => %d\n"), qse_env_insertwcs (env, QSE_WT("USER"), QSE_NULL));
	qse_printf (QSE_T("inserting WHAT => %d\n"), qse_env_insert (env, QSE_T("WHAT"), QSE_NULL));
	qse_printf (QSE_T("inserting an empty string => %d\n"), qse_env_insert (env, QSE_T(""), QSE_NULL));

	dump (env);

	qse_env_close (env);	
	return 0;

}
int main ()
{
	qse_openstdsios();

	R (test1);
	R (test2);
	R (test3);

	qse_closestdsios();
	return 0;
}
