/*
 * $Id: map.h 376 2008-09-24 07:18:50Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_CMN_MAP_H_
#define _ASE_CMN_MAP_H_

#include <ase/types.h>
#include <ase/macros.h>

typedef struct ase_map_t ase_map_t;
typedef struct ase_map_pair_t ase_map_pair_t;
typedef enum ase_map_walk_t ase_map_walk_t;

/* data copier */
typedef void* (*ase_map_copier_t) (
	ase_map_t* map, 
	void* dptr, 
	ase_size_t dlen
);

/* data freeer */
typedef void (*ase_map_freeer_t) (
	ase_map_t* map, 
	void* dptr, 
	ase_size_t dlen
);

/* key hasher */
typedef ase_size_t (*ase_map_hasher_t) (
	ase_map_t* map   /* a map */, 
	const void* kptr /* the pointer to a key */, 
	ase_size_t klen  /* the length of a key in bytes */
);

/* key comparater */
typedef int (*ase_map_comper_t) (
	ase_map_t* map    /* a map */, 
	const void* kptr1 /* the pointer to a key */, 
	ase_size_t klen1  /* the length of a key in bytes */, 
	const void* kptr2 /* the pointer to a key */, 
	ase_size_t klen2  /* the length of a key in bytes */
);

/* 
 * value keeper
 * it is called when a value can be kept without explicit free and copy 
 */
typedef void (*ase_map_keeper_t) (
	ase_map_t* map,
	void* dptr,
	ase_size_t dlen	
);

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

struct ase_map_pair_t
{
	void* kptr;
	ase_size_t klen;

	void* vptr;
	ase_size_t vlen;

	/* used internally */
	ase_map_pair_t* next;
};

struct ase_map_t
{
        ase_mmgr_t* mmgr;

        ase_map_copier_t copier[2];
        ase_map_freeer_t freeer[2];
	ase_map_hasher_t hasher; /* key hasher */
	ase_map_comper_t comper; /* key comparator */
	ase_map_keeper_t keeper; /* value keeper */
	ase_map_sizer_t  sizer;  /* bucket resizer */

	ase_size_t size;
	ase_size_t capa;

	ase_uint_t factor;
	ase_size_t threshold;
	ase_map_pair_t** bucket;
};

enum ase_map_id_t
{
	ASE_MAP_KEY = 0,
	ASE_MAP_VAL = 1
};

/* values that can be returned by ase_map_walker_t */
enum ase_map_walk_t
{
        ASE_MAP_WALK_STOP = 0,
        ASE_MAP_WALK_FORWARD = 1
};

#define ASE_MAP_COPIER_INLINE ase_map_copyinline

#define ASE_MAP_SIZE(m) ((m)->size)
#define ASE_MAP_CAPA(m) ((m)->capa)

#define ASE_MAP_KPTR(p) ((p)->kptr)
#define ASE_MAP_KLEN(p) ((p)->klen)
#define ASE_MAP_VPTR(p) ((p)->vptr)
#define ASE_MAP_VLEN(p) ((p)->vlen)
#define ASE_MAP_NEXT(p) ((p)->next)

/* special macros that you can use to get the number of characters
 * in a key and/or a value if it is a chracter string */
#define ASE_MAP_KCLEN(p) (((p)->klen)/ASE_SIZEOF(ase_char_t))
#define ASE_MAP_VCLEN(p) (((p)->vlen)/ASE_SIZEOF(ase_char_t))

