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

#ifndef _QSE_CMN_RBT_H_
#define _QSE_CMN_RBT_H_

#include <qse/types.h>
#include <qse/macros.h>

/**@file
 * This file provides a red-black tree encapsulated in the #qse_rbt_t type that
 * implements a self-balancing binary search tree.Its interface is very close 
 * to #qse_htb_t.
 *
 * This sample code adds a series of keys and values and print them
 * in descending key order.
 * @code
 * #include <qse/cmn/rbt.h>
 * #include <qse/cmn/mem.h>
 * #include <qse/cmn/sio.h>
 * 
 * static qse_rbt_walk_t walk (qse_rbt_t* rbt, qse_rbt_pair_t* pair, void* ctx)
 * {
 *   qse_printf (QSE_T("key = %d, value = %d\n"),
 *     *(int*)QSE_RBT_KPTR(pair), *(int*)QSE_RBT_VPTR(pair));
 *   return QSE_RBT_WALK_FORWARD;
 * }
 * 
 * int main ()
 * {
 *   qse_rbt_t* s1;
 *   int i;
 * 
 *   s1 = qse_rbt_open (QSE_MMGR_GETDFL(), 0, 1, 1); // error handling skipped
 *   qse_rbt_setstyle (s1, qse_getrbtstyle(QSE_RBT_STYLE_INLINE_COPIERS));
 * 
 *   for (i = 0; i < 20; i++)
 *   {
 *     int x = i * 20;
 *     qse_rbt_insert (s1, &i, QSE_SIZEOF(i), &x, QSE_SIZEOF(x)); // eror handling skipped
 *   }
 * 
 *   qse_rbt_rwalk (s1, walk, QSE_NULL);
 * 
 *   qse_rbt_close (s1);
 *   return 0;
 * }
 * @endcode
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
	const qse_rbt_t* rbt,    /**< red-black tree */ 
	const void*      kptr1,  /**< key pointer */
	qse_size_t       klen1,  /**< key length */ 
	const void*      kptr2,  /**< key pointer */
	qse_size_t       klen2   /**< key length */
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
	qse_rbt_t*      rbt,   /**< red-black tree */
	qse_rbt_pair_t* pair,  /**< pointer to a key/value pair */
	void*           ctx    /**< pointer to user-defined data */
);

/**
 * The qse_rbt_cbserter_t type defines a callback function for qse_rbt_cbsert().
 * The qse_rbt_cbserter() function calls it to allocate a new pair for the 
 * key pointed to by @a kptr of the length @a klen and the callback context
 * @a ctx. The second parameter @a pair is passed the pointer to the existing
 * pair for the key or #QSE_NULL in case of no existing key. The callback
 * must return a pointer to a new or a reallocated pair. When reallocating the
 * existing pair, this callback must destroy the existing pair and return the 
 * newly reallocated pair. It must return #QSE_NULL for failure.
 */
typedef qse_rbt_pair_t* (*qse_rbt_cbserter_t) (
	qse_rbt_t*      rbt,    /**< red-black tree */
	qse_rbt_pair_t* pair,   /**< pair pointer */
	void*           kptr,   /**< key pointer */
	qse_size_t      klen,   /**< key length */
	void*           ctx     /**< callback context */
);

/**
 * The qse_rbt_pair_t type defines red-black tree pair. A pair is composed 
 * of a key and a value. It maintains pointers to the beginning of a key and 
 * a value plus their length. The length is scaled down with the scale factor 
 * specified in an owning tree. Use macros defined in the 
 */
struct qse_rbt_pair_t
{
	qse_xptl_t key;
	qse_xptl_t val;

	/* management information below */
	enum
	{
		QSE_RBT_RED,
		QSE_RBT_BLACK
	} color;
	qse_rbt_pair_t* parent;
	qse_rbt_pair_t* child[2]; /* left and right */
};

typedef struct qse_rbt_style_t qse_rbt_style_t;

/**
 * The qse_rbt_style_t type defines callback function sets for key/value 
 * pair manipulation. 
 */
struct qse_rbt_style_t
{
	qse_rbt_copier_t copier[2]; /**< key and value copier */
	qse_rbt_freeer_t freeer[2]; /**< key and value freeer */
	qse_rbt_comper_t comper;    /**< key comparator */
	qse_rbt_keeper_t keeper;    /**< value keeper */
};

