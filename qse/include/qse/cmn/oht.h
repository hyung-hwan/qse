/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope toht it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _QSE_OHT_T_
#define _QSE_OHT_T_

/** @file
 * This file provides the open-addressed hash table for fixed-size data.
 */

#include <qse/types.h>
#include <qse/macros.h>

/**
 * The #QSE_OHT_NIL macro represents an invalid index.
 */
#define QSE_OHT_NIL ((qse_size_t)-1)

/**
 * The #QSE_OHT_SIZE macro returns the number of items.
 */
#define QSE_OHT_SIZE(oht)  ((const qse_size_t)(oht)->size)

/**
 * The #QSE_OHT_CAPA macro returns the capacity of a table.
 */
#define QSE_OHT_CAPA(oht)  ((const qse_size_t)(oht)->capa.hard)

/**
 * The #QSE_OHT_LIMIT macro returns the maximum number of items.
 * It is equal to or less than the capacity.
 */
#define QSE_OHT_LIMIT(oht) ((const qse_size_t)(oht)->capa.soft)

/**
 * The #QSE_OHT_SCALE macro returns the size of an item in bytes.
 */
#define QSE_OHT_SCALE(oht) ((const int)(oht)->scale)

/**
 * The #qse_oht_mark_t type defines enumerations values to indicate
 * the slot status.
 */
enum qse_oht_mark_t
{
	QSE_OHT_EMPTY    = 0,
	QSE_OHT_OCCUPIED = 1 /*,
	QSE_OHT_DELETED  = 2 */
};
typedef enum qse_oht_mark_t qse_oht_mark_t;

/**
 * The #qse_oht_walk_t type defines walking directions.
 */
enum qse_oht_walk_t
{
	QSE_OHT_WALK_STOP     = 0,
	QSE_OHT_WALK_FORWARD  = 1,
};
typedef enum qse_oht_walk_t qse_oht_walk_t;

typedef struct qse_oht_t qse_oht_t;

/**
 * The #qse_oht_hasher_t type defines a data hasher function.
 */
typedef qse_size_t (*qse_oht_hasher_t) (
	qse_oht_t*  oht,
	const void* data
);

/**
 * The #qse_oht_comper_t type defines a key comparator that is called when
 * the list needs to compare data.  The comparator must return 0 if the data
 * are the same and a non-zero integer otherwise.
 */
typedef int (*qse_oht_comper_t) (
	qse_oht_t*  oht,   /**< open-addressed hash table */
	const void* data1, /**< data pointer */
	const void* data2  /**< data pointer */
);

/**
 * The #qse_oht_copier_t type defines a data copier function.
 */
typedef void (*qse_oht_copier_t) (
	qse_oht_t*  oht,
	void*       dst,
	const void* src 
);

/**
 * The #qse_oht_t type defines an open-address hash table for fixed-size data.
 * Unlike #qse_rbt_t or #qse_htb_t, it does not separate a key from a value.
 */
struct qse_oht_t
{
	qse_mmgr_t* mmgr;

	int scale;
	struct
	{
		qse_size_t hard;
		qse_size_t soft;
	} capa;
	qse_size_t size;

	qse_oht_hasher_t hasher;
	qse_oht_comper_t comper;
	qse_oht_copier_t copier;

	qse_oht_mark_t* mark;
	void*           data;
};

/**
 * The #qse_oht_walker_t function defines a callback function 
 * for qse_oht_walk().
 */
typedef qse_oht_walk_t (*qse_oht_walker_t) (
	qse_oht_t* oht,
	void*      data,
	void*      ctx
);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The qse_oht_open() function creates an open-addressed hash table.
 */
QSE_EXPORT qse_oht_t* qse_oht_open (
	qse_mmgr_t* mmgr,
	qse_size_t  xtnsize,
	int         scale,
	qse_size_t  capa,
	qse_size_t  limit
);

/**
 * The qse_oht_close() function destroys an open-addressed hash table.
 */
QSE_EXPORT void qse_oht_close (
	qse_oht_t* oht  /**< open-addressed hash table */
);

/**
 * The qse_oht_open() function initializes an open-addressed hash table.
 */
QSE_EXPORT int qse_oht_init (
	qse_oht_t*  oht,
	qse_mmgr_t* mmgr,
	int         scale,
	qse_size_t  capa,
	qse_size_t  limit
);

