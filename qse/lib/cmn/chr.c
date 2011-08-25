/*
 * $Id: chr.c 555 2011-08-24 06:54:19Z hyunghwan.chung $
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

static QSE_INLINE int is_malpha (qse_mcint_t c) { return isalpha(c); }
static QSE_INLINE int is_malnum (qse_mcint_t c) { return isalnum(c); }
static QSE_INLINE int is_mblank (qse_mcint_t c) 
{ 
#ifdef HAVE_ISBLANK
	return isblank(c); 
#else
	return c == QSE_MT(' ') || c == QSE_MT('\t');
#endif
}
static QSE_INLINE int is_mcntrl (qse_mcint_t c) { return iscntrl(c); }
static QSE_INLINE int is_mdigit (qse_mcint_t c) { return isdigit(c); }
static QSE_INLINE int is_mgraph (qse_mcint_t c) { return isgraph(c); }
static QSE_INLINE int is_mlower (qse_mcint_t c) { return islower(c); }
static QSE_INLINE int is_mprint (qse_mcint_t c) { return isprint(c); }
static QSE_INLINE int is_mpunct (qse_mcint_t c) { return ispunct(c); }
static QSE_INLINE int is_mspace (qse_mcint_t c) { return isspace(c); }
static QSE_INLINE int is_mupper (qse_mcint_t c) { return isupper(c); }
static QSE_INLINE int is_mxdigit (qse_mcint_t c) { return isxdigit(c); }


static QSE_INLINE int is_walpha (qse_wcint_t c) { return iswalpha(c); }
static QSE_INLINE int is_walnum (qse_wcint_t c) { return iswalnum(c); }
static QSE_INLINE int is_wblank (qse_wcint_t c) 
{ 
#ifdef HAVE_ISWBLANK
	return iswblank(c); 
#else
	return c == QSE_WT(' ') || c == QSE_WT('\t');
#endif
}
static QSE_INLINE int is_wcntrl (qse_wcint_t c) { return iswcntrl(c); }
static QSE_INLINE int is_wdigit (qse_wcint_t c) { return iswdigit(c); }
static QSE_INLINE int is_wgraph (qse_wcint_t c) { return iswgraph(c); }
static QSE_INLINE int is_wlower (qse_wcint_t c) { return iswlower(c); }
static QSE_INLINE int is_wprint (qse_wcint_t c) { return iswprint(c); }
static QSE_INLINE int is_wpunct (qse_wcint_t c) { return iswpunct(c); }
static QSE_INLINE int is_wspace (qse_wcint_t c) { return iswspace(c); }
static QSE_INLINE int is_wupper (qse_wcint_t c) { return iswupper(c); }
static QSE_INLINE int is_wxdigit (qse_wcint_t c) { return iswxdigit(c); }

int qse_ismctype (qse_mcint_t c, qse_mctype_t type)
{ 
	/* TODO: use GetStringTypeW/A for WIN32 to implement these */

	static int (*f[]) (qse_mcint_t) = 
	{
#if 0
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
#endif

		isalnum,
		isalpha,
		is_mblank,
		iscntrl,
		isdigit,
		isgraph,
		islower,
		isprint,
		ispunct,
		isspace,
		isupper,
		isxdigit
	};

	QSE_ASSERTX (type >= QSE_WCTYPE_ALNUM && type <= QSE_WCTYPE_XDIGIT,
		"The character type should be one of qse_mctype_t values");
	return f[type-1] (c);
}

qse_mcint_t qse_tomctype (qse_mcint_t c, qse_mctype_t type)  
{ 
	QSE_ASSERTX (type == QSE_MCTYPE_UPPER || type == QSE_MCTYPE_LOWER,
		"The character type should be one of QSE_MCTYPE_UPPER and QSE_MCTYPE_LOWER");

	if (type == QSE_MCTYPE_UPPER) return toupper(c);
	if (type == QSE_MCTYPE_LOWER) return tolower(c);
	return c;
}

int qse_iswctype (qse_wcint_t c, qse_wctype_t type)
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

	QSE_ASSERTX (type >= QSE_CTYPE_UPPER && type <= QSE_CTYPE_PUNCT,
		"The character type should be one of qse_wctype_t values");
		
	if (desc[type] == (wctype_t)0) desc[type] = wctype(name[type]);
	return iswctype (c, desc[type]);
