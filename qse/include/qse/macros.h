/*
 * $Id: macros.h 441 2011-04-22 14:28:43Z hyunghwan.chung $
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

#ifndef _QSE_MACROS_H_
#define _QSE_MACROS_H_

#if defined(macintosh)
#	include <:qse:types.h>
#else
#	include <qse/types.h>
#endif

/** \file
 * This file contains various useful macro definitions.
 */

#if defined(__cplusplus) || (defined(__STDC_VERSION__) && (__STDC_VERSION__>=199901L))
#	define QSE_INLINE inline
#	define QSE_HAVE_INLINE
#elif defined(__GNUC__) && defined(__GNUC_GNU_INLINE__)
	/* gcc disables inline when -std=c89 or -ansi is used. 
	 * so use __inline__ supported by gcc regardless of the options */
#	define QSE_INLINE /*extern*/ __inline__
#	define QSE_HAVE_INLINE
#else
#	define QSE_INLINE 
#	undef QSE_HAVE_INLINE
#endif

#if defined(__GNUC__) && defined(__GNUC_GNU_INLINE__)
#	define QSE_INLINE_ALWAYS __inline__ __attribute__((__always_inline__))
#	define QSE_HAVE_INLINE_ALWAYS
#elif defined(_MSC_VER) || (defined(__CC_ARM) || defined(__ARMCC__))
#	define QSE_INLINE_ALWAYS __forceinline
#	define QSE_HAVE_INLINE_ALWAYS
#else
#	define QSE_INLINE_ALWAYS QSE_INLINE
#	undef QSE_HAVE_INLINE_ALWAYS
#endif

#if defined(__GNUC__) && defined(__GNUC_GNU_INLINE__)
#	define QSE_INLINE_NEVER __inline__ __attribute__((__noinline__))
#	define QSE_HAVE_INLINE_NEVER
#elif (defined(__CC_ARM) || defined(__ARMCC__))
#	define QSE_INLINE_NEVER __declspec(noinline)
#	define QSE_HAVE_INLINE_NEVER
#else
#	define QSE_INLINE_NEVER 
#	undef QSE_HAVE_INLINE_NEVER
#endif

#if defined(_WIN32) || (defined(__WATCOMC__) && !defined(__WINDOWS_386__))
#	define QSE_IMPORT __declspec(dllimport)
#	define QSE_EXPORT __declspec(dllexport)
#	define QSE_PRIVATE 
#elif defined(__GNUC__) && ((__GNUC__>= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3))
#	define QSE_IMPORT __attribute__((visibility("default")))
#	define QSE_EXPORT __attribute__((visibility("default")))
#	define QSE_PRIVATE __attribute__((visibility("hidden")))
/*#	define QSE_PRIVATE __attribute__((visibility("internal")))*/
#else
#	define QSE_IMPORT
#	define QSE_EXPORT
#	define QSE_PRIVATE
#endif

#if defined(__GNUC__)
#	define QSE_OPTIMIZE_NEVER __attribute__((optimize("O0")))
#else
#	define QSE_OPTIMIZE_NEVER 
#endif

/**
 * The QSE_SIZEOF() macro gets data size in bytes. It is equivalent to the
 * sizeof operator. The following code snippet should print sizeof(int)*128.
 * \code
 * int x[128];
 * printf ("%d\n", (int)QSE_SIZEOF(x));
 * \endcode
 */
#define QSE_SIZEOF(n)  (sizeof(n))

/**
 * The QSE_COUNTOF() macro returns the number elements in an array.
 * The following code snippet should print 128.
 * \code
 * int x[128];
 * printf ("%d\n", (int)QSE_COUNTOF(x));
 * \endcode
 */
#define QSE_COUNTOF(n) (sizeof(n)/sizeof((n)[0]))

/**
 * The QSE_OFFSETOF() macro returns the offset of a field from the beginning
 * of a structure.
 */
#define QSE_OFFSETOF(type,member) ((qse_size_t)&((type*)0)->member)

/**
 * The QSE_ALIGNOF() macro returns the alignment size of a structure.
 * Note that this macro may not work reliably depending on the type given.
 */
