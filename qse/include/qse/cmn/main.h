/*
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

#ifndef _QSE_CMN_MAIN_H_
#define _QSE_CMN_MAIN_H_

#include <qse/types.h>
#include <qse/macros.h>

/** @file
 * This file includes a macro and a function to help users to write a 
 * cross-platform main function.
 */

/**
 * The #qse_main macro defines a main function wrapper for an underlying 
 * platform. Combined with the qse_runmain() function, it provides a consistant
 * view to the main function.
 */
#if defined(_WIN32) && !defined(__MINGW32__)
#	if defined(QSE_CHAR_IS_MCHAR)
#		define qse_main main
		typedef qse_mchar_t qse_achar_t;
#	else
#		define qse_main wmain
		typedef qse_wchar_t qse_achar_t;
#	endif
#else
#	define qse_main main
	typedef qse_mchar_t qse_achar_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The qse_runmain() function helps to invoke a main function independent of
 * the character mode configured for the library.
 */
int qse_runmain (
	int          argc, 
	qse_achar_t* argv[],
	int        (*mf)(int,qse_char_t*[])
);

/* TODO - qse_runmain with env, namely, qse_runmaine */

#ifdef __cplusplus
}
#endif

#endif
