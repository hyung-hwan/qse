/*
 * $Id: tab.c,v 1.22 2006-10-22 11:34:53 bacon Exp $
 */

#include <sse/awk/awk_i.h>

sse_awk_tab_t* sse_awk_tab_open (sse_awk_tab_t* tab, sse_awk_t* awk)
{
	if (tab == SSE_NULL) 
	{
		tab = (sse_awk_tab_t*) SSE_AWK_MALLOC (
			awk, sse_sizeof(sse_awk_tab_t));
		if (tab == SSE_NULL) return SSE_NULL;
		tab->__dynamic = sse_true;
	}
	else tab->__dynamic = sse_false;

	tab->awk = awk;
	tab->buf = SSE_NULL;
	tab->size = 0;
	tab->capa = 0;

	return tab;
}

void sse_awk_tab_close (sse_awk_tab_t* tab)
{
	sse_awk_tab_clear (tab);
	if (tab->buf != SSE_NULL) 
	{
		SSE_AWK_FREE (tab->awk, tab->buf);
		tab->buf = SSE_NULL;
		tab->capa = 0;
	}

	if (tab->__dynamic) SSE_AWK_FREE (tab->awk, tab);
}

sse_size_t sse_awk_tab_getsize (sse_awk_tab_t* tab)
{
	return tab->size;
}

sse_size_t sse_awk_tab_getcapa (sse_awk_tab_t* tab)
{
	return tab->capa;
}

sse_awk_tab_t* sse_awk_tab_setcapa (sse_awk_tab_t* tab, sse_size_t capa)
{
	void* tmp;

	if (tab->size > capa) 
	{
		sse_awk_tab_remove (tab, capa, tab->size - capa);
		sse_awk_assert (tab->awk, tab->size <= capa);
	}

	if (capa > 0) 
	{
		if (tab->awk->syscas.realloc != SSE_NULL)
		{
			tmp = SSE_AWK_REALLOC (tab->awk, 
				tab->buf, sse_sizeof(*tab->buf) * capa);
			if (tmp == SSE_NULL) return SSE_NULL;
		}
		else
		{
			tmp = SSE_AWK_MALLOC (
				tab->awk, sse_sizeof(*tab->buf) * capa);
			if (tmp == SSE_NULL) return SSE_NULL;
			if (tab->buf != SSE_NULL) 
			{
				sse_size_t x;
				x = (capa > tab->capa)? tab->capa: capa;
				SSE_AWK_MEMCPY (
					tab->awk, tmp, tab->buf, 
					sse_sizeof(*tab->buf) * x);
				SSE_AWK_FREE (tab->awk, tab->buf);
			}
		}
	}
	else 
	{
		if (tab->buf != SSE_NULL) SSE_AWK_FREE (tab->awk, tab->buf);
		tmp = SSE_NULL;
	}

	tab->buf = tmp;
	tab->capa = capa;
	
	return tab;
}

void sse_awk_tab_clear (sse_awk_tab_t* tab)
{
	sse_size_t i;

	for (i = 0; i < tab->size; i++) 
	{
		SSE_AWK_FREE (tab->awk, tab->buf[i].name);
		tab->buf[i].name = SSE_NULL;
		tab->buf[i].name_len = 0;
	}

	tab->size = 0;
}


sse_size_t sse_awk_tab_insert (
	sse_awk_tab_t* tab, sse_size_t index, 
	const sse_char_t* str, sse_size_t len)
{
	sse_size_t i;
	sse_char_t* str_dup;

	str_dup = sse_awk_strxdup (tab->awk, str, len);
	if (str_dup == SSE_NULL) return (sse_size_t)-1;

	if (index >= tab->capa) 
	{
		sse_size_t capa;

		if (tab->capa <= 0) capa = (index + 1);
		else 
		{
			do { capa = tab->capa * 2; } while (index >= capa);
		}

		if (sse_awk_tab_setcapa(tab,capa) == SSE_NULL) 
		{
			SSE_AWK_FREE (tab->awk, str_dup);
			return (sse_size_t)-1;
		}
	}

	for (i = tab->size; i > index; i--) tab->buf[i] = tab->buf[i-1];
	tab->buf[index].name = str_dup;
	tab->buf[index].name_len = len;

	if (index > tab->size) tab->size = index + 1;
	else tab->size++;

	return index;
}

sse_size_t sse_awk_tab_remove (
	sse_awk_tab_t* tab, sse_size_t index, sse_size_t count)
{
	sse_size_t i, j, k;

	if (index >= tab->size) return 0;
	if (count > tab->size - index) count = tab->size - index;

	i = index;
	j = index + count;
	k = index + count;

	while (i < k) 
	{
		SSE_AWK_FREE (tab->awk, tab->buf[i].name);	

		if (j >= tab->size) 
		{
			tab->buf[i].name = SSE_NULL;
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

sse_size_t sse_awk_tab_add (
	sse_awk_tab_t* tab, const sse_char_t* str, sse_size_t len)
{
	return sse_awk_tab_insert (tab, tab->size, str, len);
}

sse_size_t sse_awk_tab_find (
	sse_awk_tab_t* tab, sse_size_t index, 
	const sse_char_t* str, sse_size_t len)
{
	sse_size_t i;

	for (i = index; i < tab->size; i++) 
	{
		if (sse_awk_strxncmp (
			tab->buf[i].name, tab->buf[i].name_len, 
			str, len) == 0) return i;
	}

	return (sse_size_t)-1;
}

sse_size_t sse_awk_tab_rfind (
	sse_awk_tab_t* tab, sse_size_t index, 
	const sse_char_t* str, sse_size_t len)
{
	sse_size_t i;

	if (index >= tab->size) return (sse_size_t)-1;

	for (i = index + 1; i-- > 0; ) 
	{
		if (sse_awk_strxncmp (
			tab->buf[i].name, tab->buf[i].name_len, 
			str, len) == 0) return i;
	}

	return (sse_size_t)-1;
}

sse_size_t sse_awk_tab_rrfind (
	sse_awk_tab_t* tab, sse_size_t index,
	const sse_char_t* str, sse_size_t len)
{
	sse_size_t i;

	if (index >= tab->size) return (sse_size_t)-1;

	for (i = tab->size - index; i-- > 0; ) 
	{
		if (sse_awk_strxncmp (
			tab->buf[i].name, tab->buf[i].name_len, 
			str, len) == 0) return i;
	}

	return (sse_size_t)-1;
}
