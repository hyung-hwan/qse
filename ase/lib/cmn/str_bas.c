/*
 * $Id: str_bas.c 159 2008-04-01 08:37:30Z baconevi $
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
		if (*s1 == ASE_T('\0')) return 0;
		s1++, s2++;
	}

	return (*s1 > *s2)? 1: -1;
}

int ase_strxcmp (const ase_char_t* s1, ase_size_t len, const ase_char_t* s2)
{
	const ase_char_t* end = s1 + len;
	while (s1 < end && *s2 != ASE_T('\0') && *s1 == *s2) s1++, s2++;
	if (s1 == end && *s2 == ASE_T('\0')) return 0;
	if (*s1 == *s2) return (s1 < end)? 1: -1;
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

ase_char_t* ase_strbeg (const ase_char_t* str, const ase_char_t* sub)
{
	while (*sub != ASE_T('\0'))
	{
		if (*str != *sub) return ASE_NULL;
		str++; sub++;
	}

	/* returns the pointer to the next character of the match */
	return (ase_char_t*)str;
}

ase_char_t* ase_strxbeg (
	const ase_char_t* str, ase_size_t len, const ase_char_t* sub)
{
	const ase_char_t* end = str + len;

	while (*sub != ASE_T('\0'))
	{
		if (str >= end || *str != *sub) return ASE_NULL;
		str++; sub++;
	}

	/* returns the pointer to the next character of the match */
	return (ase_char_t*)str;
}

ase_char_t* ase_strnbeg (
	const ase_char_t* str, const ase_char_t* sub, ase_size_t len)
{
	const ase_char_t* end = sub + len;
		
	while (sub < end)
	{
		if (*str == ASE_T('\0') || *str != *sub) return ASE_NULL;
		str++; sub++;
	}

	/* returns the pointer to the next character of the match */
	return (ase_char_t*)str;
}

ase_char_t* ase_strxnbeg (
	const ase_char_t* str, ase_size_t len1, 
	const ase_char_t* sub, ase_size_t len2)
{
	const ase_char_t* end1, * end2;

	if (len2 > len1) return ASE_NULL;

	end1 = str + len1;
	end2 = sub + len2;

	while (sub < end2)
	{
		if (str >= end1 || *str != *sub) return ASE_NULL;
		str++; sub++;
	}

	/* returns the pointer to the next character of the match */
	return (ase_char_t*)str;
}

ase_char_t* ase_strend (const ase_char_t* str, const ase_char_t* sub)
{
	return ase_strxnend (str, ase_strlen(str), sub, ase_strlen(sub));
}

ase_char_t* ase_strxend (
	const ase_char_t* str, ase_size_t len, const ase_char_t* sub)
{
	return ase_strxnend (str, len, sub, ase_strlen(sub));
}

ase_char_t* ase_strnend (
	const ase_char_t* str, const ase_char_t* sub, ase_size_t len)
{
	return ase_strxnend (str, ase_strlen(str), sub, len);
}

ase_char_t* ase_strxnend (
	const ase_char_t* str, ase_size_t len1, 
	const ase_char_t* sub, ase_size_t len2)
{
	const ase_char_t* end1, * end2;

	if (len2 > len1) return ASE_NULL;

	end1 = str + len1;
	end2 = sub + len2;

	while (end2 > sub)
	{
		if (end1 <= str) return ASE_NULL;
		if (*(--end1) != *(--end2)) return ASE_NULL;
	}
	
	/* returns the pointer to the match start */
	return (ase_char_t*)end1;
}
