/*
 * $Id: lda.h 556 2011-08-31 15:43:46Z hyunghwan.chung $
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

#ifndef _QSE_CMN_LDA_H_
#define _QSE_CMN_LDA_H_

#include <qse/types.h>
#include <qse/macros.h>

/** @file
 * This file provides a linear dynamic array. It grows dynamically as items 
 * are added.
 */

enum qse_lda_walk_t
{
	QSE_LDA_WALK_STOP     = 0,
	QSE_LDA_WALK_FORWARD  = 1,
	QSE_LDA_WALK_BACKWARD = 2
};

typedef struct qse_lda_t      qse_lda_t;
typedef struct qse_lda_slot_t qse_lda_slot_t;
typedef enum   qse_lda_walk_t qse_lda_walk_t;

#define QSE_LDA_COPIER_SIMPLE  ((qse_lda_copier_t)1)
#define QSE_LDA_COPIER_INLINE  ((qse_lda_copier_t)2)

#define QSE_LDA_NIL ((qse_size_t)-1)

#define QSE_LDA_SIZE(lda)        (*(const qse_size_t*)&(lda)->size)
#define QSE_LDA_CAPA(lda)        (*(const qse_size_t*)&(lda)->capa)

#define QSE_LDA_SLOT(lda,index)  ((lda)->slot[index])
#define QSE_LDA_DPTL(lda,index)  ((const qse_xptl_t*)&(lda)->slot[index]->val)
#define QSE_LDA_DPTR(lda,index)  ((lda)->slot[index]->val.ptr)
#define QSE_LDA_DLEN(lda,index)  ((lda)->slot[index]->val.len)

/**
 *  The qse_lda_copier_t type defines a callback function for slot construction.
 *  A slot is contructed when a user adds data to a list. The user can
 *  define how the data to add can be maintained in the list. A singly
 *  linked list not specified with any copiers stores the data pointer and
 *  the data length into a slot. A special copier QSE_LDA_COPIER_INLINE copies 
 *  the contents of the data a user provided into the slot. You can use the
 *  qse_lda_setcopier() function to change the copier. 
 * 
 *  A copier should return the pointer to the copied data. If it fails to copy
 *  data, it may return QSE_NULL. You need to set a proper freeer to free up
 *  memory allocated for copy.
 */
typedef void* (*qse_lda_copier_t) (
	qse_lda_t* lda    /**< array */,
	void*      dptr   /**< pointer to data to copy */,
	qse_size_t dlen   /**< length of data to copy */
);

/**
 * The qse_lda_freeer_t type defines a slot destruction callback.
 */
typedef void (*qse_lda_freeer_t) (
	qse_lda_t* lda    /**< array */,
	void*      dptr   /**< pointer to data to free */,
	qse_size_t dlen   /**< length of data to free */
);

/**
 * The qse_lda_comper_t type defines a key comparator that is called when
 * the list needs to compare data. A linear dynamic array is created with a
 * default comparator that performs bitwise comparison.
 *
 * The comparator should return 0 if the data are the same and a non-zero
 * integer otherwise.
 *
 */
typedef int (*qse_lda_comper_t) (
	qse_lda_t*  lda    /* array */, 
	const void* dptr1  /* data pointer */,
	qse_size_t  dlen1  /* data length */,
	const void* dptr2  /* data pointer */,
	qse_size_t  dlen2  /* data length */
);

/**
 * The qse_lda_keeper_t type defines a value keeper that is called when 
 * a value is retained in the context that it should be destroyed because
 * it is identical to a new value. Two values are identical if their beginning
 * pointers and their lengths are equal.
 */
typedef void (*qse_lda_keeper_t) (
	qse_lda_t* lda     /**< array */,
	void* vptr         /**< pointer to a value */,
	qse_size_t vlen    /**< length of a value */	
);

/**
 * The qse_lda_sizer_t type defines an array size claculator that is called
 * when the array needs to be resized. 
 */
typedef qse_size_t (*qse_lda_sizer_t) (
	qse_lda_t* lda,  /**< array */
	qse_size_t hint  /**< sizing hint */
);

typedef qse_lda_walk_t (*qse_lda_walker_t) (
	qse_lda_t*      lda   /* array */,
	qse_size_t      index /* index to the visited slot */,
	void*           ctx   /* user-defined context */
);

