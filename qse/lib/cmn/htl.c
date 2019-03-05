/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * hash.c Non-thread-safe split-ordered hash table.
 *
 *  The weird "reverse" function is based on an idea from
 *  "Split-Ordered Lists - Lock-free Resizable Hash Tables", with
 *  modifications so that they're not lock-free. :(
 *
 *  However, the split-order idea allows a fast & easy splitting of the
 *  hash bucket chain when the hash table is resized.  Without it, we'd
 *  have to check & update the pointers for every node in the buck chain,
 *  rather than being able to move 1/2 of the entries in the chain with
 *  one update.
 *
 * Version:	$Id$
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 *  Copyright 2005,2006  The FreeRADIUS server project
 */


#include <qse/cmn/htl.h>
#include <qse/cmn/chr.h>
#include "mem-prv.h"

/*
 *	A reasonable number of buckets to start off with.
 *	Should be a power of two.
 */
#define QSE_HTL_NUM_BUCKETS (64)


/*
 * perl -e 'foreach $i (0..255) {$r = 0; foreach $j (0 .. 7 ) { if (($i & ( 1<< $j)) != 0) { $r |= (1 << (7 - $j));}} print $r, ", ";if (($i & 7) == 7) {print "\n";}}'
 */
static const qse_uint8_t reversed_byte[256] = 
{
	0,  128, 64, 192, 32, 160, 96,  224,
	16, 144, 80, 208, 48, 176, 112, 240,
	8,  136, 72, 200, 40, 168, 104, 232,
	24, 152, 88, 216, 56, 184, 120, 248,
	4,  132, 68, 196, 36, 164, 100, 228,
	20, 148, 84, 212, 52, 180, 116, 244,
	12, 140, 76, 204, 44, 172, 108, 236,
	28, 156, 92, 220, 60, 188, 124, 252,
	2,  130, 66, 194, 34, 162, 98,  226,
	18, 146, 82, 210, 50, 178, 114, 242,
	10, 138, 74, 202, 42, 170, 106, 234,
	26, 154, 90, 218, 58, 186, 122, 250,
	6,  134, 70, 198, 38, 166, 102, 230,
	22, 150, 86, 214, 54, 182, 118, 246,
	14, 142, 78, 206, 46, 174, 110, 238,
	30, 158, 94, 222, 62, 190, 126, 254,
	1,  129, 65, 193, 33, 161, 97,  225,
	17, 145, 81, 209, 49, 177, 113, 241,
	9,  137, 73, 201, 41, 169, 105, 233,
	25, 153, 89, 217, 57, 185, 121, 249,
	5,  133, 69, 197, 37, 165, 101, 229,
	21, 149, 85, 213, 53, 181, 117, 245,
	13, 141, 77, 205, 45, 173, 109, 237,
	29, 157, 93, 221, 61, 189, 125, 253,
	3,  131, 67, 195, 35, 163, 99,  227,
	19, 147, 83, 211, 51, 179, 115, 243,
	11, 139, 75, 203, 43, 171, 107, 235,
	27, 155, 91, 219, 59, 187, 123, 251,
	7,  135, 71, 199, 39, 167, 103, 231,
	23, 151, 87, 215, 55, 183, 119, 247,
	15, 143, 79, 207, 47, 175, 111, 239,
	31, 159, 95, 223, 63, 191, 127, 255
};


/*
 * perl -e 'foreach $i (0..255) {$r = 0;foreach $j (0 .. 7) { $r = $i & (1 << (7 - $j)); last if ($r)} print $i & ~($r), ", ";if (($i & 7) == 7) {print "\n";}}'
 */
static qse_uint8_t parent_byte[256] = 
{
	0, 0, 0, 1, 0, 1, 2, 3,
	0, 1, 2, 3, 4, 5, 6, 7,
	0, 1, 2, 3, 4, 5, 6, 7,
	8, 9, 10, 11, 12, 13, 14, 15,
	0, 1, 2, 3, 4, 5, 6, 7,
	8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23,
	24, 25, 26, 27, 28, 29, 30, 31,
	0, 1, 2, 3, 4, 5, 6, 7,
	8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23,
	24, 25, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39,
	40, 41, 42, 43, 44, 45, 46, 47,
	48, 49, 50, 51, 52, 53, 54, 55,
	56, 57, 58, 59, 60, 61, 62, 63,
	0, 1, 2, 3, 4, 5, 6, 7,
	8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23,
	24, 25, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39,
	40, 41, 42, 43, 44, 45, 46, 47,
	48, 49, 50, 51, 52, 53, 54, 55,
	56, 57, 58, 59, 60, 61, 62, 63,
	64, 65, 66, 67, 68, 69, 70, 71,
	72, 73, 74, 75, 76, 77, 78, 79,
	80, 81, 82, 83, 84, 85, 86, 87,
	88, 89, 90, 91, 92, 93, 94, 95,
	96, 97, 98, 99, 100, 101, 102, 103,
	104, 105, 106, 107, 108, 109, 110, 111,
	112, 113, 114, 115, 116, 117, 118, 119,
	120, 121, 122, 123, 124, 125, 126, 127
};


