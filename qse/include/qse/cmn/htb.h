/*
 * $Id: htb.h 356 2010-09-07 12:29:25Z hyunghwan.chung $
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

#ifndef _QSE_CMN_HTB_H_
#define _QSE_CMN_HTB_H_

#include <qse/types.h>
#include <qse/macros.h>

/**@file
 * A hash table maintains buckets for key/value pairs with the same key hash
 * chained under the same bucket.
 */

typedef struct qse_htb_t qse_htb_t;
typedef struct qse_htb_pair_t qse_htb_pair_t;

/** 
 * The qse_htb_walk_t type defines values that the callback function can
 * return to control qse_htb_walk().
 */
enum qse_htb_walk_t
{
        QSE_HTB_WALK_STOP    = 0,
        QSE_HTB_WALK_FORWARD = 1
};
typedef enum qse_htb_walk_t qse_htb_walk_t;

/**
 * The qse_htb_id_t type defines IDs to indicate a key or a value in various
 * functions.
 */
enum qse_htb_id_t
{
	QSE_HTB_KEY = 0,
	QSE_HTB_VAL = 1
};
typedef enum qse_htb_id_t qse_htb_id_t;

/**
 * The qse_htb_copier_t type defines a pair contruction callback.
 */
typedef void* (*qse_htb_copier_t) (
	qse_htb_t* htb  /* hash table */,
	void*      dptr /* pointer to a key or a value */, 
	qse_size_t dlen /* length of a key or a value */
);

/**
 * The qse_htb_freeer_t defines a key/value destruction callback
 */
typedef void (*qse_htb_freeer_t) (
	qse_htb_t* htb,  /**< hash table */
	void*      dptr, /**< pointer to a key or a value */
	qse_size_t dlen  /**< length of a key or a value */
);

/* key hasher */
typedef qse_size_t (*qse_htb_hasher_t) (
	qse_htb_t*  htb,   /**< hash table */
	const void* kptr,  /**< key pointer */
	qse_size_t  klen   /**< key length in bytes */
);

/**
 * The qse_htb_comper_t type defines a key comparator that is called when
 * the htb needs to compare keys. A hash table is created with a default
 * comparator which performs bitwise comparison of two keys.
 * The comparator should return 0 if the keys are the same and a non-zero
 * integer otherwise.
 */
typedef int (*qse_htb_comper_t) (
	qse_htb_t*  htb,    /**< hash table */ 
	const void* kptr1,  /**< pointer to a key */
	qse_size_t  klen1,  /**< length of a key */ 
	const void* kptr2,  /**< pointer to a key */ 
	qse_size_t  klen2   /**< length of a key */
);

/**
 * The qse_htb_keeper_t type defines a value keeper that is called when 
 * a value is retained in the context that it should be destroyed because
 * it is identical to a new value. Two values are identical if their 
 * pointers and lengths are equal.
 */
typedef void (*qse_htb_keeper_t) (
	qse_htb_t* htb,    /**< hash table */
	void*      vptr,   /**< value pointer */
	qse_size_t vlen    /**< value length */
);

/**
 * The qse_htb_sizer_t type defines a bucket size claculator that is called
 * when hash table should resize the bucket. The current bucket size +1 is passed
 * as the hint.
 */
typedef qse_size_t (*qse_htb_sizer_t) (
	qse_htb_t* htb,  /**< htb */
	qse_size_t hint  /**< sizing hint */
);

/**
 * The qse_htb_walker_t defines a pair visitor.
 */
typedef qse_htb_walk_t (*qse_htb_walker_t) (
	qse_htb_t*      htb,   /**< htb */
	qse_htb_pair_t* pair,  /**< pointer to a key/value pair */
	void*           ctx    /**< pointer to user-defined data */
);

/**
 * The qse_htb_pair_t type defines hash table pair. A pair is composed of a key
 * and a value. It maintains pointers to the beginning of a key and a value
 * plus their length. The length is scaled down with the scale factor 
 * specified in an owning hash table. 
 */
