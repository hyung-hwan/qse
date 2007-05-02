/*
 * $Id: map.c,v 1.3 2007/04/30 05:47:33 bacon Exp $
 *
 * {License}
 */

#include <ase/awk/awk_i.h>

/* TODO: improve the entire map routines.
         support automatic bucket resizing and remaping, etc. */

static ase_size_t hashkey (const ase_char_t* keyptr, ase_size_t keylen);

#define FREE_PAIR(map,pair) \
	do { \
		ASE_AWK_FREE ((map)->awk, (ase_char_t*)PAIR_KEYPTR(pair)); \
		if ((map)->freeval != ASE_NULL) \
			(map)->freeval ((map)->owner, PAIR_VAL(pair)); \
		ASE_AWK_FREE ((map)->awk, pair); \
	} while (0)

ase_awk_map_t* ase_awk_map_open (
	ase_awk_map_t* map, void* owner, ase_size_t capa, 
	void(*freeval)(void*,void*), ase_awk_t* awk)
{
	ASE_ASSERTX (capa > 0, "the initial capacity should be greater than 0");

	if (map == ASE_NULL) 
	{
		map = (ase_awk_map_t*) ASE_AWK_MALLOC (
			awk, ASE_SIZEOF(ase_awk_map_t));
		if (map == ASE_NULL) return ASE_NULL;
		map->__dynamic = ase_true;
	}
	else map->__dynamic = ase_false;

	map->awk = awk;
	map->buck = (ase_awk_pair_t**) 
		ASE_AWK_MALLOC (awk, ASE_SIZEOF(ase_awk_pair_t*) * capa);
	if (map->buck == ASE_NULL) 
	{
		if (map->__dynamic) ASE_AWK_FREE (awk, map);
		return ASE_NULL;	
	}

	map->owner = owner;
	map->capa = capa;
	map->size = 0;
	map->freeval = freeval;
	while (capa > 0) map->buck[--capa] = ASE_NULL;

	return map;
}

void ase_awk_map_close (ase_awk_map_t* map)
{
	ase_awk_map_clear (map);
	ASE_AWK_FREE (map->awk, map->buck);
	if (map->__dynamic) ASE_AWK_FREE (map->awk, map);
}

void ase_awk_map_clear (ase_awk_map_t* map)
{
	ase_size_t i;
	ase_awk_pair_t* pair, * next;

	for (i = 0; i < map->capa; i++) 
	{
		pair = map->buck[i];

		while (pair != ASE_NULL) 
		{
			next = PAIR_LNK(pair);
			FREE_PAIR (map, pair);
			map->size--;
			pair = next;
		}

		map->buck[i] = ASE_NULL;
	}
}

ase_size_t ase_awk_map_getsize (ase_awk_map_t* map)
{
	return map->size;
}

ase_awk_pair_t* ase_awk_map_get (
	ase_awk_map_t* map, const ase_char_t* keyptr, ase_size_t keylen)
{
	ase_awk_pair_t* pair;
	ase_size_t hc;

	hc = hashkey(keyptr,keylen) % map->capa;
	pair = map->buck[hc];

	while (pair != ASE_NULL) 
	{

		if (ase_strxncmp (
			PAIR_KEYPTR(pair), PAIR_KEYLEN(pair), 
			keyptr, keylen) == 0) return pair;

		pair = PAIR_LNK(pair);
	}

	return ASE_NULL;
}

ase_awk_pair_t* ase_awk_map_put (
	ase_awk_map_t* map, ase_char_t* keyptr, ase_size_t keylen, void* val)
{
	int n;
	ase_awk_pair_t* px;

	n = ase_awk_map_putx (map, keyptr, keylen, val, &px);
	if (n < 0) return ASE_NULL;
	return px;
}

