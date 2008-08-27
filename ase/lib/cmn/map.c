/*
 * $Id: map.c 347 2008-08-26 11:04:16Z baconevi $
 *
 * {License}
 */

#include <ase/cmn/map.h>
#include "mem.h"

#define map_t    ase_map_t
#define pair_t   ase_map_pair_t
#define copier_t ase_map_copier_t
#define freeer_t ase_map_freeer_t
#define hasher_t ase_map_hasher_t
#define comper_t ase_map_comper_t
#define walker_t ase_map_walker_t

#define KPTR(p)  ASE_MAP_KPTR(p)
#define KLEN(p)  ASE_MAP_KLEN(p)
#define VPTR(p)  ASE_MAP_VPTR(p)
#define VLEN(p)  ASE_MAP_VLEN(p)
#define NEXT(p)  ASE_MAP_NEXT(p)

#define size_t   ase_size_t
#define byte_t   ase_byte_t
#define mmgr_t   ase_mmgr_t

static int reorganize (map_t* map);

static size_t hash_key (map_t* map, const void* kptr, size_t klen)
{
	size_t n = 0;
	const byte_t* p = (const byte_t*)kptr;
	const byte_t* bound = p + klen;

	while (p < bound)
	{
		n = n * 31 + *p++;
		p++;
	}	

	return n;
}

static int comp_key (map_t* map, 
	const void* kptr1, size_t klen1, 
	const void* kptr2, size_t klen2)
{
	if (klen1 == klen2) return ASE_MEMCMP (kptr1, kptr2, klen1);
	/* it just returns 1 to indicate that they are different. */
	return 1;
}

static pair_t* alloc_pair (map_t* map, 
	void* kptr, size_t klen, void* vptr, size_t vlen)
{
	pair_t* n;
	copier_t kcop = map->copier[ASE_MAP_KEY];
	copier_t vcop = map->copier[ASE_MAP_VAL];

	size_t as = ASE_SIZEOF(pair_t);

	if (kcop == ASE_MAP_COPIER_INLINE) as += klen;
	if (vcop == ASE_MAP_COPIER_INLINE) as += vlen;

	n = ASE_MMGR_ALLOC (map->mmgr, as);
	if (n == ASE_NULL) return ASE_NULL;

	NEXT(n) = ASE_NULL;

	KLEN(n) = klen;
	if (kcop == ASE_NULL)
	{
		KPTR(n) = kptr;
	}
	else if (kcop == ASE_MAP_COPIER_INLINE)
	{
		KPTR(n) = n + 1;
		ASE_MEMCPY (KPTR(n), kptr, klen);
	}
	else 
	{
		n->kptr = kcop (map, kptr, klen);
		if (n->kptr == ASE_NULL)
		{
			ASE_MMGR_FREE (map->mmgr, n);		
			return ASE_NULL;
		}
	}

	VLEN(n) = vlen;
	if (vcop == ASE_NULL) 
	{
		VPTR(n) = vptr;
	}
	else if (vcop == ASE_MAP_COPIER_INLINE)
	{
		VPTR(n) = n + 1;
		ASE_MEMCPY (VPTR(n), vptr, vlen);
	}
	else 
	{
		n->vptr = vcop (map, vptr, vlen);
		if (n->vptr != ASE_NULL)
		{
			if (map->freeer[ASE_MAP_KEY] != ASE_NULL)
				map->freeer[ASE_MAP_KEY] (map, n->kptr, n->klen);
			ASE_MMGR_FREE (map->mmgr, n);		
			return ASE_NULL;
		}
	}

	return n;
}

static void free_pair (map_t* map, pair_t* pair)
{
	if (map->freeer[ASE_MAP_KEY] != ASE_NULL) 
		map->freeer[ASE_MAP_KEY] (map, KPTR(pair), KLEN(pair));
	if (map->freeer[ASE_MAP_VAL] != ASE_NULL)
		map->freeer[ASE_MAP_VAL] (map, VPTR(pair), VLEN(pair));
	ASE_MMGR_FREE (map->mmgr, pair);
}

