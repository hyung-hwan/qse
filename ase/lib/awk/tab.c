/*
 * $Id: tab.c 192 2008-06-06 10:33:44Z baconevi $
 *
 * {License}
 */

#include "awk_i.h"

ase_awk_tab_t* ase_awk_tab_open (ase_awk_tab_t* tab, ase_awk_t* awk)
{
	if (tab == ASE_NULL) 
	{
		tab = (ase_awk_tab_t*) ASE_AWK_MALLOC (
			awk, ASE_SIZEOF(ase_awk_tab_t));
		if (tab == ASE_NULL) return ASE_NULL;
		tab->__dynamic = ase_true;
	}
	else tab->__dynamic = ase_false;

	tab->awk = awk;
	tab->buf = ASE_NULL;
	tab->size = 0;
	tab->capa = 0;

	return tab;
}

void ase_awk_tab_close (ase_awk_tab_t* tab)
{
	ase_awk_tab_clear (tab);
	if (tab->buf != ASE_NULL) 
	{
		ASE_AWK_FREE (tab->awk, tab->buf);
		tab->buf = ASE_NULL;
		tab->capa = 0;
	}

	if (tab->__dynamic) ASE_AWK_FREE (tab->awk, tab);
}

ase_size_t ase_awk_tab_getsize (ase_awk_tab_t* tab)
{
	return tab->size;
}

ase_size_t ase_awk_tab_getcapa (ase_awk_tab_t* tab)
{
	return tab->capa;
}

ase_awk_tab_t* ase_awk_tab_setcapa (ase_awk_tab_t* tab, ase_size_t capa)
{
	void* tmp;

	if (tab->size > capa) 
	{
		ase_awk_tab_remove (tab, capa, tab->size - capa);
		ASE_ASSERT (tab->size <= capa);
	}

	if (capa > 0) 
	{
		if (tab->awk->prmfns.mmgr.realloc != ASE_NULL)
		{
			tmp = ASE_AWK_REALLOC (tab->awk, 
				tab->buf, ASE_SIZEOF(*tab->buf) * capa);
			if (tmp == ASE_NULL) return ASE_NULL;
		}
		else
		{
			tmp = ASE_AWK_MALLOC (
				tab->awk, ASE_SIZEOF(*tab->buf) * capa);
			if (tmp == ASE_NULL) return ASE_NULL;
			if (tab->buf != ASE_NULL) 
			{
				ase_size_t x;
				x = (capa > tab->capa)? tab->capa: capa;
				ase_memcpy (
					tmp, tab->buf, 
					ASE_SIZEOF(*tab->buf) * x);
				ASE_AWK_FREE (tab->awk, tab->buf);
			}
		}
	}
	else 
	{
		if (tab->buf != ASE_NULL) ASE_AWK_FREE (tab->awk, tab->buf);
		tmp = ASE_NULL;
	}

	tab->buf = tmp;
	tab->capa = capa;
	
	return tab;
}

void ase_awk_tab_clear (ase_awk_tab_t* tab)
{
	ase_size_t i;

	for (i = 0; i < tab->size; i++) 
	{
		ASE_AWK_FREE (tab->awk, tab->buf[i].name.ptr);
		tab->buf[i].name.ptr = ASE_NULL;
		tab->buf[i].name.len = 0;
	}

	tab->size = 0;
}


ase_size_t ase_awk_tab_insert (
	ase_awk_tab_t* tab, ase_size_t index, 
	const ase_char_t* str, ase_size_t len)
{
	ase_size_t i;
	ase_char_t* dup;

	dup = ase_awk_strxdup (tab->awk, str, len);
	if (dup == ASE_NULL) return (ase_size_t)-1;

	if (index >= tab->capa) 
	{
		ase_size_t capa;

		if (tab->capa <= 0) capa = (index + 1);
		else 
		{
			do { capa = tab->capa * 2; } while (index >= capa);
		}

		if (ase_awk_tab_setcapa(tab,capa) == ASE_NULL) 
		{
			ASE_AWK_FREE (tab->awk, dup);
			return (ase_size_t)-1;
		}
	}

	for (i = tab->size; i > index; i--) tab->buf[i] = tab->buf[i-1];
	tab->buf[index].name.ptr = dup;
	tab->buf[index].name.len = len;

	if (index > tab->size) tab->size = index + 1;
	else tab->size++;

	return index;
}

