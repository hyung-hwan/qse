/*
 * $Id: htb.c 332 2010-07-13 11:25:24Z hyunghwan.chung $
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

#include <qse/cmn/htb.h>
#include "mem.h"

QSE_IMPLEMENT_COMMON_FUNCTIONS (htb)

#define htb_t    qse_htb_t
#define pair_t   qse_htb_pair_t
#define copier_t qse_htb_copier_t
#define freeer_t qse_htb_freeer_t
#define hasher_t qse_htb_hasher_t
#define comper_t qse_htb_comper_t
#define keeper_t qse_htb_keeper_t
#define sizer_t  qse_htb_sizer_t
#define walker_t qse_htb_walker_t

#define KPTR(p)  QSE_HTB_KPTR(p)
#define KLEN(p)  QSE_HTB_KLEN(p)
#define VPTR(p)  QSE_HTB_VPTR(p)
#define VLEN(p)  QSE_HTB_VLEN(p)
#define NEXT(p)  QSE_HTB_NEXT(p)

#define SIZEOF(x) QSE_SIZEOF(x)
#define size_t    qse_size_t
#define byte_t    qse_byte_t
#define mmgr_t    qse_mmgr_t

#define KTOB(htb,len) ((len)*(htb)->scale[QSE_HTB_KEY])
#define VTOB(htb,len) ((len)*(htb)->scale[QSE_HTB_VAL])

#define UPSERT 1
#define UPDATE 2
#define ENSERT 3
#define INSERT 4

static int reorganize (htb_t* htb);

static size_t hash_key (htb_t* htb, const void* kptr, size_t klen)
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

static QSE_INLINE int comp_key (htb_t* htb, 
	const void* kptr1, size_t klen1, 
	const void* kptr2, size_t klen2)
{
	if (klen1 == klen2) return QSE_MEMCMP (kptr1, kptr2, KTOB(htb,klen1));
	/* it just returns 1 to indicate that they are different. */
	return 1;
}

static pair_t* alloc_pair (htb_t* htb, 
	void* kptr, size_t klen, void* vptr, size_t vlen)
{
	pair_t* n;
	copier_t kcop = htb->copier[QSE_HTB_KEY];
	copier_t vcop = htb->copier[QSE_HTB_VAL];

	size_t as = SIZEOF(pair_t);
	if (kcop == QSE_HTB_COPIER_INLINE) as += KTOB(htb,klen);
	if (vcop == QSE_HTB_COPIER_INLINE) as += VTOB(htb,vlen);

	n = (pair_t*) QSE_MMGR_ALLOC (htb->mmgr, as);
	if (n == QSE_NULL) return QSE_NULL;

	NEXT(n) = QSE_NULL;

	KLEN(n) = klen;
	if (kcop == QSE_HTB_COPIER_SIMPLE)
	{
		KPTR(n) = kptr;
	}
	else if (kcop == QSE_HTB_COPIER_INLINE)
	{
		KPTR(n) = n + 1;
		QSE_MEMCPY (KPTR(n), kptr, KTOB(htb,klen));
	}
	else 
	{
		KPTR(n) = kcop (htb, kptr, klen);
		if (KPTR(n) == QSE_NULL)
		{
			QSE_MMGR_FREE (htb->mmgr, n);		
			return QSE_NULL;
		}
	}

	VLEN(n) = vlen;
	if (vcop == QSE_HTB_COPIER_SIMPLE)
	{
		VPTR(n) = vptr;
	}
	else if (vcop == QSE_HTB_COPIER_INLINE)
	{
		VPTR(n) = n + 1;
		if (kcop == QSE_HTB_COPIER_INLINE) 
			VPTR(n) = (byte_t*)VPTR(n) + KTOB(htb,klen);
		QSE_MEMCPY (VPTR(n), vptr, VTOB(htb,vlen));
	}
	else 
	{
		VPTR(n) = vcop (htb, vptr, vlen);
		if (VPTR(n) != QSE_NULL)
		{
			if (htb->freeer[QSE_HTB_KEY] != QSE_NULL)
				htb->freeer[QSE_HTB_KEY] (htb, KPTR(n), KLEN(n));
			QSE_MMGR_FREE (htb->mmgr, n);		
			return QSE_NULL;
		}
	}

	return n;
}

