/*
 * $Id: str.c,v 1.9 2006-10-22 11:34:53 bacon Exp $
 */

#include <sse/awk/awk_i.h>

sse_awk_str_t* sse_awk_str_open (
	sse_awk_str_t* str, sse_size_t capa, sse_awk_t* awk)
{
	if (str == SSE_NULL) 
	{
		str = (sse_awk_str_t*) SSE_AWK_MALLOC (awk, sizeof(sse_awk_str_t));
		if (str == SSE_NULL) return SSE_NULL;
		str->__dynamic = sse_true;
	}
	else str->__dynamic = sse_false;

	str->awk = awk;
	str->buf = (sse_char_t*) SSE_AWK_MALLOC (
		awk, sse_sizeof(sse_char_t) * (capa + 1));
	if (str->buf == SSE_NULL) 
	{
		if (str->__dynamic) SSE_AWK_FREE (awk, str);
		return SSE_NULL;
	}

	str->size = 0;
	str->capa  = capa;
	str->buf[0] = SSE_T('\0');

	return str;
}

void sse_awk_str_close (sse_awk_str_t* str)
{
	SSE_AWK_FREE (str->awk, str->buf);
	if (str->__dynamic) SSE_AWK_FREE (str->awk, str);
}

void sse_awk_str_forfeit (sse_awk_str_t* str)
{
	if (str->__dynamic) SSE_AWK_FREE (str->awk, str);
}

void sse_awk_str_swap (sse_awk_str_t* str, sse_awk_str_t* str1)
{
	sse_awk_str_t tmp;

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

sse_size_t sse_awk_str_cpy (sse_awk_str_t* str, const sse_char_t* s)
{
	/* TODO: improve it */
	return sse_awk_str_ncpy (str, s, sse_awk_strlen(s));
}

sse_size_t sse_awk_str_ncpy (sse_awk_str_t* str, const sse_char_t* s, sse_size_t len)
{
	sse_char_t* buf;

	if (len > str->capa) 
	{
		buf = (sse_char_t*) SSE_AWK_MALLOC (
			str->awk, sse_sizeof(sse_char_t) * (len + 1));
		if (buf == SSE_NULL) return (sse_size_t)-1;

		SSE_AWK_FREE (str->awk, str->buf);
		str->capa = len;
		str->buf = buf;
	}

	str->size = sse_awk_strncpy (str->buf, s, len);
	str->buf[str->size] = SSE_T('\0');
	return str->size;
}

sse_size_t sse_awk_str_cat (sse_awk_str_t* str, const sse_char_t* s)
{
	/* TODO: improve it */
	return sse_awk_str_ncat (str, s, sse_awk_strlen(s));
}

sse_size_t sse_awk_str_ncat (sse_awk_str_t* str, const sse_char_t* s, sse_size_t len)
{
	if (len > str->capa - str->size) 
	{
		sse_char_t* tmp;
		sse_size_t capa;

		capa = str->size + len;

		/* double the capa if necessary for concatenation */
		if (capa < str->capa * 2) capa = str->capa * 2;

		if (str->awk->syscas.realloc != SSE_NULL)
		{
			tmp = (sse_char_t*) SSE_AWK_REALLOC (
				str->awk, str->buf, 
				sse_sizeof(sse_char_t) * (capa + 1));
			if (tmp == SSE_NULL) return (sse_size_t)-1;
		}
		else
		{
			tmp = (sse_char_t*) SSE_AWK_MALLOC (
				str->awk, sse_sizeof(sse_char_t) * (capa + 1));
			if (tmp == SSE_NULL) return (sse_size_t)-1;
			if (str->buf != SSE_NULL)
			{
				SSE_AWK_MEMCPY (str->awk, tmp, str->buf, 
					sse_sizeof(sse_char_t) * (str->capa + 1));
				SSE_AWK_FREE (str->awk, str->buf);
			}
		}

		str->capa = capa;
		str->buf = tmp;
	}

	str->size += sse_awk_strncpy (&str->buf[str->size], s, len);
	str->buf[str->size] = SSE_T('\0');
	return str->size;
}

sse_size_t sse_awk_str_ccat (sse_awk_str_t* str, sse_char_t c)
{
	return sse_awk_str_ncat (str, &c, 1);
}

sse_size_t sse_awk_str_nccat (sse_awk_str_t* str, sse_char_t c, sse_size_t len)
{
	while (len > 0)
	{
		if (sse_awk_str_ncat (str, &c, 1) == (sse_size_t)-1) 
		{
			return (sse_size_t)-1;
		}

		len--;
	}
	return str->size;
}

void sse_awk_str_clear (sse_awk_str_t* str)
{
	str->size = 0;
	str->buf[0] = SSE_T('\0');
}

