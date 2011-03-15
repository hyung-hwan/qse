/*
 * $Id: conf_msw.h 287 2009-09-15 10:01:02Z hyunghwan.chung $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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

#define QSE_ENDIAN_LITTLE 

#define QSE_SIZEOF_CHAR 1
#define QSE_SIZEOF_SHORT 2
#define QSE_SIZEOF_INT 4

#define QSE_SIZEOF_LONG 4

#if defined(__GNUC__) 
#	define QSE_SIZEOF_LONG_LONG 8
#	define QSE_SIZEOF___INT8 0
#	define QSE_SIZEOF___INT16 0
#	define QSE_SIZEOF___INT32 0
#	define QSE_SIZEOF___INT64 0
#	define QSE_SIZEOF___INT128 0
#else
#	if defined(__WATCOMC__)
#		define QSE_SIZEOF_LONG_LONG 8
#	else
#		define QSE_SIZEOF_LONG_LONG 0
#	endif
#	define QSE_SIZEOF___INT8 1
#	define QSE_SIZEOF___INT16 2
#	define QSE_SIZEOF___INT32 4
#	define QSE_SIZEOF___INT64 8
#	define QSE_SIZEOF___INT128 0
#endif

#define QSE_SIZEOF_VOID_P 4
#define QSE_SIZEOF_FLOAT 4
#define QSE_SIZEOF_DOUBLE 8
#define QSE_SIZEOF_LONG_DOUBLE 16
#define QSE_SIZEOF_WCHAR_T 2

#define QSE_SIZEOF_OFF64_T 0
#define QSE_SIZEOF_OFF_T 8
