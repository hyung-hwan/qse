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

qse_size_t qse_mbsfcpy (
	qse_mchar_t* buf, const qse_mchar_t* fmt, const qse_mchar_t* str[])
{
	qse_mchar_t* b = buf;
	const qse_mchar_t* f = fmt;

	while (*f != QSE_MT('\0'))
	{
		if (*f == QSE_MT('$'))
		{
			if (f[1] == QSE_MT('{') && 
			    (f[2] >= QSE_MT('0') && f[2] <= QSE_MT('9')))
			{
				const qse_mchar_t* tmp;
				qse_size_t idx = 0;

				tmp = f;
				f += 2;

				do idx = idx * 10 + (*f++ - QSE_MT('0'));
				while (*f >= QSE_MT('0') && *f <= QSE_MT('9'));
	
				if (*f != QSE_MT('}'))
				{
					f = tmp;
					goto normal;
				}

				f++;

				tmp = str[idx];
				while (*tmp != QSE_MT('\0')) *b++ = *tmp++;
				continue;
			}
			else if (f[1] == QSE_MT('$')) f++;
		}

	normal:
		*b++ = *f++;
	}

	*b = QSE_MT('\0');
	return b - buf;
}

qse_size_t qse_mbsfncpy (
	qse_mchar_t* buf, const qse_mchar_t* fmt, const qse_mcstr_t str[])
{
	qse_mchar_t* b = buf;
	const qse_mchar_t* f = fmt;

	while (*f != QSE_MT('\0'))
	{
		if (*f == QSE_MT('\\'))
		{
			/* get the escaped character and treat it normally.
			 * if the escaper is the last character, treat it 
			 * normally also. */
			if (f[1] != QSE_MT('\0')) f++;
		}
		else if (*f == QSE_MT('$'))
		{
			if (f[1] == QSE_MT('{') && 
			    (f[2] >= QSE_MT('0') && f[2] <= QSE_MT('9')))
			{
				const qse_mchar_t* tmp, * tmpend;
				qse_size_t idx = 0;

				tmp = f;
				f += 2;

				do idx = idx * 10 + (*f++ - QSE_MT('0'));
				while (*f >= QSE_MT('0') && *f <= QSE_MT('9'));
	
				if (*f != QSE_MT('}'))
				{
					f = tmp;
					goto normal;
				}

				f++;
				
				tmp = str[idx].ptr;
				tmpend = tmp + str[idx].len;

				while (tmp < tmpend) *b++ = *tmp++;
				continue;
			}
			else if (f[1] == QSE_MT('$')) f++;
		}

	normal:
		*b++ = *f++;
	}

	*b = QSE_MT('\0');
	return b - buf;
}