#define QSE_ALIGNOF(type) QSE_OFFSETOF(struct { qse_uint8_t d1; type d2; }, d2)
	/*(sizeof(struct { qse_uint8_t d1; type d2; }) - sizeof(type))*/

/**
 * The QSE_ALIGNTO() macro rounds up a positive integer to the nearest
 * multiple of 'align'.
 */
#define QSE_ALIGNTO(num,align) ((((num) + (align) - 1) / (align)) * (align))

#if 0
/**
 * Round up a number, both positive and negative, to the nearest multiple of 'align' 
 */
#define QSE_ALIGNTO(num,align) ((((num) + (num >= 0? 1: -1) * (align) - 1) / (align)) * (align))
#endif

/**
 * The QSE_ALIGNTO_POW2() macro rounds up a positive integer to to the 
 * nearest multiple of 'align' which should be a multiple of a power of 2
 */
#define QSE_ALIGNTO_POW2(num,align) ((((num) + (align) - 1)) & ~((align) - 1))

#define QSE_IS_UNALIGNED_POW2(num,align) ((num) & ((align) - 1))
#define QSE_IS_ALIGNED_POW2(num,align) (!QSE_IS_UNALIGNED_POW2(num,align))

/**
 * The QSE_TYPE_IS_SIGNED() macro determines if a type is signed. 
 * \code
 * printf ("%d\n", (int)QSE_TYPE_IS_SIGNED(int));
 * printf ("%d\n", (int)QSE_TYPE_IS_SIGNED(unsigned int));
 * \endcode
 */
#define QSE_TYPE_IS_SIGNED(type) (((type)0) > ((type)-1))

/**
 * The QSE_TYPE_IS_SIGNED() macro determines if a type is unsigned. 
 * \code
 * printf ("%d\n", QSE_TYPE_IS_UNSIGNED(int));
 * printf ("%d\n", QSE_TYPE_IS_UNSIGNED(unsigned int));
 * \endcode
 */
#define QSE_TYPE_IS_UNSIGNED(type) (((type)0) < ((type)-1))

#define QSE_TYPE_SIGNED_MAX(type) \
	((type)~((type)1 << (QSE_SIZEOF(type) * 8 - 1)))
#define QSE_TYPE_UNSIGNED_MAX(type) ((type)(~(type)0))

#define QSE_TYPE_SIGNED_MIN(type) \
	((type)((type)1 << (QSE_SIZEOF(type) * 8 - 1)))
#define QSE_TYPE_UNSIGNED_MIN(type) ((type)0)

#define QSE_TYPE_MAX(type) \
	((QSE_TYPE_IS_SIGNED(type)? QSE_TYPE_SIGNED_MAX(type): QSE_TYPE_UNSIGNED_MAX(type)))
#define QSE_TYPE_MIN(type) \
	((QSE_TYPE_IS_SIGNED(type)? QSE_TYPE_SIGNED_MIN(type): QSE_TYPE_UNSIGNED_MIN(type)))

/** 
 * The #QSE_NULL macro defines a special pointer value to indicate an error or
 * that it does not point to anything.
 */
#if defined(__cplusplus)
#	if (__cplusplus >= 201103L) /* C++11 */
#		define QSE_NULL nullptr
#	elif QSE_SIZEOF_VOID_P == QSE_SIZEOF_INT
#		define QSE_NULL (0)
#	elif QSE_SIZEOF_VOID_P == QSE_SIZEOF_LONG
#		define QSE_NULL (0l)
#	elif QSE_SIZEOF_VOID_P == QSE_SIZEOF_LONG_LONG
#		define QSE_NULL (0ll)
#	else
#		define QSE_NULL (0)
#	endif
#else
#	define QSE_NULL ((void*)0)
#endif

/**
 * The QSE_MCHAR_EOF macro defines an EOF character.
 */
#define QSE_MCHAR_EOF ((qse_mcint_t)-1)

/**
 * The QSE_WCHAR_EOF macro defines an EOF character.
 */
