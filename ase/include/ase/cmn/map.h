/*
 * $Id: map.h 344 2008-08-22 15:23:09Z baconevi $
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

/* pair visitor */
typedef int (*ase_map_walker_t) (
	ase_map_t* map, 
	ase_map_pair_t* pair, 
	void* arg
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

enum  ase_map_id_t
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

/*
	void (*freeval) (void* owner,void* val);
*/
	void (*sameval) (void* owner, void* vptr, ase_size_t vlen);
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
 * Creates a hashed map with a dynamic array bucket and a list of values linked
 */
/*
ase_map_t* ase_map_open (
	void* owner, ase_size_t capa, unsigned int factor,
	void(*freeval)(void*,void*), void(*sameval)(void*,void*),
	ase_mmgr_t* mmgr);
*/
ase_map_t* ase_map_open (
        ase_mmgr_t* mmgr,
	ase_size_t ext,
        void (*init) (ase_map_t*)
);

/* destroy a map */
void ase_map_close (ase_map_t* map);

/* clear a map */
void ase_map_clear (ase_map_t* map);

/* get the number of key/value pairs in a map
ase_size_t ase_map_getsize (ase_map_t* map);

ase_map_pair_t* ase_map_get (
	ase_map_t* map,
	const void* kptr,
	ase_size_t klen
);

ase_map_pair_t* ase_map_put (
	ase_map_t* map,
	const void* kptr,
	ase_size_t klen,
	void* vptr,
	ase_size_t vlen
);

int ase_map_putx (
	ase_map_t* map,
	void* kptr,
	ase_size_t klen,
	void* vptr,
	ase_size_t vlen,
	ase_map_pair_t** px
);

ase_map_pair_t* ase_map_set (
	ase_map_t* map, 
	void* kptr, 
	ase_size_t klen, 
	void* vptr, 
	ase_size_t vlen
);

int ase_map_remove (
	ase_map_t* map, const void* kptr, ase_size_t klen);

/* traverse a map */
int ase_map_walk (ase_map_t* map, ase_map_walker_t walker, void* arg);

/* get the pointer to the first pair in the map. */
ase_map_pair_t* ase_map_getfirstpair (ase_map_t* map, ase_size_t* buckno);

/* get the pointer to the next pair in the map. */
ase_map_pair_t* ase_map_getnextpair (
	ase_map_t* map, ase_map_pair_t* pair, ase_size_t* buckno);

void* ase_map_copyinline (ase_map_t* map, void* dptr, ase_size_t dlen);

#ifdef __cplusplus
}
#endif

#endif
