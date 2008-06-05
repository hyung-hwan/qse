/*
 * $Id: conf_msw.h 182 2008-06-03 08:17:42Z baconevi $
 *
 * {License}
 */

/*
Macro Meaning 
_WIN64 A 64-bit platform. 
_WIN32 A 32-bit platform.  This value is also defined by the 64-bit 
       compiler for backward compatibility. 
_WIN16 A 16-bit platform 

The following macros are specific to the architecture.

Macro Meaning 
_M_IA64 Intel Itanium Processor Family 
_M_IX86 x86 platform 
_M_X64 x64 platform 
*/

#define ASE_ENDIAN_LITTLE 

#define ASE_SIZEOF_CHAR 1
#define ASE_SIZEOF_SHORT 2
#define ASE_SIZEOF_INT 4

/*#ifdef _WIN64
	#define ASE_SIZEOF_LONG 8
#else*/
	#define ASE_SIZEOF_LONG 4
/*#endif*/

#if defined(__POCC__) || defined(__DMC__)
	/* pelles c with no microsoft extension */
	#define ASE_SIZEOF_LONG_LONG 8

	#define ASE_SIZEOF___INT8 0
	#define ASE_SIZEOF___INT16 0
	#define ASE_SIZEOF___INT32 0
	#define ASE_SIZEOF___INT64 0
	#define ASE_SIZEOF___INT96 0
	#define ASE_SIZEOF___INT128 0
#else
	#define ASE_SIZEOF_LONG_LONG 0

	#define ASE_SIZEOF___INT8 1
	#define ASE_SIZEOF___INT16 2
	#define ASE_SIZEOF___INT32 4
	#define ASE_SIZEOF___INT64 8
	#define ASE_SIZEOF___INT96 0
	#define ASE_SIZEOF___INT128 0
#endif

#ifdef _WIN64
	#define ASE_SIZEOF_VOID_P 8
#else
	#define ASE_SIZEOF_VOID_P 4
#endif

#define ASE_SIZEOF_FLOAT 4
#define ASE_SIZEOF_DOUBLE 8
#define ASE_SIZEOF_LONG_DOUBLE 16
#define ASE_SIZEOF_WCHAR_T 2
