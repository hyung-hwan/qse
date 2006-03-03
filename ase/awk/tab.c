/*
 * $Id: tab.c,v 1.5 2006-03-03 11:51:48 bacon Exp $
 */

#include <xp/awk/awk.h>

#ifndef __STAND_ALONE
#include <xp/bas/memory.h>
#include <xp/bas/string.h>
#include <xp/bas/assert.h>
#endif

xp_awk_tab_t* xp_awk_tab_open (xp_awk_tab_t* tab)
{
	if (tab == XP_NULL) {
		tab = (xp_awk_tab_t*) xp_malloc (xp_sizeof(xp_awk_tab_t));
		if (tab == XP_NULL) return XP_NULL;
		tab->__dynamic = xp_true;
	}
	else tab->__dynamic = xp_false;

	tab->buf = XP_NULL;
	tab->size = 0;
	tab->capa = 0;

	return tab;
}

void xp_awk_tab_close (xp_awk_tab_t* tab)
{
	xp_awk_tab_clear (tab);
	if (tab->buf != XP_NULL) {
		xp_free (tab->buf);
		tab->buf = XP_NULL;
		tab->capa = 0;
	}

	if (tab->__dynamic) xp_free (tab);
}

xp_size_t xp_awk_tab_getsize (xp_awk_tab_t* tab)
{
	return tab->size;
}

xp_size_t xp_awk_tab_getcapa (xp_awk_tab_t* tab)
{
	return tab->capa;
}

xp_awk_tab_t* xp_awk_tab_setcapa (xp_awk_tab_t* tab, xp_size_t capa)
{
	xp_char_t** tmp;

	if (tab->size > capa) {
		xp_awk_tab_remrange (tab, capa, tab->size - capa);
		xp_assert (tab->size <= capa);
	}

	if (capa > 0) {
		tmp = (xp_char_t**)xp_realloc (
			tab->buf, xp_sizeof(xp_char_t*) * capa);
		if (tmp == XP_NULL) return XP_NULL;
	}
	else {
		if (tab->buf != XP_NULL) xp_free (tab->buf);
		tmp = XP_NULL;
	}

	tab->buf = tmp;
	tab->capa = capa;
	
	return tab;
}

void xp_awk_tab_clear (xp_awk_tab_t* tab)
{
	xp_size_t i;

	xp_assert (tab != XP_NULL);

	for (i = 0; i < tab->size; i++) {
		xp_free (tab->buf[i]);
		tab->buf[i] = XP_NULL;
	}

	tab->size = 0;
}


xp_size_t xp_awk_tab_insdatum (
	xp_awk_tab_t* tab, xp_size_t index, const xp_char_t* value)
{
	xp_size_t i;
	xp_char_t* value_dup;

	value_dup = xp_strdup(value);
	if (value_dup == XP_NULL) return (xp_size_t)-1;

	if (index >= tab->capa) {
		xp_size_t capa;

		if (tab->capa <= 0) capa = (index + 1);
		else {
			do { capa = tab->capa * 2; } while (index >= capa);
		}

		if (xp_awk_tab_setcapa(tab,capa) == XP_NULL) {
			xp_free (value_dup);
			return (xp_size_t)-1;
		}
	}

	for (i = tab->size; i > index; i--) tab->buf[i] = tab->buf[i-1];
	tab->buf[index] = value_dup;

	if (index > tab->size) tab->size = index + 1;
	else tab->size++;

	return index;
}

xp_size_t xp_awk_tab_remrange (
	xp_awk_tab_t* tab, xp_size_t index, xp_size_t count)
{
	xp_size_t i, j, k;

	if (index >= tab->size) return 0;
	if (count > tab->size - index) count = tab->size - index;

	i = index;
	j = index + count;
	k = index + count;

	while (i < k) {
		xp_free (tab->buf[i]);	
		if (j >= tab->size) 
		tab->buf[i] = (j >= tab->size)? XP_NULL: tab->buf[j];
		i++; j++;		
	}

	tab->size -= count;
	return count;
}

xp_size_t xp_awk_tab_adddatum (xp_awk_tab_t* tab, const xp_char_t* value)
{
	return xp_awk_tab_insdatum (tab, tab->size, value);
}


xp_size_t xp_awk_tab_find (
	xp_awk_tab_t* tab, const xp_char_t* value, xp_size_t index)
{
	xp_size_t i;

	for (i = index; i < tab->size; i++) {
		if (xp_strcmp(tab->buf[i], value) == 0) return i;
	}

	return (xp_size_t)-1;
}

xp_size_t xp_awk_tab_rfind (
	xp_awk_tab_t* tab, const xp_char_t* value, xp_size_t index)
{
	xp_size_t i;

	if (index >= tab->size) return (xp_size_t)-1;

	for (i = index + 1; i-- > 0; ) {
		if (xp_strcmp(tab->buf[i], value) == 0) return i;
	}

	return (xp_size_t)-1;
}

xp_size_t xp_awk_tab_rrfind (
	xp_awk_tab_t* tab, const xp_char_t* value, xp_size_t index)
{
	xp_size_t i;

	if (index >= tab->size) return (xp_size_t)-1;

	for (i = tab->size - index; i-- > 0; ) {
		if (xp_strcmp(tab->buf[i], value) == 0) return i;
	}

	return (xp_size_t)-1;
}
