/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
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

#ifndef _QSE_CMN_SCK_H_
#define _QSE_CMN_SCK_H_


#include <qse/types.h>
#include <qse/macros.h>

#if defined(_WIN32)
	typedef qse_uintptr_t qse_sck_hnd_t;
#	define QSE_INVALID_SCKHND (~(qse_sck_hnd_t)0)
#elif defined(__OS2__)
	typedef int qse_sck_hnd_t;
#	define QSE_INVALID_SCKHND (-1)
#elif defined(__DOS__)
	typedef int qse_sck_hnd_t;
#	define QSE_INVALID_SCKHND (-1)
#else
	typedef int qse_sck_hnd_t;
#	define QSE_INVALID_SCKHND (-1)
#endif

#if (QSE_SIZEOF_SOCKLEN_T == QSE_SIZEOF_INT)
	#if defined(QSE_SOCKLEN_T_IS_SIGNED)
		typedef int qse_sck_len_t;
	#else
		typedef unsigned int qse_sck_len_t;
	#endif
#elif (QSE_SIZEOF_SOCKLEN_T == QSE_SIZEOF_LONG)
	#if defined(QSE_SOCKLEN_T_IS_SIGNED)
		typedef long qse_sck_len_t;
	#else
		typedef unsigned long qse_sck_len_t;
	#endif
#else
	typedef int qse_sck_len_t;
#endif

enum qse_shutsckhnd_how_t
{
	QSE_SHUTSCKHND_R  = 0,
	QSE_SHUTSCKHND_W  = 1,
	QSE_SHUTSCKHND_RW = 2
};
typedef enum qse_shutsckhnd_how_t qse_shutsckhnd_how_t;

#ifdef __cplusplus
extern "C" {
#endif

QSE_EXPORT int qse_isvalidsckhnd (
	qse_sck_hnd_t handle
);

QSE_EXPORT void qse_closesckhnd (
	qse_sck_hnd_t handle
);


QSE_EXPORT void qse_shutsckhnd (
	qse_sck_hnd_t        handle,
	qse_shutsckhnd_how_t how
);

#ifdef __cplusplus
}
#endif


#endif
	
