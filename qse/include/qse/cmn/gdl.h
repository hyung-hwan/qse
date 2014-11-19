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

#ifndef _QSE_CMN_GDL_H_
#define _QSE_CMN_GDL_H_

#include <qse/types.h>
#include <qse/macros.h>

/** @file
 * This file defines a generic doubly linked list.
 *
 * When the list is empty, the primary node points to itself.
 *   +-------+
 *   |  gdl  |
 *   | tail --------+
 *   | head ----+   |
 *   +-------+  |   |
 *    ^  ^      |   |
 *    |  +------+   |
 *    +-------------+
 *
 *  The list contains 1 item.
 *      +----------------------------+
 *      V                            |
 *   +-------+            +-------+  |
 *   |  gdl  |            | first |  |
 *   | tail ------------> | prev ----+
 *   | head ------------> | next ----+
 *   +-------+            +-------+  |
 *       ^                           | 
 *       +---------------------------+
 *
 *  The list contains 2 item.
 *      +----------------------------+
 *      V                            |  +--------------------+
 *   +-------+            +-------+  |  |       +--------+   |
 *   |  gdl  |            | first | <---+       | second |   |
 *   | tail ------+       | prev ----+          | prev ------+
 *   | head ------|-----> | next -------------> | next ------+
 *   +-------+    |       +-------+             +--------+   |
 *    ^           |                                 ^        |
 *    |           +---------------------------------+        |
 *    +------------------------------------------------------+
 *
 * gdl's tail points to the last item.
 * gdl's head points to the first item.
 * the last item's next points to gdl.
 * the first item's prev points to gdl. 
 *
 *
 * \code
 * #include <qse/cmn/gdl.h>
 *
 * struct q
 * {
 *     int x;
 *     qse_gdl_link_t rr;
 * };
 *
 * int main ()
 * {
 *     struct q a, b, c, d, e;
 *     qse_gdl_link_t* x;
 *     qse_gdl_t rr;
 *
 *     a.x = 1;
 *     b.x = 2;
 *     c.x = 3;
 *     d.x = 4;
 *     e.x = 5;
 *
 *     QSE_GDL_INIT (&rr);
 *
 *     QSE_GDL_APPEND (&rr, &c.rr);
 *     QSE_GDL_APPEND (&rr, &b.rr);
 *     QSE_GDL_PREPEND (&rr, &a.rr);
 *     QSE_GDL_APPEND (&rr, &d.rr);
 *     QSE_GDL_REPLACE (&rr, &b.rr, &e.rr);
 *     QSE_GDL_DELETE (&rr, QSE_GDL_TAIL(&rr));
 *
 *     for (x = QSE_GDL_HEAD(&rr); QSE_GDL_ISLINK(&rr,x); x = QSE_GDL_NEXT(x))
 *     {
 *         struct q* qq = QSE_GDL_CONTAINER(x,struct q,rr);
 *         // do something here 
 *    }
 * 
 *    return 0;
 * }
 * \endcode
 */

/**
 * The qse_gdl_t type defines a structure to contain the pointers to 
 * the head and the tail links. It maintains the same layout as the 
 * #qse_gdl_link_t type despite different member names.
 */
typedef struct qse_gdl_t qse_gdl_t;

/**
 * The qse_gdl_link_t type defines a structure to contain forward and 
 * backward links. It maintains the same layout as the #qse_gdl_t type
 * despite different member names.
 */
typedef struct qse_gdl_link_t qse_gdl_link_t;

struct qse_gdl_t
{
	qse_gdl_link_t* tail;  /* this maps to prev of qse_gdl_link_t */
	qse_gdl_link_t* head;  /* this maps to next of qse_gdl_link_t */
};

struct qse_gdl_link_t
{
	qse_gdl_link_t* prev; /* this maps to tail of qse_gdl_t */
	qse_gdl_link_t* next; /* this maps to head of qse_gdl_t */
};

/**
 * The QSE_GDL_INIT macro initializes a link to be used for internal
 * management.
 */
#define QSE_GDL_INIT(gdl) QSE_BLOCK ( \
	(gdl)->head = (qse_gdl_link_t*)(gdl); \
	(gdl)->tail = (qse_gdl_link_t*)(gdl); \
)

#define QSE_GDL_NEXT(link) ((link)->next)
#define QSE_GDL_PREV(link) ((link)->prev)
#define QSE_GDL_CONTAINER(link,type,name) \
	((type*)(((qse_uint8_t*)link) - QSE_OFFSETOF(type,name)))

/**
 * The QSE_GDL_HEAD macro get the first node in the chain.
 */
#define QSE_GDL_HEAD(gdl) ((gdl)->head)

/**
 * The QSE_GDL_TAIL macro gets the last node in the chain.
 */
#define QSE_GDL_TAIL(gdl) ((gdl)->tail)

/**
 * The QSE_GDL_CHAIN macro chains a new member node \a x between 
 * two nodes \a p and \a n.
 */
#define QSE_GDL_CHAIN(gdl,p,x,n) qse_gdl_chain(gdl,p,x,n)

/**
 * The QSE_GDL_UNCHAIN macro unchains a member node \a x.
 */
#define QSE_GDL_UNCHAIN(gdl,x) qse_gdl_unchain(gdl,x)

#define QSE_GDL_APPEND(gdl,x) \
	qse_gdl_chain (gdl, QSE_GDL_TAIL(gdl), (x), (qse_gdl_link_t*)(gdl))

#define QSE_GDL_PREPEND(gdl,x) \
	qse_gdl_chain (gdl, (qse_gdl_link_t*)(gdl), (x), QSE_GDL_HEAD(gdl))

#define QSE_GDL_REPLACE(gdl,x,y) qse_gdl_replace (gdl, x, y)

#define QSE_GDL_DELETE(gdl,x) QSE_GDL_UNCHAIN(gdl,x)

#define QSE_GDL_ISEMPTY(gdl) ((gdl)->head == (qse_gdl_link_t*)(gdl))
#define QSE_GDL_ISLINK(gdl,x) ((gdl) != (qse_gdl_t*)(x))
#define QSE_GDL_ISHEAD(gdl,x) ((x)->prev == (qse_gdl_link_t*)(gdl))
#define QSE_GDL_ISTAIL(gdl,x) ((x)->next == (qse_gdl_link_t*)(gdl))

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT void qse_gdl_chain (
	qse_gdl_t*      gdl,
	qse_gdl_link_t* prev,
	qse_gdl_link_t* x,
	qse_gdl_link_t* next
);

QSE_EXPORT void qse_gdl_unchain (
	qse_gdl_t*      gdl,
	qse_gdl_link_t* x
);

QSE_EXPORT void qse_gdl_replace (
	qse_gdl_t*      gdl,
	qse_gdl_link_t* old_link,
	qse_gdl_link_t* new_link 
);

#if defined(__cplusplus)
}
#endif

#endif
