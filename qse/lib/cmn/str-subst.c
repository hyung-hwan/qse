/*
 * $Id: str-subst.c 556 2011-08-31 15:43:46Z hyunghwan.chung $
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

#include <qse/cmn/str.h>

qse_size_t qse_mbsxsubst (
	qse_mchar_t* buf, qse_size_t bsz, const qse_mchar_t* fmt, 
	qse_mbsxsubst_subst_t subst, void* ctx)
{
	qse_mchar_t* b = buf;
	qse_mchar_t* end = buf + bsz - 1;
	const qse_mchar_t* f = fmt;

	if (bsz <= 0) return 0;

	while (*f != QSE_MT('\0'))
	{
		if (*f == QSE_MT('\\'))
		{
			// get the escaped character and treat it normally.
			// if the escaper is the last character, treat it 
			// normally also.
			if (f[1] != QSE_MT('\0')) f++;
		}
		else if (*f == QSE_MT('$'))
		{
			if (f[1] == QSE_MT('{'))
			{
				const qse_mchar_t* tmp;
				qse_mcstr_t ident;

				f += 2; /* skip ${ */ 
				tmp = f; /* mark the beginning */

				/* scan an enclosed segment */
				while (*f != QSE_MT('\0') && *f != QSE_MT('}')) f++;
	
				if (*f != QSE_MT('}'))
				{
					/* restore to the position of $ */
					f = tmp - 2;
					goto normal;
				}

				f++; /* skip } */
			
				ident.ptr = tmp;
				ident.len = f - tmp - 1;

				b = subst (b, end - b, &ident, ctx);
				if (b >= end) goto fini;

				continue;
			}
			else if (f[1] == QSE_MT('$')) f++;
		}

	normal:
		if (b >= end) break;
		*b++ = *f++;
	}

fini:
	*b = QSE_MT('\0');
	return b - buf;
}

qse_size_t qse_wcsxsubst (
	qse_wchar_t* buf, qse_size_t bsz, const qse_wchar_t* fmt, 
	qse_wcsxsubst_subst_t subst, void* ctx)
{
	qse_wchar_t* b = buf;
	qse_wchar_t* end = buf + bsz - 1;
	const qse_wchar_t* f = fmt;

	if (bsz <= 0) return 0;

	while (*f != QSE_WT('\0'))
	{
		if (*f == QSE_WT('\\'))
		{
			// get the escaped character and treat it normally.
			// if the escaper is the last character, treat it 
			// normally also.
			if (f[1] != QSE_WT('\0')) f++;
		}
		else if (*f == QSE_WT('$'))
		{
			if (f[1] == QSE_WT('{'))
			{
				const qse_wchar_t* tmp;
				qse_wcstr_t ident;

				f += 2; /* skip ${ */ 
				tmp = f; /* mark the beginning */

				/* scan an enclosed segment */
				while (*f != QSE_WT('\0') && *f != QSE_WT('}')) f++;
	
				if (*f != QSE_WT('}'))
				{
					/* restore to the position of $ */
					f = tmp - 2;
					goto normal;
				}

				f++; /* skip } */
			
				ident.ptr = tmp;
				ident.len = f - tmp - 1;

				b = subst (b, end - b, &ident, ctx);
				if (b >= end) goto fini;

				continue;
			}
			else if (f[1] == QSE_WT('$')) f++;
		}

	normal:
		if (b >= end) break;
		*b++ = *f++;
	}

fini:
	*b = QSE_WT('\0');
	return b - buf;
}
