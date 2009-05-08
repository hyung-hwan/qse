/*
 * $Id: http.c 341 2008-08-20 10:58:19Z baconevi $
 * 
 * {License}
 */

#include <qse/utl/http.h>
#include <qse/cmn/chr.h>
#include "../cmn/mem.h"

static int is_http_space (qse_char_t c)
{
	return QSE_ISSPACE(c) && c != QSE_T('\r') && c != QSE_T('\n');
}

#define is_http_ctl(c) QSE_ISCNTRL(c)

static int is_http_separator (qse_char_t c)
{
	return c == QSE_T('(') ||
	       c == QSE_T(')') ||
	       c == QSE_T('<') ||
	       c == QSE_T('>') ||
	       c == QSE_T('@') ||
	       c == QSE_T(',') ||
	       c == QSE_T(';') ||
	       c == QSE_T(':') ||
	       c == QSE_T('\\') ||
	       c == QSE_T('\"') ||
	       c == QSE_T('/') ||
	       c == QSE_T('[') ||
	       c == QSE_T(']') ||
	       c == QSE_T('?') ||
	       c == QSE_T('=') ||
	       c == QSE_T('{') ||
	       c == QSE_T('}') ||
	       c == QSE_T('\t') ||
	       c == QSE_T(' ');
}

static int is_http_token (qse_char_t c)
{
	return QSE_ISPRINT(c) && !is_http_ctl(c) && !is_http_separator(c);
}

static int digit_to_num (qse_char_t c)
{
	if (c >= QSE_T('0') && c <= QSE_T('9')) return c - QSE_T('0');
	if (c >= QSE_T('A') && c <= QSE_T('Z')) return c - QSE_T('A') + 10;
	if (c >= QSE_T('a') && c <= QSE_T('z')) return c - QSE_T('a') + 10;
	return -1;
}

qse_char_t* qse_parsehttpreq (qse_char_t* buf, qse_http_req_t* req)
{
	qse_char_t* p = buf, * x;

	/* ignore leading spaces */
	while (is_http_space(*p)) p++;

	/* the method should start with an alphabet */
	if (!QSE_ISALPHA(*p)) return QSE_NULL;

	/* scan the method */
	req->method = p; while (QSE_ISALPHA(*p)) p++;

	/* the method should be followed by a space */
	if (!is_http_space(*p)) return QSE_NULL;

	/* null-terminate the method */
	*p++ = QSE_T('\0');

	/* skip spaces */
	while (is_http_space(*p)) p++;

	/* scan the url */
	req->path.ptr = p; 
	req->args.ptr = QSE_NULL;

	x = p;
	while (QSE_ISPRINT(*p) && !QSE_ISSPACE(*p)) 
	{
		if (*p == QSE_T('%') && QSE_ISXDIGIT(*(p+1)) && QSE_ISXDIGIT(*(p+2)))
		{
			*x++ = (digit_to_num(*(p+1)) << 4) + digit_to_num(*(p+2));
			p += 3;
		}
		else if (*p == QSE_T('?') && req->args.ptr == QSE_NULL)
		{
			/* ? must be explicit to be a argument instroducer. 
			 * %3f is just a literal. */
			req->path.len = x - req->path.ptr;
			*x++ = QSE_T('\0');
			req->args.ptr = x;
			p++;
		}
		else *x++ = *p++;
	}

	/* the url should be followed by a space */
	if (!is_http_space(*p)) return QSE_NULL;
	
	/* null-terminate the url and store the length */
	if (req->args.ptr != QSE_NULL)
		req->args.len = x - req->args.ptr;
	else
		req->path.len = x - req->path.ptr;
	*x++ = QSE_T('\0');

	/* path should start with a slash */
	if (req->path.len <= 0 || req->path.ptr[0] != QSE_T('/')) return QSE_NULL;

	/* skip spaces */
	do { p++; } while (is_http_space(*p));

	/* check http version */
	if ((p[0] == QSE_T('H') || p[0] == QSE_T('h')) &&
	    (p[1] == QSE_T('T') || p[1] == QSE_T('t')) &&
	    (p[2] == QSE_T('T') || p[2] == QSE_T('t')) &&
	    (p[3] == QSE_T('P') || p[3] == QSE_T('p')) &&
	    p[4] == QSE_T('/') && p[6] == QSE_T('.'))
	{
		if (!QSE_ISDIGIT(p[5])) return QSE_NULL;
		if (!QSE_ISDIGIT(p[7])) return QSE_NULL;
		req->vers.major = p[5] - QSE_T('0');
		req->vers.minor = p[7] - QSE_T('0');
		p += 8;
	}
	else return QSE_NULL;

	while (QSE_ISSPACE(*p)) 
	{
		if (*p++ == QSE_T('\n')) goto ok;
	}

	/* not terminating with a new line.
	 * maybe garbage after the request line */
	if (*p != QSE_T('\0')) return QSE_NULL;

ok:
	/* returns the next position */
	return p;
}

qse_char_t* qse_parsehttphdr (qse_char_t* buf, qse_http_hdr_t* hdr)
{
	qse_char_t* p = buf, * last;

	/* ignore leading spaces including CR and NL */
	while (QSE_ISSPACE(*p)) p++;

	if (*p == QSE_T('\0')) 
	{
		/* no more header line */
		QSE_MEMSET (hdr, 0, QSE_SIZEOF(*hdr));
		return p;
	}

	if (!is_http_token(*p)) return QSE_NULL;

	hdr->name.ptr = p;
	do { p++; } while (is_http_token(*p));

	last = p;
	hdr->name.len = last - hdr->name.ptr;

	while (is_http_space(*p)) p++;
	if (*p != QSE_T(':')) return QSE_NULL;

	*last = QSE_T('\0');

	do { p++; } while (is_http_space(*p));

	hdr->value.ptr = last = p;
	while (QSE_ISPRINT(*p))
	{
		if (!QSE_ISSPACE(*p++)) last = p;
	}
	hdr->value.len = last - hdr->value.ptr;

	while (QSE_ISSPACE(*p)) 
	{
		if (*p++ == QSE_T('\n')) goto ok;
	}

	/* not terminating with a new line.
	 * maybe garbage after the header line */
	if (*p != QSE_T('\0')) return QSE_NULL;

ok:
	*last = QSE_T('\0');
	return p;
}
