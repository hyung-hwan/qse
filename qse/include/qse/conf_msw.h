/*
 * $Id: conf_msw.h 561 2011-09-07 07:17:05Z hyunghwan.chung $
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
#	define QSE_SIZEOF_LONG_LONG   8
#
#	if defined(_WIN64)
#		define QSE_SIZEOF_VOID_P      8
#	else
#		define QSE_SIZEOF_VOID_P      4
#	endif
#	define QSE_SIZEOF_FLOAT       4
#	define QSE_SIZEOF_DOUBLE      8
#	define QSE_SIZEOF_LONG_DOUBLE 8
#	define QSE_SIZEOF_WCHAR_T     2
#
#	define QSE_SIZEOF___INT8      1
#	define QSE_SIZEOF___INT16     2
#	define QSE_SIZEOF___INT32     4
#	define QSE_SIZEOF___INT64     8
#	define QSE_SIZEOF___INT128    0
#
#	define QSE_SIZEOF_OFF64_T     0
#	define QSE_SIZEOF_OFF_T       8
#
#	define QSE_SIZEOF_MBSTATE_T   QSE_SIZEOF_LONG
#	define QSE_MBLEN_MAX          16
#
#	define QSE_CHAR_IS_WCHAR
#elif defined(__GNUC__) || defined(__DMC__) || defined(__POCC__)
#	define QSE_SIZEOF_CHAR        1
#	define QSE_SIZEOF_SHORT       2
#	define QSE_SIZEOF_INT         4
#	define QSE_SIZEOF_LONG        4
#	define QSE_SIZEOF_LONG_LONG   8
#
#	if defined(_WIN64)
#		define QSE_SIZEOF_VOID_P      8
#	else
#		define QSE_SIZEOF_VOID_P      4
#	endif
#	define QSE_SIZEOF_FLOAT       4
#	define QSE_SIZEOF_DOUBLE      8
#	define QSE_SIZEOF_LONG_DOUBLE 16
#	define QSE_SIZEOF_WCHAR_T     2
#
#	define QSE_SIZEOF___INT8      0
#	define QSE_SIZEOF___INT16     0
#	define QSE_SIZEOF___INT32     0
#	define QSE_SIZEOF___INT64     0
#	define QSE_SIZEOF___INT128    0
#
#	define QSE_SIZEOF_OFF64_T     0
#	define QSE_SIZEOF_OFF_T       8
#
#	define QSE_SIZEOF_MBSTATE_T   QSE_SIZEOF_LONG
#	define QSE_MBLEN_MAX          16
#
#	define QSE_CHAR_IS_WCHAR
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
#
#	if defined(_WIN64)
#		define QSE_SIZEOF_VOID_P      8
#	else
#		define QSE_SIZEOF_VOID_P      4
#	endif
#	define QSE_SIZEOF_FLOAT       4
#	define QSE_SIZEOF_DOUBLE      8
#	define QSE_SIZEOF_LONG_DOUBLE 8
#	define QSE_SIZEOF_WCHAR_T     2
#
#	define QSE_SIZEOF___INT8      1
#	define QSE_SIZEOF___INT16     2
#	define QSE_SIZEOF___INT32     4
#	define QSE_SIZEOF___INT64     8
#	define QSE_SIZEOF___INT128    0
#
#	define QSE_SIZEOF_OFF64_T     0
#	define QSE_SIZEOF_OFF_T       8
#
#	define QSE_SIZEOF_MBSTATE_T   QSE_SIZEOF_LONG
#	define QSE_MBLEN_MAX          8
#
#	define QSE_CHAR_IS_WCHAR
#else
#	error Define the size of various data types.
#endif
