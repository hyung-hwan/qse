/*
 * $Id: str.c 116 2008-03-03 11:15:37Z baconevi $
 *
 * {License}
 */

#include <ase/cmn/str.h>
#include <ase/cmn/mem.h>

ase_size_t ase_strlen (const ase_char_t* str)
{
	const ase_char_t* p = str;
	while (*p != ASE_T('\0')) p++;
	return p - str;
}

ase_size_t ase_strcpy (ase_char_t* buf, const ase_char_t* str)
{
	ase_char_t* org = buf;
	while ((*buf++ = *str++) != ASE_T('\0'));
	return buf - org - 1;
}

ase_size_t ase_strxcpy (
	ase_char_t* buf, ase_size_t bsz, const ase_char_t* str)
{
	ase_char_t* p, * p2;

	p = buf; p2 = buf + bsz - 1;

	while (p < p2) 
	{
		if (*str == ASE_T('\0')) break;
		*p++ = *str++;
	}

	if (bsz > 0) *p = ASE_T('\0');
	return p - buf;
}

ase_size_t ase_strncpy (
	ase_char_t* buf, const ase_char_t* str, ase_size_t len)
{
	/*
	const ase_char_t* end = str + len;
	while (str < end) *buf++ = *str++;
	*buf = ASE_T('\0');
	return len;
	*/

	if (len > 0)
	{
		ase_size_t n = (len-1) >> 3; /* (len-1) / 8 */

		switch (len & 7) /* len % 8 */
		{
		repeat:
			case 0: *buf++ = *str++;
			case 7: *buf++ = *str++;
			case 6: *buf++ = *str++;
			case 5: *buf++ = *str++;
			case 4: *buf++ = *str++;
			case 3: *buf++ = *str++;
			case 2: *buf++ = *str++;
			case 1: *buf++ = *str++;
			        if (n <= 0) break;
			        n--;
			        goto repeat;
		}
	}

	*buf = ASE_T('\0');
	return len;
}

ase_size_t ase_strxncpy (
	ase_char_t* buf, ase_size_t bsz, const ase_char_t* str, ase_size_t len)
{
	ase_size_t n;

	if (bsz <= 0) return 0;
	if ((n = bsz - 1) > len) n = len;
	ase_memcpy (buf, str, n * ASE_SIZEOF(ase_char_t));
	buf[n] = ASE_T('\0');

	return n;
}

ase_size_t ase_strxcat (ase_char_t* buf, ase_size_t bsz, const ase_char_t* str)
{
	ase_char_t* p, * p2;
	ase_size_t blen;

	blen = ase_strlen(buf);
	if (blen >= bsz) return blen; /* something wrong */

	p = buf + blen;
	p2 = buf + bsz - 1;

	while (p < p2) 
	{
		if (*str == ASE_T('\0')) break;
		*p++ = *str++;
	}

	if (bsz > 0) *p = ASE_T('\0');
	return p - buf;
}


ase_size_t ase_strxncat (
	ase_char_t* buf, ase_size_t bsz, const ase_char_t* str, ase_size_t len)
{
	ase_char_t* p, * p2;
	const ase_char_t* end;
	ase_size_t blen;

	blen = ase_strlen(buf);
	if (blen >= bsz) return blen; /* something wrong */

	p = buf + blen;
	p2 = buf + bsz - 1;

	end = str + len;

	while (p < p2) 
	{
		if (str >= end) break;
		*p++ = *str++;
	}

	if (bsz > 0) *p = ASE_T('\0');
	return p - buf;
}

int ase_strcmp (const ase_char_t* s1, const ase_char_t* s2)
{
	while (*s1 == *s2) 
	{
		if (*s1 == ASE_C('\0')) return 0;
		s1++, s2++;
	}

	return (*s1 > *s2)? 1: -1;
}

int ase_strxncmp (
	const ase_char_t* s1, ase_size_t len1, 
	const ase_char_t* s2, ase_size_t len2)
{
	ase_char_t c1, c2;
	const ase_char_t* end1 = s1 + len1;
	const ase_char_t* end2 = s2 + len2;

	while (s1 < end1)
	{
		c1 = *s1;
		if (s2 < end2) 
		{
			c2 = *s2;
			if (c1 > c2) return 1;
			if (c1 < c2) return -1;
		}
		else return 1;
		s1++; s2++;
	}

	return (s2 < end2)? -1: 0;
}