/**
 * The qse_lda_t type defines a linear dynamic array.
 */
struct qse_lda_t
{
	QSE_DEFINE_COMMON_FIELDS (lda)

	qse_lda_copier_t copier; /* data copier */
	qse_lda_freeer_t freeer; /* data freeer */
	qse_lda_comper_t comper; /* data comparator */
	qse_lda_keeper_t keeper; /* data keeper */
	qse_lda_sizer_t  sizer;  /* size calculator */
	qse_byte_t       scale;  /* scale factor */
	qse_size_t       size;   /* number of items */
	qse_size_t       capa;   /* capacity */
	qse_lda_slot_t** slot;
};

/**
 * The qse_lda_slot_t type defines a linear dynamic array slot
 */
struct qse_lda_slot_t
{
	qse_xptl_t val;
};

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (lda)

/**
 * The qse_lda_open() function creates a linear dynamic array.
 */
qse_lda_t* qse_lda_open (
	qse_mmgr_t* mmgr, /**< memory manager */
	qse_size_t  ext,  /**< extension size in bytes */
	qse_size_t  capa  /**< initial array capacity */
);

/**
 * The qse_lda_close() function destroys a linear dynamic array.
 */
void qse_lda_close (
	qse_lda_t* lda /**< array */
);

/**
 * The qse_lda_init() function initializes a linear dynamic array.
 */
int qse_lda_init (
	qse_lda_t*  lda,
	qse_mmgr_t* mmgr,
	qse_size_t  capa
);

/**
 * The qse_lda_fini() function finalizes a linear dynamic array.
 */
void qse_lda_fini (
	qse_lda_t* lda /**< array */
);

/**
 * The qse_lda_getscale() function returns the scale factor
 */
int qse_lda_getscale (
	qse_lda_t* lda   /**< array */
);

/**
 * The qse_lda_setscale() function sets the scale factor of the length
 * of a key and a value. A scale factor determines the actual length of
 * a key and a value in bytes. A lda is created with a scale factor of 1.
 * The scale factor should be larger than 0 and less than 256.
 * It is a bad idea to change the scale factor when @a lda is not empty.
 */
void qse_lda_setscale (
	qse_lda_t* lda   /**< array */,
	int scale        /**< scale factor */
);

qse_lda_copier_t qse_lda_getcopier (
	qse_lda_t* lda   /* array */
);

/**
 * The qse_lda_setcopier() specifies how to clone an element. The special 
 * copier #QSE_LDA_COPIER_INLINE copies the data inline to the internal slot.
 * No freeer is invoked when the slot is freeed. You may set the copier to 
 * #QSE_LDA_COPIER_SIMPLE to perform no special operation when the data 
 * pointer is stored.
 */
void qse_lda_setcopier (
	qse_lda_t* lda           /** lda */, 
	qse_lda_copier_t copier  /** element copier */
);

/**
 * The qse_lda_getfreeer() function returns a custom element destroyer.
 */
qse_lda_freeer_t qse_lda_getfreeer (
	qse_lda_t*   lda  /**< lda */
);

/**
 * The qse_lda_setfreeer() function specifies how to destroy an element.
 * The @a freeer is called when a slot containing the element is destroyed.
 */
void qse_lda_setfreeer (
	qse_lda_t* lda           /**< lda */,
	qse_lda_freeer_t freeer  /**< element freeer */
);

qse_lda_comper_t qse_lda_getcomper (
	qse_lda_t*   lda  /**< lda */
);

/**
 * The qse_lda_setcomper() function specifies how to compare two elements
 * for equality test. The comparator @a comper must return 0 if two elements
 * compared are equal, or a non-zero number otherwise.
 */
void qse_lda_setcomper (
	qse_lda_t*       lda     /**< lda */,
	qse_lda_comper_t comper  /**< comparator */
);

qse_lda_keeper_t qse_lda_getkeeper (
        qse_lda_t* lda
);

void qse_lda_setkeeper (
        qse_lda_t* lda,
        qse_lda_keeper_t keeper 
);

qse_lda_sizer_t qse_lda_getsizer (
        qse_lda_t* lda
);

