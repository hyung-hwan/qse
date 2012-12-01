/*
 * $Id$
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

#ifndef _QSE_CMN_SLL_H_
#define _QSE_CMN_SLL_H_

#include <qse/types.h>
#include <qse/macros.h>

/** @file
 * This file provides a singly linked list interface.
 */

/**
 * The qse_sll_walk_t type defines a value that qse_sll_walker_t can return.
 */
enum qse_sll_walk_t
{
	QSE_SLL_WALK_STOP     = 0,  /**< stop traversal */
	QSE_SLL_WALK_FORWARD  = 1   /**< traverse to the next node */
};
typedef enum qse_sll_walk_t qse_sll_walk_t;


typedef struct qse_sll_t      qse_sll_t;
typedef struct qse_sll_node_t qse_sll_node_t;

/**
 * The qse_sll_copier_t type defines a callback function for node construction.
 * A node is contructed when a user adds data to a list. The user can
 * define how the data to add can be maintained in the list. A singly
 * linked list not specified with any copiers stores the data pointer and
 * the data length into a node. A special copier QSE_SLL_COPIER_INLINE copies 
 * the contents of the data a user provided into the node. You can use the
 * qse_sll_setcopier() function to change the copier. 
 *
 * A copier should return the pointer to the copied data. If it fails to copy
 * data, it may return QSE_NULL. You need to set a proper freeer to free up
 * memory allocated for copy.
 */
typedef void* (*qse_sll_copier_t) (
	qse_sll_t* sll,   /**< singly linked list */
	void*      dptr,  /**< pointer to data to copy */
	qse_size_t dlen   /**< length of data to copy */
);

/**
 * The qse_sll_freeer_t type defines a node destruction callback.
 */
typedef void (*qse_sll_freeer_t) (
	qse_sll_t* sll,   /**< singly linked list */
	void*      dptr,  /**< pointer to data to free */
	qse_size_t dlen   /**< length of data to free */
);

/**
 * The qse_sll_comper_t type defines a key comparator that is called when
 * the list needs to compare data. A singly linked list is created with a
 * default comparator that performs bitwise comparison.
 *
 * The comparator must return 0 if the data are the same and a non-zero
 * integer otherwise.
 */
typedef int (*qse_sll_comper_t) (
	qse_sll_t*  sll,   /**< singly linked list */
	const void* dptr1, /**< data pointer */
	qse_size_t  dlen1, /**< data length */
	const void* dptr2, /**< data pointer */
	qse_size_t  dlen2  /**< data length */
);

/**
 * The qse_sll_walker_t type defines a list traversal callback for each node.
 * The qse_sll_walk() calls a callback function of the type qse_sll_walker_t
 * for each node until it returns QSE_SLL_WALK_STOP. The walker should return
 * QSE_SLL_WALK_FORWARD  to let qse_sll_walk() continue visiting the next node.
 * The third parameter to qse_sll_walk() is passed to the walker as the third
 * parameter.
 */
typedef qse_sll_walk_t (*qse_sll_walker_t) (
	qse_sll_t*      sll,   /**< singly linked list */
	qse_sll_node_t* node,  /**< visited node */
	void*           ctx    /**< user-defined data */
);

/**
 * The qse_sll_t type defines a singly lnked list.
 */
struct qse_sll_t
{
	qse_mmgr_t* mmgr;

	qse_sll_copier_t copier; /**< data copier */
	qse_sll_freeer_t freeer; /**< data freeer */
	qse_sll_comper_t comper; /**< data comparator */
	qse_byte_t       scale;  /**< scale factor */

	qse_size_t       size;   /**< number of nodes */
	qse_sll_node_t*  head;   /**< head node */
	qse_sll_node_t*  tail;   /**< tail node */
};

/**
 * The qse_sll_node_t type defines a list node containing a data pointer and 
 * and data length. 
 */
struct qse_sll_node_t
{
	qse_sll_node_t* next; /* point to the next node */
	qse_xptl_t      val;
};

#define QSE_SLL_COPIER_SIMPLE ((qse_sll_copier_t)1)
#define QSE_SLL_COPIER_INLINE ((qse_sll_copier_t)2)

#define QSE_SLL_HEAD(sll)   ((sll)->head)
#define QSE_SLL_TAIL(sll)   ((sll)->tail)
#define QSE_SLL_SIZE(sll)   ((sll)->size)
#define QSE_SLL_SCALE(sll)  ((sll)->scale)