/*
 *	Reverse a key.
 */
static qse_uint32_t reverse (qse_uint32_t key)
{
	return ((reversed_byte[key & 0xff] << 24) |
		(reversed_byte[(key >> 8) & 0xff] << 16) |
		(reversed_byte[(key >> 16) & 0xff] << 8) |
		(reversed_byte[(key >> 24) & 0xff]));
}

/*
 *	Take the parent by discarding the highest bit that is set.
 */
static qse_uint32_t parent_of (qse_uint32_t key)
{
	if (key > 0x00ffffff)
		return (key & 0x00ffffff) | (parent_byte[key >> 24] << 24);

	if (key > 0x0000ffff)
		return (key & 0x0000ffff) | (parent_byte[key >> 16] << 16);

	if (key > 0x000000ff)
		return (key & 0x000000ff) | (parent_byte[key >> 8] << 8);

	return parent_byte[key];
}


static qse_htl_node_t* __list_find (qse_htl_t* ht, qse_htl_node_t* head, qse_uint32_t reversed, const void* data, qse_htl_comper_t comper)
{
	qse_htl_node_t *cur;

	if (!/*ht->*/comper) return QSE_NULL;

	for (cur = head; cur != &ht->null; cur = cur->next) 
	{
		if (cur->reversed == reversed) 
		{
			int cmp = /*ht->*/comper(ht, data, cur->data);
			if (cmp > 0) break;
			if (cmp < 0) continue;
			return cur;
		}
		if (cur->reversed > reversed) break;
	}

	return QSE_NULL;
}


QSE_INLINE static qse_htl_node_t *list_find (qse_htl_t* ht, qse_htl_node_t* head, qse_uint32_t reversed, const void* data)
{
	return __list_find (ht, head, reversed, data, ht->comper);
}

/*
 *	Inserts a new entry into the list, in order.
 */
static int list_insert (qse_htl_t* ht, qse_htl_node_t** head, qse_htl_node_t* node)
{
	qse_htl_node_t **last, *cur;

	last = head;

	for (cur = *head; cur != &ht->null; cur = cur->next) 
	{
		if (cur->reversed > node->reversed) break;
		last = &(cur->next);

		if (cur->reversed == node->reversed) 
		{
			if (ht->comper) 
			{
				int cmp = ht->comper(ht, node->data, cur->data);
				if (cmp > 0) break;
				if (cmp < 0) continue;
			}
			return 0;
		}
	}

	node->next = *last;
	*last = node;

	return 1;
}


/*
 *	Delete an entry from the list.
 */
static int list_delete (qse_htl_t* ht, qse_htl_node_t** head, qse_htl_node_t* node)
{
	qse_htl_node_t **last, *cur;

	last = head;

	for (cur = *head; cur != &ht->null; cur = cur->next) 
	{
		if (cur == node) break;
		last = &(cur->next);
	}

	*last = node->next;
	return 1;
}

/* ------------------------------------------------------------------------- */


static QSE_INLINE_ALWAYS qse_uint32_t default_hasher (qse_htl_t* htl, const void* data)
{
#if 0
	qse_size_t h = 5381;
	const qse_byte_t* p = (const qse_byte_t*)data;
	const qse_byte_t* bound = p + htl->keysize;
	while (p < bound) h = ((h << 5) + h) + *p++;
	return h ; 
#else
	return qse_genhash32 (data, htl->keysize);
#endif
}

static QSE_INLINE_ALWAYS int default_comper (qse_htl_t* htl, const void* data1, const void* data2)
{
	return QSE_MEMCMP(data1, data2, htl->keysize);
}