static pair_t* change_pair_val (
	map_t* map, pair_t* pair, void* vptr, size_t vlen)
{
	if (VPTR(pair) == vptr) 
	{
		/* if the old value and the new value are the same,
		 * it just calls the handler for this condition. 
		 * No value replacement occurs. */
		if (map->sameval != ASE_NULL)
		{
			map->sameval (map, vptr, vlen);
		}
	}
	else
	{
		copier_t vcop = map->copier[ASE_MAP_VAL];
		void* ovptr = VPTR(pair);
		size_t ovlen = VLEN(pair);

		/* place the new value according to the copier */
		if (vcop == ASE_NULL)
		{
			VPTR(pair) = vptr;
			VLEN(pair) = vlen;
		}
		else if (vcop == ASE_MAP_COPIER_INLINE)
		{
			if (ovlen == vlen)
			{
				ASE_MEMCPY (VPTR(pair), vptr, vlen);
			}
			else
			{
				/* need to reconstruct the pair */
				pair_t* p = alloc_pair (map, 
					KPTR(pair), KLEN(pair),
					vptr, vlen);
				if (p == ASE_NULL) return ASE_NULL;
				free_pair (map, pair);
				return p;
			}
		}
		else 
		{
			void* nvptr = vcop (map, vptr, vlen);
			if (nvptr == ASE_NULL) return ASE_NULL;
			VPTR(pair) = nvptr;
			VLEN(pair) = vlen;
		}

		/* free up the old value */
		if (map->freeer[ASE_MAP_VAL] != ASE_NULL) 
		{
			map->freeer[ASE_MAP_VAL] (map, ovptr, ovlen);
		}
	}


	return pair;
}

map_t* ase_map_open (mmgr_t* mmgr, size_t ext, void (*init) (map_t*))
{
	map_t* map;

	if (mmgr == ASE_NULL) 
	{
		mmgr = ASE_MMGR_GETDFL();

		ASE_ASSERTX (mmgr != ASE_NULL,
			"Set the memory manager with ASE_MMGR_SETDFL()");

		if (mmgr == ASE_NULL) return ASE_NULL;
	}

	map = ASE_MMGR_ALLOC (mmgr, ASE_SIZEOF(map_t) + ext);
	if (map == ASE_NULL) return ASE_NULL;

	ASE_MEMSET (map, 0, ASE_SIZEOF(map_t) + ext);
	map->mmgr = mmgr;

	map->size = 0;
	map->capa = 0;
	map->factor = 70;

	map->hasher = hash_key;
	map->comper = comp_key;
	if (init) init (map);

	return map;
}

void ase_map_close (map_t* map)
{
	ase_map_clear (map);
	ASE_MMGR_FREE (map->mmgr, map->buck);
	ASE_MMGR_FREE (map->mmgr, map);
}

void ase_map_clear (map_t* map)
{
	size_t i;
	pair_t* pair, * next;

	for (i = 0; i < map->capa; i++) 
	{
		pair = map->buck[i];

		while (pair != ASE_NULL) 
		{
			next = NEXT(pair);
			free_pair (map, pair);
			map->size--;
			pair = next;
		}

		map->buck[i] = ASE_NULL;
	}
}

copier_t ase_map_getcopier (map_t* map, int id)
{
	ASE_ASSERTX (id == ASE_MAP_KEY || id == ASE_MAP_VAL,
		"The ID should be either ASE_MAP_KEY or ASE_MAP_VAL");
	return map->copier[id];
}

void ase_map_setcopier (map_t* map, int id, copier_t copier)
{
	ASE_ASSERTX (id == ASE_MAP_KEY || id == ASE_MAP_VAL,
		"The ID should be either ASE_MAP_KEY or ASE_MAP_VAL");
	map->copier[id] = copier;
}

freeer_t ase_map_getfreeer (map_t* map, int id)
{
	ASE_ASSERTX (id == ASE_MAP_KEY || id == ASE_MAP_VAL,
		"The ID should be either ASE_MAP_KEY or ASE_MAP_VAL");
	return map->freeer[id];
}

void ase_map_setfreeer (map_t* map, int id, freeer_t freeer)
{
	ASE_ASSERTX (id == ASE_MAP_KEY || id == ASE_MAP_VAL,
		"The ID should be either ASE_MAP_KEY or ASE_MAP_VAL");
	map->freeer[id] = freeer;
}

hasher_t ase_map_gethasher (map_t* map)
{
	return map->hasher;
}

void ase_map_sethasher (map_t* map, hasher_t hasher)
{
	map->hasher = hasher;	
}

comper_t ase_map_getcomper (map_t* map)
{
	return map->comper;
}

void ase_map_setcomper (map_t* map, comper_t comper)
{
	map->comper = comper;
}

