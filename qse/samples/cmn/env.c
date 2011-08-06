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

	env = qse_env_open (QSE_NULL, 0);

	qse_env_addvar (env, QSE_T("alice"), QSE_T("wonderland"));
	qse_env_addvar (env, QSE_T("cool"), QSE_T("mint"));

	envstr = qse_env_getstr (env);
	while (*envstr != QSE_T('\0'))
	{
		qse_printf (QSE_T("%s\n"), envstr);
		envstr += qse_strlen(envstr) + 1;
	}

	qse_env_close (env);	
	return 0;
}

int main ()
{
	R (test1);
	return 0;
}
