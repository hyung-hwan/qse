/*
 * $Id: map.h 75 2009-02-22 14:10:34Z hyunghwan.chung $
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef _QSE_CMN_MAP_H_
#define _QSE_CMN_MAP_H_

#include <qse/types.h>
#include <qse/macros.h>

/****o* Common/Hash Map
 * DESCRIPTION
 *  A hash map maintains buckets for key/value pairs with the same key hash
 *  chained under the same bucket.
 *
 *  #include <qse/cmn/map.h>
 ******
 */

/* values that can be returned by qse_map_walker_t */
enum qse_map_walk_t
{
        QSE_MAP_WALK_STOP = 0,
        QSE_MAP_WALK_FORWARD = 1
};

enum qse_map_id_t
{
	QSE_MAP_KEY = 0,
	QSE_MAP_VAL = 1
};

typedef struct qse_map_t qse_map_t;
typedef struct qse_map_pair_t qse_map_pair_t;
typedef enum qse_map_walk_t qse_map_walk_t;
typedef enum qse_map_id_t   qse_map_id_t;

/****t* Common/qse_map_copier_t
 * NAME
 *  qse_map_copier_t - define a pair contruction callback
 * SYNOPSIS
 */
typedef void* (*qse_map_copier_t) (
	qse_map_t* map  /* a map */,
	void*      dptr /* the pointer to a key or a value */, 
	qse_size_t dlen /* the length of a key or a value */
);
/******/

/****t* Common/qse_map_freeer_t
 * NAME
 *  qse_map_freeer_t - define a key/value destruction callback
 * SYNOPSIS
 */
typedef void (*qse_map_freeer_t) (
	qse_map_t* map  /* a map */,
	void*      dptr /* the pointer to a key or a value */, 
	qse_size_t dlen /* the length of a key or a value */
);
/******/

/* key hasher */
typedef qse_size_t (*qse_map_hasher_t) (
	qse_map_t* map   /* a map */, 
	const void* kptr /* the pointer to a key */, 
	qse_size_t klen  /* the length of a key in bytes */
);

/****t* Common/qse_map_comper_t
 * NAME
 *  qse_map_comper_t - define a key comparator
 *
 * DESCRIPTION
 *  The qse_map_comper_t type defines a key comparator that is called when
 *  the map needs to compare keys. A map is created with a default comparator
 *  which performs bitwise comparison between two keys.
 *
 *  The comparator should return 0 if the keys are the same and a non-zero
 *  integer otherwise.
 *
 * SYNOPSIS
 */
typedef int (*qse_map_comper_t) (
	qse_map_t*  map    /* a map */, 
	const void* kptr1  /* the pointer to a key */, 
	qse_size_t  klen1  /* the length of a key */, 
	const void* kptr2  /* the pointer to a key */, 
	qse_size_t  klen2  /* the length of a key */
);
/******/

/****t* Common/qse_map_keeper_t
 * NAME
 *  qse_map_keeper_t - define a value keeper
 *
 * DESCRIPTION
 *  The qse_map_keeper_t type defines a value keeper that is called when 
 *  a value is retained in the context that it should be destroyed because
 *  it is identical to a new value. Two values are identical if their beginning
 *  pointers and their lengths are equal.
 *
 * SYNOPSIS
 */
typedef void (*qse_map_keeper_t) (
	qse_map_t* map     /* a map */,
	void*      vptr    /* the pointer to a value */,
	qse_size_t vlen    /* the length of a value */	
);
/******/

/****t* Common/qse_map_sizer_t
 * NAME
 *  qse_map_sizer_t - define a bucket size calculator
 *
 * DESCRIPTION
 *  The qse_map_sizer_T type defines a bucket size claculator that is called
 *  when a map should resize the bucket. The current bucket size +1 is passed
 *  as the hint.
 * 
 * SYNOPSIS
 */
typedef qse_size_t (*qse_map_sizer_t) (
	qse_map_t* map,  /* a map */
	qse_size_t hint  /* a sizing hint */
);
/******/

/****t* Common/qse_map_walker_t
 * NAME
 *  qse_map_walker_t - define a pair visitor
 *
 * SYNOPSIS
 */
typedef qse_map_walk_t (*qse_map_walker_t) (
	qse_map_t*      map   /* a map */, 
	qse_map_pair_t* pair  /* the pointer to a key/value pair */, 
	void*           arg   /* the pointer to user-defined data */
);
/******/

