/*
 * $Id: lda.c 337 2008-08-20 09:17:25Z baconevi $
 *
 * {License}
 */

#include <ase/cmn/lda.h>
#include "mem.h"

#define TOB(lda,len) ((len)*(lda)->scale)

ase_lda_t* ase_lda_open (ase_mmgr_t* mmgr, ase_size_t ext, ase_size_t capa)
{
	ase_lda_t* lda;

	if (mmgr == ASE_NULL) 
	{
		mmgr = ASE_MMGR_GETDFL();

		ASE_ASSERTX (mmgr != ASE_NULL,
			"Set the memory manager with ASE_MMGR_SETDFL()");

		if (mmgr == ASE_NULL) return ASE_NULL;
	}

        lda = ASE_MMGR_ALLOC (mmgr, ASE_SIZEOF(ase_lda_t) + ext);
        if (lda == ASE_NULL) return ASE_NULL;

        return ase_lda_init (lda, mmgr, capa);
}

void ase_lda_close (ase_lda_t* lda)
{
	ase_lda_fini (lda);
	ASE_MMGR_FREE (lda->mmgr, lda);
}

ase_lda_t* ase_lda_init (ase_lda_t* lda, ase_mmgr_t* mmgr, ase_size_t capa)
{
	ASE_MEMSET (lda, 0, ASE_SIZEOF(*lda));

	lda->mmgr = mmgr;
	lda->size = 0;
	lda->capa = 0;
	lda->slot = ASE_NULL;

	if (ase_lda_setcapa (lda, capa) == ASE_NULL) return ASE_NULL;
	return lda;
}

void ase_lda_fini (ase_lda_t* lda)
{
	ase_lda_clear (lda);

	if (lda->slot != ASE_NULL) 
	{
		ASE_MMGR_FREE (lda->mmgr, lda->slot);
		lda->slot = ASE_NULL;
		lda->capa = 0;
	}
}

ase_size_t ase_lda_getsize (ase_lda_t* lda)
{
	return lda->size;
}

ase_size_t ase_lda_getcapa (ase_lda_t* lda)
{
	return lda->capa;
}

ase_lda_t* ase_lda_setcapa (ase_lda_t* lda, ase_size_t capa)
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
				lda->slot, ASE_SIZEOF(*lda->slot) * capa);
			if (tmp == ASE_NULL) return ASE_NULL;
		}
		else
		{
			tmp = ASE_MMGR_ALLOC (
				lda->mmgr, ASE_SIZEOF(*lda->slot) * capa);
			if (tmp == ASE_NULL) return ASE_NULL;
			if (lda->slot != ASE_NULL) 
			{
				ase_size_t x;
				x = (capa > lda->capa)? lda->capa: capa;
				ASE_MEMCPY (
					tmp, lda->slot, 
					ASE_SIZEOF(*lda->slot) * x);
				ASE_MMGR_FREE (lda->mmgr, lda->slot);
			}
		}
	}
	else 
	{
		if (lda->slot != ASE_NULL) ASE_MMGR_FREE (lda->mmgr, lda->slot);
		tmp = ASE_NULL;
	}

	lda->slot = tmp;
	lda->capa = capa;
	
	return lda;
}