qse_size_t qse_mbsxfcpy (
	qse_mchar_t* buf, qse_size_t bsz, 
	const qse_mchar_t* fmt, const qse_mchar_t* str[])
{
	qse_mchar_t* b = buf;
	qse_mchar_t* end = buf + bsz - 1;
	const qse_mchar_t* f = fmt;

	if (bsz <= 0) return 0;

	while (*f != QSE_MT('\0'))
	{
		if (*f == QSE_MT('\\'))
		{
			/* get the escaped character and treat it normally.
			 * if the escaper is the last character, treat it 
			 * normally also. */
			if (f[1] != QSE_MT('\0')) f++;
		}
		else if (*f == QSE_MT('$'))
		{
			if (f[1] == QSE_MT('{') && 
			    (f[2] >= QSE_MT('0') && f[2] <= QSE_MT('9')))
			{
				const qse_mchar_t* tmp;
				qse_size_t idx = 0;

				tmp = f;
				f += 2;

				do idx = idx * 10 + (*f++ - QSE_MT('0'));
				while (*f >= QSE_MT('0') && *f <= QSE_MT('9'));
	
				if (*f != QSE_MT('}'))
				{
					f = tmp;
					goto normal;
				}

				f++;
				
				tmp = str[idx];
				while (*tmp != QSE_MT('\0')) 
				{
					if (b >= end) goto fini;
					*b++ = *tmp++;
				}
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

qse_size_t qse_mbsxfncpy (
	qse_mchar_t* buf, qse_size_t bsz, 
	const qse_mchar_t* fmt, const qse_mcstr_t str[])
{
	qse_mchar_t* b = buf;
	qse_mchar_t* end = buf + bsz - 1;
	const qse_mchar_t* f = fmt;

	if (bsz <= 0) return 0;

	while (*f != QSE_MT('\0'))
	{
		if (*f == QSE_MT('\\'))
		{
			/* get the escaped character and treat it normally.
			 * if the escaper is the last character, treat it 
			 * normally also. */
			if (f[1] != QSE_MT('\0')) f++;
		}
		else if (*f == QSE_MT('$'))
		{
			if (f[1] == QSE_MT('{') && 
			    (f[2] >= QSE_MT('0') && f[2] <= QSE_MT('9')))
			{
				const qse_mchar_t* tmp, * tmpend;
				qse_size_t idx = 0;

				tmp = f;
				f += 2;

				do idx = idx * 10 + (*f++ - QSE_MT('0'));
				while (*f >= QSE_MT('0') && *f <= QSE_MT('9'));
	
				if (*f != QSE_MT('}'))
				{
					f = tmp;
					goto normal;
				}

				f++;
				
				tmp = str[idx].ptr;
				tmpend = tmp + str[idx].len;

				while (tmp < tmpend)
				{
					if (b >= end) goto fini;
					*b++ = *tmp++;
				}
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

qse_size_t qse_wcsfcpy (
	qse_wchar_t* buf, const qse_wchar_t* fmt, const qse_wchar_t* str[])
{
	qse_wchar_t* b = buf;
	const qse_wchar_t* f = fmt;

	while (*f != QSE_WT('\0'))
	{
		if (*f == QSE_WT('$'))
		{
			if (f[1] == QSE_WT('{') && 
			    (f[2] >= QSE_WT('0') && f[2] <= QSE_WT('9')))
			{
				const qse_wchar_t* tmp;
				qse_size_t idx = 0;

				tmp = f;
				f += 2;

				do idx = idx * 10 + (*f++ - QSE_WT('0'));
				while (*f >= QSE_WT('0') && *f <= QSE_WT('9'));
	
				if (*f != QSE_WT('}'))
				{
					f = tmp;
					goto normal;
				}

				f++;

				tmp = str[idx];
				while (*tmp != QSE_WT('\0')) *b++ = *tmp++;
				continue;
			}
			else if (f[1] == QSE_WT('$')) f++;
		}

	normal:
		*b++ = *f++;
	}

	*b = QSE_WT('\0');
	return b - buf;
}

qse_size_t qse_wcsfncpy (
	qse_wchar_t* buf, const qse_wchar_t* fmt, const qse_wcstr_t str[])
{
	qse_wchar_t* b = buf;
	const qse_wchar_t* f = fmt;

	while (*f != QSE_WT('\0'))
	{
		if (*f == QSE_WT('\\'))
		{
			/* get the escaped character and treat it normally.
			 * if the escaper is the last character, treat it 
			 * normally also. */
			if (f[1] != QSE_WT('\0')) f++;
		}
		else if (*f == QSE_WT('$'))
		{
			if (f[1] == QSE_WT('{') && 
			    (f[2] >= QSE_WT('0') && f[2] <= QSE_WT('9')))
			{
				const qse_wchar_t* tmp, * tmpend;
				qse_size_t idx = 0;

				tmp = f;
				f += 2;

				do idx = idx * 10 + (*f++ - QSE_WT('0'));
				while (*f >= QSE_WT('0') && *f <= QSE_WT('9'));
	
				if (*f != QSE_WT('}'))
				{
					f = tmp;
					goto normal;
				}

				f++;
				
				tmp = str[idx].ptr;
				tmpend = tmp + str[idx].len;

				while (tmp < tmpend) *b++ = *tmp++;
				continue;
			}
			else if (f[1] == QSE_WT('$')) f++;
		}

	normal:
		*b++ = *f++;
	}

	*b = QSE_WT('\0');
	return b - buf;
}

qse_size_t qse_wcsxfcpy (
	qse_wchar_t* buf, qse_size_t bsz, 
	const qse_wchar_t* fmt, const qse_wchar_t* str[])
{
	qse_wchar_t* b = buf;
	qse_wchar_t* end = buf + bsz - 1;
	const qse_wchar_t* f = fmt;

	if (bsz <= 0) return 0;

	while (*f != QSE_WT('\0'))
	{
		if (*f == QSE_WT('\\'))
		{
			/* get the escaped character and treat it normally.
			 * if the escaper is the last character, treat it 
			 * normally also. */
			if (f[1] != QSE_WT('\0')) f++;
		}
		else if (*f == QSE_WT('$'))
		{
			if (f[1] == QSE_WT('{') && 
			    (f[2] >= QSE_WT('0') && f[2] <= QSE_WT('9')))
			{
				const qse_wchar_t* tmp;
				qse_size_t idx = 0;

				tmp = f;
				f += 2;

				do idx = idx * 10 + (*f++ - QSE_WT('0'));
				while (*f >= QSE_WT('0') && *f <= QSE_WT('9'));
	
				if (*f != QSE_WT('}'))
				{
					f = tmp;
					goto normal;
				}

				f++;
				
				tmp = str[idx];
				while (*tmp != QSE_WT('\0')) 
				{
					if (b >= end) goto fini;
					*b++ = *tmp++;
				}
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

qse_size_t qse_wcsxfncpy (
	qse_wchar_t* buf, qse_size_t bsz, 
	const qse_wchar_t* fmt, const qse_wcstr_t str[])
{
	qse_wchar_t* b = buf;
	qse_wchar_t* end = buf + bsz - 1;
	const qse_wchar_t* f = fmt;

	if (bsz <= 0) return 0;

	while (*f != QSE_WT('\0'))
	{
		if (*f == QSE_WT('\\'))
		{
			/* get the escaped character and treat it normally.
			 * if the escaper is the last character, treat it 
			 * normally also. */
			if (f[1] != QSE_WT('\0')) f++;
		}
		else if (*f == QSE_WT('$'))
		{
			if (f[1] == QSE_WT('{') && 
			    (f[2] >= QSE_WT('0') && f[2] <= QSE_WT('9')))
			{
				const qse_wchar_t* tmp, * tmpend;
				qse_size_t idx = 0;

				tmp = f;
				f += 2;

				do idx = idx * 10 + (*f++ - QSE_WT('0'));
				while (*f >= QSE_WT('0') && *f <= QSE_WT('9'));
	
				if (*f != QSE_WT('}'))
				{
					f = tmp;
					goto normal;
				}

				f++;
				
				tmp = str[idx].ptr;
				tmpend = tmp + str[idx].len;

				while (tmp < tmpend)
				{
					if (b >= end) goto fini;
					*b++ = *tmp++;
				}
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
