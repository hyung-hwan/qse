/*
 * $Id: map.c 289 2009-09-16 06:35:29Z hyunghwan.chung $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qse/cmn/map.h>
#include "mem.h"

QSE_IMPLEMENT_COMMON_FUNCTIONS (map)

#define map_t    qse_map_t
#define pair_t   qse_map_pair_t
#define copier_t qse_map_copier_t
#define freeer_t qse_map_freeer_t
#define hasher_t qse_map_hasher_t
#define comper_t qse_map_comper_t
#define keeper_t qse_map_keeper_t
#define sizer_t  qse_map_sizer_t
#define walker_t qse_map_walker_t

#define KPTR(p)  QSE_MAP_KPTR(p)
#define KLEN(p)  QSE_MAP_KLEN(p)
#define VPTR(p)  QSE_MAP_VPTR(p)
#define VLEN(p)  QSE_MAP_VLEN(p)
#define NEXT(p)  QSE_MAP_NEXT(p)

#define SIZEOF(x) QSE_SIZEOF(x)
#define size_t    qse_size_t
#define byte_t    qse_byte_t
#define uint_t    qse_uint_t
#define mmgr_t    qse_mmgr_t

#define KTOB(map,len) ((len)*(map)->scale[QSE_MAP_KEY])
#define VTOB(map,len) ((len)*(map)->scale[QSE_MAP_VAL])

#define UPSERT 1
#define UPDATE 2
#define ENSERT 3
#define INSERT 4

static int reorganize (map_t* map);

static size_t hash_key (map_t* map, const void* kptr, size_t klen)
{
	/*size_t h = 2166136261;*/
	/*size_t h = 0;*/
	size_t h = 5381;
	const byte_t* p = (const byte_t*)kptr;
	const byte_t* bound = p + klen;

	while (p < bound)
	{
		/*h = (h * 16777619) ^ *p++;*/
		/*h = h * 31 + *p++;*/
		h = ((h << 5) + h) + *p++;
	}	

	return h ; 
}

static int comp_key (map_t* map, 
	const void* kptr1, size_t klen1, 
	const void* kptr2, size_t klen2)
{
	if (klen1 == klen2) return QSE_MEMCMP (kptr1, kptr2, KTOB(map,klen1));
	/* it just returns 1 to indicate that they are different. */
	return 1;
}

static pair_t* alloc_pair (map_t* map, 
	void* kptr, size_t klen, void* vptr, size_t vlen)
{
	pair_t* n;
	copier_t kcop = map->copier[QSE_MAP_KEY];
	copier_t vcop = map->copier[QSE_MAP_VAL];

	size_t as = SIZEOF(pair_t);
	if (kcop == QSE_MAP_COPIER_INLINE) as += KTOB(map,klen);
	if (vcop == QSE_MAP_COPIER_INLINE) as += VTOB(map,vlen);

	n = (pair_t*) QSE_MMGR_ALLOC (map->mmgr, as);
	if (n == QSE_NULL) return QSE_NULL;

	NEXT(n) = QSE_NULL;

	KLEN(n) = klen;
	if (kcop == QSE_MAP_COPIER_SIMPLE)
	{
		KPTR(n) = kptr;
	}
	else if (kcop == QSE_MAP_COPIER_INLINE)
	{
		KPTR(n) = n + 1;
		QSE_MEMCPY (KPTR(n), kptr, KTOB(map,klen));
	}
	else 
	{
		KPTR(n) = kcop (map, kptr, klen);
		if (KPTR(n) == QSE_NULL)
		{
			QSE_MMGR_FREE (map->mmgr, n);		
			return QSE_NULL;
		}
	}

	VLEN(n) = vlen;
	if (vcop == QSE_MAP_COPIER_SIMPLE)
	{
		VPTR(n) = vptr;
	}
	else if (vcop == QSE_MAP_COPIER_INLINE)
	{
		VPTR(n) = n + 1;
		if (kcop == QSE_MAP_COPIER_INLINE) 
			VPTR(n) = (byte_t*)VPTR(n) + KTOB(map,klen);
		QSE_MEMCPY (VPTR(n), vptr, VTOB(map,vlen));
	}
	else 
	{
		VPTR(n) = vcop (map, vptr, vlen);
		if (VPTR(n) != QSE_NULL)
		{
			if (map->freeer[QSE_MAP_KEY] != QSE_NULL)
				map->freeer[QSE_MAP_KEY] (map, KPTR(n), KLEN(n));
			QSE_MMGR_FREE (map->mmgr, n);		
			return QSE_NULL;
		}
	}

	return n;
}

