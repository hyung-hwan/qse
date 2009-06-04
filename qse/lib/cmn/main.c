/*
 * $Id: main.c 463 2008-12-09 06:52:03Z baconevi $
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include <qse/cmn/main.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#if defined(_WIN32) && !defined(__MINGW32__)

int qse_runmain (int argc, qse_achar_t* argv[], int(*mf) (int,qse_char_t*[]))
{
	return mf (argc, argv);
}

#elif defined(QSE_CHAR_IS_WCHAR)

int qse_runmain (int argc, qse_achar_t* argv[], int(*mf) (int,qse_char_t*[]))
{
	int i, ret;
	qse_char_t** v;

	setlocale (LC_ALL, "");

	v = (qse_char_t**) malloc (argc * QSE_SIZEOF(qse_char_t*));
	if (v == NULL) return -1;

	for (i = 0; i < argc; i++) v[i] = NULL;
	for (i = 0; i < argc; i++) 
	{
		qse_size_t n, len, rem;
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

	#if (defined(vms) || defined(__vms)) && (QSE_SIZEOF_VOID_P >= 8)
		v[i] = (qse_char_t*) _malloc32 ((len+1)*QSE_SIZEOF(qse_char_t));
	#else
		v[i] = (qse_char_t*) malloc ((len+1)*QSE_SIZEOF(qse_char_t));
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

		if (n == len) v[i][len] = QSE_T('\0');
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

int qse_runmain (int argc, qse_achar_t* argv[], int(*mf) (int,qse_char_t*[]))
{
	return mf (argc, argv);
}

#endif
