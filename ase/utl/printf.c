/*
 * $Id: printf.c,v 1.1 2006-12-24 16:07:13 bacon Exp $
 */

#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>

static ase_char_t* __adjust_format (const ase_char_t* format);

int xp_vprintf (const ase_char_t* fmt, va_list ap);
int xp_vfprintf (FILE *stream, const ase_char_t* fmt, va_list ap);
int xp_vsprintf (ase_char_t* buf, size_t size, const ase_char_t* fmt, va_list ap);

int xp_printf (const ase_char_t* fmt, ...)
{
	int n;
	va_list ap;

	va_start (ap, fmt);
	n = xp_vprintf (fmt, ap);
	va_end (ap);
	return n;
}

int xp_fprintf (FILE* file, const ase_char_t* fmt, ...)
{
	int n;
	va_list ap;

	va_start (ap, fmt);
	n = xp_vfprintf (file, fmt, ap);
	va_end (ap);
	return n;
}

int xp_vprintf (const ase_char_t* fmt, va_list ap)
{
	return xp_vfprintf (stdout, fmt, ap);
}

int xp_vfprintf (FILE *stream, const ase_char_t* fmt, va_list ap)
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

int xp_sprintf (ase_char_t* buf, size_t size, const ase_char_t* fmt, ...)
{
	int n;
	va_list ap;

	va_start (ap, fmt);
	n = xp_vsprintf (buf, size, fmt, ap);
	va_end (ap);
	return n;
}

int xp_vsprintf (ase_char_t* buf, size_t size, const ase_char_t* fmt, va_list ap)
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

#define MOD_SHORT       1
#define MOD_LONG        2
#define MOD_LONGLONG    3

#define ADDC(str,c) \
	do { \
		if (xp_str_ccat(&str, c) == (size_t)-1) { \
			xp_str_close (&str); \
			return NULL; \
		} \
	} while (0)

static ase_char_t* __adjust_format (const ase_char_t* format)
{
	const ase_char_t* fp = format;
	int modifier;
	xp_str_t str;
	ase_char_t ch;

	if (xp_str_open (&str, 256) == NULL) return NULL;

	while (*fp != ASE_T('\0')) 
	{
		while (*fp != ASE_T('\0') && *fp != ASE_T('%')) 
		{
			ADDC (str, *fp++);
		}

		if (*fp == ASE_T('\0')) break;
		xp_assert (*fp == ASE_T('%'));

		ch = *fp++;	
		ADDC (str, ch); /* add % */

		ch = *fp++;

		/* flags */
		while (1)
		{
			if (ch == ASE_T(' ') || ch == ASE_T('+') ||
			    ch == ASE_T('-') || ch == ASE_T('#')) 
			{
				ADDC (str, ch);
				ch = *fp++;
			}
			else 
			{
				if (ch == ASE_T('0')) 
				{
					ADDC (str, ch);
					ch = *fp++; 
				}

				break;
			}
		}

		/* check the width */
		if (ch == ASE_T('*')) 
		{
			ADDC (str, ch);
			ch = *fp++;
		}
		else 
		{
			while (xp_isdigit(ch)) 
			{
				ADDC (str, ch);
				ch = *fp++;
			}
		}

		/* precision */
		if (ch == ASE_T('.')) 
		{
			ADDC (str, ch);
			ch = *fp++;

			if (ch == ASE_T('*')) 
			{
				ADDC (str, ch);
				ch = *fp++;
			}
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
			if (ch == ASE_T('h')) modifier = MOD_SHORT;
			else if (ch == ASE_T('l')) 
			{
				modifier = (modifier == MOD_LONG)? MOD_LONGLONG: MOD_LONG;
			}
			else break;
			ch = *fp++;
		}		


		/* type */
		if (ch == ASE_T('%')) ADDC (str, ch);
		else if (ch == ASE_T('c') || ch == ASE_T('s')) 
		{
#if !defined(ASE_CHAR_IS_MCHAR) && !defined(_WIN32)
			ADDC (str, 'l');
#endif
			ADDC (str, ch);
		}
		else if (ch == ASE_T('C') || ch == ASE_T('S')) 
		{
#ifdef _WIN32
			ADDC (str, ch);
#else
	#ifdef ASE_CHAR_IS_MCHAR
			ADDC (str, 'l');
	#endif
			ADDC (str, xp_tolower(ch));
#endif
		}
		else if (ch == ASE_T('d') || ch == ASE_T('i') || 
		         ch == ASE_T('o') || ch == ASE_T('u') || 
		         ch == ASE_T('x') || ch == ASE_T('X')) 
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
		else if (ch == ASE_T('\0')) break;
		else ADDC (str, ch);
	}

	return xp_str_cield (&str);
}