static void free_pair (map_t* map, pair_t* pair)
{
	if (map->freeer[QSE_MAP_KEY] != QSE_NULL) 
		map->freeer[QSE_MAP_KEY] (map, KPTR(pair), KLEN(pair));
	if (map->freeer[QSE_MAP_VAL] != QSE_NULL)
		map->freeer[QSE_MAP_VAL] (map, VPTR(pair), VLEN(pair));
	QSE_MMGR_FREE (map->mmgr, pair);
}

static pair_t* change_pair_val (
	map_t* map, pair_t* pair, void* vptr, size_t vlen)
{
	if (VPTR(pair) == vptr && VLEN(pair) == vlen) 
	{
		/* if the old value and the new value are the same,
		 * it just calls the handler for this condition. 
		 * No value replacement occurs. */
		if (map->keeper != QSE_NULL)
		{
			map->keeper (map, vptr, vlen);
		}
	}
	else
	{
		copier_t vcop = map->copier[QSE_MAP_VAL];
		void* ovptr = VPTR(pair);
		size_t ovlen = VLEN(pair);

		/* place the new value according to the copier */
		if (vcop == QSE_MAP_COPIER_SIMPLE)
		{
			VPTR(pair) = vptr;
			VLEN(pair) = vlen;
		}
		else if (vcop == QSE_MAP_COPIER_INLINE)
		{
			if (ovlen == vlen)
			{
				QSE_MEMCPY (VPTR(pair), vptr, VTOB(map,vlen));
			}
			else
			{
				/* need to reconstruct the pair */
				pair_t* p = alloc_pair (map, 
					KPTR(pair), KLEN(pair),
					vptr, vlen);
				if (p == QSE_NULL) return QSE_NULL;
				free_pair (map, pair);
				return p;
			}
		}
		else 
		{
			void* nvptr = vcop (map, vptr, vlen);
			if (nvptr == QSE_NULL) return QSE_NULL;
			VPTR(pair) = nvptr;
			VLEN(pair) = vlen;
		}

		/* free up the old value */
		if (map->freeer[QSE_MAP_VAL] != QSE_NULL) 
		{
			map->freeer[QSE_MAP_VAL] (map, ovptr, ovlen);
		}
	}


	return pair;
}

map_t* qse_map_open (mmgr_t* mmgr, size_t ext, size_t capa, int factor)
{
	map_t* map;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	map = (map_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(map_t) + ext);
	if (map == QSE_NULL) return QSE_NULL;

	if (qse_map_init (map, mmgr, capa, factor) == QSE_NULL)
	{
		QSE_MMGR_FREE (mmgr, map);
		return QSE_NULL;
	}

	return map;
}

void qse_map_close (map_t* map)
{
	qse_map_fini (map);
	QSE_MMGR_FREE (map->mmgr, map);
}