int ase_strcasecmp (
	const ase_char_t* s1, const ase_char_t* s2, ase_ccls_t* ccls)
{
	while (ASE_TOUPPER(ccls,*s1) == ASE_TOUPPER(ccls,*s2)) 
	{
		if (*s1 == ASE_C('\0')) return 0;
		s1++, s2++;
	}

	return (ASE_TOUPPER(ccls,*s1) > ASE_TOUPPER(ccls,*s2))? 1: -1;
}

int ase_strxncasecmp (
	const ase_char_t* s1, ase_size_t len1, 
	const ase_char_t* s2, ase_size_t len2, ase_ccls_t* ccls)
{
	ase_char_t c1, c2;
	const ase_char_t* end1 = s1 + len1;
	const ase_char_t* end2 = s2 + len2;

	while (s1 < end1)
	{
		c1 = ASE_TOUPPER (ccls, *s1); 
		if (s2 < end2) 
		{
			c2 = ASE_TOUPPER (ccls, *s2);
			if (c1 > c2) return 1;
			if (c1 < c2) return -1;
		}
		else return 1;
		s1++; s2++;
	}

	return (s2 < end2)? -1: 0;
}

ase_char_t* ase_strdup (const ase_char_t* str, ase_mmgr_t* mmgr)
{
	ase_char_t* tmp;

	tmp = (ase_char_t*) ASE_MALLOC (
		mmgr, (ase_strlen(str)+1)*ASE_SIZEOF(ase_char_t));
	if (tmp == ASE_NULL) return ASE_NULL;

	ase_strcpy (tmp, str);
	return tmp;
}

ase_char_t* ase_strxdup (
	const ase_char_t* str, ase_size_t len, ase_mmgr_t* mmgr)
{
	ase_char_t* tmp;

	tmp = (ase_char_t*) ASE_MALLOC (
		mmgr, (len+1)*ASE_SIZEOF(ase_char_t));
	if (tmp == ASE_NULL) return ASE_NULL;

	ase_strncpy (tmp, str, len);
	return tmp;
}

ase_char_t* ase_strxdup2 (
	const ase_char_t* str1, ase_size_t len1,
	const ase_char_t* str2, ase_size_t len2, ase_mmgr_t* mmgr)
{
	ase_char_t* tmp;

	tmp = (ase_char_t*) ASE_MALLOC (
		mmgr, (len1+len2+1) * ASE_SIZEOF(ase_char_t));
	if (tmp == ASE_NULL) return ASE_NULL;

	ase_strncpy (tmp, str1, len1);
	ase_strncpy (tmp + len1, str2, len2);
	return tmp;
}

ase_char_t* ase_strstr (const ase_char_t* str, const ase_char_t* sub)
{
	const ase_char_t* x, * y;

	y = sub;
	if (*y == ASE_T('\0')) return (ase_char_t*)str;

	while (*str != ASE_T('\0')) 
	{
		if (*str != *y) 
		{
			str++;
			continue;
		}

		x = str;
		while (1)
		{
			if (*y == ASE_T('\0')) return (ase_char_t*)str;
			if (*x++ != *y++) break;
		}

		y = sub;
		str++;
	}

	return ASE_NULL;
}

ase_char_t* ase_strxstr (
	const ase_char_t* str, ase_size_t size, const ase_char_t* sub)
{
	return ase_strxnstr (str, size, sub, ase_strlen(sub));
}

ase_char_t* ase_strxnstr (
	const ase_char_t* str, ase_size_t strsz, 
	const ase_char_t* sub, ase_size_t subsz)
{
	const ase_char_t* end, * subp;

	if (subsz == 0) return (ase_char_t*)str;
	if (strsz < subsz) return ASE_NULL;
	
	end = str + strsz - subsz;
	subp = sub + subsz;

	while (str <= end) 
	{
		const ase_char_t* x = str;
		const ase_char_t* y = sub;

		while (ase_true) 
		{
			if (y >= subp) return (ase_char_t*)str;
			if (*x != *y) break;
			x++; y++;
		}	

		str++;
	}
		
	return ASE_NULL;
}

ase_char_t* ase_strchr (const ase_char_t* str, ase_cint_t c)
{
	while (*str != ASE_T('\0')) 
	{
		if (*str == c) return (ase_char_t*)str;
		str++;
	}
	return ASE_NULL;
}

ase_char_t* ase_strxchr (const ase_char_t* str, ase_size_t len, ase_cint_t c)
{
	const ase_char_t* end = str + len;

	while (str < end) 
	{
		if (*str == c) return (ase_char_t*)str;
		str++;
	}

	return ASE_NULL;
}

