/*
 * $Id: sa.c,v 1.2 2006-01-20 15:58:42 bacon Exp $
 */

#include <xp/awk/sa.h>

#ifdef __STAND_ALONE

xp_char_t* xp_strdup (const xp_char_t* str)
{
	xp_char_t* tmp;

	tmp = (xp_char_t*)xp_malloc ((xp_strlen(str) + 1) * xp_sizeof(xp_char_t));
	if (tmp == XP_NULL) return XP_NULL;

	xp_strcpy (tmp, str);
	return tmp;
}

xp_str_t* xp_str_open (xp_str_t* str, xp_size_t capa)
{
	if (str == XP_NULL) {
		str = (xp_str_t*)xp_malloc (sizeof(xp_str_t));
		if (str == XP_NULL) return XP_NULL;
		str->__dynamic = xp_true;
	}
	else str->__dynamic = xp_false;

	str->buf = (xp_char_t*)xp_malloc (
		xp_sizeof(xp_char_t) * (capa + 1));
	if (str->buf == XP_NULL) {
		if (str->__dynamic) xp_free (str);
		return XP_NULL;
	}

	str->size      = 0;
	str->capa  = capa;
	str->buf[0] = XP_CHAR('\0');

	return str;
}

void xp_str_close (xp_str_t* str)
{
	xp_free (str->buf);
	if (str->__dynamic) xp_free (str);
}

xp_size_t xp_str_cat (xp_str_t* str, const xp_char_t* s)
{
	/* TODO: improve it */
	return xp_str_ncat (str, s, xp_strlen(s));
}

static xp_size_t __strncpy (xp_char_t* buf, const xp_char_t* str, xp_size_t len)
{
	const xp_char_t* end = str + len;
	while (str < end) *buf++ = *str++;
	*buf = XP_CHAR('\0');
	return len;
}

xp_size_t xp_str_ncat (xp_str_t* str, const xp_char_t* s, xp_size_t len)
{
	xp_char_t* buf;
	xp_size_t capa;

	if (len > str->capa - str->size) {
		capa = str->size + len;

		/* double the capa if necessary for concatenation */
		if (capa < str->capa * 2) capa = str->capa * 2;

		buf = (xp_char_t*)xp_realloc (
			str->buf, xp_sizeof(xp_char_t) * (capa + 1));
		if (buf == XP_NULL) return (xp_size_t)-1;

		str->capa = capa;
		str->buf = buf;
	}

	str->size += __strncpy (&str->buf[str->size], s, len);
	str->buf[str->size] = XP_CHAR('\0');
	return str->size;
}

xp_size_t xp_str_ccat (xp_str_t* str, xp_char_t c)
{
	return xp_str_ncat (str, &c, 1);
}

void xp_str_clear (xp_str_t* str)
{
	str->size = 0;
	str->buf[0] = XP_CHAR('\0');
}

#endif