map_t* qse_map_init (map_t* map, mmgr_t* mmgr, size_t capa, int factor)
{
	QSE_ASSERTX (capa > 0,
		"The initial capacity should be greater than 0. Otherwise, it is adjusted to 1 in the release mode");
	QSE_ASSERTX (factor >= 0 && factor <= 100,
		"The load factor should be between 0 and 100 inclusive. In the release mode, a value out of the range is adjusted to 100");

	/* some initial adjustment */
	if (capa <= 0) capa = 1;
	if (factor > 100) factor = 100;

	/* do not zero out the extension */
	QSE_MEMSET (map, 0, SIZEOF(*map));
	map->mmgr = mmgr;

	map->bucket = QSE_MMGR_ALLOC (mmgr, capa*SIZEOF(pair_t*));
	if (map->bucket == QSE_NULL) return QSE_NULL;

	/*for (i = 0; i < capa; i++) map->bucket[i] = QSE_NULL;*/
	QSE_MEMSET (map->bucket, 0, capa*SIZEOF(pair_t*));

	map->scale[QSE_MAP_KEY] = 1;
	map->scale[QSE_MAP_VAL] = 1;
	map->factor = factor;

	map->size = 0;
	map->capa = capa;
	map->threshold = map->capa * map->factor / 100;
	if (map->capa > 0 && map->threshold <= 0) map->threshold = 1;

	map->hasher = hash_key;
	map->comper = comp_key;
	map->copier[QSE_MAP_KEY] = QSE_MAP_COPIER_SIMPLE;
	map->copier[QSE_MAP_VAL] = QSE_MAP_COPIER_SIMPLE;

	/*
	map->freeer[QSE_MAP_KEY] = QSE_NULL;
	map->freeer[QSE_MAP_VAL] = QSE_NULL;
	map->keeper = QSE_NULL;
	map->sizer = QSE_NULL;
	*/

	return map;
}

void qse_map_fini (map_t* map)
{
	qse_map_clear (map);
	QSE_MMGR_FREE (map->mmgr, map->bucket);
}

int qse_map_getscale (map_t* map, qse_map_id_t id)
{
	QSE_ASSERTX (id == QSE_MAP_KEY || id == QSE_MAP_VAL,
		"The ID should be either QSE_MAP_KEY or QSE_MAP_VAL");
	return map->scale[id];
}

void qse_map_setscale (map_t* map, qse_map_id_t id, int scale)
{
	QSE_ASSERTX (id == QSE_MAP_KEY || id == QSE_MAP_VAL,
		"The ID should be either QSE_MAP_KEY or QSE_MAP_VAL");

	QSE_ASSERTX (scale > 0 && scale <= QSE_TYPE_MAX(qse_byte_t), 
		"The scale should be larger than 0 and less than or equal to the maximum value that the qse_byte_t type can hold");

	if (scale <= 0) scale = 1;
	if (scale > QSE_TYPE_MAX(qse_byte_t)) scale = QSE_TYPE_MAX(qse_byte_t);

	map->scale[id] = scale;
}

copier_t qse_map_getcopier (map_t* map, qse_map_id_t id)
{
	QSE_ASSERTX (id == QSE_MAP_KEY || id == QSE_MAP_VAL,
		"The ID should be either QSE_MAP_KEY or QSE_MAP_VAL");
	return map->copier[id];
}

void qse_map_setcopier (map_t* map, qse_map_id_t id, copier_t copier)
{
	QSE_ASSERTX (id == QSE_MAP_KEY || id == QSE_MAP_VAL,
		"The ID should be either QSE_MAP_KEY or QSE_MAP_VAL");
	if (copier == QSE_NULL) copier = QSE_MAP_COPIER_SIMPLE;
	map->copier[id] = copier;
}

freeer_t qse_map_getfreeer (map_t* map, qse_map_id_t id)
{
	QSE_ASSERTX (id == QSE_MAP_KEY || id == QSE_MAP_VAL,
		"The ID should be either QSE_MAP_KEY or QSE_MAP_VAL");
	return map->freeer[id];
}

void qse_map_setfreeer (map_t* map, qse_map_id_t id, freeer_t freeer)
{
	QSE_ASSERTX (id == QSE_MAP_KEY || id == QSE_MAP_VAL,
		"The ID should be either QSE_MAP_KEY or QSE_MAP_VAL");
	map->freeer[id] = freeer;
}