struct qse_htb_pair_t
{
	void*           kptr;  /**< pointer to a key */
	qse_size_t      klen;  /**< length of a key */
	void*           vptr;  /**< pointer to a value */
	qse_size_t      vlen;  /**< length of a value */

	/* management information below */
	qse_htb_pair_t* next; 
};

/**
 * The qse_htb_t type defines a hash table.
 */
struct qse_htb_t
{
	QSE_DEFINE_COMMON_FIELDS (htb)

	qse_htb_copier_t copier[2];
	qse_htb_freeer_t freeer[2];
	qse_htb_hasher_t hasher;   /**< key hasher */
	qse_htb_comper_t comper;   /**< key comparator */
	qse_htb_keeper_t keeper;   /**< value keeper */
	qse_htb_sizer_t  sizer;    /**< bucket capacity recalculator */

	qse_byte_t       scale[2]; /**< length scale */
	qse_byte_t       factor;   /**< load factor in percentage */

	qse_size_t       size;
	qse_size_t       capa;
	qse_size_t       threshold;

	qse_htb_pair_t** bucket;
};

#define QSE_HTB_COPIER_SIMPLE ((qse_htb_copier_t)1)
#define QSE_HTB_COPIER_INLINE ((qse_htb_copier_t)2)

/**
 * The QSE_HTB_SIZE() macro returns the number of pairs in a hash table.
 */
#define QSE_HTB_SIZE(m) ((const qse_size_t)(m)->size)

/**
 * The QSE_HTB_CAPA() macro returns the maximum number of pairs that can be
 * stored in a hash table without further reorganization.
 */
#define QSE_HTB_CAPA(m) ((const qse_size_t)(m)->capa)

#define QSE_HTB_FACTOR(m) ((const int)(m)->factor)
#define QSE_HTB_KSCALE(m) ((const int)(m)->scale[QSE_HTB_KEY])
#define QSE_HTB_VSCALE(m) ((const int)(m)->scale[QSE_HTB_VAL])

#define QSE_HTB_KPTR(p) ((p)->kptr)
#define QSE_HTB_KLEN(p) ((p)->klen)
#define QSE_HTB_VPTR(p) ((p)->vptr)
#define QSE_HTB_VLEN(p) ((p)->vlen)
#define QSE_HTB_NEXT(p) ((p)->next)

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (htb)

/**
 * The qse_htb_open() function creates a hash table with a dynamic array 
 * bucket and a list of values chained. The initial capacity should be larger
 * than 0. The load factor should be between 0 and 100 inclusive and the load
 * factor of 0 disables bucket resizing. If you need extra space associated
 * with hash table, you may pass a non-zero value as the second parameter. 
 * The QSE_HTB_XTN() macro and the qse_htb_getxtn() function return the 
 * pointer to the beginning of the extension.
 * @return qse_htb_t pointer on success, QSE_NULL on failure.
 */
qse_htb_t* qse_htb_open (
	qse_mmgr_t* mmgr,   /**< memory manager */
	qse_size_t  ext,    /**< extension size in bytes */
	qse_size_t  capa,   /**< initial capacity */
	int         factor  /**< load factor */
);


/**
 * The qse_htb_close() function destroys a hash table.
 */
void qse_htb_close (
	qse_htb_t* htb /**< hash table */
);

/**
 * The qse_htb_init() function initializes a hash table
 */
qse_htb_t* qse_htb_init (
	qse_htb_t*  htb,
	qse_mmgr_t* mmgr,
	qse_size_t  capa,
	int         factor
);

/**
 * The qse_htb_fini() funtion finalizes a hash table
 */
void qse_htb_fini (
	qse_htb_t* htb
);

/**
 * The qse_htb_getsize() function gets the number of pairs in hash table.
 */
qse_size_t qse_htb_getsize (
	qse_htb_t* htb
);

