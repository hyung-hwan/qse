/*
 * $Id: mem.c 177 2008-04-26 04:58:10Z baconevi $
 *
 * {License}
 */

#include <ase/cmn/mem.h>

#if defined(__SPU__)
#include <spu_intrinsics.h>
#define SPU_VUC_SIZE ASE_SIZEOF(vector unsigned char)
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

#elif defined(__SPU__)

	ase_byte_t* d;
	ase_byte_t* s;

	if (n >= SPU_VUC_SIZE &&
	    (((ase_size_t)dst) & (SPU_VUC_SIZE-1)) == 0 &&
	    (((ase_size_t)src) & (SPU_VUC_SIZE-1)) == 0)
	{
		vector unsigned char* du = (vector unsigned char*)dst;
		vector unsigned char* su = (vector unsigned char*)src;

		do
		{
			*du++ = *su++;
			n -= SPU_VUC_SIZE;
		}
		while (n >= SPU_VUC_SIZE);

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

#else

	ase_byte_t* d;
	ase_byte_t* s;

	if (n >= ASE_SIZEOF(ase_size_t) && IS_BOTH_ALIGNED(dst,src))
	{
		ase_size_t* du = (ase_size_t*)dst;
		ase_size_t* su = (ase_size_t*)src;

		do
		{
			*du++ = *su++;
			n -= ASE_SIZEOF(ase_size_t);
		}
		while (n >= ASE_SIZEOF(ase_size_t));

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

void* ase_memmove (void* dst, const void* src, ase_size_t n)
{
	const ase_byte_t* sre = (const ase_byte_t*)src + n;

	if (dst <= src || dst >= (const void*)sre) 
	{
		ase_byte_t* d = (ase_byte_t*)dst;
		const ase_byte_t* s = (const ase_byte_t*)src;
		while (n-- > 0) *d++ = *s++;
	}
	else 
	{
		ase_byte_t* dse = (ase_byte_t*)dst + n;
		while (n-- > 0) *--dse = *--sre;
	}

	return dst;
}

void* ase_memset (void* dst, int val, ase_size_t n)
{
#if defined(ASE_BUILD_FOR_SIZE)

	ase_byte_t* d = (ase_byte_t*)dst;
	while (n-- > 0) *d++ = (ase_byte_t)val;
	return dst;

#elif defined(__SPU__)

	ase_byte_t* d;
	ase_size_t rem;

	if (n <= 0) return dst;

	d = (ase_byte_t*)dst;

	/* spu SIMD instructions require 16-byte alignment */
	rem = ((ase_size_t)dst) & (SPU_VUC_SIZE-1);
	if (rem > 0)
	{
		/* handle leading unaligned part */
		do { *d++ = (ase_byte_t)val; } 
		while (n-- > 0 && ++rem < SPU_VUC_SIZE);
	}
	
	/* do the vector copy */
	if (n >= SPU_VUC_SIZE)
	{
		/* a vector of 16 unsigned char cells */
		vector unsigned char v16;
		/* a pointer to such a vector */
		vector unsigned char* vd = (vector unsigned char*)d;

		/* fills all 16 unsigned char cells with the same value 
		 * no need to use shift and bitwise-or owing to splats */
		v16 = spu_splats((ase_byte_t)val);

		do
		{
			*vd++ = v16;
			n -= SPU_VUC_SIZE;
		}
		while (n >= SPU_VUC_SIZE);

		d = (ase_byte_t*)vd;
	}

	/* handle the trailing part */
	while (n-- > 0) *d++ = (ase_byte_t)val;
	return dst;
	
#else

	ase_byte_t* d;
	ase_size_t rem;

	if (n <= 0) return dst;

	d = (ase_byte_t*)dst;

	rem = IS_UNALIGNED(dst);
	if (rem > 0)
	{
		do { *d++ = (ase_byte_t)val; } 
		while (n-- > 0 && ++rem < ASE_SIZEOF(ase_size_t));
	}

	if (n >= ASE_SIZEOF(ase_size_t))
	{
		ase_size_t* u = (ase_size_t*)d;
		ase_size_t uv = 0;
		int i;

		if (val != 0) 
		{
			for (i = 0; i < ASE_SIZEOF(ase_size_t); i++)
				uv = (uv << 8) | (ase_byte_t)val;
		}

		ASE_ASSERT (IS_ALIGNED(u));
		do
		{
			*u++ = uv;
			n -= ASE_SIZEOF(ase_size_t);
		}
		while (n >= ASE_SIZEOF(ase_size_t));

		d = (ase_byte_t*)u;
	}

	while (n-- > 0) *d++ = (ase_byte_t)val;
	return dst;

#endif
}

int ase_memcmp (const void* s1, const void* s2, ase_size_t n)
{
#if defined(ASE_BUILD_FOR_SIZE)

	const ase_byte_t* b1 = (const ase_byte_t*)s1;
	const ase_byte_t* b2 = (const ase_byte_t*)s2;

	while (n-- > 0)
	{
		if (*b1 != *b2) return *b1 - *b2;
		b1++; b2++;
	}

	return 0;

#elif defined(__SPU__)

	const ase_byte_t* b1;
	const ase_byte_t* b2;

	if (n >= SPU_VUC_SIZE &&
	    (((ase_size_t)s1) & (SPU_VUC_SIZE-1)) == 0 &&
	    (((ase_size_t)s2) & (SPU_VUC_SIZE-1)) == 0)
	{
		vector unsigned char* v1 = (vector unsigned char*)s1;
		vector unsigned char* v2 = (vector unsigned char*)s2;

		vector unsigned int tmp;

		do
		{
			unsigned int cnt;
			unsigned int pat;

			/* compare 16 chars at one time */
			tmp = spu_gather(spu_cmpeq(*v1,*v2));
			/* extract the bit pattern */
			pat = spu_extract(tmp, 0);
			/* invert the bit patterns */
			pat = 0xFFFF & ~pat;

			/* put it back to the vector */
			tmp = spu_insert (pat, tmp, 0);
			/* count the leading zeros */
			cnt = spu_extract(spu_cntlz(tmp),0);
			/* 32 leading zeros mean that 
			 * all characters are the same */
			if (cnt != 32) 
			{
				/* otherwise, calculate the 
				 * unmatching pointer address */
				b1 = (const ase_byte_t*)v1 + (cnt - 16);
				b2 = (const ase_byte_t*)v2 + (cnt - 16);
				break;
			}

			v1++; v2++;
			n -= SPU_VUC_SIZE;

			if (n < SPU_VUC_SIZE)
			{
				b1 = (const ase_byte_t*)v1;
				b2 = (const ase_byte_t*)v2;
				break;
			}
		}
		while (1);
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

#else
	const ase_byte_t* b1;
	const ase_byte_t* b2;

	if (n >= ASE_SIZEOF(ase_size_t) && IS_BOTH_ALIGNED(s1,s2))
	{
		const ase_size_t* u1 = (const ase_size_t*)s1;
		const ase_size_t* u2 = (const ase_size_t*)s2;

		do
		{
			if (*u1 != *u2) break;
			u1++; u2++;
			n -= ASE_SIZEOF(ase_size_t);
		}
		while (n >= ASE_SIZEOF(ase_size_t));

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

void* ase_memchr (const void* s, int val, ase_size_t n)
{
	const ase_byte_t* x = (const ase_byte_t*)s;

	while (n-- > 0)
	{
		if (*x == (ase_byte_t)val) return (void*)x;
		x++;
	}

	return ASE_NULL;
}

void* ase_memrchr (const void* s, int val, ase_size_t n)
{
	const ase_byte_t* x = (ase_byte_t*)s + n - 1;

	while (n-- > 0)
	{
		if (*x == (ase_byte_t)val) return (void*)x;
		x--;
	}

	return ASE_NULL;
}

void* ase_memmem (const void* hs, ase_size_t hl, const void* nd, ase_size_t nl)
{
	if (nl <= hl) 
	{
		ase_size_t i;
		const ase_byte_t* h = (const ase_byte_t*)hs;

		for (i = hl - nl + 1; i > 0; i--)  
		{
			if (ase_memcmp(h, nd, nl) == 0) return (void*)h;
			h++;
		}
	}

	return ASE_NULL;
}

void* ase_memrmem (const void* hs, ase_size_t hl, const void* nd, ase_size_t nl)
{
	if (nl <= hl) 
	{
		ase_size_t i;
		const ase_byte_t* h;

		/* things are slightly more complacated 
		 * when searching backward */
		if (nl == 0) 
		{
			/* when the needle is empty, it returns
			 * the pointer to the last byte of the haystack.
			 * this is because ase_memmem returns the pointer
			 * to the first byte of the haystack when the
			 * needle is empty. but I'm not so sure if this
			 * is really desirable behavior */
			h = (const ase_byte_t*)hs + hl - 1;
			return (void*)h;
		}

		h = (const ase_byte_t*)hs + hl - nl;
		for (i = hl - nl + 1; i > 0; i--)  
		{
			if (ase_memcmp(h, nd, nl) == 0) return (void*)h;
			h--;
		}
	}

	return ASE_NULL;
}