static void free_pair (htb_t* htb, pair_t* pair)
{
	if (htb->freeer[QSE_HTB_KEY] != QSE_NULL) 
		htb->freeer[QSE_HTB_KEY] (htb, KPTR(pair), KLEN(pair));
	if (htb->freeer[QSE_HTB_VAL] != QSE_NULL)
		htb->freeer[QSE_HTB_VAL] (htb, VPTR(pair), VLEN(pair));
	QSE_MMGR_FREE (htb->mmgr, pair);
}

static pair_t* change_pair_val (
	htb_t* htb, pair_t* pair, void* vptr, size_t vlen)
{
	if (VPTR(pair) == vptr && VLEN(pair) == vlen) 
	{
		/* if the old value and the new value are the same,
		 * it just calls the handler for this condition. 
		 * No value replacement occurs. */
		if (htb->keeper != QSE_NULL)
		{
			htb->keeper (htb, vptr, vlen);
		}
	}
	else
	{
		copier_t vcop = htb->copier[QSE_HTB_VAL];
		void* ovptr = VPTR(pair);
		size_t ovlen = VLEN(pair);

		/* place the new value according to the copier */
		if (vcop == QSE_HTB_COPIER_SIMPLE)
		{
			VPTR(pair) = vptr;
			VLEN(pair) = vlen;
		}
		else if (vcop == QSE_HTB_COPIER_INLINE)
		{
			if (ovlen == vlen)
			{
				QSE_MEMCPY (VPTR(pair), vptr, VTOB(htb,vlen));
			}
			else
			{
				/* need to reconstruct the pair */
				pair_t* p = alloc_pair (htb, 
					KPTR(pair), KLEN(pair),
					vptr, vlen);
				if (p == QSE_NULL) return QSE_NULL;
				free_pair (htb, pair);
				return p;
			}
		}
		else 
		{
			void* nvptr = vcop (htb, vptr, vlen);
			if (nvptr == QSE_NULL) return QSE_NULL;
			VPTR(pair) = nvptr;
			VLEN(pair) = vlen;
		}

		/* free up the old value */
		if (htb->freeer[QSE_HTB_VAL] != QSE_NULL) 
		{
			htb->freeer[QSE_HTB_VAL] (htb, ovptr, ovlen);
		}
	}


	return pair;
}

htb_t* qse_htb_open (mmgr_t* mmgr, size_t ext, size_t capa, int factor)
{
	htb_t* htb;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	htb = (htb_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(htb_t) + ext);
	if (htb == QSE_NULL) return QSE_NULL;

	if (qse_htb_init (htb, mmgr, capa, factor) == QSE_NULL)
	{
		QSE_MMGR_FREE (mmgr, htb);
		return QSE_NULL;
	}

	return htb;
}

void qse_htb_close (htb_t* htb)
{
	qse_htb_fini (htb);
	QSE_MMGR_FREE (htb->mmgr, htb);
}

