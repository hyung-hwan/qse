/*
 * $Id: http.c 341 2008-08-20 10:58:19Z baconevi $
 * 
 * {License}
 */

#include <ase/utl/http.h>
#include "../cmn/mem.h"
#include "../cmn/chr.h"

static int is_http_space (ase_char_t c)
{
	return ASE_ISSPACE(c) && c != ASE_T('\r') && c != ASE_T('\n');
}

#define is_http_ctl(c) ASE_ISCNTRL(c)

static int is_http_separator (ase_char_t c)
{
	return c == ASE_T('(') ||
	       c == ASE_T(')') ||
	       c == ASE_T('<') ||
	       c == ASE_T('>') ||
	       c == ASE_T('@') ||
	       c == ASE_T(',') ||
	       c == ASE_T(';') ||
	       c == ASE_T(':') ||
	       c == ASE_T('\\') ||
	       c == ASE_T('\"') ||
	       c == ASE_T('/') ||
	       c == ASE_T('[') ||
	       c == ASE_T(']') ||
	       c == ASE_T('?') ||
	       c == ASE_T('=') ||
	       c == ASE_T('{') ||
	       c == ASE_T('}') ||
	       c == ASE_T('\t') ||
	       c == ASE_T(' ');
}

static int is_http_token (ase_char_t c)
{
	return ASE_ISPRINT(c) && !is_http_ctl(c) && !is_http_separator(c);
}

static int digit_to_num (ase_char_t c)
{
	if (c >= ASE_T('0') && c <= ASE_T('9')) return c - ASE_T('0');
	if (c >= ASE_T('A') && c <= ASE_T('Z')) return c - ASE_T('A') + 10;
	if (c >= ASE_T('a') && c <= ASE_T('z')) return c - ASE_T('a') + 10;
	return -1;
}

ase_char_t* ase_parsehttpreq (ase_char_t* buf, ase_http_req_t* req)
{
	ase_char_t* p = buf, * x;

	/* ignore leading spaces */
	while (is_http_space(*p)) p++;

	/* the method should start with an alphabet */
	if (!ASE_ISALPHA(*p)) return ASE_NULL;

	/* scan the method */
	req->method = p; while (ASE_ISALPHA(*p)) p++;

	/* the method should be followed by a space */
	if (!is_http_space(*p)) return ASE_NULL;

	/* null-terminate the method */
	*p++ = ASE_T('\0');

	/* skip spaces */
	while (is_http_space(*p)) p++;

	/* scan the url */
	req->path.ptr = p; 
	req->args.ptr = ASE_NULL;

	x = p;
	while (ASE_ISPRINT(*p) && !ASE_ISSPACE(*p)) 
	{
		if (*p == ASE_T('%') && ASE_ISXDIGIT(*(p+1)) && ASE_ISXDIGIT(*(p+2)))
		{
			*x++ = (digit_to_num(*(p+1)) << 4) + digit_to_num(*(p+2));
			p += 3;
		}
		else if (*p == ASE_T('?') && req->args.ptr == ASE_NULL)
		{
			/* ? must be explicit to be a argument instroducer. 
			 * %3f is just a literal. */
			req->path.len = x - req->path.ptr;
			*x++ = ASE_T('\0');
			req->args.ptr = x;
			p++;
		}
		else *x++ = *p++;
	}

	/* the url should be followed by a space */
	if (!is_http_space(*p)) return ASE_NULL;
	
	/* null-terminate the url and store the length */
	if (req->args.ptr != ASE_NULL)
		req->args.len = x - req->args.ptr;
	else
		req->path.len = x - req->path.ptr;
	*x++ = ASE_T('\0');

	/* path should start with a slash */
	if (req->path.len <= 0 || req->path.ptr[0] != ASE_T('/')) return ASE_NULL;

	/* skip spaces */
	do { p++; } while (is_http_space(*p));

	/* check http version */
	if ((p[0] == ASE_T('H') || p[0] == ASE_T('h')) &&
	    (p[1] == ASE_T('T') || p[1] == ASE_T('t')) &&
	    (p[2] == ASE_T('T') || p[2] == ASE_T('t')) &&
	    (p[3] == ASE_T('P') || p[3] == ASE_T('p')) &&
	    p[4] == ASE_T('/') && p[6] == ASE_T('.'))
	{
		if (!ASE_ISDIGIT(p[5])) return ASE_NULL;
		if (!ASE_ISDIGIT(p[7])) return ASE_NULL;
		req->vers.major = p[5] - ASE_T('0');
		req->vers.minor = p[7] - ASE_T('0');
		p += 8;
	}
	else return ASE_NULL;

	while (ASE_ISSPACE(*p)) 
	{
		if (*p++ == ASE_T('\n')) goto ok;
	}

	/* not terminating with a new line.
	 * maybe garbage after the request line */
	if (*p != ASE_T('\0')) return ASE_NULL;

ok:
	/* returns the next position */
	return p;
}

ase_char_t* ase_parsehttphdr (ase_char_t* buf, ase_http_hdr_t* hdr)
{
	ase_char_t* p = buf, * last;

	/* ignore leading spaces including CR and NL */
	while (ASE_ISSPACE(*p)) p++;

	if (*p == ASE_T('\0')) 
	{
		/* no more header line */
		ASE_MEMSET (hdr, 0, ASE_SIZEOF(*hdr));
		return p;
	}

	if (!is_http_token(*p)) return ASE_NULL;

	hdr->name.ptr = p;
	do { p++; } while (is_http_token(*p));

	last = p;
	hdr->name.len = last - hdr->name.ptr;

	while (is_http_space(*p)) p++;
	if (*p != ASE_T(':')) return ASE_NULL;

	*last = ASE_T('\0');

	do { p++; } while (is_http_space(*p));

	hdr->value.ptr = last = p;
	while (ASE_ISPRINT(*p))
	{
		if (!ASE_ISSPACE(*p++)) last = p;
	}
	hdr->value.len = last - hdr->value.ptr;

	while (ASE_ISSPACE(*p)) 
	{
		if (*p++ == ASE_T('\n')) goto ok;
	}

	/* not terminating with a new line.
	 * maybe garbage after the header line */
	if (*p != ASE_T('\0')) return ASE_NULL;

ok:
	*last = ASE_T('\0');
	return p;
}
