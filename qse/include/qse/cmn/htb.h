/*
 * $Id: htb.h 556 2011-08-31 15:43:46Z hyunghwan.chung $
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

#ifndef _QSE_CMN_HTB_H_
#define _QSE_CMN_HTB_H_

#include <qse/types.h>
#include <qse/macros.h>

/**@file
 * This file provides a hash table encapsulated in the #qse_htb_t type that 
 * maintains buckets for key/value pairs with the same key hash chained under
 * the same bucket. Its interface is very close to #qse_rbt_t.
 *
 * This sample code adds a series of keys and values and print them
 * in the randome order.
 * @code
 * #include <qse/cmn/htb.h>
 * #include <qse/cmn/mem.h>
 * #include <qse/cmn/stdio.h>
 * 
 * static qse_htb_walk_t walk (qse_htb_t* htb, qse_htb_pair_t* pair, void* ctx)
 * {
 *   qse_printf (QSE_T("key = %d, value = %d\n"),
 *     *(int*)QSE_HTB_KPTR(pair), *(int*)QSE_HTB_VPTR(pair));
 *   return QSE_HTB_WALK_FORWARD;
 * }
 * 
 * int main ()
 * {
 *   qse_htb_t* s1;
 *   int i;
 * 
 *   s1 = qse_htb_open (QSE_MMGR_GETDFL(), 0, 30, 75, 1, 1); // error handling skipped
 *   qse_htb_setmancbs (s1, qse_gethtbmancbs(QSE_HTB_MANCBS_INLINE_COPIERS));
 * 
 *   for (i = 0; i < 20; i++)
 *   {
 *     int x = i * 20;
 *     qse_htb_insert (s1, &i, QSE_SIZEOF(i), &x, QSE_SIZEOF(x)); // eror handling skipped
 *   }
 * 
 *   qse_htb_walk (s1, walk, QSE_NULL);
 * 
 *   qse_htb_close (s1);
 *   return 0;
 * }
 * @endcode
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
 * A special copier #QSE_HTB_COPIER_INLINE is provided. This copier enables
 * you to copy the data inline to the internal node. No freeer is invoked
 * when the node is freeed.
 */
typedef void* (*qse_htb_copier_t) (
	qse_htb_t* htb  /* hash table */,
	void*      dptr /* pointer to a key or a value */, 
	qse_size_t dlen /* length of a key or a value */
);

/**
 * The qse_htb_freeer_t defines a key/value destruction callback
 * The freeer is called when a node containing the element is destroyed.
 */
typedef void (*qse_htb_freeer_t) (
	qse_htb_t* htb,  /**< hash table */
	void*      dptr, /**< pointer to a key or a value */
	qse_size_t dlen  /**< length of a key or a value */
);


/**
 * The qse_htb_comper_t type defines a key comparator that is called when
 * the htb needs to compare keys. A hash table is created with a default
 * comparator which performs bitwise comparison of two keys.
 * The comparator should return 0 if the keys are the same and a non-zero
 * integer otherwise.
 */
