/*
 * $Id: main.h,v 1.3 2007-03-06 14:16:53 bacon Exp $
 */

#ifndef _ASE_UTL_MAIN_H_
#define _ASE_UTL_MAIN_H_

#include <ase/cmn/types.h>
#include <ase/cmn/macros.h>

#if defined(_WIN32)

	#include <tchar.h>
	#define ase_main _tmain

#else

	#ifdef __cplusplus
	extern "C" { int ase_main (...); }
	#else
	extern int ase_main ();
	#endif

#endif

#endif
