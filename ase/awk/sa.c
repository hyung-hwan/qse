/*
 * $Id: sa.c,v 1.28 2006-07-17 04:17:40 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifdef XP_AWK_STAND_ALONE

static xp_char_t* __adjust_format (const xp_char_t* format);

xp_size_t xp_strlen (const xp_char_t* str)
{
	const xp_char_t* p = str;
	while (*p != XP_T('\0')) p++;
	return p - str;
}

xp_char_t* xp_strdup (const xp_char_t* str)
{
	xp_char_t* tmp;

	tmp = (xp_char_t*) xp_malloc (
		(xp_strlen(str) + 1) * xp_sizeof(xp_char_t));
	if (tmp == XP_NULL) return XP_NULL;

	xp_strcpy (tmp, str);
	return tmp;
}

xp_char_t* xp_strxdup (const xp_char_t* str, xp_size_t len)
{
	xp_char_t* tmp;

	tmp = (xp_char_t*) xp_malloc ((len + 1) * xp_sizeof(xp_char_t));
	if (tmp == XP_NULL) return XP_NULL;

	xp_strncpy (tmp, str, len);
	return tmp;
}

xp_char_t* xp_strxdup2 (
	const xp_char_t* str1, xp_size_t len1,
	const xp_char_t* str2, xp_size_t len2)
{
	xp_char_t* tmp;

	tmp = (xp_char_t*) xp_malloc (
		(len1 + len2 + 1) * xp_sizeof(xp_char_t));
	if (tmp == XP_NULL) return XP_NULL;

	xp_strncpy (tmp, str1, len1);
	xp_strncpy (tmp + len1, str2, len2);
	return tmp;
}

xp_size_t xp_strcpy (xp_char_t* buf, const xp_char_t* str)
{
	xp_char_t* org = buf;
	while ((*buf++ = *str++) != XP_T('\0'));
	return buf - org - 1;
}

xp_size_t xp_strncpy (xp_char_t* buf, const xp_char_t* str, xp_size_t len)
{
	const xp_char_t* end = str + len;
	while (str < end) *buf++ = *str++;
	*buf = XP_T('\0');
	return len;
}

int xp_strcmp (const xp_char_t* s1, const xp_char_t* s2)
{
	while (*s1 == *s2) 
	{
		if (*s1 == XP_C('\0')) return 0;
		s1++, s2++;
	}

	return (*s1 > *s2)? 1: -1;
}

int xp_strxncmp (
	const xp_char_t* s1, xp_size_t len1, 
	const xp_char_t* s2, xp_size_t len2)
{
	const xp_char_t* end1 = s1 + len1;
	const xp_char_t* end2 = s2 + len2;

	while (s1 < end1 && s2 < end2 && *s1 == *s2) s1++, s2++;
	if (s1 == end1 && s2 == end2) return 0;
	if (*s1 == *s2) return (s1 < end1)? 1: -1;
	return (*s1 > *s2)? 1: -1;
}

xp_char_t* xp_strtok (const xp_char_t* s, 
	const xp_char_t* delim, xp_char_t** tok, xp_size_t* tok_len)
{
	const xp_char_t* p = s, *d;
	const xp_char_t* sp = XP_NULL, * ep = XP_NULL;
	xp_char_t c; 
	int delim_mode;

	/* skip preceding space xp_char_tacters */
	while (/* *p != XP_T('\0') && */ xp_isspace(*p)) p++;

	if (delim == XP_NULL) delim_mode = 0;
	else {
		delim_mode = 1;
		for (d = delim; *d != XP_T('\0'); d++) 
			if (!xp_isspace(*d)) delim_mode = 2;
	}

	if (delim_mode == 0) { 
		/* when XP_NULL is given as "delim", it has an effect of cutting
		   preceding and trailing space characters off "s". */
		while ((c = *p) != XP_T('\0')) 
		{
			if (!xp_isspace(c)) 
			{
				if (sp == XP_NULL) sp = p;
				ep = p;
			}
			p++;
		}
	}
	else if (delim_mode == 1) 
	{
		while ((c = *p) != XP_T('\0')) 
		{
			if (xp_isspace(c)) break;

			if (sp == XP_NULL) sp = p;
			ep = p++;
		}
	}
	else /* if (delim_mode == 2) */
	{ 
		while ((c = *p) != XP_T('\0')) {
			if (xp_isspace(c)) {
				p++;
				continue;
			}
			for (d = delim; *d; d++) {
				if (c == *d) {
					goto exit_loop;
				}
			}
			if (sp == XP_NULL) sp = p;
			ep = p++;
		}
	}

