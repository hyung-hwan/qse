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

#include <qse/http/http.h>
#include <qse/cmn/str.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/htb.h>
#include "../cmn/mem.h"

int qse_comparehttpversions (
	const qse_http_version_t* v1,
	const qse_http_version_t* v2)
{
	if (v1->major == v2->major) return v1->minor - v2->minor;
	return v1->major - v2->major;
}

const qse_mchar_t* qse_httpstatustombs (int code)
{
	const qse_mchar_t* msg;

	switch (code)
	{
		case 100: msg = QSE_MT("Continue"); break;
		case 101: msg = QSE_MT("Switching Protocols"); break;

		case 200: msg = QSE_MT("OK"); break;
		case 201: msg = QSE_MT("Created"); break;
		case 202: msg = QSE_MT("Accepted"); break;
		case 203: msg = QSE_MT("Non-Authoritative Information"); break;
		case 204: msg = QSE_MT("No Content"); break;
		case 205: msg = QSE_MT("Reset Content"); break;
		case 206: msg = QSE_MT("Partial Content"); break;
		
		case 300: msg = QSE_MT("Multiple Choices"); break;
		case 301: msg = QSE_MT("Moved Permanently"); break;
		case 302: msg = QSE_MT("Found"); break;
		case 303: msg = QSE_MT("See Other"); break;
		case 304: msg = QSE_MT("Not Modified"); break;
		case 305: msg = QSE_MT("Use Proxy"); break;
		case 307: msg = QSE_MT("Temporary Redirect"); break;
		case 308: msg = QSE_MT("Permanent Redirect"); break;

		case 400: msg = QSE_MT("Bad Request"); break;
		case 401: msg = QSE_MT("Unauthorized"); break;
		case 402: msg = QSE_MT("Payment Required"); break;
		case 403: msg = QSE_MT("Forbidden"); break;
		case 404: msg = QSE_MT("Not Found"); break;
		case 405: msg = QSE_MT("Method Not Allowed"); break;
		case 406: msg = QSE_MT("Not Acceptable"); break;
		case 407: msg = QSE_MT("Proxy Authentication Required"); break;
		case 408: msg = QSE_MT("Request Timeout"); break;
		case 409: msg = QSE_MT("Conflict"); break;
		case 410: msg = QSE_MT("Gone"); break;
		case 411: msg = QSE_MT("Length Required"); break;
		case 412: msg = QSE_MT("Precondition Failed"); break;
		case 413: msg = QSE_MT("Request Entity Too Large"); break;
		case 414: msg = QSE_MT("Request-URI Too Long"); break;
		case 415: msg = QSE_MT("Unsupported Media Type"); break;
		case 416: msg = QSE_MT("Requested Range Not Satisfiable"); break;
		case 417: msg = QSE_MT("Expectation Failed"); break;
		case 426: msg = QSE_MT("Upgrade Required"); break;
		case 428: msg = QSE_MT("Precondition Required"); break;
		case 429: msg = QSE_MT("Too Many Requests"); break;
		case 431: msg = QSE_MT("Request Header Fields Too Large"); break;

		case 500: msg = QSE_MT("Internal Server Error"); break;
		case 501: msg = QSE_MT("Not Implemented"); break;
		case 502: msg = QSE_MT("Bad Gateway"); break;
		case 503: msg = QSE_MT("Service Unavailable"); break;
		case 504: msg = QSE_MT("Gateway Timeout"); break;
		case 505: msg = QSE_MT("HTTP Version Not Supported"); break;

		default: msg = QSE_MT("Unknown Error"); break;
	}

	return msg;
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

typedef struct mname_t mname_t;
struct mname_t
{
	const qse_mchar_t* s;
	const qse_mchar_t* l;
};
	
static mname_t wday_name[] =
{
	{ QSE_MT("Sun"), QSE_MT("Sunday") },
	{ QSE_MT("Mon"), QSE_MT("Monday") },
	{ QSE_MT("Tue"), QSE_MT("Tuesday") },
	{ QSE_MT("Wed"), QSE_MT("Wednesday") },
	{ QSE_MT("Thu"), QSE_MT("Thursday") },
	{ QSE_MT("Fri"), QSE_MT("Friday") },
	{ QSE_MT("Sat"), QSE_MT("Saturday") }
};

static mname_t mon_name[] =
{
	{ QSE_MT("Jan"), QSE_MT("January") },
	{ QSE_MT("Feb"), QSE_MT("February") },
	{ QSE_MT("Mar"), QSE_MT("March") },
	{ QSE_MT("Apr"), QSE_MT("April") },
	{ QSE_MT("May"), QSE_MT("May") },
	{ QSE_MT("Jun"), QSE_MT("June") },
	{ QSE_MT("Jul"), QSE_MT("July") },
	{ QSE_MT("Aug"), QSE_MT("August") },
	{ QSE_MT("Sep"), QSE_MT("September") },
	{ QSE_MT("Oct"), QSE_MT("October") },
	{ QSE_MT("Nov"), QSE_MT("November") },
	{ QSE_MT("Dec"), QSE_MT("December") }
};

int qse_parsehttptime (const qse_mchar_t* str, qse_ntime_t* nt)
{
	qse_btime_t bt;
	const qse_mchar_t* word;
	qse_size_t wlen, i;

	/* TODO: support more formats */

	QSE_MEMSET (&bt, 0, QSE_SIZEOF(bt));

	/* weekday */
	while (QSE_ISMSPACE(*str)) str++;
	for (word = str; QSE_ISMALPHA(*str); str++);
	wlen = str - word;
	for (i = 0; i < QSE_COUNTOF(wday_name); i++)
	{
		if (qse_mbsxcmp (word, wlen, wday_name[i].s) == 0)
		{
			bt.wday = i;
			break;
		}
	}
	if (i >= QSE_COUNTOF(wday_name)) return -1;

	/* comma - i'm just loose as i don't care if it doesn't exist */
	while (QSE_ISMSPACE(*str)) str++;
	if (*str == QSE_MT(',')) str++;

	/* day */
	while (QSE_ISMSPACE(*str)) str++;
	if (!QSE_ISMDIGIT(*str)) return -1;
	do bt.mday = bt.mday * 10 + *str++ - QSE_MT('0'); while (QSE_ISMDIGIT(*str));

	/* month */
	while (QSE_ISMSPACE(*str)) str++;
	for (word = str; QSE_ISMALPHA(*str); str++);
	wlen = str - word;
	for (i = 0; i < QSE_COUNTOF(mon_name); i++)
	{
		if (qse_mbsxcmp (word, wlen, mon_name[i].s) == 0)
		{
			bt.mon = i;
			break;
		}
	}
	if (i >= QSE_COUNTOF(mon_name)) return -1;

	/* year */
	while (QSE_ISMSPACE(*str)) str++;
	if (!QSE_ISMDIGIT(*str)) return -1;
	do bt.year = bt.year * 10 + *str++ - QSE_MT('0'); while (QSE_ISMDIGIT(*str));
	bt.year -= QSE_BTIME_YEAR_BASE;

	/* hour */
	while (QSE_ISMSPACE(*str)) str++;
	if (!QSE_ISMDIGIT(*str)) return -1;
	do bt.hour = bt.hour * 10 + *str++ - QSE_MT('0'); while (QSE_ISMDIGIT(*str));
	if (*str != QSE_MT(':'))  return -1;
	str++;

	/* min */
	while (QSE_ISMSPACE(*str)) str++;
	if (!QSE_ISMDIGIT(*str)) return -1;
	do bt.min = bt.min * 10 + *str++ - QSE_MT('0'); while (QSE_ISMDIGIT(*str));
	if (*str != QSE_MT(':'))  return -1;
	str++;

	/* sec */
	while (QSE_ISMSPACE(*str)) str++;
	if (!QSE_ISMDIGIT(*str)) return -1;
	do bt.sec = bt.sec * 10 + *str++ - QSE_MT('0'); while (QSE_ISMDIGIT(*str));

	/* GMT */
	while (QSE_ISMSPACE(*str)) str++;
	for (word = str; QSE_ISMALPHA(*str); str++);
	wlen = str - word;
	if (qse_mbsxcmp (word, wlen, QSE_MT("GMT")) != 0) return -1;

	while (QSE_ISMSPACE(*str)) str++;
	if (*str != QSE_MT('\0')) return -1;

	return qse_timegm (&bt, nt);
}

qse_mchar_t* qse_fmthttptime (
	const qse_ntime_t* nt, qse_mchar_t* buf, qse_size_t bufsz)
{
	qse_btime_t bt;

	qse_gmtime (nt, &bt);

	qse_mbsxfmt (
		buf, bufsz,
		QSE_MT("%s, %d %s %d %02d:%02d:%02d GMT"),
		wday_name[bt.wday].s,
		bt.mday,
		mon_name[bt.mon].s,
		bt.year + QSE_BTIME_YEAR_BASE,
		bt.hour, bt.min, bt.sec
	);

	return buf;
}

int qse_isperencedhttpstr (const qse_mchar_t* str)
{
	const qse_mchar_t* p = str;

	while (*p != QSE_T('\0'))
	{
		if (*p == QSE_MT('%') && *(p + 1) != QSE_MT('\0') && *(p + 2) != QSE_MT('\0'))
		{
			int q = QSE_MXDIGITTONUM (*(p + 1));
			if (q >= 0)
			{
				/* return true if the first valid percent-encoded sequence is found */
				int w = QSE_MXDIGITTONUM (*(p + 2));
				if (w >= 0) return 1; 
			}
		}

		p++;
	}

	return 1;
}

qse_size_t qse_perdechttpstr (const qse_mchar_t* str, qse_mchar_t* buf, qse_size_t* ndecs)
{
	const qse_mchar_t* p = str;
	qse_mchar_t* out = buf;
	qse_size_t dec_count = 0;

	while (*p != QSE_T('\0'))
	{
		if (*p == QSE_MT('%') && *(p + 1) != QSE_MT('\0') && *(p + 2) != QSE_MT('\0'))
		{
			int q = QSE_MXDIGITTONUM (*(p + 1));
			if (q >= 0)
			{
				int w = QSE_MXDIGITTONUM (*(p + 2));
				if (w >= 0)
				{
					/* we don't care if it contains a null character */
					*out++ = ((q << 4) + w);
					p += 3;
					dec_count++;
					continue;
				}
			}
		}

		*out++ = *p++;
	}

	*out = QSE_MT('\0');
	if (ndecs) *ndecs = dec_count;
	return out - buf;
}

#define IS_UNRESERVED(c) \
	(((c) >= QSE_MT('A') && (c) <= QSE_MT('Z')) || \
	 ((c) >= QSE_MT('a') && (c) <= QSE_MT('z')) || \
	 ((c) >= QSE_MT('0') && (c) <= QSE_MT('9')) || \
	 (c) == QSE_MT('-') || (c) == QSE_T('_') || \
	 (c) == QSE_MT('.') || (c) == QSE_T('~'))

#define TO_HEX(v) (QSE_MT("0123456789ABCDEF")[(v) & 15])

qse_size_t qse_perenchttpstr (int opt, const qse_mchar_t* str, qse_mchar_t* buf, qse_size_t* nencs)
{
	const qse_mchar_t* p = str;
	qse_mchar_t* out = buf;
	qse_size_t enc_count = 0;

	/* this function doesn't accept the size of the buffer. the caller must 
	 * ensure that the buffer is large enough */

	if (opt & QSE_PERENCHTTPSTR_KEEP_SLASH)
	{
		while (*p != QSE_T('\0'))
		{
			if (IS_UNRESERVED(*p) || *p == QSE_MT('/')) *out++ = *p;
			else
			{
				*out++ = QSE_MT('%');
				*out++ = TO_HEX (*p >> 4);
				*out++ = TO_HEX (*p & 15);
				enc_count++;
			}
			p++;
		}
	}
	else
	{
		while (*p != QSE_T('\0'))
		{
			if (IS_UNRESERVED(*p)) *out++ = *p;
			else
			{
				*out++ = QSE_MT('%');
				*out++ = TO_HEX (*p >> 4);
				*out++ = TO_HEX (*p & 15);
				enc_count++;
			}
			p++;
		}
	}
	*out = QSE_MT('\0');
	if (nencs) *nencs = enc_count;
	return out - buf;
}

qse_mchar_t* qse_perenchttpstrdup (int opt, const qse_mchar_t* str, qse_mmgr_t* mmgr)
{
	qse_mchar_t* buf;
	qse_size_t len = 0;
	qse_size_t count = 0;
	
	/* count the number of characters that should be encoded */
	if (opt & QSE_PERENCHTTPSTR_KEEP_SLASH)
	{
		for (len = 0; str[len] != QSE_T('\0'); len++)
		{
			if (!IS_UNRESERVED(str[len]) && str[len] != QSE_MT('/')) count++;
		}
	}
	else
	{
		for (len = 0; str[len] != QSE_T('\0'); len++)
		{
			if (!IS_UNRESERVED(str[len])) count++;
		}
	}

	/* if there are no characters to escape, just return the original string */
	if (count <= 0) return (qse_mchar_t*)str;

	/* allocate a buffer of an optimal size for escaping, otherwise */
	buf = QSE_MMGR_ALLOC (mmgr, (len  + (count * 2) + 1)  * QSE_SIZEOF(*buf));
	if (buf == QSE_NULL) return QSE_NULL;

	/* perform actual escaping */
	qse_perenchttpstr (opt, str, buf, QSE_NULL);

	return buf;
}
