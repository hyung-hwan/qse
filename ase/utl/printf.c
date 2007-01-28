/*
 * $Id: printf.c,v 1.8 2007-01-28 11:33:23 bacon Exp $
 */

#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>

#include <ctype.h>
#include <wctype.h>

#if defined(_WIN32)

#include <tchar.h>
#define ase_printf   _tprintf
#define ase_vprintf  _vtprintf
#define ase_fprintf  _ftprintf
#define ase_vfprintf _vftprintf

int ase_vsprintf (ase_char_t* buf, size_t size, const ase_char_t* fmt, va_list ap)
{
	int n;

	n = _vsntprintf (buf, size, fmt, ap);
	if (n < 0 || (size_t)n >= size)
	{
		if (size > 0) buf[size-1] = ASE_T('\0');
		n = -1;
	}

	return n;
}

int ase_sprintf (ase_char_t* buf, size_t size, const ase_char_t* fmt, ...)
{
	int n;
	va_list ap;

	va_start (ap, fmt);
	n = ase_vsprintf (buf, size, fmt, ap);
	va_end (ap);
	return n;
}

#else

#if defined(ASE_CHAR_IS_MCHAR)
	#define ase_tolower(x) tolower(x)
	#define ase_isdigit(x) isdigit(x)
#elif defined(ASE_CHAR_IS_WCHAR)
	#define ase_tolower(x) towlower(x)
	#define ase_isdigit(x) iswdigit(x)
#else
	#error define ASE_CHAR_IS_MCHAR or ASE_CHAR_IS_WCHAR
#endif

static ase_char_t* __adjust_format (const ase_char_t* format);

int ase_vfprintf (FILE *stream, const ase_char_t* fmt, va_list ap)
{
	int n;
	ase_char_t* nf = __adjust_format (fmt);
	if (nf == NULL) return -1;

#ifdef ASE_CHAR_IS_MCHAR
	n = vfprintf (stream, nf, ap);
#else
	n = vfwprintf (stream, nf, ap);
#endif
	free (nf);
	return n;
}

int ase_vprintf (const ase_char_t* fmt, va_list ap)
{
	return ase_vfprintf (stdout, fmt, ap);
}

int ase_fprintf (FILE* file, const ase_char_t* fmt, ...)
{
	int n;
	va_list ap;

	va_start (ap, fmt);
	n = ase_vfprintf (file, fmt, ap);
	va_end (ap);
	return n;
}

int ase_printf (const ase_char_t* fmt, ...)
{
	int n;
	va_list ap;

	va_start (ap, fmt);
	n = ase_vprintf (fmt, ap);
	va_end (ap);
	return n;
}

int ase_vsprintf (ase_char_t* buf, size_t size, const ase_char_t* fmt, va_list ap)
{
	int n;
	ase_char_t* nf = __adjust_format (fmt);
	if (nf == NULL) return -1;

#if defined(ASE_CHAR_IS_MCHAR)
	n = vsnprintf (buf, size, nf, ap);
#elif defined(_WIN32)
	n = _vsnwprintf (buf, size, nf, ap);
#else
	n = vswprintf (buf, size, nf, ap);
#endif
	if (n < 0 || (size_t)n >= size)
	{
		if (size > 0) buf[size-1] = ASE_T('\0');
		n = -1;
	}

	free (nf);
	return n;
}

int ase_sprintf (ase_char_t* buf, size_t size, const ase_char_t* fmt, ...)
{
	int n;
	va_list ap;

	va_start (ap, fmt);
	n = ase_vsprintf (buf, size, fmt, ap);
	va_end (ap);
	return n;
}

#define MOD_SHORT       1
#define MOD_LONG        2
#define MOD_LONGLONG    3

#define ADDC(buf,c) \
	do { \
		if (buf.len >= buf.cap) \
		{ \
			ase_char_t* tmp; \
			tmp = (ase_char_t*)realloc ( \
				buf.ptr, sizeof(ase_char_t)*(buf.cap+256+1)); \
			if (tmp == NULL) \
			{ \
				free (buf.ptr); \
				return NULL; \
			} \
			buf.ptr = tmp; \
			buf.cap = buf.cap + 256; \
		} \
		buf.ptr[buf.len++] = c; \
	} while (0)