ase_char_t* ase_strrchr (const ase_char_t* str, ase_cint_t c)
{
	const ase_char_t* end = str;

	while (*end != ASE_T('\0')) end++;

	while (end > str) 
	{
		if (*--end == c) return (ase_char_t*)end;
	}

	return ASE_NULL;
}

ase_char_t* ase_strxrchr (const ase_char_t* str, ase_size_t len, ase_cint_t c)
{
	const ase_char_t* end = str + len;

	while (end > str) 
	{
		if (*--end == c) return (ase_char_t*)end;
	}

	return ASE_NULL;
}

ase_str_t* ase_str_open (ase_str_t* str, ase_size_t capa, ase_mmgr_t* mmgr)
{
	if (str == ASE_NULL) 
	{
		str = (ase_str_t*) 
			ASE_MALLOC (mmgr, ASE_SIZEOF(ase_str_t));
		if (str == ASE_NULL) return ASE_NULL;
		str->__dynamic = ase_true;
	}
	else str->__dynamic = ase_false;

	str->mmgr = mmgr;
	str->buf = (ase_char_t*) ASE_MALLOC (
		mmgr, ASE_SIZEOF(ase_char_t) * (capa + 1));
	if (str->buf == ASE_NULL) 
	{
		if (str->__dynamic) ASE_FREE (mmgr, str);
		return ASE_NULL;
	}

	str->size = 0;
	str->capa  = capa;
	str->buf[0] = ASE_T('\0');

	return str;
}

void ase_str_close (ase_str_t* str)
{
	ASE_FREE (str->mmgr, str->buf);
	if (str->__dynamic) ASE_FREE (str->mmgr, str);
}

void ase_str_clear (ase_str_t* str)
{
	str->size = 0;
	str->buf[0] = ASE_T('\0');
}

void ase_str_forfeit (ase_str_t* str)
{
	if (str->__dynamic) ASE_FREE (str->mmgr, str);
}

void ase_str_swap (ase_str_t* str, ase_str_t* str1)
{
	ase_str_t tmp;

	tmp.buf = str->buf;
	tmp.size = str->size;
	tmp.capa = str->capa;
	tmp.mmgr = str->mmgr;

	str->buf = str1->buf;
	str->size = str1->size;
	str->capa = str1->capa;
	str->mmgr = str1->mmgr;

	str1->buf = tmp.buf;
	str1->size = tmp.size;
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

	if (len > str->capa) 
	{
		buf = (ase_char_t*) ASE_MALLOC (
			str->mmgr, ASE_SIZEOF(ase_char_t) * (len + 1));
		if (buf == ASE_NULL) return (ase_size_t)-1;

		ASE_FREE (str->mmgr, str->buf);
		str->capa = len;
		str->buf = buf;
	}

	str->size = ase_strncpy (str->buf, s, len);
	str->buf[str->size] = ASE_T('\0');
	return str->size;
}

ase_size_t ase_str_cat (ase_str_t* str, const ase_char_t* s)
{
	/* TODO: improve it */
	return ase_str_ncat (str, s, ase_strlen(s));
}

ase_size_t ase_str_ncat (ase_str_t* str, const ase_char_t* s, ase_size_t len)
{
	if (len > str->capa - str->size) 
	{
		ase_char_t* tmp;
		ase_size_t capa;

		capa = str->size + len;

		/* double the capa if necessary for concatenation */
		if (capa < str->capa * 2) capa = str->capa * 2;

		if (str->mmgr->realloc != ASE_NULL)
		{
			tmp = (ase_char_t*) ASE_REALLOC (
				str->mmgr, str->buf, 
				ASE_SIZEOF(ase_char_t) * (capa + 1));
			if (tmp == ASE_NULL) return (ase_size_t)-1;
		}
		else
		{
			tmp = (ase_char_t*) ASE_MALLOC (
				str->mmgr, ASE_SIZEOF(ase_char_t)*(capa+1));
			if (tmp == ASE_NULL) return (ase_size_t)-1;
			if (str->buf != ASE_NULL)
			{
				ase_memcpy (tmp, str->buf, 
					ASE_SIZEOF(ase_char_t)*(str->capa+1));
				ASE_FREE (str->mmgr, str->buf);
			}
		}

		str->capa = capa;
		str->buf = tmp;
	}

	str->size += ase_strncpy (&str->buf[str->size], s, len);
	str->buf[str->size] = ASE_T('\0');
	return str->size;
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
	return str->size;
}

