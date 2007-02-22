/*
 * $Id: main.h,v 1.2 2007-02-22 14:32:08 bacon Exp $
 */

#ifndef _ASE_UTL_MAIN_H_
#define _ASE_UTL_MAIN_H_

#include <ase/types.h>
#include <ase/macros.h>

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
