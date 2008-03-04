/*
 * $Id: map.c 116 2008-03-03 11:15:37Z baconevi $
 *
 * {License}
 */

#include <ase/cmn/map.h>
#include <ase/cmn/str.h>

static ase_size_t hashkey (const ase_char_t* keyptr, ase_size_t keylen);
static int rehash (ase_map_t* map);

#define FREE_PAIR(map,pair) \
	do { \
		if ((map)->freeval != ASE_NULL) \
			(map)->freeval ((map)->owner, ASE_PAIR_VAL(pair)); \
		ASE_FREE ((map)->mmgr, pair); \
	} while (0)

#define RECYCLE_PAIR(map,pair) \
	do { \
		if ((map)->freeval != ASE_NULL) \
			(map)->freeval ((map)->owner, ASE_PAIR_VAL(pair)); \
		(pair)->next = (map)->fp; \
		(map)->fp = (pair); \
	} while (0)

ase_map_t* ase_map_open (
	void* owner, ase_size_t capa, unsigned int factor,
	void(*freeval)(void*,void*), void(*sameval)(void*,void*), 
	ase_mmgr_t* mmgr)
{
	ase_map_t* map;

	ASE_ASSERTX (capa > 0, "the initial capacity should be greater than 0");

	map = (ase_map_t*) ASE_MALLOC (mmgr, ASE_SIZEOF(ase_map_t));
	if (map == ASE_NULL) return ASE_NULL;

	map->mmgr = mmgr;
	map->buck = (ase_pair_t**) 
		ASE_MALLOC (mmgr, ASE_SIZEOF(ase_pair_t*)*capa);
	if (map->buck == ASE_NULL) 
	{
		ASE_FREE (mmgr, map);
		return ASE_NULL;	
	}

	map->owner = owner;
	map->capa = capa;
	map->size = 0;
	map->freeval = freeval;
	map->sameval = sameval;
	while (capa > 0) map->buck[--capa] = ASE_NULL;

	map->factor = factor;
	map->threshold = ((ase_size_t)map->factor) * map->capa / 100;

	/*map->fp = ASE_NULL;*/
	return map;
}

void ase_map_close (ase_map_t* map)
{
	ase_map_clear (map);
	ASE_FREE (map->mmgr, map->buck);
	ASE_FREE (map->mmgr, map);
}

void ase_map_clear (ase_map_t* map)
{
	ase_size_t i;
	ase_pair_t* pair, * next;

	/*
	while (map->fp != ASE_NULL)
	{
		next = ASE_PAIR_LNK(map->fp);
		ASE_FREE (map->mmgr, map->fp);
		map->fp = next;
	}
	*/

	for (i = 0; i < map->capa; i++) 
	{
		pair = map->buck[i];

		while (pair != ASE_NULL) 
		{
			next = ASE_PAIR_LNK(pair);
			FREE_PAIR (map, pair);
			map->size--;
			pair = next;
		}

		map->buck[i] = ASE_NULL;
	}
}

ase_size_t ase_map_getsize (ase_map_t* map)
{
	return map->size;
}

ase_pair_t* ase_map_get (
	ase_map_t* map, const ase_char_t* keyptr, ase_size_t keylen)
{
	ase_pair_t* pair;
	ase_size_t hc;

	hc = hashkey(keyptr,keylen) % map->capa;
	pair = map->buck[hc];

	while (pair != ASE_NULL) 
	{
		if (ase_strxncmp (
			ASE_PAIR_KEYPTR(pair), ASE_PAIR_KEYLEN(pair), 
			keyptr, keylen) == 0) return pair;

		pair = ASE_PAIR_LNK(pair);
	}

	return ASE_NULL;
}

ase_pair_t* ase_map_put (
	ase_map_t* map, const ase_char_t* keyptr, ase_size_t keylen, void* val)
{
	int n;
	ase_pair_t* px;

	n = ase_map_putx (map, keyptr, keylen, val, &px);
	if (n < 0) return ASE_NULL;
	return px;
}

int ase_map_putx (
	ase_map_t* map, const ase_char_t* keyptr, ase_size_t keylen, 
	void* val, ase_pair_t** px)
{
	ase_pair_t* pair, * fp, * fp2;
	ase_size_t hc;

	hc = hashkey(keyptr,keylen) % map->capa;
	pair = map->buck[hc];

	while (pair != ASE_NULL) 
	{
		if (ase_strxncmp (
			ASE_PAIR_KEYPTR(pair), ASE_PAIR_KEYLEN(pair), 
			keyptr, keylen) == 0) 
		{
			if (px != ASE_NULL)
				*px = ase_map_setpair (map, pair, val);
			else
				ase_map_setpair (map, pair, val);

			return 0; /* value changed for the existing key */
		}
		pair = ASE_PAIR_LNK(pair);
	}

	if (map->threshold > 0 && 
	    map->size >= map->threshold)
	{
		if (rehash(map) == 0) /* ignore the rehash error */
		{
			hc = hashkey(keyptr,keylen) % map->capa;
		}
	}


	ASE_ASSERT (pair == ASE_NULL);

	/*
	fp = map->fp; fp2 = ASE_NULL;
	while (fp != ASE_NULL)
	{
		if (fp->key.len == keylen) 
		{
			pair = fp;
			if (fp2 == ASE_NULL) map->fp = fp->next;
			else fp2->next = fp->next;
			break;	
		}

		fp2 = fp;
		fp = fp->next;
	}

	if (pair == ASE_NULL)
	{
	*/
		pair = (ase_pair_t*) ASE_MALLOC (map->mmgr, 
			ASE_SIZEOF(ase_pair_t) + ((keylen+1)*ASE_SIZEOF(*keyptr)));
		if (pair == ASE_NULL) return -1; /* error */
	/*}*/

	/* duplicate the key if it is new */
	ASE_PAIR_KEYPTR(pair) = (ase_char_t*)(pair + 1);
	ase_strncpy (ASE_PAIR_KEYPTR(pair), keyptr, keylen);

	ASE_PAIR_KEYLEN(pair) = keylen;
	ASE_PAIR_VAL(pair) = val;
	ASE_PAIR_LNK(pair) = map->buck[hc];
	map->buck[hc] = pair;
	map->size++;

	if (px != ASE_NULL) *px = pair;
	return 1; /* new key added */
}

