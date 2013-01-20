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

#if defined(macintosh)
#	include <:qse:cmn:alg.h>
#else
#	include <qse/cmn/alg.h>
#endif

#define ENC(x) \
	((x < 26)? (QSE_MT('A') + x): \
	 (x < 52)? (QSE_MT('a') + (x - 26)): \
	 (x < 62)? (QSE_MT('0') + (x - 52)): \
	 (x == 62)? QSE_MT('+'): QSE_MT('/'))

#define DEC(x) \
	((x >= QSE_MT('A') && x <= QSE_MT('Z'))? (x - QSE_MT('A')): \
	 (x >= QSE_MT('a') && x <= QSE_MT('z'))? (x - QSE_MT('a') + 26): \
	 (x >= QSE_MT('0') && x <= QSE_MT('9'))? (x - QSE_MT('0') + 52): \
	 (x == QSE_MT('+'))? 62: 63)

qse_size_t qse_enbase64 (
	const void* in, qse_size_t isz,
	qse_mchar_t* out, qse_size_t osz, qse_size_t* xsz)
{
	const qse_uint8_t* ib = (const qse_uint8_t*)in;
	qse_size_t idx = 0, idx2 = 0, i;

	/* 3 8-bit values to 4 6-bit values */

	for (i = 0; i < isz; i += 3) 
	{
		qse_uint8_t b1, b2, b3;
		qse_uint8_t c1, c2, c3, c4;

		b1 = ib[i];
		b2 = (i + 1 < isz)? ib[i + 1]: 0;
		b3 = (i + 2 < isz)? ib[i + 2]: 0;

		c1 = b1 >> 2;
		c2 = ((b1 & 0x03) << 4) | (b2 >> 4);
		c3 = ((b2 & 0x0F) << 2) | (b3 >> 6);
		c4 = b3 & 0x3F;

		if (idx + 3 < osz) 
		{
			out[idx++] = ENC(c1);
			out[idx++] = ENC(c2);
			out[idx++] = (i + 1 < isz)? ENC(c3): QSE_MT('=');
			out[idx++] = (i + 2 < isz)? ENC(c4): QSE_MT('=');
		}
		idx2 += 4;
	}

	if (xsz) *xsz = idx2;
	return idx;
}

qse_size_t qse_debase64 (
	const qse_mchar_t* in, qse_size_t isz,
	void* out, qse_size_t osz, qse_size_t* xsz)
{
	qse_uint8_t* ob = (qse_uint8_t*)out;
	qse_size_t idx = 0, idx2 = 0, i;

	for (i = 0; i < isz; i += 4)  
	{
		qse_uint8_t c1, c2, c3, c4;
		qse_uint8_t b1, b2, b3, b4;

		c1 = in[i];
		c2 = (i + 1 < isz)? in[i + 1]: QSE_MT('A');
		c3 = (i + 2 < isz)? in[i + 2]: QSE_MT('A');
		c4 = (i + 3 < isz)? in[i + 3]: QSE_MT('A');

		b1 = DEC(c1);
		b2 = DEC(c2);
		b3 = DEC(c3);
		b4 = DEC(c4);

		idx2++;
		if (idx < osz) ob[idx++] = (b1 << 2) | (b2 >> 4);

		if (c3 != QSE_MT('=')) 
		{
			idx2++;	
			if (idx < osz) ob[idx++] = ((b2 & 0x0F) << 4) | (b3 >> 2);
		}
		if (c4 != QSE_MT('=')) 
		{
			idx2++;	
			if (idx < osz) ob[idx++] = ((b3 & 0x03) << 6) | b4;
		}
	}

	if (xsz) *xsz = idx2;
	return idx;
}