/****s* Common/qse_map_pair_t
 * NAME
 *  qse_map_pair_t - define a pair
 *
 * DESCRIPTION
 *  A pair is composed of a key and a value. It maintains pointers to the 
 *  beginning of a key and a value plus their length. The length is scaled
 *  down with the scale factor specified in an owning map. Use macros defined
 *  in the SEE ALSO section below to access individual fields.
 *
 * SEE ALSO
 *  QSE_MAP_KPTR, QSE_MAP_KLEN, QSE_MAP_VPTR, QSE_MAP_VLEN
 *
 * SYNOPSIS
 */
struct qse_map_pair_t
{
	void*           kptr;  /* the pointer to a key */
	qse_size_t      klen;  /* the length of a key */
	void*           vptr;  /* the pointer to a value */
	qse_size_t      vlen;  /* the length of a value */
	qse_map_pair_t* next;  /* the next pair under the same slot */
};
/*****/

/****s* Common/qse_map_t
 * NAME
 *  qse_map_t - define a hash map
 *
 * SYNOPSIS
 */
struct qse_map_t
{
	QSE_DEFINE_COMMON_FIELDS (map)

        qse_map_copier_t copier[2];
        qse_map_freeer_t freeer[2];
	qse_map_hasher_t hasher;   /* key hasher */
	qse_map_comper_t comper;   /* key comparator */
	qse_map_keeper_t keeper;   /* value keeper */
	qse_map_sizer_t  sizer;    /* bucket capacity recalculator */
	qse_byte_t       scale[2]; /* length scale */
	qse_byte_t       factor;   /* load factor */
	qse_byte_t       filler0;
	qse_size_t       size;
	qse_size_t       capa;
	qse_size_t       threshold;
	qse_map_pair_t** bucket;
};
/******/

#define QSE_MAP_COPIER_SIMPLE ((qse_map_copier_t)1)
#define QSE_MAP_COPIER_INLINE ((qse_map_copier_t)2)

/****d* Common/QSE_MAP_SIZE
 * NAME
 *  QSE_MAP_SIZE - get the number of pairs
 * DESCRIPTION
 *  The QSE_MAP_SIZE() macro returns the number of pairs in a map.
 * SYNOPSIS
 */
#define QSE_MAP_SIZE(m) ((m)->size)
/*****/

/****d* Common/QSE_MAP_CAPA
 * NAME
 *  QSE_MAP_CAPA - get the capacity of a map
 *
 * DESCRIPTION
 *  The QSE_MAP_CAPA() macro returns the maximum number of pairs a map can hold.
 *
 * SYNOPSIS
 */
#define QSE_MAP_CAPA(m) ((m)->capa)
/*****/

#define QSE_MAP_KCOPIER(m)   ((m)->copier[QSE_MAP_KEY])
#define QSE_MAP_VCOPIER(m)   ((m)->copier[QSE_MAP_VAL])
#define QSE_MAP_KFREEER(m)   ((m)->freeer[QSE_MAP_KEY])
#define QSE_MAP_VFREEER(m)   ((m)->freeer[QSE_MAP_VAL])
#define QSE_MAP_HASHER(m)    ((m)->hasher)
#define QSE_MAP_COMPER(m)    ((m)->comper)
#define QSE_MAP_KEEPER(m)    ((m)->keeper)
#define QSE_MAP_SIZER(m)     ((m)->sizer)

#define QSE_MAP_FACTOR(m)    ((m)->factor)
#define QSE_MAP_KSCALE(m)    ((m)->scale[QSE_MAP_KEY])
#define QSE_MAP_VSCALE(m)    ((m)->scale[QSE_MAP_VAL])

#define QSE_MAP_KPTR(p) ((p)->kptr)
#define QSE_MAP_KLEN(p) ((p)->klen)
#define QSE_MAP_VPTR(p) ((p)->vptr)
#define QSE_MAP_VLEN(p) ((p)->vlen)
#define QSE_MAP_NEXT(p) ((p)->next)

