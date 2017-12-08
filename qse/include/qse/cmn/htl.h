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

#ifndef _QSE_HTL_T_
#define _QSE_HTL_T_

/** \file
 * This file provides the hash table for fixed-size data.
 */

#include <qse/types.h>
#include <qse/macros.h>

/**
 * The #qse_htl_walk_t type defines walking directions.
 */
enum qse_htl_walk_t
{
	QSE_HTL_WALK_STOP     = 0,
	QSE_HTL_WALK_FORWARD  = 1,
};
typedef enum qse_htl_walk_t qse_htl_walk_t;

typedef struct qse_htl_t qse_htl_t;
typedef struct qse_htl_node_t  qse_htl_node_t;

/**
 * The #qse_htl_hasher_t type defines a data hasher function.
 */
typedef qse_uint32_t (*qse_htl_hasher_t) (
	qse_htl_t*  htl,
	const void* data
);

/**
 * The #qse_htl_comper_t type defines a key comparator that is called when
 * the list needs to compare data.  The comparator must return 0 if the data
 * are the same and a non-zero integer otherwise.
 */
typedef int (*qse_htl_comper_t) (
	qse_htl_t*  htl,   /**< hash table */
	const void* data1, /**< data pointer */
	const void* data2  /**< data pointer */
);

/**
 * The #qse_htl_feeeer_t type defines a data deallocation function.
 */
typedef void (*qse_htl_freeer_t) (
	qse_htl_t*  htl,
	void*       data
);

/**
 * The #qse_htl_copier_t type defines a data copier function.
 */
typedef void* (*qse_htl_copier_t) (
	qse_htl_t*  htl,
	void*       data
);

/**
 * The #qse_htl_walker_t function defines a callback function 
 * for qse_htl_walk().
 */
typedef qse_htl_walk_t (*qse_htl_walker_t) (
	qse_htl_t* htl,
	void*      data,
	void*      ctx
);

struct qse_htl_node_t
{
	struct qse_htl_node_t* next;
	qse_uint32_t           reversed;
	qse_uint32_t           key;
 	void*                  data;
};

struct qse_htl_t 
{
	qse_mmgr_t* mmgr;

	int keysize; /* default key size */

	int num_elements;
	int num_buckets; /* power of 2 */
	int next_grow;
	int mask;

	qse_htl_hasher_t hasher;
	qse_htl_comper_t comper;
	qse_htl_freeer_t freeer;
	qse_htl_copier_t copier;

	qse_htl_node_t   null;
	qse_htl_node_t** buckets;
};

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * The qse_htl_open() function creates an hash table.
 */
QSE_EXPORT qse_htl_t* qse_htl_open (
	qse_mmgr_t* mmgr,
	qse_size_t  xtnsize,
	int         scale
);

/**
 * The qse_htl_close() function destroys an hash table.
 */
QSE_EXPORT void qse_htl_close (
	qse_htl_t* htl  /**< hash table */
);

/**
 * The qse_htl_open() function initializes an hash table.
 */
QSE_EXPORT int qse_htl_init (
	qse_htl_t*  htl,
	qse_mmgr_t* mmgr,
	int         scale
);

/**
 * The qse_htl_close() function finalizes an hash table.
 */
QSE_EXPORT void qse_htl_fini (
	qse_htl_t* htl  /**< hash table */
);


#if defined(QSE_HAVE_INLINE)
static QSE_INLINE qse_mmgr_t* qse_htl_getmmgr (qse_htl_t* htl) { return (htl)->mmgr; }
#else
#define qse_htl_getmmgr(htl) ((htl)->mmgr))
#endif

#if defined(QSE_HAVE_INLINE)
static QSE_INLINE void* qse_htl_getxtn (qse_htl_t* htl) { return QSE_XTN(htl); }
#else
#define qse_htl_getxtn(htl) (QSE_XTN(htl))
#endif

#if defined(QSE_HAVE_INLINE)
static QSE_INLINE int qse_htl_getsize (qse_htl_t* htl) { return htl->num_elements; }
#else
#define qse_htl_getsize(htl) ((htl)->num_elements)
#endif

#if defined(QSE_HAVE_INLINE)
static QSE_INLINE qse_htl_hasher_t qse_htl_gethasher (qse_htl_t* htl) { return htl->hasher; }
static QSE_INLINE qse_htl_comper_t qse_htl_getcomper (qse_htl_t* htl) { return htl->comper; }
static QSE_INLINE qse_htl_freeer_t qse_htl_getfreeer (qse_htl_t* htl) { return htl->freeer; }

static QSE_INLINE void qse_htl_sethasher (qse_htl_t* htl, qse_htl_hasher_t _hasher) { htl->hasher = _hasher; }
static QSE_INLINE void qse_htl_setcomper (qse_htl_t* htl, qse_htl_comper_t _comper) { htl->comper = _comper; }
static QSE_INLINE void qse_htl_setfreeer (qse_htl_t* htl, qse_htl_freeer_t _freeer) { htl->freeer = _freeer; }
static QSE_INLINE void qse_htl_setcopier (qse_htl_t* htl, qse_htl_copier_t _copier) { htl->copier = _copier; }
#else
#define qse_htl_gethasher(htl) ((htl)->hasher)
#define qse_htl_getcomper(htl) ((htl)->comper)
#define qse_htl_getfreeer(htl) ((htl)->freeer)

