/*
 * $Id$
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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

/** @file
 * This file provides the open-addressed hash table for fixed-size data.
 */
#ifndef _QSE_OHT_T_
#define _QSE_OHT_T_

#include <qse/types.h>
#include <qse/macros.h>

#define QSE_OHT_INVALID_INDEX ((qse_size_t)-1)

enum  qse_oht_mark_t
{
	QSE_OHT_EMPTY    = 0,
	QSE_OHT_OCCUPIED = 1 /*,
	QSE_OHT_DELETED  = 2 */
};
typedef enum qse_oht_mark_t qse_oht_mark_t;

enum qse_oht_walk_t
{
	QSE_OHT_WALK_STOP     = 0,
	QSE_OHT_WALK_FORWARD  = 1,
};
typedef enum qse_oht_walk_t qse_oht_walk_t;

typedef struct qse_oht_t qse_oht_t;

/**
 * The qse_oht_comper_t type defines a key comparator that is called when
 * the list needs to compare data.  The comparator must return 0 if the data
 * are the same and a non-zero integer otherwise.
 */
typedef int (*qse_oht_comper_t) (
	qse_oht_t*  oht,   /**< open-addressed hash table */
	const void* data1, /**< data pointer */
	const void* data2  /**< data pointer */
);

typedef void (*qse_oht_copier_t) (
	qse_oht_t*  oht,
	void*       dst,
	const void* src 
);

typedef qse_size_t (*qse_oht_hasher_t) (
	qse_oht_t*  oht,
	const void* data
);

struct qse_oht_t
{
	QSE_DEFINE_COMMON_FIELDS(oht)

	struct
	{
		qse_size_t hard;
		qse_size_t soft;
	} capa;
	qse_size_t size;
	qse_size_t scale;

	qse_oht_hasher_t hasher;
	qse_oht_comper_t comper;
	qse_oht_copier_t copier;

	qse_oht_mark_t* mark;
	void*           data;
};

typedef qse_oht_walk_t (*qse_oht_walker_t) (
	qse_oht_t* oht,
	void*      data,
	void*      ctx
);

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (oht)

qse_oht_t* qse_oht_open (
	qse_mmgr_t* mmgr,
	qse_size_t  xtnsize,
	qse_size_t  scale,
	qse_size_t  capa,
	qse_size_t  limit
);

void qse_oht_close (
	qse_oht_t* oht
);

qse_oht_t* qse_oht_init (
	qse_oht_t*  oht,
	qse_mmgr_t* mmgr,
	qse_size_t  scale,
	qse_size_t  capa,
	qse_size_t  limit
);

void qse_oht_fini (
	qse_oht_t* oht
);

qse_oht_hasher_t qse_oht_gethasher (
	qse_oht_t* oht
);

void qse_oht_sethasher (
	qse_oht_t*       oht,
	qse_oht_hasher_t hahser
);

qse_oht_comper_t qse_oht_getcomper (
	qse_oht_t* oht
);

void qse_oht_setcomper (
	qse_oht_t*       oht,
	qse_oht_comper_t hahser
);

qse_oht_copier_t qse_oht_getcopier (
	qse_oht_t* oht
);

void qse_oht_setcopier (
	qse_oht_t*       oht,
	qse_oht_copier_t hahser
);

qse_size_t qse_oht_search (
	qse_oht_t* oht,
	void*      data
);

qse_size_t qse_oht_insert (
	qse_oht_t*  oht,
	const void* data
);

qse_size_t qse_oht_upsert (
	qse_oht_t*  oht,
	const void* data
);

qse_size_t qse_oht_update (
	qse_oht_t*  oht,
	const void* data
);

qse_size_t qse_oht_delete (
	qse_oht_t*  oht,
	const void* data
);

void qse_oht_clear (
	qse_oht_t* oht
);


void qse_oht_walk (
	qse_oht_t*       oht,    /**< open-addressed hash table */
	qse_oht_walker_t walker, /**< walker function */
	void*            ctx     /**< context */
);

#ifdef __cplusplus
}
#endif

#endif
