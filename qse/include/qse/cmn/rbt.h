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

/**@file
 * A red-black tree is a self-balancing binary search tree.
 */

typedef struct qse_rbt_t qse_rbt_t;
typedef struct qse_rbt_pair_t qse_rbt_pair_t;

/** 
 * The qse_rbt_walk_t type defines values that the callback function can
 * return to control qse_rbt_walk() and qse_rbt_rwalk().
 */
enum qse_rbt_walk_t
{
        QSE_RBT_WALK_STOP    = 0,
        QSE_RBT_WALK_FORWARD = 1
};
typedef enum qse_rbt_walk_t qse_rbt_walk_t;

/**
 * The qse_rbt_id_t type defines IDs to indicate a key or a value in various
 * functions
 */
enum qse_rbt_id_t
{
	QSE_RBT_KEY = 0, /**< indicate a key */
	QSE_RBT_VAL = 1  /**< indicate a value */
};
typedef enum qse_rbt_id_t qse_rbt_id_t;

/**
 * The qse_rbt_copier_t type defines a pair contruction callback.
 */
typedef void* (*qse_rbt_copier_t) (
	qse_rbt_t* rbt  /* red-black tree */,
	void*      dptr /* pointer to a key or a value */, 
	qse_size_t dlen /* length of a key or a value */
);

/**
 * The qse_rbt_freeer_t defines a key/value destruction callback.
 */
typedef void (*qse_rbt_freeer_t) (
	qse_rbt_t* rbt,  /**< red-black tree */
	void*      dptr, /**< pointer to a key or a value */
	qse_size_t dlen  /**< length of a key or a value */
);

/**
 * The qse_rbt_comper_t type defines a key comparator that is called when
 * the rbt needs to compare keys. A red-black tree is created with a default
 * comparator which performs bitwise comparison of two keys.
 * The comparator should return 0 if the keys are the same, 1 if the first
 * key is greater than the second key, -1 otherwise.
 */
typedef int (*qse_rbt_comper_t) (
	qse_rbt_t*  rbt,    /**< red-black tree */ 
	const void* kptr1,  /**< key pointer */
	qse_size_t  klen1,  /**< key length */ 
	const void* kptr2,  /**< key pointer */
	qse_size_t  klen2   /**< key length */
);

/**
 * The qse_rbt_keeper_t type defines a value keeper that is called when 
 * a value is retained in the context that it should be destroyed because
 * it is identical to a new value. Two values are identical if their 
 * pointers and lengths are equal.
 */
typedef void (*qse_rbt_keeper_t) (
	qse_rbt_t* rbt,    /**< red-black tree */
	void*      vptr,   /**< value pointer */
	qse_size_t vlen    /**< value length */
);

/**
 * The qse_rbt_walker_t defines a pair visitor.
 */
typedef qse_rbt_walk_t (*qse_rbt_walker_t) (
	qse_rbt_t*      rbt,   /**< rbt */
	qse_rbt_pair_t* pair,  /**< pointer to a key/value pair */
	void*           ctx    /**< pointer to user-defined data */
);

/**
 * The qse_rbt_pair_t type defines red-black tree pair. A pair is composed 
 * of a key and a value. It maintains pointers to the beginning of a key and 
 * a value plus their length. The length is scaled down with the scale factor 
 * specified in an owning tree. Use macros defined in the 
 */
struct qse_rbt_pair_t
{
	void*           kptr;  /**< key pointer */
	qse_size_t      klen;  /**< key length */
	void*           vptr;  /**< value pointer */
	qse_size_t      vlen;  /**< value length */

	/* management information below */
	enum
	{
		QSE_RBT_RED,
		QSE_RBT_BLACK
	} color;
	qse_rbt_pair_t* parent;
	qse_rbt_pair_t* child[2]; /* left and right */
};

/**
 * The qse_rbt_t type defines a red-black tree.
 */
struct qse_rbt_t
{
	QSE_DEFINE_COMMON_FIELDS (rbt)

	qse_rbt_copier_t copier[2]; /**< key and value copier */
	qse_rbt_freeer_t freeer[2]; /**< key and value freeer */
	qse_rbt_comper_t comper;    /**< key comparator */
	qse_rbt_keeper_t keeper;    /**< value keeper */

	qse_byte_t       scale[2];  /**< length scale */

	qse_rbt_pair_t   nil;       /**< internal nil node */

	qse_size_t       size;      /**< number of pairs */
	qse_rbt_pair_t*  root;      /**< root pair */
};

/**
 * The QSE_RBT_COPIER_SIMPLE macros defines a copier that remembers the
 * pointer and length of data in a pair.
 */
#define QSE_RBT_COPIER_SIMPLE ((qse_rbt_copier_t)1)