qse_htl_t* qse_htl_open (qse_mmgr_t* mmgr, qse_size_t xtnsize, int keysize)
{
	qse_htl_t* htl;

	htl = QSE_MMGR_ALLOC(mmgr, QSE_SIZEOF(qse_htl_t) + xtnsize);
	if (htl == QSE_NULL) return QSE_NULL;

	if (qse_htl_init(htl, mmgr, keysize) <= -1)
	{
		QSE_MMGR_FREE (mmgr, htl);
		return QSE_NULL;
	}

	QSE_MEMSET (htl + 1, 0, xtnsize);
	return htl;
}

void qse_htl_close (qse_htl_t* htl)
{
	qse_htl_fini (htl);
	QSE_MMGR_FREE (htl->mmgr, htl);
}

/*
 *	Create the table.
 *
 *	Memory usage in bytes is (20/3) * number of entries.
 */
int qse_htl_init (qse_htl_t* ht, qse_mmgr_t* mmgr, int keysize)
{
	QSE_MEMSET (ht, 0, sizeof(*ht));
	ht->mmgr = mmgr;
	ht->keysize = keysize;

	ht->hasher = default_hasher;
	ht->comper = default_comper;
	ht->freeer = QSE_NULL;
	ht->copier = QSE_NULL;
	ht->num_buckets = QSE_HTL_NUM_BUCKETS;
	ht->mask = ht->num_buckets - 1;

	/*
	 *	Have a default load factor of 2.5.  In practice this
	 *	means that the average load will hit 3 before the
	 *	table grows.
	 */
	ht->next_grow = (ht->num_buckets << 1) + (ht->num_buckets >> 1);

	ht->buckets = QSE_MMGR_ALLOC (ht->mmgr, QSE_SIZEOF(*ht->buckets) * ht->num_buckets);
	if (!ht->buckets) return -1;

	QSE_MEMSET (ht->buckets, 0, sizeof(*ht->buckets) * ht->num_buckets);

	ht->null.reversed = ~0;
	ht->null.key = ~0;
	ht->null.next = &ht->null;

	ht->buckets[0] = &ht->null;

	return 0;
}

void qse_htl_fini (qse_htl_t* ht)
{
	qse_htl_clear (ht);
	QSE_MMGR_FREE (ht->mmgr, ht->buckets);
}

void qse_htl_clear (qse_htl_t* ht)
{
	int i;
	qse_htl_node_t* node, * next;

	/*
	 *	Walk over the buckets, freeing them all.
	 */
	for (i = 0; i < ht->num_buckets; i++) 
	{
		if (ht->buckets[i]) 
		{
			for (node = ht->buckets[i]; node != &ht->null; node = next) 
			{
				next = node->next;

				if (!node->data) continue; /* dummy entry */

				if (ht->freeer) ht->freeer (ht, node->data);
				QSE_MMGR_FREE (ht->mmgr, node);
			}
		}
	}

	QSE_MEMSET (ht->buckets, 0, sizeof(*ht->buckets) * ht->num_buckets);
	ht->buckets[0] = &ht->null;
	ht->num_elements = 0;
}
/* ------------------------------------------------------------------------- */

/*
 *	If the current bucket is uninitialized, initialize it
 *	by recursively copying information from the parent.
 *
 *	We may have a situation where entry E is a parent to 2 other
 *	entries E' and E".  If we split E into E and E', then the
 *	nodes meant for E" end up in E or E', either of which is
 *	wrong.  To solve that problem, we walk down the whole chain,
 *	inserting the elements into the correct place.
 */
static void fixup (qse_htl_t *ht, qse_uint32_t entry)
{
	qse_uint32_t parent_entry;
	qse_htl_node_t** last, * cur;
	qse_uint32_t thiss;

	parent_entry = parent_of(entry);

	/* parent_entry == entry if and only if entry == 0 */

	if (!ht->buckets[parent_entry]) 
	{
		fixup(ht, parent_entry);
	}

	/*
	 *	Keep walking down cur, trying to find entries that
	 *	don't belong here any more.  There may be multiple
	 *	ones, so we can't have a naive algorithm...
	 */
	last = &ht->buckets[parent_entry];
	thiss = parent_entry;

	for (cur = *last; cur != &ht->null; cur = cur->next) 
	{
		qse_uint32_t real_entry;

		real_entry = cur->key & ht->mask;
		if (real_entry != thiss) 
		{ /* ht->buckets[real_entry] == QSE_NULL */
			*last = &ht->null;
			ht->buckets[real_entry] = cur;
			thiss = real_entry;
		}

		last = &(cur->next);
	}

	/*
	 *	We may NOT have initialized this bucket, so do it now.
	 */
	if (!ht->buckets[entry]) ht->buckets[entry] = &ht->null;
}

