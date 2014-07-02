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

qse_size_t strfcpy (
	char_t* buf, const char_t* fmt, const char_t* str[])
{
	char_t* b = buf;
	const char_t* f = fmt;

	while (*f != T('\0'))
	{
		if (*f == T('$'))
		{
			if (f[1] == T('{') && 
			    (f[2] >= T('0') && f[2] <= T('9')))
			{
				const char_t* tmp;
				qse_size_t idx = 0;

				tmp = f;
				f += 2;

				do idx = idx * 10 + (*f++ - T('0'));
				while (*f >= T('0') && *f <= T('9'));
	
				if (*f != T('}'))
				{
					f = tmp;
					goto normal;
				}

				f++;

				tmp = str[idx];
				while (*tmp != T('\0')) *b++ = *tmp++;
				continue;
			}
			else if (f[1] == T('$')) f++;
		}

	normal:
		*b++ = *f++;
	}

	*b = T('\0');
	return b - buf;
}

qse_size_t strfncpy (
	char_t* buf, const char_t* fmt, const xstr_t str[])
{
	char_t* b = buf;
	const char_t* f = fmt;

	while (*f != T('\0'))
	{
		if (*f == T('\\'))
		{
			/* get the escaped character and treat it normally.
			 * if the escaper is the last character, treat it 
			 * normally also. */
			if (f[1] != T('\0')) f++;
		}
		else if (*f == T('$'))
		{
			if (f[1] == T('{') && 
			    (f[2] >= T('0') && f[2] <= T('9')))
			{
				const char_t* tmp, * tmpend;
				qse_size_t idx = 0;

				tmp = f;
				f += 2;

				do idx = idx * 10 + (*f++ - T('0'));
				while (*f >= T('0') && *f <= T('9'));
	
				if (*f != T('}'))
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
			else if (f[1] == T('$')) f++;
		}

	normal:
		*b++ = *f++;
	}

	*b = T('\0');
	return b - buf;
}

qse_size_t strxfcpy (
	char_t* buf, qse_size_t bsz, 
	const char_t* fmt, const char_t* str[])
{
	char_t* b = buf;
	char_t* end = buf + bsz - 1;
	const char_t* f = fmt;

	if (bsz <= 0) return 0;

	while (*f != T('\0'))
	{
		if (*f == T('\\'))
		{
			/* get the escaped character and treat it normally.
			 * if the escaper is the last character, treat it 
			 * normally also. */
			if (f[1] != T('\0')) f++;
		}
		else if (*f == T('$'))
		{
			if (f[1] == T('{') && 
			    (f[2] >= T('0') && f[2] <= T('9')))
			{
				const char_t* tmp;
				qse_size_t idx = 0;

				tmp = f;
				f += 2;

				do idx = idx * 10 + (*f++ - T('0'));
				while (*f >= T('0') && *f <= T('9'));
	
				if (*f != T('}'))
				{
					f = tmp;
					goto normal;
				}

				f++;
				
				tmp = str[idx];
				while (*tmp != T('\0')) 
				{
					if (b >= end) goto fini;
					*b++ = *tmp++;
				}
				continue;
			}
			else if (f[1] == T('$')) f++;
		}

	normal:
		if (b >= end) break;
		*b++ = *f++;
	}

fini:
	*b = T('\0');
	return b - buf;
}

qse_size_t strxfncpy (
	char_t* buf, qse_size_t bsz, 
	const char_t* fmt, const xstr_t str[])
{
	char_t* b = buf;
	char_t* end = buf + bsz - 1;
	const char_t* f = fmt;

	if (bsz <= 0) return 0;

	while (*f != T('\0'))
	{
		if (*f == T('\\'))
		{
			/* get the escaped character and treat it normally.
			 * if the escaper is the last character, treat it 
			 * normally also. */
			if (f[1] != T('\0')) f++;
		}
		else if (*f == T('$'))
		{
			if (f[1] == T('{') && 
			    (f[2] >= T('0') && f[2] <= T('9')))
			{
				const char_t* tmp, * tmpend;
				qse_size_t idx = 0;

				tmp = f;
				f += 2;

				do idx = idx * 10 + (*f++ - T('0'));
				while (*f >= T('0') && *f <= T('9'));
	
				if (*f != T('}'))
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
			else if (f[1] == T('$')) f++;
		}

	normal:
		if (b >= end) break;
		*b++ = *f++;
	}

fini:
	*b = T('\0');
	return b - buf;
}
