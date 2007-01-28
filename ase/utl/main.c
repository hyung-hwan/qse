/*
 * $Id: main.c,v 1.6 2007-01-28 11:33:23 bacon Exp $
 */

#include <ase/types.h>
#include <ase/macros.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>

#if defined(_WIN32)

#include <tchar.h>
#define ase_main _tmain

#elif defined(ASE_CHAR_IS_MCHAR)

#define ase_main main

#else /* ASE_CHAR_IS_WCHAR */

#ifdef __cplusplus
extern "C" { int ase_main (...); }
#else
extern int ase_main ();
#endif

int main (int argc, char* argv[]/*, char** envp*/)
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

		v[i] = (ase_char_t*) malloc (
			(len + 1) * ASE_SIZEOF(ase_char_t));
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
	ret = ase_main (argc, v, NULL);

exit_main:
	for (i = 0; i < argc; i++) 
	{
		if (v[i] != NULL) free (v[i]);
	}
	free (v);

	return ret;
}

#endif

