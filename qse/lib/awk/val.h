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

#ifndef _QSE_LIB_AWK_VAL_H_
#define _QSE_LIB_AWK_VAL_H_

#define QSE_AWK_VAL_CHUNK_SIZE 100

typedef struct qse_awk_val_chunk_t qse_awk_val_chunk_t;
typedef struct qse_awk_val_ichunk_t qse_awk_val_ichunk_t;
typedef struct qse_awk_val_rchunk_t qse_awk_val_rchunk_t;

struct qse_awk_val_chunk_t
{
        qse_awk_val_chunk_t* next;
};

struct qse_awk_val_ichunk_t
{
	qse_awk_val_chunk_t* next;
	/* make sure that it has the same fields as 
	   qse_awk_val_chunk_t up to this point */

	qse_awk_val_int_t slot[QSE_AWK_VAL_CHUNK_SIZE];
};

struct qse_awk_val_rchunk_t
{
	qse_awk_val_chunk_t* next;
	/* make sure that it has the same fields as 
	   qse_awk_val_chunk_t up to this point */

	qse_awk_val_flt_t slot[QSE_AWK_VAL_CHUNK_SIZE];
};


/* 
 * if shared objects link a static library, statically defined objects
 * in the static library will be instatiated in the multiple shared objects.
 *
 * so equality check with a value pointer doesn't work
 * if the code crosses the library boundaries. instead, i decided to
 * add a field to indicate if a value is static.
 * 

#define IS_STATICVAL(val) \
	((val) == QSE_NULL || \
	 (val) == qse_awk_val_nil || \
	 (val) == qse_awk_val_zls || \
	 (val) == qse_awk_val_zero || \
	 (val) == qse_awk_val_one || \
	 ((val) >= (qse_awk_val_t*)&awk_int[0] && \
	  (val) <= (qse_awk_val_t*)&awk_int[QSE_COUNTOF(awk_int)-1]))
*/
#define IS_STATICVAL(val) ((val)->stat)



/* qse_awk_val_t pointer encoding assumes the pointer is an even number.
 * is this a safe assumption? do i have to use memalign or write my own
 * aligned malloc()? */
#define VAL_NUM_TYPE_BITS        1
#define VAL_MASK_TYPE_BITS       1 

#define VAL_TYPE_BITS_POINTER    0
#define VAL_TYPE_BITS_QUICKINT   1
#define VAL_SIGN_BIT ((qse_uintptr_t)1 << (QSE_SIZEOF_UINTPTR_T * 8 - 1))

/* shrink the bit range by 1 more bit to ease signbit handling. 
 * i want abs(max) == abs(min).
 * i don't want abs(max) + 1 == abs(min). e.g min: -32768, max: 32767
 */
#define QUICKINT_MAX ((qse_awk_int_t)((~(qse_uintptr_t)0) >> (VAL_NUM_TYPE_BITS + 1)))
#define QUICKINT_MIN (-QUICKINT_MAX)

#define IS_QUICKINT(i) ((i) >= QUICKINT_MIN && (i) <= QUICKINT_MAX)

#define POINTER_TYPE_BITS(p) (((qse_uintptr_t)(p)) & VAL_MASK_TYPE_BITS)
#define IS_REAL_POINTER(p) (POINTER_TYPE_BITS(p) == VAL_TYPE_BITS_POINTER)
#define IS_QUICKINT_POINTER(p) (POINTER_TYPE_BITS(p) == VAL_TYPE_BITS_QUICKINT)

/* sizeof(qse_intptr_t) may not be the same as sizeof(qse_awk_int_t).
 * so step-by-step type conversions are needed.
 * e.g) pointer to uintptr_t, uintptr_t to intptr_t, intptr_t to awk_int_t */
#define POSITIVE_QUICKINT_FROM_POINTER(p) ((qse_intptr_t)((qse_uintptr_t)(p) >> VAL_NUM_TYPE_BITS))
#define NEGATIVE_QUICKINT_FROM_POINTER(p) (-(qse_intptr_t)(((qse_uintptr_t)(p) & ~VAL_SIGN_BIT) >> VAL_NUM_TYPE_BITS))
#define GET_QUICKINT_FROM_POINTER(p) (((qse_uintptr_t)(p) & VAL_SIGN_BIT)? NEGATIVE_QUICKINT_FROM_POINTER(p): POSITIVE_QUICKINT_FROM_POINTER(p))

#define QSE_AWK_RTX_GETVALTYPE(rtx,p) (IS_QUICKINT_POINTER(p)? QSE_AWK_VAL_INT: (p)->v_type)
#define QSE_AWK_RTX_GETINTFROMVAL(rtx,p) ((IS_QUICKINT_POINTER(p)? (qse_awk_int_t)GET_QUICKINT_FROM_POINTER(p): ((qse_awk_val_int_t*)(p))->i_val))

#if defined(__cplusplus)
extern "C" {
#endif

/* represents a nil value */
extern qse_awk_val_t* qse_awk_val_nil;

/* represents an empty string  */
extern qse_awk_val_t* qse_awk_val_zls;

/* represents a numeric value -1 */
extern qse_awk_val_t* qse_awk_val_negone;

/* represents a numeric value 0 */
extern qse_awk_val_t* qse_awk_val_zero;

/* represents a numeric value 1 */
extern qse_awk_val_t* qse_awk_val_one;


void qse_awk_rtx_freeval (
        qse_awk_rtx_t* rtx,
        qse_awk_val_t* val,
        int            cache
);

void qse_awk_rtx_freevalchunk (
	qse_awk_rtx_t*       rtx,
	qse_awk_val_chunk_t* chunk
);

#if defined(__cplusplus)
}
#endif

#endif
