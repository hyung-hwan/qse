/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <qse/cmn/alg.h>

#define qsort_min(a,b) (((a)<(b))? a: b)

/*
 * Qsort routine from Bentley & McIlroy's "Engineering a Sort Function".
 */
#define swapcode(TYPE,parmi,parmj,n) { \
	qse_size_t i = (n) / sizeof (TYPE); \
	register TYPE *pi = (TYPE*)(parmi); \
	register TYPE *pj = (TYPE*)(parmj); \
	do { 						\
		register TYPE t = *pi;	\
		*pi++ = *pj;	\
		*pj++ = t;		\
	} while (--i > 0);	\
}

#define SWAPINIT(a,size) \
	swaptype = ((qse_byte_t*)a-(qse_byte_t*)0)%sizeof(long) || \
	           size % sizeof(long) ? 2 : size == sizeof(long)? 0 : 1;

static QSE_INLINE void swapfunc (
	qse_byte_t* a, qse_byte_t* b, int n, int swaptype)
{
	if (swaptype <= 1) 
		swapcode(long, a, b, n)
	else
		swapcode(qse_byte_t, a, b, n)
}

#define swap(a, b) \
	if (swaptype == 0) { \
		long t = *(long*)(a); \
		*(long*)(a) = *(long*)(b); \
		*(long*)(b) = t; \
	} else swapfunc(a, b, size, swaptype)

#define vecswap(a,b,n) if ((n) > 0) swapfunc(a, b, n, swaptype)

static QSE_INLINE qse_byte_t* med3 (
	qse_byte_t* a, qse_byte_t* b, qse_byte_t* c, 
	qse_sort_comper_t comper, void* ctx)
{
	if (comper(a, b, ctx) < 0) 
	{
		if (comper(b, c, ctx) < 0) return b;
		return (comper(a, c, ctx) < 0)? c: a;
	}
	else 
	{
		if (comper(b, c, ctx) > 0) return b;
		return (comper(a, c, ctx) > 0)? c: a;
	}
}

void qse_qsort (
	void* base, qse_size_t nmemb, qse_size_t size, 
	qse_sort_comper_t comper, void* ctx)
{
	qse_byte_t*pa, *pb, *pc, *pd, *pl, *pm, *pn;
	int swaptype, swap_cnt;
	long r;
	qse_size_t d;
	register qse_byte_t*a = (qse_byte_t*)base;

loop:	
	SWAPINIT(a, size);

	swap_cnt = 0;
	if (nmemb < 7) 
	{
		for (pm = (qse_byte_t*)a + size;
		     pm < (qse_byte_t*) a + nmemb * size; pm += size)
		{
			for (pl = pm; pl > (qse_byte_t*)a &&
			              comper(pl - size, pl, ctx) > 0; pl -= size)
			{
				swap(pl, pl - size);
			}
		}
		return;
	}
	pm = (qse_byte_t*)a + (nmemb / 2) * size;
	if (nmemb > 7) 
	{
		pl = (qse_byte_t*)a;
		pn = (qse_byte_t*)a + (nmemb - 1) * size;
		if (nmemb > 40) 
		{
			d = (nmemb / 8) * size;
			pl = med3(pl, pl + d, pl + 2 * d, comper, ctx);
			pm = med3(pm - d, pm, pm + d, comper, ctx);
			pn = med3(pn - 2 * d, pn - d, pn, comper, ctx);
		}
		pm = med3(pl, pm, pn, comper, ctx);
	}
	swap(a, pm);
	pa = pb = (qse_byte_t*)a + size;

	pc = pd = (qse_byte_t*)a + (nmemb - 1) * size;
	for (;;) 
	{
		while (pb <= pc && (r = comper(pb, a, ctx)) <= 0) 
		{
			if (r == 0) 
			{
				swap_cnt = 1;
				swap(pa, pb);
				pa += size;
			}
			pb += size;
		}
		while (pb <= pc && (r = comper(pc, a, ctx)) >= 0) 
		{
			if (r == 0) 
			{
				swap_cnt = 1;
				swap(pc, pd);
				pd -= size;
			}
			pc -= size;
		}
		if (pb > pc) break;
		swap (pb, pc);
		swap_cnt = 1;
		pb += size;
		pc -= size;
	}

	if (swap_cnt == 0) 
	{
		 /* switch to insertion sort */
		for (pm = (qse_byte_t*)a + size; 
		     pm < (qse_byte_t*)a + nmemb * size; pm += size)
		{
			for (pl = pm; pl > (qse_byte_t*)a && 
			              comper(pl - size, pl, ctx) > 0; pl -= size)
			{
				swap(pl, pl - size);
			}
		}
		return;
	}

	pn = (qse_byte_t*)a + nmemb * size;
	r = qsort_min(pa - (qse_byte_t*)a, pb - pa);
	vecswap (a, pb - r, r);
	r = qsort_min (pd - pc, pn - pd - size);
	vecswap (pb, pn - r, r);

	if ((r = pb - pa) > size) qse_qsort(a, r / size, size, comper, ctx);

	if ((r = pd - pc) > size) 
	{
		/* Iterate rather than recurse to save stack space */
		a = pn - r;
		nmemb = r / size;
		goto loop;
	}
/*	qsort(pn - r, r / size, size, comper);*/
}