#define QSE_WCHAR_EOF ((qse_wcint_t)-1)

/**
 * The QSE_CHAR_EOF macro defines an EOF character.
 */
#define QSE_CHAR_EOF  ((qse_cint_t)-1)

/**
 * The QSE_BLOCK macro encloses one or more statements in a block with 
 * no side-effect.
 */
#define QSE_BLOCK(code) do { code } while(0)

#define QSE_IS_POWOF2(x) (((x) & ((x) - 1)) == 0)

#define QSE_SWAP(x,y,original_type,casting_type) \
	QSE_BLOCK ( \
		x = (original_type)((casting_type)(x) ^ (casting_type)(y)); \
		y = (original_type)((casting_type)(y) ^ (casting_type)(x)); \
		x = (original_type)((casting_type)(x) ^ (casting_type)(y)); \
	)

#define QSE_ABS(x) ((x) < 0? -(x): (x))

#define QSE_THROW_ERR(id) goto  __err_ ## id 
#define QSE_CATCH_ERR(id) while(0) __err_ ## id:

#define QSE_CONTINUE_LOOP(id) goto __loop_ ## id ## _begin__
#define QSE_BREAK_LOOP(id)    goto __loop_ ## id ## _end__
#define QSE_BEGIN_LOOP(id)    __loop_ ## id ## _begin__: {
#define QSE_END_LOOP(id)      QSE_LOOP_CONTINUE(id) } __loop_ ## id ## _end__:

#define QSE_REPEAT(n,blk) \
	QSE_BLOCK ( \
		qse_size_t __qse_repeat_x1__ = (qse_size_t)(n); \
		qse_size_t __qse_repeat_x2__ = __qse_repeat_x1__ >> 4; \
		__qse_repeat_x1__ &= 15; \
		while (__qse_repeat_x1__-- > 0) { blk; } \
		while (__qse_repeat_x2__-- > 0) { \
			blk; blk; blk; blk; blk; blk; blk; blk; \
			blk; blk; blk; blk; blk; blk; blk; blk; \
		} \
	)

/**
 * The QSE_FV() macro is used to specify a initial value
 * for a field of an aggregate type.
 */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
#	define QSE_FV(field,value) field = value
#else
#	define QSE_FV(field,value) value
#endif

/* number of characters to number of bytes */
#define QSE_NCTONB(x) ((x)*sizeof(qse_char_t))
/* number of bytes to number of characters */
#define QSE_NBTONC(x) ((x)/sizeof(qse_char_t))

#define QSE_MQ_I(val) #val
#define QSE_MQ(val)   QSE_MQ_I(val)
/** 
 * The #QSE_MT macro maps a multi-byte literal string literal as it is. 
 */
#define QSE_MT(txt)   (txt)

#if defined(QSE_WCHAR_IS_CHAR16_T)
#	define QSE_WQ_I(val)  (u ## #val)
#	define QSE_WQ(val)    QSE_WQ_I(val)
#else
#	define QSE_WQ_I(val)  (L ## #val)
#	define QSE_WQ(val)    QSE_WQ_I(val)
#endif

/** 
 * The #QSE_WT macro maps a multi-byte literal string to a wide character 
 * string by prefixing it with \b L.
 */
#if (QSE_SIZEOF_WCHAR_T == QSE_SIZEOF_MCHAR_T)
#	define QSE_WT(txt)    (txt)
#elif defined(QSE_WCHAR_IS_CHAR16_T)
#	define QSE_WT(txt)    (u ## txt)
#else
#	define QSE_WT(txt)    (L ## txt)
#endif

/** \def QSE_T
 * The #QSE_T macro maps to #QSE_MT if #QSE_CHAR_IS_MCHAR is defined, and to
 * #QSE_WT if #QSE_CHAR_IS_WCHAR is defined.
 */
#if defined(QSE_CHAR_IS_MCHAR)
#	define QSE_Q(val) QSE_MQ(val)
#	define QSE_T(txt) QSE_MT(txt)
#else
#	define QSE_Q(val) QSE_WQ(val)
#	define QSE_T(txt) QSE_WT(txt)
#endif