hasher_t qse_map_gethasher (map_t* map)
{
	return map->hasher;
}

void qse_map_sethasher (map_t* map, hasher_t hasher)
{
	if (hasher == QSE_NULL) hasher = hash_key;
	map->hasher = hasher;	
}

comper_t qse_map_getcomper (map_t* map)
{
	return map->comper;
}

void qse_map_setcomper (map_t* map, comper_t comper)
{
	if (comper == QSE_NULL) comper = comp_key;
	map->comper = comper;
}

keeper_t qse_map_getkeeper (map_t* map)
{
	return map->keeper;
}

void qse_map_setkeeper (map_t* map, keeper_t keeper)
{
	map->keeper = keeper;
}

sizer_t qse_map_getsizer (map_t* map)
{
	return map->sizer;
}

void qse_map_setsizer (map_t* map, sizer_t sizer)
{
	map->sizer = sizer;
}

size_t qse_map_getsize (map_t* map)
{
	return map->size;
}

size_t qse_map_getcapa (map_t* map)
{
	return map->capa;
}

pair_t* qse_map_search (map_t* map, const void* kptr, size_t klen)
{
	pair_t* pair;
	size_t hc;

	hc = map->hasher(map,kptr,klen) % map->capa;
	pair = map->bucket[hc];

	while (pair != QSE_NULL) 
	{
		if (map->comper (map, KPTR(pair), KLEN(pair), kptr, klen) == 0)
		{
			return pair;
		}

		pair = NEXT(pair);
	}

	return QSE_NULL;
}

static pair_t* insert (
	map_t* map, void* kptr, size_t klen, void* vptr, size_t vlen, int opt)
{
	pair_t* pair, * p, * prev, * next;
	size_t hc;

	hc = map->hasher(map,kptr,klen) % map->capa;
	pair = map->bucket[hc];
	prev = QSE_NULL;

	while (pair != QSE_NULL) 
	{
		next = NEXT(pair);

		if (map->comper (map, KPTR(pair), KLEN(pair), kptr, klen) == 0) 
		{
			/* found a pair with a matching key */
			switch (opt)
			{
				case UPSERT:
				case UPDATE:
					p = change_pair_val (map, pair, vptr, vlen);
					if (p == QSE_NULL) 
					{
						/* error in change the value */
						return QSE_NULL; 
					}
					if (p != pair) 
					{
						/* pair reallocated. 
						 * relink it */
						if (prev == QSE_NULL) 
							map->bucket[hc] = p;
						else NEXT(prev) = p;
						NEXT(p) = next;
					}
					return p;

				case ENSERT:
					/* return existing pair */
					return pair; 

				case INSERT:
					/* return failure */
					return QSE_NULL;
			}
		}

		prev = pair;
		pair = next;
	}

	if (opt == UPDATE) return QSE_NULL;

	if (map->threshold > 0 && map->size >= map->threshold)
	{
		if (reorganize(map) == 0) /* ignore the error */
		{
			hc = map->hasher(map,kptr,klen) % map->capa;
		}
	}

	QSE_ASSERT (pair == QSE_NULL);

	pair = alloc_pair (map, kptr, klen, vptr, vlen);
	if (pair == QSE_NULL) return QSE_NULL; /* error */

	NEXT(pair) = map->bucket[hc];
	map->bucket[hc] = pair;
	map->size++;

	return pair; /* new key added */
}

pair_t* qse_map_upsert (
	map_t* map, void* kptr, size_t klen, void* vptr, size_t vlen)
{
	return insert (map, kptr, klen, vptr, vlen, UPSERT);
}

pair_t* qse_map_ensert (
	map_t* map, void* kptr, size_t klen, void* vptr, size_t vlen)
{
	return insert (map, kptr, klen, vptr, vlen, ENSERT);
}

pair_t* qse_map_insert (
	map_t* map, void* kptr, size_t klen, void* vptr, size_t vlen)
{
	return insert (map, kptr, klen, vptr, vlen, INSERT);
}


