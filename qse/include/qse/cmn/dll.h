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

#ifndef _QSE_CMN_DLL_H_
#define _QSE_CMN_DLL_H_

/** @file
 * This file defines a doubly linked list interface.
 */

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/gdl.h>

#define QSE_DLL_SIZE(dll) ((const qse_size_t)(dll)->size)

/* ----------- more general implementation below ----------------- */
enum qse_dll_walk_t
{
	QSE_DLL_WALK_STOP     = 0,
	QSE_DLL_WALK_FORWARD  = 1,
	QSE_DLL_WALK_BACKWARD = 2
};
typedef enum qse_dll_walk_t qse_dll_walk_t;

typedef struct qse_dll_t qse_dll_t;
typedef struct qse_dll_node_t qse_dll_node_t;

/* data copier */
typedef void* (*qse_dll_copier_t) (
	qse_dll_t* dll,
	void*      dptr,
	qse_size_t dlen
);

/* data freeer */
typedef void (*qse_dll_freeer_t) (
	qse_dll_t* dll,
	void*      dptr,
	qse_size_t dlen
);

/**
 * The qse_dll_comper_t type defines a key comparator that is called when
 * the list needs to compare data. A doubly linked list is created with a
 * default comparator that performs bitwise comparison.
 *
 * The comparator must return 0 if the data are the same and a non-zero
 * integer otherwise.
 */
typedef int (*qse_dll_comper_t) (
	qse_dll_t*  dll,   /**< doubly linked list */
	const void* dptr1, /**< data pointer */
	qse_size_t  dlen1, /**< data length */
	const void* dptr2, /**< data pointer */
	qse_size_t  dlen2  /**< data length */
);

/* node visitor */
typedef qse_dll_walk_t (*qse_dll_walker_t) (
	qse_dll_t*      dll, 
	qse_dll_node_t* node,
	void*           ctx
);

/**
 * The qse_dll_node_t type defines a doubly linked list node.
 */
struct qse_dll_node_t
{
	/* the first two fields in sync with qse_gdl_t */
	/*qse_dll_node_t* prev;
	qse_dll_node_t* next;*/
	qse_gdl_link_t  link;
	/* data */
	qse_xptl_t      val;
};

/**
 * The qse_dll_t type defines a doubly linked list.
 */
struct qse_dll_t
{
	qse_mmgr_t* mmgr;

	/*qse_dll_node_t gdl;*/
	qse_gdl_t      gdl;
	qse_size_t     size;

	qse_byte_t       scale;  
	qse_dll_copier_t copier; 
	qse_dll_freeer_t freeer;
	qse_dll_comper_t comper;
};

#define QSE_DLL_COPIER_SIMPLE ((qse_dll_copier_t)1)
#define QSE_DLL_COPIER_INLINE ((qse_dll_copier_t)2)

#define QSE_DLL_SCALE(dll) ((const int)(dll)->scale)

#define QSE_DLL_DPTR(node) ((node)->val.ptr)
#define QSE_DLL_DLEN(node) ((node)->val.len)

