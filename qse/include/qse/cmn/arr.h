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

#ifndef _QSE_CMN_ARR_H_
#define _QSE_CMN_ARR_H_

#include <qse/types.h>
#include <qse/macros.h>

/** @file
 * This file provides a linear dynamic array. It grows dynamically as items 
 * are added.
 */

enum qse_arr_walk_t
{
	QSE_ARR_WALK_STOP     = 0,
	QSE_ARR_WALK_FORWARD  = 1,
	QSE_ARR_WALK_BACKWARD = 2
};

typedef struct qse_arr_t      qse_arr_t;
typedef struct qse_arr_slot_t qse_arr_slot_t;
typedef enum   qse_arr_walk_t qse_arr_walk_t;

#define QSE_ARR_COPIER_SIMPLE  ((qse_arr_copier_t)1)
#define QSE_ARR_COPIER_INLINE  ((qse_arr_copier_t)2)

#define QSE_ARR_NIL ((qse_size_t)-1)

#define QSE_ARR_SIZE(arr)        (*(const qse_size_t*)&(arr)->size)
#define QSE_ARR_CAPA(arr)        (*(const qse_size_t*)&(arr)->capa)

#define QSE_ARR_SLOT(arr,index)  ((arr)->slot[index])
#define QSE_ARR_DPTL(arr,index)  ((const qse_xptl_t*)&(arr)->slot[index]->val)
#define QSE_ARR_DPTR(arr,index)  ((arr)->slot[index]->val.ptr)
#define QSE_ARR_DLEN(arr,index)  ((arr)->slot[index]->val.len)

/**
 *  The qse_arr_copier_t type defines a callback function for slot construction.
 *  A slot is contructed when a user adds data to an array. The user can
 *  define how the data to add can be maintained in the array. A dynamic
 *  array not specified with any copiers stores the data pointer and
 *  the data length into a slot. A special copier QSE_ARR_COPIER_INLINE copies 
 *  the contents of the data a user provided into the slot. You can use the
 *  qse_arr_setcopier() function to change the copier. 
 * 
 *  A copier should return the pointer to the copied data. If it fails to copy
 *  data, it may return QSE_NULL. You need to set a proper freeer to free up
 *  memory allocated for copy.
 */
typedef void* (*qse_arr_copier_t) (
	qse_arr_t* arr    /**< array */,
	void*      dptr   /**< pointer to data to copy */,
	qse_size_t dlen   /**< length of data to copy */
);

/**
 * The qse_arr_freeer_t type defines a slot destruction callback.
 */
typedef void (*qse_arr_freeer_t) (
	qse_arr_t* arr    /**< array */,
	void*      dptr   /**< pointer to data to free */,
	qse_size_t dlen   /**< length of data to free */
);

/**
 * The qse_arr_comper_t type defines a key comparator that is called when
 * the arry needs to compare data. A linear dynamic array is created with a
 * default comparator that performs bitwise comparison. 
 *
 * The default comparator compares data in a memcmp-like fashion.
 * It is not suitable when you want to implement a heap of numbers
 * greater than a byte size. You must implement a comparator that 
 * takes the whole element and performs comparison in such a case.
 * 
 * The comparator should return 0 if the data are the same, a negative 
 * integer if the first data is less than the second data, a positive
 * integer otherwise.
 *
 */
typedef int (*qse_arr_comper_t) (
	qse_arr_t*  arr    /* array */, 
	const void* dptr1  /* data pointer */,
	qse_size_t  dlen1  /* data length */,
	const void* dptr2  /* data pointer */,
	qse_size_t  dlen2  /* data length */
);

/**
 * The qse_arr_keeper_t type defines a value keeper that is called when 
 * a value is retained in the context that it should be destroyed because
 * it is identical to a new value. Two values are identical if their beginning
 * pointers and their lengths are equal.
 */
typedef void (*qse_arr_keeper_t) (
	qse_arr_t* arr     /**< array */,
	void* vptr         /**< pointer to a value */,
	qse_size_t vlen    /**< length of a value */	
);

/**
 * The qse_arr_sizer_t type defines an array size claculator that is called
 * when the array needs to be resized. 
 */
typedef qse_size_t (*qse_arr_sizer_t) (
	qse_arr_t* arr,  /**< array */
	qse_size_t hint  /**< sizing hint */
);

typedef qse_arr_walk_t (*qse_arr_walker_t) (
	qse_arr_t*      arr   /* array */,
	qse_size_t      index /* index to the visited slot */,
	void*           ctx   /* user-defined context */
);

/**
 * The qse_arr_t type defines a linear dynamic array.
 */
struct qse_arr_t
{
	qse_mmgr_t* mmgr;
	qse_arr_copier_t copier; /* data copier */
	qse_arr_freeer_t freeer; /* data freeer */
	qse_arr_comper_t comper; /* data comparator */
	qse_arr_keeper_t keeper; /* data keeper */
	qse_arr_sizer_t  sizer;  /* size calculator */
	qse_byte_t       scale;  /* scale factor */
	qse_size_t       heap_pos_offset; /* offset in the data element where position 
	                                   * is stored when heap operation is performed. */
	qse_size_t       size;   /* number of items */
	qse_size_t       capa;   /* capacity */
	qse_arr_slot_t** slot;
};