/**
 * The QSE_RBT_COPIER_INLINE macros defines a copier that copies data into
 * a pair.
 */
#define QSE_RBT_COPIER_INLINE ((qse_rbt_copier_t)2)

/**
 * The QSE_RBT_SIZE() macro returns the number of pairs in red-black tree.
 */
#define QSE_RBT_SIZE(m)   ((const qse_size_t)(m)->size)
#define QSE_RBT_KSCALE(m) ((const int)(m)->scale[QSE_RBT_KEY])
#define QSE_RBT_VSCALE(m) ((const int)(m)->scale[QSE_RBT_VAL])

#define QSE_RBT_KPTR(p) ((p)->kptr)
#define QSE_RBT_KLEN(p) ((p)->klen)
#define QSE_RBT_VPTR(p) ((p)->vptr)
#define QSE_RBT_VLEN(p) ((p)->vlen)
#define QSE_RBT_NEXT(p) ((p)->next)

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (rbt)

/**
 * The qse_rbt_open() function creates a red-black tree.
 * @return qse_rbt_t pointer on success, QSE_NULL on failure.
 */
qse_rbt_t* qse_rbt_open (
	qse_mmgr_t* mmgr,   /**< memory manager */
	qse_size_t  ext     /**< extension size in bytes */
);

/**
 * The qse_rbt_close() function destroys a red-black tree.
 */
void qse_rbt_close (
	qse_rbt_t* rbt   /**< red-black tree */
);

/**
 * The qse_rbt_init() function initializes a red-black tree
 */
qse_rbt_t* qse_rbt_init (
	qse_rbt_t*  rbt, /**< red-black tree */
	qse_mmgr_t* mmgr /**< memory manager */
);

/**
 * The qse_rbt_fini() funtion finalizes a red-black tree
 */
void qse_rbt_fini (
	qse_rbt_t* rbt  /**< red-black tree */
);

/**
 * The qse_rbt_getsize() function gets the number of pairs in red-black tree.
 */
qse_size_t qse_rbt_getsize (
	qse_rbt_t* rbt  /**< red-black tree */
);

/**
 * The qse_rbt_getscale() function returns the scale factor
 */
int qse_rbt_getscale (
	qse_rbt_t*   rbt, /**< red-black tree */
	qse_rbt_id_t id   /**< #QSE_RBT_KEY or #QSE_RBT_VAL */
);

/**
 * The qse_rbt_setscale() function sets the scale factor of the length
 * of a key and a value. A scale factor determines the actual length of
 * a key and a value in bytes. A rbt is created with a scale factor of 1.
 * The scale factor should be larger than 0 and less than 256.
 * Note that it is a bad idea to change the scale factor while a red-black tree 
 * is not empty.
 */
void qse_rbt_setscale (
	qse_rbt_t*   rbt,  /**< red-black tree */
	qse_rbt_id_t id,   /**< #QSE_RBT_KEY or #QSE_RBT_VAL */
	int          scale /**< scale factor in bytes */
);

/**
 * The qse_rbt_getcopier() function gets a data copier.
 */
qse_rbt_copier_t qse_rbt_getcopier (
	qse_rbt_t*   rbt, /**< red-black tree */
	qse_rbt_id_t id   /**< #QSE_RBT_KEY or #QSE_RBT_VAL */
);

/**
 * The qse_rbt_setcopier() function specifies how to clone an element.
 *  A special copier QSE_RBT_COPIER_INLINE is provided. This copier enables
 *  you to copy the data inline to the internal node. No freeer is invoked
 *  when the node is freeed.
 *
 *  You may set the copier to QSE_NULL to perform no special operation 
 *  when the data pointer is rememebered.
 */
void qse_rbt_setcopier (
	qse_rbt_t* rbt,          /**< red-black tree */
	qse_rbt_id_t id,         /**< #QSE_RBT_KEY or #QSE_RBT_VAL */
	qse_rbt_copier_t copier  /**< callback for copying a key or a value */
);

/**
 * The qse_rb_getfreeer() function returns the element destroyer.
 */
qse_rbt_freeer_t qse_rbt_getfreeer (
	qse_rbt_t*   rbt, /**< red-black tree */
	qse_rbt_id_t id   /**< #QSE_RBT_KEY or #QSE_RBT_VAL */
);

/**
 * The qse_rbt_setfreeer() function specifies how to destroy an element.
 * The @a freeer is called when a node containing the element is destroyed.
 */
void qse_rbt_setfreeer (
	qse_rbt_t*       rbt,    /**< red-black tree */
	qse_rbt_id_t     id,     /**< #QSE_RBT_KEY or #QSE_RBT_VAL */
	qse_rbt_freeer_t freeer  /**< callback for destroying a key or a value */
);

/**
 * The qse_rbt_getcomper() function returns the key comparator.
 */