htb_t* qse_htb_init (htb_t* htb, mmgr_t* mmgr, size_t capa, int factor)
{
	QSE_ASSERTX (capa > 0,
		"The initial capacity should be greater than 0. Otherwise, it is adjusted to 1 in the release mode");
	QSE_ASSERTX (factor >= 0 && factor <= 100,
		"The load factor should be between 0 and 100 inclusive. In the release mode, a value out of the range is adjusted to 100");

	/* some initial adjustment */
	if (capa <= 0) capa = 1;
	if (factor > 100) factor = 100;

	/* do not zero out the extension */
	QSE_MEMSET (htb, 0, SIZEOF(*htb));
	htb->mmgr = mmgr;

	htb->bucket = QSE_MMGR_ALLOC (mmgr, capa*SIZEOF(pair_t*));
	if (htb->bucket == QSE_NULL) return QSE_NULL;

	/*for (i = 0; i < capa; i++) htb->bucket[i] = QSE_NULL;*/
	QSE_MEMSET (htb->bucket, 0, capa*SIZEOF(pair_t*));

	htb->scale[QSE_HTB_KEY] = 1;
	htb->scale[QSE_HTB_VAL] = 1;
	htb->factor = factor;

	htb->size = 0;
	htb->capa = capa;
	htb->threshold = htb->capa * htb->factor / 100;
	if (htb->capa > 0 && htb->threshold <= 0) htb->threshold = 1;

	htb->hasher = hash_key;
	htb->comper = comp_key;
	htb->copier[QSE_HTB_KEY] = QSE_HTB_COPIER_SIMPLE;
	htb->copier[QSE_HTB_VAL] = QSE_HTB_COPIER_SIMPLE;

	/*
	htb->freeer[QSE_HTB_KEY] = QSE_NULL;
	htb->freeer[QSE_HTB_VAL] = QSE_NULL;
	htb->keeper = QSE_NULL;
	htb->sizer = QSE_NULL;
	*/

	return htb;
}

void qse_htb_fini (htb_t* htb)
{
	qse_htb_clear (htb);
	QSE_MMGR_FREE (htb->mmgr, htb->bucket);
}

int qse_htb_getscale (htb_t* htb, qse_htb_id_t id)
{
	QSE_ASSERTX (id == QSE_HTB_KEY || id == QSE_HTB_VAL,
		"The ID should be either QSE_HTB_KEY or QSE_HTB_VAL");
	return htb->scale[id];
}

void qse_htb_setscale (htb_t* htb, qse_htb_id_t id, int scale)
{
	QSE_ASSERTX (id == QSE_HTB_KEY || id == QSE_HTB_VAL,
		"The ID should be either QSE_HTB_KEY or QSE_HTB_VAL");

	QSE_ASSERTX (scale > 0 && scale <= QSE_TYPE_MAX(qse_byte_t), 
		"The scale should be larger than 0 and less than or equal to the maximum value that the qse_byte_t type can hold");

	if (scale <= 0) scale = 1;
	if (scale > QSE_TYPE_MAX(qse_byte_t)) scale = QSE_TYPE_MAX(qse_byte_t);

	htb->scale[id] = scale;
}

copier_t qse_htb_getcopier (htb_t* htb, qse_htb_id_t id)
{
	QSE_ASSERTX (id == QSE_HTB_KEY || id == QSE_HTB_VAL,
		"The ID should be either QSE_HTB_KEY or QSE_HTB_VAL");
	return htb->copier[id];
}

void qse_htb_setcopier (htb_t* htb, qse_htb_id_t id, copier_t copier)
{
	QSE_ASSERTX (id == QSE_HTB_KEY || id == QSE_HTB_VAL,
		"The ID should be either QSE_HTB_KEY or QSE_HTB_VAL");
	if (copier == QSE_NULL) copier = QSE_HTB_COPIER_SIMPLE;
	htb->copier[id] = copier;
}

freeer_t qse_htb_getfreeer (htb_t* htb, qse_htb_id_t id)
{
	QSE_ASSERTX (id == QSE_HTB_KEY || id == QSE_HTB_VAL,
		"The ID should be either QSE_HTB_KEY or QSE_HTB_VAL");
	return htb->freeer[id];
}

void qse_htb_setfreeer (htb_t* htb, qse_htb_id_t id, freeer_t freeer)
{
	QSE_ASSERTX (id == QSE_HTB_KEY || id == QSE_HTB_VAL,
		"The ID should be either QSE_HTB_KEY or QSE_HTB_VAL");
	htb->freeer[id] = freeer;
}

hasher_t qse_htb_gethasher (htb_t* htb)
{
	return htb->hasher;
}

