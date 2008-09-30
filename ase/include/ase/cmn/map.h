/*
 * $Id: map.h 398 2008-09-29 10:01:15Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_CMN_MAP_H_
#define _ASE_CMN_MAP_H_

#include <ase/types.h>
#include <ase/macros.h>

/****o* ase.cmn.map/hash map
 * DESCRIPTION
 *  A hash map maintains buckets for key/value pairs with the same key hash
 *  chained under the same bucket.
 *
 *  #include <ase/cmn/map.h>
 *
 * EXAMPLES
 *  void f (void)
 *  {
 *  }
 ******
 */

/* values that can be returned by ase_map_walker_t */
enum ase_map_walk_t
{
        ASE_MAP_WALK_STOP = 0,
        ASE_MAP_WALK_FORWARD = 1
};

enum ase_map_id_t
{
	ASE_MAP_KEY = 0,
	ASE_MAP_VAL = 1
};

typedef struct ase_map_t ase_map_t;
typedef struct ase_map_pair_t ase_map_pair_t;
typedef enum ase_map_walk_t ase_map_walk_t;
typedef enum ase_map_id_t   ase_map_id_t;

/****b* ase.cmn.map/ase_map_copier_t
 * NAME
 *  ase_map_copier_t - define a pair contruction callback
 * SYNOPSIS
 */
typedef void* (*ase_map_copier_t) (
	ase_map_t* map  /* a map */,
	void*      dptr /* the pointer to a key or a value */, 
	ase_size_t dlen /* the length of a key or a value */
);
/******/

/****b* ase.cmn.map/ase_map_freeer_t
 * NAME
 *  ase_map_freeer_t - define a key/value destruction callback
 * SYNOPSIS
 */
typedef void (*ase_map_freeer_t) (
	ase_map_t* map  /* a map */,
	void*      dptr /* the pointer to a key or a value */, 
	ase_size_t dlen /* the length of a key or a value */
);
/******/

/* key hasher */
typedef ase_size_t (*ase_map_hasher_t) (
	ase_map_t* map   /* a map */, 
	const void* kptr /* the pointer to a key */, 
	ase_size_t klen  /* the length of a key in bytes */
);

/****t* ase.cmn.map/ase_map_comper_t
 * NAME
 *  ase_map_comper_t - define a key comparator
 *
 * DESCRIPTION
 *  The ase_map_comper_t type defines a key comparator that is called when
 *  the map needs to compare keys. A map is created with a default comparator
 *  which performs bitwise comparison between two keys.
 *
 *  The comparator should return 0 if the keys are the same and a non-zero
 *  integer otherwise.
 *
 * SYNOPSIS
 */
typedef int (*ase_map_comper_t) (
	ase_map_t*  map    /* a map */, 
	const void* kptr1  /* the pointer to a key */, 
	ase_size_t  klen1  /* the length of a key */, 
	const void* kptr2  /* the pointer to a key */, 
	ase_size_t  klen2  /* the length of a key */
);
/******/

/****t* ase.cmn.map/ase_map_keeper_t
 * NAME
 *  ase_map_keeper_t - define a value keeper
 *
 * DESCRIPTION
 *  The ase_map_keeper_t type defines a value keeper that is called when 
 *  a value is retained in the context that it should be destroyed because
 *  it is identical to a new value. Two values are identical if their beginning
 *  pointers and their lengths are equal.
 *
 * SYNOPSIS
 */
typedef void (*ase_map_keeper_t) (
	ase_map_t* map     /* a map */,
	void* vptr         /* the pointer to a value */,
	ase_size_t vlen    /* the length of a value */	
);
/******/

/*
 * bucket resizer
 */
typedef ase_size_t (*ase_map_sizer_t) (ase_map_t* data, ase_size_t hint);

/* pair visitor - should return ASE_MAP_WALK_STOP or ASE_MAP_WALK_FORWARD */
typedef ase_map_walk_t (*ase_map_walker_t) (
	ase_map_t* map        /* a map */, 
	ase_map_pair_t* pair  /* the pointer to a key/value pair */, 
	void* arg             /* the pointer to user-defined data */
);

/****s* ase.cmn.map/ase_map_pair_t
 * NAME
 *  ase_map_pair_t - define a pair
 *
 * DESCRIPTION
 *  A pair is composed of a key and a value. It maintains pointers to the 
 *  beginning of a key and a value plus their length. The length is scaled
 *  down with the scale factor specified in an owning map. Use macros defined
 *  in the SEE ALSO section below to access individual fields.
 *
 * SEE ALSO
 *  ASE_MAP_KPTR, ASE_MAP_KLEN, ASE_MAP_VPTR, ASE_MAP_VLEN
 *
 * SYNOPSIS
 */