/**
 * The qse_htb_getcapa() function gets the number of slots allocated 
 * in a hash bucket.
 */
qse_size_t qse_htb_getcapa (
	qse_htb_t* htb /**< hash table */
);

/**
 * The qse_htb_getscale() function returns the scale factor
 */
int qse_htb_getscale (
	qse_htb_t*   htb, /**< hash table */
	qse_htb_id_t id   /**< QSE_HTB_KEY or QSE_HTB_VAL */
);

/**
 * The qse_htb_setscale() function sets the scale factor of the length
 * of a key and a value. A scale factor determines the actual length of
 * a key and a value in bytes. A htb is created with a scale factor of 1.
 * The scale factor should be larger than 0 and less than 256.
 * Note that it is a bad idea to change the scale factor while a hash table 
 * is not empty.
 */
void qse_htb_setscale (
	qse_htb_t*   htb,  /**< hash table */
	qse_htb_id_t id,   /**< QSE_HTB_KEY or QSE_HTB_VAL */
	int          scale /**< scale factor in bytes */
);

/**
 * The qse_htb_getcopier() function gets a data copier.
 */
qse_htb_copier_t qse_htb_getcopier (
	qse_htb_t*   htb, /**< hash table */
	qse_htb_id_t id   /**< QSE_HTB_KEY or QSE_HTB_VAL */
);

/**
 * The qse_htb_setcopier() function specifies how to clone an element.
 *  A special copier QSE_HTB_COPIER_INLINE is provided. This copier enables
 *  you to copy the data inline to the internal node. No freeer is invoked
 *  when the node is freeed.
 *
 *  You may set the copier to QSE_NULL to perform no special operation 
 *  when the data pointer is rememebered.
 */
void qse_htb_setcopier (
	qse_htb_t* htb,          /**< hash table */
	qse_htb_id_t id,         /**< QSE_HTB_KEY or QSE_HTB_VAL */
	qse_htb_copier_t copier  /**< element copier */
);

/**
 * The qse_htb_getfreeer() function returns a custom element destroyer.
 */
qse_htb_freeer_t qse_htb_getfreeer (
	qse_htb_t*   htb, /**< hash table */
	qse_htb_id_t id   /**< QSE_HTB_KEY or QSE_HTB_VAL */
);

/**
 * The qse_htb_setfreeer() function specifies how to destroy an element.
 * The @a freeer is called when a node containing the element is destroyed.
 */
void qse_htb_setfreeer (
	qse_htb_t*       htb,    /**< hash table */
	qse_htb_id_t     id,     /**< QSE_HTB_KEY or QSE_HTB_VAL */
	qse_htb_freeer_t freeer  /**< an element freeer */
);

qse_htb_hasher_t qse_htb_gethasher (
	qse_htb_t* htb 
);

void qse_htb_sethasher (
	qse_htb_t*       htb,
	qse_htb_hasher_t hasher
);

qse_htb_comper_t qse_htb_getcomper (
	qse_htb_t* htb
);

void qse_htb_setcomper (
	qse_htb_t* htb,
	qse_htb_comper_t comper
);

qse_htb_keeper_t qse_htb_getkeeper (
	qse_htb_t* htb
);

void qse_htb_setkeeper (
	qse_htb_t* htb,
	qse_htb_keeper_t keeper
);

qse_htb_sizer_t qse_htb_getsizer (
	qse_htb_t* htb
);

/* the sizer function is passed hash table object and htb->capa + 1 */
void qse_htb_setsizer (
	qse_htb_t* htb,
	qse_htb_sizer_t sizer
);

/**
 * The qse_htb_search() function searches a hash table to find a pair with a 
 * matching key. It returns the pointer to the pair found. If it fails
 * to find one, it returns QSE_NULL.
 * @return pointer to the pair with a maching key, 
 *         or QSE_NULL if no match is found.
 */
