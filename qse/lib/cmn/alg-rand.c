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

#include <qse/cmn/alg.h>

/* Park-Miller "minimal standard" 31 bit 
 * pseudo-random number generator, implemented
 * with David G. Carta's optimisation: with
 * 32 bit math and wihtout division.
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
