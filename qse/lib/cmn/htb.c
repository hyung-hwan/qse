/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
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


#define htb_t           qse_htb_t
#define pair_t          qse_htb_pair_t
#define copier_t        qse_htb_copier_t
#define freeer_t        qse_htb_freeer_t
#define hasher_t        qse_htb_hasher_t
#define comper_t        qse_htb_comper_t
#define keeper_t        qse_htb_keeper_t
#define sizer_t         qse_htb_sizer_t
#define walker_t        qse_htb_walker_t
#define cbserter_t      qse_htb_cbserter_t
#define style_t        qse_htb_style_t
#define style_kind_t   qse_htb_style_kind_t

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

QSE_INLINE pair_t* qse_htb_allocpair (
	htb_t* htb, void* kptr, size_t klen, void* vptr, size_t vlen)
{
	pair_t* n;
	copier_t kcop, vcop;
	size_t as;

	kcop = htb->style->copier[QSE_HTB_KEY];
	vcop = htb->style->copier[QSE_HTB_VAL];

	as = SIZEOF(pair_t);
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
		/* if kptr is QSE_NULL, the inline copier does not fill
		 * the actual key area */
		if (kptr) QSE_MEMCPY (KPTR(n), kptr, KTOB(htb,klen));
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
		/* if vptr is QSE_NULL, the inline copier does not fill
		 * the actual value area */
		if (vptr) QSE_MEMCPY (VPTR(n), vptr, VTOB(htb,vlen));
	}
	else 
	{
		VPTR(n) = vcop (htb, vptr, vlen);
		if (VPTR(n) != QSE_NULL)
		{
			if (htb->style->freeer[QSE_HTB_KEY] != QSE_NULL)
				htb->style->freeer[QSE_HTB_KEY] (htb, KPTR(n), KLEN(n));
			QSE_MMGR_FREE (htb->mmgr, n);		
			return QSE_NULL;
		}
	}

	return n;
}

QSE_INLINE void qse_htb_freepair (htb_t* htb, pair_t* pair)
{
	if (htb->style->freeer[QSE_HTB_KEY] != QSE_NULL) 
		htb->style->freeer[QSE_HTB_KEY] (htb, KPTR(pair), KLEN(pair));
	if (htb->style->freeer[QSE_HTB_VAL] != QSE_NULL)
		htb->style->freeer[QSE_HTB_VAL] (htb, VPTR(pair), VLEN(pair));
	QSE_MMGR_FREE (htb->mmgr, pair);
}

static QSE_INLINE pair_t* change_pair_val (
	htb_t* htb, pair_t* pair, void* vptr, size_t vlen)
{
	if (VPTR(pair) == vptr && VLEN(pair) == vlen) 
	{
		/* if the old value and the new value are the same,
		 * it just calls the handler for this condition. 
		 * No value replacement occurs. */
		if (htb->style->keeper != QSE_NULL)
		{
			htb->style->keeper (htb, vptr, vlen);
		}
	}
	else
	{
		copier_t vcop = htb->style->copier[QSE_HTB_VAL];
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
				if (vptr) QSE_MEMCPY (VPTR(pair), vptr, VTOB(htb,vlen));
			}
			else
			{
				/* need to reconstruct the pair */
				pair_t* p = qse_htb_allocpair (htb, 
					KPTR(pair), KLEN(pair),
					vptr, vlen);
				if (p == QSE_NULL) return QSE_NULL;
				qse_htb_freepair (htb, pair);
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
		if (htb->style->freeer[QSE_HTB_VAL] != QSE_NULL) 
		{
			htb->style->freeer[QSE_HTB_VAL] (htb, ovptr, ovlen);
		}
	}


	return pair;
}

static style_t style[] =
{
    	/* == QSE_HTB_STYLE_DEFAULT == */
	{
		{
			QSE_HTB_COPIER_DEFAULT,
			QSE_HTB_COPIER_DEFAULT
		},
		{
			QSE_HTB_FREEER_DEFAULT,
			QSE_HTB_FREEER_DEFAULT
		},
		QSE_HTB_COMPER_DEFAULT,
		QSE_HTB_KEEPER_DEFAULT,
		QSE_HTB_SIZER_DEFAULT,
		QSE_HTB_HASHER_DEFAULT
	},

	/* == QSE_HTB_STYLE_INLINE_COPIERS == */
	{
		{
			QSE_HTB_COPIER_INLINE,
			QSE_HTB_COPIER_INLINE
		},
		{
			QSE_HTB_FREEER_DEFAULT,
			QSE_HTB_FREEER_DEFAULT
		},
		QSE_HTB_COMPER_DEFAULT,
		QSE_HTB_KEEPER_DEFAULT,
		QSE_HTB_SIZER_DEFAULT,
		QSE_HTB_HASHER_DEFAULT
	},

	/* == QSE_HTB_STYLE_INLINE_KEY_COPIER == */
	{
		{
			QSE_HTB_COPIER_INLINE,
			QSE_HTB_COPIER_DEFAULT
		},
		{
			QSE_HTB_FREEER_DEFAULT,
			QSE_HTB_FREEER_DEFAULT
		},
		QSE_HTB_COMPER_DEFAULT,
		QSE_HTB_KEEPER_DEFAULT,
		QSE_HTB_SIZER_DEFAULT,
		QSE_HTB_HASHER_DEFAULT
	},

	/* == QSE_HTB_STYLE_INLINE_VALUE_COPIER == */
	{
		{
			QSE_HTB_COPIER_DEFAULT,
			QSE_HTB_COPIER_INLINE
		},
		{
			QSE_HTB_FREEER_DEFAULT,
			QSE_HTB_FREEER_DEFAULT
		},
		QSE_HTB_COMPER_DEFAULT,
		QSE_HTB_KEEPER_DEFAULT,
		QSE_HTB_SIZER_DEFAULT,
		QSE_HTB_HASHER_DEFAULT
	}
};

