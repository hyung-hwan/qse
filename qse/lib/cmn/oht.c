#include <qse/cmn/oht.h>
#include "mem.h"

#define DATA_PTR(oht,index) \
	((void*)(((qse_byte_t*)(oht)->data) + ((index) * (oht)->scale)))

QSE_IMPLEMENT_COMMON_FUNCTIONS (oht)

static QSE_INLINE_ALWAYS qse_size_t default_hasher (
	qse_oht_t* oht, const void* data)
{
	qse_size_t h = 5381;
	const qse_byte_t* p = (const qse_byte_t*)data;
	const qse_byte_t* bound = p + oht->scale;
	while (p < bound) h = ((h << 5) + h) + *p++;
	return h ; 
}

static QSE_INLINE_ALWAYS int default_comper (
	qse_oht_t* oht, const void* data1, const void* data2)
{
	return QSE_MEMCMP(data1, data2, oht->scale);
}

static QSE_INLINE_ALWAYS void default_copier (
	qse_oht_t* oht, void* dst, const void* src)
{
	QSE_MEMCPY (dst, src, oht->scale);
}

#define HASH_DATA(oht,data) \
		((oht)->hasher? (oht)->hasher ((oht), (data)): \
		                default_hasher (oht, data))

#define COMP_DATA(oht,d1,d2) \
		((oht)->comper? (oht)->comper ((oht), (d1), (d2)): \
		                QSE_MEMCMP ((d1), (d2), (oht)->scale))

#define COPY_DATA(oht,dst,src) \
	QSE_BLOCK ( \
		if ((oht)->copier) (oht)->copier ((oht), (dst), (src)); \
		else QSE_MEMCPY ((dst), (src), (oht)->scale); \
	) 

qse_oht_t* qse_oht_open (
	qse_mmgr_t* mmgr, qse_size_t xtnsize,
	int scale, qse_size_t capa, qse_size_t limit) 
{
	qse_oht_t* oht;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	oht = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_oht_t) + xtnsize);
	if (oht == QSE_NULL) return QSE_NULL;

	if (qse_oht_init (oht, mmgr, scale, capa, limit) <= -1)
	{
		QSE_MMGR_FREE (mmgr, oht);
		return QSE_NULL;
	}

	return oht;
}

void qse_oht_close (qse_oht_t* oht)
{
	qse_oht_fini (oht);
	QSE_MMGR_FREE (oht->mmgr, oht);
}

int qse_oht_init (
	qse_oht_t* oht, qse_mmgr_t* mmgr,
	int scale, qse_size_t capa, qse_size_t limit) 
{
	qse_size_t i;

	if (scale <= 0) scale = 1;
	if (capa >= QSE_OHT_NIL - 1) capa = QSE_OHT_NIL - 1;
	if (limit > capa || limit <= 0) limit = capa;

	QSE_MEMSET (oht, 0, QSE_SIZEOF(*oht));

	oht->mmgr = mmgr;
	oht->capa.hard = capa;
	oht->capa.soft = limit;
	oht->scale = scale;
	oht->size = 0;

	/*oht->hasher = default_hasher;
	oht->comper = default_comper;
	oht->copier = default_copier;*/

	oht->mark = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_oht_mark_t) * capa);
	if (!oht->mark) return -1;

	oht->data = QSE_MMGR_ALLOC (mmgr, scale * capa);
	if (!oht->data)
	{
		QSE_MMGR_FREE (mmgr, oht->mark);
		return -1;
	}

	for (i = 0; i < capa; i++) oht->mark[i] = QSE_OHT_EMPTY;
	return 0;
}

void qse_oht_fini (qse_oht_t* oht)
{
	QSE_MMGR_FREE (oht->mmgr, oht->mark);
	QSE_MMGR_FREE (oht->mmgr, oht->data);
	oht->size = 0;
}

qse_oht_hasher_t qse_oht_gethasher (qse_oht_t* oht)
{
	return oht->hasher? oht->hasher: default_hasher;
}

void qse_oht_sethasher (qse_oht_t* oht, qse_oht_hasher_t hasher)
{
	oht->hasher = hasher;
}

qse_oht_comper_t qse_oht_getcomper (qse_oht_t* oht)
{
	return oht->comper? oht->comper: default_comper;
}

void qse_oht_setcomper (qse_oht_t* oht, qse_oht_comper_t comper)
{
	oht->comper = comper;
}

qse_oht_copier_t qse_oht_getcopier (qse_oht_t* oht)
{
	return oht->copier? oht->copier: default_copier;
}

void qse_oht_setcopier (qse_oht_t* oht, qse_oht_copier_t copier)
{
	oht->copier = copier;
}

static QSE_INLINE qse_size_t search (
	qse_oht_t* oht, const void* data, qse_size_t hash)
{
	qse_size_t i;

	for (i = 0; i < oht->capa.hard; i++)
	{
		qse_size_t index = (hash + i) % oht->capa.hard;
		if (oht->mark[index] == QSE_OHT_EMPTY) break;
		if (oht->mark[index] == QSE_OHT_OCCUPIED)
		{
			if (COMP_DATA (oht, data, DATA_PTR(oht,index)) == 0) 
				return index;
		}
	}

	return QSE_OHT_NIL;
}