ase_size_t ase_awk_tab_remove (
	ase_awk_tab_t* tab, ase_size_t index, ase_size_t count)
{
	ase_size_t i, j, k;

	if (index >= tab->size) return 0;
	if (count > tab->size - index) count = tab->size - index;

	i = index;
	j = index + count;
	k = index + count;

	while (i < k) 
	{
		ASE_AWK_FREE (tab->awk, tab->buf[i].name.ptr);	

		if (j >= tab->size) 
		{
			tab->buf[i].name.ptr = ASE_NULL;
			tab->buf[i].name.len = 0; 
			i++;
		}
		else
		{
			tab->buf[i].name.ptr = tab->buf[j].name.ptr;
			tab->buf[i].name.len = tab->buf[j].name.len;
			i++; j++;		
		}
	}

	tab->size -= count;
	return count;
}

ase_size_t ase_awk_tab_add (
	ase_awk_tab_t* tab, const ase_char_t* str, ase_size_t len)
{
	return ase_awk_tab_insert (tab, tab->size, str, len);
}

ase_size_t ase_awk_tab_adduniq (
	ase_awk_tab_t* tab, const ase_char_t* str, ase_size_t len)
{
	ase_size_t i;
	i = ase_awk_tab_find (tab, 0, str, len);
	if (i != (ase_size_t)-1) return i; /* found. return the current index */

	/* insert a new entry */
	return ase_awk_tab_insert (tab, tab->size, str, len);
}

ase_size_t ase_awk_tab_find (
	ase_awk_tab_t* tab, ase_size_t index, 
	const ase_char_t* str, ase_size_t len)
{
	ase_size_t i;

	for (i = index; i < tab->size; i++) 
	{
		if (ase_strxncmp (
			tab->buf[i].name.ptr, tab->buf[i].name.len, 
			str, len) == 0) return i;
	}

	return (ase_size_t)-1;
}

ase_size_t ase_awk_tab_rfind (
	ase_awk_tab_t* tab, ase_size_t index, 
	const ase_char_t* str, ase_size_t len)
{
	ase_size_t i;

	if (index >= tab->size) return (ase_size_t)-1;

	for (i = index + 1; i-- > 0; ) 
	{
		if (ase_strxncmp (
			tab->buf[i].name.ptr, tab->buf[i].name.len, 
			str, len) == 0) return i;
	}

	return (ase_size_t)-1;
}

ase_size_t ase_awk_tab_rrfind (
	ase_awk_tab_t* tab, ase_size_t index,
	const ase_char_t* str, ase_size_t len)
{
	ase_size_t i;

	if (index >= tab->size) return (ase_size_t)-1;

	for (i = tab->size - index; i-- > 0; ) 
	{
		if (ase_strxncmp (
			tab->buf[i].name.ptr, tab->buf[i].name.len, 
			str, len) == 0) return i;
	}

	return (ase_size_t)-1;
}

ase_size_t ase_awk_tab_findx (
	ase_awk_tab_t* tab, ase_size_t index, 
	const ase_char_t* str, ase_size_t len,
	void(*transform)(ase_size_t, ase_cstr_t*,void*), void* arg)
{
	ase_size_t i;

	for (i = index; i < tab->size; i++) 
	{
		ase_cstr_t x;

		x.ptr = tab->buf[i].name.ptr;
		x.len = tab->buf[i].name.len;

		transform (i, &x, arg);
		if (ase_strxncmp (x.ptr, x.len, str, len) == 0) return i;
	}

	return (ase_size_t)-1;
}

ase_size_t ase_awk_tab_rfindx (
	ase_awk_tab_t* tab, ase_size_t index, 
	const ase_char_t* str, ase_size_t len,
	void(*transform)(ase_size_t, ase_cstr_t*,void*), void* arg)
{
	ase_size_t i;

	if (index >= tab->size) return (ase_size_t)-1;

	for (i = index + 1; i-- > 0; ) 
	{
		ase_cstr_t x;

		x.ptr = tab->buf[i].name.ptr;
		x.len = tab->buf[i].name.len;

		transform (i, &x, arg);
		if (ase_strxncmp (x.ptr, x.len, str, len) == 0) return i;
	}

	return (ase_size_t)-1;
}

ase_size_t ase_awk_tab_rrfindx (
	ase_awk_tab_t* tab, ase_size_t index,
	const ase_char_t* str, ase_size_t len, 
	void(*transform)(ase_size_t, ase_cstr_t*,void*), void* arg)
{
	ase_size_t i;

	if (index >= tab->size) return (ase_size_t)-1;

	for (i = tab->size - index; i-- > 0; ) 
	{
		ase_cstr_t x;

		x.ptr = tab->buf[i].name.ptr;
		x.len = tab->buf[i].name.len;

		transform (i, &x, arg);
		if (ase_strxncmp (x.ptr, x.len, str, len) == 0) return i;
	}

	return (ase_size_t)-1;
}
