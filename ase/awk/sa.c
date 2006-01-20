/*
 * $Id: sa.c,v 1.3 2006-01-20 16:28:57 bacon Exp $
 */

#include <xp/awk/sa.h>

#ifdef __STAND_ALONE

static xp_char_t* __adjust_format (const xp_char_t* format);

xp_char_t* xp_strdup (const xp_char_t* str)
{
	xp_char_t* tmp;

	tmp = (xp_char_t*)xp_malloc ((xp_strlen(str) + 1) * xp_sizeof(xp_char_t));
	if (tmp == XP_NULL) return XP_NULL;

	xp_strcpy (tmp, str);
	return tmp;
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

int xp_fprintf (XP_FILE* file, const xp_char_t* fmt, ...)
{
	int n;
	xp_va_list ap;

	xp_va_start (ap, fmt);
	n = xp_vfprintf (file, fmt, ap);
	xp_va_end (ap);
	return n;
}

int xp_vprintf (const xp_char_t* fmt, xp_va_list ap)
{
	return xp_vfprintf (xp_stdout, fmt, ap);
}

int xp_vfprintf (XP_FILE *stream, const xp_char_t* fmt, xp_va_list ap)
{
	int n;
	xp_char_t* nf = __adjust_format (fmt);
	if (nf == XP_NULL) return -1;

#ifdef XP_CHAR_IS_MCHAR
	n = vfprintf (stream, nf, ap);
#else
	n =  vfwprintf (stream, nf, ap);
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

#if defined(XP_CHAR_IS_MCHAR)
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
	if (str == XP_NULL) {
		str = (xp_str_t*)xp_malloc (sizeof(xp_str_t));
		if (str == XP_NULL) return XP_NULL;
		str->__dynamic = xp_true;
	}
	else str->__dynamic = xp_false;

	str->buf = (xp_char_t*)xp_malloc (
		xp_sizeof(xp_char_t) * (capa + 1));
	if (str->buf == XP_NULL) {
		if (str->__dynamic) xp_free (str);
		return XP_NULL;
	}

	str->size      = 0;
	str->capa  = capa;
	str->buf[0] = XP_CHAR('\0');

	return str;
}

void xp_str_close (xp_str_t* str)
{
	xp_free (str->buf);
	if (str->__dynamic) xp_free (str);
}

xp_size_t xp_str_cat (xp_str_t* str, const xp_char_t* s)
{
	/* TODO: improve it */
	return xp_str_ncat (str, s, xp_strlen(s));
}

static xp_size_t __strncpy (xp_char_t* buf, const xp_char_t* str, xp_size_t len)
{
	const xp_char_t* end = str + len;
	while (str < end) *buf++ = *str++;
	*buf = XP_CHAR('\0');
	return len;
}

xp_size_t xp_str_ncat (xp_str_t* str, const xp_char_t* s, xp_size_t len)
{
	xp_char_t* buf;
	xp_size_t capa;

	if (len > str->capa - str->size) {
		capa = str->size + len;

		/* double the capa if necessary for concatenation */
		if (capa < str->capa * 2) capa = str->capa * 2;

		buf = (xp_char_t*)xp_realloc (
			str->buf, xp_sizeof(xp_char_t) * (capa + 1));
		if (buf == XP_NULL) return (xp_size_t)-1;

		str->capa = capa;
		str->buf = buf;
	}

	str->size += __strncpy (&str->buf[str->size], s, len);
	str->buf[str->size] = XP_CHAR('\0');
	return str->size;
}

xp_size_t xp_str_ccat (xp_str_t* str, xp_char_t c)
{
	return xp_str_ncat (str, &c, 1);
}

void xp_str_clear (xp_str_t* str)
{
	str->size = 0;
	str->buf[0] = XP_CHAR('\0');
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
	int modifier;
	xp_str_t str;
	xp_char_t ch;

	if (xp_str_open (&str, 256) == XP_NULL) return XP_NULL;

	while (*fp != XP_CHAR('\0')) {
		while (*fp != XP_CHAR('\0') && *fp != XP_CHAR('%')) {
			ADDC (str, *fp++);
		}

		if (*fp == XP_CHAR('\0')) break;
		xp_assert (*fp == XP_CHAR('%'));

		ch = *fp++;	
		ADDC (str, ch); /* add % */

		ch = *fp++;

		/* flags */
		for (;;) {
			if (ch == XP_CHAR(' ') || ch == XP_CHAR('+') ||
			    ch == XP_CHAR('-') || ch == XP_CHAR('#')) {
				ADDC (str, ch);
			}
			else if (ch == XP_CHAR('0')) {
				ADDC (str, ch);
				ch = *fp++; 
				break;
			}
			else break;

			ch = *fp++;
		}

		/* check the width */
		if (ch == XP_CHAR('*')) ADDC (str, ch);
		else {
			while (xp_isdigit(ch)) {
				ADDC (str, ch);
				ch = *fp++;
			}
		}

		/* precision */
		if (ch == XP_CHAR('.')) {
			ADDC (str, ch);
			ch = *fp++;

			if (ch == XP_CHAR('*')) ADDC (str, ch);
			else {
				while (xp_isdigit(ch)) {
					ADDC (str, ch);
					ch = *fp++;
				}
			}
		}

		/* modifier */
		for (modifier = 0;;) {
			if (ch == XP_CHAR('h')) modifier = MOD_SHORT;
			else if (ch == XP_CHAR('l')) {
				modifier = (modifier == MOD_LONG)? MOD_LONGLONG: MOD_LONG;
			}
			else break;
			ch = *fp++;
		}		


		/* type */
		if (ch == XP_CHAR('%')) ADDC (str, ch);
		else if (ch == XP_CHAR('c') || ch == XP_CHAR('s')) {
#if !defined(XP_CHAR_IS_MCHAR) && !defined(_WIN32)
			ADDC (str, 'l');
#endif
			ADDC (str, ch);
		}
		else if (ch == XP_CHAR('C') || ch == XP_CHAR('S')) {
#ifdef _WIN32
			ADDC (str, ch);
#else
	#ifdef XP_CHAR_IS_MCHAR
			ADDC (str, 'l');
	#endif
			ADDC (str, xp_tolower(ch));
#endif
		}
		else if (ch == XP_CHAR('d') || ch == XP_CHAR('i') || 
		         ch == XP_CHAR('o') || ch == XP_CHAR('u') || 
		         ch == XP_CHAR('x') || ch == XP_CHAR('X')) {
			if (modifier == MOD_SHORT) {
				ADDC (str, 'h');
			}
			else if (modifier == MOD_LONG) {
				ADDC (str, 'l');
			}
			else if (modifier == MOD_LONGLONG) {
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
		else if (ch == XP_CHAR('\0')) break;
		else ADDC (str, ch);
	}

	return xp_str_cield (&str);
}

#endif
