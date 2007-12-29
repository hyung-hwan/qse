/*
 * $Id: mem.c,v 1.3 2007/04/30 05:55:36 bacon Exp $
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
	/*
	void* p = dst;
	void* e = (ase_byte_t*)dst + n;

	while (dst < e) 
	{
		*(ase_byte_t*)dst = *(ase_byte_t*)src;
		dst = (ase_byte_t*)dst + 1;
		src = (ase_byte_t*)src + 1;
	}

	return p;
	*/

	void* p = dst;
	void* e = (ase_byte_t*)dst + n;

	ASE_ASSERT (sizeof(ase_size_t) == sizeof(void*));

	/*if (IS_ALIGNED(dst) && IS_ALIGNED(src))*/
	if (IS_BOTH_ALIGNED(dst,src))
	{
		/* if both src and dst are aligned, 
		 * blockcopy sizeof(void*) bytes. */

	#if (ASE_SIZEOF_VOID_P==2)
		ase_size_t count = n >> 1;
	#elif (ASE_SIZEOF_VOID_P==4)
		ase_size_t count = n >> 2;
	#elif (ASE_SIZEOF_VOID_P==8)
		ase_size_t count = n >> 3;
	#else
		ase_size_t count = n / sizeof(dst);
	#endif

		while (count >= 4)
		{
			*(void**)dst = *(void**)src;
			dst = (void**)dst + 1;
			src = (void**)src + 1;

			*(void**)dst = *(void**)src;
			dst = (void**)dst + 1;
			src = (void**)src + 1;

			*(void**)dst = *(void**)src;
			dst = (void**)dst + 1;
			src = (void**)src + 1;

			*(void**)dst = *(void**)src;
			dst = (void**)dst + 1;
			src = (void**)src + 1;

			count -= 4;
		}

		while (count > 0)
		{
			*(void**)dst = *(void**)src;
			dst = (void**)dst + 1;
			src = (void**)src + 1;
			count--;
		}
	}

	/* bytecopy for remainders or unaligned data */
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

	register const ase_byte_t* b1 = (const ase_byte_t*)s1;
	register const ase_byte_t* b2 = (const ase_byte_t*)s2;

	while (n > 0)
	{
		n--;
		if (*b1++ != *b2++) return *b1 - *b2;
	}

	return 0;
}
