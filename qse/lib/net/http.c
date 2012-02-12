/*
 * $Id$
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

#include <qse/net/http.h>
#include <qse/cmn/str.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/htb.h>

int qse_comparehttpversions (
	const qse_http_version_t* v1,
	const qse_http_version_t* v2)
{
	if (v1->major == v2->major) return v1->minor - v2->minor;
	return v1->major - v2->major;
}

const qse_mchar_t* qse_httpmethodtombs (qse_http_method_t type)
{
	/* keep this table in the same order as qse_httpd_method_t enumerators */
	static qse_mchar_t* names[]  =
	{
		QSE_MT("OTHER"),

		QSE_MT("HEAD"),
		QSE_MT("GET"),
		QSE_MT("POST"),
		QSE_MT("PUT"),
		QSE_MT("DELETE"),
		QSE_MT("OPTIONS"),
		QSE_MT("TRACE"),
		QSE_MT("CONNECT")
	}; 

	return (type < 0 || type >= QSE_COUNTOF(names))? QSE_NULL: names[type];
}

struct mtab_t
{
	const qse_mchar_t* name;
	qse_http_method_t type;
};

static struct mtab_t mtab[] =
{
	/* keep this table sorted by name for binary search */
	{ QSE_MT("CONNECT"), QSE_HTTP_CONNECT },
	{ QSE_MT("DELETE"),  QSE_HTTP_DELETE },
	{ QSE_MT("GET"),     QSE_HTTP_GET },
	{ QSE_MT("HEAD"),    QSE_HTTP_HEAD },
	{ QSE_MT("OPTIONS"), QSE_HTTP_OPTIONS },
	{ QSE_MT("POST"),    QSE_HTTP_POST },
	{ QSE_MT("PUT"),     QSE_HTTP_PUT },
	{ QSE_MT("TRACE"),   QSE_HTTP_TRACE }
};

qse_http_method_t qse_mbstohttpmethod (const qse_mchar_t* name)
{
	/* perform binary search */

	/* declaring left, right, mid to be of int is ok
	 * because we know mtab is small enough. */
	int left = 0, right = QSE_COUNTOF(mtab) - 1, mid;

	while (left <= right)
	{
		int n;
		struct mtab_t* entry;

		mid = (left + right) / 2;	
		entry = &mtab[mid];

		n = qse_mbscmp (name, entry->name);
		if (n < 0) 
		{
			/* if left, right, mid were of qse_size_t,
			 * you would need the following line. 
			if (mid == 0) break;
			 */
			right = mid - 1;
		}
		else if (n > 0) left = mid + 1;
		else return entry->type;
	}

	return QSE_HTTP_OTHER;
}

qse_http_method_t qse_mcstrtohttpmethod (const qse_mcstr_t* name)
{
	/* perform binary search */

	/* declaring left, right, mid to be of int is ok
	 * because we know mtab is small enough. */
	int left = 0, right = QSE_COUNTOF(mtab) - 1, mid;

	while (left <= right)
	{
		int n;
		struct mtab_t* entry;

		mid = (left + right) / 2;	
		entry = &mtab[mid];

		n = qse_mbsxcmp (name->ptr, name->len, entry->name);
		if (n < 0) 
		{
			/* if left, right, mid were of qse_size_t,
			 * you would need the following line. 
			if (mid == 0) break;
			 */
			right = mid - 1;
		}
		else if (n > 0) left = mid + 1;
		else return entry->type;
	}

	return QSE_HTTP_OTHER;
}

int qse_parsehttprange (const qse_mchar_t* str, qse_http_range_t* range)
{
	/* NOTE: this function does not support a range set 
	 *       like bytes=1-20,30-50 */

	qse_http_range_int_t from, to;
	int type = QSE_HTTP_RANGE_PROPER;

	if (str[0] != QSE_MT('b') ||
	    str[1] != QSE_MT('y') ||
	    str[2] != QSE_MT('t') ||
	    str[3] != QSE_MT('e') ||
	    str[4] != QSE_MT('s') ||
	    str[5] != QSE_MT('=')) return -1;
	
	str += 6;

	from = 0;
	if (QSE_ISDIGIT(*str))
	{
		do
		{
			from = from * 10 + (*str - QSE_MT('0'));
			str++;
		}
		while (QSE_ISDIGIT(*str));
	}
	else type = QSE_HTTP_RANGE_SUFFIX;

	if (*str != QSE_MT('-')) return -1;
	str++;

	if (QSE_ISDIGIT(*str))
	{
		to = 0;
		do
		{
			to = to * 10 + (*str - QSE_MT('0'));
			str++;
		}
		while (QSE_ISDIGIT(*str));
	}
	else to = QSE_TYPE_MAX(qse_ulong_t); 

	if (from > to) return -1;

	range->type = type;
	range->from = from;
	range->to = to;
	return 0;
}

#if 0
int qse_parsehttpdatetime (const qse_mchar_t* str, qse_ntime_t* t)
{
/* TODO: */
	return -1;
}
#endif



