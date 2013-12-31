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

void* qse_bsearch (
	const void *key, const void *base, qse_size_t nmemb,
	qse_size_t size, qse_search_comper_t comper, void* ctx)
{
	int n;
	const void* mid;
	qse_size_t lim;

	for (lim = nmemb; lim > 0; lim >>= 1)
	{
		mid = ((qse_byte_t*)base) + (lim >> 1) * size;

		n = (comper) (key, mid, ctx);
		if (n == 0) return (void*)mid;
		if (n > 0) { base = ((const qse_byte_t*)mid) + size; lim--; }
	}

	return QSE_NULL;
}

void* qse_lsearch (
	const void* key, const void* base, qse_size_t nmemb,
	qse_size_t size, qse_search_comper_t comper, void* ctx)
{
	while (nmemb > 0) 
	{
		if (comper(key, base, ctx) == 0) return (void*)base;
		base = ((const qse_byte_t*)base) + size;
		nmemb--;
	}

	return QSE_NULL;
}
