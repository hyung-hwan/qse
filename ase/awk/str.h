/*
 * $Id: str.h,v 1.3 2006-10-22 11:34:53 bacon Exp $
 */

#ifndef _SSE_AWK_STR_H_
#define _SSE_AWK_STR_H_

#ifndef _SSE_AWK_AWK_H_
#error Never include this file directly. Include <sse/awk/awk.h> instead
#endif

#define SSE_AWK_STR_LEN(x)  ((x)->size)
#define SSE_AWK_STR_SIZE(x) ((x)->size + 1)
#define SSE_AWK_STR_CAPA(x) ((x)->capa)
#define SSE_AWK_STR_BUF(x)  ((x)->buf)
#define SSE_AWK_STR_CHAR(x,idx) ((x)->buf[idx])

typedef struct sse_awk_str_t sse_awk_str_t;

struct sse_awk_str_t
{
	sse_char_t* buf;
	sse_size_t size;
	sse_size_t capa;
	sse_awk_t* awk;
	sse_bool_t __dynamic;
};

#ifdef __cplusplus
extern "C" {
#endif

sse_awk_str_t* sse_awk_str_open (
	sse_awk_str_t* str, sse_size_t capa, sse_awk_t* awk);

void sse_awk_str_close (sse_awk_str_t* str);

void sse_awk_str_forfeit (sse_awk_str_t* str);
void sse_awk_str_swap (sse_awk_str_t* str, sse_awk_str_t* str2);

sse_size_t sse_awk_str_cpy (sse_awk_str_t* str, const sse_char_t* s);

sse_size_t sse_awk_str_ncpy (
	sse_awk_str_t* str, const sse_char_t* s, sse_size_t len);

sse_size_t sse_awk_str_cat (sse_awk_str_t* str, const sse_char_t* s);

sse_size_t sse_awk_str_ncat (
	sse_awk_str_t* str, const sse_char_t* s, sse_size_t len);

sse_size_t sse_awk_str_ccat (sse_awk_str_t* str, sse_char_t c);

sse_size_t sse_awk_str_nccat (sse_awk_str_t* str, sse_char_t c, sse_size_t len);

void sse_awk_str_clear (sse_awk_str_t* str);

#ifdef __cplusplus
}
#endif

#endif