typedef int (*qse_htb_comper_t) (
	const qse_htb_t* htb,    /**< hash table */ 
	const void*      kptr1,  /**< key pointer */
	qse_size_t       klen1,  /**< key length */ 
	const void*      kptr2,  /**< key pointer */ 
	qse_size_t       klen2   /**< key length */
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
 * when hash table should resize the bucket. The current bucket size + 1 is 
 * passed as the hint.
 */
typedef qse_size_t (*qse_htb_sizer_t) (
	qse_htb_t* htb,  /**< htb */
	qse_size_t hint  /**< sizing hint */
);

/**
 * The qse_htb_hasher_t type defines a key hash function
 */
typedef qse_size_t (*qse_htb_hasher_t) (
	const qse_htb_t*  htb,   /**< hash table */
	const void*       kptr,  /**< key pointer */
	qse_size_t        klen   /**< key length in bytes */
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
 * The qse_htb_cbserter_t type defines a callback function for qse_htb_cbsert().
 * The qse_htb_cbserter() function calls it to allocate a new pair for the 
 * key pointed to by @a kptr of the length @a klen and the callback context
 * @a ctx. The second parameter @a pair is passed the pointer to the existing
 * pair for the key or #QSE_NULL in case of no existing key. The callback
 * must return a pointer to a new or a reallocated pair. When reallocating the
 * existing pair, this callback must destroy the existing pair and return the 
 * newly reallocated pair. It must return #QSE_NULL for failure.
 */
typedef qse_htb_pair_t* (*qse_htb_cbserter_t) (
	qse_htb_t*      htb,    /**< hash table */
	qse_htb_pair_t* pair,   /**< pair pointer */
	void*           kptr,   /**< key pointer */
	qse_size_t      klen,   /**< key length */
	void*           ctx     /**< callback context */
);


/**
 * The qse_htb_pair_t type defines hash table pair. A pair is composed of a key
 * and a value. It maintains pointers to the beginning of a key and a value
 * plus their length. The length is scaled down with the scale factor 
 * specified in an owning hash table. 
 */
struct qse_htb_pair_t
{
	qse_xptl_t key;
	qse_xptl_t val;

	/* management information below */
	qse_htb_pair_t* next; 
};

typedef struct qse_htb_mancbs_t qse_htb_mancbs_t;

struct qse_htb_mancbs_t
{
	qse_htb_copier_t copier[2];
	qse_htb_freeer_t freeer[2];
	qse_htb_comper_t comper;   /**< key comparator */
	qse_htb_keeper_t keeper;   /**< value keeper */
	qse_htb_sizer_t  sizer;    /**< bucket capacity recalculator */
	qse_htb_hasher_t hasher;   /**< key hasher */
};

/**
 * The qse_htb_mancbs_kind_t type defines the type of predefined
 * callback set for pair manipulation.
 */
enum qse_htb_mancbs_kind_t
{
	/** store the key and the value pointer */
	QSE_HTB_MANCBS_DEFAULT,
	/** copy both key and value into the pair */
	QSE_HTB_MANCBS_INLINE_COPIERS,
	/** copy the key into the pair but store the value pointer */
	QSE_HTB_MANCBS_INLINE_KEY_COPIER,
	/** copy the value into the pair but store the key pointer */
	QSE_HTB_MANCBS_INLINE_VALUE_COPIER
};

typedef enum qse_htb_mancbs_kind_t  qse_htb_mancbs_kind_t;

/**
 * The qse_htb_t type defines a hash table.
 */
struct qse_htb_t
{
	QSE_DEFINE_COMMON_FIELDS (htb)

	const qse_htb_mancbs_t* mancbs;

	qse_byte_t       scale[2]; /**< length scale */
	qse_byte_t       factor;   /**< load factor in percentage */

	qse_size_t       size;
	qse_size_t       capa;
	qse_size_t       threshold;

	qse_htb_pair_t** bucket;
};

/**
 * The QSE_HTB_COPIER_SIMPLE macros defines a copier that remembers the
 * pointer and length of data in a pair.
 **/
#define QSE_HTB_COPIER_SIMPLE ((qse_htb_copier_t)1)

/**
 * The QSE_HTB_COPIER_INLINE macros defines a copier that copies data into
 * a pair.
 **/
#define QSE_HTB_COPIER_INLINE ((qse_htb_copier_t)2)

#define QSE_HTB_COPIER_DEFAULT (QSE_HTB_COPIER_SIMPLE)
#define QSE_HTB_FREEER_DEFAULT (QSE_NULL)
#define QSE_HTB_COMPER_DEFAULT (qse_htb_dflcomp)
#define QSE_HTB_KEEPER_DEFAULT (QSE_NULL)
#define QSE_HTB_SIZER_DEFAULT  (QSE_NULL)
#define QSE_HTB_HASHER_DEFAULT (qse_htb_dflhash)

/**
 * The QSE_HTB_SIZE() macro returns the number of pairs in a hash table.
 */
#define QSE_HTB_SIZE(m) (*(const qse_size_t*)&(m)->size)

/**
 * The QSE_HTB_CAPA() macro returns the maximum number of pairs that can be
 * stored in a hash table without further reorganization.
 */
#define QSE_HTB_CAPA(m) (*(const qse_size_t*)&(m)->capa)

#define QSE_HTB_FACTOR(m) (*(const int*)&(m)->factor)
#define QSE_HTB_KSCALE(m) (*(const int*)&(m)->scale[QSE_HTB_KEY])
#define QSE_HTB_VSCALE(m) (*(const int*)&(m)->scale[QSE_HTB_VAL])

#define QSE_HTB_KPTL(p) (&(p)->key)
#define QSE_HTB_VPTL(p) (&(p)->val)

#define QSE_HTB_KPTR(p) ((p)->key.ptr)
#define QSE_HTB_KLEN(p) ((p)->key.len)
#define QSE_HTB_VPTR(p) ((p)->val.ptr)
#define QSE_HTB_VLEN(p) ((p)->val.len)

#define QSE_HTB_NEXT(p) ((p)->next)

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (htb)

/**
 * The qse_gethtbmancbs() functions returns a predefined callback set for
 * pair manipulation.
 */
const qse_htb_mancbs_t* qse_gethtbmancbs (
	qse_htb_mancbs_kind_t kind
);

/**
 * The qse_htb_open() function creates a hash table with a dynamic array 
 * bucket and a list of values chained. The initial capacity should be larger
 * than 0. The load factor should be between 0 and 100 inclusive and the load
 * factor of 0 disables bucket resizing. If you need extra space associated
 * with hash table, you may pass a non-zero value for @a xtnsize.
 * The QSE_HTB_XTN() macro and the qse_htb_getxtn() function return the 
 * pointer to the beginning of the extension.
 * The @a kscale and @a vscale parameters specify the unit of the key and 
 * value size. 
 * @return #qse_htb_t pointer on success, #QSE_NULL on failure.
 */
qse_htb_t* qse_htb_open (
	qse_mmgr_t* mmgr,    /**< memory manager */
	qse_size_t  xtnsize, /**< extension size in bytes */
	qse_size_t  capa,    /**< initial capacity */
	int         factor,  /**< load factor */
	int         kscale,  /**< key scale */
	int         vscale   /**< value scale */
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
int qse_htb_init (
	qse_htb_t*  htb,    /**< hash table */
	qse_mmgr_t* mmgr,   /**< memory manager */
	qse_size_t  capa,   /**< initial capacity */
	int         factor, /**< load factor */
	int         kscale, /**< key scale */
	int         vscale  /**< value scale */
);

/**
 * The qse_htb_fini() funtion finalizes a hash table
 */
void qse_htb_fini (
	qse_htb_t* htb
);

/**
 * The qse_htb_getmancbs() function gets manipulation callback function set.
 */
const qse_htb_mancbs_t* qse_htb_getmancbs (
	const qse_htb_t* htb /**< hash table */
);

/**
 * The qse_htb_setmancbs() function sets internal manipulation callback 
 * functions for data construction, destruction, resizing, hashing, etc.
 */
void qse_htb_setmancbs (
	qse_htb_t*              htb,   /**< hash table */
	const qse_htb_mancbs_t* mancbs /**< callback function set */
);

/**
 * The qse_htb_getsize() function gets the number of pairs in hash table.
 */
qse_size_t qse_htb_getsize (
	const qse_htb_t* htb
);

/**
 * The qse_htb_getcapa() function gets the number of slots allocated 
 * in a hash bucket.
 */
qse_size_t qse_htb_getcapa (
	const qse_htb_t* htb /**< hash table */
);

/**
 * The qse_htb_search() function searches a hash table to find a pair with a 
 * matching key. It returns the pointer to the pair found. If it fails
 * to find one, it returns QSE_NULL.
 * @return pointer to the pair with a maching key, 
 *         or #QSE_NULL if no match is found.
 */
qse_htb_pair_t* qse_htb_search (
	const qse_htb_t* htb,   /**< hash table */
	const void*      kptr,  /**< key pointer */
	qse_size_t       klen   /**< key length */
);

/**
 * The qse_htb_upsert() function searches a hash table for the pair with a 
 * matching key. If one is found, it updates the pair. Otherwise, it inserts
 * a new pair with the key and value given. It returns the pointer to the 
 * pair updated or inserted.
 * @return pointer to the updated or inserted pair on success, 
 *         #QSE_NULL on failure. 
 */
qse_htb_pair_t* qse_htb_upsert (
	qse_htb_t* htb,   /**< hash table */
	void*      kptr,  /**< key pointer */
	qse_size_t klen,  /**< key length */
	void*      vptr,  /**< value pointer */
	qse_size_t vlen   /**< value length */
);

/**
 * The qse_htb_ensert() function inserts a new pair with the key and the value
 * given. If there exists a pair with the key given, the function returns 
 * the pair containing the key.
 * @return pointer to a pair on success, #QSE_NULL on failure. 
 */
qse_htb_pair_t* qse_htb_ensert (
	qse_htb_t* htb,   /**< hash table */
	void*      kptr,  /**< key pointer */
	qse_size_t klen,  /**< key length */
	void*      vptr,  /**< value pointer */
	qse_size_t vlen   /**< value length */
);

/**
 * The qse_htb_insert() function inserts a new pair with the key and the value
 * given. If there exists a pair with the key given, the function returns 
 * #QSE_NULL without channging the value.
 * @return pointer to the pair created on success, #QSE_NULL on failure. 
 */
qse_htb_pair_t* qse_htb_insert (
	qse_htb_t* htb,   /**< hash table */
	void*      kptr,  /**< key pointer */
	qse_size_t klen,  /**< key length */
	void*      vptr,  /**< value pointer */
	qse_size_t vlen   /**< value length */
);

/**
 * The qse_htb_update() function updates the value of an existing pair
 * with a matching key.
 * @return pointer to the pair on success, #QSE_NULL on no matching pair
 */
qse_htb_pair_t* qse_htb_update (
	qse_htb_t* htb,   /**< hash table */
	void*      kptr,  /**< key pointer */
	qse_size_t klen,  /**< key length */
	void*      vptr,  /**< value pointer */
	qse_size_t vlen   /**< value length */
);

/**
 * The qse_htb_cbsert() function inserts a key/value pair by delegating pair 
 * allocation to a callback function. Depending on the callback function,
 * it may behave like qse_htb_insert(), qse_htb_upsert(), qse_htb_update(),
 * qse_htb_ensert(), or totally differently. The sample code below inserts
 * a new pair if the key is not found and appends the new value to the
 * existing value delimited by a comma if the key is found.
 *
 * @code
 * qse_htb_walk_t print_map_pair (qse_htb_t* map, qse_htb_pair_t* pair, void* ctx)
 * {
 *   qse_printf (QSE_T("%.*s[%d] => %.*s[%d]\n"),
 *     (int)QSE_HTB_KLEN(pair), QSE_HTB_KPTR(pair), (int)QSE_HTB_KLEN(pair),
 *     (int)QSE_HTB_VLEN(pair), QSE_HTB_VPTR(pair), (int)QSE_HTB_VLEN(pair));
 *   return QSE_HTB_WALK_FORWARD;
 * }
 * 
 * qse_htb_pair_t* cbserter (
 *   qse_htb_t* htb, qse_htb_pair_t* pair,
 *   void* kptr, qse_size_t klen, void* ctx)
 * {
 *   qse_xstr_t* v = (qse_xstr_t*)ctx;
 *   if (pair == QSE_NULL)
 *   {
 *     // no existing key for the key 
 *     return qse_htb_allocpair (htb, kptr, klen, v->ptr, v->len);
 *   }
 *   else
 *   {
 *     // a pair with the key exists. 
 *     // in this sample, i will append the new value to the old value 
 *     // separated by a comma
 *     qse_htb_pair_t* new_pair;
 *     qse_char_t comma = QSE_T(',');
 *     qse_byte_t* vptr;
 * 
 *     // allocate a new pair, but without filling the actual value. 
 *     // note vptr is given QSE_NULL for that purpose 
 *     new_pair = qse_htb_allocpair (
 *       htb, kptr, klen, QSE_NULL, pair->vlen + 1 + v->len); 
 *     if (new_pair == QSE_NULL) return QSE_NULL;
 * 
 *     // fill in the value space 
 *     vptr = new_pair->vptr;
 *     qse_memcpy (vptr, pair->vptr, pair->vlen*QSE_SIZEOF(qse_char_t));
 *     vptr += pair->vlen*QSE_SIZEOF(qse_char_t);
 *     qse_memcpy (vptr, &comma, QSE_SIZEOF(qse_char_t));
 *     vptr += QSE_SIZEOF(qse_char_t);
 *     qse_memcpy (vptr, v->ptr, v->len*QSE_SIZEOF(qse_char_t));
 * 
 *     // this callback requires the old pair to be destroyed 
 *     qse_htb_freepair (htb, pair);
 * 
 *     // return the new pair 
 *     return new_pair;
 *   }
 * }
 * 
 * int main ()
 * {
 *   qse_htb_t* s1;
 *   int i;
 *   qse_char_t* keys[] = { QSE_T("one"), QSE_T("two"), QSE_T("three") };
 *   qse_char_t* vals[] = { QSE_T("1"), QSE_T("2"), QSE_T("3"), QSE_T("4"), QSE_T("5") };
 * 
 *   s1 = qse_htb_open (
 *     QSE_MMGR_GETDFL(), 0, 10, 70,
 *     QSE_SIZEOF(qse_char_t), QSE_SIZEOF(qse_char_t)
 *   ); // note error check is skipped 
 *   qse_htb_setmancbs (s1, &mancbs1);
 * 
 *   for (i = 0; i < QSE_COUNTOF(vals); i++)
 *   {
 *     qse_xstr_t ctx;
 *     ctx.ptr = vals[i]; ctx.len = qse_strlen(vals[i]);
 *     qse_htb_cbsert (s1,
 *       keys[i%QSE_COUNTOF(keys)], qse_strlen(keys[i%QSE_COUNTOF(keys)]),
 *       cbserter, &ctx
 *     ); // note error check is skipped
 *   }
 *   qse_htb_walk (s1, print_map_pair, QSE_NULL);
 * 
 *   qse_htb_close (s1);
 *   return 0;
 * }
 * @endcode
 */
qse_htb_pair_t* qse_htb_cbsert (
	qse_htb_t*         htb,      /**< hash table */
	void*              kptr,     /**< key pointer */
	qse_size_t         klen,     /**< key length */
	qse_htb_cbserter_t cbserter, /**< callback function */
	void*              ctx       /**< callback context */
);

/**
 * The qse_htb_delete() function deletes a pair with a matching key 
 * @return 0 on success, -1 on failure
 */
int qse_htb_delete (
	qse_htb_t* htb,   /**< hash table */
	const void* kptr, /**< key pointer */
	qse_size_t klen   /**< key length */
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
	qse_htb_t*       htb,    /**< hash table */
	qse_htb_walker_t walker, /**< callback function for each pair */
	void*            ctx     /**< pointer to user-specific data */
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

/**
 * The qse_htb_allocpair() function allocates a pair for a key and a value 
 * given. But it does not chain the pair allocated into the hash table @a htb.
 * Use this function at your own risk. 
 *
 * Take note of he following special behavior when the copier is 
 * #QSE_HTB_COPIER_INLINE.
 * - If @a kptr is #QSE_NULL, the key space of the size @a klen is reserved but
 *   not propagated with any data.
 * - If @a vptr is #QSE_NULL, the value space of the size @a vlen is reserved
 *   but not propagated with any data.
 */
qse_htb_pair_t* qse_htb_allocpair (
	qse_htb_t* htb,
	void*      kptr, 
	qse_size_t klen,	
	void*      vptr,
	qse_size_t vlen
);

/**
 * The qse_htb_freepair() function destroys a pair. But it does not detach
 * the pair destroyed from the hash table @a htb. Use this function at your
 * own risk.
 */
void qse_htb_freepair (
	qse_htb_t*      htb,
	qse_htb_pair_t* pair
);

/**
 * The qse_htb_dflhash() function is a default hash function.
 */
qse_size_t qse_htb_dflhash (
	const qse_htb_t*  htb,
	const void*       kptr,
	qse_size_t        klen
);

/**
 * The qse_htb_dflcomp() function is default comparator.
 */
int qse_htb_dflcomp (
	const qse_htb_t* htb,
	const void*      kptr1,
	qse_size_t       klen1,
	const void*      kptr2,
	qse_size_t       klen2
);

#ifdef __cplusplus
}
#endif

#endif
