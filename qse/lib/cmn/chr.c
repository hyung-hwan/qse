/*
 * $Id: chr.c 554 2011-08-22 05:26:26Z hyunghwan.chung $
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

#include <qse/cmn/chr.h>
#include <qse/cmn/str.h>

#include <ctype.h>
#include <wctype.h>

static qse_bool_t is_malpha (qse_mcint_t c) { return isalpha(c); }
static qse_bool_t is_malnum (qse_mcint_t c) { return isalnum(c); }
static qse_bool_t is_mblank (qse_mcint_t c) 
{ 
#ifdef HAVE_ISBLANK
	return isblank(c); 
#else
	return c == QSE_MT(' ') || c == QSE_MT('\t');
#endif
}
static qse_bool_t is_mcntrl (qse_mcint_t c) { return iscntrl(c); }
static qse_bool_t is_mdigit (qse_mcint_t c) { return isdigit(c); }
static qse_bool_t is_mgraph (qse_mcint_t c) { return isgraph(c); }
static qse_bool_t is_mlower (qse_mcint_t c) { return islower(c); }
static qse_bool_t is_mprint (qse_mcint_t c) { return isprint(c); }
static qse_bool_t is_mpunct (qse_mcint_t c) { return ispunct(c); }
static qse_bool_t is_mspace (qse_mcint_t c) { return isspace(c); }
static qse_bool_t is_mupper (qse_mcint_t c) { return isupper(c); }
static qse_bool_t is_mxdigit (qse_mcint_t c) { return isxdigit(c); }


static qse_bool_t is_walpha (qse_wcint_t c) { return iswalpha(c); }
static qse_bool_t is_walnum (qse_wcint_t c) { return iswalnum(c); }
static qse_bool_t is_wblank (qse_wcint_t c) 
{ 
#ifdef HAVE_ISWBLANK
	return iswblank(c); 
#else
	return c == QSE_WT(' ') || c == QSE_WT('\t');
#endif
}
static qse_bool_t is_wcntrl (qse_wcint_t c) { return iswcntrl(c); }
static qse_bool_t is_wdigit (qse_wcint_t c) { return iswdigit(c); }
static qse_bool_t is_wgraph (qse_wcint_t c) { return iswgraph(c); }
static qse_bool_t is_wlower (qse_wcint_t c) { return iswlower(c); }
static qse_bool_t is_wprint (qse_wcint_t c) { return iswprint(c); }
static qse_bool_t is_wpunct (qse_wcint_t c) { return iswpunct(c); }
static qse_bool_t is_wspace (qse_wcint_t c) { return iswspace(c); }
static qse_bool_t is_wupper (qse_wcint_t c) { return iswupper(c); }
static qse_bool_t is_wxdigit (qse_wcint_t c) { return iswxdigit(c); }

qse_bool_t qse_ismccls (qse_mcint_t c, qse_mccls_id_t type)
{ 
	/* TODO: use GetStringTypeW/A for WIN32 to implement these */

	static qse_bool_t (*f[]) (qse_mcint_t) = 
	{
		is_malnum,
		is_malpha,
		is_mblank,
		is_mcntrl,
		is_mdigit,
		is_mgraph,
		is_mlower,
		is_mprint,
		is_mpunct,
		is_mspace,
		is_mupper,
		is_mxdigit
	};

	QSE_ASSERTX (type >= QSE_WCCLS_ALNUM && type <= QSE_WCCLS_XDIGIT,
		"The character type should be one of qse_mccls_id_t values");
	return f[type-1] (c);
}

qse_mcint_t qse_tomccls (qse_mcint_t c, qse_mccls_id_t type)  
{ 
	QSE_ASSERTX (type == QSE_MCCLS_UPPER || type == QSE_MCCLS_LOWER,
		"The character type should be one of QSE_MCCLS_UPPER and QSE_MCCLS_LOWER");

	if (type == QSE_MCCLS_UPPER) return toupper(c);
	if (type == QSE_MCCLS_LOWER) return tolower(c);
	return c;
}

qse_bool_t qse_iswccls (qse_wcint_t c, qse_wccls_id_t type)
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
		is_walnum,
		is_walpha,
		is_wblank,
		is_wcntrl,
		is_wdigit,
		is_wgraph,
		is_wlower,
		is_wprint,
		is_wpunct,
		is_wspace,
		is_wupper,
		is_wxdigit
	};

	QSE_ASSERTX (type >= QSE_WCCLS_ALNUM && type <= QSE_WCCLS_XDIGIT,
		"The character type should be one of qse_wccls_id_t values");
	return f[type-1] (c);
/*
#endif
*/
}

qse_wcint_t qse_towccls (qse_wcint_t c, qse_wccls_id_t type)  
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
	QSE_ASSERTX (type == QSE_WCCLS_UPPER || type == QSE_WCCLS_LOWER,
		"The type should be one of QSE_WCCLS_UPPER and QSE_WCCLS_LOWER");
	if (type == QSE_WCCLS_UPPER) return towupper(c);
	if (type == QSE_WCCLS_LOWER) return towlower(c);
	return c;
/*
#endif
*/
}

