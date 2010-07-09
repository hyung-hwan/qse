/*
 * $Id$
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

#ifndef _QSE_CMN_RBT_H_
#define _QSE_CMN_RBT_H_

#include <qse/types.h>
#include <qse/macros.h>

enum qse_rbt_id_t
{
	QSE_RBT_KEY = 0,
	QSE_RBT_VAL = 1
};

typedef struct qse_rbt_t qse_rbt_t;
typedef struct qse_rbt_node_t qse_rbt_node_t;
typedef enum qse_rbt_id_t qse_rbt_id_t;

typedef void* (*qse_rbt_copier_t) (
	qse_rbt_t* rbt,  /**< red-black tree */
	void*      dptr, /**< the pointer to a key or a value */
	qse_size_t dlen  /**< the length of a key or a value */
);

typedef void (*qse_rbt_freeer_t) (
	qse_rbt_t* rbt,  /**< red-black tree */
	void*      dptr, /**< the pointer to a key or a value */
	qse_size_t dlen  /**< the length of a key or a value */
);

typedef int (*qse_rbt_comper_t) (
	qse_rbt_t*  rbt,    /**< red-black tree */
	const void* kptr1,  /**< the pointer to a key */
	qse_size_t  klen1,  /**< the length of a key */
	const void* kptr2,  /**< the pointer to a key */
	qse_size_t  klen2   /**< the length of a key */
);

struct qse_rbt_node_t
{
	int key;
	int value;

	enum 
	{
		QSE_RBT_RED,
		QSE_RBT_BLACK
	} color;

	qse_rbt_node_t* parent;
	qse_rbt_node_t* child[2]; /* left and right */
};

struct qse_rbt_t
{
	QSE_DEFINE_COMMON_FIELDS (rbt)

	qse_rbt_node_t nil; /**< internal nil node */

	qse_byte_t       scale[2];  /**< scale factor */

	qse_rbt_copier_t copier[2];
	qse_rbt_freeer_t freeer[2];
	qse_rbt_comper_t comper;

	qse_size_t        size;   /**< number of nodes */
     qse_rbt_node_t*   root;
};

enum qse_rbt_walk_t
{
	QSE_RBT_WALK_STOP = 0,
	QSE_RBT_WALK_FORWARD = 1
};

typedef enum qse_rbt_walk_t qse_rbt_walk_t;

/**
 * The qse_rbt_walker_t defines a pair visitor.
 */
typedef qse_rbt_walk_t (*qse_rbt_walker_t) (
	qse_rbt_t*      rbt,   /**< tree */
	qse_rbt_node_t* node,  /**< pointer to a node */
	void*           ctx    /**< pointer to user-defined context */
);

#define QSE_RBT_COPIER_SIMPLE ((qse_rbt_copier_t)1)
#define QSE_RBT_COPIER_INLINE ((qse_rbt_copier_t)2)

#ifdef __cplusplus
extern "C" {
#endif

qse_rbt_t* qse_rbt_open (
	qse_mmgr_t* mmgr, /**< memory manager */
	qse_size_t  ext   /**< size of extension area in bytes */
);

void qse_rbt_close (
	qse_rbt_t* rbt    /**< red-black tree */
);


qse_rbt_t* qse_rbt_init (
	qse_rbt_t*  rbt,  /**< red-black tree */
	qse_mmgr_t* mmgr  /**< a memory manager */
);

void qse_rbt_fini (
	qse_rbt_t* rbt    /**< red-black tree */
);

void qse_rbt_clear (
	qse_rbt_t* rbt    /**< red-black tree */
);

#ifdef __cplusplus
}
#endif


#endif