#ifdef __cplusplus
extern "C" {
#endif

QSE_DEFINE_COMMON_FUNCTIONS (map)

/****f* Common/qse_map_open
 * NAME
 *  qse_map_open - creates a hash map
 * DESCRIPTION 
 *  The qse_map_open() function creates a hash map with a dynamic array 
 *  bucket and a list of values chained. The initial capacity should be larger
 *  than 0. The load factor should be between 0 and 100 inclusive and the load
 *  factor of 0 disables bucket resizing. If you need extra space associated
 *  with a map, you may pass a non-zero value as the second parameter. 
 *  The QSE_MAP_XTN() macro and the qse_map_getxtn() function return the 
 *  pointer to the beginning of the extension.
 * RETURN
 *  The qse_map_open() function returns an qse_map_t pointer on success and 
 *  QSE_NULL on failure.
 * SEE ALSO 
 *  QSE_MAP_XTN, qse_map_getxtn
 * SYNOPSIS
 */
qse_map_t* qse_map_open (
	qse_mmgr_t* mmgr   /* a memory manager */,
	qse_size_t  ext    /* extension size in bytes */,
	qse_size_t  capa   /* initial capacity */,
	int         factor /* load factor */
);
/******/


/****f* Common/qse_map_close 
 * NAME
 *  qse_map_close - destroy a hash map
 * DESCRIPTION 
 *  The qse_map_close() function destroys a hash map.
 * SYNOPSIS
 */
void qse_map_close (
	qse_map_t* map /* a map */
);
/******/

qse_map_t* qse_map_init (
	qse_map_t*  map,
	qse_mmgr_t* mmgr,
	qse_size_t  capa,
	int         factor
);

void qse_map_fini (
	qse_map_t* map
);

/* get the number of key/value pairs in a map */
qse_size_t qse_map_getsize (
	qse_map_t* map /* a map */
);

qse_size_t qse_map_getcapa (
	qse_map_t* map /* a map */
);

int qse_map_getscale (
	qse_map_t* map   /* a map */,
	qse_map_id_t id  /* QSE_MAP_KEY or QSE_MAP_VAL */
);

/****f* Common/qse_map_setscale
 * NAME
 *  qse_map_setscale - set the scale factor
 *
 * DESCRIPTION 
 *  The qse_map_setscale() function sets the scale factor of the length
 *  of a key and a value. A scale factor determines the actual length of
 *  a key and a value in bytes. A map is created with a scale factor of 1.
 *  The scale factor should be larger than 0 and less than 256.
 *
 * NOTES
 *  It is a bad idea to change the scale factor when a map is not empty.
 *  
 * SYNOPSIS
 */
void qse_map_setscale (
	qse_map_t* map   /* a map */,
	qse_map_id_t id  /* QSE_MAP_KEY or QSE_MAP_VAL */,
	int scale        /* a scale factor */
);
/******/

qse_map_copier_t qse_map_getcopier (
	qse_map_t* map   /* a map */,
	qse_map_id_t id  /* QSE_MAP_KEY or QSE_MAP_VAL */
);

/****f* Common/qse_map_setcopier
 * NAME 
 *  qse_map_setcopier - specify how to clone an element
 *
 * DESCRIPTION
 *  A special copier QSE_MAP_COPIER_INLINE is provided. This copier enables
 *  you to copy the data inline to the internal node. No freeer is invoked
 *  when the node is freeed.
 *
 *  You may set the copier to QSE_NULL to perform no special operation 
 *  when the data pointer is rememebered.
 *
 * SYNOPSIS
 */
void qse_map_setcopier (
	qse_map_t* map           /* a map */, 
	qse_map_id_t id          /* QSE_MAP_KEY or QSE_MAP_VAL */,
	qse_map_copier_t copier  /* an element copier */
);
/******/

qse_map_freeer_t qse_map_getfreeer (
	qse_map_t*   map  /* a map */,
	qse_map_id_t id   /* QSE_MAP_KEY or QSE_MAP_VAL */
);

/****f* Common/qse_map_setfreeer
 * NAME 
 *  qse_map_setfreeer - specify how to destroy an element
 *
 * DESCRIPTION
 *  The freeer is called when a node containing the element is destroyed.
 *
 * SYNOPSIS
 */
void qse_map_setfreeer (
	qse_map_t* map           /* a map */,
	qse_map_id_t id          /* QSE_MAP_KEY or QSE_MAP_VAL */,
	qse_map_freeer_t freeer  /* an element freeer */
);
/******/


qse_map_hasher_t qse_map_gethasher (
	qse_map_t* map
);

void qse_map_sethasher (
	qse_map_t* map,
	qse_map_hasher_t hasher
);

qse_map_comper_t qse_map_getcomper (
	qse_map_t* map
);

void qse_map_setcomper (
	qse_map_t* map,
	qse_map_comper_t comper
);

qse_map_keeper_t qse_map_getkeeper (
	qse_map_t* map
);

void qse_map_setkeeper (
	qse_map_t* map,
	qse_map_keeper_t keeper
);

qse_map_sizer_t qse_map_getsizer (
	qse_map_t* map
);

/* the sizer function is passed a map object and map->capa + 1 */
void qse_map_setsizer (
	qse_map_t* map,
	qse_map_sizer_t sizer
);

/****f* Common/qse_map_search
 * NAME
 *  qse_map_search - find a pair with a matching key 
 * DESCRIPTION
 *  The qse_map_search() function searches a map to find a pair with a 
 *  matching key. It returns the pointer to the pair found. If it fails
 *  to find one, it returns QSE_NULL.
 * RETURN
 *  The qse_map_search() function returns the pointer to the pair with a 
 *  maching key, and QSE_NULL if no match is found.
 * SYNOPSIS
 */
qse_map_pair_t* qse_map_search (
	qse_map_t*  map   /* a map */,
	const void* kptr  /* the pointer to a key */,
	qse_size_t  klen  /* the size of the key in bytes */
);
/******/

/****f* Common/qse_map_upsert
 * NAME
 *  qse_map_upsert - update an existing pair or inesrt a new pair
 * DESCRIPTION 
 *  The qse_map_upsert() function searches a map for the pair with a matching
 *  key. If one is found, it updates the pair. Otherwise, it inserts a new
 *  pair with a key and a value. It returns the pointer to the pair updated 
 *  or inserted.
 * RETURN 
 *  The qse_map_upsert() function returns a pointer to the updated or inserted
 *  pair on success, and QSE_NULL on failure. 
 * SYNOPSIS
 */
qse_map_pair_t* qse_map_upsert (
	qse_map_t* map   /* a map */,
	void*      kptr  /* the pointer to a key */,
	qse_size_t klen  /* the length of the key in bytes */,
	void*      vptr  /* the pointer to a value */,
	qse_size_t vlen  /* the length of the value in bytes */
);
/******/

/****f* Common/qse_map_insert 
 * NAME
 *  qse_map_insert - insert a new pair with a key and a value 
 * DESCRIPTION
 *  The qse_map_insert() function inserts a new pair with the key and the value
 *  given. If there exists a pair with the key given, the function returns 
 *  QSE_NULL without channging the value.
 * RETURN 
 *  The qse_map_insert() function returns a pointer to the pair created on 
 *  success, and QSE_NULL on failure. 
 * SYNOPSIS
 */
qse_map_pair_t* qse_map_insert (
	qse_map_t* map   /* a map */,
	void*      kptr  /* the pointer to a key */,
	qse_size_t klen  /* the length of the key in bytes */,
	void*      vptr  /* the pointer to a value */,
	qse_size_t vlen  /* the length of the value in bytes */
);
/******/

/* update the value of a existing pair with a matching key */
qse_map_pair_t* qse_map_update (
	qse_map_t* map   /* a map */,
	void* kptr       /* the pointer to a key */,
	qse_size_t klen  /* the length of the key in bytes */,
	void* vptr       /* the pointer to a value */,
	qse_size_t vlen  /* the length of the value in bytes */
);

/* delete a pair with a matching key */
int qse_map_delete (
	qse_map_t* map   /* a map */,
	const void* kptr /* the pointer to a key */,
	qse_size_t klen  /* the size of the key in bytes */
);

/* clear a map */
void qse_map_clear (
	qse_map_t* map /* a map */
);

/* traverse a map */
void qse_map_walk (
	qse_map_t* map          /* a map */,
	qse_map_walker_t walker /* the pointer to the function for each pair */,
	void* arg               /* a pointer to user-specific data */
);

/* get the pointer to the first pair in the map. */
qse_map_pair_t* qse_map_getfirstpair (
	qse_map_t* map /* a map */, 
	qse_size_t* buckno
);

/* get the pointer to the next pair in the map. */
qse_map_pair_t* qse_map_getnextpair (
	qse_map_t* map /* a map */,
	qse_map_pair_t* pair,
	qse_size_t* buckno
);

#ifdef __cplusplus
}
#endif

#endif