static struct wtab_t
{
	const qse_wchar_t* name;
	int class;
} wtab[] =
{
	{ QSE_WT("alnum"), QSE_WCCLS_ALNUM },
	{ QSE_WT("alpha"), QSE_WCCLS_ALPHA },
	{ QSE_WT("blank"), QSE_WCCLS_BLANK },
	{ QSE_WT("cntrl"), QSE_WCCLS_CNTRL },
	{ QSE_WT("digit"), QSE_WCCLS_DIGIT },
	{ QSE_WT("graph"), QSE_WCCLS_GRAPH },
	{ QSE_WT("lower"), QSE_WCCLS_LOWER },
	{ QSE_WT("print"), QSE_WCCLS_PRINT },
	{ QSE_WT("punct"), QSE_WCCLS_PUNCT },
	{ QSE_WT("space"), QSE_WCCLS_SPACE },
	{ QSE_WT("upper"), QSE_WCCLS_UPPER },
	{ QSE_WT("xdigit"), QSE_WCCLS_XDIGIT }
};

int qse_getwcclsidbyname (const qse_wchar_t* name, qse_wccls_id_t* id)
{
	int left = 0, right = QSE_COUNTOF(wtab) - 1, mid;
	while (left <= right)
	{
		int n;
		struct wtab_t* kwp;

		mid = (left + right) / 2;	
		kwp = &wtab[mid];

		n = qse_wcscmp (name, wtab->name);
		if (n > 0) 
		{
			/* if left, right, mid were of qse_size_t,
			 * you would need the following line. 
			if (mid == 0) break;
			 */
			right = mid - 1;
		}
		else if (n < 0) left = mid + 1;
		else
		{ 
			*id = kwp->class;
			return 0;
		}
	}

	return -1;
}

int qse_getwcclsidbyxname (const qse_wchar_t* name, qse_size_t len, qse_wccls_id_t* id)
{
	int left = 0, right = QSE_COUNTOF(wtab) - 1, mid;
	while (left <= right)
	{
		int n;
		struct wtab_t* kwp;

		mid = (left + right) / 2;	
		kwp = &wtab[mid];

		n = qse_wcsxcmp (name, len, wtab->name);
		if (n > 0) 
		{
			/* if left, right, mid were of qse_size_t,
			 * you would need the following line. 
			if (mid == 0) break;
			 */
			right = mid - 1;
		}
		else if (n < 0) left = mid + 1;
		else 
		{ 
			*id = kwp->class;
			return 0;
		}
	}

	return -1;
}

qse_wccls_id_t qse_getwcclsid (const qse_wchar_t* name)
{
	qse_wccls_id_t id;
	return (qse_getwcclsidbyname(name,&id) <= -1)? ((qse_wccls_id_t)0): id;
}

static struct mtab_t
{
	const qse_mchar_t* name;
	int class;
} mtab[] =
{
	{ QSE_MT("alnum"), QSE_MCCLS_ALNUM },
	{ QSE_MT("alpha"), QSE_MCCLS_ALPHA },
	{ QSE_MT("blank"), QSE_MCCLS_BLANK },
	{ QSE_MT("cntrl"), QSE_MCCLS_CNTRL },
	{ QSE_MT("digit"), QSE_MCCLS_DIGIT },
	{ QSE_MT("graph"), QSE_MCCLS_GRAPH },
	{ QSE_MT("lower"), QSE_MCCLS_LOWER },
	{ QSE_MT("print"), QSE_MCCLS_PRINT },
	{ QSE_MT("punct"), QSE_MCCLS_PUNCT },
	{ QSE_MT("space"), QSE_MCCLS_SPACE },
	{ QSE_MT("upper"), QSE_MCCLS_UPPER },
	{ QSE_MT("xdigit"), QSE_MCCLS_XDIGIT },
	{ QSE_NULL, 0 }
};

int qse_getmcclsidbyname (const qse_mchar_t* name, qse_mccls_id_t* id)
{
	int left = 0, right = QSE_COUNTOF(mtab) - 1, mid;
	while (left <= right)
	{
		int n;
		struct mtab_t* kwp;

		mid = (left + right) / 2;	
		kwp = &mtab[mid];

		n = qse_mbscmp (name, mtab->name);
		if (n > 0) 
		{
			/* if left, right, mid were of qse_size_t,
			 * you would need the following line. 
			if (mid == 0) break;
			 */
			right = mid - 1;
		}
		else if (n < 0) left = mid + 1;
		else
		{ 
			*id = kwp->class;
			return 0;
		}
	}

	return (qse_mccls_id_t)0;
}

int qse_getmcclsidbyxname (const qse_mchar_t* name, qse_size_t len, qse_mccls_id_t* id)
{
	int left = 0, right = QSE_COUNTOF(mtab) - 1, mid;
	while (left <= right)
	{
		int n;
		struct mtab_t* kwp;

		mid = (left + right) / 2;	
		kwp = &mtab[mid];

		n = qse_mbsxcmp (name, len, mtab->name);
		if (n > 0) 
		{
			/* if left, right, mid were of qse_size_t,
			 * you would need the following line. 
			if (mid == 0) break;
			 */
			right = mid - 1;
		}
		else if (n < 0) left = mid + 1;
		else
		{ 
			*id = kwp->class;
			return 0;
		}
	}

	return (qse_mccls_id_t)0;
}

qse_mccls_id_t qse_getmcclsid (const qse_mchar_t* name)
{
	qse_mccls_id_t id;
	return (qse_getmcclsidbyname(name,&id) <= -1)? ((qse_mccls_id_t)0): id;
}
