/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
		v[i]= qse_mbstowcsalldup (argv[i], QSE_NULL, mmgr);
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

		v[i]= qse_mbstowcsalldup (x, QSE_NULL, mmgr);
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

