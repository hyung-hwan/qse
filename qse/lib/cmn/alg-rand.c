/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if defined(macintosh)
#	include <:qse:cmn:alg.h>
#else
#	include <qse/cmn/alg.h>
#endif

/* Park-Miller "minimal standard" 31 bit 
 * pseudo-random number generator, implemented
 * with David G. Carta's optimisation: with
 * 32 bit math and without division.
 */
qse_uint32_t qse_rand31 (qse_uint32_t seed)
{
	qse_uint32_t hi, lo;

	if (seed == 0) seed++;

	lo = 16807 * (seed & 0xFFFF);
	hi = 16807 * (seed >> 16);

	lo += (hi & 0x7FFF) << 16;
	lo += hi >> 15;

	if (lo > 0x7FFFFFFFul) lo -= 0x7FFFFFFFul;

	return lo;
}

/*
 * Xorshift RNGs by George Marsaglia, The Florida State University
 * http://www.jstatsoft.org/v08/i14/paper
 */


#if (QSE_SIZEOF_UINT32_T > 0)
qse_uint32_t qse_randxs32 (qse_uint32_t seed)
{
	qse_uint32_t x;

	QSE_ASSERT (seed != 0);

	x = seed;

	x ^= (x << 13);
	x ^= (x >> 17); 
	x ^= (x << 5);

	return x;
}
#endif

#if (QSE_SIZEOF_UINT64_T > 0)
qse_uint64_t qse_randxs64 (qse_uint64_t seed)
{
	qse_uint64_t x;

	QSE_ASSERT (seed != 0);

	x = seed;

	x ^= (x << 21);
	x ^= (x >> 35);
	x ^= (x << 4);

	return x;
}
#endif

#if (QSE_SIZEOF_UINT128_T > 0)
qse_uint128_t qse_randxs128 (qse_uint128_t seed)
{
	qse_uint128_t x;

	QSE_ASSERT (seed != 0);

	x = seed;

	x ^= (x << 42);
	x ^= (x >> 71);
	x ^= (x << 3);

	return x;
}
#endif
