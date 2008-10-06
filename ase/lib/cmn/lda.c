/*
 * $Id: lda.c 337 2008-08-20 09:17:25Z baconevi $
 *
 * {License}
 */

#include <ase/cmn/lda.h>
#include "mem.h"

#define DPTR(cell) ASE_LDA_DPTR(cell)
#define DLEN(cell) ASE_LDA_DLEN(cell)
#define TOB(lda,len) ((len)*(lda)->scale)
#define INVALID ASE_LDA_INVALID

#define cell_t ase_lda_cell_t
#define lda_t  ase_lda_t
#define size_t ase_size_t
#define mmgr_t ase_mmgr_t

lda_t* ase_lda_open (mmgr_t* mmgr, size_t ext, size_t capa)
{
	lda_t* lda;

	if (mmgr == ASE_NULL) 
	{
		mmgr = ASE_MMGR_GETDFL();

		ASE_ASSERTX (mmgr != ASE_NULL,
			"Set the memory manager with ASE_MMGR_SETDFL()");

		if (mmgr == ASE_NULL) return ASE_NULL;
	}

        lda = ASE_MMGR_ALLOC (mmgr, ASE_SIZEOF(lda_t) + ext);
        if (lda == ASE_NULL) return ASE_NULL;

        return ase_lda_init (lda, mmgr, capa);
}

void ase_lda_close (lda_t* lda)
{
	ase_lda_fini (lda);
	ASE_MMGR_FREE (lda->mmgr, lda);
}

lda_t* ase_lda_init (lda_t* lda, mmgr_t* mmgr, size_t capa)
{
	ASE_MEMSET (lda, 0, ASE_SIZEOF(*lda));

	lda->mmgr = mmgr;
	lda->size = 0;
	lda->capa = 0;
	lda->cell = ASE_NULL;

	if (ase_lda_setcapa (lda, capa) == ASE_NULL) return ASE_NULL;
	return lda;
}

void ase_lda_fini (lda_t* lda)
{
	ase_lda_clear (lda);

	if (lda->cell != ASE_NULL) 
	{
		ASE_MMGR_FREE (lda->mmgr, lda->cell);
		lda->cell = ASE_NULL;
		lda->capa = 0;
	}
}

void* ase_lda_getextension (lda_t* lda)
{
	return lda + 1;
}

mmgr_t* ase_lda_getmmgr (lda_t* lda)
{
	return lda->mmgr;
}

void ase_lda_setmmgr (lda_t* lda, mmgr_t* mmgr)
{
	lda->mmgr = mmgr;
}

size_t ase_lda_getsize (lda_t* lda)
{
	return lda->size;
}

size_t ase_lda_getcapa (lda_t* lda)
{
	return lda->capa;
}

lda_t* ase_lda_setcapa (lda_t* lda, size_t capa)
{
	void* tmp;

	if (lda->size > capa) 
	{
		ase_lda_delete (lda, capa, lda->size - capa);
		ASE_ASSERT (lda->size <= capa);
	}

	if (capa > 0) 
	{
		if (lda->mmgr->realloc != ASE_NULL)
		{
			tmp = ASE_MMGR_REALLOC (lda->mmgr,
				lda->cell, ASE_SIZEOF(*lda->cell) * capa);
			if (tmp == ASE_NULL) return ASE_NULL;
		}
		else
		{
			tmp = ASE_MMGR_ALLOC (
				lda->mmgr, ASE_SIZEOF(*lda->cell) * capa);
			if (tmp == ASE_NULL) return ASE_NULL;
			if (lda->cell != ASE_NULL) 
			{
				size_t x;
				x = (capa > lda->capa)? lda->capa: capa;
				ASE_MEMCPY (tmp, lda->cell, 
					ASE_SIZEOF(*lda->cell) * x);
				ASE_MMGR_FREE (lda->mmgr, lda->cell);
			}
		}
	}
	else 
	{
		if (lda->cell != ASE_NULL) 
		{
			ase_lda_clear (lda);
			ASE_MMGR_FREE (lda->mmgr, lda->cell);
		}
		tmp = ASE_NULL;
	}

	lda->cell = tmp;
	lda->capa = capa;
	
	return lda;
}

static cell_t* alloc_cell (lda_t* lda, void* dptr, size_t dlen)
{
	cell_t* n;

	if (lda->copier == ASE_NULL)
	{
		n = ASE_MMGR_ALLOC (lda->mmgr, ASE_SIZEOF(cell_t));
		if (n == ASE_NULL) return ASE_NULL;
		DPTR(n) = dptr;
	}
	else if (lda->copier == ASE_LDA_COPIER_INLINE)
	{
		n = ASE_MMGR_ALLOC (lda->mmgr, 
			ASE_SIZEOF(cell_t) + TOB(lda,dlen));
		if (n == ASE_NULL) return ASE_NULL;

		ASE_MEMCPY (n + 1, dptr, TOB(lda,dlen));
		DPTR(n) = n + 1;
	}
	else
	{
		n = ASE_MMGR_ALLOC (lda->mmgr, ASE_SIZEOF(cell_t));
		if (n == ASE_NULL) return ASE_NULL;
		DPTR(n) = lda->copier (lda, dptr, dlen);
		if (DPTR(n) == ASE_NULL) 
		{
			ASE_MMGR_FREE (lda->mmgr, n);
			return ASE_NULL;
		}
	}

	DLEN(n) = dlen; 

	return n;
}

