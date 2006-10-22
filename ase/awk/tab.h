/*
 * $Id: tab.h,v 1.13 2006-10-22 11:34:53 bacon Exp $
 */

#ifndef _SSE_AWK_TAB_H_
#define _SSE_AWK_TAB_H_

#ifndef _SSE_AWK_AWK_H_
#error Never include this file directly. Include <sse/awk/awk.h> instead
#endif

/* TODO: you have to turn this into a hash table.
	 as of now, this is an arrayed table. */

typedef struct sse_awk_tab_t sse_awk_tab_t;

struct sse_awk_tab_t
{
	struct
	{
		sse_char_t* name;
		sse_size_t name_len;
	}* buf;
	sse_size_t size;
	sse_size_t capa;
	sse_awk_t* awk;
	sse_bool_t __dynamic;	
};

#ifdef __cplusplus
extern "C" {
#endif

sse_awk_tab_t* sse_awk_tab_open (sse_awk_tab_t* tab, sse_awk_t* awk);
void sse_awk_tab_close (sse_awk_tab_t* tab);

sse_size_t sse_awk_tab_getsize (sse_awk_tab_t* tab);
sse_size_t sse_awk_tab_getcapa (sse_awk_tab_t* tab);
sse_awk_tab_t* sse_awk_tab_setcapa (sse_awk_tab_t* tab, sse_size_t capa);

void sse_awk_tab_clear (sse_awk_tab_t* tab);

sse_size_t sse_awk_tab_insert (
	sse_awk_tab_t* tab, sse_size_t index, 
	const sse_char_t* str, sse_size_t len);

sse_size_t sse_awk_tab_remove (
	sse_awk_tab_t* tab, sse_size_t index, sse_size_t count);

sse_size_t sse_awk_tab_add (
	sse_awk_tab_t* tab, const sse_char_t* str, sse_size_t len);

sse_size_t sse_awk_tab_find (
	sse_awk_tab_t* tab, sse_size_t index,
	const sse_char_t* str, sse_size_t len);
sse_size_t sse_awk_tab_rfind (
	sse_awk_tab_t* tab, sse_size_t index,
	const sse_char_t* str, sse_size_t len);
sse_size_t sse_awk_tab_rrfind (
	sse_awk_tab_t* tab, sse_size_t index,
	const sse_char_t* str, sse_size_t len);

#ifdef __cplusplus
}
#endif

#endif