/**
 * The qse_oht_close() function finalizes an open-addressed hash table.
 */
QSE_EXPORT void qse_oht_fini (
	qse_oht_t* oht  /**< open-addressed hash table */
);

QSE_EXPORT qse_mmgr_t* qse_oht_getmmgr (
	qse_oht_t* oht
);

QSE_EXPORT void* qse_oht_getxtn (
	qse_oht_t* oht
);

/**
 * The qse_oht_getcomper() function returns the data hasher.
 */
QSE_EXPORT qse_oht_hasher_t qse_oht_gethasher (
	qse_oht_t* oht  /**< open-addressed hash table */
);

/**
 * The qse_oht_setcomper() function changes the data hasher
 */
QSE_EXPORT void qse_oht_sethasher (
	qse_oht_t*       oht,    /**< open-addressed hash table */
	qse_oht_hasher_t hasher  /**< hasher */
);

/**
 * The qse_oht_getcomper() function returns the data comparator.
 */
QSE_EXPORT qse_oht_comper_t qse_oht_getcomper (
	qse_oht_t* oht  /**< open-addressed hash table */
);

/**
 * The qse_oht_setcomper() function changes the data comparator
 */
QSE_EXPORT void qse_oht_setcomper (
	qse_oht_t*       oht,    /**< open-addressed hash table */
	qse_oht_comper_t comper  /**< comparator */
);

/**
 * The qse_oht_getcomper() function returns the data copier.
 */
QSE_EXPORT qse_oht_copier_t qse_oht_getcopier (
	qse_oht_t*       oht     /**< open-addressed hash table */
);

/**
 * The qse_oht_setcomper() function changes the data copier.
 */
QSE_EXPORT void qse_oht_setcopier (
	qse_oht_t*       oht,    /**< open-addressed hash table */
	qse_oht_copier_t copier  /**< copier */
);

/**
 * The qse_oht_search() function searches a hash table to find a 
 * matching datum. It returns the index to the slot of an internal array
 * where the matching datum is found. 
 * @return slot index if a match if found,
 *         #QSE_OHT_NIL if no match is found.
 */
QSE_EXPORT qse_size_t qse_oht_search (
	qse_oht_t*       oht,    /**< open-addressed hash table */
	void*            data    /**< data pointer */
);

/**
 * The qse_oht_insert() function inserts a new datum. It fails if it finds
 * an existing datum.
 * @return slot index where the new datum is inserted on success,
 *         #QSE_OHT_NIL on failure.
 */
QSE_EXPORT qse_size_t qse_oht_insert (
	qse_oht_t*       oht,    /**< open-addressed hash table */
	const void*      data    /**< data pointer */
);

/**
 * The qse_oht_upsert() function inserts a new datum if it finds no matching
 * datum or updates an exsting datum if finds a matching datum.
 * @return slot index where the datum is inserted or updated.
 */
QSE_EXPORT qse_size_t qse_oht_upsert (
	qse_oht_t*       oht,    /**< open-addressed hash table */
	const void*      data    /**< data pointer */
);

/**
 * The qse_oht_update() function updates an existing datum. It fails if it finds
 * no existing datum.
 * @return slot index where an existing datum is updated on success,
 *         #QSE_OHT_NIL on failure.
 */
QSE_EXPORT qse_size_t qse_oht_update (
	qse_oht_t*       oht,    /**< open-addressed hash table */
	const void*      data    /**< data pointer */
);

/**
 * The qse_oht_delete() function deletes an existing datum. It fails if it finds
 * no existing datum.
 * @return slot index where an existing datum is deleted on success,
 *         #QSE_OHT_NIL on failure.
 */
QSE_EXPORT qse_size_t qse_oht_delete (
	qse_oht_t*       oht,    /**< open-addressed hash table */
	const void*      data    /**< data pointer */
);

/**
 * The qse_oht_clear() functions deletes all data items.
 */
QSE_EXPORT void qse_oht_clear (
	qse_oht_t*       oht     /**< open-addressed hash table */
);

/**
 * The qse_oht_walk() function executes the callback function @a walker for
 * each valid data item.
 */
QSE_EXPORT void qse_oht_walk (
	qse_oht_t*       oht,    /**< open-addressed hash table */
	qse_oht_walker_t walker, /**< callback function */
	void*            ctx     /**< context */
);

#ifdef __cplusplus
}
#endif

#endif