size_t ase_lda_insert (lda_t* lda, size_t index, void* dptr, size_t dlen)
{
	size_t i;
	cell_t* cell;

	cell = alloc_cell (lda, dptr, dlen);
	if (cell == ASE_NULL) return INVALID;

	if (index >= lda->capa) 
	{
		size_t capa;

		if (lda->capa <= 0) capa = (index + 1);
		else 
		{
			do { capa = lda->capa * 2; } while (index >= capa);
		}

		if (ase_lda_setcapa(lda,capa) == ASE_NULL) 
		{
			if (lda->freeer) 
				lda->freeer (lda, DPTR(cell), DLEN(cell));
			return INVALID;
		}
	}

	for (i = lda->size; i > index; i--) 
		lda->cell[i] = lda->cell[i-1];
	lda->cell[index] = cell;

	if (index > lda->size) lda->size = index + 1;
	else lda->size++;

	return index;
}

size_t ase_lda_delete (lda_t* lda, size_t index, size_t count)
{
	size_t i, j, k;

	if (index >= lda->size) return 0;
	if (count > lda->size - index) count = lda->size - index;

	i = index;
	j = index + count;
	k = index + count;

	while (i < k) 
	{
		cell_t* c = lda->cell[i];

		if (lda->freeer != ASE_NULL)
			lda->freeer (lda, DPTR(c), DLEN(c));
		ASE_MMGR_FREE (lda->mmgr, c);

		if (j >= lda->size) 
		{
			lda->cell[i] = ASE_NULL;
			i++;
		}
		else
		{
			lda->cell[i] = lda->cell[j];
			i++; j++;		
		}
	}

	lda->size -= count;
	return count;
}

void ase_lda_clear (lda_t* lda)
{
	size_t i;

	for (i = 0; i < lda->size; i++) 
	{
		cell_t* c = lda->cell[i];
		if (lda->freeer)
			lda->freeer (lda, DPTR(c), DLEN(c));
		ASE_MMGR_FREE (lda->mmgr, c);
		lda->cell[i] = ASE_NULL;
	}

	lda->size = 0;
}

size_t ase_lda_add (
	lda_t* lda, ase_char_t* str, size_t len)
{
	return ase_lda_insert (lda, lda->size, str, len);
}

size_t ase_lda_adduniq (
	lda_t* lda, ase_char_t* str, size_t len)
{
	size_t i;
	i = ase_lda_find (lda, 0, str, len);
	if (i != INVALID) return i; /* found. return the current index */

	/* insert a new entry */
	return ase_lda_insert (lda, lda->size, str, len);
}

#if 0
size_t ase_lda_find (
	lda_t* lda, size_t index, 
	const ase_char_t* str, size_t len)
{
	size_t i;

	for (i = index; i < lda->size; i++) 
	{
		if (ase_strxncmp (
			lda->cell[i].name.ptr, lda->cell[i].name.len, 
			str, len) == 0) return i;
	}

	return INVALID;
}

size_t ase_lda_rfind (
	lda_t* lda, size_t index, 
	const ase_char_t* str, size_t len)
{
	size_t i;

	if (index >= lda->size) return INVALID;

	for (i = index + 1; i-- > 0; ) 
	{
		if (ase_strxncmp (
			lda->cell[i].name.ptr, lda->cell[i].name.len, 
			str, len) == 0) return i;
	}

	return INVALID;
}

size_t ase_lda_rrfind (
	lda_t* lda, size_t index,
	const ase_char_t* str, size_t len)
{
	size_t i;

	if (index >= lda->size) return INVALID;

	for (i = lda->size - index; i-- > 0; ) 
	{
		if (ase_strxncmp (
			lda->cell[i].name.ptr, lda->cell[i].name.len, 
			str, len) == 0) return i;
	}

	return INVALID;
}

size_t ase_lda_findx (
	lda_t* lda, size_t index, 
	const ase_char_t* str, size_t len,
	void(*transform)(size_t, ase_cstr_t*,void*), void* arg)
{
	size_t i;

	for (i = index; i < lda->size; i++) 
	{
		ase_cstr_t x;

		x.ptr = lda->cell[i].name.ptr;
		x.len = lda->cell[i].name.len;

		transform (i, &x, arg);
		if (ase_strxncmp (x.ptr, x.len, str, len) == 0) return i;
	}

	return INVALID;
}

size_t ase_lda_rfindx (
	lda_t* lda, size_t index, 
	const ase_char_t* str, size_t len,
	void(*transform)(size_t, ase_cstr_t*,void*), void* arg)
{
	size_t i;

	if (index >= lda->size) return INVALID;

	for (i = index + 1; i-- > 0; ) 
	{
		ase_cstr_t x;

		x.ptr = lda->cell[i].name.ptr;
		x.len = lda->cell[i].name.len;

		transform (i, &x, arg);
		if (ase_strxncmp (x.ptr, x.len, str, len) == 0) return i;
	}

	return INVALID;
}

size_t ase_lda_rrfindx (
	lda_t* lda, size_t index,
	const ase_char_t* str, size_t len, 
	void(*transform)(size_t, ase_cstr_t*,void*), void* arg)
{
	size_t i;

	if (index >= lda->size) return INVALID;

	for (i = lda->size - index; i-- > 0; ) 
	{
		ase_cstr_t x;

		x.ptr = lda->cell[i].name.ptr;
		x.len = lda->cell[i].name.len;

		transform (i, &x, arg);
		if (ase_strxncmp (x.ptr, x.len, str, len) == 0) return i;
	}

	return INVALID;
}


void* ase_lda_copyinline (lda_t* lda, void* dptr, size_t dlen)
{
	/* this is a dummy copier */
	return ASE_NULL;
}

#endif
