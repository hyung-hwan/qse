/*
 * $Id: macros.h 455 2008-11-26 09:05:00Z baconevi $
 *
 * {License}
 */

#ifndef _QSE_MACROS_H_
#define _QSE_MACROS_H_

#include <qse/types.h>

#ifdef __cplusplus
	/*#define QSE_NULL ((qse_uint_t)0)*/
#	define QSE_NULL (0)
#else
#	define QSE_NULL ((void*)0)
#endif

/****d* ase/QSE_TRUE,QSE_FALSE
 * NAME
 *  QSE_TRUE - represent a boolean true
 *  QSE_FALSE - represent a boolean false
 ******
 */
#define QSE_TRUE  (0 == 0)
#define QSE_FALSE (0 != 0)


/****d* ase/QSE_ALIVE,QSE_ZOMBIE,QSE_DEAD
 * NAME
 *  QSE_ALIVE - represent a living state
 *  QSE_ZOMBIE - represent a zombie state
 *  QSE_DEAD - represent a dead state
 ******
 */
#define QSE_ALIVE   1
#define QSE_ZOMBIE  0
#define QSE_DEAD   -1


#define AES_MCHAR_EOF ((qse_mcint_t)-1)
#define AES_WCHAR_EOF ((qse_wcint_t)-1)
#define QSE_CHAR_EOF  ((qse_cint_t)-1)

#define QSE_SIZEOF(n)  (sizeof(n))
#define QSE_COUNTOF(n) (sizeof(n)/sizeof(n[0]))
#define QSE_OFFSETOF(type,member) ((qse_size_t)&((type*)0)->member)

#define QSE_TYPE_IS_SIGNED(type) (((type)0) > ((type)-1))
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

#define QSE_LOOP_CONTINUE(id) goto __loop_ ## id ## _begin__;
#define QSE_LOOP_BREAK(id)    goto __loop_ ## id ## _end__;
#define QSE_LOOP_BEGIN(id)    __loop_ ## id ## _begin__: {
#define QSE_LOOP_END(id)      QSE_LOOP_CONTINUE(id) } __loop_ ## id ## _end__:;

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
#define QSE_MT(txt)   (txt)

#define QSE_WQ_I(val)  (L ## #val)
#define QSE_WQ(val)    QSE_WQ_I(val)
#define QSE_WC(ch)     ((qse_wchar_t)L ## ch)
#define QSE_WS(str)    ((const qse_wchar_t*)L ## str)
#define QSE_WT(txt)    (L ## txt)

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

#define QSE_DEFINE_STD_FIELDS(name) \
	qse_mmgr_t* mmgr;
	
#define QSE_DEFINE_STD_FUNCTIONS(name) \
qse_##name##_t qse_##name##_setmmgr (qse_##name##_t* name, qse_mmgr_t* mmgr); \
qse_mmgr_t* qse_##name##_getmmgr (qse_##name##_t* name); \
void* qse_##name##_getxtn (qse_##name##_t* name);

#define QSE_IMPLEMENT_STD_FUNCTIONS(name) \
qse_##name##_t qse_##name##_setmmgr (qse_##name##_t* name, qse_mmgr_t* mmgr) \
{ \
	name->mmgr = mmgr; \
} \
qse_mmgr_t* qse_##name##_getmmgr (qse_##name##_t* name) \
{ \
	return name->mmgr; \
} \
void* qse_##name##_getxtn (qse_##name##_t* name) \
{ \
	return (void*)(name + 1); \
}

#endif
