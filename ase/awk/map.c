/*
 * $Id: map.c,v 1.15 2006-04-24 07:46:35 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/memory.h>
#include <xp/bas/string.h>
#include <xp/bas/assert.h>
#endif

/* TODO: improve the entire map routines.
         support automatic bucket resizing and remaping, etc. */

static xp_size_t __hash (const xp_char_t* key);

#define FREE_PAIR(map,pair) \
	do \
	{ \
		xp_free ((xp_char_t*)(pair)->key); \
		if ((map)->freeval != XP_NULL) \
			(map)->freeval ((map)->owner, (pair)->val); \
		xp_free (pair); \
	} while (0)

xp_awk_map_t* xp_awk_map_open (xp_awk_map_t* map, 
	void* owner, xp_size_t capa, void(*freeval)(void*,void*))
{
	if (map == XP_NULL) 
	{
		map = (xp_awk_map_t*) xp_malloc (xp_sizeof(xp_awk_map_t));
		if (map == XP_NULL) return XP_NULL;
		map->__dynamic = xp_true;
	}
	else map->__dynamic = xp_false;

	map->buck = (xp_awk_pair_t**) 
		xp_malloc (xp_sizeof(xp_awk_pair_t*) * capa);
	if (map->buck == XP_NULL) 
	{
		if (map->__dynamic) xp_free (map);
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
	xp_free (map->buck);
	if (map->__dynamic) xp_free (map);
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

	xp_assert (map->size == 0);
}

xp_awk_pair_t* xp_awk_map_get (xp_awk_map_t* map, xp_char_t* key)
{
	xp_awk_pair_t* pair;
	xp_size_t hc;

	hc = __hash(key) % map->capa;
	pair = map->buck[hc];

	while (pair != XP_NULL) 
	{
		if (xp_strcmp(pair->key,key) == 0) return pair;
		pair = pair->next;
	}

	return XP_NULL;
}

xp_awk_pair_t* xp_awk_map_put (xp_awk_map_t* map, xp_char_t* key, void* val)
{
/*
	xp_awk_pair_t* pair;
	xp_size_t hc;

	hc = __hash(key) % map->capa;
	pair = map->buck[hc];

	while (pair != XP_NULL) 
	{
		if (xp_strcmp(pair->key,key) == 0) 
		{
			if (pair->key != key) 
			{
				xp_free ((xp_char_t*)pair->key);
				pair->key = key;
			}

			return xp_awk_map_setpair (map, pair, val);
		}
		pair = pair->next;
	}

	pair = (xp_awk_pair_t*) xp_malloc (xp_sizeof(xp_awk_pair_t));
	if (pair == XP_NULL) return XP_NULL;

	pair->key = key;
	pair->val = val;
	pair->next = map->buck[hc];
	map->buck[hc] = pair;
	map->size++;

	return pair;
*/
	int n;
	xp_awk_pair_t* px;

	n = xp_awk_map_putx (map, key, val, &px);
	if (n < 0) return XP_NULL;
	return px;
}

int xp_awk_map_putx (
	xp_awk_map_t* map, xp_char_t* key, void* val, xp_awk_pair_t** px)
{
	xp_awk_pair_t* pair;
	xp_size_t hc;

	hc = __hash(key) % map->capa;
	pair = map->buck[hc];

	while (pair != XP_NULL) 
	{
		if (xp_strcmp(pair->key,key) == 0) 
		{
			if (px != XP_NULL)
				*px = xp_awk_map_setpair (map, pair, val);
			else
				xp_awk_map_setpair (map, pair, val);

			return 0; /* value changed for the existing key */
		}
		pair = pair->next;
	}

	pair = (xp_awk_pair_t*) xp_malloc (xp_sizeof(xp_awk_pair_t));
	if (pair == XP_NULL) return -1; /* error */

	/*pair->key = key;*/
	pair->key = xp_strdup (key); /* duplicate the key if it is new */
	if (pair->key == XP_NULL)
	{
		xp_free (pair);
		return -1; /* error */
	}
	pair->val = val;
	pair->next = map->buck[hc];
	map->buck[hc] = pair;
	map->size++;

	if (px != XP_NULL) *px = pair;
	return 1; /* new key added */
}

xp_awk_pair_t* xp_awk_map_set (xp_awk_map_t* map, xp_char_t* key, void* val)
{
	xp_awk_pair_t* pair;
	xp_size_t hc;

	hc = __hash(key) % map->capa;
	pair = map->buck[hc];

	while (pair != XP_NULL) 
	{
		if (xp_strcmp(pair->key,key) == 0) 
		{
			return xp_awk_map_setpair (map, pair, val);
		}
		pair = pair->next;
	}

	return XP_NULL;
}

xp_awk_pair_t* xp_awk_map_getpair (
	xp_awk_map_t* map, xp_char_t* key, void** val)
{
	xp_awk_pair_t* pair;

	pair = xp_awk_map_get (map, key);
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

int xp_awk_map_remove (xp_awk_map_t* map, xp_char_t* key)
{
	xp_awk_pair_t* pair, * prev;
	xp_size_t hc;

	hc = __hash(key) % map->capa;
	pair = map->buck[hc];
	prev = XP_NULL;

	while (pair != XP_NULL) 
	{
		if (xp_strcmp(pair->key,key) == 0) 
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

int xp_awk_map_walk (xp_awk_map_t* map, int (*walker) (xp_awk_pair_t*))
{
	xp_size_t i;
	xp_awk_pair_t* pair, * next;

	for (i = 0; i < map->capa; i++) 
	{
		pair = map->buck[i];

		while (pair != XP_NULL) 
		{
			next = pair->next;
			if (walker(pair) == -1) return -1;
			pair = next;
		}
	}

	return 0;
}

static xp_size_t __hash (const xp_char_t* key)
{
	xp_size_t n = 0, i;

	while (*key != XP_CHAR('\0')) 
	{
		xp_byte_t* bp = (xp_byte_t*)key;
		for (i = 0; i < xp_sizeof(*key); i++) n = n * 31 + *bp++;
		key++;
	}	

	return n;
}
