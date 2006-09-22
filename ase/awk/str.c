/*
 * $Id: str.c,v 1.5 2006-09-22 14:04:26 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/assert.h>
#endif

xp_awk_str_t* xp_awk_str_open (
	xp_awk_str_t* str, xp_size_t capa, xp_awk_t* awk)
{
	if (str == XP_NULL) 
	{
		str = (xp_awk_str_t*) XP_AWK_MALLOC (awk, sizeof(xp_awk_str_t));
		if (str == XP_NULL) return XP_NULL;
		str->__dynamic = xp_true;
	}
	else str->__dynamic = xp_false;

	str->awk = awk;
	str->buf = (xp_char_t*) XP_AWK_MALLOC (
		awk, xp_sizeof(xp_char_t) * (capa + 1));
	if (str->buf == XP_NULL) 
	{
		if (str->__dynamic) XP_AWK_FREE (awk, str);
		return XP_NULL;
	}

	str->size = 0;
	str->capa  = capa;
	str->buf[0] = XP_T('\0');

	return str;
}

void xp_awk_str_close (xp_awk_str_t* str)
{
	XP_AWK_FREE (str->awk, str->buf);
	if (str->__dynamic) XP_AWK_FREE (str->awk, str);
}

void xp_awk_str_forfeit (xp_awk_str_t* str)
{
	if (str->__dynamic) XP_AWK_FREE (str->awk, str);
}

void xp_awk_str_swap (xp_awk_str_t* str, xp_awk_str_t* str1)
{
	xp_awk_str_t tmp;

	tmp.buf = str->buf;
	tmp.size = str->size;
	tmp.capa = str->capa;
	tmp.awk = str->awk;

	str->buf = str1->buf;
	str->size = str1->size;
	str->capa = str1->capa;
	str->awk = str1->awk;

	str1->buf = tmp.buf;
	str1->size = tmp.size;
	str1->capa = tmp.capa;
	str1->awk = tmp.awk;
}

xp_size_t xp_awk_str_cpy (xp_awk_str_t* str, const xp_char_t* s)
{
	/* TODO: improve it */
	return xp_awk_str_ncpy (str, s, xp_awk_strlen(s));
}

xp_size_t xp_awk_str_ncpy (xp_awk_str_t* str, const xp_char_t* s, xp_size_t len)
{
	xp_char_t* buf;

	if (len > str->capa) 
	{
		buf = (xp_char_t*) XP_AWK_MALLOC (
			str->awk, xp_sizeof(xp_char_t) * (len + 1));
		if (buf == XP_NULL) return (xp_size_t)-1;

		XP_AWK_FREE (str->awk, str->buf);
		str->capa = len;
		str->buf = buf;
	}

	str->size = xp_awk_strncpy (str->buf, s, len);
	str->buf[str->size] = XP_T('\0');
	return str->size;
}

xp_size_t xp_awk_str_cat (xp_awk_str_t* str, const xp_char_t* s)
{
	/* TODO: improve it */
	return xp_awk_str_ncat (str, s, xp_awk_strlen(s));
}

xp_size_t xp_awk_str_ncat (xp_awk_str_t* str, const xp_char_t* s, xp_size_t len)
{
	if (len > str->capa - str->size) 
	{
		xp_char_t* tmp;
		xp_size_t capa;

		capa = str->size + len;

		/* double the capa if necessary for concatenation */
		if (capa < str->capa * 2) capa = str->capa * 2;

		if (str->awk->syscas->realloc != XP_NULL)
		{
			tmp = (xp_char_t*) XP_AWK_REALLOC (
				str->awk, str->buf, 
				xp_sizeof(xp_char_t) * (capa + 1));
			if (tmp == XP_NULL) return (xp_size_t)-1;
		}
		else
		{
			tmp = (xp_char_t*) XP_AWK_MALLOC (
				str->awk, xp_sizeof(xp_char_t) * (capa + 1));
			if (tmp == XP_NULL) return (xp_size_t)-1;
			if (str->buf != XP_NULL)
			{
				xp_memcpy (tmp, str->buf, 
					xp_sizeof(xp_char_t) * (str->capa + 1));
				XP_AWK_FREE (str->awk, str->buf);
			}
		}

		str->capa = capa;
		str->buf = tmp;
	}

	str->size += xp_awk_strncpy (&str->buf[str->size], s, len);
	str->buf[str->size] = XP_T('\0');
	return str->size;
}

xp_size_t xp_awk_str_ccat (xp_awk_str_t* str, xp_char_t c)
{
	return xp_awk_str_ncat (str, &c, 1);
}

xp_size_t xp_awk_str_nccat (xp_awk_str_t* str, xp_char_t c, xp_size_t len)
{
	while (len > 0)
	{
		if (xp_awk_str_ncat (str, &c, 1) == (xp_size_t)-1) 
		{
			return (xp_size_t)-1;
		}

		len--;
	}
	return str->size;
}

void xp_awk_str_clear (xp_awk_str_t* str)
{
	str->size = 0;
	str->buf[0] = XP_T('\0');
}

