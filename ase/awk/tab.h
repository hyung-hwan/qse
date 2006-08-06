/*
 * $Id: tab.h,v 1.11 2006-08-06 15:02:55 bacon Exp $
 */

#ifndef _XP_AWK_TAB_H_
#define _XP_AWK_TAB_H_

#ifndef _XP_AWK_AWK_H_
#error Never include this file directly. Include <xp/awk/awk.h> instead
#endif

/* TODO: you have to turn this into a hash table.
	 as of now, this is an arrayed table. */

typedef struct xp_awk_tab_t xp_awk_tab_t;

struct xp_awk_tab_t
{
	struct
	{
		xp_char_t* name;
		xp_size_t name_len;
	}* buf;
	xp_size_t size;
	xp_size_t capa;
	xp_bool_t __dynamic;	
};

#ifdef __cplusplus
extern "C" {
#endif

xp_awk_tab_t* xp_awk_tab_open (xp_awk_tab_t* tab);
void xp_awk_tab_close (xp_awk_tab_t* tab);

xp_size_t xp_awk_tab_getsize (xp_awk_tab_t* tab);
xp_size_t xp_awk_tab_getcapa (xp_awk_tab_t* tab);
xp_awk_tab_t* xp_awk_tab_setcapa (xp_awk_tab_t* tab, xp_size_t capa);

void xp_awk_tab_clear (xp_awk_tab_t* tab);

xp_size_t xp_awk_tab_insert (
	xp_awk_tab_t* tab, xp_size_t index, 
	const xp_char_t* str, xp_size_t len);

xp_size_t xp_awk_tab_remove (
	xp_awk_tab_t* tab, xp_size_t index, xp_size_t count);

xp_size_t xp_awk_tab_add (
	xp_awk_tab_t* tab, const xp_char_t* str, xp_size_t len);

xp_size_t xp_awk_tab_find (
	xp_awk_tab_t* tab, xp_size_t index,
	const xp_char_t* str, xp_size_t len);
xp_size_t xp_awk_tab_rfind (
	xp_awk_tab_t* tab, xp_size_t index,
	const xp_char_t* str, xp_size_t len);
xp_size_t xp_awk_tab_rrfind (
	xp_awk_tab_t* tab, xp_size_t index,
	const xp_char_t* str, xp_size_t len);

#ifdef __cplusplus
}
#endif

#endif