static ase_char_t* __adjust_format (const ase_char_t* format)
{
	const ase_char_t* fp = format;
	int modifier;
	ase_char_t ch;

	struct
	{
		ase_char_t* ptr;
		ase_size_t  len;
		ase_size_t  cap;
	} buf;

	buf.len = 0;
	buf.cap = 256;
	buf.ptr = (ase_char_t*) malloc (sizeof(ase_char_t)*(buf.cap+1));
	if (buf.ptr == NULL) return NULL;

	while (*fp != ASE_T('\0')) 
	{
		while (*fp != ASE_T('\0') && *fp != ASE_T('%')) 
		{
			ADDC (buf, *fp++);
		}

		if (*fp == ASE_T('\0')) break;

		ch = *fp++;	
		ADDC (buf, ch); /* add % */

		ch = *fp++;

		/* flags */
		while (1)
		{
			if (ch == ASE_T(' ') || ch == ASE_T('+') ||
			    ch == ASE_T('-') || ch == ASE_T('#')) 
			{
				ADDC (buf, ch);
				ch = *fp++;
			}
			else 
			{
				if (ch == ASE_T('0')) 
				{
					ADDC (buf, ch);
					ch = *fp++; 
				}

				break;
			}
		}

		/* check the width */
		if (ch == ASE_T('*')) 
		{
			ADDC (buf, ch);
			ch = *fp++;
		}
		else 
		{
			while (ase_isdigit(ch)) 
			{
				ADDC (buf, ch);
				ch = *fp++;
			}
		}

		/* precision */
		if (ch == ASE_T('.')) 
		{
			ADDC (buf, ch);
			ch = *fp++;

			if (ch == ASE_T('*')) 
			{
				ADDC (buf, ch);
				ch = *fp++;
			}
			else 
			{
				while (ase_isdigit(ch)) 
				{
					ADDC (buf, ch);
					ch = *fp++;
				}
			}
		}

		/* modifier */
		for (modifier = 0;;) 
		{
			if (ch == ASE_T('h')) modifier = MOD_SHORT;
			else if (ch == ASE_T('l')) 
			{
				modifier = (modifier == MOD_LONG)? MOD_LONGLONG: MOD_LONG;
			}
			else break;
			ch = *fp++;
		}		


		/* type */
		if (ch == ASE_T('%')) ADDC (buf, ch);
		else if (ch == ASE_T('c') || ch == ASE_T('s')) 
		{
#if !defined(ASE_CHAR_IS_MCHAR) && !defined(_WIN32)
			ADDC (buf, 'l');
#endif
			ADDC (buf, ch);
		}
		else if (ch == ASE_T('C') || ch == ASE_T('S')) 
		{
#ifdef _WIN32
			ADDC (buf, ch);
#else
	#ifdef ASE_CHAR_IS_MCHAR
			ADDC (buf, 'l');
	#endif
			ADDC (buf, ase_tolower(ch));
#endif
		}
		else if (ch == ASE_T('d') || ch == ASE_T('i') || 
		         ch == ASE_T('o') || ch == ASE_T('u') || 
		         ch == ASE_T('x') || ch == ASE_T('X')) 
		{
			if (modifier == MOD_SHORT) 
			{
				ADDC (buf, 'h');
			}
			else if (modifier == MOD_LONG) 
			{
				ADDC (buf, 'l');
			}
			else if (modifier == MOD_LONGLONG) 
			{
#if defined(_WIN32) && !defined(__LCC__)
				ADDC (buf, 'I');
				ADDC (buf, '6');
				ADDC (buf, '4');
#else
				ADDC (buf, 'l');
				ADDC (buf, 'l');
#endif
			}
			ADDC (buf, ch);
		}
		else if (ch == ASE_T('\0')) break;
		else ADDC (buf, ch);
	}

	buf.ptr[buf.len] = ASE_T('\0');
	return buf.ptr;
}

#endif