#if defined(__cplusplus)
extern "C" {
#endif

/** 
 *  The qse_dll_open() function creates an empty doubly linked list.
 *  If the memory manager mmgr is QSE_NULL, the function gets the default
 *  memory manager with QSE_MMGR_GETMMGR() and uses it if it is not QSE_NULL.
 *  The extension area is allocated when the positive extension size extension 
 *  is specified. It calls the extension initialization function initializer 
 *  after initializing the main area. The extension initializer is passed
 *  the pointer to the doubly linked list created.
 *
 * @return
 *  the pointer to a newly created doubly linked list on success.
 *  QSE_NULL on failure.
 */
QSE_EXPORT qse_dll_t* qse_dll_open (
	qse_mmgr_t* mmgr,   /**< memory manager */ 
	qse_size_t  xtnsize /**< size of extension area in bytes */
);

/** 
 * The qse_dll_close() function destroys a doubly linked list.
 */
QSE_EXPORT void qse_dll_close (
	qse_dll_t* dll /** doubly linked list */
);

/**
 * The qse_dll_init() function initializes a statically declared list.
 */
QSE_EXPORT int qse_dll_init (
	qse_dll_t*  dll, /**< doubly linked list */
	qse_mmgr_t* mmgr /**< memory manager */
);

/**
 * The qse_dll_fini() function finalizes a statically initialized list.
 */
QSE_EXPORT void qse_dll_fini (
	qse_dll_t* dll /**< doubly linked list */
);

QSE_EXPORT qse_mmgr_t* qse_dll_getmmgr (
	qse_dll_t* dll
);

QSE_EXPORT void* qse_dll_getxtn (
	qse_dll_t* dll
);

/**
 * The qse_dll_getscale() function gets the scale factor
 */
QSE_EXPORT int qse_dll_getscale (
	qse_dll_t* dll  /**< doubly linked list */
);

/**
 *  The qse_dll_setscale() function sets the scale factor of the data length.
 *  A scale factor determines the actual length of data in bytes. A doubly 
 *  linked list created with a scale factor of 1. The scale factor should be
 *  larger than 0 and less than 256.
 */
QSE_EXPORT void qse_dll_setscale (
	qse_dll_t* dll,     /**< doubly linked list */
	int        scale    /**< scale factor */
);

/**
 * The qse_dll_setcopier() function changes the element copier.
 * A special copier QSE_DLL_COPIER_INLINE is provided. This copier enables
 * you to copy the data inline to the internal node. No freeer is invoked
 * when the node is freeed. You may set the copier to QSE_NULL to perform
 * no special operation when the data pointer is rememebered.
 */
QSE_EXPORT void qse_dll_setcopier (
	qse_dll_t*       dll,   /**< doubly linked list */
	qse_dll_copier_t copier /**< element copier */
);

/**
 * The qse_dll_getcopier() function returns the element copier.
 */
QSE_EXPORT qse_dll_copier_t qse_dll_getcopier (
	qse_dll_t* dll /**< doubly linked list */
);

/**
 * The qse_dll_setfreeer() function changes the element freeer.
 * The freeer is called when a node containing the element is destroyed.
 */
QSE_EXPORT void qse_dll_setfreeer (
	qse_dll_t*       dll,   /**< doubly linked list */
	qse_dll_freeer_t freeer /**< element freeer */
);

/**
 * The qse_dll_getfreeer() function returns the element freeer.
 */
QSE_EXPORT qse_dll_freeer_t qse_dll_getfreeer (
	qse_dll_t* dll /**< doubly linked list */
);

/**
 * The qse_dll_getcomper() function returns the data comparator.
 */
QSE_EXPORT qse_dll_comper_t qse_dll_getcomper (
	qse_dll_t* dll  /**< doubly linked list */
);

/**
 * The qse_dll_setcomper() function changes the data comparator
 */
QSE_EXPORT void qse_dll_setcomper (
	qse_dll_t*       dll,    /**< doubly linked list */
	qse_dll_comper_t comper  /**< comparator */
);

/**
 * The qse_dll_getsize() function returns the number of the data nodes held
 * in a doubly linked list.
 */
QSE_EXPORT qse_size_t qse_dll_getsize (
	qse_dll_t* dll  /**< doubly linked list */
);

/**
 * The qse_dll_gethead() function gets the head node. You may use the 
 * #QSE_DLL_HEAD macro instead.
 */
QSE_EXPORT qse_dll_node_t* qse_dll_gethead (
	qse_dll_t* dll  /**< doubly linked list */
);

/**
 * The qse_dll_gettail() function gets the head node. You may use the 
 * #QSE_DLL_TAIL macro instead.
 */
QSE_EXPORT qse_dll_node_t* qse_dll_gettail (
	qse_dll_t* dll  /**< doubly linked list */
);


QSE_EXPORT qse_dll_node_t* qse_dll_search (
	qse_dll_t*      dll,   /**< doubly linked list */
	qse_dll_node_t* pos,   /**< positional node */
	const void*     dptr,  /**< data pointer */
	qse_size_t      dlen   /**< data length */
);

QSE_EXPORT qse_dll_node_t* qse_dll_rsearch (
	qse_dll_t*      dll,   /**< doubly linked list */
	qse_dll_node_t* pos,   /**< positional node */
	const void*     dptr,  /**< data pointer */
	qse_size_t      dlen   /**< data length */
);

/**
 * The qse_dll_insert() function insert an element into a list 
 */
QSE_EXPORT qse_dll_node_t* qse_dll_insert (
	qse_dll_t*      dll,  /**< doubly linked list */
	qse_dll_node_t* pos,  /**< node before which a new node is inserted */
	void*           dptr, /**< data pointer */
	qse_size_t      dlen  /**< data length */
);

QSE_EXPORT void qse_dll_delete (
	qse_dll_t*      dll, 
	qse_dll_node_t* pos
);

/** 
 * The qse_dll_clear() functions deletes all elements of a list
 */
QSE_EXPORT void qse_dll_clear (
	qse_dll_t* dll /**< doubly linked list */
);

QSE_EXPORT void qse_dll_walk (
	qse_dll_t*       dll,    /**< doubly linked list */
	qse_dll_walker_t walker, /**< user-defined walker function */
	void*            ctx     /**< pointer to user-defined data */
);

QSE_EXPORT void qse_dll_rwalk (
	qse_dll_t*       dll,    /**< doubly linked list */
	qse_dll_walker_t walker, /**< user-defined walker function */
	void*            ctx     /**< pointer to user-defined data */
);

QSE_EXPORT qse_dll_node_t* qse_dll_pushhead (
	qse_dll_t* dll, /* doubly linked list */
	void*      dptr, 
	qse_size_t dlen
);

QSE_EXPORT qse_dll_node_t* qse_dll_pushtail (
	qse_dll_t* dll, /* doubly linked list */ 
	void*      dptr, 
	qse_size_t dlen
);

QSE_EXPORT void qse_dll_pophead (
	qse_dll_t* dll
);

QSE_EXPORT void qse_dll_poptail (
	qse_dll_t* dll
);

#if defined(__cplusplus)
}
#endif

#endif
