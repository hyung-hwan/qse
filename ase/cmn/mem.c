/*
 * $Id: mem.c,v 1.1 2007/03/28 14:05:21 bacon Exp $
 *
 * {License}
 */

#include <ase/cmn/mem.h>

void* ase_memcpy (void* dst, const void* src, ase_size_t n)
{
	void* p = dst;
	void* e = (ase_byte_t*)dst + n;

	while (dst < e) 
	{
		*(ase_byte_t*)dst = *(ase_byte_t*)src;
		dst = (ase_byte_t*)dst + 1;
		src = (ase_byte_t*)src + 1;
	}

	return p;
}

void* ase_memset (void* dst, int val, ase_size_t n)
{
	void* p = dst;
	void* e = (ase_byte_t*)p + n;

	while (p < e) 
	{
		*(ase_byte_t*)p = (ase_byte_t)val;
		p = (ase_byte_t*)p + 1;
	}

	return dst;
}

int ase_memcmp (const void* s1, const void* s2, ase_size_t n)
{
	const void* e;

	if (n == 0) return 0;

	e = (const ase_byte_t*)s1 + n - 1;
	while (s1 < e && *(ase_byte_t*)s1 == *(ase_byte_t*)s2) 
	{
		s1 = (ase_byte_t*)s1 + 1;
		s2 = (ase_byte_t*)s2 + 1;
	}

	return *((ase_byte_t*)s1) - *((ase_byte_t*)s2);
}
