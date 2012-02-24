/*
 * $Id$
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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

#include <qse/cmn/cp949.h>
#include "cp949.h"

qse_size_t qse_uctocp949 (qse_wchar_t uc, qse_mchar_t* cp949, qse_size_t size)
{
	if (uc & ~(qse_wchar_t)0x7F)
	{

		if (uc >= 0xffffu) return 0; /* illegal character */
		if (size >= 2)
		{
			qse_uint16_t mb;

			mb = wctomb (uc);
			if (mb == 0xffffu) return 0; /* illegal character */

			cp949[0] = (mb >> 8);
			cp949[1] = (mb & 0xFF);
		}

		return 2;
	}
	else
	{	
		/* uc >= 0 && uc <= 127 */
		if (size >= 1) *cp949 = uc;
		return 1; /* ok or buffer to small */
	}
}

qse_size_t qse_cp949touc (
	const qse_mchar_t* cp949, qse_size_t size, qse_wchar_t* uc)
{
	QSE_ASSERT (cp949 != QSE_NULL);
	QSE_ASSERT (size > 0);
	QSE_ASSERT (QSE_SIZEOF(qse_mchar_t) == 1);
	QSE_ASSERT (QSE_SIZEOF(qse_wchar_t) >= 2);

	if (cp949[0] & 0x80)
	{
		if (size >= 2)
		{
			qse_uint16_t wc;	
			wc = mbtowc ((((qse_uint16_t)(qse_uint8_t)cp949[0]) << 8) | (qse_uint8_t)cp949[1]);
			if (wc == 0xffffu) return 0; /* illegal sequence */
			if (uc) *uc = wc;
		}
		return 2; /* ok or incomplete sequence */
	}
	else
	{
		if (uc) *uc = cp949[0];
		return 1;
	}
}

qse_size_t qse_cp949len (const qse_mchar_t* cp949, qse_size_t size)
{
	return qse_cp949touc (cp949, size, QSE_NULL);
}

qse_size_t qse_cp949lenmax (void)
{
	return QSE_CP949LEN_MAX;
}