/**
 * The QSE_SLL_DPTR macro gets the data pointer in a node.
 */
#define QSE_SLL_DPTR(node)  ((node)->val.ptr)

/**
 * The QSE_SLL_DLEN macro gets the length of data in a node.
 */
#define QSE_SLL_DLEN(node)  ((node)->val.len)

/**
 * The QSE_SLL_NEXT macro gets the next node.
 */
#define QSE_SLL_NEXT(node)  ((node)->next)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The qse_sll_open() function creates an empty singly linked list.
 * If the memory manager mmgr is QSE_NULL, the function gets the default
 * memory manager with QSE_MMGR_GETDFL() and uses it if it is not QSE_NULL.
 * The extension area is allocated when the positive extension size extension 
 * is specified. It calls the extension initialization function initializer 
 * after initializing the main area. The extension initializer is passed
 * the pointer to the singly linked list created.
 *
 * @return pointer to a new singly linked list on success,
 *         QSE_NULL on failure
 *
 * @note
 *  In the debug build, it fails an assertion if QSE_MMGR_GETDFL() returns
 *  QSE_NULL and @a mmgr is QSE_NULL. In the release build, it returns QSE_NULL
 *  in such case.
 */
QSE_EXPORT qse_sll_t* qse_sll_open (
	qse_mmgr_t* mmgr, /* memory manager */ 
	qse_size_t  ext   /* size of extension area in bytes */
);

/**
 * The qse_sll_close() function destroys a singly linked list freeing up
 * the memory.
 */
QSE_EXPORT void qse_sll_close (
	qse_sll_t* sll /**< singly linked list */
);

/**
 * The qse_sll_init() function initializes a statically declared singly 
 * linked list. The memory should be allocated by a caller and be passed 
 * to it. The caller may declare a static variable of the qse_sll_t type
 * and pass its address. A memory manager still needs to be passed for 
 * node manipulation later.
 * @return 0 on success, -1 on failure
 *  The qse_sll_init() function returns the first parameter on success and
 *  QSE_NULL on failure.
 */
QSE_EXPORT int qse_sll_init (
	qse_sll_t*  sll,  /* singly linked list */
	qse_mmgr_t* mmgr  /* memory manager */
);

/**
 * The qse_sll_fini() function finalizes a statically initialized list.
 */
QSE_EXPORT void qse_sll_fini (
	qse_sll_t* sll  /**< singly linked list */
);

QSE_EXPORT qse_mmgr_t* qse_sll_getmmgr (
	qse_sll_t* sll
);

QSE_EXPORT void* qse_sll_getxtn (
	qse_sll_t* sll
);

/**
 * The qse_sll_getscale() function gets the scale factor
 */
QSE_EXPORT int qse_sll_getscale (
	qse_sll_t* sll  /**< singly linked list */
);

/**
 *  The qse_sll_setscale() function sets the scale factor of the data length.
 *  A scale factor determines the actual length of data in bytes. A singly 
 *  linked list created with a scale factor of 1. The scale factor should be
 *  larger than 0 and less than 256.
 */
QSE_EXPORT void qse_sll_setscale (
	qse_sll_t* sll,     /**< singly linked list */
	int        scale    /**< scale factor */
);

/**
 * The qse_sll_getfreeer() function gets the data copier.
 */
QSE_EXPORT qse_sll_copier_t qse_sll_getcopier (
	qse_sll_t* sll  /* singly linked list */
);

/**
 * The qse_sll_setcopier() function changes the element copier.
 * A special copier QSE_SLL_COPIER_INLINE is provided. This copier enables
 * you to copy the data inline to the internal node. No freeer is invoked
 * when the node is freeed. You may set the copier to QSE_NULL to perform 
 * no special operation when the data pointer is rememebered.
 */
QSE_EXPORT void qse_sll_setcopier (
	qse_sll_t*       sll,   /**< singly linked list */
	qse_sll_copier_t copier /**< data copier */
);

/**
 * The qse_sll_getfreeer() function returns the element freeer.
 */
QSE_EXPORT qse_sll_freeer_t qse_sll_getfreeer (
	qse_sll_t* sll  /**< singly linked list */
);

/**
 * The qse_sll_setfreeer() function changes the element freeer.
 * The freeer is called when a node containing the element is destroyed.
 */
QSE_EXPORT void qse_sll_setfreeer (
	qse_sll_t*       sll,   /**< singly linked list */
	qse_sll_freeer_t freeer /**< data freeer */
);

