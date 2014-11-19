/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

#if defined(_WIN32) && defined(_MSC_VER)
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

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * The qse_runmain() function helps to invoke a main function independent of
 * the character mode configured for the library.
 */
QSE_EXPORT int qse_runmain (
	int                   argc,
	qse_achar_t*          argv[],
	qse_runmain_handler_t handler
);

/**
 * The qse_runmainwithenv() function helps to invoke a main function 
 * independent of the character mode configured for the library providing
 * the enviroment list.
 */
QSE_EXPORT int qse_runmainwithenv (
	int                          argc,
	qse_achar_t*                 argv[],
	qse_achar_t*                 envp[],
	qse_runmainwithenv_handler_t handler
);

/* TODO: support more weird main functions. for example,
 *  int main(int argc, char **argv, char **envp, char **apple) in Mac OS X */

#if defined(__cplusplus)
}
#endif

#endif