void qse_lda_setsizer (
        qse_lda_t* lda,
        qse_lda_sizer_t sizer
);

qse_size_t qse_lda_getsize (
	qse_lda_t* lda
);

qse_size_t qse_lda_getcapa (
	qse_lda_t* lda
);

qse_lda_t* qse_lda_setcapa (
	qse_lda_t* lda,
	qse_size_t capa
);

qse_size_t qse_lda_search (
	qse_lda_t*  lda,
	qse_size_t  pos,
	const void* dptr,
	qse_size_t  dlen
);

qse_size_t qse_lda_rsearch (
	qse_lda_t*  lda,
	qse_size_t  pos,
	const void* dptr,
	qse_size_t  dlen
);

qse_size_t qse_lda_upsert (
	qse_lda_t* lda,
	qse_size_t index, 
	void*      dptr,
	qse_size_t dlen
);

qse_size_t qse_lda_insert (
	qse_lda_t* lda,
	qse_size_t index, 
	void*      dptr,
	qse_size_t dlen
);

qse_size_t qse_lda_update (
	qse_lda_t* lda,
	qse_size_t pos,
	void*      dptr,
	qse_size_t dlen
);

/**
 * The qse_lda_delete() function deletes the as many data as the count 
 * from the index. It returns the number of data deleted.
 */
qse_size_t qse_lda_delete (
	qse_lda_t* lda,
	qse_size_t index,
	qse_size_t count
);

/**
 *  The qse_lda_uplete() function deletes data slot without compaction.
 *  It returns the number of data affected.
 */
qse_size_t qse_lda_uplete (
	qse_lda_t* lda,
	qse_size_t index,
	qse_size_t count
);

void qse_lda_clear (
	qse_lda_t* lda
);

/**
 * The qse_lda_walk() function calls the @a walker function for each
 * element in the array beginning from the first. The @a walker function
 * should return one of #QSE_LDA_WALK_FORWARD, #QSE_LDA_WALK_BACKWARD,
 * #QSE_LDA_WALK_STOP.
 * @return number of calls to the @a walker fucntion made
 */
qse_size_t qse_lda_walk (
	qse_lda_t*       lda,
	qse_lda_walker_t walker,
	void*            ctx
);

/**
 * The qse_lda_rwalk() function calls the @a walker function for each
 * element in the array beginning from the last. The @a walker function
 * should return one of #QSE_LDA_WALK_FORWARD, #QSE_LDA_WALK_BACKWARD,
 * #QSE_LDA_WALK_STOP.
 * @return number of calls to the @a walker fucntion made
 */
qse_size_t qse_lda_rwalk (
	qse_lda_t*       lda,
	qse_lda_walker_t walker,
	void*            ctx
);

/**
 * The qse_lda_pushstack() function appends data to the array. It is a utility
 * function to allow stack-like operations over an array. To do so, you should
 * not play with other non-stack related functions.
 */
qse_size_t qse_lda_pushstack (
	qse_lda_t* lda,
	void*      dptr, 
	qse_size_t dlen
);

/**
 * The qse_lda_popstack() function deletes the last array data. It is a utility
 * function to allow stack-like operations over an array. To do so, you should
 * not play with other non-stack related functions. 
 * @note You must not call this function if @a lda is empty.
 */
void qse_lda_popstack (
	qse_lda_t* lda
);

/**
 * The qse_lda_pushheap() function inserts data to an array while keeping the
 * largest data at position 0. It is a utiltiy funtion to implement a binary
 * max-heap over an array. Inverse the comparator to implement a min-heap.
 * @return number of array elements
 * @note You must not mess up the array with other non-heap related functions
 *       to keep the heap property.
 */
qse_size_t qse_lda_pushheap (
	qse_lda_t* lda,
	void*      dptr,
	qse_size_t dlen
);

/**
 * The qse_lda_popheap() function deletes data at position 0 while keeping
 * the largest data at positon 0. It is a utiltiy funtion to implement a binary
 * max-heap over an array. 
 * @note You must not mess up the array with other non-heap related functions
 *       to keep the heap property.
 */
void qse_lda_popheap (
	qse_lda_t* lda
);

#ifdef __cplusplus
}
#endif

#endif
