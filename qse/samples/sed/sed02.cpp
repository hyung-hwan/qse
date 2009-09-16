/**
 * $Id$
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

#include <qse/sed/StdSed.hpp>
#include <qse/cmn/main.h>
#include <qse/cmn/stdio.h>

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