void* ase_map_getextension (map_t* map)
{
	return map + 1;
}

mmgr_t* ase_map_getmmgr (map_t* map)
{
	return map->mmgr;
}

void ase_map_setmmgr (map_t* map, mmgr_t* mmgr)
{
	map->mmgr = mmgr;
}

size_t ase_map_getsize (map_t* map)
{
	return map->size;
}

pair_t* ase_map_get (
	map_t* map, const void* kptr, size_t klen)
{
	pair_t* pair;
	size_t hc;

	hc = map->hasher(map,kptr,klen) % map->capa;
	pair = map->buck[hc];

	while (pair != ASE_NULL) 
	{
		if (map->comper (map, KPTR(pair), KLEN(pair), kptr, klen) == 0)
		{
			return pair;
		}

		pair = NEXT(pair);
	}

	return ASE_NULL;
}

pair_t* ase_map_put (map_t* map, 
	void* kptr, size_t klen, void* vptr, size_t vlen)
{
	int n;
	pair_t* px;

	n = ase_map_putx (map, kptr, klen, vptr, vlen, &px);
	if (n < 0) return ASE_NULL;
	return px;
}

int ase_map_putx (
	map_t* map, void* kptr, size_t klen, 
	void* vptr, size_t vlen, pair_t** px)
{
	pair_t* pair, * p, * prev, * next;
	size_t hc;

	hc = map->hasher(map,kptr,klen) % map->capa;
	pair = map->buck[hc];
	prev = ASE_NULL;

	while (pair != ASE_NULL) 
	{
		next = NEXT(pair);

		if (map->comper (map, KPTR(pair), KLEN(pair), kptr, klen) == 0) 
		{
			p = change_pair_val (map, pair, vptr, vlen);
			if (p == ASE_NULL) return -1; /* change error */
			if (p != pair) 
			{
				/* the pair has been reallocated. relink it */
				if (prev == ASE_NULL) map->buck[hc] = p;
				else NEXT(prev) = p;
				NEXT(p) = next;
			}

			if (px != ASE_NULL) *px = p;
			return 0; /* value changed for the existing key */
		}

		prev = pair;
		pair = next;
	}

	if (map->threshold > 0 && map->size >= map->threshold)
	{
		if (reorganize(map) == 0) /* ignore the error */
		{
			hc = map->hasher(map,kptr,klen) % map->capa;
		}
	}

	ASE_ASSERT (pair == ASE_NULL);

#if 0
	pair = (pair_t*) ASE_MMGR_ALLOC (map->mmgr, 
		ASE_SIZEOF(pair_t) + ((klen+1)*ASE_SIZEOF(*kptr)));
	if (pair == ASE_NULL) return -1; /* error */

	/* duplicate the key if it is new */
	KPTR(pair) = (void*)(pair + 1);
	ase_strncpy (KPTR(pair), kptr, klen);

	KLEN(pair) = klen;
	VPTR(pair) = vptr;
	VLEN(pair) = vlen;
#endif
	pair = alloc_pair (map, kptr, klen, vptr, vlen);
	if (pair == ASE_NULL) return -1; /* error */

	NEXT(pair) = map->buck[hc];
	map->buck[hc] = pair;
	map->size++;

	if (px != ASE_NULL) *px = pair;
	return 1; /* new key added */
}


pair_t* ase_map_insert (map_t* map, void* kptr, size_t klen, void* vptr, size_t vlen)
{
	pair_t* pair;
	size_t hc;

	hc = map->hasher(map,kptr,klen) % map->capa;
	pair = map->buck[hc];

	while (pair != ASE_NULL) 
	{
		if (map->comper (map, KPTR(pair), KLEN(pair), kptr, klen) == 0) 
		{
			return ASE_NULL;
		}

		pair = NEXT(pair);
	}

	if (map->threshold > 0 && map->size >= map->threshold)
	{
		if (reorganize(map) == 0) /* ignore the error */
		{
			hc = map->hasher(map,kptr,klen) % map->capa;
		}
	}

	ASE_ASSERT (pair == ASE_NULL);

	pair = alloc_pair (map, kptr, klen, vptr, vlen);
	if (pair == ASE_NULL) return ASE_NULL;

	NEXT(pair) = map->buck[hc];
	map->buck[hc] = pair;
	map->size++;

	return pair;
}