const style_t* qse_gethtbstyle (style_kind_t kind)
{
	return &style[kind];
}

htb_t* qse_htb_open (
	mmgr_t* mmgr, size_t xtnsize, size_t capa, 
	int factor, int kscale, int vscale)
{
	htb_t* htb;

	htb = (htb_t*) QSE_MMGR_ALLOC (mmgr, SIZEOF(htb_t) + xtnsize);
	if (htb == QSE_NULL) return QSE_NULL;

	if (qse_htb_init (htb, mmgr, capa, factor, kscale, vscale) <= -1)
	{
		QSE_MMGR_FREE (mmgr, htb);
		return QSE_NULL;
	}

	QSE_MEMSET (htb + 1, 0, xtnsize);
	return htb;
}

void qse_htb_close (htb_t* htb)
{
	qse_htb_fini (htb);
	QSE_MMGR_FREE (htb->mmgr, htb);
}

int qse_htb_init (
	htb_t* htb, mmgr_t* mmgr, size_t capa,
	int factor, int kscale, int vscale)
{
	QSE_ASSERTX (capa > 0,
		"The initial capacity should be greater than 0. Otherwise, it is adjusted to 1 in the release mode");
	QSE_ASSERTX (factor >= 0 && factor <= 100,
		"The load factor should be between 0 and 100 inclusive. In the release mode, a value out of the range is adjusted to 100");

	QSE_ASSERT (kscale >= 0 && kscale <= QSE_TYPE_MAX(qse_byte_t));
	QSE_ASSERT (vscale >= 0 && vscale <= QSE_TYPE_MAX(qse_byte_t));

	/* some initial adjustment */
	if (capa <= 0) capa = 1;
	if (factor > 100) factor = 100;

	/* do not zero out the extension */
	QSE_MEMSET (htb, 0, SIZEOF(*htb));
	htb->mmgr = mmgr;

	htb->bucket = QSE_MMGR_ALLOC (mmgr, capa*SIZEOF(pair_t*));
	if (htb->bucket == QSE_NULL) return -1;

	/*for (i = 0; i < capa; i++) htb->bucket[i] = QSE_NULL;*/
	QSE_MEMSET (htb->bucket, 0, capa * SIZEOF(pair_t*));

	htb->factor = factor;
	htb->scale[QSE_HTB_KEY] = (kscale < 1)? 1: kscale;
	htb->scale[QSE_HTB_VAL] = (vscale < 1)? 1: vscale;

	htb->size = 0;
	htb->capa = capa;
	htb->threshold = htb->capa * htb->factor / 100;
	if (htb->capa > 0 && htb->threshold <= 0) htb->threshold = 1;

	htb->style = &style[0];
	return 0;
}

void qse_htb_fini (htb_t* htb)
{
	qse_htb_clear (htb);
	QSE_MMGR_FREE (htb->mmgr, htb->bucket);
}

qse_mmgr_t* qse_htb_getmmgr (qse_htb_t* htb)
{
	return htb->mmgr;
}

void* qse_htb_getxtn (qse_htb_t* htb)
{
	return QSE_XTN (htb);
}

const style_t* qse_htb_getstyle (const htb_t* htb)
{
	return htb->style;
}

void qse_htb_setstyle (htb_t* htb, const style_t* style)
{
	QSE_ASSERT (style != QSE_NULL);
	htb->style = style;
}

size_t qse_htb_getsize (const htb_t* htb)
{
	return htb->size;
}

size_t qse_htb_getcapa (const htb_t* htb)
{
	return htb->capa;
}

pair_t* qse_htb_search (const htb_t* htb, const void* kptr, size_t klen)
{
	pair_t* pair;
	size_t hc;

	hc = htb->style->hasher(htb,kptr,klen) % htb->capa;
	pair = htb->bucket[hc];

	while (pair != QSE_NULL) 
	{
		if (htb->style->comper (htb, KPTR(pair), KLEN(pair), kptr, klen) == 0)
		{
			return pair;
		}

		pair = NEXT(pair);
	}

	return QSE_NULL;
}

