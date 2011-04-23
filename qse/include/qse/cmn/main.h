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

#ifndef _QSE_CMN_MAIN_H_
#define _QSE_CMN_MAIN_H_

#include <qse/types.h>
#include <qse/macros.h>

/** @file
 * This file includes a macro and a function to help users to write a 
 * cross-platform main function.
 */

/**
 * @def qse_main
 * The qse_main macro defines a main function wrapper for an underlying 
 * platform. It is defined to @b main or @b wmain depending on the choice of
 * the default character type #qse_char_t. Combined with the qse_runmain() 
 * function, it provides a consistant view to the main function.
 *
 * @typedef qse_achar_t
 * The qse_achar_t type defines a character type for the second parameter to 
 * #qse_main. It is defined to #qse_mchar_t or #qse_wchar_t depending on the
 * choice of the default character type #qse_char_t.
 */

#if defined(_WIN32) && !defined(__MINGW32__)
#	if defined(QSE_CHAR_IS_MCHAR)
#		define qse_main main
#		define QSE_ACHAR_IS_MCHAR
		typedef qse_mchar_t qse_achar_t;
#	else
#		define qse_main wmain
#		define QSE_ACHAR_IS_WCHAR
		typedef qse_wchar_t qse_achar_t;
#	endif
#elif defined(__OS2__)
#	define qse_main main
#	define QSE_ACHAR_IS_MCHAR
	typedef qse_mchar_t qse_achar_t;
#else
#	define qse_main main
#	define QSE_ACHAR_IS_MCHAR
	typedef qse_mchar_t qse_achar_t;
#endif

/**
 * The qse_runmain_handler_t type defines the actual function to be
 * executed by qse_runmain(). Unlike the standard main(), it is passed
 * arguments in the #qse_char_t type.
 */
typedef int (*qse_runmain_handler_t) (
	int         argc,
	qse_char_t* argv[]
);

typedef int (*qse_runmainwithenv_handler_t) (
	int         argc,
	qse_char_t* argv[],
	qse_char_t* envp[]
);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The qse_runmain() function helps to invoke a main function independent of
 * the character mode configured for the library.
 */
int qse_runmain (
	int                   argc,
	qse_achar_t*          argv[],
	qse_runmain_handler_t handler
);

/**
 * The qse_runmainwithenv() function helps to invoke a main function 
 * independent of the character mode configured for the library providing
 * the enviroment list.
 */
int qse_runmainwithenv (
	int                          argc,
	qse_achar_t*                 argv[],
	qse_achar_t*                 envp[],
	qse_runmainwithenv_handler_t handler
);

/* TODO: support more weird main functions. for example,
 *  int main(int argc, char **argv, char **envp, char **apple) in Mac OS X */

#ifdef __cplusplus
}
#endif

#endif
