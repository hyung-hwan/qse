/*
 * $Id: macros.h 212 2009-06-25 07:39:27Z hyunghwan.chung $
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef _QSE_MACROS_H_
#define _QSE_MACROS_H_

#include <qse/types.h>

/** @file
 * <qse/macros.h> contains various useful macro definitions.
 */

/** 
 * The #QSE_NULL macro defines a special pointer value to indicate an error or
 * that it does not point to anything.
 */
#ifdef __cplusplus
#	if QSE_SIZEOF_VOID_P == QSE_SIZEOF_INT
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

#define QSE_MCHAR_EOF ((qse_mcint_t)-1)
#define QSE_WCHAR_EOF ((qse_wcint_t)-1)
#define QSE_CHAR_EOF  ((qse_cint_t)-1)

/**
 * The QSE_SIZEOF() macro gets data size in bytes. It is equivalent to the
 * sizeof operator. The following code snippet should print sizeof(int)*128.
 * @code
 * int x[128];
 * printf ("%d\n", (int)QSE_SIZEOF(x));
 * @endcode
 */
#define QSE_SIZEOF(n)  (sizeof(n))

/**
 * The QSE_COUNTOF() macro gets the number elements in a array.
 * The following code snippet should print 128.
 * @code
 * int x[128];
 * printf ("%d\n", (int)QSE_COUNTOF(x));
 * @endcode
 */
#define QSE_COUNTOF(n) (sizeof(n)/sizeof(n[0]))

/**
 * The QSE_OFFSETOF() macro get the offset of a fields from the beginning
 * of a structure.
 */
#define QSE_OFFSETOF(type,member) ((qse_size_t)&((type*)0)->member)

/**
 * The QSE_TYPE_IS_SIGNED() macro determines if a type is signed. 
 * @code
 * printf ("%d\n", QSE_TYPE_IS_SIGNED(int));
 * printf ("%d\n", QSE_TYPE_IS_SIGNED(unsigned int));
 * @endcode
 */
#define QSE_TYPE_IS_SIGNED(type) (((type)0) > ((type)-1))

/**
 * The QSE_TYPE_IS_SIGNED() macro determines if a type is unsigned. 
 * @code
 * printf ("%d\n", QSE_TYPE_IS_UNSIGNED(int));
 * printf ("%d\n", QSE_TYPE_IS_UNSIGNED(unsigned int));
 * @endcode
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

#define QSE_IS_POWOF2(x) (((x) & ((x) - 1)) == 0)

#define QSE_SWAP(x,y,original_type,casting_type) \
	do { \
		x = (original_type)((casting_type)(x) ^ (casting_type)(y)); \
		y = (original_type)((casting_type)(y) ^ (casting_type)(x)); \
		x = (original_type)((casting_type)(x) ^ (casting_type)(y)); \
	} while (0)

#define QSE_ABS(x) ((x) < 0? -(x): (x))

#define QSE_ERR_THROW(id) goto  __err_ ## id 
#define QSE_ERR_CATCH(id) while(0) __err_ ## id:

#define QSE_LOOP_CONTINUE(id) goto __loop_ ## id ## _begin__
#define QSE_LOOP_BREAK(id)    goto __loop_ ## id ## _end__
#define QSE_LOOP_BEGIN(id)    __loop_ ## id ## _begin__: {
#define QSE_LOOP_END(id)      QSE_LOOP_CONTINUE(id) } __loop_ ## id ## _end__:

#define QSE_REPEAT(n,blk) \
	do { \
		qse_size_t __qse_repeat_x1__ = (qse_size_t)(n); \
		qse_size_t __qse_repeat_x2__ = __qse_repeat_x1__ >> 4; \
		__qse_repeat_x1__ &= 15; \
		while (__qse_repeat_x1__-- > 0) { blk; } \
		while (__qse_repeat_x2__-- > 0) { \
			blk; blk; blk; blk; blk; blk; blk; blk; \
			blk; blk; blk; blk; blk; blk; blk; blk; \
		} \
	} while (0);

/* number of characters to number of bytes */
#define QSE_NCTONB(x) ((x)*sizeof(qse_char_t))
/* number of bytes to number of characters */
#define QSE_NBTONC(x) ((x)/sizeof(qse_char_t))

#define QSE_MQ_I(val) #val
#define QSE_MQ(val)   QSE_MQ_I(val)
#define QSE_MC(ch)    ((qse_mchar_t)ch)
#define QSE_MS(str)   ((const qse_mchar_t*)str)
/** 
 * The #QSE_MT macro maps a multi-byte literal string literal as it is. 
 */
#define QSE_MT(txt)   (txt)

#define QSE_WQ_I(val)  (L ## QSE_MQ_I(val))
#define QSE_WQ(val)    QSE_WQ_I(val)
#define QSE_WC(ch)     ((qse_wchar_t)L ## ch)
#define QSE_WS(str)    ((const qse_wchar_t*)L ## str)
/** 
 * The #QSE_WT macro maps a multi-byte literal string to a wide character 
 * string by prefixing it with @b L.
 */
