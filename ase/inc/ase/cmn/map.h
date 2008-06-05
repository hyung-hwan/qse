/*
 * $Id: map.h 182 2008-06-03 08:17:42Z baconevi $
 *
 * {License}
 */

#ifndef _ASE_CMN_MAP_H_
#define _ASE_CMN_MAP_H_

#include <ase/cmn/types.h>
#include <ase/cmn/macros.h>

typedef struct ase_map_t ase_map_t;
typedef struct ase_pair_t ase_pair_t;

struct ase_pair_t
{
	struct
	{
		ase_char_t* ptr;
		ase_size_t len;
	} key;

	void* val;

	/* used internally */
	ase_pair_t* next;
};

struct ase_map_t
{
	/* map owner. passed to freeval and sameval as the first argument */
	void* owner;

	ase_size_t size;
	ase_size_t capa;

	unsigned int factor;
	ase_size_t threshold;

	ase_pair_t** buck;

	void (*freeval) (void* owner,void* val);
	void (*sameval) (void* owner,void* val);

	/* the mmgr pointed at by mmgr should be valid
	 * while the map is alive as the contents are 
	 * not replicated */
	ase_mmgr_t* mmgr;

	/* list of free pairs */
	/*ase_pair_t* fp;*/
};

#define ASE_PAIR_KEYPTR(p) ((p)->key.ptr)
#define ASE_PAIR_KEYLEN(p) ((p)->key.len)
#define ASE_PAIR_VAL(p) ((p)->val)
#define ASE_PAIR_LNK(p) ((p)->next)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a hashed map with a dynamic array bucket and a list of values linked
 *
 * @param owner [in]
 * @param capa [in]
 * @param factor [in]
 * @param freeval [in]
 * @param sameval [in]
 * @param mmgr [in]
 */
ase_map_t* ase_map_open (
	void* owner, ase_size_t capa, unsigned int factor,
	void(*freeval)(void*,void*), void(*sameval)(void*,void*),
	ase_mmgr_t* mmgr);

/**
 * Destroys a hashed map 
 */
void ase_map_close (ase_map_t* map);

void ase_map_clear (ase_map_t* map);

ase_size_t ase_map_getsize (ase_map_t* map);

ase_pair_t* ase_map_get (
	ase_map_t* map, const ase_char_t* keyptr, ase_size_t keylen);

ase_pair_t* ase_map_put (
	ase_map_t* map, const ase_char_t* keyptr, ase_size_t keylen,
	void* val);

int ase_map_putx (
	ase_map_t* map, const ase_char_t* keyptr, ase_size_t keylen,
	void* val, ase_pair_t** px);

ase_pair_t* ase_map_set (
	ase_map_t* map, const ase_char_t* keyptr, ase_size_t keylen, 
	void* val);

ase_pair_t* ase_map_getpair (
	ase_map_t* map, const ase_char_t* keyptr, ase_size_t keylen, 
	void** val);

ase_pair_t* ase_map_setpair (ase_map_t* map, ase_pair_t* pair, void* val);

int ase_map_remove (
	ase_map_t* map, const ase_char_t* keyptr, ase_size_t keylen);

int ase_map_walk (ase_map_t* map, int (*walker)(ase_pair_t*,void*), void* arg);

/**
 * Gets the pointer to the first pair in the map.
 * @param map [in]
 * @param buckno [out]
 */
ase_pair_t* ase_map_getfirstpair (ase_map_t* map, ase_size_t* buckno);

/**
 * Gets the pointer to the next pair in the map.
 * @param map [in]
 * @param pair [in]
 * @param buckno [in out]
 */
ase_pair_t* ase_map_getnextpair (
	ase_map_t* map, ase_pair_t* pair, ase_size_t* buckno);

#ifdef __cplusplus
}
#endif

#endif