exit_loop:
	if (sp == XP_NULL) {
		*tok = XP_NULL;
		*tok_len = (xp_size_t)0;
	}
	else {
		*tok = (xp_char_t*)sp;
		*tok_len = ep - sp + 1;
	}
	return (c == XP_T('\0'))? XP_NULL: ((xp_char_t*)++p);
}

xp_char_t* xp_strxtok (const xp_char_t* s, xp_size_t len,
	const xp_char_t* delim, xp_char_t** tok, xp_size_t* tok_len)
{
	const xp_char_t* p = s, *d;
	const xp_char_t* end = s + len;	
	const xp_char_t* sp = XP_NULL, * ep = XP_NULL;
	xp_char_t c; 
	int delim_mode;

	/* skip preceding space xp_char_tacters */
	while (p < end && xp_isspace(*p)) p++;

	if (delim == XP_NULL) delim_mode = 0;
	else 
	{
		delim_mode = 1;
		for (d = delim; *d != XP_T('\0'); d++) 
			if (!xp_isspace(*d)) delim_mode = 2;
	}

	if (delim_mode == 0) 
	{ 
		/* when XP_NULL is given as "delim", it has an effect of cutting
		   preceding and trailing space xp_char_tacters off "s". */
		while (p < end) 
		{
			c = *p;

			if (!xp_isspace(c)) 
			{
				if (sp == XP_NULL) sp = p;
				ep = p;
			}
			p++;
		}
	}
	else if (delim_mode == 1) 
	{
		while (p < end) 
		{
			c = *p;
			if (xp_isspace(c)) break;
			if (sp == XP_NULL) sp = p;
			ep = p++;
		}
	}
	else /* if (delim_mode == 2) */ 
	{
		while (p < end) 
		{
			c = *p;
			if (xp_isspace(c)) 
			{
				p++;
				continue;
			}
			for (d = delim; *d != XP_T('\0'); d++) 
			{
				if (c == *d) goto exit_loop;
			}
			if (sp == XP_NULL) sp = p;
			ep = p++;
		}
	}

exit_loop:
	if (sp == XP_NULL) 
	{
		*tok = XP_NULL;
		*tok_len = (xp_size_t)0;
	}
	else 
	{
		*tok = (xp_char_t*)sp;
		*tok_len = ep - sp + 1;
	}

	return (p >= end)? XP_NULL: ((xp_char_t*)++p);
}

int xp_printf (const xp_char_t* fmt, ...)
{
	int n;
	xp_va_list ap;

	xp_va_start (ap, fmt);
	n = xp_vprintf (fmt, ap);
	xp_va_end (ap);
	return n;
}

int xp_vprintf (const xp_char_t* fmt, xp_va_list ap)
{
	int n;
	xp_char_t* nf = __adjust_format (fmt);
	if (nf == XP_NULL) return -1;

#ifdef XP_CHAR_IS_MCHAR
	n = vprintf (nf, ap);
#else
	n =  vwprintf (nf, ap);
#endif

	xp_free (nf);
	return n;
}

int xp_sprintf (xp_char_t* buf, xp_size_t size, const xp_char_t* fmt, ...)
{
	int n;
	xp_va_list ap;

	xp_va_start (ap, fmt);
	n = xp_vsprintf (buf, size, fmt, ap);
	xp_va_end (ap);
	return n;
}

