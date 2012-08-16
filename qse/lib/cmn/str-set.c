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

#include <qse/cmn/str.h>


qse_size_t qse_mbsset (qse_mchar_t* buf, qse_mchar_t c, qse_size_t n)
{
	qse_size_t len = n;
	buf[n] = QSE_MT('\0');
	while (n > 0) buf[--n] = c;
	return len;
}

qse_size_t qse_mbsxset (
	qse_mchar_t* buf, qse_size_t bsz, qse_mchar_t c, qse_size_t n)
{
	qse_size_t len;

	if (bsz <= 0) return 0;

	if (n >= bsz) n = bsz - 1;
	len = n;

	buf[n] = QSE_MT('\0');
	while (n > 0) buf[--n] = c;
	return len;
}

qse_size_t qse_wcsset (qse_wchar_t* buf, qse_wchar_t c, qse_size_t n)
{
	qse_size_t len = n;
	buf[n] = QSE_WT('\0');
	while (n > 0) buf[--n] = c;
	return len;
}

qse_size_t qse_wcsxset (
	qse_wchar_t* buf, qse_size_t bsz, qse_wchar_t c, qse_size_t n)
{
	qse_size_t len;

	if (bsz <= 0) return 0;

	if (n >= bsz) n = bsz - 1;
	len = n;

	buf[n] = QSE_WT('\0');
	while (n > 0) buf[--n] = c;
	return len;
}
