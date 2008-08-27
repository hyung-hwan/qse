/*
 * $Id: map.h 345 2008-08-26 08:50:12Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_CMN_MAP_H_
#define _ASE_CMN_MAP_H_

#include <ase/types.h>
#include <ase/macros.h>

typedef struct ase_map_t ase_map_t;
typedef struct ase_map_pair_t ase_map_pair_t;

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

/* pair visitor - should return ASE_MAP_WALK_STOP or ASE_MAP_WALK_FORWARD */
typedef int (*ase_map_walker_t) (
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

enum ase_map_id_t
{
	ASE_MAP_KEY = 0,
	ASE_MAP_VAL = 1
};

struct ase_map_t
{
        ase_mmgr_t* mmgr;

        ase_map_copier_t copier[2];
        ase_map_freeer_t freeer[2];
	ase_map_hasher_t hasher;
	ase_map_comper_t comper;

	ase_size_t size;
	ase_size_t capa;

	unsigned int factor;
	ase_size_t threshold;

	ase_map_pair_t** buck;

	void (*sameval) (void* owner, void* vptr, ase_size_t vlen);
};

/* values that can be returned by ase_map_walker_t */
enum ase_map_walk_t
{
        ASE_MAP_WALK_STOP = 0,
        ASE_MAP_WALK_FORWARD = 1
};

#define ASE_MAP_COPIER_INLINE ase_map_copyinline

#define ASE_MAP_KPTR(p) ((p)->kptr)
#define ASE_MAP_KLEN(p) ((p)->klen)
#define ASE_MAP_VPTR(p) ((p)->vptr)
#define ASE_MAP_VLEN(p) ((p)->vlen)
#define ASE_MAP_NEXT(p) ((p)->next)

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
        ase_mmgr_t* mmgr,
	ase_size_t ext,
        void (*init) (ase_map_t*)
);

/* destroy a map */
void ase_map_close (
	ase_map_t* map /* a map */
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

/* get the pointer to the pair with a matching key */
ase_map_pair_t* ase_map_get (
	ase_map_t* map /* a map */,
	const void* kptr,
	ase_size_t klen
);


/* insert or update a pair with a matching key */
ase_map_pair_t* ase_map_put (
	ase_map_t* map /* a map */,
	void* kptr /* the pointer to the beginning of a key */,
	ase_size_t klen /* the length of the key in bytes */,
	void* vptr /* the pointer to the beginning of a value */,
	ase_size_t vlen /* the length of the value in bytes */
);

int ase_map_putx (
	ase_map_t* map /* a map */,
	void* kptr,
	ase_size_t klen,
	void* vptr,
	ase_size_t vlen,
	ase_map_pair_t** px
);

ase_map_pair_t* ase_map_set (
	ase_map_t* map /* a map */,
	void* kptr, 
	ase_size_t klen, 
	void* vptr, 
	ase_size_t vlen
);

/* remove a pair with a matching key */
int ase_map_remove (
	ase_map_t* map /* a map */,
	const void* kptr,
	ase_size_t klen
);

/* traverse a map */
void ase_map_walk (
	ase_map_t* map /* a map */,
	ase_map_walker_t walker /* the pointer to the function for each pair */,
	void* arg /* a pointer to user-specific data */
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
