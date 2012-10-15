/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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
#include <qse/cmn/mbwc.h>
#include "mem.h"

int qse_runmain (
	int argc, qse_achar_t* argv[], qse_runmain_handler_t handler)
{
#if (defined(QSE_ACHAR_IS_MCHAR) && defined(QSE_CHAR_IS_MCHAR)) || \
    (defined(QSE_ACHAR_IS_WCHAR) && defined(QSE_CHAR_IS_WCHAR))

	return handler (argc, (qse_char_t**)argv);

#else
	int i, ret;
	qse_char_t** v;
	qse_mmgr_t* mmgr = QSE_MMGR_GETDFL ();

	v = (qse_char_t**) QSE_MMGR_ALLOC (
		mmgr, (argc + 1) * QSE_SIZEOF(qse_char_t*));
	if (v == QSE_NULL) return -1;

	for (i = 0; i < argc + 1; i++) v[i] = QSE_NULL;

	for (i = 0; i < argc; i++)
	{
		v[i]= qse_mbstowcsalldup (argv[i], mmgr);
		if (v[i] == QSE_NULL)
		{
			ret = -1;
			goto oops;
		}
	}

	ret = handler (argc, v);

oops:
	for (i = 0; i < argc + 1; i++)
	{
		if (v[i] != QSE_NULL) QSE_MMGR_FREE (mmgr, v[i]);
	}
	QSE_MMGR_FREE (mmgr, v);

	return ret;
#endif
}

int qse_runmainwithenv (
	int argc, qse_achar_t* argv[], 
	qse_achar_t* envp[], qse_runmainwithenv_handler_t handler)
{
#if (defined(QSE_ACHAR_IS_MCHAR) && defined(QSE_CHAR_IS_MCHAR)) || \
    (defined(QSE_ACHAR_IS_WCHAR) && defined(QSE_CHAR_IS_WCHAR))
	return handler (argc, (qse_char_t**)argv, (qse_char_t**)envp);
#else
	int i, ret, envc;
	qse_char_t** v;
	qse_mmgr_t* mmgr = QSE_MMGR_GETDFL ();

	if (envp) for (envc = 0; envp[envc]; envc++) ; /* count the number of env items */
	else envc = 0;

	v = (qse_char_t**) QSE_MMGR_ALLOC (
		mmgr, (argc + 1 + envc + 1) * QSE_SIZEOF(qse_char_t*));
	if (v == QSE_NULL) return -1;

	for (i = 0; i < argc + 1 + envc + 1; i++) v[i] = QSE_NULL;

	for (i = 0; i < argc + 1 + envc; i++)
	{
		qse_achar_t* x;

		if (i < argc) x = argv[i];
		else if (i == argc) continue;
		else x = envp[i - argc - 1];

		v[i]= qse_mbstowcsalldup (x, mmgr);
		if (v[i] == QSE_NULL)
		{
			ret = -1;
			goto oops;
		}
	}

	ret = handler (argc, v, &v[argc + 1]);

oops:
	for (i = 0; i < argc + 1 + envc + 1; i++)
	{
		if (v[i] != QSE_NULL) QSE_MMGR_FREE (mmgr, v[i]);
	}
	QSE_MMGR_FREE (mmgr, v);

	return ret;
#endif
}

