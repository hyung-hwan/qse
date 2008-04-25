/*
 * $Id: mem.c 162 2008-04-24 12:33:26Z baconevi $
 *
 * {License}
 */

#include <ase/cmn/mem.h>

/*#define IS_UNALIGNED(ptr) (((ase_size_t)ptr)%sizeof(ase_size_t))*/
#define IS_UNALIGNED(ptr) (((ase_size_t)ptr)&(sizeof(ase_size_t)-1))
#define IS_ALIGNED(ptr) (!IS_UNALIGNED(ptr))

#define IS_EITHER_UNALIGNED(ptr1,ptr2) \
	(((ase_size_t)ptr1|(ase_size_t)ptr2)&(sizeof(ase_size_t)-1))
#define IS_BOTH_ALIGNED(ptr1,ptr2) (!IS_EITHER_UNALIGNED(ptr1,ptr2))

void* ase_memcpy (void* dst, const void* src, ase_size_t n)
{
#if defined(ASE_BUILD_FOR_SIZE)
	ase_byte_t* d = (ase_byte_t*)dst;
	ase_byte_t* s = (ase_byte_t*)src;
	while (n-- > 0) *d++ = *s++;
	return dst;
#elif defined(__SPU__)
	/* cell spu */
#else
	ase_byte_t* d;
	ase_byte_t* s;

	if (n >= ASE_SIZEOF(ase_ulong_t) && IS_BOTH_ALIGNED(dst,src))
	{
		ase_ulong_t* du = (ase_ulong_t*)dst;
		ase_ulong_t* su = (ase_ulong_t*)src;

		while (n >= ASE_SIZEOF(ase_ulong_t))
		{
			*du++ = *su++;
			n -= ASE_SIZEOF(ase_ulong_t);
		}

		d = (ase_byte_t*)du;
		s = (ase_byte_t*)su;
	}
	else
	{
		d = (ase_byte_t*)dst;
		s = (ase_byte_t*)src;
	}

	while (n-- > 0) *d++ = *s++;
	return dst;
#endif
}

void* ase_memset (void* dst, int val, ase_size_t n)
{
#if defined(ASE_BUILD_FOR_SIZE)
	ase_byte_t* d = (ase_byte_t*)dst;
	while (n-- > 0) *d++ = (ase_byte_t)val;
	return dst;
#elif defined(__SPU__)
	/* cell spu */
	if ((ase_size_t)dst & (16-1))
	{
		/* the leading bytes are not aligned */
	}

#else
	ase_byte_t* d;

	if (n >= ASE_SIZEOF(ase_ulong_t) && IS_ALIGNED(dst))
	{
		ase_ulong_t* u = (ase_ulong_t*)dst;
		ase_ulong_t uv = 0;
		int i;

		if (val != 0) 
		{
			for (i = 0; i < ASE_SIZEOF(ase_ulong_t); i++)
				uv = (uv << 8) | (ase_byte_t)val;
		}

		while (n >= ASE_SIZEOF(ase_ulong_t))
		{
			*u++ = uv;
			n -= ASE_SIZEOF(ase_ulong_t);
		}

		d = (ase_byte_t*)u;
	}
	else 
	{
		d = (ase_byte_t*)dst;
	}

	while (n-- > 0) *d++ = (ase_byte_t)val;
	return dst;
#endif
}

int ase_memcmp (const void* s1, const void* s2, ase_size_t n)
{
#if defined(ASE_BUILD_FOR_SIZE)
	/*
	const void* e;

	if (n == 0) return 0;

	e = (const ase_byte_t*)s1 + n - 1;
	while (s1 < e && *(ase_byte_t*)s1 == *(ase_byte_t*)s2) 
	{
		s1 = (ase_byte_t*)s1 + 1;
		s2 = (ase_byte_t*)s2 + 1;
	}

	return *((ase_byte_t*)s1) - *((ase_byte_t*)s2);
	*/

	const ase_byte_t* b1 = (const ase_byte_t*)s1;
	const ase_byte_t* b2 = (const ase_byte_t*)s2;

	while (n-- > 0)
	{
		if (*b1 != *b2) return *b1 - *b2;
		b1++; b2++;
	}

	return 0;

#elif defined(__SPU__)
	/* cell spu */
	if ((ase_size_t)dst & (16-1))
	{
		/* the leading bytes are not aligned */
	}

#else
	const ase_byte_t* b1;
	const ase_byte_t* b2;

	if (n >= ASE_SIZEOF(ase_ulong_t) && IS_BOTH_ALIGNED(s1,s2))
	{
		const ase_ulong_t* u1 = (const ase_ulong_t*)s1;
		const ase_ulong_t* u2 = (const ase_ulong_t*)s2;

		while (n >= ASE_SIZEOF(ase_ulong_t))
		{
			if (*u1 != *u2) break;
			u1++; u2++;
			n -= ASE_SIZEOF(ase_ulong_t);
		}

		b1 = (const ase_byte_t*)u1;
		b2 = (const ase_byte_t*)u2;
	}
	else
	{
		b1 = (const ase_byte_t*)s1;
		b2 = (const ase_byte_t*)s2;
	}

	while (n-- > 0)
	{
		if (*b1 != *b2) return *b1 - *b2;
		b1++; b2++;
	}

	return 0;
#endif
}
