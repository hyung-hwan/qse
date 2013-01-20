/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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

/* This file is for class Mac OS */

/* Mac OS on PPC and m68k uses the big endian mode */
#define QSE_ENDIAN_BIG 

/*
 * You must define which character type to use as a default character here.
 *
 * #define QSE_CHAR_IS_WCHAR
 * #define QSE_CHAR_IS_MCHAR
 */

#if defined(__MWERKS__)
#	define QSE_SIZEOF_CHAR        1
#	define QSE_SIZEOF_SHORT       2
#	define QSE_SIZEOF_INT         4
#	define QSE_SIZEOF_LONG        4
#	define QSE_SIZEOF_LONG_LONG   8
#	define QSE_SIZEOF_VOID_P      4
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

#	if !defined(QSE_CHAR_IS_WCHAR) && !defined(QSE_CHAR_IS_MCHAR)
#		define QSE_CHAR_IS_WCHAR      1
#	endif

#	undef QSE_ENABLE_BUNDLED_UNICODE
#	define QSE_ENABLE_BUNDLED_UNICODE 1

#else
#	error Define the size of various data types.
#endif

#include <:qse:conf-inf.h>