static QSE_INLINE_ALWAYS int reorganize (htb_t* htb)
{
	size_t i, hc, new_capa;
	pair_t** new_buck;

	if (htb->style->sizer)
	{
		new_capa = htb->style->sizer (htb, htb->capa + 1);

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

			hc = htb->style->hasher (htb,
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

/* insert options */
#define UPSERT 1
#define UPDATE 2
#define ENSERT 3
#define INSERT 4

static pair_t* insert (
	htb_t* htb, void* kptr, size_t klen, void* vptr, size_t vlen, int opt)
{
	pair_t* pair, * p, * prev, * next;
	size_t hc;

	hc = htb->style->hasher(htb,kptr,klen) % htb->capa;
	pair = htb->bucket[hc];
	prev = QSE_NULL;

	while (pair != QSE_NULL) 
	{
		next = NEXT(pair);

		if (htb->style->comper (htb, KPTR(pair), KLEN(pair), kptr, klen) == 0) 
		{
			/* found a pair with a matching key */
			switch (opt)
			{
				case UPSERT:
				case UPDATE:
					p = change_pair_val (htb, pair, vptr, vlen);
					if (p == QSE_NULL) 
					{
						/* error in changing the value */
						return QSE_NULL; 
					}
					if (p != pair) 
					{
						/* old pair destroyed. new pair reallocated.
						 * relink to include the new pair but to drop
						 * the old pair. */
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
		/* ingore reorganization error as it simply means
		 * more bucket collision and performance penalty. */
		if (reorganize(htb) == 0) 
		{
			hc = htb->style->hasher(htb,kptr,klen) % htb->capa;
		}
	}

	QSE_ASSERT (pair == QSE_NULL);

	pair = qse_htb_allocpair (htb, kptr, klen, vptr, vlen);
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

pair_t* qse_htb_cbsert (
	htb_t* htb, void* kptr, size_t klen, cbserter_t cbserter, void* ctx)
{
	pair_t* pair, * p, * prev, * next;
	size_t hc;

	hc = htb->style->hasher(htb,kptr,klen) % htb->capa;
	pair = htb->bucket[hc];
	prev = QSE_NULL;

	while (pair != QSE_NULL) 
	{
		next = NEXT(pair);

		if (htb->style->comper (htb, KPTR(pair), KLEN(pair), kptr, klen) == 0) 
		{
			/* found a pair with a matching key */
			p = cbserter (htb, pair, kptr, klen, ctx);
			if (p == QSE_NULL) 
			{
				/* error returned by the callback function */
				return QSE_NULL; 
			}
			if (p != pair) 
			{
				/* old pair destroyed. new pair reallocated.
				 * relink to include the new pair but to drop
				 * the old pair. */
				if (prev == QSE_NULL) 
					htb->bucket[hc] = p;
				else NEXT(prev) = p;
				NEXT(p) = next; 
			}
			return p;
		}

		prev = pair;
		pair = next;
	}

	if (htb->threshold > 0 && htb->size >= htb->threshold)
	{
		/* ingore reorganization error as it simply means
		 * more bucket collision and performance penalty. */
		if (reorganize(htb) == 0)
		{
			hc = htb->style->hasher(htb,kptr,klen) % htb->capa;
		}
	}

	QSE_ASSERT (pair == QSE_NULL);

	pair = cbserter (htb, QSE_NULL, kptr, klen, ctx);
	if (pair == QSE_NULL) return QSE_NULL; /* error */

	NEXT(pair) = htb->bucket[hc];
	htb->bucket[hc] = pair;
	htb->size++;

	return pair; /* new key added */
}

int qse_htb_delete (htb_t* htb, const void* kptr, size_t klen)
{
	pair_t* pair, * prev;
	size_t hc;

	hc = htb->style->hasher(htb,kptr,klen) % htb->capa;
	pair = htb->bucket[hc];
	prev = QSE_NULL;

	while (pair != QSE_NULL) 
	{
		if (htb->style->comper (htb, KPTR(pair), KLEN(pair), kptr, klen) == 0) 
		{
			if (prev == QSE_NULL) 
				htb->bucket[hc] = NEXT(pair);
			else NEXT(prev) = NEXT(pair);

			qse_htb_freepair (htb, pair);
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
			qse_htb_freepair (htb, pair);
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

size_t qse_htb_dflhash (
	const htb_t* htb, const void* kptr, size_t klen)
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

int qse_htb_dflcomp (
	const htb_t* htb, 
	const void* kptr1, size_t klen1, 
	const void* kptr2, size_t klen2)
{
	if (klen1 == klen2) return QSE_MEMCMP (kptr1, kptr2, KTOB(htb,klen1));
	/* it just returns 1 to indicate that they are different. */
	return 1;
}

