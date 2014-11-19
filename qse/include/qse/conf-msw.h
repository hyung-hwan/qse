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

/* windows for most of non-x86 platforms dropped. 
 * make it selective to support old non-x86 windows platforms. */
#define QSE_ENDIAN_LITTLE 

/*
 * You must define which character type to use as a default character here.
 *
 * #define QSE_CHAR_IS_WCHAR
 * #define QSE_CHAR_IS_MCHAR
 */

#if defined(__WATCOMC__)
#	define QSE_SIZEOF_CHAR        1
#	define QSE_SIZEOF_SHORT       2
#	define QSE_SIZEOF_INT         4
#	define QSE_SIZEOF_LONG        4
#	if (__WATCOMC__ < 1200)
#		define QSE_SIZEOF_LONG_LONG   0
#	else
#		define QSE_SIZEOF_LONG_LONG   8
#	endif

#	if defined(_WIN64)
#		define QSE_SIZEOF_VOID_P      8
#	else
#		define QSE_SIZEOF_VOID_P      4
#	endif
#	define QSE_SIZEOF_FLOAT       4
#	define QSE_SIZEOF_DOUBLE      8
#	define QSE_SIZEOF_LONG_DOUBLE 8
#	define QSE_SIZEOF_WCHAR_T     2

#	define QSE_SIZEOF___INT8      1
#	define QSE_SIZEOF___INT16     2
#	define QSE_SIZEOF___INT32     4
#	define QSE_SIZEOF___INT64     8
#	define QSE_SIZEOF___INT128    0

#	define QSE_SIZEOF_OFF64_T     0
#	define QSE_SIZEOF_OFF_T       8

#	define QSE_SIZEOF_MBSTATE_T   QSE_SIZEOF_LONG
#	define QSE_MBLEN_MAX          16

	/* these two have only to be large enough */
#	define QSE_SIZEOF_STRUCT_SOCKADDR_IN 32
#	define QSE_SIZEOF_STRUCT_SOCKADDR_IN6 64
#	define QSE_SIZEOF_SOCKLEN_T 4

#	if !defined(QSE_CHAR_IS_WCHAR) && !defined(QSE_CHAR_IS_MCHAR)
#		define QSE_CHAR_IS_WCHAR      1
#	endif

	/* old watcom c/c++ compiler requires this */
#	if (__WATCOMC__ < 1200)
#		undef QSE_ENABLE_BUNDLED_UNICODE
#		define QSE_ENABLE_BUNDLED_UNICODE 1
#	endif

#elif defined(__GNUC__) || defined(__DMC__) || defined(__POCC__)
#	define QSE_SIZEOF_CHAR        1
#	define QSE_SIZEOF_SHORT       2
#	define QSE_SIZEOF_INT         4
#	define QSE_SIZEOF_LONG        4
#	define QSE_SIZEOF_LONG_LONG   8

#	if defined(_WIN64)
#		define QSE_SIZEOF_VOID_P      8
#	else
#		define QSE_SIZEOF_VOID_P      4
#	endif
#	define QSE_SIZEOF_FLOAT       4
#	define QSE_SIZEOF_DOUBLE      8
#	define QSE_SIZEOF_LONG_DOUBLE 16
#	define QSE_SIZEOF_WCHAR_T     2

#	define QSE_SIZEOF___INT8      0
#	define QSE_SIZEOF___INT16     0
#	define QSE_SIZEOF___INT32     0
#	define QSE_SIZEOF___INT64     0
#	define QSE_SIZEOF___INT128    0

#	define QSE_SIZEOF_OFF64_T     0
#	define QSE_SIZEOF_OFF_T       8

#	define QSE_SIZEOF_MBSTATE_T   QSE_SIZEOF_LONG
#	define QSE_MBLEN_MAX          16

	/* these two have only to be large enough */