pair_t* ase_map_update (map_t* map, void* kptr, size_t klen, void* vptr, size_t vlen)
{
	pair_t* pair, * p, * prev, * next;
	size_t hc;

	hc = map->hasher(map,kptr,klen) % map->capa;
	pair = map->buck[hc];
	prev = ASE_NULL;

	while (pair != ASE_NULL) 
	{
		next = NEXT(pair);

		if (map->comper (map, KPTR(pair), KLEN(pair), kptr, klen) == 0) 
		{
			p = change_pair_val (map, pair, vptr, vlen);

			if (p == ASE_NULL) return ASE_NULL; /* change error */
			if (p != pair) 
			{
				/* the pair has been reallocated. relink it */
				if (prev == ASE_NULL) map->buck[hc] = p;
				else NEXT(prev) = p;
				NEXT(p) = next;
			}

			return 0; /* value changed for the existing key */
		}

		prev = pair;
		pair = next;
	}

	return ASE_NULL;
}

int ase_map_delete (map_t* map, const void* kptr, size_t klen)
{
	pair_t* pair, * prev;
	size_t hc;

	hc = map->hasher(map,kptr,klen) % map->capa;
	pair = map->buck[hc];
	prev = ASE_NULL;

	while (pair != ASE_NULL) 
	{
		if (map->comper (map, KPTR(pair), KLEN(pair), kptr, klen) == 0) 
		{
			if (prev == ASE_NULL) 
				map->buck[hc] = NEXT(pair);
			else NEXT(prev) = NEXT(pair);

			free_pair (map, pair);
			map->size--;

			return 0;
		}

		prev = pair;
		pair = NEXT(pair);
	}

	return -1;
}

void ase_map_walk (map_t* map, walker_t walker, void* arg)
{
	size_t i;
	pair_t* pair, * next;

	for (i = 0; i < map->capa; i++) 
	{
		pair = map->buck[i];

		while (pair != ASE_NULL) 
		{
			next = NEXT(pair);
			if (walker(map, pair, arg) == ASE_MAP_WALK_STOP) return;
			pair = next;
		}
	}
}

pair_t* ase_map_getfirstpair (map_t* map, size_t* buckno)
{
	size_t i;
	pair_t* pair;

	for (i = 0; i < map->capa; i++)
	{
		pair = map->buck[i];
		if (pair != ASE_NULL) 
		{
			*buckno = i;
			return pair;
		}
	}

	return ASE_NULL;
}

pair_t* ase_map_getnextpair (
	map_t* map, pair_t* pair, size_t* buckno)
{
	size_t i;
	pair_t* next;

	next = NEXT(pair);
	if (next != ASE_NULL) 
	{
		/* no change in bucket number */
		return next;
	}

	for (i = (*buckno)+1; i < map->capa; i++)
	{
		pair = map->buck[i];
		if (pair != ASE_NULL) 
		{
			*buckno = i;
			return pair;
		}
	}

	return ASE_NULL;
}

static int reorganize (map_t* map)
{
	size_t i, hc, new_capa;
	pair_t** new_buck;

	/* the bucket is doubled until it grows up to 65536 slots.
	 * once it has reached it, it grows by 65536 slots */
	new_capa = (map->capa >= 65536)? (map->capa + 65536): (map->capa << 1);

	new_buck = (pair_t**) ASE_MMGR_ALLOC (
		map->mmgr, ASE_SIZEOF(pair_t*) * new_capa);
	if (new_buck == ASE_NULL) 
	{
		/* once reogranization fails, reogranization is disabled */
		map->threshold = 0;
		return -1;
	}

	for (i = 0; i < new_capa; i++) new_buck[i] = ASE_NULL;

	for (i = 0; i < map->capa; i++)
	{
		pair_t* pair = map->buck[i];

		while (pair != ASE_NULL) 
		{
			pair_t* next = NEXT(pair);

			hc = map->hasher (map,
				KPTR(pair),
				KLEN(pair)) % new_capa;

			NEXT(pair) = new_buck[hc];
			new_buck[hc] = pair;

			pair = next;
		}
	}

	ASE_MMGR_FREE (map->mmgr, map->buck);
	map->buck = new_buck;
	map->capa = new_capa;
	map->threshold = ((size_t)map->factor) * map->capa / 100;

	return 0;
}

void* ase_map_copyinline (map_t* map, void* dptr, size_t dlen)
{
        /* this is a dummy copier */
        return ASE_NULL;
}
