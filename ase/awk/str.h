/*
 * $Id: str.h,v 1.1 2006-08-31 16:00:20 bacon Exp $
 */

#ifndef _XP_AWK_STR_H_
#define _XP_AWK_STR_H_

#ifndef _XP_AWK_AWK_H_
#error Never include this file directly. Include <xp/awk/awk.h> instead
#endif

#define XP_AWK_STR_LEN(x)  ((x)->size)
#define XP_AWK_STR_SIZE(x) ((x)->size + 1)
#define XP_AWK_STR_CAPA(x) ((x)->capa)
#define XP_AWK_STR_BUF(x)  ((x)->buf)
#define XP_AWK_STR_CHAR(x,idx) ((x)->buf[idx])

typedef struct xp_awk_str_t xp_awk_str_t;

struct xp_awk_str_t
{
	xp_char_t* buf;
	xp_size_t size;
	xp_size_t capa;
	xp_awk_t* awk;
	xp_bool_t __dynamic;
};

#ifdef __cplusplus
extern "C" {
#endif

xp_awk_str_t* xp_awk_str_open (
	xp_awk_str_t* str, xp_size_t capa, xp_awk_t* awk);

void xp_awk_str_close (xp_awk_str_t* str);

void xp_awk_str_forfeit (xp_awk_str_t* str);

xp_size_t xp_awk_str_cpy (xp_awk_str_t* str, const xp_char_t* s);

xp_size_t xp_awk_str_ncpy (
	xp_awk_str_t* str, const xp_char_t* s, xp_size_t len);

xp_size_t xp_awk_str_cat (xp_awk_str_t* str, const xp_char_t* s);

xp_size_t xp_awk_str_ncat (
	xp_awk_str_t* str, const xp_char_t* s, xp_size_t len);

xp_size_t xp_awk_str_ccat (xp_awk_str_t* str, xp_char_t c);

xp_size_t xp_awk_str_nccat (xp_awk_str_t* str, xp_char_t c, xp_size_t len);

void xp_awk_str_clear (xp_awk_str_t* str);

#ifdef __cplusplus
}
#endif

#endif
