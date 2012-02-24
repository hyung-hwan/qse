/*
 * $Id: conf_vms.h 441 2011-04-22 14:28:43Z hyunghwan.chung $
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

/* all of vax, alpha, ia64 are in the little endian. */
#define QSE_ENDIAN_LITTLE 

/*
 * Refer to the chapter 3 of the following URL for the data sizes.
 * http://h71000.www7.hp.com/commercial/c/docs/6180profile.html
 */

#define QSE_SIZEOF_CHAR 1
#define QSE_SIZEOF_SHORT 2
#define QSE_SIZEOF_INT 4
#define QSE_SIZEOF_LONG 4

#if defined(vax) || defined(__vax)
	#define QSE_SIZEOF_LONG_LONG 0
#elif defined(alpha) || defined(__alpha)
	#define QSE_SIZEOF_LONG_LONG 8
#elif defined(ia64) || defined(__ia64)
	#define QSE_SIZEOF_LONG_LONG 8
#else
	#define QSE_SIZEOF_LONG_LONG 0
#endif

#define QSE_SIZEOF___INT8 1
#define QSE_SIZEOF___INT16 2
#define QSE_SIZEOF___INT32 4

#if defined(vax) || defined(__vax)
	#define QSE_SIZEOF___INT64 0
#elif defined(alpha) || defined(__alpha)
	#define QSE_SIZEOF___INT64 8
#elif defined(ia64) || defined(__ia64)
	#define QSE_SIZEOF___INT64 8
#else
	#define QSE_SIZEOF___INT64 0
#endif

#define QSE_SIZEOF___INT128 0

#if defined(vax) || defined(__vax)
	#define QSE_SIZEOF_VOID_P 4
#elif defined(alpha) || defined(__alpha)
	#if __INITIAL_POINTER_SIZE==64
		#pragma pointer_size 64
		#define QSE_SIZEOF_VOID_P 8
	#elif __INITIAL_POINTER_SIZE==32
		#pragma pointer_size 32
		#define QSE_SIZEOF_VOID_P 4
	#elif __INITIAL_POINTER_SIZE==0
		#define QSE_SIZEOF_VOID_P 4
	#else
		#error "unsupported initial pointer size"
	#endif
#elif defined(ia64) || defined(__ia64)
	#if __INITIAL_POINTER_SIZE==64
		#pragma pointer_size 64
		#define QSE_SIZEOF_VOID_P 8
	#elif __INITIAL_POINTER_SIZE==32 
		#pragma pointer_size 32
		#define QSE_SIZEOF_VOID_P 4
	#elif __INITIAL_POINTER_SIZE==0
		#define QSE_SIZEOF_VOID_P 4
	#else
		#error "unsupported initial pointer size"
	#endif
#else
	#error "unsupported architecture"
#endif

#define QSE_SIZEOF_FLOAT 4
#define QSE_SIZEOF_DOUBLE 8

#if defined(vax) || defined(__vax)
	#define QSE_SIZEOF_LONG_DOUBLE 8
#elif defined(alpha) || defined(__alpha)
	#define QSE_SIZEOF_LONG_DOUBLE 16
#elif defined(ia64) || defined(__ia64)
	#define QSE_SIZEOF_LONG_DOUBLE 16
#else
	#define QSE_SIZEOF_LONG_DOUBLE 0
#endif

#define QSE_SIZEOF_WCHAR_T 4


/* make sure you change these when you change 
 * the version in configure.ac */
#define QSE_PACKAGE_VERSION "0.5.6"
#define QSE_PACKAGE_VERSION_MAJOR 0
#define QSE_PACKAGE_VERSION_MINOR 5
#define QSE_PACKAGE_VERSION_PATCH 6

