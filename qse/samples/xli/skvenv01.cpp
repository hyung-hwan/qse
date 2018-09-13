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

#include <qse/xli/SkvEnv.hpp>
#include <qse/si/sio.h>
#include <qse/cmn/main.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/str.h>
#include <qse/cmn/String.hpp>

#include <locale.h>
#if defined(_WIN32)
#	include <windows.h>
#endif

class MyEnv: public QSE::SkvEnv
{
public:
	int probe_agent_name (const qse_char_t* name)
	{
qse_printf (QSE_T("probing agent_name %s\n"), name);
		return 0;
	}
};

MyEnv myenv;

static int env_main (int argc, qse_char_t* argv[])
{
	myenv.addItem (QSE_T("main*target"), QSE_T("file"));
	myenv.addItem (QSE_T("main*targetq"), QSE_T("file"));
	myenv.addItem (QSE_T("agent*name"), QSE_T("my-agent"), (MyEnv::ProbeProc)&MyEnv::probe_agent_name);

	myenv.load (QSE_T("myenv.conf"));

	QSE::String x (myenv.getValue(QSE_T("main*target")));
	x.append (QSE_T("-jj"));
	myenv.setValue(QSE_T("main*target"), x);

	const qse_char_t* v = myenv.getValue(QSE_T("main*target"));
	qse_printf (QSE_T("%s\n"), (v? v: QSE_T("<null>")));

	myenv.store (QSE_T("myenv.conf"));
	return 0;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	int x;
	qse_open_stdsios ();

	{
	#if defined(_WIN32)
		char locale[100];
		UINT codepage = GetConsoleOutputCP();
		if (codepage == CP_UTF8)
		{
			/*SetConsoleOUtputCP (CP_UTF8);*/
			qse_setdflcmgrbyid (QSE_CMGR_UTF8);
		}
		else
		{
			sprintf (locale, ".%u", (unsigned int)codepage);
			setlocale (LC_ALL, locale);
			/*qse_setdflcmgrbyid (QSE_CMGR_SLMB);*/
		 }
	#else
		setlocale (LC_ALL, "");
		/*qse_setdflcmgrbyid (QSE_CMGR_SLMB);*/
	#endif
	}

	x = qse_run_main (argc,argv,env_main);
	qse_close_stdsios ();
	return x;
}
