/*
 * $Id: main.c 463 2008-12-09 06:52:03Z baconevi $
 *
 * {License}
 */

#include <ase/utl/main.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#if defined(_WIN32) && !defined(__MINGW32__)

int ase_runmain (int argc, ase_achar_t* argv[], int(*mf) (int,ase_char_t*[]))
{
	return mf (argc, argv);
}

#elif defined(ASE_CHAR_IS_WCHAR)

int ase_runmain (int argc, ase_achar_t* argv[], int(*mf) (int,ase_char_t*[]))
{
	int i, ret;
	ase_char_t** v;

	setlocale (LC_ALL, "");

	v = (ase_char_t**) malloc (argc * ASE_SIZEOF(ase_char_t*));
	if (v == NULL) return -1;

	for (i = 0; i < argc; i++) v[i] = NULL;
	for (i = 0; i < argc; i++) 
	{
		ase_size_t n, len, rem;
		char* p = argv[i];

		len = 0; rem = strlen (p);
		while (*p != '\0')
		{
			int x = mblen (p, rem); 
			if (x == -1) 
			{
				ret = -1;
				goto exit_main;
			}
			if (x == 0) break;
			p += x; rem -= x; len++;
		}

	#if (defined(vms) || defined(__vms)) && (ASE_SIZEOF_VOID_P >= 8)
		v[i] = (ase_char_t*) _malloc32 ((len+1)*ASE_SIZEOF(ase_char_t));
	#else
		v[i] = (ase_char_t*) malloc ((len+1)*ASE_SIZEOF(ase_char_t));
	#endif
		if (v[i] == NULL) 
		{
			ret = -1;
			goto exit_main;
		}

		n = mbstowcs (v[i], argv[i], len);
		if (n == (size_t)-1) 
		{
			/* error */
			return -1;
		}

		if (n == len) v[i][len] = ASE_T('\0');
	}

	/* TODO: envp... */
	//ret = mf (argc, v, NULL);
	ret = mf (argc, v);

exit_main:
	for (i = 0; i < argc; i++) 
	{
		if (v[i] != NULL) free (v[i]);
	}
	free (v);

	return ret;
}

#else

int ase_runmain (int argc, ase_achar_t* argv[], int(*mf) (int,ase_char_t*[]))
{
	return mf (argc, argv);
}

#endif