#if defined(__GNUC__)
#	define QSE_BEGIN_PACKED_STRUCT(x) struct x {
#	define QSE_END_PACKED_STRUCT() } __attribute__((packed));
#else
#	define QSE_BEGIN_PACKED_STRUCT(x) struct x {
#	define QSE_END_PACKED_STRUCT() };
#endif

/**
 * The QSE_STRUCT_FIELD() macro provides a neutral way to specify
 * a field ID for initialzing a structure  in both C9X and C89
 */
#if (defined(__STDC_VERSION__) && (__STDC_VERSION__>=199901L)) || defined(__GNUC__)
#	define QSE_STRUCT_FIELD(id,value) .id = value
#else
#	define QSE_STRUCT_FIELD(id,value) value
#endif


/**
 * The QSE_XTN() macro is a convenience macro to retrieve the pointer to 
 * extension space located at the end of an object. The type of the object
 * should be known in advance for it to work properly.
 */
#define QSE_XTN(obj) ((void*)(obj + 1))

/* ---------------------------------------------------------------------- 
 * ASSERTION 
 * ---------------------------------------------------------------------- */

#ifdef NDEBUG
#	define QSE_ASSERT(expr) ((void)0)
#	define QSE_ASSERTX(expr,desc) ((void)0)
#else

