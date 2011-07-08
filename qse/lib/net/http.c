/*
 * $Id: http.c 341 2008-08-20 10:58:19Z baconevi $
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
#include <qse/cmn/htb.h>

const qse_htoc_t* qse_gethttpmethodname (qse_http_method_t type)
{
	static qse_htoc_t* names[]  =
	{
		"GET",
		"HEAD",
		"POST",
		"PUT",
		"DELETE",
		"TRACE",
		"OPTIONS",
		"CONNECT"
	}; 

	return (type < 0 || type >= QSE_COUNTOF(names))? QSE_NULL: names[type];
}

struct mtab_t
{
	const qse_htoc_t* name;
	qse_http_method_t type;
};

static struct mtab_t mtab[] =
{
	{ "CONNECT", QSE_HTTP_CONNECT },
	{ "DELETE",  QSE_HTTP_DELETE },
	{ "GET",     QSE_HTTP_GET },
	{ "HEAD",    QSE_HTTP_HEAD },
	{ "OPTIONS", QSE_HTTP_OPTIONS },
	{ "POST",    QSE_HTTP_POST },
	{ "PUT",     QSE_HTTP_PUT },
	{ "TRACE",   QSE_HTTP_TRACE }
};

int qse_gethttpmethodtype (
	const qse_htoc_t* name,
	qse_http_method_t* type)
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
		else 
		{
			*type = entry->type;
			return 0;
		}
	}

	return -1;
}

int qse_gethttpmethodtypefromstr (
	const qse_mcstr_t* name,
	qse_http_method_t* type)
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
		else 
		{
			*type = entry->type;
			return 0;
		}
	}

	return -1;
}

int qse_gethttpdatetime (const qse_htoc_t* str, qse_ntime_t* t)
{
/* TODO: */
	return -1;
}

int qse_gethttpdatetimefromstr (const qse_mcstr_t* str, qse_ntime_t* t)
{
/* TODO: */
	return -1;
}

