/*
 * $Id: dll.h 287 2009-09-15 10:01:02Z hyunghwan.chung $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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

#ifndef _QSE_CMN_DLL_H_
#define _QSE_CMN_DLL_H_

#include <qse/types.h>
#include <qse/macros.h>

/*
 * Doubly Linked List
 */
typedef struct qse_dll_t qse_dll_t;
typedef struct qse_dll_node_t qse_dll_node_t;

/* data copier */
typedef void* (*qse_dll_copier_t) (qse_dll_t* dll, void* dptr, qse_size_t dlen);

/* data freeer */
typedef void (*qse_dll_freeer_t) (qse_dll_t* dll, void* dptr, qse_size_t dlen);

/* node visitor */
typedef int (*qse_dll_walker_t) (
	qse_dll_t* dll, qse_dll_node_t* node, void* arg);

struct qse_dll_t
{
	qse_mmgr_t* mmgr;

	qse_dll_copier_t copier;
	qse_dll_freeer_t freeer;

	qse_size_t size;
	qse_dll_node_t* head;
	qse_dll_node_t* tail;
};

struct qse_dll_node_t
{
	void* dptr; /* pointer to the beginning of data */
	qse_size_t dlen; /* length of data in bytes */
	qse_dll_node_t* next; /* pointer to the next node */
	qse_dll_node_t* prev; /* pointer to the prev node */
};


enum qse_dll_walk_t
{
	QSE_DLL_WALK_STOP = 0,
	QSE_DLL_WALK_FORWARD = 1
};

#define QSE_DLL_COPIER_INLINE qse_dll_copyinline

#define QSE_DLL_HEAD(dll) ((dll)->head)
#define QSE_DLL_TAIL(dll) ((dll)->tail)
#define QSE_DLL_SIZE(dll) ((dll)->size)

#define QSE_DLL_DPTR(n) ((n)->dptr)
#define QSE_DLL_DLEN(n) ((n)->dlen)
#define QSE_DLL_NEXT(n) ((n)->next)
#define QSE_DLL_PREV(n) ((n)->prev)

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * NAME: creates a doubly linked list with extension area
 *
 * DESCRIPTION:
 *  The qse_dll_open() function creates an empty doubly linked list.
 *  If the memory manager mmgr is QSE_NULL, the function gets the default
 *  memory manager with QSE_MMGR_GETMMGR() and uses it if it is not QSE_NULL.
 *  The extension area is allocated when the positive extension size extension 
 *  is specified. It calls the extension initialization function initializer 
 *  after initializing the main area. The extension initializer is passed
 *  the pointer to the doubly linked list created.
 *
 * RETURNS: 
 *  the pointer to a newly created doubly linked list on success.
 *  QSE_NULL on failure.
 *
 * WARNING:
 *  In the debug build, it fails the assertion if QSE_MMGR_SETMMGR() returns
 *  QSE_NULL when QSE_NULL is passed as the first parameter. In the release
 *  build, it returns QSE_NULL if such a thing happens. 
 */

qse_dll_t* qse_dll_open (
	qse_mmgr_t* mmgr /* memory manager */ , 
	qse_size_t ext /* size of extension area in bytes */
);

/* 
 * NAME destroys a singly linked list 
 */
void qse_dll_close (
	qse_dll_t* dll /* a singly linked list */
);

/* 
 * NAME deletes all elements of a singly linked list 
 */
void qse_dll_clear (
	qse_dll_t* dll /* a singly linked list */
);

/*
 * NAME specifies how to clone an element
 *
 * DESCRIPTION
 *  A special copier QSE_DLL_COPIER_INLINE is provided. This copier enables
 *  you to copy the data inline to the internal node. No freeer is invoked
 *  when the node is freeed.
 *
 *  You may set the copier to QSE_NULL to perform no special operation 
 *  when the data pointer is rememebered.
 */
void qse_dll_setcopier (
	qse_dll_t* dll /* a singly linked list */, 
	qse_dll_copier_t copier /* a element copier */
);