#	define QSE_ASSERT(expr) (void)((expr) || \
		(qse_assert_failed (QSE_T(#expr), QSE_NULL, QSE_T(__FILE__), __LINE__), 0))
#	define QSE_ASSERTX(expr,desc) (void)((expr) || \
		(qse_assert_failed (QSE_T(#expr), QSE_T(desc), QSE_T(__FILE__), __LINE__), 0))
#endif


#if defined(__cplusplus)
extern "C" {
#endif
QSE_EXPORT void qse_assert_failed (
	const qse_char_t* expr, const qse_char_t* desc,
	const qse_char_t* file, qse_size_t line);
#if defined(__cplusplus)
}
#endif


/* ---------------------------------------------------------------------- 
 * INTEGER LIMITS
 * ---------------------------------------------------------------------- */
#if defined(QSE_HAVE_UINT128_T)
#define QSE_UINT128_MAX QSE_TYPE_UNSIGNED_MAX(qse_uint128_t)
#define QSE_UINT128_MIN QSE_TYPE_UNSIGNED_MIN(qse_uint128_t)
#define QSE_INT128_MAX QSE_TYPE_SIGNED_MAX(qse_int128_t)
#define QSE_INT128_MIN QSE_TYPE_SIGNED_MIN(qse_int128_t)
#endif

#if defined(QSE_HAVE_UINT64_T)
#define QSE_UINT64_MAX QSE_TYPE_UNSIGNED_MAX(qse_uint64_t)
#define QSE_UINT64_MIN QSE_TYPE_UNSIGNED_MIN(qse_uint64_t)
#define QSE_INT64_MAX QSE_TYPE_SIGNED_MAX(qse_int64_t)
#define QSE_INT64_MIN QSE_TYPE_SIGNED_MIN(qse_int64_t)
#endif

#define QSE_UINT32_MAX QSE_TYPE_UNSIGNED_MAX(qse_uint32_t)
#define QSE_UINT32_MIN QSE_TYPE_UNSIGNED_MIN(qse_uint32_t)
#define QSE_INT32_MAX QSE_TYPE_SIGNED_MAX(qse_int32_t)
#define QSE_INT32_MIN QSE_TYPE_SIGNED_MIN(qse_int32_t)

#define QSE_UINT16_MAX QSE_TYPE_UNSIGNED_MAX(qse_uint16_t)
#define QSE_UINT16_MIN QSE_TYPE_UNSIGNED_MIN(qse_uint16_t)
#define QSE_INT16_MAX QSE_TYPE_SIGNED_MAX(qse_int16_t)
#define QSE_INT16_MIN QSE_TYPE_SIGNED_MIN(qse_int16_t)

#define QSE_UINT8_MAX QSE_TYPE_UNSIGNED_MAX(qse_uint8_t)
#define QSE_UINT8_MIN QSE_TYPE_UNSIGNED_MIN(qse_uint8_t)
#define QSE_INT8_MAX QSE_TYPE_SIGNED_MAX(qse_int8_t)
#define QSE_INT8_MIN QSE_TYPE_SIGNED_MIN(qse_int8_t)

/* ---------------------------------------------------------------------- 
 * BIT MANIPULATIONS
 * ---------------------------------------------------------------------- */
#if defined(QSE_HAVE_UINT64_T)
#define QSE_FETCH64BE(ptr) \
	(((qse_uint64_t)(((qse_uint8_t*)(ptr))[0]) << 56) | ((qse_uint64_t)(((qse_uint8_t*)(ptr))[1]) << 48) | \
	 ((qse_uint64_t)(((qse_uint8_t*)(ptr))[2]) << 40) | ((qse_uint64_t)(((qse_uint8_t*)(ptr))[3]) << 32) | \
	 ((qse_uint64_t)(((qse_uint8_t*)(ptr))[4]) << 24) | ((qse_uint64_t)(((qse_uint8_t*)(ptr))[5]) << 16) | \
	 ((qse_uint64_t)(((qse_uint8_t*)(ptr))[6]) << 8)  | ((qse_uint64_t)(((qse_uint8_t*)(ptr))[7])))

#define QSE_FETCH64LE(ptr) \
	(((qse_uint64_t)(((qse_uint8_t*)(ptr))[7]) << 56) | ((qse_uint64_t)(((qse_uint8_t*)(ptr))[6]) << 48) | \
	 ((qse_uint64_t)(((qse_uint8_t*)(ptr))[5]) << 40) | ((qse_uint64_t)(((qse_uint8_t*)(ptr))[4]) << 32) | \
	 ((qse_uint64_t)(((qse_uint8_t*)(ptr))[3]) << 24) | ((qse_uint64_t)(((qse_uint8_t*)(ptr))[2]) << 16) | \
	 ((qse_uint64_t)(((qse_uint8_t*)(ptr))[1]) << 8)  | ((qse_uint64_t)(((qse_uint8_t*)(ptr))[0]))))



#define QSE_STORE64BE(ptr,v) do { \
	((qse_uint8_t*)(ptr))[0] = (((qse_uint64_t)(v)) >> 56) & 0xFF; \
	((qse_uint8_t*)(ptr))[1] = (((qse_uint64_t)(v)) >> 48) & 0xFF; \
	((qse_uint8_t*)(ptr))[2] = (((qse_uint64_t)(v)) >> 40) & 0xFF; \
	((qse_uint8_t*)(ptr))[3] = (((qse_uint64_t)(v)) >> 32) & 0xFF; \
	((qse_uint8_t*)(ptr))[5] = (((qse_uint64_t)(v)) >> 24) & 0xFF; \
	((qse_uint8_t*)(ptr))[6] = (((qse_uint64_t)(v)) >> 16) & 0xFF; \
	((qse_uint8_t*)(ptr))[7] = (((qse_uint64_t)(v)) >> 8) & 0xFF; \
	((qse_uint8_t*)(ptr))[8] = (((qse_uint64_t)(v)) >> 0) & 0xFF; \
} while(0)

#define QSE_STORE64LE(ptr,v) do { \
	((qse_uint8_t*)(ptr))[0] = (((qse_uint64_t)(v)) >> 0) & 0xFF; \
	((qse_uint8_t*)(ptr))[1] = (((qse_uint64_t)(v)) >> 8) & 0xFF; \
	((qse_uint8_t*)(ptr))[2] = (((qse_uint64_t)(v)) >> 16) & 0xFF; \
	((qse_uint8_t*)(ptr))[3] = (((qse_uint64_t)(v)) >> 24) & 0xFF; \
	((qse_uint8_t*)(ptr))[5] = (((qse_uint64_t)(v)) >> 32) & 0xFF; \
	((qse_uint8_t*)(ptr))[6] = (((qse_uint64_t)(v)) >> 40) & 0xFF; \
	((qse_uint8_t*)(ptr))[7] = (((qse_uint64_t)(v)) >> 48) & 0xFF; \
	((qse_uint8_t*)(ptr))[8] = (((qse_uint64_t)(v)) >> 56) & 0xFF; \
} while(0)
#endif

#define QSE_FETCH32BE(ptr) \
	(((qse_uint32_t)(((qse_uint8_t*)(ptr))[0]) << 24) | \
	 ((qse_uint32_t)(((qse_uint8_t*)(ptr))[1]) << 16) | \
	 ((qse_uint32_t)(((qse_uint8_t*)(ptr))[2]) << 8)  | \
	 ((qse_uint32_t)(((qse_uint8_t*)(ptr))[3])))

#define QSE_FETCH32LE(ptr) \
	(((qse_uint32_t)(((qse_uint8_t*)(ptr))[3]) << 24) | \
	 ((qse_uint32_t)(((qse_uint8_t*)(ptr))[2]) << 16) | \
	 ((qse_uint32_t)(((qse_uint8_t*)(ptr))[1]) << 8)  | \
	 ((qse_uint32_t)(((qse_uint8_t*)(ptr))[0])))

#define QSE_STORE32BE(ptr,v) do { \
	((qse_uint8_t*)(ptr))[0] = (((qse_uint32_t)(v)) >> 24) & 0xFF; \
	((qse_uint8_t*)(ptr))[1] = (((qse_uint32_t)(v)) >> 16) & 0xFF; \
	((qse_uint8_t*)(ptr))[2] = (((qse_uint32_t)(v)) >> 8) & 0xFF; \
	((qse_uint8_t*)(ptr))[3] = (((qse_uint32_t)(v)) >> 0) & 0xFF; \
} while(0)

#define QSE_STORE32LE(ptr,v) do { \
	((qse_uint8_t*)(ptr))[0] = (((qse_uint32_t)(v)) >> 0) & 0xFF; \
	((qse_uint8_t*)(ptr))[1] = (((qse_uint32_t)(v)) >> 8) & 0xFF; \
	((qse_uint8_t*)(ptr))[2] = (((qse_uint32_t)(v)) >> 16) & 0xFF; \
	((qse_uint8_t*)(ptr))[3] = (((qse_uint32_t)(v)) >> 24) & 0xFF; \
} while(0)



#define QSE_ROTL_INTERNAL(type_t,x,y,nbits) \
	(((x) << ((y) & ((nbits) - 1))) | ((x) >> ((type_t)(nbits) - ((y) & ((nbits) - 1)))))
#define QSE_ROTR_INTERNAL(type_t,x,y,nbits) \
	(((x) >> ((y) & ((nbits) - 1))) | ((x) << ((type_t)(nbits) - ((y) & ((nbits) - 1)))))

#if defined(QSE_HAVE_UINT64_T)

#if !defined(__STRICT_ANSI__) && !defined(INTEL_CC) && defined(__GNUC__) && (defined(__i386) || defined(i386) || defined(__x86_64) || defined(__amd64))
static inline qse_uint64_t QSE_ROTL64 (qse_uint64_t v, int i)
{
	__asm__ volatile ("rolq %%cl,%0": "=r"(v): "0"(v), "c"(i) /*: no clobbers */);
	return v;
}

static inline qse_uint64_t QSE_ROTR64 (qse_uint64_t v, int i)
{
	__asm__ volatile ("rorq %%cl,%0" :"=r"(v) :"0"(v), "c"(i) /*: no clobbers */);
	return v;
}
#else

#	define QSE_ROTL64(x,y) QSE_ROTL_INTERNAL(qse_uint64_t, (qse_uint64_t)x, (qse_uint64_t)y, 64)
#	define QSE_ROTR64(x,y) QSE_ROTR_INTERNAL(qse_uint64_t, (qse_uint64_t)x, (qse_uint64_t)y, 64)

#endif

#endif

#if !defined(__STRICT_ANSI__) && !defined(INTEL_CC) && defined(__GNUC__) && (defined(__i386) || defined(i386) || defined(__x86_64) || defined(__amd64))
static inline qse_uint32_t QSE_ROTL32 (qse_uint32_t v, int i)
{
	__asm__ volatile ("roll %%cl,%0": "=r"(v): "0"(v), "c"(i) /*: no clobbers */);
	return v;
}

static inline qse_uint32_t QSE_ROTR32 (qse_uint32_t v, int i)
{
	__asm__ volatile ("rorl %%cl,%0" :"=r"(v) :"0"(v), "c"(i) /*: no clobbers */);
	return v;
}
#else

#	define QSE_ROTL32(x,y) QSE_ROTL_INTERNAL(qse_uint32_t, (qse_uint32_t)x, (qse_uint32_t)y, 32)
#	define QSE_ROTR32(x,y) QSE_ROTR_INTERNAL(qse_uint32_t, (qse_uint32_t)x, (qse_uint32_t)y, 32)

#endif



/* =========================================================================
 * COMPILER FEATURE TEST MACROS
 * =========================================================================*/
#if !defined(__has_builtin) && defined(_INTELC32_)
	/* intel c code builder 1.0 ended up with an error without this override */
	#define __has_builtin(x) 0
#endif

/*
#if !defined(__is_identifier)
	#define __is_identifier(x) 0
#endif

#if !defined(__has_attribute)
	#define __has_attribute(x) 0
#endif
*/


#if defined(__has_builtin) 
	#if __has_builtin(__builtin_ctz)
		#define QSE_HAVE_BUILTIN_CTZ
	#endif
	#if __has_builtin(__builtin_ctzl)
		#define QSE_HAVE_BUILTIN_CTZL
	#endif
	#if __has_builtin(__builtin_ctzll)
		#define QSE_HAVE_BUILTIN_CTZLL
	#endif

	#if __has_builtin(__builtin_clz)
		#define QSE_HAVE_BUILTIN_CLZ
	#endif
	#if __has_builtin(__builtin_clzl)
		#define QSE_HAVE_BUILTIN_CLZL
	#endif
	#if __has_builtin(__builtin_clzll)
		#define QSE_HAVE_BUILTIN_CLZLL
	#endif

	#if __has_builtin(__builtin_uadd_overflow)
		#define QSE_HAVE_BUILTIN_UADD_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_uaddl_overflow)
		#define QSE_HAVE_BUILTIN_UADDL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_uaddll_overflow)
		#define QSE_HAVE_BUILTIN_UADDLL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_umul_overflow)
		#define QSE_HAVE_BUILTIN_UMUL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_umull_overflow)
		#define QSE_HAVE_BUILTIN_UMULL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_umulll_overflow)
		#define QSE_HAVE_BUILTIN_UMULLL_OVERFLOW 
	#endif

	#if __has_builtin(__builtin_sadd_overflow)
		#define QSE_HAVE_BUILTIN_SADD_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_saddl_overflow)
		#define QSE_HAVE_BUILTIN_SADDL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_saddll_overflow)
		#define QSE_HAVE_BUILTIN_SADDLL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_smul_overflow)
		#define QSE_HAVE_BUILTIN_SMUL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_smull_overflow)
		#define QSE_HAVE_BUILTIN_SMULL_OVERFLOW 
	#endif
	#if __has_builtin(__builtin_smulll_overflow)
		#define QSE_HAVE_BUILTIN_SMULLL_OVERFLOW 
	#endif

	#if __has_builtin(__builtin_expect)
		#define QSE_HAVE_BUILTIN_EXPECT
	#endif


	#if __has_builtin(__sync_lock_test_and_set)
		#define QSE_HAVE_SYNC_LOCK_TEST_AND_SET
	#endif
	#if __has_builtin(__sync_lock_release)
		#define QSE_HAVE_SYNC_LOCK_RELEASE
	#endif

	#if __has_builtin(__sync_synchronize)
		#define QSE_HAVE_SYNC_SYNCHRONIZE
	#endif
	#if __has_builtin(__sync_bool_compare_and_swap)
		#define QSE_HAVE_SYNC_BOOL_COMPARE_AND_SWAP
	#endif
	#if __has_builtin(__sync_val_compare_and_swap)
		#define QSE_HAVE_SYNC_VAL_COMPARE_AND_SWAP
	#endif

	#if __has_builtin(__builtin_bswap16)
		#define QSE_HAVE_BUILTIN_BSWAP16
	#endif
	#if __has_builtin(__builtin_bswap32)
		#define QSE_HAVE_BUILTIN_BSWAP32
	#endif
	#if __has_builtin(__builtin_bswap64)
		#define QSE_HAVE_BUILTIN_BSWAP64
	#endif
	#if __has_builtin(__builtin_bswap128)
		#define QSE_HAVE_BUILTIN_BSWAP128
	#endif

#elif defined(__GNUC__) && defined(__GNUC_MINOR__)

	#if (__GNUC__ >= 5) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)
		#define QSE_HAVE_SYNC_LOCK_TEST_AND_SET
		#define QSE_HAVE_SYNC_LOCK_RELEASE

		#define QSE_HAVE_SYNC_SYNCHRONIZE
		#define QSE_HAVE_SYNC_BOOL_COMPARE_AND_SWAP
		#define QSE_HAVE_SYNC_VAL_COMPARE_AND_SWAP
	#endif

	#if (__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
		#define QSE_HAVE_BUILTIN_CTZ
		#define QSE_HAVE_BUILTIN_CTZL
		#define QSE_HAVE_BUILTIN_CTZLL
		#define QSE_HAVE_BUILTIN_CLZ
		#define QSE_HAVE_BUILTIN_CLZL
		#define QSE_HAVE_BUILTIN_CLZLL
		#define QSE_HAVE_BUILTIN_EXPECT
	#endif

	#if (__GNUC__ >= 5)
		#define QSE_HAVE_BUILTIN_UADD_OVERFLOW
		#define QSE_HAVE_BUILTIN_UADDL_OVERFLOW
		#define QSE_HAVE_BUILTIN_UADDLL_OVERFLOW
		#define QSE_HAVE_BUILTIN_UMUL_OVERFLOW
		#define QSE_HAVE_BUILTIN_UMULL_OVERFLOW
		#define QSE_HAVE_BUILTIN_UMULLL_OVERFLOW

		#define QSE_HAVE_BUILTIN_SADD_OVERFLOW
		#define QSE_HAVE_BUILTIN_SADDL_OVERFLOW
		#define QSE_HAVE_BUILTIN_SADDLL_OVERFLOW
		#define QSE_HAVE_BUILTIN_SMUL_OVERFLOW
		#define QSE_HAVE_BUILTIN_SMULL_OVERFLOW
		#define QSE_HAVE_BUILTIN_SMULLL_OVERFLOW
	#endif

	#if (__GNUC__ >= 5) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)
		/* 4.8.0 or later */
		#define QSE_HAVE_BUILTIN_BSWAP16
	#endif
	#if (__GNUC__ >= 5) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
		/* 4.3.0 or later */
		#define QSE_HAVE_BUILTIN_BSWAP32
		#define QSE_HAVE_BUILTIN_BSWAP64
		/*#define QSE_HAVE_BUILTIN_BSWAP128*/
	#endif
#endif

#if defined(QSE_HAVE_BUILTIN_EXPECT)
#	define QSE_LIKELY(x) (__builtin_expect(!!(x),1))
#	define QSE_UNLIKELY(x) (__builtin_expect(!!(x),0))
#else
#	define QSE_LIKELY(x) (x)
#	define QSE_UNLIKELY(x) (x)
#endif

/* ---------------------------------------------------------------------- 
 * C++ NAMESPACE
 * ---------------------------------------------------------------------- */
#if defined(__cplusplus)
#	define QSE_BEGIN_NAMESPACE(x)    namespace x {
#	define QSE_END_NAMESPACE(x)      }
#	define QSE_BEGIN_NAMESPACE2(x,y) namespace x { namespace y {
#	define QSE_END_NAMESPACE2(y,x)   }}
#endif

#endif