struct ase_map_pair_t
{
	void* kptr;           /* the pointer to a key */
	ase_size_t klen;      /* the length of a key */

	void* vptr;           /* the pointer to a value */
	ase_size_t vlen;      /* the length of a value */

	ase_map_pair_t* next; /* the next pair under the same slot */
};
/*****/

/****s* ase.cmn.map/ase_map_t
 * NAME
 *  ase_map_t - define a hash map
 *
 * SYNOPSIS
 */
struct ase_map_t
{
        ase_mmgr_t* mmgr;

        ase_map_copier_t copier[2];
        ase_map_freeer_t freeer[2];
	ase_map_hasher_t hasher; /* key hasher */
	ase_map_comper_t comper; /* key comparator */
	ase_map_keeper_t keeper; /* value keeper */
	ase_map_sizer_t  sizer;  /* bucket capacity recalculator */

	ase_byte_t scale[2];     /* length scale */
	ase_byte_t factor;       /* load factor */
	ase_byte_t filler0;

	ase_size_t size;
	ase_size_t capa;
	ase_size_t threshold;

	ase_map_pair_t** bucket;
};
/******/



#define ASE_MAP_COPIER_INLINE ase_map_copyinline

/****d* ase.cmn.map/ASE_MAP_SIZE
 * NAME
 *  ASE_MAP_SIZE - get the number of pairs
 * 
 * DESCRIPTION
 *  The ASE_MAP_SIZE() macro returns the number of pairs in a map.
 *
 * SYNOPSIS
 */
#define ASE_MAP_SIZE(m) ((m)->size)
/*****/

/****d* ase.cmn.map/ASE_MAP_CAPA
 * NAME
 *  ASE_MAP_CAPA - get the capacity of a map
 *
 * DESCRIPTION
 *  The ASE_MAP_CAPA() macro returns the maximum number of pairs a map can hold.
 *
 * SYNOPSIS
 */
#define ASE_MAP_CAPA(m) ((m)->capa)
/*****/

#define ASE_MAP_MMGR(m)      ((m)->mmgr)
#define ASE_MAP_KCOPIER(m)   ((m)->copier[ASE_MAP_KEY])
#define ASE_MAP_VCOPIER(m)   ((m)->copier[ASE_MAP_VAL])
#define ASE_MAP_KFREEER(m)   ((m)->freeer[ASE_MAP_KEY])
#define ASE_MAP_VFREEER(m)   ((m)->freeer[ASE_MAP_VAL])
#define ASE_MAP_HASHER(m)    ((m)->hasher)
#define ASE_MAP_COMPER(m)    ((m)->comper)
#define ASE_MAP_KEEPER(m)    ((m)->keeper)
#define ASE_MAP_SIZER(m)     ((m)->sizer)
#define ASE_MAP_EXTENSION(m) ((void*)(((ase_map_t*)m) + 1))

#define ASE_MAP_FACTOR(m)    ((m)->factor)
#define ASE_MAP_KSCALE(m)    ((m)->scale[ASE_MAP_KEY])
#define ASE_MAP_VSCALE(m)    ((m)->scale[ASE_MAP_VAL])

#define ASE_MAP_KPTR(p) ((p)->kptr)
#define ASE_MAP_KLEN(p) ((p)->klen)
#define ASE_MAP_VPTR(p) ((p)->vptr)
#define ASE_MAP_VLEN(p) ((p)->vlen)
#define ASE_MAP_NEXT(p) ((p)->next)