qse_dll_copier_t qse_dll_getcopier (
	qse_dll_t* dll /* a singly linked list */
);

/*
 * NAME specifies how to destroy an element
 *
 * DESCRIPTION
 *  The freeer is called when a node containing the element is destroyed.
 */
void qse_dll_setfreeer (
	qse_dll_t* dll /* a singly linked list */,
	qse_dll_freeer_t freeer /* a element freeer */
);

qse_dll_freeer_t qse_dll_getfreeer (
	qse_dll_t* dll /* a singly linked list */
);

/*
 * NAME Gets the pointer to the extension area
 * RETURN the pointer to the extension area
 */
void* qse_dll_getxtn (
	qse_dll_t* dll /* a singly linked list */
);

/*
 * NAME: get the pointer to the memory manager in use 
 */
qse_mmgr_t* qse_dll_getmmgr (
	qse_dll_t* dll /* a singly linked list */
);

void qse_dll_setmmgr (qse_dll_t* dll, qse_mmgr_t* mmgr);

/*
 * NAME Gets the number of elements held in a singly linked list
 * RETURN the number of elements the list holds
 */
qse_size_t qse_dll_getsize (
	qse_dll_t* dll /* a singly linked list */
);

/*
 * NAME Gets the head(first) node 
 * RETURN the tail node of a singly linked list
 */
qse_dll_node_t* qse_dll_gethead (
	qse_dll_t* dll /* a singly linked list */
);

/* 
 * NAME Gets the tail(last) node 
 * RETURN the tail node of a singly linked list
 */
qse_dll_node_t* qse_dll_gettail (
	qse_dll_t* dll /* a singly linked list */
);

/* 
 * NAME Inserts data before a positional node given 
 *
 * DESCRIPTION
 *   Inserts data.
 */
qse_dll_node_t* qse_dll_insert (
	qse_dll_t* dll /* a singly linked list */,
	qse_dll_node_t* pos /* a node before which a new node is inserted */,
	void* dptr /* the pointer to the data */ ,
	qse_size_t dlen /* the length of the data in bytes */
);

qse_dll_node_t* qse_dll_pushhead (
	qse_dll_t* dll /* a singly linked list */,
	void* dptr, 
	qse_size_t dlen
);

qse_dll_node_t* qse_dll_pushtail (
	qse_dll_t* dll /* a singly linked list */, 
	void* dptr, 
	qse_size_t dlen
);

void qse_dll_delete (
	qse_dll_t* dll, 
	qse_dll_node_t* pos
);

void qse_dll_pophead (
	qse_dll_t* dll
);

void qse_dll_poptail (
	qse_dll_t* dll
);

/* 
 * NAME Traverses s singly linked list 
 *
 * DESCRIPTION
 *   A singly linked list allows uni-directional in-order traversal.
 *   The qse_dll_walk() function traverses a singly linkked list from its 
 *   head node down to its tail node as long as the walker function returns 
 *   QSE_DLL_WALK_FORWARD. A walker can return QSE_DLL_WALK_STOP to cause 
 *   immediate stop of traversal.
 *   For each node, the walker function is called and it is passed three
 *   parameters: the singly linked list, the visiting node, and the 
 *   user-defined data passed as the third parameter in a call to the 
 *   qse_dll_walk() function.
 */
void qse_dll_walk (
	qse_dll_t* dll /* a singly linked list */,
	qse_dll_walker_t walker /* a user-defined walker function */,
	void* arg /* pointer to user-defined data */
);

/* 
 * Causes a singly linked list to copy in data to a node.
 * Use QSE_DLL_COPIER_INLINE instead.
 */
void* qse_dll_copyinline (
	qse_dll_t* dll /* a singly linked list */,
	void* data /* pointer to data to copy */ , 
	qse_size_t len /* length of data in bytes */
);

#ifdef __cplusplus
}
#endif

#endif
