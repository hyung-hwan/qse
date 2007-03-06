/*
 * $Id: tab.c,v 1.32 2007-03-06 14:51:53 bacon Exp $
 *
 * {License}
 */

#include <ase/awk/awk_i.h>

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
		ASE_AWK_FREE (tab->awk, tab->buf[i].name);
		tab->buf[i].name = ASE_NULL;
		tab->buf[i].name_len = 0;
	}

	tab->size = 0;
}


ase_size_t ase_awk_tab_insert (
	ase_awk_tab_t* tab, ase_size_t index, 
	const ase_char_t* str, ase_size_t len)
{
	ase_size_t i;
	ase_char_t* str_dup;

	str_dup = ase_strxdup (str, len, &tab->awk->prmfns.mmgr);
	if (str_dup == ASE_NULL) return (ase_size_t)-1;

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
			ASE_AWK_FREE (tab->awk, str_dup);
			return (ase_size_t)-1;
		}
	}

	for (i = tab->size; i > index; i--) tab->buf[i] = tab->buf[i-1];
	tab->buf[index].name = str_dup;
	tab->buf[index].name_len = len;

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
		ASE_AWK_FREE (tab->awk, tab->buf[i].name);	

		if (j >= tab->size) 
		{
			tab->buf[i].name = ASE_NULL;
			tab->buf[i].name_len = 0; 
			i++;
		}
		else
		{
			tab->buf[i].name = tab->buf[j].name;
			tab->buf[i].name_len = tab->buf[j].name_len;
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

ase_size_t ase_awk_tab_find (
	ase_awk_tab_t* tab, ase_size_t index, 
	const ase_char_t* str, ase_size_t len)
{
	ase_size_t i;

	for (i = index; i < tab->size; i++) 
	{
		if (ase_strxncmp (
			tab->buf[i].name, tab->buf[i].name_len, 
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
			tab->buf[i].name, tab->buf[i].name_len, 
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
			tab->buf[i].name, tab->buf[i].name_len, 
			str, len) == 0) return i;
	}

	return (ase_size_t)-1;
}
