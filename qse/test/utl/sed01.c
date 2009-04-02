/*
 * $Id$
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

/****S* SED/Simple Example
 * SOURCE
 */

#include <qse/utl/sed.h>
#include <qse/utl/stdio.h>
#include <qse/utl/main.h>
#include <qse/cmn/str.h>

int sed_main (int argc, qse_char_t* argv[])
{
	qse_sed_t* sed = QSE_NULL;
	int ret = -1;

	if (argc != 2 && argc != 3)
	{
		qse_fprintf (QSE_STDERR, QSE_T("usage: %s string\n"), argv[0]);
		return -1;
	}

	sed = qse_sed_open (QSE_NULL, 0);
	if (sed == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("cannot open a stream editor\n"));
		goto oops;
	}
	
	if (argc == 3) qse_sed_setoption (sed, qse_strtoi(argv[2]));

	if (qse_sed_compile (sed, argv[1], qse_strlen(argv[1])) == -1)
	{
		qse_fprintf (QSE_STDERR, 
			QSE_T("cannot compile - %s\n"),
			qse_sed_geterrmsg(sed)
		);
		goto oops;
	}

	if (qse_sed_execute (sed/*, io*/) == -1)
	{
	}

	ret = 0;

oops:
	if (sed != QSE_NULL) qse_sed_close (sed);
	return ret;
}

int qse_main (int argc, char* argv[])
{
	return qse_runmain (argc, argv, sed_main);
}

/******/
