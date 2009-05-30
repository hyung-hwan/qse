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

#ifndef _QSE_UTL_MAIN_H_
#define _QSE_UTL_MAIN_H_

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
