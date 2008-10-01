/*
 * $Id: dar.c 337 2008-08-20 09:17:25Z baconevi $
 *
 * {License}
 */

#include <ase/cmn/dar.h>
#include "mem.h"

ase_dar_t* ase_dar_open (ase_mmgr_t* dar, ase_size_t ext, ase_size_t capa)
{
	ase_dar_t* dar;

	if (mmgr == ASE_NULL) 
	{
		mmgr = ASE_MMGR_GETDFL();

		ASE_ASSERTX (mmgr != ASE_NULL,
			"Set the memory manager with ASE_MMGR_SETDFL()");

		if (mmgr == ASE_NULL) return ASE_NULL;
	}

        dar = ASE_MMGR_ALLOC (mmgr, ASE_SIZEOF(ase_dar_t) + ext);
        if (dar == ASE_NULL) return ASE_NULL;

        return ase_dar_init (dar, mmgr, capa);
}

void ase_dar_close (ase_dar_t* dar)
{
	ase_dar_fini (dar);
	ASE_MMGR_FREE (dar->mmgr, dar);
}

ase_dar_t* ase_dar_init (ase_dar_t* dar, ase_mmgr_t* mmgr, ase_size_t capa)
{
	ASE_MEMSET (dar, 0, ASE_SIZEOF(*dar));

	dar->mmgr = mmgr;
	dar->size = 0;
	dar->capa = capa;

	if (capa == 0) dar->buf = ASE_NULL;
	else 
	{
		dar->buf = ASE_MMGR_ALLOC (....);
		if (dar->buf == ASE_NULL) return ASE_NULL;
	}

	return dar;
}

void ase_dar_fini (ase_dar_t* dar)
{
	ase_dar_clear (dar);

	if (dar->buf != ASE_NULL) 
	{
		ASE_MMGR_FREE (dar->mmgr, dar->buf);
		dar->buf = ASE_NULL;
		dar->capa = 0;
	}
}

ase_size_t ase_dar_getsize (ase_dar_t* dar)
{
	return dar->size;
}

ase_size_t ase_dar_getcapa (ase_dar_t* dar)
{
	return dar->capa;
}

ase_dar_t* ase_dar_setcapa (ase_dar_t* dar, ase_size_t capa)
{
	void* tmp;

	if (dar->size > capa) 
	{
		ase_dar_delete (dar, capa, dar->size - capa);
		ASE_ASSERT (dar->size <= capa);
	}

	if (capa > 0) 
	{
		if (dar->awk->mmgr->realloc != ASE_NULL)
		{
			tmp = ASE_AWK_REALLOC (dar->awk, 
				dar->buf, ASE_SIZEOF(*dar->buf) * capa);
			if (tmp == ASE_NULL) return ASE_NULL;
		}
		else
		{
			tmp = ASE_AWK_ALLOC (
				dar->awk, ASE_SIZEOF(*dar->buf) * capa);
			if (tmp == ASE_NULL) return ASE_NULL;
			if (dar->buf != ASE_NULL) 
			{
				ase_size_t x;
				x = (capa > dar->capa)? dar->capa: capa;
				ASE_MEMCPY (
					tmp, dar->buf, 
					ASE_SIZEOF(*dar->buf) * x);
				ASE_AWK_FREE (dar->awk, dar->buf);
			}
		}
	}
	else 
	{
		if (dar->buf != ASE_NULL) ASE_AWK_FREE (dar->awk, dar->buf);
		tmp = ASE_NULL;
	}

	dar->buf = tmp;
	dar->capa = capa;
	
	return dar;
}

void ase_dar_clear (ase_dar_t* dar)
{
	ase_size_t i;

	for (i = 0; i < dar->size; i++) 
	{
		ASE_AWK_FREE (dar->awk, dar->buf[i].name.ptr);
		dar->buf[i].name.ptr = ASE_NULL;
		dar->buf[i].name.len = 0;
	}

	dar->size = 0;
}

ase_size_t ase_dar_insert (
	ase_dar_t* dar, ase_size_t index, 
	const ase_char_t* str, ase_size_t len)
{
	ase_size_t i;
	ase_char_t* dup;

	dup = ASE_AWK_STRXDUP (dar->awk, str, len);
	if (dup == ASE_NULL) return (ase_size_t)-1;

	if (index >= dar->capa) 
	{
		ase_size_t capa;

		if (dar->capa <= 0) capa = (index + 1);
		else 
		{
			do { capa = dar->capa * 2; } while (index >= capa);
		}

		if (ase_dar_setcapa(dar,capa) == ASE_NULL) 
		{
			ASE_AWK_FREE (dar->awk, dup);
			return (ase_size_t)-1;
		}
	}

	for (i = dar->size; i > index; i--) dar->buf[i] = dar->buf[i-1];
	dar->buf[index].name.ptr = dup;
	dar->buf[index].name.len = len;

	if (index > dar->size) dar->size = index + 1;
	else dar->size++;

	return index;
}

