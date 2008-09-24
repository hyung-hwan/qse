/*
 * $Id: str_dyn.c 372 2008-09-23 09:51:24Z baconevi $
 *
 * {License}
 */

#include <ase/cmn/str.h>
#include "mem.h"

ase_str_t* ase_str_open (ase_mmgr_t* mmgr, ase_size_t ext, ase_size_t capa)
{
	ase_str_t* str;

	str = (ase_str_t*) ASE_MMGR_ALLOC (mmgr, ASE_SIZEOF(ase_str_t) + ext);
	if (str == ASE_NULL) return ASE_NULL;

	if (ase_str_init (str, mmgr, capa) == ASE_NULL)
	{
		ASE_MMGR_FREE (mmgr, str);
		return ASE_NULL;
	}

	return str;
}

void ase_str_close (ase_str_t* str)
{
	ase_str_fini (str);
	ASE_MMGR_FREE (str->mmgr, str);
}

ase_str_t* ase_str_init (ase_str_t* str, ase_mmgr_t* mmgr, ase_size_t capa)
{
	ASE_MEMSET (str, 0, ASE_SIZEOF(ase_str_t));

	str->mmgr = mmgr;
	str->sizer = ASE_NULL;

	if (capa == 0) str->ptr = ASE_NULL;
	else
	{
		str->ptr = (ase_char_t*) ASE_MMGR_ALLOC (
			mmgr, ASE_SIZEOF(ase_char_t) * (capa + 1));
		if (str->ptr == ASE_NULL) return ASE_NULL;
		str->ptr[0] = ASE_T('\0');
	}

	str->len = 0;
	str->capa = capa;

	return str;
}

void ase_str_fini (ase_str_t* str)
{
	if (str->ptr != ASE_NULL) ASE_MMGR_FREE (str->mmgr, str->ptr);
}

int ase_str_yield (ase_str_t* str, ase_xstr_t* buf, int new_capa)
{
	ase_char_t* tmp;

	if (new_capa == 0) tmp = ASE_NULL;
	else
	{
		tmp = (ase_char_t*) ASE_MMGR_ALLOC (
			str->mmgr, ASE_SIZEOF(ase_char_t) * (new_capa + 1));
		if (tmp == ASE_NULL) return -1;
		tmp[0] = ASE_T('\0');
	}

	if (buf != ASE_NULL)
	{
		buf->ptr = str->ptr;
		buf->len = str->len;
	}

	str->ptr = tmp;
	str->len = 0;
	str->capa = new_capa;

	return 0;
}

ase_sizer_t ase_str_getsizer (ase_str_t* str)
{
	return str->sizer;	
}

void ase_str_setsizer (ase_str_t* str, ase_sizer_t sizer)
{
	str->sizer = sizer;
}

ase_size_t ase_str_getcapa (ase_str_t* str)
{
	return str->capa;
}

ase_size_t ase_str_setcapa (ase_str_t* str, ase_size_t capa)
{
	ase_char_t* tmp;

	if (str->mmgr->realloc != ASE_NULL && str->ptr != ASE_NULL)
	{
		tmp = (ase_char_t*) ASE_MMGR_REALLOC (
			str->mmgr, str->ptr, 
			ASE_SIZEOF(ase_char_t)*(capa+1));
		if (tmp == ASE_NULL) return (ase_size_t)-1;
	}
	else
	{
		tmp = (ase_char_t*) ASE_MMGR_ALLOC (
			str->mmgr, ASE_SIZEOF(ase_char_t)*(capa+1));
		if (tmp == ASE_NULL) return (ase_size_t)-1;

		if (str->ptr != ASE_NULL)
		{
			ase_size_t ncopy = (str->len <= capa)? str->len: capa;
			ASE_MEMCPY (tmp, str->ptr, 
				ASE_SIZEOF(ase_char_t)*(ncopy+1));
			ASE_MMGR_FREE (str->mmgr, str->ptr);
		}
	}

	if (capa < str->len)
	{
		str->len = capa;
		tmp[capa] = ASE_T('\0');
	}

	str->capa = capa;
	str->ptr = tmp;

	return str->capa;
}

void ase_str_clear (ase_str_t* str)
{
	str->len = 0;
	str->ptr[0] = ASE_T('\0');
}

void ase_str_swap (ase_str_t* str, ase_str_t* str1)
{
	ase_str_t tmp;

	tmp.ptr = str->ptr;
	tmp.len = str->len;
	tmp.capa = str->capa;
	tmp.mmgr = str->mmgr;

	str->ptr = str1->ptr;
	str->len = str1->len;
	str->capa = str1->capa;
	str->mmgr = str1->mmgr;

	str1->ptr = tmp.ptr;
	str1->len = tmp.len;
	str1->capa = tmp.capa;
	str1->mmgr = tmp.mmgr;
}

ase_size_t ase_str_cpy (ase_str_t* str, const ase_char_t* s)
{
	/* TODO: improve it */
	return ase_str_ncpy (str, s, ase_strlen(s));
}

ase_size_t ase_str_ncpy (ase_str_t* str, const ase_char_t* s, ase_size_t len)
{
	ase_char_t* buf;

	if (len > str->capa || str->ptr == ASE_NULL) 
	{
		buf = (ase_char_t*) ASE_MMGR_ALLOC (
			str->mmgr, ASE_SIZEOF(ase_char_t) * (len + 1));
		if (buf == ASE_NULL) return (ase_size_t)-1;

		if (str->ptr != ASE_NULL) ASE_MMGR_FREE (str->mmgr, str->ptr);
		str->capa = len;
		str->ptr = buf;
	}

	str->len = ase_strncpy (str->ptr, s, len);
	str->ptr[str->len] = ASE_T('\0');
	return str->len;
}

ase_size_t ase_str_cat (ase_str_t* str, const ase_char_t* s)
{
	/* TODO: improve it */
	return ase_str_ncat (str, s, ase_strlen(s));
}


ase_size_t ase_str_ncat (ase_str_t* str, const ase_char_t* s, ase_size_t len)
{
	if (len > str->capa - str->len) 
	{
		ase_size_t ncapa;

		if (str->sizer == ASE_NULL)
		{
			/* increase the capacity by the length to add */
			ncapa = str->len + len;
			/* if the new capacity is less than the double,
			 * just double it */
			if (ncapa < str->capa * 2) ncapa = str->capa * 2;
		}
		else
		{
			/* let the user determine the new capacity.
			 * pass the minimum capacity required as a hint */
			ncapa = str->sizer (str, str->len + len);
		}

		if (ase_str_setcapa (str, ncapa) == (ase_size_t)-1) 
		{
			return (ase_size_t)-1;
		}
	}

	if (len > str->capa - str->len) 
	{
		/* copy as many characters as the number of cells available */
		len = str->capa - str->len;
	}

	if (len > 0)
	{
		ASE_MEMCPY (&str->ptr[str->len], s, len*ASE_SIZEOF(*s));
		str->len += len;
		str->ptr[str->len] = ASE_T('\0');
	}
	return str->len;
}

ase_size_t ase_str_ccat (ase_str_t* str, ase_char_t c)
{
	return ase_str_ncat (str, &c, 1);
}

ase_size_t ase_str_nccat (ase_str_t* str, ase_char_t c, ase_size_t len)
{
	while (len > 0)
	{
		if (ase_str_ncat (str, &c, 1) == (ase_size_t)-1) 
		{
			return (ase_size_t)-1;
		}

		len--;
	}
	return str->len;
}