/**
 * The qse_arr_slot_t type defines a linear dynamic array slot
 */
struct qse_arr_slot_t
{
	qse_xptl_t val;
};

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * The qse_arr_open() function creates a linear dynamic array.
 */
QSE_EXPORT qse_arr_t* qse_arr_open (
	qse_mmgr_t* mmgr, /**< memory manager */
	qse_size_t  ext,  /**< extension size in bytes */
	qse_size_t  capa  /**< initial array capacity */
);

/**
 * The qse_arr_close() function destroys a linear dynamic array.
 */
QSE_EXPORT void qse_arr_close (
	qse_arr_t* arr /**< array */
);

/**
 * The qse_arr_init() function initializes a linear dynamic array.
 */
QSE_EXPORT int qse_arr_init (
	qse_arr_t*  arr,
	qse_mmgr_t* mmgr,
	qse_size_t  capa
);

/**
 * The qse_arr_fini() function finalizes a linear dynamic array.
 */
QSE_EXPORT void qse_arr_fini (
	qse_arr_t* arr /**< array */
);

QSE_EXPORT qse_mmgr_t* qse_arr_getmmgr (
	qse_arr_t* arr
);

QSE_EXPORT void* qse_arr_getxtn (
	qse_arr_t* arr
);

/**
 * The qse_arr_getscale() function returns the scale factor
 */
QSE_EXPORT int qse_arr_getscale (
	qse_arr_t* arr   /**< array */
);

/**
 * The qse_arr_setscale() function sets the scale factor of the length
 * of a key and a value. A scale factor determines the actual length of
 * a key and a value in bytes. A arr is created with a scale factor of 1.
 * The scale factor should be larger than 0 and less than 256.
 * It is a bad idea to change the scale factor when @a arr is not empty.
 */
QSE_EXPORT void qse_arr_setscale (
	qse_arr_t* arr   /**< array */,
	int scale        /**< scale factor - 1 to 255 */
);

QSE_EXPORT qse_arr_copier_t qse_arr_getcopier (
	qse_arr_t* arr   /* array */
);

/**
 * The qse_arr_setcopier() specifies how to clone an element. The special 
 * copier #QSE_ARR_COPIER_INLINE copies the data inline to the internal slot.
 * No freeer is invoked when the slot is freeed. You may set the copier to 
 * #QSE_ARR_COPIER_SIMPLE to perform no special operation when the data 
 * pointer is stored.
 */
QSE_EXPORT void qse_arr_setcopier (
	qse_arr_t* arr           /** arr */, 
	qse_arr_copier_t copier  /** element copier */
);

/**
 * The qse_arr_getfreeer() function returns a custom element destroyer.
 */
QSE_EXPORT qse_arr_freeer_t qse_arr_getfreeer (
	qse_arr_t*   arr  /**< arr */
);

/**
 * The qse_arr_setfreeer() function specifies how to destroy an element.
 * The @a freeer is called when a slot containing the element is destroyed.
 */
QSE_EXPORT void qse_arr_setfreeer (
	qse_arr_t* arr           /**< arr */,
	qse_arr_freeer_t freeer  /**< element freeer */
);

QSE_EXPORT qse_arr_comper_t qse_arr_getcomper (
	qse_arr_t*   arr  /**< arr */
);

/**
 * The qse_arr_setcomper() function specifies how to compare two elements
 * for equality test. The comparator @a comper must return 0 if two elements
 * compared are equal, 1 if the first element is greater than the 
 * second, -1 if the second element is greater than the first.
 */
QSE_EXPORT void qse_arr_setcomper (
	qse_arr_t*       arr     /**< arr */,
	qse_arr_comper_t comper  /**< comparator */
);

QSE_EXPORT qse_arr_keeper_t qse_arr_getkeeper (
        qse_arr_t* arr
);

QSE_EXPORT void qse_arr_setkeeper (
        qse_arr_t* arr,
        qse_arr_keeper_t keeper 
);

QSE_EXPORT qse_arr_sizer_t qse_arr_getsizer (
        qse_arr_t* arr
);

QSE_EXPORT void qse_arr_setsizer (
        qse_arr_t* arr,
        qse_arr_sizer_t sizer
);

QSE_EXPORT qse_size_t qse_arr_getsize (
	qse_arr_t* arr
);

QSE_EXPORT qse_size_t qse_arr_getcapa (
	qse_arr_t* arr
);

QSE_EXPORT qse_arr_t* qse_arr_setcapa (
	qse_arr_t* arr,
	qse_size_t capa
);

QSE_EXPORT qse_size_t qse_arr_search (
	qse_arr_t*  arr,
	qse_size_t  pos,
	const void* dptr,
	qse_size_t  dlen
);

QSE_EXPORT qse_size_t qse_arr_rsearch (
	qse_arr_t*  arr,
	qse_size_t  pos,
	const void* dptr,
	qse_size_t  dlen
);