void qse_htb_sethasher (htb_t* htb, hasher_t hasher)
{
	if (hasher == QSE_NULL) hasher = hash_key;
	htb->hasher = hasher;	
}

comper_t qse_htb_getcomper (htb_t* htb)
{
	return htb->comper;
}

void qse_htb_setcomper (htb_t* htb, comper_t comper)
{
	if (comper == QSE_NULL) comper = comp_key;
	htb->comper = comper;
}

keeper_t qse_htb_getkeeper (htb_t* htb)
{
	return htb->keeper;
}

void qse_htb_setkeeper (htb_t* htb, keeper_t keeper)
{
	htb->keeper = keeper;
}

sizer_t qse_htb_getsizer (htb_t* htb)
{
	return htb->sizer;
}

void qse_htb_setsizer (htb_t* htb, sizer_t sizer)
{
	htb->sizer = sizer;
}

size_t qse_htb_getsize (htb_t* htb)
{
	return htb->size;
}

size_t qse_htb_getcapa (htb_t* htb)
{
	return htb->capa;
}

pair_t* qse_htb_search (htb_t* htb, const void* kptr, size_t klen)
{
	pair_t* pair;
	size_t hc;

	hc = htb->hasher(htb,kptr,klen) % htb->capa;
	pair = htb->bucket[hc];

	while (pair != QSE_NULL) 
	{
		if (htb->comper (htb, KPTR(pair), KLEN(pair), kptr, klen) == 0)
		{
			return pair;
		}

		pair = NEXT(pair);
	}

	return QSE_NULL;
}

