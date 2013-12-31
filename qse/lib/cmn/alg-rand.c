/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
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
