/**
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

#include <qse/sed/StdSed.hpp>
#include <qse/utl/main.h>
#include <qse/utl/stdio.h>

int sed_main (int argc, qse_char_t* argv[])
{
	if (argc !=  2)
	{
		qse_fprintf (QSE_STDERR, 
			QSE_T("usage: %s command-string\n"), argv[0]);
		return -1;
	}

	QSE::StdSed sed;

	if (sed.open () == -1)
	{
		qse_printf (QSE_T("cannot open a stream editor - %s\n"), 
			sed.getErrorMessage());
		return -1;
	}

	if (sed.compile (argv[1]) == -1)
	{
		qse_printf (QSE_T("cannot compile - %s\n"), 
			sed.getErrorMessage());
		sed.close ();
		return -1;
	}

	if (sed.execute () == -1)
	{
		qse_printf (QSE_T("cannot execute - %s\n"),
			sed.getErrorMessage());
		sed.close ();
		return -1;
	}

	sed.close ();
	return 0;
}

int qse_main (int argc, char* argv[])
{
	return qse_runmain (argc, argv, sed_main);
}
