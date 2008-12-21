/*
 * $Id: main.h 463 2008-12-09 06:52:03Z baconevi $
 * 
 * {License}
 */

#ifndef _ASE_UTL_MAIN_H_
#define _ASE_UTL_MAIN_H_

#include <ase/types.h>
#include <ase/macros.h>

#if defined(_WIN32) && !defined(__MINGW32__)
	#if defined(ASE_CHAR_IS_MCHAR)
		#define ase_main main
		typedef ase_mchar_t ase_achar_t;
	#else
		#define ase_main wmain
		typedef ase_wchar_t ase_achar_t;
	#endif
#else
	#define ase_main main
	typedef ase_mchar_t ase_achar_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

int ase_runmain (int argc, ase_achar_t* argv[], int(*mf) (int,ase_char_t*[]));
/* TODO - ase_runmain with env */

#ifdef __cplusplus
}
#endif

#endif
