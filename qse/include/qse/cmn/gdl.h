/*
 * $Id$
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
 * The QSE_GDL_INIT macro initializes a host link to be used for internal
 * management.
 */
#define QSE_GDL_INIT(host) QSE_BLOCK ( \
	(host)->next = (host); (host)->prev = (host); \
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
#define QSE_GDL_ISEMPTY(host) ((host)->next == (host))

/**
 * The QSE_GDL_HEAD macro get the first node in the chain.
 */
#define QSE_GDL_HEAD(host) ((host)->next)

/**
 * The QSE_GDL_TAIL macro gets the last node in the chain.
 */
#define QSE_GDL_TAIL(host) ((host)->prev)


#ifdef __cplusplus
extern "C" {
#endif

void qse_gdl_chain (qse_gdl_t* p, qse_gdl_t* x, qse_gdl_t* n);
void qse_gdl_unchain (qse_gdl_t* x);

#ifdef __cplusplus
}
#endif

#endif
