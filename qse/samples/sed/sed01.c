/*
 * $Id$
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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

#include <qse/sed/std.h>
#include <qse/cmn/main.h>
#include <qse/cmn/stdio.h>

int sed_main (int argc, qse_char_t* argv[])
{
	qse_sed_t* sed = QSE_NULL;
	qse_char_t* infile;
	qse_char_t* outfile;
	int ret = -1;

	if (argc <  2 || argc > 4)
	{
		qse_fprintf (QSE_STDERR, QSE_T("USAGE: %s command-string [input-file [output-file]]\n"), argv[0]);
		return -1;
	}

	sed = qse_sed_openstd (0);
	if (sed == QSE_NULL)  
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: cannot open sed\n"));
		goto oops;
	}

	if (qse_sed_compstdmem (sed, argv[1]) <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_sed_geterrmsg(sed));
		goto oops;
	}

	infile = (argc >= 3)? argv[2]: QSE_NULL;
	outfile = (argc >= 4)? argv[3]: QSE_NULL;

	if (qse_sed_execstdfile (sed, infile, outfile) <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("ERROR: %s\n"), qse_sed_geterrmsg(sed));
		goto oops;
	}

oops:
	if (sed != QSE_NULL) qse_sed_close (sed);
	return ret;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	return qse_runmain (argc, argv, sed_main);
}

