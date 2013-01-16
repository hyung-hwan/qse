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

#include <qse/cmn/mb8.h>

qse_size_t qse_wctomb8 (qse_wchar_t wc, qse_mchar_t* utf8, qse_size_t size)
{
	if (size <= 0) return size + 1; /* buffer too small */
	if (wc > QSE_TYPE_MAX(qse_uint8_t)) return 0; /* illegal character */
	if (utf8) *(qse_uint8_t*)utf8 = wc;
	return 1;
}

qse_size_t qse_mb8towc (
	const qse_mchar_t* utf8, qse_size_t size, qse_wchar_t* wc)
{
	QSE_ASSERT (utf8 != QSE_NULL);
	QSE_ASSERT (size > 0);
	*wc = *(const qse_uint8_t*)utf8;
	return 1;
}