int xp_vsprintf (xp_char_t* buf, xp_size_t size, const xp_char_t* fmt, xp_va_list ap)
{
	int n;
	xp_char_t* nf = __adjust_format (fmt);
	if (nf == XP_NULL) return -1;

#if defined(dos) || defined(__dos)
	n = vsprintf (buf, nf, ap); /* TODO: write your own vsnprintf */
#elif defined(XP_CHAR_IS_MCHAR)
	n = vsnprintf (buf, size, nf, ap);
#elif defined(_WIN32)
	n = _vsnwprintf (buf, size, nf, ap);
#else
	n = vswprintf (buf, size, nf, ap);
#endif
	xp_free (nf);
	return n;
}

xp_str_t* xp_str_open (xp_str_t* str, xp_size_t capa)
{
	if (str == XP_NULL) 
	{
		str = (xp_str_t*) xp_malloc (sizeof(xp_str_t));
		if (str == XP_NULL) return XP_NULL;
		str->__dynamic = xp_true;
	}
	else str->__dynamic = xp_false;

	str->buf = (xp_char_t*) xp_malloc (xp_sizeof(xp_char_t) * (capa + 1));
	if (str->buf == XP_NULL) 
	{
		if (str->__dynamic) xp_free (str);
		return XP_NULL;
	}

	str->size = 0;
	str->capa  = capa;
	str->buf[0] = XP_T('\0');

	return str;
}

void xp_str_close (xp_str_t* str)
{
	xp_free (str->buf);
	if (str->__dynamic) xp_free (str);
}

void xp_str_forfeit (xp_str_t* str)
{
	if (str->__dynamic) xp_free (str);
}

xp_size_t xp_str_cpy (xp_str_t* str, const xp_char_t* s)
{
	/* TODO: improve it */
	return xp_str_ncpy (str, s, xp_strlen(s));
}

xp_size_t xp_str_ncpy (xp_str_t* str, const xp_char_t* s, xp_size_t len)
{
	xp_char_t* buf;

	if (len > str->capa) 
	{
		buf = (xp_char_t*) xp_malloc (xp_sizeof(xp_char_t) * (len + 1));
		if (buf == XP_NULL) return (xp_size_t)-1;

		xp_free (str->buf);
		str->capa = len;
		str->buf = buf;
	}

	str->size = xp_strncpy (str->buf, s, len);
	str->buf[str->size] = XP_T('\0');
	return str->size;
}

xp_size_t xp_str_cat (xp_str_t* str, const xp_char_t* s)
{
	/* TODO: improve it */
	return xp_str_ncat (str, s, xp_strlen(s));
}

xp_size_t xp_str_ncat (xp_str_t* str, const xp_char_t* s, xp_size_t len)
{
	xp_char_t* buf;
	xp_size_t capa;

	if (len > str->capa - str->size) 
	{
		capa = str->size + len;

		/* double the capa if necessary for concatenation */
		if (capa < str->capa * 2) capa = str->capa * 2;

		buf = (xp_char_t*) xp_realloc (
			str->buf, xp_sizeof(xp_char_t) * (capa + 1));
		if (buf == XP_NULL) return (xp_size_t)-1;

		str->capa = capa;
		str->buf = buf;
	}

	str->size += xp_strncpy (&str->buf[str->size], s, len);
	str->buf[str->size] = XP_T('\0');
	return str->size;
}

xp_size_t xp_str_ccat (xp_str_t* str, xp_char_t c)
{
	return xp_str_ncat (str, &c, 1);
}

xp_size_t xp_str_nccat (xp_str_t* str, xp_char_t c, xp_size_t len)
{
	while (len > 0)
	{
		if (xp_str_ncat (str, &c, 1) == (xp_size_t)-1) 
		{
			return (xp_size_t)-1;
		}

		len--;
	}
	return str->size;
}

