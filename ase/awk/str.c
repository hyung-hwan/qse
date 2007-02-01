/*
 * $Id: str.c,v 1.15 2007-02-01 08:38:24 bacon Exp $
 */

#include <ase/awk/awk_i.h>

ase_awk_str_t* ase_awk_str_open (
	ase_awk_str_t* str, ase_size_t capa, ase_awk_t* awk)
{
	if (str == ASE_NULL) 
	{
		str = (ase_awk_str_t*) 
			ASE_AWK_MALLOC (awk, ASE_SIZEOF(ase_awk_str_t));
		if (str == ASE_NULL) return ASE_NULL;
		str->__dynamic = ase_true;
	}
	else str->__dynamic = ase_false;

	str->awk = awk;
	str->buf = (ase_char_t*) ASE_AWK_MALLOC (
		awk, ASE_SIZEOF(ase_char_t) * (capa + 1));
	if (str->buf == ASE_NULL) 
	{
		if (str->__dynamic) ASE_AWK_FREE (awk, str);
		return ASE_NULL;
	}

	str->size = 0;
	str->capa  = capa;
	str->buf[0] = ASE_T('\0');

	return str;
}

void ase_awk_str_close (ase_awk_str_t* str)
{
	ASE_AWK_FREE (str->awk, str->buf);
	if (str->__dynamic) ASE_AWK_FREE (str->awk, str);
}

void ase_awk_str_forfeit (ase_awk_str_t* str)
{
	if (str->__dynamic) ASE_AWK_FREE (str->awk, str);
}

void ase_awk_str_swap (ase_awk_str_t* str, ase_awk_str_t* str1)
{
	ase_awk_str_t tmp;

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

ase_size_t ase_awk_str_cpy (ase_awk_str_t* str, const ase_char_t* s)
{
	/* TODO: improve it */
	return ase_awk_str_ncpy (str, s, ase_awk_strlen(s));
}

ase_size_t ase_awk_str_ncpy (
	ase_awk_str_t* str, const ase_char_t* s, ase_size_t len)
{
	ase_char_t* buf;

	if (len > str->capa) 
	{
		buf = (ase_char_t*) ASE_AWK_MALLOC (
			str->awk, ASE_SIZEOF(ase_char_t) * (len + 1));
		if (buf == ASE_NULL) return (ase_size_t)-1;

		ASE_AWK_FREE (str->awk, str->buf);
		str->capa = len;
		str->buf = buf;
	}

	str->size = ase_awk_strncpy (str->buf, s, len);
	str->buf[str->size] = ASE_T('\0');
	return str->size;
}

ase_size_t ase_awk_str_cat (ase_awk_str_t* str, const ase_char_t* s)
{
	/* TODO: improve it */
	return ase_awk_str_ncat (str, s, ase_awk_strlen(s));
}

ase_size_t ase_awk_str_ncat (
	ase_awk_str_t* str, const ase_char_t* s, ase_size_t len)
{
	if (len > str->capa - str->size) 
	{
		ase_char_t* tmp;
		ase_size_t capa;

		capa = str->size + len;

		/* double the capa if necessary for concatenation */
		if (capa < str->capa * 2) capa = str->capa * 2;

		if (str->awk->prmfns.realloc != ASE_NULL)
		{
			tmp = (ase_char_t*) ASE_AWK_REALLOC (
				str->awk, str->buf, 
				ASE_SIZEOF(ase_char_t) * (capa + 1));
			if (tmp == ASE_NULL) return (ase_size_t)-1;
		}
		else
		{
			tmp = (ase_char_t*) ASE_AWK_MALLOC (
				str->awk, ASE_SIZEOF(ase_char_t) * (capa + 1));
			if (tmp == ASE_NULL) return (ase_size_t)-1;
			if (str->buf != ASE_NULL)
			{
				ASE_AWK_MEMCPY (str->awk, tmp, str->buf, 
					ASE_SIZEOF(ase_char_t) * (str->capa + 1));
				ASE_AWK_FREE (str->awk, str->buf);
			}
		}

		str->capa = capa;
		str->buf = tmp;
	}

	str->size += ase_awk_strncpy (&str->buf[str->size], s, len);
	str->buf[str->size] = ASE_T('\0');
	return str->size;
}

ase_size_t ase_awk_str_ccat (ase_awk_str_t* str, ase_char_t c)
{
	return ase_awk_str_ncat (str, &c, 1);
}

ase_size_t ase_awk_str_nccat (ase_awk_str_t* str, ase_char_t c, ase_size_t len)
{
	while (len > 0)
	{
		if (ase_awk_str_ncat (str, &c, 1) == (ase_size_t)-1) 
		{
			return (ase_size_t)-1;
		}

		len--;
	}
	return str->size;
}

void ase_awk_str_clear (ase_awk_str_t* str)
{
	str->size = 0;
	str->buf[0] = ASE_T('\0');
}

