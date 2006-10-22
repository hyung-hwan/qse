/*
 * $Id: map.c,v 1.27 2006-10-22 11:34:53 bacon Exp $
 */

#include <sse/awk/awk_i.h>

/* TODO: improve the entire map routines.
         support automatic bucket resizing and remaping, etc. */

static sse_size_t __hash (const sse_char_t* key, sse_size_t key_len);

#define FREE_PAIR(map,pair) \
	do { \
		SSE_AWK_FREE ((map)->awk, (sse_char_t*)(pair)->key); \
		if ((map)->freeval != SSE_NULL) \
			(map)->freeval ((map)->owner, (pair)->val); \
		SSE_AWK_FREE ((map)->awk, pair); \
	} while (0)

sse_awk_map_t* sse_awk_map_open (
	sse_awk_map_t* map, void* owner, sse_size_t capa, 
	void(*freeval)(void*,void*), sse_awk_t* awk)
{
	if (map == SSE_NULL) 
	{
		map = (sse_awk_map_t*) SSE_AWK_MALLOC (
			awk, sse_sizeof(sse_awk_map_t));
		if (map == SSE_NULL) return SSE_NULL;
		map->__dynamic = sse_true;
	}
	else map->__dynamic = sse_false;

	map->awk = awk;
	map->buck = (sse_awk_pair_t**) 
		SSE_AWK_MALLOC (awk, sse_sizeof(sse_awk_pair_t*) * capa);
	if (map->buck == SSE_NULL) 
	{
		if (map->__dynamic) SSE_AWK_FREE (awk, map);
		return SSE_NULL;	
	}

	map->owner = owner;
	map->capa = capa;
	map->size = 0;
	map->freeval = freeval;
	while (capa > 0) map->buck[--capa] = SSE_NULL;

	return map;
}

void sse_awk_map_close (sse_awk_map_t* map)
{
	sse_awk_map_clear (map);
	SSE_AWK_FREE (map->awk, map->buck);
	if (map->__dynamic) SSE_AWK_FREE (map->awk, map);
}

void sse_awk_map_clear (sse_awk_map_t* map)
{
	sse_size_t i;
	sse_awk_pair_t* pair, * next;

	for (i = 0; i < map->capa; i++) 
	{
		pair = map->buck[i];

		while (pair != SSE_NULL) 
		{
			next = pair->next;
			FREE_PAIR (map, pair);
			map->size--;
			pair = next;
		}

		map->buck[i] = SSE_NULL;
	}

	sse_awk_assert (map->awk, map->size == 0);
}

sse_awk_pair_t* sse_awk_map_get (
	sse_awk_map_t* map, const sse_char_t* key, sse_size_t key_len)
{
	sse_awk_pair_t* pair;
	sse_size_t hc;

	hc = __hash(key,key_len) % map->capa;
	pair = map->buck[hc];

	while (pair != SSE_NULL) 
	{

		if (sse_awk_strxncmp (
			pair->key, pair->key_len, 
			key, key_len) == 0) return pair;

		pair = pair->next;
	}

	return SSE_NULL;
}

sse_awk_pair_t* sse_awk_map_put (
	sse_awk_map_t* map, sse_char_t* key, sse_size_t key_len, void* val)
{
	int n;
	sse_awk_pair_t* px;

	n = sse_awk_map_putx (map, key, key_len, val, &px);
	if (n < 0) return SSE_NULL;
	return px;
}

int sse_awk_map_putx (
	sse_awk_map_t* map, sse_char_t* key, sse_size_t key_len, 
	void* val, sse_awk_pair_t** px)
{
	sse_awk_pair_t* pair;
	sse_size_t hc;

	hc = __hash(key,key_len) % map->capa;
	pair = map->buck[hc];

	while (pair != SSE_NULL) 
	{
		if (sse_awk_strxncmp (
			pair->key, pair->key_len, key, key_len) == 0) 
		{
			if (px != SSE_NULL)
				*px = sse_awk_map_setpair (map, pair, val);
			else
				sse_awk_map_setpair (map, pair, val);

			return 0; /* value changed for the existing key */
		}
		pair = pair->next;
	}

	pair = (sse_awk_pair_t*) SSE_AWK_MALLOC (
		map->awk, sse_sizeof(sse_awk_pair_t));
	if (pair == SSE_NULL) return -1; /* error */

	/*pair->key = key;*/ 

	/* duplicate the key if it is new */
	pair->key = sse_awk_strxdup (map->awk, key, key_len);
	if (pair->key == SSE_NULL)
	{
		SSE_AWK_FREE (map->awk, pair);
		return -1; /* error */
	}

	pair->key_len = key_len;
	pair->val = val;
	pair->next = map->buck[hc];
	map->buck[hc] = pair;
	map->size++;

	if (px != SSE_NULL) *px = pair;
	return 1; /* new key added */
}

sse_awk_pair_t* sse_awk_map_set (
	sse_awk_map_t* map, sse_char_t* key, sse_size_t key_len, void* val)
{
	sse_awk_pair_t* pair;
	sse_size_t hc;

	hc = __hash(key,key_len) % map->capa;
	pair = map->buck[hc];

	while (pair != SSE_NULL) 
	{
		if (sse_awk_strxncmp (
			pair->key, pair->key_len, key, key_len) == 0) 
		{
			return sse_awk_map_setpair (map, pair, val);
		}
		pair = pair->next;
	}

	return SSE_NULL;
}

sse_awk_pair_t* sse_awk_map_getpair (
	sse_awk_map_t* map, const sse_char_t* key, sse_size_t key_len, void** val)
{
	sse_awk_pair_t* pair;

	pair = sse_awk_map_get (map, key, key_len);
	if (pair == SSE_NULL) return SSE_NULL; 
	*val = pair->val;

	return pair;
}

sse_awk_pair_t* sse_awk_map_setpair (
	sse_awk_map_t* map, sse_awk_pair_t* pair, void* val)
{
	/* use this function with care */
	if (pair->val != val) 
	{
		if (map->freeval != SSE_NULL) 
		{
			map->freeval (map->owner, pair->val);
		}
		pair->val = val;
	}

	return pair;
}

int sse_awk_map_remove (sse_awk_map_t* map, sse_char_t* key, sse_size_t key_len)
{
	sse_awk_pair_t* pair, * prev;
	sse_size_t hc;

	hc = __hash(key,key_len) % map->capa;
	pair = map->buck[hc];
	prev = SSE_NULL;

	while (pair != SSE_NULL) 
	{
		if (sse_awk_strxncmp (
			pair->key, pair->key_len, key, key_len) == 0) 
		{
			if (prev == SSE_NULL) 
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

int sse_awk_map_walk (sse_awk_map_t* map, 
	int (*walker) (sse_awk_pair_t*,void*), void* arg)
{
	sse_size_t i;
	sse_awk_pair_t* pair, * next;

	for (i = 0; i < map->capa; i++) 
	{
		pair = map->buck[i];

		while (pair != SSE_NULL) 
		{
			next = pair->next;
			if (walker(pair,arg) == -1) return -1;
			pair = next;
		}
	}

	return 0;
}

static sse_size_t __hash (const sse_char_t* key, sse_size_t key_len)
{
	sse_size_t n = 0, i;
	const sse_char_t* end = key + key_len;

	while (key < end)
	{
		sse_byte_t* bp = (sse_byte_t*)key;
		for (i = 0; i < sse_sizeof(*key); i++) n = n * 31 + *bp++;
		key++;
	}	

	return n;
}
