/*
 * $Id: chr.c 414 2011-03-25 04:52:47Z hyunghwan.chung $
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

#include <ctype.h>
#include <wctype.h>

static qse_bool_t is_mupper (qse_mcint_t c) { return isupper(c); }
static qse_bool_t is_mlower (qse_mcint_t c) { return islower(c); }
static qse_bool_t is_malpha (qse_mcint_t c) { return isalpha(c); }
static qse_bool_t is_mdigit (qse_mcint_t c) { return isdigit(c); }
static qse_bool_t is_mxdigit (qse_mcint_t c) { return isxdigit(c); }
static qse_bool_t is_malnum (qse_mcint_t c) { return isalnum(c); }
static qse_bool_t is_mspace (qse_mcint_t c) { return isspace(c); }
static qse_bool_t is_mprint (qse_mcint_t c) { return isprint(c); }
static qse_bool_t is_mgraph (qse_mcint_t c) { return isgraph(c); }
static qse_bool_t is_mcntrl (qse_mcint_t c) { return iscntrl(c); }
static qse_bool_t is_mpunct (qse_mcint_t c) { return ispunct(c); }

static qse_bool_t is_wupper (qse_wcint_t c) { return iswupper(c); }
static qse_bool_t is_wlower (qse_wcint_t c) { return iswlower(c); }
static qse_bool_t is_walpha (qse_wcint_t c) { return iswalpha(c); }
static qse_bool_t is_wdigit (qse_wcint_t c) { return iswdigit(c); }
static qse_bool_t is_wxdigit (qse_wcint_t c) { return iswxdigit(c); }
static qse_bool_t is_walnum (qse_wcint_t c) { return iswalnum(c); }
static qse_bool_t is_wspace (qse_wcint_t c) { return iswspace(c); }
static qse_bool_t is_wprint (qse_wcint_t c) { return iswprint(c); }
static qse_bool_t is_wgraph (qse_wcint_t c) { return iswgraph(c); }
static qse_bool_t is_wcntrl (qse_wcint_t c) { return iswcntrl(c); }
static qse_bool_t is_wpunct (qse_wcint_t c) { return iswpunct(c); }

qse_bool_t qse_mccls_is (qse_mcint_t c, qse_mccls_id_t type)
{ 
	/* TODO: use GetStringTypeW/A for WIN32 to implement these */

	static qse_bool_t (*f[]) (qse_mcint_t) = 
	{
		is_mupper,
		is_mlower,
		is_malpha,
		is_mdigit,
		is_mxdigit,
		is_malnum,
		is_mspace,
		is_mprint,
		is_mgraph,
		is_mcntrl,
		is_mpunct
	};

	QSE_ASSERTX (type >= QSE_MCCLS_UPPER && type <= QSE_MCCLS_PUNCT,
		"The character type should be one of qse_mccls_id_t values");
	return f[type] (c);
}

qse_mcint_t qse_mccls_to (qse_mcint_t c, qse_mccls_id_t type)  
{ 
	QSE_ASSERTX (type >= QSE_MCCLS_UPPER && type <= QSE_MCCLS_LOWER,
		"The character type should be one of QSE_MCCLS_UPPER and QSE_MCCLS_LOWER");

	if (type == QSE_MCCLS_UPPER) return toupper(c);
	if (type == QSE_MCCLS_LOWER) return tolower(c);
	return c;
}

qse_bool_t qse_wccls_is (qse_wcint_t c, qse_wccls_id_t type)
{ 
/*
#ifdef HAVE_WCTYPE
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
		"The character type should be one of qse_wccls_id_t values");
		
	if (desc[type] == (wctype_t)0) desc[type] = wctype(name[type]);
	return iswctype (c, desc[type]);
#else
*/
	static qse_bool_t (*f[]) (qse_wcint_t) = 
	{
		is_wupper,
		is_wlower,
		is_walpha,
		is_wdigit,
		is_wxdigit,
		is_walnum,
		is_wspace,
		is_wprint,
		is_wgraph,
		is_wcntrl,
		is_wpunct
	};

	QSE_ASSERTX (type >= QSE_WCCLS_UPPER && type <= QSE_WCCLS_PUNCT,
		"The character type should be one of qse_wccls_id_t values");
	return f[type] (c);
/*
#endif
*/
}

qse_wcint_t qse_wccls_to (qse_wcint_t c, qse_wccls_id_t type)  
{ 
/*
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

	QSE_ASSERTX (type >= QSE_WCCLS_UPPER && type <= QSE_WCCLS_LOWER,
		"The type should be one of QSE_WCCLS_UPPER and QSE_WCCLS_LOWER");

	if (desc[type] == (wctrans_t)0) desc[type] = wctrans(name[type]);
	return towctrans (c, desc[type]);
#else
*/
	QSE_ASSERTX (type >= QSE_WCCLS_UPPER && type <= QSE_WCCLS_LOWER,
		"The type should be one of QSE_WCCLS_UPPER and QSE_WCCLS_LOWER");
	return (type == QSE_CCLS_UPPER)? towupper(c): towlower(c);
/*
#endif
*/
}