qse_htb_pair_t* qse_htb_search (
	qse_htb_t*  htb,   /**< hash table */
	const void* kptr,  /**< the pointer to a key */
	qse_size_t  klen   /**< the size of the key */
);

/**
 * The qse_htb_upsert() function searches a hash table for the pair with a 
 * matching key. If one is found, it updates the pair. Otherwise, it inserts
 * a new pair with the key and value given. It returns the pointer to the 
 * pair updated or inserted.
 * @return a pointer to the updated or inserted pair on success, 
 *         QSE_NULL on failure. 
 */
qse_htb_pair_t* qse_htb_upsert (
	qse_htb_t* htb,   /**< hash table */
	void*      kptr,  /**< the pointer to a key */
	qse_size_t klen,  /**< the length of the key */
	void*      vptr,  /**< the pointer to a value */
	qse_size_t vlen   /**< the length of the value */
);

/**
 * The qse_htb_ensert() function inserts a new pair with the key and the value
 * given. If there exists a pair with the key given, the function returns 
 * the pair containing the key.
 * @return pointer to a pair on success, QSE_NULL on failure. 
 */
qse_htb_pair_t* qse_htb_ensert (
	qse_htb_t* htb,   /**< hash table */
	void*      kptr,  /**< the pointer to a key */
	qse_size_t klen,  /**< the length of the key */
	void*      vptr,  /**< the pointer to a value */
	qse_size_t vlen   /**< the length of the value */
);

/**
 * The qse_htb_insert() function inserts a new pair with the key and the value
 * given. If there exists a pair with the key given, the function returns 
 * QSE_NULL without channging the value.
 * @return pointer to the pair created on success, QSE_NULL on failure. 
 */
qse_htb_pair_t* qse_htb_insert (
	qse_htb_t* htb,   /**< hash table */
	void*      kptr,  /**< the pointer to a key */
	qse_size_t klen,  /**< the length of the key */
	void*      vptr,  /**< the pointer to a value */
	qse_size_t vlen   /**< the length of the value */
);

/**
 * The qse_htb_update() function updates the value of an existing pair
 * with a matching key.
 * @return pointer to the pair on success, QSE_NULL on no matching pair
 */
qse_htb_pair_t* qse_htb_update (
	qse_htb_t* htb,   /**< hash table */
	void*      kptr,  /**< the pointer to a key */
	qse_size_t klen,  /**< the length of the key */
	void*      vptr,  /**< the pointer to a value */
	qse_size_t vlen   /**< the length of the value */
);

/**
 * The qse_htb_delete() function deletes a pair with a matching key 
 * @return 0 on success, -1 on failure
 */
int qse_htb_delete (
	qse_htb_t* htb,   /**< hash table */
	const void* kptr, /**< the pointer to a key */
	qse_size_t klen   /**< the size of the key */
);

/**
 * The qse_htb_clear() function empties a hash table
 */
void qse_htb_clear (
	qse_htb_t* htb /**< hash table */
);

/**
 * The qse_htb_walk() function traverses a hash table.
 */
void qse_htb_walk (
	qse_htb_t* htb,          /**< hash table */
	qse_htb_walker_t walker, /**< callback function for each pair */
	void* ctx                /**< pointer to user-specific data */
);

/**
 * The qse_htb_getfirstpair() function returns the pointer to the first pair
 * in a hash table.
 */
qse_htb_pair_t* qse_htb_getfirstpair (
	qse_htb_t*   htb,     /**< hash table */
	qse_size_t*  buckno   /**< bucket number */
);

/**
 * The qse_htb_getnextpair() function returns the pointer to the next pair 
 * to the current pair @a pair in a hash table.
 */
qse_htb_pair_t* qse_htb_getnextpair (
	qse_htb_t*      htb,    /**< hash table */
	qse_htb_pair_t* pair,   /**< current pair  */
	qse_size_t*     buckno  /**< bucket number */
);

#ifdef __cplusplus
}
#endif

#endif
