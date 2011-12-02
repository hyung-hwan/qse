/**
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

#include <qse/cmn/fs.h>
#include <qse/cmn/main.h>
#include <qse/cmn/stdio.h>

#if defined(_WIN32)
#	include <windows.h>
#	include <tchar.h>
#	include <process.h>
#elif defined(__OS2__)
#	define INCL_DOSPROCESS
#	define INCL_DOSEXCEPTIONS
#	define INCL_ERRORS
#	include <os2.h>
#	include <signal.h>
#elif defined(__DOS__)
#	include <dos.h>
#	include <signal.h>
#else
#	include <unistd.h>
#	include <errno.h>
#	include <signal.h>
#endif

int fs_main (int argc, qse_char_t* argv[])
{
	qse_fs_t* fs;


	fs = qse_fs_open (QSE_NULL, 0);
	if (fs == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR,  QSE_T("ERROR: cannot open fs\n"));
		return -1;
	}
	

	if (qse_fs_move (fs, argv[1], argv[2]) <= -1)
	{
		qse_fprintf (QSE_STDERR,  QSE_T("ERROR: cannot move %s to %s - %s\n"), argv[1], argv[2], qse_fs_geterrmsg(fs));
		qse_fs_close (fs);
		return -1;		
	}

	qse_fs_close (fs);
	return 0;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	return qse_runmain (argc, argv, fs_main);
}
