/*
 * $Id: chr.c 287 2009-09-15 10:01:02Z hyunghwan.chung $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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

#include <qse/cmn/chr.h>

#if defined(QSE_CHAR_IS_MCHAR)

#include <ctype.h>

static qse_bool_t is_upper (qse_cint_t c) { return isupper(c); }
static qse_bool_t is_lower (qse_cint_t c) { return islower(c); }
static qse_bool_t is_alpha (qse_cint_t c) { return isalpha(c); }
static qse_bool_t is_digit (qse_cint_t c) { return isdigit(c); }
static qse_bool_t is_xdigit (qse_cint_t c) { return isxdigit(c); }
static qse_bool_t is_alnum (qse_cint_t c) { return isalnum(c); }
static qse_bool_t is_space (qse_cint_t c) { return isspace(c); }
static qse_bool_t is_print (qse_cint_t c) { return isprint(c); }
static qse_bool_t is_graph (qse_cint_t c) { return isgraph(c); }
static qse_bool_t is_cntrl (qse_cint_t c) { return iscntrl(c); }
static qse_bool_t is_punct (qse_cint_t c) { return ispunct(c); }

qse_bool_t qse_ccls_is (qse_cint_t c, qse_ccls_id_t type)
{ 
	/* TODO: use GetStringTypeW/A for WIN32 to implement these */

	static qse_bool_t (*f[]) (qse_cint_t) = 
	{
		is_upper,
		is_lower,
		is_alpha,
		is_digit,
		is_xdigit,
		is_alnum,
		is_space,
		is_print,
		is_graph,
		is_cntrl,
		is_punct
	};

	QSE_ASSERTX (type >= QSE_CCLS_UPPER && type <= QSE_CCLS_PUNCT,
		"The character type should be one of qse_ccls_id_t values");
	return f[type] (c);
}

qse_cint_t qse_ccls_to (qse_cint_t c, qse_ccls_id_t type)  
{ 
	QSE_ASSERTX (type >= QSE_CCLS_UPPER && type <= QSE_CCLS_LOWER,
		"The character type should be one of QSE_CCLS_UPPER and QSE_CCLS_LOWER");

	if (type == QSE_CCLS_UPPER) return toupper(c);
	if (type == QSE_CCLS_LOWER) return tolower(c);
	return c;
}

#elif defined(QSE_CHAR_IS_WCHAR)

#include <wctype.h>

qse_bool_t qse_ccls_is (qse_cint_t c, qse_ccls_id_t type)
{ 
	static const char* name[] = 
	{
		"upper",
		"lower",
		"alpha",
		"digit",
		"xdigit",
		"alnum",
		"space",
		"print",
		"graph",
		"cntrl",
		"punct"
	};

	static wctype_t desc[] =
	{
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0,
		(wctype_t)0
	};

	QSE_ASSERTX (type >= QSE_CCLS_UPPER && type <= QSE_CCLS_PUNCT,
		"The character type should be one of qse_ccls_id_t values");
		
	if (desc[type] == (wctype_t)0) desc[type] = wctype(name[type]);
	return iswctype (c, desc[type]);
}

qse_cint_t qse_ccls_to (qse_cint_t c, qse_ccls_id_t type)  
{ 
#ifdef HAVE_WCTRANS
	static const char* name[] = 
	{
		"toupper",
		"tolower"
	};

	static wctrans_t desc[] =
	{
		(wctrans_t)0,
		(wctrans_t)0
	};

	QSE_ASSERTX (type >= QSE_CCLS_UPPER && type <= QSE_CCLS_LOWER,
		"The type should be one of QSE_CCLS_UPPER and QSE_CCLS_LOWER");

	if (desc[type] == (wctrans_t)0) desc[type] = wctrans(name[type]);
	return towctrans (c, desc[type]);

#else
	QSE_ASSERTX (type >= QSE_CCLS_UPPER && type <= QSE_CCLS_LOWER,
		"The type should be one of QSE_CCLS_UPPER and QSE_CCLS_LOWER");
	return (type == QSE_CCLS_UPPER)? towupper(c): towlower(c);
#endif
}

#else
	#error unsupported character type
#endif