#ifdef __cplusplus
extern "C" {
#endif

/****f* ase.cmn.map/ase_map_open
 * NAME
 *  ase_map_open - creates a hash map
 *
 * DESCRIPTION 
 *  The ase_map_open() function creates a hash map with a dynamic array 
 *  bucket and a list of values chained. The initial capacity should be larger
 *  than 0. The load factor should be between 0 and 100 inclusive and the load
 *  factor of 0 disables bucket resizing. If you need extra space associated
 *  with a map, you may pass a non-zero value as the second parameter. 
 *  The ASE_MAP_EXTENSION() macro and the ase_map_getextension() function 
 *  return the pointer to the beginning of the extension.
 *
 * RETURN
 *  The ase_map_open() function returns an ase_map_t pointer on success and 
 *  ASE_NULL on failure.
 *
 * SEE ALSO 
 *  ASE_MAP_EXTENSION, ase_map_getextension
 * 
 * SYNOPSIS
 */
ase_map_t* ase_map_open (
	ase_mmgr_t* mmgr /* a memory manager */,
	ase_size_t ext   /* extension size in bytes */,
	ase_size_t capa  /* initial capacity */,
	int factor       /* load factor */
);
/******/


/****f* ase.cmn.map/ase_map_close 
 * NAME
 *  ase_map_close - destroy a hash map
 *  
 * DESCRIPTION 
 *  The ase_map_close() function destroys a hash map.
 *
 * SYNOPSIS
 */
void ase_map_close (
	ase_map_t* map /* a map */
);
/******/

ase_map_t* ase_map_init (
	ase_map_t* map,
	ase_mmgr_t* mmgr,
	ase_size_t capa,
	int factor
);

void ase_map_fini (
	ase_map_t* map
);

void* ase_map_getextension (
	ase_map_t* map
);

ase_mmgr_t* ase_map_getmmgr (
	ase_map_t* map
);

void ase_map_setmmgr (
	ase_map_t* map,
	ase_mmgr_t* mmgr
);

/* get the number of key/value pairs in a map */
ase_size_t ase_map_getsize (
	ase_map_t* map /* a map */
);

ase_size_t ase_map_getcapa (
	ase_map_t* map /* a map */
);

int ase_map_getscale (
	ase_map_t* map   /* a map */,
	ase_map_id_t id  /* ASE_MAP_KEY or ASE_MAP_VAL */
);

/****f* ase.cmn.map/ase_map_setscale
 * NAME
 *  ase_map_setscale - set the scale factor
 *
 * DESCRIPTION 
 *  The ase_map_setscale() function sets the scale factor of the length
 *  of a key and a value. A scale factor determines the actual length of
 *  a key and a value in bytes. A map is created with a scale factor of 1.
 *  The scale factor should be larger than 0 and less than 256.
 *
 * NOTES
 *  It is a bad idea to change the scale factor when a map is not empty.
 *  
 * SYNOPSIS
 */
void ase_map_setscale (
	ase_map_t* map   /* a map */,
	ase_map_id_t id  /* ASE_MAP_KEY or ASE_MAP_VAL */,
	int scale        /* a scale factor */
);
/******/

ase_map_copier_t ase_map_getcopier (
	ase_map_t* map   /* a map */,
	ase_map_id_t id  /* ASE_MAP_KEY or ASE_MAP_VAL */
);

/****f* ase.cmn.map/ase_map_setcopier
 * NAME 
 *  ase_map_setcopier - specify how to clone an element
 *
 * DESCRIPTION
 *  A special copier ASE_MAP_COPIER_INLINE is provided. This copier enables
 *  you to copy the data inline to the internal node. No freeer is invoked
 *  when the node is freeed.
 *
 *  You may set the copier to ASE_NULL to perform no special operation 
 *  when the data pointer is rememebered.
 *
 * SYNOPSIS
 */
void ase_map_setcopier (
	ase_map_t* map           /* a map */, 
	ase_map_id_t id          /* ASE_MAP_KEY or ASE_MAP_VAL */,
	ase_map_copier_t copier  /* an element copier */
);
/******/

ase_map_freeer_t ase_map_getfreeer (
	ase_map_t*   map  /* a map */,
	ase_map_id_t id   /* ASE_MAP_KEY or ASE_MAP_VAL */
);

/****f* ase.cmn.map/ase_map_setfreeer
 * NAME 
 *  ase_map_setfreeer - specify how to destroy an element
 *
 * DESCRIPTION
 *  The freeer is called when a node containing the element is destroyed.
 *
 * SYNOPSIS
 */
void ase_map_setfreeer (
	ase_map_t* map           /* a map */,
	ase_map_id_t id          /* ASE_MAP_KEY or ASE_MAP_VAL */,
	ase_map_freeer_t freeer  /* an element freeer */
);
/******/


ase_map_hasher_t ase_map_gethasher (
	ase_map_t* map
);

void ase_map_sethasher (
	ase_map_t* map,
	ase_map_hasher_t hasher
);

ase_map_comper_t ase_map_getcomper (
	ase_map_t* map
);

void ase_map_setcomper (
	ase_map_t* map,
	ase_map_comper_t comper
);

ase_map_keeper_t ase_map_getkeeper (
	ase_map_t* map
);

void ase_map_setkeeper (
	ase_map_t* map,
	ase_map_keeper_t keeper
);

ase_map_sizer_t ase_map_getsizer (
	ase_map_t* map
);

/* the sizer function is passed a map object and map->capa + 1 */
void ase_map_setsizer (
	ase_map_t* map,
	ase_map_sizer_t sizer
);

int ase_map_put (
	ase_map_t* map,
	void* kptr,
	ase_size_t klen,
	void* vptr,
	ase_size_t vlen,
	ase_map_pair_t** px
);

/****f* ase.cmn.map/ase_map_search
 * NAME
 *  ase_map_search - find a pair with a matching key 
 * 
 * DESCRIPTION
 *  The ase_map_search() function searches a map to find a pair with a 
 *  matching key. It returns the pointer to the pair found. If it fails
 *  to find one, it returns ASE_NULL.
 *
 * RETURN
 *  The ase_map_search() function returns the pointer to the pair with a 
 *  maching key, and ASE_NULL if no match is found.
 * 
 * SYNOPSIS
 */
ase_map_pair_t* ase_map_search (
	ase_map_t* map   /* a map */,
	const void* kptr /* the pointer to a key */,
	ase_size_t klen  /* the size of the key in bytes */
);
/******/

/****f* ase.cmn.map/ase_map_upsert
 * NAME
 *  ase_map_upsert - update an existing pair or inesrt a new pair
 *
 * DESCRIPTION 
 *  The ase_map_upsert() function searches a map for the pair with a matching
 *  key. If one is found, it updates the pair. Otherwise, it inserts a new
 *  pair with a key and a value. It returns the pointer to the pair updated 
 *  or inserted.
 *
 * RETURN 
 *  The ase_map_upsert() function returns a pointer to the updated or inserted
 *  pair on success, and ASE_NULL on failure. 
 *
 * SYNOPSIS
 */
ase_map_pair_t* ase_map_upsert (
	ase_map_t* map   /* a map */,
	void* kptr       /* the pointer to a key */,
	ase_size_t klen  /* the length of the key in bytes */,
	void* vptr       /* the pointer to a value */,
	ase_size_t vlen  /* the length of the value in bytes */
);
/******/

/****f* ase.cmn.map/ase_map_insert 
 * NAME
 *  ase_map_insert - insert a new pair with a key and a value 
 *
 * DESCRIPTION
 *  The ase_map_insert() function inserts a new pair with the key and the value
 *  given. If there exists a pair with the key given, the function returns 
 *  ASE_NULL without channging the value.
 *
 * RETURN 
 *  The ase_map_insert() function returns a pointer to the pair created on 
 *  success, and ASE_NULL on failure. 
 *
 * SYNOPSIS
 */
ase_map_pair_t* ase_map_insert (
	ase_map_t* map   /* a map */,
	void* kptr       /* the pointer to a key */,
	ase_size_t klen  /* the length of the key in bytes */,
	void* vptr       /* the pointer to a value */,
	ase_size_t vlen  /* the length of the value in bytes */
);
/******/

/* update the value of a existing pair with a matching key */
ase_map_pair_t* ase_map_update (
	ase_map_t* map   /* a map */,
	void* kptr       /* the pointer to a key */,
	ase_size_t klen  /* the length of the key in bytes */,
	void* vptr       /* the pointer to a value */,
	ase_size_t vlen  /* the length of the value in bytes */
);

/* delete a pair with a matching key */
int ase_map_delete (
	ase_map_t* map   /* a map */,
	const void* kptr /* the pointer to a key */,
	ase_size_t klen  /* the size of the key in bytes */
);

/* clear a map */
void ase_map_clear (
	ase_map_t* map /* a map */
);

/* traverse a map */
void ase_map_walk (
	ase_map_t* map          /* a map */,
	ase_map_walker_t walker /* the pointer to the function for each pair */,
	void* arg               /* a pointer to user-specific data */
);

/* get the pointer to the first pair in the map. */
ase_map_pair_t* ase_map_getfirstpair (
	ase_map_t* map /* a map */, 
	ase_size_t* buckno
);

/* get the pointer to the next pair in the map. */
ase_map_pair_t* ase_map_getnextpair (
	ase_map_t* map /* a map */,
	ase_map_pair_t* pair,
	ase_size_t* buckno
);

void* ase_map_copyinline (ase_map_t* map, void* dptr, ase_size_t dlen);

#ifdef __cplusplus
}
#endif

#endif