#	define QSE_SIZEOF_STRUCT_SOCKADDR_IN 32
#	define QSE_SIZEOF_STRUCT_SOCKADDR_IN6 64

#	if !defined(QSE_CHAR_IS_WCHAR) && !defined(QSE_CHAR_IS_MCHAR)
#		define QSE_CHAR_IS_WCHAR      1
#	endif

#elif defined(_MSC_VER)
#	define QSE_SIZEOF_CHAR        1
#	define QSE_SIZEOF_SHORT       2
#	define QSE_SIZEOF_INT         4
#	define QSE_SIZEOF_LONG        4
#	if (_MSC_VER>=1310)
#		define QSE_SIZEOF_LONG_LONG   8
#	else
#		define QSE_SIZEOF_LONG_LONG   0
#	endif

#	if defined(_WIN64)
#		define QSE_SIZEOF_VOID_P      8
#	else
#		define QSE_SIZEOF_VOID_P      4
#	endif
#	define QSE_SIZEOF_FLOAT       4
#	define QSE_SIZEOF_DOUBLE      8
#	define QSE_SIZEOF_LONG_DOUBLE 8
#	define QSE_SIZEOF_WCHAR_T     2

#	define QSE_SIZEOF___INT8      1
#	define QSE_SIZEOF___INT16     2
#	define QSE_SIZEOF___INT32     4
#	define QSE_SIZEOF___INT64     8
#	define QSE_SIZEOF___INT128    0

#	define QSE_SIZEOF_OFF64_T     0
#	define QSE_SIZEOF_OFF_T       8

#	define QSE_SIZEOF_MBSTATE_T   QSE_SIZEOF_LONG
#	define QSE_MBLEN_MAX          8

	/* these two have only to be large enough */
#	define QSE_SIZEOF_STRUCT_SOCKADDR_IN 32
#	define QSE_SIZEOF_STRUCT_SOCKADDR_IN6 64
#	define QSE_SIZEOF_SOCKLEN_T 4

#	if !defined(QSE_CHAR_IS_WCHAR) && !defined(QSE_CHAR_IS_MCHAR)
#		define QSE_CHAR_IS_WCHAR      1
#	endif

#elif defined(__BORLANDC__)

#	define QSE_SIZEOF_CHAR        1
#	define QSE_SIZEOF_SHORT       2
#	define QSE_SIZEOF_INT         4
#	define QSE_SIZEOF_LONG        4
#	define QSE_SIZEOF_LONG_LONG   0

#	if defined(_WIN64)
#		define QSE_SIZEOF_VOID_P      8
#	else
#		define QSE_SIZEOF_VOID_P      4
#	endif
#	define QSE_SIZEOF_FLOAT       4
#	define QSE_SIZEOF_DOUBLE      8
#	define QSE_SIZEOF_LONG_DOUBLE 8
#	define QSE_SIZEOF_WCHAR_T     2

#	define QSE_SIZEOF___INT8      1
#	define QSE_SIZEOF___INT16     2
#	define QSE_SIZEOF___INT32     4
#	define QSE_SIZEOF___INT64     8
#	define QSE_SIZEOF___INT128    0

#	define QSE_SIZEOF_OFF64_T     0
#	define QSE_SIZEOF_OFF_T       8

#	define QSE_SIZEOF_MBSTATE_T   QSE_SIZEOF_LONG
#	define QSE_MBLEN_MAX          8

	/* these two have only to be large enough */
#	define QSE_SIZEOF_STRUCT_SOCKADDR_IN 32
#	define QSE_SIZEOF_STRUCT_SOCKADDR_IN6 64
#	define QSE_SIZEOF_SOCKLEN_T 4

#	if !defined(QSE_CHAR_IS_WCHAR) && !defined(QSE_CHAR_IS_MCHAR)
#		define QSE_CHAR_IS_WCHAR      1
#	endif

#else
#	error Define the size of various data types.
#endif

#include <qse/conf-inf.h>