int ase_awk_map_putx (
	ase_awk_map_t* map, ase_char_t* keyptr, ase_size_t keylen, 
	void* val, ase_awk_pair_t** px)
{
	ase_awk_pair_t* pair;
	ase_size_t hc;

	hc = hashkey(keyptr,keylen) % map->capa;
	pair = map->buck[hc];

	while (pair != ASE_NULL) 
	{
		if (ase_strxncmp (
			PAIR_KEYPTR(pair), PAIR_KEYLEN(pair), 
			keyptr, keylen) == 0) 
		{
			if (px != ASE_NULL)
				*px = ase_awk_map_setpair (map, pair, val);
			else
				ase_awk_map_setpair (map, pair, val);

			return 0; /* value changed for the existing key */
		}
		pair = PAIR_LNK(pair);
	}

	pair = (ase_awk_pair_t*) ASE_AWK_MALLOC (
		map->awk, ASE_SIZEOF(ase_awk_pair_t));
	if (pair == ASE_NULL) return -1; /* error */

	/* duplicate the key if it is new */
	PAIR_KEYPTR(pair) = ase_strxdup (
		keyptr, keylen, &map->awk->prmfns.mmgr);
	if (PAIR_KEYPTR(pair) == ASE_NULL)
	{
		ASE_AWK_FREE (map->awk, pair);
		return -1; /* error */
	}

	PAIR_KEYLEN(pair) = keylen;
	PAIR_VAL(pair) = val;
	PAIR_LNK(pair) = map->buck[hc];
	map->buck[hc] = pair;
	map->size++;

	if (px != ASE_NULL) *px = pair;
	return 1; /* new key added */
}

ase_awk_pair_t* ase_awk_map_set (
	ase_awk_map_t* map, ase_char_t* keyptr, ase_size_t keylen, void* val)
{
	ase_awk_pair_t* pair;
	ase_size_t hc;

	hc = hashkey(keyptr,keylen) % map->capa;
	pair = map->buck[hc];

	while (pair != ASE_NULL) 
	{
		if (ase_strxncmp (
			PAIR_KEYPTR(pair), PAIR_KEYLEN(pair), 
			keyptr, keylen) == 0) 
		{
			return ase_awk_map_setpair (map, pair, val);
		}
		pair = PAIR_LNK(pair);
	}

	return ASE_NULL;
}

ase_awk_pair_t* ase_awk_map_getpair (
	ase_awk_map_t* map, const ase_char_t* keyptr, ase_size_t keylen, 
	void** val)
{
	ase_awk_pair_t* pair;

	pair = ase_awk_map_get (map, keyptr, keylen);
	if (pair == ASE_NULL) return ASE_NULL; 
	*val = PAIR_VAL(pair);

	return pair;
}

ase_awk_pair_t* ase_awk_map_setpair (
	ase_awk_map_t* map, ase_awk_pair_t* pair, void* val)
{
	/* use this function with care */
	if (PAIR_VAL(pair) != val) 
	{
		if (map->freeval != ASE_NULL) 
		{
			map->freeval (map->owner, PAIR_VAL(pair));
		}
		PAIR_VAL(pair) = val;
	}

	return pair;
}

int ase_awk_map_remove (
	ase_awk_map_t* map, ase_char_t* keyptr, ase_size_t keylen)
{
	ase_awk_pair_t* pair, * prev;
	ase_size_t hc;

	hc = hashkey(keyptr,keylen) % map->capa;
	pair = map->buck[hc];
	prev = ASE_NULL;

	while (pair != ASE_NULL) 
	{
		if (ase_strxncmp (
			PAIR_KEYPTR(pair), PAIR_KEYLEN(pair), 
			keyptr, keylen) == 0) 
		{
			if (prev == ASE_NULL) 
				map->buck[hc] = PAIR_LNK(pair);
			else prev->next = PAIR_LNK(pair);

			FREE_PAIR (map, pair);
			map->size--;

			return 0;
		}

		prev = pair;
		pair = PAIR_LNK(pair);
	}

	return -1;
}

int ase_awk_map_walk (ase_awk_map_t* map, 
	int (*walker) (ase_awk_pair_t*,void*), void* arg)
{
	ase_size_t i;
	ase_awk_pair_t* pair, * next;

	for (i = 0; i < map->capa; i++) 
	{
		pair = map->buck[i];

		while (pair != ASE_NULL) 
		{
			next = PAIR_LNK(pair);
			if (walker(pair,arg) == -1) return -1;
			pair = next;
		}
	}

	return 0;
}

static ase_size_t hashkey (const ase_char_t* keyptr, ase_size_t keylen)
{
	ase_size_t n = 0, i;
	const ase_char_t* end = keyptr + keylen;

	while (keyptr < end)
	{
		ase_byte_t* bp = (ase_byte_t*)keyptr;
		for (i = 0; i < ASE_SIZEOF(*keyptr); i++) n = n * 31 + *bp++;
		keyptr++;
	}	

	return n;
}