qse_size_t qse_oht_search (qse_oht_t* oht, void* data)
{
	qse_size_t i = search (oht, data, HASH_DATA(oht,data));
	if (i != QSE_OHT_NIL && data) 
		COPY_DATA (oht, data, DATA_PTR(oht,i));
	return i;
}

qse_size_t qse_oht_update (qse_oht_t* oht, const void* data)
{
	qse_size_t i = search (oht, data, HASH_DATA(oht,data));
	if (i != QSE_OHT_NIL) 
		COPY_DATA (oht, DATA_PTR(oht,i), data);
	return i;
}

qse_size_t qse_oht_upsert (qse_oht_t* oht, const void* data)
{
	qse_size_t i, hash = HASH_DATA (oht, data);

	/* find the existing item */
	i = search (oht, data, hash);
	if (i != QSE_OHT_NIL)
	{
		COPY_DATA (oht, DATA_PTR(oht,i), data);
		return i;
	}

	/* check if there is a free slot to insert data into */
	if (oht->size >= oht->capa.soft) return QSE_OHT_NIL;

	/* get the unoccupied slot and insert the data into it.
	 * iterate at most 'the number of items (oht->size)' times + 1. */
	for (i = 0; i <= oht->size; i++) 
	{
		qse_size_t index = (hash + i) % oht->capa.hard;
		if (oht->mark[index] != QSE_OHT_OCCUPIED) 
		{
			oht->mark[index] = QSE_OHT_OCCUPIED;
			COPY_DATA (oht, DATA_PTR(oht,index), data);
			oht->size++;
			return index;
		}
	}

	return QSE_OHT_NIL;
}

qse_size_t qse_oht_insert (qse_oht_t* oht, const void* data)
{
	qse_size_t i, hash;

	/* check if there is a free slot to insert data into */
	if (oht->size >= oht->capa.soft) return QSE_OHT_NIL;

	hash = HASH_DATA (oht, data);

	/* check if the item already exits */
	i = search (oht, data, hash);
	if (i != QSE_OHT_NIL) return QSE_OHT_NIL;

	/* get the unoccupied slot and insert the data into it.
	 * iterate at most 'the number of items (oht->size)' times + 1. */
	for (i = 0; i <= oht->size; i++) 
	{
		qse_size_t index = (hash + i) % oht->capa.hard;
		if (oht->mark[index] != QSE_OHT_OCCUPIED) 
		{
			oht->mark[index] = QSE_OHT_OCCUPIED;
			COPY_DATA (oht, DATA_PTR(oht,index), data);
			oht->size++;
			return index;
		}
	}

	return QSE_OHT_NIL;
}

qse_size_t qse_oht_delete (qse_oht_t* oht, const void* data)
{
#if 0
	qse_size_t index;

	if (oht->size <= 0) return QSE_OHT_NIL; 

	index = search (oht, data, HASH_DATA(oht,data));
	if (index != QSE_OHT_NIL)
	{
		oht->mark[index] = QSE_OHT_DELETED;
		oht->size--;
	}

	return index;
#endif

	qse_size_t index, i, x, y, z;

	/* check if the oht is empty. if so, do nothing */
	if (oht->size <= 0) return QSE_OHT_NIL; 

	/* check if the item exists. otherwise, do nothing. */
	index = search (oht, data, HASH_DATA(oht,data));
	if (index == QSE_OHT_NIL) return QSE_OHT_NIL;

	/* compact the cluster */
	for (i = 0, x = index, y = index; i < oht->size; i++)
	{
		y = (y + 1) % oht->capa.hard;

		/* done if the slot at the current hash index is empty */
		if (oht->mark[y] == QSE_OHT_EMPTY) break;

		/* get the natural hash index for the data in the slot at 
		 * the current hash index */
		z = HASH_DATA(oht,DATA_PTR(oht,y)) % oht->capa.hard;
		
		/* move an element if necesary */
		if ((y > x && (z <= x || z > y)) ||
		    (y < x && (z <= x && z > y)))
		{
			COPY_DATA (oht, DATA_PTR(oht,x), DATA_PTR(oht,y));
			x = y;
		}
	}

	oht->mark[x] = QSE_OHT_EMPTY;
	oht->size--;

	return index;
}

void qse_oht_clear (qse_oht_t* oht)
{
	qse_size_t i;
	for (i = 0; i < oht->capa.hard; i++)
		oht->mark[i] = QSE_OHT_EMPTY;
	oht->size = 0;
}

void qse_oht_walk (qse_oht_t* oht, qse_oht_walker_t walker, void* ctx)
{
	qse_size_t i;

	for (i = 0; i < oht->capa.hard; i++)
	{
		if (oht->mark[i] == QSE_OHT_OCCUPIED)
		{
			if (walker (oht, DATA_PTR(oht,i), ctx) == QSE_OHT_WALK_STOP) 
				return;
		}
	}
}