QSE_EXPORT qse_size_t qse_arr_upsert (
	qse_arr_t* arr,
	qse_size_t index, 
	void*      dptr,
	qse_size_t dlen
);

QSE_EXPORT qse_size_t qse_arr_insert (
	qse_arr_t* arr,
	qse_size_t index, 
	void*      dptr,
	qse_size_t dlen
);

QSE_EXPORT qse_size_t qse_arr_update (
	qse_arr_t* arr,
	qse_size_t pos,
	void*      dptr,
	qse_size_t dlen
);

/**
 * The qse_arr_delete() function deletes the as many data as the count 
 * from the index. It returns the number of data deleted.
 */
QSE_EXPORT qse_size_t qse_arr_delete (
	qse_arr_t* arr,
	qse_size_t index,
	qse_size_t count
);

/**
 *  The qse_arr_uplete() function deletes data slot without compaction.
 *  It returns the number of data affected.
 */
QSE_EXPORT qse_size_t qse_arr_uplete (
	qse_arr_t* arr,
	qse_size_t index,
	qse_size_t count
);

QSE_EXPORT void qse_arr_clear (
	qse_arr_t* arr
);

/**
 * The qse_arr_walk() function calls the @a walker function for each
 * element in the array beginning from the first. The @a walker function
 * should return one of #QSE_ARR_WALK_FORWARD, #QSE_ARR_WALK_BACKWARD,
 * #QSE_ARR_WALK_STOP.
 * @return number of calls to the @a walker fucntion made
 */
QSE_EXPORT qse_size_t qse_arr_walk (
	qse_arr_t*       arr,
	qse_arr_walker_t walker,
	void*            ctx
);

/**
 * The qse_arr_rwalk() function calls the @a walker function for each
 * element in the array beginning from the last. The @a walker function
 * should return one of #QSE_ARR_WALK_FORWARD, #QSE_ARR_WALK_BACKWARD,
 * #QSE_ARR_WALK_STOP.
 * @return number of calls to the @a walker fucntion made
 */
QSE_EXPORT qse_size_t qse_arr_rwalk (
	qse_arr_t*       arr,
	qse_arr_walker_t walker,
	void*            ctx
);

/**
 * The qse_arr_pushstack() function appends data to the array. It is a utility
 * function to allow stack-like operations over an array. To do so, you should
 * not play with other non-stack related functions.
 */
QSE_EXPORT qse_size_t qse_arr_pushstack (
	qse_arr_t* arr,
	void*      dptr, 
	qse_size_t dlen
);

/**
 * The qse_arr_popstack() function deletes the last array data. It is a utility
 * function to allow stack-like operations over an array. To do so, you should
 * not play with other non-stack related functions. 
 * @note You must not call this function if @a arr is empty.
 */
QSE_EXPORT void qse_arr_popstack (
	qse_arr_t* arr
);

/**
 * The qse_arr_pushheap() function inserts data to an array while keeping the
 * largest data at position 0. It is a utiltiy funtion to implement a binary
 * max-heap over an array. Inverse the comparator to implement a min-heap.
 * @return number of array elements
 * @note You must not mess up the array with other non-heap related functions
 *       to keep the heap property.
 */
QSE_EXPORT qse_size_t qse_arr_pushheap (
	qse_arr_t* arr,
	void*      dptr,
	qse_size_t dlen
);

/**
 * The qse_arr_popheap() function deletes data at position 0 while keeping
 * the largest data at positon 0. It is a utiltiy funtion to implement a binary
 * max-heap over an array. 
 * @note You must not mess up the array with other non-heap related functions
 *       to keep the heap property.
 */
QSE_EXPORT void qse_arr_popheap (
	qse_arr_t* arr
);

QSE_EXPORT void qse_arr_deleteheap (
	qse_arr_t* arr,
	qse_size_t index
);

QSE_EXPORT qse_size_t qse_arr_updateheap (
	qse_arr_t* arr,
	qse_size_t index,
	void*      dptr,
	qse_size_t dlen
);

QSE_EXPORT qse_size_t qse_arr_getheapposoffset (
	qse_arr_t* arr
);

/**
 * The qse_arr_setheapposoffset() function sets the offset to a position holding 
 * field within a data element. It assumes that the field is of the qse_size_t type.
 *
 * \code
 * struct data_t
 * {
 *    int v;
 *    qse_size_t pos;
 * };
 * struct data_t d;
 * qse_arr_setheapposoffset (arr, QSE_OFFSETOF(struct data_t, pos)); 
 * d.v = 20;
 * qse_arr_pushheap (arr, &d, 1);
 * \endcode
 * 
 * In the code above, the 'pos' field of the first element in the array must be 0.
 * 
 * If it's set to QSE_ARR_NIL, position is not updated when heapification is 
 * performed.  
 */
QSE_EXPORT void qse_arr_setheapposoffset (
	qse_arr_t* arr,
	qse_size_t offset
);

#if defined(__cplusplus)
}
#endif

#endif
