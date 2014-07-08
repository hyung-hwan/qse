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

#if !defined(char_t) && !defined(cstr_t) && !defined(strxsubst)
#	error Never include this file
#endif

static const char_t* scan_dollar (
	const char_t* f, qse_size_t l, cstr_t* ident, cstr_t* dfl, int depth)
{
	const char_t* end = f + l;

	QSE_ASSERT (l >= 2);
	
	f += 2; /* skip ${ */ 
	if (ident) ident->ptr = f;

	while (1)
	{
		if (f >= end) return QSE_NULL;
		if (*f == T('}') || *f == T(':')) break;
		f++;
	}

	if (*f == T(':'))
	{
		if (f >= end || *(f + 1) != T('=')) 
		{
			/* not := */
			return QSE_NULL; 
		}

		if (ident) ident->len = f - ident->ptr;

		f += 2; /* skip := */

		if (dfl) dfl->ptr = f;
		while (1)
		{
			if (f >= end) return QSE_NULL;

			else if (*f == T('$') && *(f + 1) == T('{'))
			{
				if (depth >= 64) return QSE_NULL; /* depth too deep */

				/* TODO: remove recursion */
				f = scan_dollar (f, end - f, QSE_NULL, QSE_NULL, depth + 1);
				if (f == QSE_NULL) return QSE_NULL;
			}
			else if (*f == T('}')) 
			{
				/* ending bracket  */
				if (dfl) dfl->len = f - dfl->ptr;
				return f + 1;
			}
			else	f++;
		}
	}
	else if (*f == T('}')) 
	{
		if (ident) ident->len = f - ident->ptr;
		if (dfl) 
		{
			dfl->ptr = QSE_NULL;
			dfl->len = 0;
		}
		return f + 1;
	}

	/* this part must not be reached */
	return QSE_NULL;
}

static char_t* expand_dollar (
	char_t* buf, qse_size_t bsz, const cstr_t* ident, const cstr_t* dfl,
	subst_t subst, void* ctx)
{
	char_t* tmp;

	tmp = subst (buf, bsz, ident, ctx);
	if (tmp == QSE_NULL)
	{
		/* substitution failed */
		if (dfl->len > 0)
		{
			/* take the default value */
			qse_size_t len;

			/* TODO: remove recursion */
			len = strxnsubst (buf, bsz, dfl->ptr, dfl->len, subst, ctx);
			tmp = buf + len;
		}
		else tmp = buf;
	}

	return tmp;
}

qse_size_t strxnsubst (
	char_t* buf, qse_size_t bsz, const char_t* fmt, qse_size_t fsz,
	subst_t subst, void* ctx)
{
	char_t* b = buf;
	char_t* end = buf + bsz - 1;
	const char_t* f = fmt;
	const char_t* fend = fmt + fsz;

	if (buf != NOBUF && bsz <= 0) return 0;

	while (f < fend)
	{
		if (*f == T('\\'))
		{
			/* get the escaped character and treat it normally.
			 * if the escaper is the last character, treat it 
			 * normally also. */
			if (f < fend - 1) f++;
		}
		else if (*f == T('$') && f < fend - 1)
		{
			if (*(f + 1) == T('{'))
			{
				const char_t* tmp;
				cstr_t ident, dfl;

				tmp = scan_dollar (f, fend - f, &ident, &dfl, 0);
				if (tmp == QSE_NULL || ident.len <= 0) goto normal;
				f = tmp;

				if (buf != NOBUF)
				{
					b = expand_dollar (b, end - b + 1, &ident, &dfl, subst, ctx);
					if (b >= end) goto fini;
				}
				else
				{
					/* the buffer points to NOBUF. */
					tmp = expand_dollar (buf, bsz, &ident, &dfl, subst, ctx);
					/* increment b by the length of the expanded string */
					b += (tmp - buf);
				}

				continue;
			}
			else if (*(f + 1) == T('$')) 
			{
				/* $$ -> $. \$ is also $. */
				f++;
			}
		}

	normal:
		if (buf != NOBUF)
		{
			if (b >= end) break;
			*b = *f;
		}
		b++; f++;
	}

fini:
	if (buf != NOBUF) *b = T('\0');
	return b - buf;
}

qse_size_t strxsubst (
	char_t* buf, qse_size_t bsz, const char_t* fmt, 
	subst_t subst, void* ctx)
{
	return strxnsubst (buf, bsz, fmt, strlen(fmt), subst, ctx);
}

