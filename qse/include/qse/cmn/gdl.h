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

#ifndef _QSE_CMN_GDL_H_
#define _QSE_CMN_GDL_H_

/** @file
 * This file defins a generic double link and provides basic macros to 
 * manipulate a chain of links.
 */

/**
 * The qse_gdl_t type defines a structure to contain forward and 
 * backward links. 
 */
typedef struct qse_gdl_t qse_gdl_t;
struct qse_gdl_t
{
	qse_gdl_t* prev;
	qse_gdl_t* next;
};

/**
 * The QSE_GDL_INIT macro initializes a link to be used for internal
 * management.
 */
#define QSE_GDL_INIT(link) QSE_BLOCK ( \
	(link)->next = (link); (link)->prev = (link); \
)

/**
 * The QSE_GDL_CHAIN macro chains a new member node @a x between 
 * two nodes @a p and @a n.
 */
#define QSE_GDL_CHAIN(p,x,n) qse_gdl_chain(p,x,n)

/**
 * The QSE_GDL_UNCHAIN macro unchains a member node @a x.
 */
#define QSE_GDL_UNCHAIN(x) qse_gdl_unchain(x)

/**
 * The QSE_GDL_ISEMPTY macro checks if the chain is empty.
 */
#define QSE_GDL_ISEMPTY(link) ((link)->next == (link))

/**
 * The QSE_GDL_HEAD macro get the first node in the chain.
 */
#define QSE_GDL_HEAD(link) ((link)->next)

/**
 * The QSE_GDL_TAIL macro gets the last node in the chain.
 */
#define QSE_GDL_TAIL(link) ((link)->prev)

#ifdef __cplusplus
extern "C" {
#endif

void qse_gdl_chain (
	qse_gdl_t* p,
	qse_gdl_t* x,
	qse_gdl_t* n
);

void qse_gdl_unchain (
	qse_gdl_t* x
);

#ifdef __cplusplus
}
#endif

#endif