/**
 * The qse_sll_getcomper() function returns the data comparator.
 */
QSE_EXPORT qse_sll_comper_t qse_sll_getcomper (
	qse_sll_t* sll  /**< singly linked list */
);

/**
 * The qse_sll_setcomper() function changes  the data comparator
 */
QSE_EXPORT void qse_sll_setcomper (
	qse_sll_t*       sll,    /**< singly linked list */
	qse_sll_comper_t comper  /**< comparator */
);

/**
 * The qse_sll_getsize() function returns the number of the data nodes held
 * in a singly linked list.
 */
QSE_EXPORT qse_size_t qse_sll_getsize (
	qse_sll_t* sll  /** singly linked list */
);

/**
 * The qse_sll_gethead() function gets the head node. You may use the 
 * #QSE_SLL_HEAD macro instead.
 */
QSE_EXPORT qse_sll_node_t* qse_sll_gethead (
	qse_sll_t* sll  /**< a singly linked list */
);

/**
 * The qse_sll_gettail() function gets the head node. You may use the 
 * #QSE_SLL_TAIL macro instead.
 */
QSE_EXPORT qse_sll_node_t* qse_sll_gettail (
	qse_sll_t* sll  /**< singly linked list */
);

/**
 * The qse_sll_search() function traverses a list to find a node containing
 * the same value as the the data pointer and length. The traversal begins
 * from the next node of the positional node. If the positional node is 
 * QSE_NULL, the traversal begins from the head node. 
 *
 * Note that no reverse search is provided because a reverse traversal can not 
 * be achieved efficiently.
 *
 * @return pointer to the node found. QSE_NULL if no match is found
 */
QSE_EXPORT qse_sll_node_t* qse_sll_search (
	qse_sll_t*      sll,   /**< singly linked list */
	qse_sll_node_t* pos,   /**< positional node */
	const void*     dptr,  /**< data pointer */
	qse_size_t      dlen   /**< data length */
);

/**
 * The qse_sll_insert() function inserts data to the list @a sll.
 * There is performance penalty unless the positional node is neither
 * the head node nor QSE_NULL. You should consider a different data
 * structure such as a doubly linked list if you need to insert data
 * into a random position.
 * @return pointer to a new node on success, QSE_NULL on failure
 */
QSE_EXPORT qse_sll_node_t* qse_sll_insert (
	qse_sll_t*      sll,  /**< singly linked list */
	qse_sll_node_t* pos,  /**< node before which a new node is inserted */
	void*           dptr, /**< the pointer to the data */
	qse_size_t      dlen  /**< the length of the data */
);

/**
 * The qse_sll_delete() function deletes a node. 
 */
QSE_EXPORT void qse_sll_delete (
	qse_sll_t*      sll, /**< singly linked list */
	qse_sll_node_t* pos  /**< node to delete */
);

/**
 * The qse_sll_clear() function empties a singly linked list by deletinng
 * all the nodes.
 */
QSE_EXPORT void qse_sll_clear (
	qse_sll_t* sll  /**< singly linked list */
);

/**
 *  The qse_sll_walk() function traverses a singly linkked list from its 
 *  head node down to its tail node as long as the walker function returns 
 *  QSE_SLL_WALK_FORWARD . A walker can return QSE_SLL_WALK_STOP to cause 
 *  immediate stop of traversal.
 * 
 *  For each node, the walker function is called and it is passed three
 *  parameters: the singly linked list, the visiting node, and the 
 *  user-defined data passed as the third parameter in a call to the 
 *  qse_sll_walk() function.
 */
QSE_EXPORT void qse_sll_walk (
	qse_sll_t*       sll,     /**< singly linked list */
	qse_sll_walker_t walker,  /**< user-defined walker function */
	void*            ctx      /**< the pointer to user-defined data */
);

QSE_EXPORT qse_sll_node_t* qse_sll_pushhead (
	qse_sll_t* sll /**< singly linked list */,
	void*      dptr, 
	qse_size_t dlen
);

QSE_EXPORT qse_sll_node_t* qse_sll_pushtail (
	qse_sll_t* sll /**< singly linked list */, 
	void*      dptr, 
	qse_size_t dlen
);

QSE_EXPORT void qse_sll_pophead (
	qse_sll_t* sll
);

QSE_EXPORT void qse_sll_poptail (
	qse_sll_t* sll
);

#ifdef __cplusplus
}
#endif

#endif