ase_pair_t* ase_map_set (
	ase_map_t* map, const ase_char_t* keyptr, ase_size_t keylen, 
	void* val)
{
	ase_pair_t* pair;
	ase_size_t hc;

	hc = hashkey(keyptr,keylen) % map->capa;
	pair = map->buck[hc];

	while (pair != ASE_NULL) 
	{
		if (ase_strxncmp (
			ASE_PAIR_KEYPTR(pair), ASE_PAIR_KEYLEN(pair), 
			keyptr, keylen) == 0) 
		{
			return ase_map_setpair (map, pair, val);
		}
		pair = ASE_PAIR_LNK(pair);
	}

	return ASE_NULL;
}

ase_pair_t* ase_map_getpair (
	ase_map_t* map, const ase_char_t* keyptr, ase_size_t keylen, 
	void** val)
{
	ase_pair_t* pair;

	pair = ase_map_get (map, keyptr, keylen);
	if (pair == ASE_NULL) return ASE_NULL; 
	*val = ASE_PAIR_VAL(pair);

	return pair;
}

ase_pair_t* ase_map_setpair (
	ase_map_t* map, ase_pair_t* pair, void* val)
{
	/* use this function with care */
	if (ASE_PAIR_VAL(pair) == val) 
	{
		/* if the old value and the new value are the same,
		 * it just calls the handler for this condition. 
		 * No value replacement occurs. */
		if (map->sameval != ASE_NULL)
		{
			map->sameval (map->owner, val);
		}
	}
	else
	{
		/* frees the old value */
		if (map->freeval != ASE_NULL) 
		{
			map->freeval (map->owner, ASE_PAIR_VAL(pair));
		}

		/* the new value takes the place */
		ASE_PAIR_VAL(pair) = val;
	}


	return pair;
}

int ase_map_remove (
	ase_map_t* map, const ase_char_t* keyptr, ase_size_t keylen)
{
	ase_pair_t* pair, * prev;
	ase_size_t hc;

	hc = hashkey(keyptr,keylen) % map->capa;
	pair = map->buck[hc];
	prev = ASE_NULL;

	while (pair != ASE_NULL) 
	{
		if (ase_strxncmp (
			ASE_PAIR_KEYPTR(pair), ASE_PAIR_KEYLEN(pair), 
			keyptr, keylen) == 0) 
		{
			if (prev == ASE_NULL) 
				map->buck[hc] = ASE_PAIR_LNK(pair);
			else ASE_PAIR_LNK(prev) = ASE_PAIR_LNK(pair);

			/*RECYCLE_PAIR (map, pair);*/
			FREE_PAIR (map, pair);

			map->size--;

			return 0;
		}

		prev = pair;
		pair = ASE_PAIR_LNK(pair);
	}

	return -1;
}

int ase_map_walk (ase_map_t* map, 
	int (*walker) (ase_pair_t*,void*), void* arg)
{
	ase_size_t i;
	ase_pair_t* pair, * next;

	for (i = 0; i < map->capa; i++) 
	{
		pair = map->buck[i];

		while (pair != ASE_NULL) 
		{
			next = ASE_PAIR_LNK(pair);
			if (walker(pair,arg) == -1) return -1;
			pair = next;
		}
	}

	return 0;
}

ase_pair_t* ase_map_getfirstpair (
	ase_map_t* map, ase_size_t* buckno)
{
	ase_size_t i;
	ase_pair_t* pair;

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

ase_pair_t* ase_map_getnextpair (
	ase_map_t* map, ase_pair_t* pair, ase_size_t* buckno)
{
	ase_size_t i;
	ase_pair_t* next;

	next = ASE_PAIR_LNK(pair);
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

static int rehash (ase_map_t* map)
{
	ase_size_t i, hc, new_capa;
	ase_pair_t** new_buck;

	new_capa = (map->capa >= 65536)? (map->capa + 65536): (map->capa << 1);

	new_buck = (ase_pair_t**) ASE_MALLOC (
		map->mmgr, ASE_SIZEOF(ase_pair_t*) * new_capa);
	if (new_buck == ASE_NULL) 
	{
		/* once rehash fails, the rehashing is disabled */
		map->threshold = 0;
		return -1;
	}

	for (i = 0; i < new_capa; i++) new_buck[i] = ASE_NULL;

	for (i = 0; i < map->capa; i++)
	{
		ase_pair_t* pair = map->buck[i];

		while (pair != ASE_NULL) 
		{
			ase_pair_t* next = ASE_PAIR_LNK(pair);

			hc = hashkey(
				ASE_PAIR_KEYPTR(pair),
				ASE_PAIR_KEYLEN(pair)) % new_capa;

			ASE_PAIR_LNK(pair) = new_buck[hc];
			new_buck[hc] = pair;

			pair = next;
		}
	}

	ASE_FREE (map->mmgr, map->buck);
	map->buck = new_buck;
	map->capa = new_capa;
	map->threshold = ((ase_size_t)map->factor) * map->capa / 100;

	return 0;
}