static ase_lda_slot_t* alloc_slot (ase_lda_t* lda, void* dptr, ase_size_t dlen)
{
	ase_lda_slot_t* n;

	if (lda->copier == ASE_NULL)
	{
		n = ASE_MMGR_ALLOC (lda->mmgr, ASE_SIZEOF(ase_lda_slot_t));
		if (n == ASE_NULL) return ASE_NULL;
		DPTR(n) = dptr;
	}
	else if (lda->copier == ASE_LDA_COPIER_INLINE)
	{
		n = ASE_MMGR_ALLOC (lda->mmgr, 
			ASE_SIZEOF(ase_lda_slot_t) + TOB(lda,dlen));
		if (n == ASE_NULL) return ASE_NULL;

		ASE_MEMCPY (n + 1, dptr, TOB(lda,dlen));
		DPTR(n) = n + 1;
	}
	else
	{
		n = ASE_MMGR_ALLOC (lda->mmgr, ASE_SIZEOF(slot_t));
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

ase_lda_slot_t* ase_lda_insert (
	ase_lda_t* lda, ase_size_t index, void* dptr, ase_size_t dlen)
{
	ase_size_t i;
	ase_lda_slot_t* slot;

	slot = alloc_slot (lda, dptr, dlen);
	if (slot == ASE_NULL) return ASE_NULL;

	if (index >= lda->capa) 
	{
		ase_size_t capa;

		if (lda->capa <= 0) capa = (index + 1);
		else 
		{
			do { capa = lda->capa * 2; } while (index >= capa);
		}

		if (ase_lda_setcapa(lda,capa) == ASE_NULL) 
		{
			if (lda->freeer) 
				lda->freeer (lda, slot->dptr, slot->dlen);
			return ASE_NULL;
		}
	}

	for (i = lda->size; i > index; i--) 
		lda->slot[i] = lda->slot[i-1];
	lda->slot[index] = slot;

	if (index > lda->size) lda->size = index + 1;
	else lda->size++;

	return index;
}

ase_size_t ase_lda_delete (
	ase_lda_t* lda, ase_size_t index, ase_size_t count)
{
	ase_size_t i, j, k;

	if (index >= lda->size) return 0;
	if (count > lda->size - index) count = lda->size - index;

	i = index;
	j = index + count;
	k = index + count;

	while (i < k) 
	{
		ASE_AWK_FREE (lda->awk, lda->slot[i].name.ptr);	

		if (j >= lda->size) 
		{
			lda->slot[i].name.ptr = ASE_NULL;
			lda->slot[i].name.len = 0; 
			i++;
		}
		else
		{
			lda->slot[i].name.ptr = lda->slot[j].name.ptr;
			lda->slot[i].name.len = lda->slot[j].name.len;
			i++; j++;		
		}
	}

	lda->size -= count;
	return count;
}

void ase_lda_clear (ase_lda_t* lda)
{
	ase_size_t i;

	for (i = 0; i < lda->size; i++) 
	{
		ASE_MMGR_FREE (lda->mmgr, lda->slot[i].name.ptr);
		lda->slot[i].name.ptr = ASE_NULL;
		lda->slot[i].name.len = 0;
	}

	lda->size = 0;
}

ase_size_t ase_lda_add (
	ase_lda_t* lda, ase_char_t* str, ase_size_t len)
{
	return ase_lda_insert (lda, lda->size, str, len);
}

ase_size_t ase_lda_adduniq (
	ase_lda_t* lda, ase_char_t* str, ase_size_t len)
{
	ase_size_t i;
	i = ase_lda_find (lda, 0, str, len);
	if (i != (ase_size_t)-1) return i; /* found. return the current index */

	/* insert a new entry */
	return ase_lda_insert (lda, lda->size, str, len);
}

ase_size_t ase_lda_find (
	ase_lda_t* lda, ase_size_t index, 
	const ase_char_t* str, ase_size_t len)
{
	ase_size_t i;

	for (i = index; i < lda->size; i++) 
	{
		if (ase_strxncmp (
			lda->slot[i].name.ptr, lda->slot[i].name.len, 
			str, len) == 0) return i;
	}

	return (ase_size_t)-1;
}

ase_size_t ase_lda_rfind (
	ase_lda_t* lda, ase_size_t index, 
	const ase_char_t* str, ase_size_t len)
{
	ase_size_t i;

	if (index >= lda->size) return (ase_size_t)-1;

	for (i = index + 1; i-- > 0; ) 
	{
		if (ase_strxncmp (
			lda->slot[i].name.ptr, lda->slot[i].name.len, 
			str, len) == 0) return i;
	}

	return (ase_size_t)-1;
}

ase_size_t ase_lda_rrfind (
	ase_lda_t* lda, ase_size_t index,
	const ase_char_t* str, ase_size_t len)
{
	ase_size_t i;

	if (index >= lda->size) return (ase_size_t)-1;

	for (i = lda->size - index; i-- > 0; ) 
	{
		if (ase_strxncmp (
			lda->slot[i].name.ptr, lda->slot[i].name.len, 
			str, len) == 0) return i;
	}

	return (ase_size_t)-1;
}

ase_size_t ase_lda_findx (
	ase_lda_t* lda, ase_size_t index, 
	const ase_char_t* str, ase_size_t len,
	void(*transform)(ase_size_t, ase_cstr_t*,void*), void* arg)
{
	ase_size_t i;

	for (i = index; i < lda->size; i++) 
	{
		ase_cstr_t x;

		x.ptr = lda->slot[i].name.ptr;
		x.len = lda->slot[i].name.len;

		transform (i, &x, arg);
		if (ase_strxncmp (x.ptr, x.len, str, len) == 0) return i;
	}

	return (ase_size_t)-1;
}

ase_size_t ase_lda_rfindx (
	ase_lda_t* lda, ase_size_t index, 
	const ase_char_t* str, ase_size_t len,
	void(*transform)(ase_size_t, ase_cstr_t*,void*), void* arg)
{
	ase_size_t i;

	if (index >= lda->size) return (ase_size_t)-1;

	for (i = index + 1; i-- > 0; ) 
	{
		ase_cstr_t x;

		x.ptr = lda->slot[i].name.ptr;
		x.len = lda->slot[i].name.len;

		transform (i, &x, arg);
		if (ase_strxncmp (x.ptr, x.len, str, len) == 0) return i;
	}

	return (ase_size_t)-1;
}

ase_size_t ase_lda_rrfindx (
	ase_lda_t* lda, ase_size_t index,
	const ase_char_t* str, ase_size_t len, 
	void(*transform)(ase_size_t, ase_cstr_t*,void*), void* arg)
{
	ase_size_t i;

	if (index >= lda->size) return (ase_size_t)-1;

	for (i = lda->size - index; i-- > 0; ) 
	{
		ase_cstr_t x;

		x.ptr = lda->slot[i].name.ptr;
		x.len = lda->slot[i].name.len;

		transform (i, &x, arg);
		if (ase_strxncmp (x.ptr, x.len, str, len) == 0) return i;
	}

	return (ase_size_t)-1;
}
