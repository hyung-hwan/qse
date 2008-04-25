/*
 * $Id: mem.c 164 2008-04-24 13:21:17Z baconevi $
 *
 * {License}
 */

#include <ase/cmn/mem.h>

#if defined(__SPU__)
#include <spu_intrinsics.h>
#endif

/*#define IS_UNALIGNED(ptr) (((ase_size_t)ptr)%ASE_SIZEOF(ase_size_t))*/
#define IS_UNALIGNED(ptr) (((ase_size_t)ptr)&(ASE_SIZEOF(ase_size_t)-1))
#define IS_ALIGNED(ptr) (!IS_UNALIGNED(ptr))

#define IS_EITHER_UNALIGNED(ptr1,ptr2) \
	(((ase_size_t)ptr1|(ase_size_t)ptr2)&(ASE_SIZEOF(ase_size_t)-1))
#define IS_BOTH_ALIGNED(ptr1,ptr2) (!IS_EITHER_UNALIGNED(ptr1,ptr2))

void* ase_memcpy (void* dst, const void* src, ase_size_t n)
{
#if defined(ASE_BUILD_FOR_SIZE)
	ase_byte_t* d = (ase_byte_t*)dst;
	ase_byte_t* s = (ase_byte_t*)src;
	while (n-- > 0) *d++ = *s++;
	return dst;
#else
	ase_byte_t* d;
	ase_byte_t* s;

	if (n >= ASE_SIZEOF(ase_size_t) && IS_BOTH_ALIGNED(dst,src))
	{
		ase_size_t* du = (ase_size_t*)dst;
		ase_size_t* su = (ase_size_t*)src;

		while (n >= ASE_SIZEOF(ase_size_t))
		{
			*du++ = *su++;
			n -= ASE_SIZEOF(ase_size_t);
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

	/* a vector of 16 unsigned char cells */
	vector unsigned char v16;
	/* a pointer to such a vector */
	vector unsigned char* vd;
	ase_size_t rem;

	/* fills all 16 unsigned char cells with the same value 
	 * no need to use shift and bitwise-or owing to splats */
	v16 = spu_splats((ase_byte_t)val);

	/* spu SIMD instructions require 16-byte alignment */
	rem = ((ase_size_t)dst) & (ASE_SIZEOF(v16)-1);
	if (rem > 0)
	{
		ase_byte_t* d = (ase_byte_t*)dst;

		do { *d++ = (ase_byte_t)val; } 
		while (n-- > 0 && ++rem <= ASE_SIZEOF(v16));

		vd = (vector unsigned char*)d;
	}
	
	/* do the vector copy */
	while (n >= ASE_SIZEOF(v16))
	{
		*vd++ = v16;
		n += ASE_SIZEOF(v16);
	}

	{
		/* handle the trailing bytes */
		ase_byte_t* d = (ase_byte_t*)dst;
		while (n-- > 0) *d++ = (ase_byte_t)val;
	}

	return dst;
	
#else

	ase_byte_t* d = (ase_byte_t*)dst;
	ase_size_t rem;

	rem = IS_UNALIGNED(dst);
	if (rem > 0)
	{
		d = (ase_byte_t*)dst;
		do { *d++ = (ase_byte_t)val; 
printf ("unaligned...\n");
} 
		while (n-- > 0 && ++rem <= ASE_SIZEOF(ase_size_t));
	}

	if (n >= ASE_SIZEOF(ase_size_t))
	{
		ase_size_t* u = d;
		ase_size_t uv = 0;
		int i;

		if (val != 0) 
		{
			for (i = 0; i < ASE_SIZEOF(ase_size_t); i++)
				uv = (uv << 8) | (ase_byte_t)val;
		}

		while (n >= ASE_SIZEOF(ase_size_t))
		{
printf ("block...\n");
			*u++ = uv;
			n -= ASE_SIZEOF(ase_size_t);
		}

		d = (ase_byte_t*)u;
	}

printf ("unaligned %lld...\n", (long long)n);
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

#else
	const ase_byte_t* b1;
	const ase_byte_t* b2;

	if (n >= ASE_SIZEOF(ase_size_t) && IS_BOTH_ALIGNED(s1,s2))
	{
		const ase_size_t* u1 = (const ase_size_t*)s1;
		const ase_size_t* u2 = (const ase_size_t*)s2;

		while (n >= ASE_SIZEOF(ase_size_t))
		{
			if (*u1 != *u2) break;
			u1++; u2++;
			n -= ASE_SIZEOF(ase_size_t);
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
