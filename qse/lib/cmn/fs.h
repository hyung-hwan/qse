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

#include <qse/cmn/fs.h>

#if defined(_WIN32)
#	include <windows.h>
	typedef DWORD qse_fs_syserr_t;
#elif defined(__OS2__)
#	define INCL_DOSERRORS
#	include <os2.h>
	typedef APIRET qse_fs_syserr_t;
#elif defined(__DOS__)
#	include <errno.h>
	typedef int qse_fs_syserr_t;
#else
#	include "syscall.h"
	typedef int qse_fs_syserr_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

qse_fs_errnum_t qse_fs_syserrtoerrnum (
	qse_fs_t*       fs,
	qse_fs_syserr_t e
);

#ifdef __cplusplus
}
#endif