void xp_str_clear (xp_str_t* str)
{
	str->size = 0;
	str->buf[0] = XP_T('\0');
}


#define MOD_SHORT       1
#define MOD_LONG        2
#define MOD_LONGLONG    3

#define ADDC(str,c) \
	do { \
		if (xp_str_ccat(&str, c) == (xp_size_t)-1) { \
			xp_str_close (&str); \
			return XP_NULL; \
		} \
	} while (0)

static xp_char_t* __adjust_format (const xp_char_t* format)
{
	const xp_char_t* fp = format;
	xp_char_t* tmp;
	xp_str_t str;
	xp_char_t ch;
	int modifier;

	if (xp_str_open (&str, 256) == XP_NULL) return XP_NULL;

	while (*fp != XP_T('\0')) 
	{
		while (*fp != XP_T('\0') && *fp != XP_T('%')) 
		{
			ADDC (str, *fp++);
		}

		if (*fp == XP_T('\0')) break;
		xp_assert (*fp == XP_T('%'));

		ch = *fp++;	
		ADDC (str, ch); /* add % */

		ch = *fp++;

		/* flags */
		for (;;) 
		{
			if (ch == XP_T(' ') || ch == XP_T('+') ||
			    ch == XP_T('-') || ch == XP_T('#')) 
			{
				ADDC (str, ch);
			}
			else if (ch == XP_T('0')) 
			{
				ADDC (str, ch);
				ch = *fp++; 
				break;
			}
			else break;

			ch = *fp++;
		}

		/* check the width */
		if (ch == XP_T('*')) ADDC (str, ch);
		else 
		{
			while (xp_isdigit(ch)) 
			{
				ADDC (str, ch);
				ch = *fp++;
			}
		}

		/* precision */
		if (ch == XP_T('.')) 
		{
			ADDC (str, ch);
			ch = *fp++;

			if (ch == XP_T('*')) ADDC (str, ch);
			else 
			{
				while (xp_isdigit(ch)) 
				{
					ADDC (str, ch);
					ch = *fp++;
				}
			}
		}

		/* modifier */
		for (modifier = 0;;) 
		{
			if (ch == XP_T('h')) modifier = MOD_SHORT;
			else if (ch == XP_T('l')) 
			{
				modifier = (modifier == MOD_LONG)? MOD_LONGLONG: MOD_LONG;
			}
			else break;
			ch = *fp++;
		}		


		/* type */
		if (ch == XP_T('%')) ADDC (str, ch);
		else if (ch == XP_T('c') || ch == XP_T('s')) 
		{
#if !defined(XP_CHAR_IS_MCHAR) && !defined(_WIN32)
			ADDC (str, 'l');
#endif
			ADDC (str, ch);
		}
		else if (ch == XP_T('C') || ch == XP_T('S')) 
		{
#ifdef _WIN32
			ADDC (str, ch);
#else
	#ifdef XP_CHAR_IS_MCHAR
			ADDC (str, 'l');
	#endif
			ADDC (str, xp_tolower(ch));
#endif
		}
		else if (ch == XP_T('d') || ch == XP_T('i') || 
		         ch == XP_T('o') || ch == XP_T('u') || 
		         ch == XP_T('x') || ch == XP_T('X')) 
		{
			if (modifier == MOD_SHORT) 
			{
				ADDC (str, 'h');
			}
			else if (modifier == MOD_LONG) 
			{
				ADDC (str, 'l');
			}
			else if (modifier == MOD_LONGLONG) 
			{
#if defined(_WIN32) && !defined(__LCC__)
				ADDC (str, 'I');
				ADDC (str, '6');
				ADDC (str, '4');
#else
				ADDC (str, 'l');
				ADDC (str, 'l');
#endif
			}
			ADDC (str, ch);
		}
		else if (ch == XP_T('\0')) break;
		else ADDC (str, ch);
	}

	tmp = XP_STR_BUF(&str);
	xp_str_forfeit (&str);
	return tmp;
}

#endif
