/*
 * $Id: map.c,v 1.26 2006-10-12 04:17:31 bacon Exp $
 */

#include <xp/awk/awk_i.h>

/* TODO: improve the entire map routines.
         support automatic bucket resizing and remaping, etc. */

static xp_size_t __hash (const xp_char_t* key, xp_size_t key_len);

#define FREE_PAIR(map,pair) \
	do { \
		XP_AWK_FREE ((map)->awk, (xp_char_t*)(pair)->key); \
		if ((map)->freeval != XP_NULL) \
			(map)->freeval ((map)->owner, (pair)->val); \
		XP_AWK_FREE ((map)->awk, pair); \
	} while (0)

xp_awk_map_t* xp_awk_map_open (
	xp_awk_map_t* map, void* owner, xp_size_t capa, 
	void(*freeval)(void*,void*), xp_awk_t* awk)
{
	if (map == XP_NULL) 
	{
		map = (xp_awk_map_t*) XP_AWK_MALLOC (
			awk, xp_sizeof(xp_awk_map_t));
		if (map == XP_NULL) return XP_NULL;
		map->__dynamic = xp_true;
	}
	else map->__dynamic = xp_false;

	map->awk = awk;
	map->buck = (xp_awk_pair_t**) 
		XP_AWK_MALLOC (awk, xp_sizeof(xp_awk_pair_t*) * capa);
	if (map->buck == XP_NULL) 
	{
		if (map->__dynamic) XP_AWK_FREE (awk, map);
		return XP_NULL;	
	}

	map->owner = owner;
	map->capa = capa;
	map->size = 0;
	map->freeval = freeval;
	while (capa > 0) map->buck[--capa] = XP_NULL;

	return map;
}

void xp_awk_map_close (xp_awk_map_t* map)
{
	xp_awk_map_clear (map);
	XP_AWK_FREE (map->awk, map->buck);
	if (map->__dynamic) XP_AWK_FREE (map->awk, map);
}

void xp_awk_map_clear (xp_awk_map_t* map)
{
	xp_size_t i;
	xp_awk_pair_t* pair, * next;

	for (i = 0; i < map->capa; i++) 
	{
		pair = map->buck[i];

		while (pair != XP_NULL) 
		{
			next = pair->next;
			FREE_PAIR (map, pair);
			map->size--;
			pair = next;
		}

		map->buck[i] = XP_NULL;
	}

	xp_awk_assert (map->awk, map->size == 0);
}

xp_awk_pair_t* xp_awk_map_get (
	xp_awk_map_t* map, const xp_char_t* key, xp_size_t key_len)
{
	xp_awk_pair_t* pair;
	xp_size_t hc;

	hc = __hash(key,key_len) % map->capa;
	pair = map->buck[hc];

	while (pair != XP_NULL) 
	{

		if (xp_awk_strxncmp (
			pair->key, pair->key_len, 
			key, key_len) == 0) return pair;

		pair = pair->next;
	}

	return XP_NULL;
}

xp_awk_pair_t* xp_awk_map_put (
	xp_awk_map_t* map, xp_char_t* key, xp_size_t key_len, void* val)
{
	int n;
	xp_awk_pair_t* px;

	n = xp_awk_map_putx (map, key, key_len, val, &px);
	if (n < 0) return XP_NULL;
	return px;
}

int xp_awk_map_putx (
	xp_awk_map_t* map, xp_char_t* key, xp_size_t key_len, 
	void* val, xp_awk_pair_t** px)
{
	xp_awk_pair_t* pair;
	xp_size_t hc;

	hc = __hash(key,key_len) % map->capa;
	pair = map->buck[hc];

	while (pair != XP_NULL) 
	{
		if (xp_awk_strxncmp (
			pair->key, pair->key_len, key, key_len) == 0) 
		{
			if (px != XP_NULL)
				*px = xp_awk_map_setpair (map, pair, val);
			else
				xp_awk_map_setpair (map, pair, val);

			return 0; /* value changed for the existing key */
		}
		pair = pair->next;
	}

	pair = (xp_awk_pair_t*) XP_AWK_MALLOC (
		map->awk, xp_sizeof(xp_awk_pair_t));
	if (pair == XP_NULL) return -1; /* error */

	/*pair->key = key;*/ 

	/* duplicate the key if it is new */
	pair->key = xp_awk_strxdup (map->awk, key, key_len);
	if (pair->key == XP_NULL)
	{
		XP_AWK_FREE (map->awk, pair);
		return -1; /* error */
	}

	pair->key_len = key_len;
	pair->val = val;
	pair->next = map->buck[hc];
	map->buck[hc] = pair;
	map->size++;

	if (px != XP_NULL) *px = pair;
	return 1; /* new key added */
}

xp_awk_pair_t* xp_awk_map_set (
	xp_awk_map_t* map, xp_char_t* key, xp_size_t key_len, void* val)
{
	xp_awk_pair_t* pair;
	xp_size_t hc;

	hc = __hash(key,key_len) % map->capa;
	pair = map->buck[hc];

	while (pair != XP_NULL) 
	{
		if (xp_awk_strxncmp (
			pair->key, pair->key_len, key, key_len) == 0) 
		{
			return xp_awk_map_setpair (map, pair, val);
		}
		pair = pair->next;
	}

	return XP_NULL;
}

xp_awk_pair_t* xp_awk_map_getpair (
	xp_awk_map_t* map, const xp_char_t* key, xp_size_t key_len, void** val)
{
	xp_awk_pair_t* pair;

	pair = xp_awk_map_get (map, key, key_len);
	if (pair == XP_NULL) return XP_NULL; 
	*val = pair->val;

	return pair;
}

xp_awk_pair_t* xp_awk_map_setpair (
	xp_awk_map_t* map, xp_awk_pair_t* pair, void* val)
{
	/* use this function with care */
	if (pair->val != val) 
	{
		if (map->freeval != XP_NULL) 
		{
			map->freeval (map->owner, pair->val);
		}
		pair->val = val;
	}

	return pair;
}

int xp_awk_map_remove (xp_awk_map_t* map, xp_char_t* key, xp_size_t key_len)
{
	xp_awk_pair_t* pair, * prev;
	xp_size_t hc;

	hc = __hash(key,key_len) % map->capa;
	pair = map->buck[hc];
	prev = XP_NULL;

	while (pair != XP_NULL) 
	{
		if (xp_awk_strxncmp (
			pair->key, pair->key_len, key, key_len) == 0) 
		{
			if (prev == XP_NULL) 
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

int xp_awk_map_walk (xp_awk_map_t* map, 
	int (*walker) (xp_awk_pair_t*,void*), void* arg)
{
	xp_size_t i;
	xp_awk_pair_t* pair, * next;

	for (i = 0; i < map->capa; i++) 
	{
		pair = map->buck[i];

		while (pair != XP_NULL) 
		{
			next = pair->next;
			if (walker(pair,arg) == -1) return -1;
			pair = next;
		}
	}

	return 0;
}

static xp_size_t __hash (const xp_char_t* key, xp_size_t key_len)
{
	xp_size_t n = 0, i;
	const xp_char_t* end = key + key_len;

	while (key < end)
	{
		xp_byte_t* bp = (xp_byte_t*)key;
		for (i = 0; i < xp_sizeof(*key); i++) n = n * 31 + *bp++;
		key++;
	}	

	return n;
}