/*
 *	This should be a power of two.  Changing it to 4 doesn't seem
 *	to make any difference.
 */
#define GROW_FACTOR (2)

/*
 *	Grow the hash table.
 */
static void grow (qse_htl_t*ht)
{
	qse_htl_node_t **buckets;

	buckets = QSE_MMGR_ALLOC (ht->mmgr, QSE_SIZEOF(*buckets) * GROW_FACTOR * ht->num_buckets);
	if (!buckets) return;

	QSE_MEMCPY (buckets, ht->buckets, QSE_SIZEOF(*buckets) * ht->num_buckets);
	QSE_MEMSET (&buckets[ht->num_buckets], 0, QSE_SIZEOF(*buckets) * ht->num_buckets);

	QSE_MMGR_FREE (ht->mmgr, ht->buckets);
	ht->buckets = buckets;
	ht->num_buckets *= GROW_FACTOR;
	ht->next_grow *= GROW_FACTOR;
	ht->mask = ht->num_buckets - 1;
}

qse_htl_node_t *qse_htl_search (qse_htl_t* ht, const void* data)
{
	qse_uint32_t key;
	qse_uint32_t entry;
	qse_uint32_t reversed;

	key = ht->hasher(ht, data);
	entry = key & ht->mask;
	reversed = reverse(key);

	if (!ht->buckets[entry]) fixup(ht, entry);
	return list_find(ht, ht->buckets[entry], reversed, data);
}

qse_htl_node_t *qse_htl_heterosearch (qse_htl_t* ht, const void* data, qse_htl_hasher_t hasher, qse_htl_comper_t comper)
{
	qse_uint32_t key;
	qse_uint32_t entry;
	qse_uint32_t reversed;

	key = /*ht->*/hasher(ht, data);
	entry = key & ht->mask;
	reversed = reverse(key);

	if (!ht->buckets[entry]) fixup(ht, entry);
	return __list_find(ht, ht->buckets[entry], reversed, data, comper);
}

/*
 *	Insert data.
 */
qse_htl_node_t* qse_htl_insert (qse_htl_t* ht, void* data)
{
	qse_uint32_t key;
	qse_uint32_t entry;
	qse_uint32_t reversed;
	qse_htl_node_t *node;

	key = ht->hasher(ht, data);
	entry = key & ht->mask;
	reversed = reverse(key);

	if (!ht->buckets[entry]) fixup(ht, entry);

	/*
	 *	If we try to do our own memory allocation here, the
	 *	speedup is only ~15% or so, which isn't worth it.
	 */
	node = QSE_MMGR_ALLOC(ht->mmgr, QSE_SIZEOF(*node));
	if (!node) return QSE_NULL;
	QSE_MEMSET (node, 0, QSE_SIZEOF(*node));

	node->next = &ht->null;
	node->reversed = reversed;
	node->key = key;

	if (ht->copier) 
	{
		node->data = ht->copier (ht, data);
		if (!node->data)
		{
			QSE_MMGR_FREE (ht->mmgr, node);
			return QSE_NULL;
		}
	}
	else node->data = data;

	/* already in the table, can't insert it */
	if (!list_insert(ht, &ht->buckets[entry], node)) 
	{
		if (ht->freeer) ht->freeer (ht, node->data);
		QSE_MMGR_FREE (ht->mmgr, node);
		return QSE_NULL;
	}

	/*
	 *	Check the load factor, and grow the table if
	 *	necessary.
	 */
	ht->num_elements++;
	if (ht->num_elements >= ht->next_grow) grow(ht);

	return node;
}

/*
 *	Replace old data with new data, OR insert if there is no old.
 */
qse_htl_node_t* qse_htl_upsert (qse_htl_t* ht, void* data)
{
	qse_htl_node_t *node;
	void* datap;

	node = qse_htl_search(ht, data);
	if (!node) return qse_htl_insert(ht, data);

	if (ht->copier) 
	{
		datap = ht->copier (ht, data);
		if (!datap) return QSE_NULL;
	}
	else datap = data;

	if (ht->freeer) ht->freeer(ht, node->data);
	node->data = datap;

	return node;
}

qse_htl_node_t* qse_htl_update (qse_htl_t* ht, void* data)
{
	qse_htl_node_t *node;
	void* datap;

	node = qse_htl_search(ht, data);
	if (!node) return QSE_NULL;

	if (ht->copier) 
	{
		datap = ht->copier (ht, data);
		if (!datap) return QSE_NULL;
	}
	else datap = data;

	if (ht->freeer) ht->freeer(ht, node->data);
	node->data = datap;

	return node;
}