#define qse_htl_sethasher(htl,_hahser) ((htl)->hasher = _hasher)
#define qse_htl_setcomper(htl,_comper) ((htl)->comper = _comper)
#define qse_htl_setfreeer(htl,_freeer) ((htl)->freeer = _freeer)
#define qse_htl_setcopier(htl,_copier) ((htl)->copier = _copier)
#endif

/**
 * The qse_htl_search() function searches a hash table to find a 
 * matching datum. It returns the index to the slot of an internal array
 * where the matching datum is found. 
 * \return slot index if a match if found,
 *         #QSE_HTL_NIL if no match is found.
 */
QSE_EXPORT qse_htl_node_t* qse_htl_search (
	qse_htl_t*       htl,    /**< hash table */
	const void*      data    /**< data pointer */
);

QSE_EXPORT qse_htl_node_t* qse_htl_heterosearch (
	qse_htl_t*       htl,     /**< hash table */
	const void*      data,    /**< data pointer */
	qse_htl_hasher_t hasher,
	qse_htl_comper_t comper
);

/**
 * The qse_htl_insert() function inserts a new datum. It fails if it finds
 * an existing datum.
 * \return slot index where the new datum is inserted on success,
 *         #QSE_HTL_NIL on failure.
 */
QSE_EXPORT qse_htl_node_t* qse_htl_insert (
	qse_htl_t*  htl,    /**< hash table */
	void*       data    /**< data pointer */
);

/**
 * The qse_htl_upsert() function inserts a new datum if it finds no matching
 * datum or updates an exsting datum if finds a matching datum.
 * \return slot index where the datum is inserted or updated.
 */
QSE_EXPORT qse_htl_node_t* qse_htl_upsert (
	qse_htl_t*       htl,    /**< hash table */
	void*            data    /**< data pointer */
);

/**
 * The qse_htl_update() function updates an existing datum. It fails if it finds
 * no existing datum.
 * \return slot index where an existing datum is updated on success,
 *         #QSE_HTL_NIL on failure.
 */
QSE_EXPORT qse_htl_node_t* qse_htl_update (
	qse_htl_t*       htl,    /**< hash table */
	void*            data    /**< data pointer */
);

/**
 * The qse_htl_ensert() function inserts a new item if one is not found.
 * It returns an existing item otherwise.
 */
QSE_EXPORT qse_htl_node_t* qse_htl_ensert (
	qse_htl_t*       htl,    /**< hash table */
	void*            data    /**< data pointer */
);

/**
 * The qse_htl_delete() function deletes an existing datum. It fails if it finds
 * no existing datum.
 * \return 0 on success, -1 on failure
 */
QSE_EXPORT int qse_htl_delete (
	qse_htl_t*       htl,    /**< hash table */
	void*            data    /**< data pointer */
);

QSE_EXPORT void* qse_htl_yank (
	qse_htl_t*       ht,
	void*            data
);

/**
 * The qse_htl_clear() functions deletes all data items.
 */
QSE_EXPORT void qse_htl_clear (
	qse_htl_t*       htl     /**< hash table */
);

/**
 * The qse_htl_walk() function executes the callback function @a walker for
 * each valid data item.
 */
QSE_EXPORT void qse_htl_walk (
	qse_htl_t*       htl,    /**< hash table */
	qse_htl_walker_t walker, /**< callback function */
	void*            ctx     /**< context */
);

/* ------------------------------------------------------------------------- */

QSE_EXPORT qse_uint32_t qse_genhash32_update (const void* data, qse_size_t size, qse_uint32_t hash);
QSE_EXPORT qse_uint32_t qse_genhash32 (const void *data, qse_size_t size);
/*qse_uint32_t qse_foldhash32 (qse_uint32_t hash, int bits);*/

QSE_EXPORT qse_uint32_t qse_mbshash32 (const qse_mchar_t* p);
QSE_EXPORT qse_uint32_t qse_wcshash32 (const qse_wchar_t* p);
QSE_EXPORT qse_uint32_t qse_mbscasehash32 (const qse_mchar_t* p);
QSE_EXPORT qse_uint32_t qse_wcscasehash32 (const qse_wchar_t* p);

#if defined(QSE_CHAR_IS_WCHAR)
#	define qse_strhash32(x)     qse_wcshash32(x)
#	define qse_strcasehash32(x) qse_wcscasehash32(x)
#else
#	define qse_strhash32(x)     qse_mbshash32(x)
#	define qse_strcasehash32(x) qse_mbscasehash32(x)
#endif

#if defined(__cplusplus)
}
#endif

#endif