#if 0

/* 
 * Below is an example of a naive qsort implementation
 */

#define swap(a,b,size) \
	do  { \
		qse_size_t i = 0; \
		qse_byte_t* p1 = (a); \
		qse_byte_t* p2 = (b); \
		for (i = 0; i < size; i++) { \
			qse_byte_t t = *p1; \
			*p1 = *p2; \
			*p2 = t; \
			p1++; p2++; \
		} \
	} while (0)

#define REF(x,i) (&((x)[(i)*size]))

void qse_qsort (void* base, qse_size_t nmemb, qse_size_t size, void* arg,
	int (*compar)(const void*, const void*, void*))
{
	qse_size_t pivot, start, end;
	qse_byte_t* p = (qse_byte_t*)base;

	if (nmemb <= 1) return;
	if (nmemb == 2) {
		if (compar(REF(p,0), REF(p,1), arg) > 0)
			swap (REF(p,0), REF(p,1), size);
		return;
	}

	pivot = nmemb >> 1; /* choose the middle as the pivot index */
	swap (REF(p,pivot), REF(p,nmemb-1), size); /* swap the pivot with the last item */

	start = 0; end = nmemb - 2; 

	while (1) {
		/* look for the larger value than pivot */
		while (start <= end && 
		       compar(REF(p,start), REF(p,nmemb-1), arg) <= 0) start++;

		/* look for the less value than pivot. */
		while (end > start && 
		       compar(REF(p,end), REF(p,nmemb-1), arg) >= 0) end--;

		if (start >= end) break; /* no more to swap */
		swap (REF(p,start), REF(p,end), size);
		start++; end--;
	}

	swap (REF(p,nmemb-1), REF(p,start), size);
	pivot = start;  /* adjust the pivot index */

	qse_qsort (REF(p,0), pivot, size, arg, compar);
	qse_qsort (REF(p,pivot+1), nmemb - pivot - 1, size, arg, compar);
}

/* 
 * For easier understanding, see the following
 */

#define swap(a, b) do { a ^= b; b ^= a; a ^= b; } while (0)

void qsort (int* x, size_t n)
{
	size_t index, start, end;
	int pivot;

	if (n <= 1) return;
	if (n == 2) {
		if (x[0] > x[1]) swap (x[0], x[1]);
		return;
	}

	index = n / 2; /* choose the middle as the pivot index */
	pivot = x[index]; /* store the pivot value */
	swap (x[index], x[n - 1]); /* swap the pivot with the last item */

	start = 0; end = n - 2; 
	while (1) {
		/* look for the larger value than pivot */
		while (start <= end && x[start] <= pivot) start++;

		/* look for the less value than pivot. */
		while (end > start && x[end] >= pivot) end--;

		if (start >= end) {
			/* less values all on the left, 
			 * larger values all on the right
			 */
			break;
		}

		swap (x[start], x[end]);
		start++; end--;
	}

	x[n - 1] = x[start]; /* restore the pivot value */
	x[start] = pivot;
	index = start;  /* adjust the pivot index */

	qsort (x, index);
	qsort (&x[index + 1], n - index - 1);
}

#endif