qse_htl_node_t* qse_htl_upyank (qse_htl_t* ht, void* data, void** olddata)
{
	qse_htl_node_t* node;
	void* datap;

	node = qse_htl_search(ht, data);
	if (!node) return QSE_NULL;

	if (ht->copier) 
	{
		datap = ht->copier (ht, data);
		if (!datap) return QSE_NULL;
	}
	else datap = data;

	*olddata = node->data;
	node->data = datap;

	return node;
}

qse_htl_node_t* qse_htl_ensert (qse_htl_t* ht, void* data)
{
	qse_htl_node_t* node;

	node = qse_htl_search(ht, data);
	if (!node) node = qse_htl_insert(ht, data);

	return node;
}


qse_htl_node_t* qse_htl_yanknode (qse_htl_t* ht, void* data)
{
	qse_uint32_t key;
	qse_uint32_t entry;
	qse_uint32_t reversed;
	qse_htl_node_t* node;

	key = ht->hasher(ht, data);
	entry = key & ht->mask;
	reversed = reverse(key);

	if (!ht->buckets[entry]) fixup(ht, entry);

	node = list_find(ht, ht->buckets[entry], reversed, data);
	if (!node) return QSE_NULL;

	list_delete(ht, &ht->buckets[entry], node);
	ht->num_elements--;

	return node;
}


/*
 *	Yank an entry from the hash table, without freeing the data.
 */
void* qse_htl_yank (qse_htl_t* ht, void* data)
{
	qse_htl_node_t* node;
	void* old;

	node = qse_htl_yanknode (ht, data);
	if (!node) return QSE_NULL;

	old = node->data;
	QSE_MMGR_FREE (ht->mmgr, node);
	return old;
}

/*
 *	Delete a piece of data from the hash table.
 */
int qse_htl_delete(qse_htl_t *ht, void* data)
{
	void* old;

	old = qse_htl_yank(ht, data);
	if (!old) return -1;

	if (ht->freeer) ht->freeer(ht, old);

	return 0;
}

/*
 *	Walk over the nodes, allowing deletes & inserts to happen.
 */
void qse_htl_walk (qse_htl_t *ht, qse_htl_walker_t walker, void *ctx)
{
	int i;

	for (i = ht->num_buckets - 1; i >= 0; i--) 
	{
		qse_htl_node_t* node, * next;

		/*
		 *	Ensure that the current bucket is filled.
		 */
		if (!ht->buckets[i]) fixup(ht, i);

		for (node = ht->buckets[i]; node != &ht->null; node = next) 
		{
			next = node->next;
			if (walker(ht, node->data, ctx) == QSE_HTL_WALK_STOP) return;
		}
	}
}
/* ------------------------------------------------------------------------- */


#if 0
/*
 *	Find data from a template
 */
void *qse_htl_finddata(qse_htl_t *ht, const void *data)
{
	qse_htl_node_t *node;

	node = qse_htl_find(ht, data);
	if (!node) return NULL;

	return node->data;
}


#ifdef TESTING
/*
 *	Show what the hash table is doing.
 */
int qse_htl_info(qse_htl_t *ht)
{
	int i, a, collisions, uninitialized;
	int array[256];

	if (!ht) return 0;

	uninitialized = collisions = 0;
	memset(array, 0, sizeof(array));

	for (i = 0; i < ht->num_buckets; i++) {
		qse_uint32_t key;
		int load;
		qse_htl_node_t *node, *next;

		/*
		 *	If we haven't inserted or looked up an entry
		 *	in a bucket, it's uninitialized.
		 */
		if (!ht->buckets[i]) {
			uninitialized++;
			continue;
		}

		load = 0;
		key = ~0;
		for (node = ht->buckets[i]; node != &ht->null; node = next) {
			if (node->reversed == key) {
				collisions++;
			} else {
				key = node->reversed;
			}
			next = node->next;
			load++;
		}

		if (load > 255) load = 255;
		array[load]++;
	}

	printf("HASH TABLE %p\tbuckets: %d\t(%d uninitialized)\n", ht,
		ht->num_buckets, uninitialized);
	printf("\tnum entries %d\thash collisions %d\n",
		ht->num_elements, collisions);

	a = 0;
	for (i = 1; i < 256; i++) {
		if (!array[i]) continue;
		printf("%d\t%d\n", i, array[i]);

		/*
		 *	Since the entries are ordered, the lookup cost
		 *	for any one element in a chain is (on average)
		 *	the cost of walking half of the chain.
		 */
		if (i > 1) {
			a += array[i] * i;
		}
	}
	a /= 2;
	a += array[1];

	printf("\texpected lookup cost = %d/%d or %f\n\n",
	       ht->num_elements, a,
	       (float) ht->num_elements / (float) a);

	return 0;
}
#endif


