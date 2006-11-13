/*
 * $Id: map.c,v 1.31 2006-11-13 15:08:53 bacon Exp $
 */

#include <ase/awk/awk_i.h>

/* TODO: improve the entire map routines.
         support automatic bucket resizing and remaping, etc. */

static ase_size_t __hash (const ase_char_t* key, ase_size_t key_len);

#define FREE_PAIR(map,pair) \
	do { \
		ASE_AWK_FREE ((map)->awk, (ase_char_t*)(pair)->key); \
		if ((map)->freeval != ASE_NULL) \
			(map)->freeval ((map)->owner, (pair)->val); \
		ASE_AWK_FREE ((map)->awk, pair); \
	} while (0)

ase_awk_map_t* ase_awk_map_open (
	ase_awk_map_t* map, void* owner, ase_size_t capa, 
	void(*freeval)(void*,void*), ase_awk_t* awk)
{
	if (map == ASE_NULL) 
	{
		map = (ase_awk_map_t*) ASE_AWK_MALLOC (
			awk, ase_sizeof(ase_awk_map_t));
		if (map == ASE_NULL) return ASE_NULL;
		map->__dynamic = ase_true;
	}
	else map->__dynamic = ase_false;

	map->awk = awk;
	map->buck = (ase_awk_pair_t**) 
		ASE_AWK_MALLOC (awk, ase_sizeof(ase_awk_pair_t*) * capa);
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
			next = pair->next;
			FREE_PAIR (map, pair);
			map->size--;
			pair = next;
		}

		map->buck[i] = ASE_NULL;
	}

	ASE_AWK_ASSERTX (map->awk, map->size == 0, 
		"the map should not contain any pairs of a key and a value after it has been cleared");
}

ase_awk_pair_t* ase_awk_map_get (
	ase_awk_map_t* map, const ase_char_t* key, ase_size_t key_len)
{
	ase_awk_pair_t* pair;
	ase_size_t hc;

	hc = __hash(key,key_len) % map->capa;
	pair = map->buck[hc];

	while (pair != ASE_NULL) 
	{

		if (ase_awk_strxncmp (
			pair->key, pair->key_len, 
			key, key_len) == 0) return pair;

		pair = pair->next;
	}

	return ASE_NULL;
}

ase_awk_pair_t* ase_awk_map_put (
	ase_awk_map_t* map, ase_char_t* key, ase_size_t key_len, void* val)
{
	int n;
	ase_awk_pair_t* px;

	n = ase_awk_map_putx (map, key, key_len, val, &px);
	if (n < 0) return ASE_NULL;
	return px;
}

int ase_awk_map_putx (
	ase_awk_map_t* map, ase_char_t* key, ase_size_t key_len, 
	void* val, ase_awk_pair_t** px)
{
	ase_awk_pair_t* pair;
	ase_size_t hc;

	hc = __hash(key,key_len) % map->capa;
	pair = map->buck[hc];

	while (pair != ASE_NULL) 
	{
		if (ase_awk_strxncmp (
			pair->key, pair->key_len, key, key_len) == 0) 
		{
			if (px != ASE_NULL)
				*px = ase_awk_map_setpair (map, pair, val);
			else
				ase_awk_map_setpair (map, pair, val);

			return 0; /* value changed for the existing key */
		}
		pair = pair->next;
	}

	pair = (ase_awk_pair_t*) ASE_AWK_MALLOC (
		map->awk, ase_sizeof(ase_awk_pair_t));
	if (pair == ASE_NULL) return -1; /* error */

	/*pair->key = key;*/ 

	/* duplicate the key if it is new */
	pair->key = ase_awk_strxdup (map->awk, key, key_len);
	if (pair->key == ASE_NULL)
	{
		ASE_AWK_FREE (map->awk, pair);
		return -1; /* error */
	}

	pair->key_len = key_len;
	pair->val = val;
	pair->next = map->buck[hc];
	map->buck[hc] = pair;
	map->size++;

	if (px != ASE_NULL) *px = pair;
	return 1; /* new key added */
}

ase_awk_pair_t* ase_awk_map_set (
	ase_awk_map_t* map, ase_char_t* key, ase_size_t key_len, void* val)
{
	ase_awk_pair_t* pair;
	ase_size_t hc;

	hc = __hash(key,key_len) % map->capa;
	pair = map->buck[hc];

	while (pair != ASE_NULL) 
	{
		if (ase_awk_strxncmp (
			pair->key, pair->key_len, key, key_len) == 0) 
		{
			return ase_awk_map_setpair (map, pair, val);
		}
		pair = pair->next;
	}

	return ASE_NULL;
}

ase_awk_pair_t* ase_awk_map_getpair (
	ase_awk_map_t* map, const ase_char_t* key, ase_size_t key_len, void** val)
{
	ase_awk_pair_t* pair;

	pair = ase_awk_map_get (map, key, key_len);
	if (pair == ASE_NULL) return ASE_NULL; 
	*val = pair->val;

	return pair;
}

ase_awk_pair_t* ase_awk_map_setpair (
	ase_awk_map_t* map, ase_awk_pair_t* pair, void* val)
{
	/* use this function with care */
	if (pair->val != val) 
	{
		if (map->freeval != ASE_NULL) 
		{
			map->freeval (map->owner, pair->val);
		}
		pair->val = val;
	}

	return pair;
}

int ase_awk_map_remove (ase_awk_map_t* map, ase_char_t* key, ase_size_t key_len)
{
	ase_awk_pair_t* pair, * prev;
	ase_size_t hc;

	hc = __hash(key,key_len) % map->capa;
	pair = map->buck[hc];
	prev = ASE_NULL;

	while (pair != ASE_NULL) 
	{
		if (ase_awk_strxncmp (
			pair->key, pair->key_len, key, key_len) == 0) 
		{
			if (prev == ASE_NULL) 
				map->buck[hc] = pair->next;
			else prev->next = pair->next;

			FREE_PAIR (map, pair);
			map->size--;

			return 0;
		}

		prev = pair;
		pair = pair->next;
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
			next = pair->next;
			if (walker(pair,arg) == -1) return -1;
			pair = next;
		}
	}

	return 0;
}

static ase_size_t __hash (const ase_char_t* key, ase_size_t key_len)
{
	ase_size_t n = 0, i;
	const ase_char_t* end = key + key_len;

	while (key < end)
	{
		ase_byte_t* bp = (ase_byte_t*)key;
		for (i = 0; i < ase_sizeof(*key); i++) n = n * 31 + *bp++;
		key++;
	}	

	return n;
}