#ifdef __cplusplus
extern "C" {
#endif

/*
 * NAME: create a map
 *
 * DESCRIPTION:
 *  The ase_map_open() function creates a hashed map with a dynamic array 
 *  bucket and a list of values linked.
 */
ase_map_t* ase_map_open (
        ase_mmgr_t* mmgr /* memory manager */,
	ase_size_t ext /* size of extension area in bytes */,
	ase_size_t capa /* initial capacity */,  
	ase_uint_t factor /* load factor */
);

/* destroy a map */
void ase_map_close (
	ase_map_t* map /* a map */
);

ase_map_t* ase_map_init (
	ase_map_t* map,
	ase_mmgr_t* mmgr,
	ase_size_t capa,
	ase_uint_t factor
);

void ase_map_fini (
	ase_map_t* map
);

/* clear a map */
void ase_map_clear (
	ase_map_t* map /* a map */
);

/*
 * NAME: specifies how to clone an element
 *
 * DESCRIPTION:
 *  A special copier ASE_MAP_COPIER_INLINE is provided. This copier enables
 *  you to copy the data inline to the internal node. No freeer is invoked
 *  when the node is freeed.
 *
 *  You may set the copier to ASE_NULL to perform no special operation 
 *  when the data pointer is rememebered.
 */
void ase_map_setcopier (
	ase_map_t* map /* a map */, 
	int id /* ASE_MAP_KEY or ASE_MAP_VAL */,
	ase_map_copier_t copier /* a element copier */
);

ase_map_copier_t ase_map_getcopier (
	ase_map_t* map /* a map */,
	int id /* ASE_MAP_KEY or ASE_MAP_VAL */
);

/*
 * NAME: specifies how to destroy an element
 *
 * DESCRIPTION
 *  The freeer is called when a node containing the element is destroyed.
 */
void ase_map_setfreeer (
	ase_map_t* map /* a map */,
	int id /* ASE_MAP_KEY or ASE_MAP_VAL */,
	ase_map_freeer_t freeer /* a element freeer */
);

ase_map_freeer_t ase_map_getfreeer (
	ase_map_t* map /* a map */,
	int id /* ASE_MAP_KEY or ASE_MAP_VAL */
);

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

int ase_map_put (
	ase_map_t* map,
	void* kptr,
	ase_size_t klen,
	void* vptr,
	ase_size_t vlen,
	ase_map_pair_t** px
);

/* 
 * NAME: find a pair with a matching key 
 * 
 * DESCRIPTION:
 *  The ase_map_search() functions searches a map to find a pair with a 
 *  matching key. It returns the pointer to the pair found. If it fails
 *  to find one, it returns ASE_NULL.
 *
 * RETURNS:
 *  The pointer to the pair with a maching key.
 *  ASE_NULL if no match is found.
 */
ase_map_pair_t* ase_map_search (
	ase_map_t* map   /* a map */,
	const void* kptr /* the pointer to a key */,
	ase_size_t klen  /* the size of the key in bytes */
);

/* 
 * NAME: update an existing pair with a matching or inesrt a new pair
 *
 * DESCRIPTION:
 *   The ase_map_upsert() function searches a map for the pair with a matching
 *   key. If one is found, it updates the pair. Otherwise, it inserts a new
 *   pair with a key and a value. It returns the pointer to the pair updated 
 *   or inserted.
 *
 * RETURNS:
 *  a pointer to the updated or inserted pair.
 *  ASE_NULL on failure. 
 */
ase_map_pair_t* ase_map_upsert (
	ase_map_t* map   /* a map */,
	void* kptr       /* the pointer to a key */,
	ase_size_t klen  /* the length of the key in bytes */,
	void* vptr       /* the pointer to a value */,
	ase_size_t vlen  /* the length of the value in bytes */
);

/* 
 * NAME: insert a new pair with a key and a value 
 *
 * DESCRIPTION:
 *  The ase_map_insert() function inserts a new pair with the key and the value
 *  given. If there exists a pair with the key given, the function returns 
 *  ASE_NULL without channging the value.
 *
 * RETURNS: 
 *  a pointer to the pair successfully created on success.
 *  ASE_NULL on failure. 
 */
ase_map_pair_t* ase_map_insert (
	ase_map_t* map   /* a map */,
	void* kptr       /* the pointer to a key */,
	ase_size_t klen  /* the length of the key in bytes */,
	void* vptr       /* the pointer to a value */,
	ase_size_t vlen  /* the length of the value in bytes */
);

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
