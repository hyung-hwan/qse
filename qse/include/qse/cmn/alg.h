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

#ifndef _QSE_ALG_H_
#define _QSE_ALG_H_

/** \file
 * This file provides functions for commonly used algorithms.
 */

#if defined(macintosh)
#	include <:qse:types.h>
#	include <:qse:macros.h>
#else
#	include <qse/types.h>
#	include <qse/macros.h>
#endif

/**
 * The qse_search_comper_t type defines a search callback function.
 * The callback function is called by search functions for each comparison
 * performed. It should return 0 if \a ptr1 and \a ptr2 are 
 * euqal, a positive integer if \a ptr1 is greater than \a ptr2, a negative 
 * if \a ptr2 is greater than \a ptr1. Both \a ptr1 and \a ptr2 are 
 * pointers to any two items in the array. \a ctx which is a pointer to 
 * user-defined data passed to a search function is passed to the callback
 * with no modification.
 */
typedef int (*qse_search_comper_t) (
	const void* ptr1, 
	const void* ptr2, 
	void*       ctx
);

/**
 * The qse_sort_comper_t type defines a sort callback function.
 */
typedef qse_search_comper_t qse_sort_comper_t;

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * The qse_bsearch() function performs binary search over a sorted array.
 * It looks for an item matching \a key in an array \a base containing 
 * \a nmemb items each of which is as large as \a size. The comparison
 * function \a comper is invoked with \a key as the first parameter and
 * an item being tested as the second parameter. The \a ctx parameter is
 * passed to \a comper as the third parameter.
 *
 * See the example below:
 * \code
 * static int compstr (const void* s1, const void* s2, void* ctx)
 * {
 * 	return qse_strcmp ((const qse_char_t*)s1, *(const qse_char_t**)s2);
 * }

 * int find (const qse_char_t* name)
 * {
 * 	static const qse_char_t* tgtnames[] =
 * 	{
 * 		QSE_T("awk"),
 *  		QSE_T("cut"),
 *  		QSE_T("ls"),
 *  		QSE_T("rm"),
 * 		QSE_T("sed"),
 *  		QSE_T("tr"),
 *  		QSE_T("watch")
 * 	};
 * 
 * 	const qse_char_t** ptr;
 * 	ptr = (const qse_char_t**) qse_bsearch (
 * 		name, tgtnames, QSE_COUNTOF(tgtnames),
 * 		QSE_SIZEOF(qse_char_t*), QSE_NULL, compstr
 * 	);
 * 	return ptr? (ptr - tgtnames): -1;
 * }
 * \endcode
 */
QSE_EXPORT void* qse_bsearch (
	const void*         key,
	const void*         base,
	qse_size_t          nmemb,
	qse_size_t          size,
	qse_search_comper_t comper,
	void*               ctx
);

/**
 * The qse_lsearch() function performs linear search over an array.
 */
QSE_EXPORT void* qse_lsearch (
	const void*         key,
	const void*         base,
	qse_size_t          nmemb,
	qse_size_t          size,
	qse_search_comper_t comper,
	void*               ctx
);


/**
 * The qse_qsort() function performs quick-sorting over an array.
 */
QSE_EXPORT void qse_qsort (
	void*             base,
	qse_size_t        nmemb,
	qse_size_t        size,
	qse_sort_comper_t comper,
	void*             ctx
);


/**
 * The qse_rand31() function implements Park-Miller's minimal standard
 * 32 bit pseudo-random number generator.
 */
QSE_EXPORT qse_uint32_t qse_rand31 (
	qse_uint32_t seed
);


#if (QSE_SIZEOF_UINT32_T > 0)
/**
 * The qse_randxs32() function implements the xorshift random number generator
 * by George Marsaglia. 
 */
QSE_EXPORT qse_uint32_t qse_randxs32 (
	qse_uint32_t seed
);
#endif

#if (QSE_SIZEOF_UINT64_T > 0)
/**
 * The qse_randxs64() function implements the xorshift random number generator
 * by George Marsaglia. 
 */
QSE_EXPORT qse_uint64_t qse_randxs64 (
	qse_uint64_t seed
);
#endif

#if (QSE_SIZEOF_UINT128_T > 0)
/**
 * The qse_randxs128() function implements the xorshift random number generator
 * by George Marsaglia. 
 */
QSE_EXPORT qse_uint128_t qse_randxs128 (
	qse_uint128_t seed
);
#endif

#if (QSE_SIZEOF_ULONG_T == QSE_SIZEOF_UINT128_T)
#	define qse_randxsulong(seed) qse_randxs128(seed)
#elif (QSE_SIZEOF_ULONG_T == QSE_SIZEOF_UINT64_T)
#	define qse_randxsulong(seed) qse_randxs64(seed)
#elif (QSE_SIZEOF_ULONG_T == QSE_SIZEOF_UINT32_T)
#	define qse_randxsulong(seed) qse_randxs32(seed)
#else
#	error Unsupported
#endif

#if (QSE_SIZEOF_UINT_T == QSE_SIZEOF_UINT128_T)
#	define qse_randxsuint(seed) qse_randxs128(seed)
#elif (QSE_SIZEOF_UINT_T == QSE_SIZEOF_UINT64_T)
#	define qse_randxsuint(seed) qse_randxs64(seed)
#elif (QSE_SIZEOF_UINT_T == QSE_SIZEOF_UINT32_T)
#	define qse_randxsuint(seed) qse_randxs32(seed)
#else
#	error Unsupported
#endif

#if (QSE_SIZEOF_UINTMAX_T == QSE_SIZEOF_UINT128_T)
#	define qse_randxsuintmax(seed) qse_randxs128(seed)
#elif (QSE_SIZEOF_UINTMAX_T == QSE_SIZEOF_UINT64_T)
#	define qse_randxsuintmax(seed) qse_randxs64(seed)
#elif (QSE_SIZEOF_UINTMAX_T == QSE_SIZEOF_UINT32_T)
#	define qse_randxsuintmax(seed) qse_randxs32(seed)
#else
#	error Unsupported
#endif

QSE_EXPORT qse_size_t qse_enbase64 (
	const void*        in,
	qse_size_t         isz,
	qse_mchar_t*       out,
	qse_size_t         osz,
	qse_size_t*        xsz
);

QSE_EXPORT qse_size_t qse_debase64 (
	const qse_mchar_t* in,
	qse_size_t         isz,
	void*              out,
	qse_size_t         osz,
	qse_size_t*        xsz
);

#if defined(__cplusplus)
}
#endif

#endif
