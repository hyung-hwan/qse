/*
 * $Id: conf_vms.h 441 2011-04-22 14:28:43Z hyunghwan.chung $
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
#define QSE_CHAR_IS_WCHAR
 
#if defined(vax) || defined(__vax)
#	define QSE_SIZEOF_OFF_T 4
#elif defined(_LARGEFILE)
#	define QSE_SIZEOF_OFF_T 8
#else
#	define QSE_SIZEOF_OFF_T 4
#endif

#define QSE_SIZEOF_OFF64_T     0
#define QSE_SIZEOF_MBSTATE_T   24 
#define QSE_MBLEN_MAX          8

/* these two have only to be large enough */
#define QSE_SIZEOF_STRUCT_SOCKADDR_IN 32
#define QSE_SIZEOF_STRUCT_SOCKADDR_IN6 64
#define QSE_SIZEOF_SOCKLEN_T 4

#if !defined(QSE_CHAR_IS_WCHAR) && !defined(QSE_CHAR_IS_MCHAR)
#	define QSE_CHAR_IS_WCHAR      1
#endif

#undef QSE_ENABLE_BUNDLED_UNICODE
#define QSE_ENABLE_BUNDLED_UNICODE 1

#include <qse/conf-inf.h>