#else
*/
	static int (*f[]) (qse_wcint_t) = 
	{
#if 0
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
#endif
		iswalnum,
		iswalpha,
		is_wblank,
		iswcntrl,
		iswdigit,
		iswgraph,
		iswlower,
		iswprint,
		iswpunct,
		iswspace,
		iswupper,
		iswxdigit
	};

	QSE_ASSERTX (type >= QSE_WCTYPE_ALNUM && type <= QSE_WCTYPE_XDIGIT,
		"The character type should be one of qse_wctype_t values");
	return f[type-1] (c);
/*
#endif
*/
}

qse_wcint_t qse_towctype (qse_wcint_t c, qse_wctype_t type)  
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

	QSE_ASSERTX (type >= QSE_WCTYPE_UPPER && type <= QSE_WCTYPE_LOWER,
		"The type should be one of QSE_WCTYPE_UPPER and QSE_WCTYPE_LOWER");

	if (desc[type] == (wctrans_t)0) desc[type] = wctrans(name[type]);
	return towctrans (c, desc[type]);
#else
*/
	QSE_ASSERTX (type == QSE_WCTYPE_UPPER || type == QSE_WCTYPE_LOWER,
		"The type should be one of QSE_WCTYPE_UPPER and QSE_WCTYPE_LOWER");
	if (type == QSE_WCTYPE_UPPER) return towupper(c);
	if (type == QSE_WCTYPE_LOWER) return towlower(c);
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
	{ QSE_WT("alnum"), QSE_WCTYPE_ALNUM },
	{ QSE_WT("alpha"), QSE_WCTYPE_ALPHA },
	{ QSE_WT("blank"), QSE_WCTYPE_BLANK },
	{ QSE_WT("cntrl"), QSE_WCTYPE_CNTRL },
	{ QSE_WT("digit"), QSE_WCTYPE_DIGIT },
	{ QSE_WT("graph"), QSE_WCTYPE_GRAPH },
	{ QSE_WT("lower"), QSE_WCTYPE_LOWER },
	{ QSE_WT("print"), QSE_WCTYPE_PRINT },
	{ QSE_WT("punct"), QSE_WCTYPE_PUNCT },
	{ QSE_WT("space"), QSE_WCTYPE_SPACE },
	{ QSE_WT("upper"), QSE_WCTYPE_UPPER },
	{ QSE_WT("xdigit"), QSE_WCTYPE_XDIGIT }
};

int qse_getwctypebyname (const qse_wchar_t* name, qse_wctype_t* id)
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

int qse_getwctypebyxname (const qse_wchar_t* name, qse_size_t len, qse_wctype_t* id)
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

qse_wctype_t qse_getwctype (const qse_wchar_t* name)
{
	qse_wctype_t id;
	return (qse_getwctypebyname(name,&id) <= -1)? ((qse_wctype_t)0): id;
}

static struct mtab_t
{
	const qse_mchar_t* name;
	int class;
} mtab[] =
{
	{ QSE_MT("alnum"), QSE_MCTYPE_ALNUM },
	{ QSE_MT("alpha"), QSE_MCTYPE_ALPHA },
	{ QSE_MT("blank"), QSE_MCTYPE_BLANK },
	{ QSE_MT("cntrl"), QSE_MCTYPE_CNTRL },
	{ QSE_MT("digit"), QSE_MCTYPE_DIGIT },
	{ QSE_MT("graph"), QSE_MCTYPE_GRAPH },
	{ QSE_MT("lower"), QSE_MCTYPE_LOWER },
	{ QSE_MT("print"), QSE_MCTYPE_PRINT },
	{ QSE_MT("punct"), QSE_MCTYPE_PUNCT },
	{ QSE_MT("space"), QSE_MCTYPE_SPACE },
	{ QSE_MT("upper"), QSE_MCTYPE_UPPER },
	{ QSE_MT("xdigit"), QSE_MCTYPE_XDIGIT },
	{ QSE_NULL, 0 }
};

int qse_getmctypebyname (const qse_mchar_t* name, qse_mctype_t* id)
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

	return (qse_mctype_t)0;
}

int qse_getmctypebyxname (const qse_mchar_t* name, qse_size_t len, qse_mctype_t* id)
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

	return (qse_mctype_t)0;
}

qse_mctype_t qse_getmctype (const qse_mchar_t* name)
{
	qse_mctype_t id;
	return (qse_getmctypebyname(name,&id) <= -1)? ((qse_mctype_t)0): id;
}
