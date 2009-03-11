/*
 * $Id: main.h 463 2008-12-09 06:52:03Z baconevi $
 *
 * {License}
 */

#ifndef _QSE_UTL_MAIN_H_
#define _QSE_UTL_MAIN_H_

#include <qse/types.h>
#include <qse/macros.h>

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