/**
 * The qse_rbt_style_kind_t type defines the type of predefined
 * callback set for pair manipulation.
 */
enum qse_rbt_style_kind_t
{
	/** store the key and the value pointer */
	QSE_RBT_STYLE_DEFAULT,
	/** copy both key and value into the pair */
	QSE_RBT_STYLE_INLINE_COPIERS,
	/** copy the key into the pair but store the value pointer */
	QSE_RBT_STYLE_INLINE_KEY_COPIER,
	/** copy the value into the pair but store the key pointer */
	QSE_RBT_STYLE_INLINE_VALUE_COPIER
};

typedef enum qse_rbt_style_kind_t  qse_rbt_style_kind_t;

/**
 * The qse_rbt_t type defines a red-black tree.
 */
struct qse_rbt_t
{
	qse_mmgr_t* mmgr;

	const qse_rbt_style_t* style;

	qse_byte_t       scale[2];  /**< length scale */

	qse_rbt_pair_t   xnil;      /**< internal nil node */

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

#define QSE_RBT_COPIER_DEFAULT (QSE_RBT_COPIER_SIMPLE)
#define QSE_RBT_FREEER_DEFAULT (QSE_NULL)
#define QSE_RBT_COMPER_DEFAULT (qse_rbt_dflcomp)
#define QSE_RBT_KEEPER_DEFAULT (QSE_NULL)

/**
 * The QSE_RBT_SIZE() macro returns the number of pairs in red-black tree.
 */
#define QSE_RBT_SIZE(m)   ((const qse_size_t)(m)->size)
#define QSE_RBT_KSCALE(m) ((const int)(m)->scale[QSE_RBT_KEY])
#define QSE_RBT_VSCALE(m) ((const int)(m)->scale[QSE_RBT_VAL])

#define QSE_RBT_KPTL(p) (&(p)->key)
#define QSE_RBT_VPTL(p) (&(p)->val)

#define QSE_RBT_KPTR(p) ((p)->key.ptr)
#define QSE_RBT_KLEN(p) ((p)->key.len)
#define QSE_RBT_VPTR(p) ((p)->val.ptr)
#define QSE_RBT_VLEN(p) ((p)->val.len)

#define QSE_RBT_NEXT(p) ((p)->next)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The qse_getrbtstyle() functions returns a predefined callback set for
 * pair manipulation.
 */
QSE_EXPORT const qse_rbt_style_t* qse_getrbtstyle (
	qse_rbt_style_kind_t kind
);

/**
 * The qse_rbt_open() function creates a red-black tree.
 * @return qse_rbt_t pointer on success, QSE_NULL on failure.
 */
QSE_EXPORT qse_rbt_t* qse_rbt_open (
	qse_mmgr_t* mmgr,    /**< memory manager */
	qse_size_t  xtnsize, /**< extension size in bytes */
	int         kscale,  /**< key scale */
	int         vscale   /**< value scale */
);

/**
 * The qse_rbt_close() function destroys a red-black tree.
 */
QSE_EXPORT void qse_rbt_close (
	qse_rbt_t* rbt   /**< red-black tree */
);

/**
 * The qse_rbt_init() function initializes a red-black tree
 */
QSE_EXPORT int qse_rbt_init (
	qse_rbt_t*  rbt,    /**< red-black tree */
	qse_mmgr_t* mmgr,   /**< memory manager */
	int         kscale, /**< key scale */
	int         vscale  /**< value scale */
);

/**
 * The qse_rbt_fini() funtion finalizes a red-black tree
 */
QSE_EXPORT void qse_rbt_fini (
	qse_rbt_t* rbt  /**< red-black tree */
);

QSE_EXPORT qse_mmgr_t* qse_rbt_getmmgr (
	qse_rbt_t* rbt
);

QSE_EXPORT void* qse_rbt_getxtn (
	qse_rbt_t* rbt
);

/**
 * The qse_rbt_getstyle() function gets manipulation callback function set.
 */
QSE_EXPORT const qse_rbt_style_t* qse_rbt_getstyle (
	const qse_rbt_t* rbt /**< red-black tree */
);

/**
 * The qse_rbt_setstyle() function sets internal manipulation callback 
 * functions for data construction, destruction, comparison, etc.
 * The callback structure pointed to by \a style must outlive the tree
 * pointed to by \a htb as the tree doesn't copy the contents of the 
 * structure.
 */
QSE_EXPORT void qse_rbt_setstyle (
	qse_rbt_t*             rbt,  /**< red-black tree */
	const qse_rbt_style_t* style /**< callback function set */
);

/**
 * The qse_rbt_getsize() function gets the number of pairs in red-black tree.
 */
QSE_EXPORT qse_size_t qse_rbt_getsize (
	const qse_rbt_t* rbt  /**< red-black tree */
);

/**
 * The qse_rbt_search() function searches red-black tree to find a pair with a 
 * matching key. It returns the pointer to the pair found. If it fails
 * to find one, it returns QSE_NULL.
 * @return pointer to the pair with a maching key, 
 *         or QSE_NULL if no match is found.
 */
QSE_EXPORT qse_rbt_pair_t* qse_rbt_search (
	const qse_rbt_t* rbt,   /**< red-black tree */
	const void*      kptr,  /**< key pointer */
	qse_size_t       klen   /**< the size of the key */
);

/**
 * The qse_rbt_upsert() function searches red-black tree for the pair with a 
 * matching key. If one is found, it updates the pair. Otherwise, it inserts
 * a new pair with the key and the value given. It returns the pointer to the 
 * pair updated or inserted.
 * @return a pointer to the updated or inserted pair on success, 
 *         QSE_NULL on failure. 
 */
QSE_EXPORT qse_rbt_pair_t* qse_rbt_upsert (
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
QSE_EXPORT qse_rbt_pair_t* qse_rbt_ensert (
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
QSE_EXPORT qse_rbt_pair_t* qse_rbt_insert (
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
QSE_EXPORT qse_rbt_pair_t* qse_rbt_update (
	qse_rbt_t* rbt,   /**< red-black tree */
	void*      kptr,  /**< key pointer */
	qse_size_t klen,  /**< key length */
	void*      vptr,  /**< value pointer */
	qse_size_t vlen   /**< value length */
);

/**
 * The qse_rbt_cbsert() function inserts a key/value pair by delegating pair 
 * allocation to a callback function. Depending on the callback function,
 * it may behave like qse_rbt_insert(), qse_rbt_upsert(), qse_rbt_update(),
 * qse_rbt_ensert(), or totally differently. The sample code below inserts
 * a new pair if the key is not found and appends the new value to the
 * existing value delimited by a comma if the key is found.
 *
 * @code
 * qse_rbt_walk_t print_map_pair (qse_rbt_t* map, qse_rbt_pair_t* pair, void* ctx)
 * {
 *   qse_printf (QSE_T("%.*s[%d] => %.*s[%d]\n"),
 *     (int)QSE_RBT_KLEN(pair), QSE_RBT_KPTR(pair), (int)QSE_RBT_KLEN(pair),
 *     (int)QSE_RBT_VLEN(pair), QSE_RBT_VPTR(pair), (int)QSE_RBT_VLEN(pair));
 *   return QSE_RBT_WALK_FORWARD;
 * }
 * 
 * qse_rbt_pair_t* cbserter (
 *   qse_rbt_t* rbt, qse_rbt_pair_t* pair,
 *   void* kptr, qse_size_t klen, void* ctx)
 * {
 *   qse_xstr_t* v = (qse_xstr_t*)ctx;
 *   if (pair == QSE_NULL)
 *   {
 *     // no existing key for the key 
 *     return qse_rbt_allocpair (rbt, kptr, klen, v->ptr, v->len);
 *   }
 *   else
 *   {
 *     // a pair with the key exists. 
 *     // in this sample, i will append the new value to the old value 
 *     // separated by a comma
 *     qse_rbt_pair_t* new_pair;
 *     qse_char_t comma = QSE_T(',');
 *     qse_byte_t* vptr;
 * 
 *     // allocate a new pair, but without filling the actual value. 
 *     // note vptr is given QSE_NULL for that purpose 
 *     new_pair = qse_rbt_allocpair (
 *       rbt, kptr, klen, QSE_NULL, pair->vlen + 1 + v->len); 
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
 *     qse_rbt_freepair (rbt, pair);
 * 
 *     // return the new pair 
 *     return new_pair;
 *   }
 * }
 * 
 * int main ()
 * {
 *   qse_rbt_t* s1;
 *   int i;
 *   qse_char_t* keys[] = { QSE_T("one"), QSE_T("two"), QSE_T("three") };
 *   qse_char_t* vals[] = { QSE_T("1"), QSE_T("2"), QSE_T("3"), QSE_T("4"), QSE_T("5") };
 * 
 *   s1 = qse_rbt_open (
 *     QSE_MMGR_GETDFL(), 0,
 *     QSE_SIZEOF(qse_char_t), QSE_SIZEOF(qse_char_t)
 *   ); // note error check is skipped 
 *   qse_rbt_setstyle (s1, &style1);
 * 
 *   for (i = 0; i < QSE_COUNTOF(vals); i++)
 *   {
 *     qse_xstr_t ctx;
 *     ctx.ptr = vals[i]; ctx.len = qse_strlen(vals[i]);
 *     qse_rbt_cbsert (s1,
 *       keys[i%QSE_COUNTOF(keys)], qse_strlen(keys[i%QSE_COUNTOF(keys)]),
 *       cbserter, &ctx
 *     ); // note error check is skipped
 *   }
 *   qse_rbt_walk (s1, print_map_pair, QSE_NULL);
 * 
 *   qse_rbt_close (s1);
 *   return 0;
 * }
 * @endcode
 */
QSE_EXPORT qse_rbt_pair_t* qse_rbt_cbsert (
	qse_rbt_t*         rbt,      /**< red-black tree */
	void*              kptr,     /**< key pointer */
	qse_size_t         klen,     /**< key length */
	qse_rbt_cbserter_t cbserter, /**< callback function */
	void*              ctx       /**< callback context */
);

/**
 * The qse_rbt_delete() function deletes a pair with a matching key 
 * @return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_rbt_delete (
	qse_rbt_t* rbt,   /**< red-black tree */
	const void* kptr, /**< key pointer */
	qse_size_t klen   /**< key size */
);

/**
 * The qse_rbt_clear() function empties a red-black tree.
 */
QSE_EXPORT void qse_rbt_clear (
	qse_rbt_t* rbt /**< red-black tree */
);

/**
 * The qse_rbt_walk() function traverses a red-black tree in preorder 
 * from the leftmost child.
 */
QSE_EXPORT void qse_rbt_walk (
	qse_rbt_t*       rbt,    /**< red-black tree */
	qse_rbt_walker_t walker, /**< callback function for each pair */
	void*            ctx     /**< pointer to user-specific data */
);

/**
 * The qse_rbt_walk() function traverses a red-black tree in preorder 
 * from the rightmost child.
 */
QSE_EXPORT void qse_rbt_rwalk (
	qse_rbt_t*       rbt,    /**< red-black tree */
	qse_rbt_walker_t walker, /**< callback function for each pair */
	void*            ctx     /**< pointer to user-specific data */
);

/**
 * The qse_rbt_allocpair() function allocates a pair for a key and a value 
 * given. But it does not chain the pair allocated into the red-black tree @a rbt.
 * Use this function at your own risk. 
 *
 * Take note of he following special behavior when the copier is 
 * #QSE_RBT_COPIER_INLINE.
 * - If @a kptr is #QSE_NULL, the key space of the size @a klen is reserved but
 *   not propagated with any data.
 * - If @a vptr is #QSE_NULL, the value space of the size @a vlen is reserved
 *   but not propagated with any data.
 */
QSE_EXPORT qse_rbt_pair_t* qse_rbt_allocpair (
	qse_rbt_t* rbt,
	void*      kptr, 
	qse_size_t klen,	
	void*      vptr,
	qse_size_t vlen
);

/**
 * The qse_rbt_freepair() function destroys a pair. But it does not detach
 * the pair destroyed from the red-black tree @a rbt. Use this function at your
 * own risk.
 */
QSE_EXPORT void qse_rbt_freepair (
	qse_rbt_t*      rbt,
	qse_rbt_pair_t* pair
);

/**
 * The qse_rbt_dflcomp() function defines the default key comparator.
 */
QSE_EXPORT int qse_rbt_dflcomp (
	const qse_rbt_t* rbt,
	const void*      kptr1,
	qse_size_t       klen1,
	const void*      kptr2,
	qse_size_t       klen2
);

#ifdef __cplusplus
}
#endif

#endif