ase_size_t ase_dar_delete (
	ase_dar_t* dar, ase_size_t index, ase_size_t count)
{
	ase_size_t i, j, k;

	if (index >= dar->size) return 0;
	if (count > dar->size - index) count = dar->size - index;

	i = index;
	j = index + count;
	k = index + count;

	while (i < k) 
	{
		ASE_AWK_FREE (dar->awk, dar->buf[i].name.ptr);	

		if (j >= dar->size) 
		{
			dar->buf[i].name.ptr = ASE_NULL;
			dar->buf[i].name.len = 0; 
			i++;
		}
		else
		{
			dar->buf[i].name.ptr = dar->buf[j].name.ptr;
			dar->buf[i].name.len = dar->buf[j].name.len;
			i++; j++;		
		}
	}

	dar->size -= count;
	return count;
}

ase_size_t ase_dar_add (
	ase_dar_t* dar, const ase_char_t* str, ase_size_t len)
{
	return ase_dar_insert (dar, dar->size, str, len);
}

ase_size_t ase_dar_adduniq (
	ase_dar_t* dar, const ase_char_t* str, ase_size_t len)
{
	ase_size_t i;
	i = ase_dar_find (dar, 0, str, len);
	if (i != (ase_size_t)-1) return i; /* found. return the current index */

	/* insert a new entry */
	return ase_dar_insert (dar, dar->size, str, len);
}

ase_size_t ase_dar_find (
	ase_dar_t* dar, ase_size_t index, 
	const ase_char_t* str, ase_size_t len)
{
	ase_size_t i;

	for (i = index; i < dar->size; i++) 
	{
		if (ase_strxncmp (
			dar->buf[i].name.ptr, dar->buf[i].name.len, 
			str, len) == 0) return i;
	}

	return (ase_size_t)-1;
}

ase_size_t ase_dar_rfind (
	ase_dar_t* dar, ase_size_t index, 
	const ase_char_t* str, ase_size_t len)
{
	ase_size_t i;

	if (index >= dar->size) return (ase_size_t)-1;

	for (i = index + 1; i-- > 0; ) 
	{
		if (ase_strxncmp (
			dar->buf[i].name.ptr, dar->buf[i].name.len, 
			str, len) == 0) return i;
	}

	return (ase_size_t)-1;
}

ase_size_t ase_dar_rrfind (
	ase_dar_t* dar, ase_size_t index,
	const ase_char_t* str, ase_size_t len)
{
	ase_size_t i;

	if (index >= dar->size) return (ase_size_t)-1;

	for (i = dar->size - index; i-- > 0; ) 
	{
		if (ase_strxncmp (
			dar->buf[i].name.ptr, dar->buf[i].name.len, 
			str, len) == 0) return i;
	}

	return (ase_size_t)-1;
}

ase_size_t ase_dar_findx (
	ase_dar_t* dar, ase_size_t index, 
	const ase_char_t* str, ase_size_t len,
	void(*transform)(ase_size_t, ase_cstr_t*,void*), void* arg)
{
	ase_size_t i;

	for (i = index; i < dar->size; i++) 
	{
		ase_cstr_t x;

		x.ptr = dar->buf[i].name.ptr;
		x.len = dar->buf[i].name.len;

		transform (i, &x, arg);
		if (ase_strxncmp (x.ptr, x.len, str, len) == 0) return i;
	}

	return (ase_size_t)-1;
}

ase_size_t ase_dar_rfindx (
	ase_dar_t* dar, ase_size_t index, 
	const ase_char_t* str, ase_size_t len,
	void(*transform)(ase_size_t, ase_cstr_t*,void*), void* arg)
{
	ase_size_t i;

	if (index >= dar->size) return (ase_size_t)-1;

	for (i = index + 1; i-- > 0; ) 
	{
		ase_cstr_t x;

		x.ptr = dar->buf[i].name.ptr;
		x.len = dar->buf[i].name.len;

		transform (i, &x, arg);
		if (ase_strxncmp (x.ptr, x.len, str, len) == 0) return i;
	}

	return (ase_size_t)-1;
}

ase_size_t ase_dar_rrfindx (
	ase_dar_t* dar, ase_size_t index,
	const ase_char_t* str, ase_size_t len, 
	void(*transform)(ase_size_t, ase_cstr_t*,void*), void* arg)
{
	ase_size_t i;

	if (index >= dar->size) return (ase_size_t)-1;

	for (i = dar->size - index; i-- > 0; ) 
	{
		ase_cstr_t x;

		x.ptr = dar->buf[i].name.ptr;
		x.len = dar->buf[i].name.len;

		transform (i, &x, arg);
		if (ase_strxncmp (x.ptr, x.len, str, len) == 0) return i;
	}

	return (ase_size_t)-1;
}
