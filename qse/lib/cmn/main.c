/*
 * $Id: main.c 463 2008-12-09 06:52:03Z baconevi $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qse/cmn/main.h>
#include <qse/cmn/str.h>
#include <locale.h>
#include "mem.h"

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
	qse_mmgr_t* mmgr = QSE_MMGR_GETDFL ();

	setlocale (LC_ALL, "");

	v = (qse_char_t**) QSE_MMGR_ALLOC (
		mmgr, argc * QSE_SIZEOF(qse_char_t*));
	if (v == QSE_NULL) return -1;

	for (i = 0; i < argc; i++) v[i] = QSE_NULL;

	for (i = 0; i < argc; i++) 
	{
		qse_size_t n, len, nlen;
		qse_size_t mbslen;

		mbslen = qse_mbslen (argv[i]);

		n = qse_mbstowcslen (argv[i], &len);
		if (n < mbslen)
		{
			ret = -1; goto oops;
		}

		len++; /* include the terminating null */

		v[i] = (qse_char_t*) QSE_MMGR_ALLOC (
			mmgr, len*QSE_SIZEOF(qse_char_t));
		if (v[i] == QSE_NULL) 
		{
			ret = -1; goto oops;
		}

		nlen = len;
		n = qse_mbstowcs (argv[i], v[i], &nlen);
		if (nlen >= len)
		{
			/* no null-termination */
			ret = -1; goto oops;
		}
		if (argv[i][n] != '\0')
		{		
			/* partial processing */
			ret = -1; goto oops;
		}
	}

	/* TODO: envp... */
	//ret = mf (argc, v, QSE_NULL);
	ret = mf (argc, v);

oops:
	for (i = 0; i < argc; i++) 
	{
		if (v[i] != QSE_NULL) QSE_MMGR_FREE (mmgr, v[i]);
	}
	QSE_MMGR_FREE (mmgr, v);

	return ret;
}

#else

int qse_runmain (int argc, qse_achar_t* argv[], int(*mf) (int,qse_char_t*[]))
{
	return mf (argc, argv);
}

#endif