pair_t* qse_map_update (
	map_t* map, void* kptr, size_t klen, void* vptr, size_t vlen)
{
	return insert (map, kptr, klen, vptr, vlen, UPDATE);
}

int qse_map_delete (map_t* map, const void* kptr, size_t klen)
{
	pair_t* pair, * prev;
	size_t hc;

	hc = map->hasher(map,kptr,klen) % map->capa;
	pair = map->bucket[hc];
	prev = QSE_NULL;

	while (pair != QSE_NULL) 
	{
		if (map->comper (map, KPTR(pair), KLEN(pair), kptr, klen) == 0) 
		{
			if (prev == QSE_NULL) 
				map->bucket[hc] = NEXT(pair);
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

void qse_map_clear (map_t* map)
{
	size_t i;
	pair_t* pair, * next;

	for (i = 0; i < map->capa; i++) 
	{
		pair = map->bucket[i];

		while (pair != QSE_NULL) 
		{
			next = NEXT(pair);
			free_pair (map, pair);
			map->size--;
			pair = next;
		}

		map->bucket[i] = QSE_NULL;
	}
}


void qse_map_walk (map_t* map, walker_t walker, void* arg)
{
	size_t i;
	pair_t* pair, * next;

	for (i = 0; i < map->capa; i++) 
	{
		pair = map->bucket[i];

		while (pair != QSE_NULL) 
		{
			next = NEXT(pair);
			if (walker(map, pair, arg) == QSE_MAP_WALK_STOP) return;
			pair = next;
		}
	}
}

pair_t* qse_map_getfirstpair (map_t* map, size_t* buckno)
{
	size_t i;
	pair_t* pair;

	for (i = 0; i < map->capa; i++)
	{
		pair = map->bucket[i];
		if (pair != QSE_NULL) 
		{
			*buckno = i;
			return pair;
		}
	}

	return QSE_NULL;
}

pair_t* qse_map_getnextpair (map_t* map, pair_t* pair, size_t* buckno)
{
	size_t i;
	pair_t* next;

	next = NEXT(pair);
	if (next != QSE_NULL) 
	{
		/* no change in bucket number */
		return next;
	}

	for (i = (*buckno)+1; i < map->capa; i++)
	{
		pair = map->bucket[i];
		if (pair != QSE_NULL) 
		{
			*buckno = i;
			return pair;
		}
	}

	return QSE_NULL;
}

static int reorganize (map_t* map)
{
	size_t i, hc, new_capa;
	pair_t** new_buck;

	if (map->sizer)
	{
		new_capa = map->sizer (map, map->capa + 1);

		/* if no change in capacity, return success 
		 * without reorganization */
		if (new_capa == map->capa) return 0; 

		/* adjust to 1 if the new capacity is not reasonable */
		if (new_capa <= 0) new_capa = 1;
	}
	else
	{
		/* the bucket is doubled until it grows up to 65536 slots.
		 * once it has reached it, it grows by 65536 slots */
		new_capa = (map->capa >= 65536)? (map->capa + 65536): (map->capa << 1);
	}

	new_buck = (pair_t**) QSE_MMGR_ALLOC (
		map->mmgr, new_capa*SIZEOF(pair_t*));
	if (new_buck == QSE_NULL) 
	{
		/* reorganization is disabled once it fails */
		map->threshold = 0;
		return -1;
	}

	/*for (i = 0; i < new_capa; i++) new_buck[i] = QSE_NULL;*/
	QSE_MEMSET (new_buck, 0, new_capa*SIZEOF(pair_t*));

	for (i = 0; i < map->capa; i++)
	{
		pair_t* pair = map->bucket[i];

		while (pair != QSE_NULL) 
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

	QSE_MMGR_FREE (map->mmgr, map->bucket);
	map->bucket = new_buck;
	map->capa = new_capa;
	map->threshold = map->capa * map->factor / 100;

	return 0;
}