static pair_t* insert (
	htb_t* htb, void* kptr, size_t klen, void* vptr, size_t vlen, int opt)
{
	pair_t* pair, * p, * prev, * next;
	size_t hc;

	hc = htb->hasher(htb,kptr,klen) % htb->capa;
	pair = htb->bucket[hc];
	prev = QSE_NULL;

	while (pair != QSE_NULL) 
	{
		next = NEXT(pair);

		if (htb->comper (htb, KPTR(pair), KLEN(pair), kptr, klen) == 0) 
		{
			/* found a pair with a matching key */
			switch (opt)
			{
				case UPSERT:
				case UPDATE:
					p = change_pair_val (htb, pair, vptr, vlen);
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
							htb->bucket[hc] = p;
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

	if (htb->threshold > 0 && htb->size >= htb->threshold)
	{
		if (reorganize(htb) == 0) /* ignore the error */
		{
			hc = htb->hasher(htb,kptr,klen) % htb->capa;
		}
	}

	QSE_ASSERT (pair == QSE_NULL);

	pair = alloc_pair (htb, kptr, klen, vptr, vlen);
	if (pair == QSE_NULL) return QSE_NULL; /* error */

	NEXT(pair) = htb->bucket[hc];
	htb->bucket[hc] = pair;
	htb->size++;

	return pair; /* new key added */
}

pair_t* qse_htb_upsert (
	htb_t* htb, void* kptr, size_t klen, void* vptr, size_t vlen)
{
	return insert (htb, kptr, klen, vptr, vlen, UPSERT);
}

pair_t* qse_htb_ensert (
	htb_t* htb, void* kptr, size_t klen, void* vptr, size_t vlen)
{
	return insert (htb, kptr, klen, vptr, vlen, ENSERT);
}

pair_t* qse_htb_insert (
	htb_t* htb, void* kptr, size_t klen, void* vptr, size_t vlen)
{
	return insert (htb, kptr, klen, vptr, vlen, INSERT);
}


pair_t* qse_htb_update (
	htb_t* htb, void* kptr, size_t klen, void* vptr, size_t vlen)
{
	return insert (htb, kptr, klen, vptr, vlen, UPDATE);
}

int qse_htb_delete (htb_t* htb, const void* kptr, size_t klen)
{
	pair_t* pair, * prev;
	size_t hc;

	hc = htb->hasher(htb,kptr,klen) % htb->capa;
	pair = htb->bucket[hc];
	prev = QSE_NULL;

	while (pair != QSE_NULL) 
	{
		if (htb->comper (htb, KPTR(pair), KLEN(pair), kptr, klen) == 0) 
		{
			if (prev == QSE_NULL) 
				htb->bucket[hc] = NEXT(pair);
			else NEXT(prev) = NEXT(pair);

			free_pair (htb, pair);
			htb->size--;

			return 0;
		}

		prev = pair;
		pair = NEXT(pair);
	}

	return -1;
}

void qse_htb_clear (htb_t* htb)
{
	size_t i;
	pair_t* pair, * next;

	for (i = 0; i < htb->capa; i++) 
	{
		pair = htb->bucket[i];

		while (pair != QSE_NULL) 
		{
			next = NEXT(pair);
			free_pair (htb, pair);
			htb->size--;
			pair = next;
		}

		htb->bucket[i] = QSE_NULL;
	}
}


void qse_htb_walk (htb_t* htb, walker_t walker, void* ctx)
{
	size_t i;
	pair_t* pair, * next;

	for (i = 0; i < htb->capa; i++) 
	{
		pair = htb->bucket[i];

		while (pair != QSE_NULL) 
		{
			next = NEXT(pair);
			if (walker(htb, pair, ctx) == QSE_HTB_WALK_STOP) return;
			pair = next;
		}
	}
}

pair_t* qse_htb_getfirstpair (htb_t* htb, size_t* buckno)
{
	size_t i;
	pair_t* pair;

	for (i = 0; i < htb->capa; i++)
	{
		pair = htb->bucket[i];
		if (pair != QSE_NULL) 
		{
			*buckno = i;
			return pair;
		}
	}

	return QSE_NULL;
}

pair_t* qse_htb_getnextpair (htb_t* htb, pair_t* pair, size_t* buckno)
{
	size_t i;
	pair_t* next;

	next = NEXT(pair);
	if (next != QSE_NULL) 
	{
		/* no change in bucket number */
		return next;
	}

	for (i = (*buckno)+1; i < htb->capa; i++)
	{
		pair = htb->bucket[i];
		if (pair != QSE_NULL) 
		{
			*buckno = i;
			return pair;
		}
	}

	return QSE_NULL;
}

static int reorganize (htb_t* htb)
{
	size_t i, hc, new_capa;
	pair_t** new_buck;

	if (htb->sizer)
	{
		new_capa = htb->sizer (htb, htb->capa + 1);

		/* if no change in capacity, return success 
		 * without reorganization */
		if (new_capa == htb->capa) return 0; 

		/* adjust to 1 if the new capacity is not reasonable */
		if (new_capa <= 0) new_capa = 1;
	}
	else
	{
		/* the bucket is doubled until it grows up to 65536 slots.
		 * once it has reached it, it grows by 65536 slots */
		new_capa = (htb->capa >= 65536)? (htb->capa + 65536): (htb->capa << 1);
	}

	new_buck = (pair_t**) QSE_MMGR_ALLOC (
		htb->mmgr, new_capa*SIZEOF(pair_t*));
	if (new_buck == QSE_NULL) 
	{
		/* reorganization is disabled once it fails */
		htb->threshold = 0;
		return -1;
	}

	/*for (i = 0; i < new_capa; i++) new_buck[i] = QSE_NULL;*/
	QSE_MEMSET (new_buck, 0, new_capa*SIZEOF(pair_t*));

	for (i = 0; i < htb->capa; i++)
	{
		pair_t* pair = htb->bucket[i];

		while (pair != QSE_NULL) 
		{
			pair_t* next = NEXT(pair);

			hc = htb->hasher (htb,
				KPTR(pair),
				KLEN(pair)) % new_capa;

			NEXT(pair) = new_buck[hc];
			new_buck[hc] = pair;

			pair = next;
		}
	}

	QSE_MMGR_FREE (htb->mmgr, htb->bucket);
	htb->bucket = new_buck;
	htb->capa = new_capa;
	htb->threshold = htb->capa * htb->factor / 100;

	return 0;
}