qse_rbt_comper_t qse_rbt_getcomper (
	qse_rbt_t* rbt /**< red-black tree */
);

/**
 * The qse_rbt_setcomper() function changes the key comparator.
 */
void qse_rbt_setcomper (
	qse_rbt_t* rbt,         /**< red-black tree */
	qse_rbt_comper_t comper /**< comparator function pointer */
);

/**
 * The qse_rbt_getkeeper() function returns the value retainer function
 * that is called when you change the value of an existing key with the
 * same value.
 */
qse_rbt_keeper_t qse_rbt_getkeeper (
	qse_rbt_t* rbt
);

/**
 * The qse_rbt_setkeeper() function changes the value retainer function.
 */
void qse_rbt_setkeeper (
	qse_rbt_t*       rbt,
	qse_rbt_keeper_t keeper
);

/**
 * The qse_rbt_search() function searches red-black tree to find a pair with a 
 * matching key. It returns the pointer to the pair found. If it fails
 * to find one, it returns QSE_NULL.
 * @return pointer to the pair with a maching key, 
 *         or QSE_NULL if no match is found.
 */
qse_rbt_pair_t* qse_rbt_search (
	qse_rbt_t*  rbt,   /**< red-black tree */
	const void* kptr,  /**< key pointer */
	qse_size_t  klen   /**< the size of the key */
);

/**
 * The qse_rbt_upsert() function searches red-black tree for the pair with a 
 * matching key. If one is found, it updates the pair. Otherwise, it inserts
 * a new pair with the key and the value given. It returns the pointer to the 
 * pair updated or inserted.
 * @return a pointer to the updated or inserted pair on success, 
 *         QSE_NULL on failure. 
 */
qse_rbt_pair_t* qse_rbt_upsert (
	qse_rbt_t* rbt,   /**< red-black tree */
	void*      kptr,  /**< key pointer */
	qse_size_t klen,  /**< key length */
	void*      vptr,  /**< value pointer */
	qse_size_t vlen   /**< value length */
);

/**
 * The qse_rbt_ensert() function inserts a new pair with the key and the value
 * given. If there exists a pair with the key given, the function returns 
 * the pair containing the key.
 * @return pointer to a pair on success, QSE_NULL on failure. 
 */
qse_rbt_pair_t* qse_rbt_ensert (
	qse_rbt_t* rbt,   /**< red-black tree */
	void*      kptr,  /**< key pointer */
	qse_size_t klen,  /**< key length */
	void*      vptr,  /**< value pointer */
	qse_size_t vlen   /**< value length */
);

/**
 * The qse_rbt_insert() function inserts a new pair with the key and the value
 * given. If there exists a pair with the key given, the function returns 
 * QSE_NULL without channging the value.
 * @return pointer to the pair created on success, QSE_NULL on failure. 
 */
qse_rbt_pair_t* qse_rbt_insert (
	qse_rbt_t* rbt,   /**< red-black tree */
	void*      kptr,  /**< key pointer */
	qse_size_t klen,  /**< key length */
	void*      vptr,  /**< value pointer */
	qse_size_t vlen   /**< value length */
);

/**
 * The qse_rbt_update() function updates the value of an existing pair
 * with a matching key.
 * @return pointer to the pair on success, QSE_NULL on no matching pair
 */
qse_rbt_pair_t* qse_rbt_update (
	qse_rbt_t* rbt,   /**< red-black tree */
	void*      kptr,  /**< key pointer */
	qse_size_t klen,  /**< key length */
	void*      vptr,  /**< value pointer */
	qse_size_t vlen   /**< value length */
);

/**
 * The qse_rbt_delete() function deletes a pair with a matching key 
 * @return 0 on success, -1 on failure
 */
int qse_rbt_delete (
	qse_rbt_t* rbt,   /**< red-black tree */
	const void* kptr, /**< key pointer */
	qse_size_t klen   /**< key size */
);

/**
 * The qse_rbt_clear() function empties a red-black tree.
 */
void qse_rbt_clear (
	qse_rbt_t* rbt /**< red-black tree */
);

/**
 * The qse_rbt_walk() function traverses a red-black tree in preorder 
 * from the leftmost child.
 */
void qse_rbt_walk (
	qse_rbt_t*       rbt,    /**< red-black tree */
	qse_rbt_walker_t walker, /**< callback function for each pair */
	void*            ctx     /**< pointer to user-specific data */
);

/**
 * The qse_rbt_walk() function traverses a red-black tree in preorder 
 * from the rightmost child.
 */
void qse_rbt_rwalk (
	qse_rbt_t*       rbt,    /**< red-black tree */
	qse_rbt_walker_t walker, /**< callback function for each pair */
	void*            ctx     /**< pointer to user-specific data */
);

#ifdef __cplusplus
}
#endif

#endif
