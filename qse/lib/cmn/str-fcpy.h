/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
	char_t* buf, const char_t* fmt, const cstr_t str[])
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
	const char_t* fmt, const cstr_t str[])
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