#define QSE_WT(txt)    (L ## txt)

/** @def QSE_T
 * The #QSE_T macro maps to #QSE_MT if #QSE_CHAR_IS_MCHAR is defined, and to
 * #QSE_WT if #QSE_CHAR_IS_WCHAR is defined.
 */
#if defined(QSE_CHAR_IS_MCHAR)
#	define QSE_Q(val) QSE_MQ(val)
#	define QSE_C(ch)  QSE_MC(ch)
#	define QSE_S(str) QSE_MS(str)
#	define QSE_T(txt) QSE_MT(txt)
#else
#	define QSE_Q(val) QSE_WQ(val)
#	define QSE_C(ch)  QSE_WC(ch)
#	define QSE_S(str) QSE_WS(str)
#	define QSE_T(txt) QSE_WT(txt)
#endif

#if defined(__GNUC__)
#	define QSE_BEGIN_PACKED_STRUCT(x) struct x {
#	define QSE_END_PACKED_STRUCT() } __attribute__((packed));
#else
#	define QSE_BEGIN_PACKED_STRUCT(x) struct x {
#	define QSE_END_PACKED_STRUCT() };
#endif

#ifdef NDEBUG
#	define QSE_ASSERT(expr) ((void)0)
#	define QSE_ASSERTX(expr,desc) ((void)0)
#else
#	ifdef __cplusplus
		extern "C" {
#	endif
		void qse_assert_failed (
        		const qse_char_t* expr, const qse_char_t* desc,
        		const qse_char_t* file, qse_size_t line);
#	ifdef __cplusplus
		}
#	endif

#	define QSE_ASSERT(expr) (void)((expr) || \
		(qse_assert_failed (QSE_T(#expr), QSE_NULL, QSE_T(__FILE__), __LINE__), 0))
#	define QSE_ASSERTX(expr,desc) (void)((expr) || \
		(qse_assert_failed (QSE_T(#expr), QSE_T(desc), QSE_T(__FILE__), __LINE__), 0))
#endif

#ifdef __cplusplus
#	define QSE_BEGIN_NAMESPACE(x)    namespace x {
#	define QSE_END_NAMESPACE(x)      }
#	define QSE_BEGIN_NAMESPACE2(x,y) namespace x { namespace y {
#	define QSE_END_NAMESPACE2(y,x)   }}
#endif

/**
 * The QSE_DEFINE_COMMON_FIELDS() macro defines common object fields.
 */
#define QSE_DEFINE_COMMON_FIELDS(name) \
	qse_mmgr_t* mmgr;
	
/**
 * The QSE_DEFINE_COMMON_FUNCTIONS() macro defines common object functions.
 * - @code void qse_xxx_setmmgr (qse_xxx_t* xxx, qse_mmgr_t* mmgr); @endcode
 *   The qse_xxx_setmmgr() function change the memory manager of a relevant
 *   object. Take extreme care if you want to use this function.
 * - @code qse_mmgr_t* qse_xxx_getmmgr (qse_xxx_t* xxx); @endcode
 *   The qse_xxx_getmmgr() function returns the memory manager of a relevant
 *   object.
 * - @code void qse_xxx_getxtn (qse_xxx_t* xxx); @endcode
 *   The qse_xxx_getxtn() function returns the pointer to an extension area
 *   of a relevant object created with an extension size greater than 0.
 */
#define QSE_DEFINE_COMMON_FUNCTIONS(name) \
void qse_##name##_setmmgr (qse_##name##_t* name, qse_mmgr_t* mmgr); \
qse_mmgr_t* qse_##name##_getmmgr (qse_##name##_t* name); \
void* qse_##name##_getxtn (qse_##name##_t* name);


/**
 * The QSE_MMGR() macro gets the memory manager field from an object.
 */
#define QSE_MMGR(obj) ((obj)->mmgr)

/**
 * The QSE_XTN() macro is a convenience macro to retrieve the pointer to 
 * extension space located at the end of an object. The type of the object
 * should be known in advance for it to work properly.
 */
#define QSE_XTN(obj) ((void*)(obj + 1))

/**
 * The QSE_IMPLEMENT_COMMON_FUNCTIONS() implement common functions for 
 * an object.
 */
#define QSE_IMPLEMENT_COMMON_FUNCTIONS(name) \
void qse_##name##_setmmgr (qse_##name##_t* name, qse_mmgr_t* mmgr) \
{ \
	QSE_MMGR(name) = mmgr; \
} \
qse_mmgr_t* qse_##name##_getmmgr (qse_##name##_t* name) \
{ \
	return QSE_MMGR(name); \
} \
void* qse_##name##_getxtn (qse_##name##_t* name) \
{ \
	return QSE_XTN(name); \
}

#endif