#endif




/* ------------------------------------------------------------------------- */


#define FNV_MAGIC_INIT (0x811c9dc5)
#define FNV_MAGIC_PRIME (0x01000193)

/*
 *	A fast hash function.  For details, see:
 *
 *	http://www.isthe.com/chongo/tech/comp/fnv/
 *
 *	Which also includes public domain source.  We've re-written
 *	it here for our purposes.
 */

/*
 *	Continue hashing data.
 */
QSE_INLINE qse_uint32_t qse_genhash32_update (const void* data, qse_size_t size, qse_uint32_t hash)
{
	const qse_uint8_t *p = data;
	const qse_uint8_t *q = p + size;

	/*
	 *	FNV-1 hash each octet in the buffer
	 */
	while (p != q) 
	{
		/*
		 *	XOR the 8-bit quantity into the bottom of
		 *	the hash.
		 */
		hash ^= (qse_uint32_t) (*p++);

		/*
		 *	Multiple by 32-bit magic FNV prime, mod 2^32
		 */
		hash *= FNV_MAGIC_PRIME;
	#if 0
		/*
		 *	Potential optimization.
		 */
		hash += (hash<<1) + (hash<<4) + (hash<<7) + (hash<<8) + (hash<<24);
	#endif
	}

	return hash;
}

qse_uint32_t qse_genhash32 (const void *data, qse_size_t size)
{
	return qse_genhash32_update(data, size, FNV_MAGIC_INIT);
}

/*
 *	Hash a C string, so we loop over it once.
 */
qse_uint32_t qse_mbshash32 (const qse_mchar_t* p)
{
	qse_uint32_t hash = FNV_MAGIC_INIT;

	while (*p) 
	{
		hash ^= (qse_uint32_t)(*p++);
		hash *= FNV_MAGIC_PRIME;
	}

	return hash;
}

qse_uint32_t qse_wcshash32 (const qse_wchar_t* p)
{

	qse_uint32_t hash = FNV_MAGIC_INIT;

	while (*p) 
	{
	#if (QSE_SIZEOF_WCHAR_T <= QSE_SIZEOF_UINT32_T)
		hash ^= (qse_uint32_t)(*p);
		hash *= FNV_MAGIC_PRIME;
	#else
		hash = qse_genhash_update(*p, QSE_SIZEOF(*p), hash);
	#endif
		p++;
	}

	return hash;
}

qse_uint32_t qse_mbscasehash32 (const qse_mchar_t* p)
{
	qse_uint32_t hash = FNV_MAGIC_INIT;

	while (*p) 
	{
		qse_mchar_t mc = *p++;
		mc = QSE_TOMLOWER(mc);
		hash ^= (qse_uint32_t)mc;
		hash *= FNV_MAGIC_PRIME;
	}

	return hash;
}

qse_uint32_t qse_wcscasehash32 (const qse_wchar_t* p)
{
	qse_uint32_t hash = FNV_MAGIC_INIT;

	while (*p) 
	{
		qse_wchar_t wc = *p++;
		wc = QSE_TOWLOWER(wc);
	#if (QSE_SIZEOF_WCHAR_T <= QSE_SIZEOF_UINT32_T)
		hash ^= (qse_uint32_t)(wc);
		hash *= FNV_MAGIC_PRIME;
	#else
		hash = qse_genhash_update (&wc, QSE_SIZEOF(wc), hash);
	#endif
	}

	return hash;
}

#if 0
/*
 *	Return a "folded" hash, where the lower "bits" are the
 *	hash, and the upper bits are zero.
 *
 *	If you need a non-power-of-two hash, cope.
 */
qse_uint32_t qse_foldhash32 (qse_uint32_t hash, int bits)
{
	int count;
	qse_uint32_t result;

	if ((bits <= 0) || (bits >= 32)) return hash;

	result = hash;

	/*
	 *	Never use the same bits twice in an xor.
	 */
	for (count = 0; count < 32; count += bits) 
	{
		hash >>= bits;
		result ^= hash;
	}

	return result & (((qse_uint32_t) (1 << bits)) - 1);
}
#endif
