/*
 * $Id: main.h 116 2008-03-03 11:15:37Z baconevi $
 */

#ifndef _ASE_UTL_MAIN_H_
#define _ASE_UTL_MAIN_H_

#include <ase/cmn/types.h>
#include <ase/cmn/macros.h>

#if defined(_WIN32)
	#include <tchar.h>
	#define ase_main _tmain
	typedef ase_char_t ase_achar_t;
#else
	#define ase_main main
	typedef ase_mchar_t ase_achar_t;
#endif


#ifdef __cplusplus
extern "C" {
#endif

int ase_runmain (int argc, ase_achar_t* argv[], int(*mf) (int,ase_char_t*[]));

#ifdef __cplusplus
}
#endif

#endif
