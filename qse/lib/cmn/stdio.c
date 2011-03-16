/*
 * $Id: stdio.c 463 2008-12-09 06:52:03Z baconevi $
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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

#include <qse/cmn/stdio.h>
#include <qse/cmn/chr.h>

#include <wchar.h>
#include <stdlib.h>
#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 2048
#endif

#if defined(_WIN32)

int qse_vsprintf (qse_char_t* buf, size_t size, const qse_char_t* fmt, va_list ap)
{
	int n;

#ifdef QSE_CHAR_IS_MCHAR
	n = _vsnprintf (buf, size, fmt, ap);
#else
	n = _vsnwprintf (buf, size, fmt, ap);
#endif
	if (n < 0 || (size_t)n >= size)
	{
		if (size > 0) buf[size-1] = QSE_T('\0');
		n = -1;
	}

	return n;
}

int qse_sprintf (qse_char_t* buf, size_t size, const qse_char_t* fmt, ...)
{
	int n;
	va_list ap;

	va_start (ap, fmt);
	n = qse_vsprintf (buf, size, fmt, ap);
	va_end (ap);
	return n;
}

#else

static qse_char_t* __adjust_format (const qse_char_t* format);

int qse_vfprintf (QSE_FILE *stream, const qse_char_t* fmt, va_list ap)
{
	int n;
	qse_char_t* nf = __adjust_format (fmt);
	if (nf == NULL) return -1;

#ifdef QSE_CHAR_IS_MCHAR
	n = vfprintf (stream, nf, ap);
#else
	n = vfwprintf (stream, nf, ap);
#endif
	free (nf);
	return n;
}

int qse_vprintf (const qse_char_t* fmt, va_list ap)
{
	return qse_vfprintf (stdout, fmt, ap);
}

int qse_fprintf (QSE_FILE* file, const qse_char_t* fmt, ...)
{
	int n;
	va_list ap;

	va_start (ap, fmt);
	n = qse_vfprintf (file, fmt, ap);
	va_end (ap);
	return n;
}

int qse_printf (const qse_char_t* fmt, ...)
{
	int n;
	va_list ap;

	va_start (ap, fmt);
	n = qse_vprintf (fmt, ap);
	va_end (ap);
	return n;
}

int qse_vsprintf (qse_char_t* buf, size_t size, const qse_char_t* fmt, va_list ap)
{
	int n;
	qse_char_t* nf = __adjust_format (fmt);
	if (nf == NULL) return -1;

#if defined(QSE_CHAR_IS_MCHAR)
	n = vsnprintf (buf, size, nf, ap);
#elif defined(_WIN32)
	n = _vsnwprintf (buf, size, nf, ap);
#else
	n = vswprintf (buf, size, nf, ap);
#endif
	if (n < 0 || (size_t)n >= size)
	{
		if (size > 0) buf[size-1] = QSE_T('\0');
		n = -1;
	}

	free (nf);
	return n;
}

int qse_sprintf (qse_char_t* buf, size_t size, const qse_char_t* fmt, ...)
{
	int n;
	va_list ap;

	va_start (ap, fmt);
	n = qse_vsprintf (buf, size, fmt, ap);
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
			qse_char_t* tmp; \
			tmp = (qse_char_t*)realloc ( \
				buf.ptr, sizeof(qse_char_t)*(buf.cap+256+1)); \
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

static qse_char_t* __adjust_format (const qse_char_t* format)
{
	const qse_char_t* fp = format;
	int modifier;
	qse_char_t ch;

	struct
	{
		qse_char_t* ptr;
		qse_size_t  len;
		qse_size_t  cap;
	} buf;

	buf.len = 0;
	buf.cap = 256;
#if (defined(vms) || defined(__vms)) && (QSE_SIZEOF_VOID_P >= 8)
	buf.ptr = (qse_char_t*) _malloc32 (sizeof(qse_char_t)*(buf.cap+1));
#else
	buf.ptr = (qse_char_t*) malloc (sizeof(qse_char_t)*(buf.cap+1));
#endif
	if (buf.ptr == NULL) return NULL;

	while (*fp != QSE_T('\0')) 
	{
		while (*fp != QSE_T('\0') && *fp != QSE_T('%')) 
		{
			ADDC (buf, *fp++);
		}

		if (*fp == QSE_T('\0')) break;

		ch = *fp++;	
		ADDC (buf, ch); /* add % */

		ch = *fp++;

		/* flags */
		while (1)
		{
			if (ch == QSE_T(' ') || ch == QSE_T('+') ||
			    ch == QSE_T('-') || ch == QSE_T('#')) 
			{
				ADDC (buf, ch);
				ch = *fp++;
			}
			else 
			{
				if (ch == QSE_T('0')) 
				{
					ADDC (buf, ch);
					ch = *fp++; 
				}

				break;
			}
		}

		/* check the width */
		if (ch == QSE_T('*')) 
		{
			ADDC (buf, ch);
			ch = *fp++;
		}
		else 
		{
			while (QSE_ISDIGIT(ch)) 
			{
				ADDC (buf, ch);
				ch = *fp++;
			}
		}

		/* precision */
		if (ch == QSE_T('.')) 
		{
			ADDC (buf, ch);
			ch = *fp++;

			if (ch == QSE_T('*')) 
			{
				ADDC (buf, ch);
				ch = *fp++;
			}
			else 
			{
				while (QSE_ISDIGIT(ch)) 
				{
					ADDC (buf, ch);
					ch = *fp++;
				}
			}
		}

		/* modifier */
		for (modifier = 0;;) 
		{
			if (ch == QSE_T('h')) modifier = MOD_SHORT;
			else if (ch == QSE_T('l')) 
			{
				modifier = (modifier == MOD_LONG)? MOD_LONGLONG: MOD_LONG;
			}
			else break;
			ch = *fp++;
		}		


		/* type */
		if (ch == QSE_T('%')) ADDC (buf, ch);
		else if (ch == QSE_T('c') || ch == QSE_T('s')) 
		{
#if !defined(QSE_CHAR_IS_MCHAR) && !defined(_WIN32)
			ADDC (buf, 'l');
#endif
			ADDC (buf, ch);
		}
		else if (ch == QSE_T('C') || ch == QSE_T('S')) 
		{
#if defined(_WIN32)
			ADDC (buf, ch);
#else
	#ifdef QSE_CHAR_IS_MCHAR
			ADDC (buf, 'l');
	#endif
			ADDC (buf, QSE_TOLOWER(ch));
#endif
		}
		else if (ch == QSE_T('d') || ch == QSE_T('i') || 
		         ch == QSE_T('o') || ch == QSE_T('u') || 
		         ch == QSE_T('x') || ch == QSE_T('X')) 
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
		else if (ch == QSE_T('\0')) break;
		else ADDC (buf, ch);
	}

	buf.ptr[buf.len] = QSE_T('\0');

	return buf.ptr;
}

#endif

int qse_dprintf (const qse_char_t* fmt, ...)
{
	int n;
	va_list ap;

	va_start (ap, fmt);
	n = qse_vfprintf (stderr, fmt, ap);
	va_end (ap);
	return n;
}

QSE_FILE* qse_fopen (const qse_char_t* path, const qse_char_t* mode)
{
#if defined(QSE_CHAR_IS_MCHAR)
	return fopen (path, mode);
#elif defined(_WIN32)
	return _wfopen (path, mode);
#else

	char path_mb[PATH_MAX + 1];
	char mode_mb[32];
	size_t n;

	n = wcstombs (path_mb, path, QSE_COUNTOF(path_mb));
	if (n == (size_t)-1) return NULL;
	if (n == QSE_COUNTOF(path_mb)) path_mb[QSE_COUNTOF(path_mb)-1] = '\0';

	n = wcstombs (mode_mb, mode, QSE_COUNTOF(mode_mb));
	if (n == (size_t)-1) return NULL;
	if (n == QSE_COUNTOF(mode_mb)) path_mb[QSE_COUNTOF(mode_mb)-1] = '\0';

	return fopen (path_mb, mode_mb);
#endif
}

QSE_FILE* qse_popen (const qse_char_t* cmd, const qse_char_t* mode)
{
#if defined(QSE_CHAR_IS_MCHAR)
#if defined(__OS2__)
	return _popen (cmd, mode);
#else
	return popen (cmd, mode);
#endif
#elif defined(_WIN32) || defined(__OS2__)
	return _wpopen (cmd, mode);
#else
	char cmd_mb[PATH_MAX + 1];
	char mode_mb[32];
	size_t n;

	n = wcstombs (cmd_mb, cmd, QSE_COUNTOF(cmd_mb));
	if (n == (size_t)-1) return NULL;
	if (n == QSE_COUNTOF(cmd_mb)) cmd_mb[QSE_COUNTOF(cmd_mb)-1] = '\0';

	n = wcstombs (mode_mb, mode, QSE_COUNTOF(mode_mb));
	if (n == (size_t)-1) return NULL;
	if (n == QSE_COUNTOF(mode_mb)) cmd_mb[QSE_COUNTOF(mode_mb)-1] = '\0';

	return popen (cmd_mb, mode_mb);
#endif
}

static int isnl (const qse_char_t* ptr, qse_size_t len, void* delim)
{
	return (ptr[len-1] == *(qse_char_t*)delim)? 1: 0;
}

qse_ssize_t qse_getline (qse_char_t **buf, qse_size_t *n, QSE_FILE *fp)
{
	qse_char_t nl = QSE_T('\n');
	return qse_getdelim (buf, n, isnl, &nl, fp);
}

qse_ssize_t qse_getdelim (
	qse_char_t **buf, qse_size_t *n, 
	qse_getdelim_t fn, void* fnarg, QSE_FILE *fp)
{
	qse_char_t* b;
	qse_size_t capa;
	qse_size_t len = 0;
	int x;

	QSE_ASSERT (buf != QSE_NULL);
	QSE_ASSERT (n != QSE_NULL);

	b = *buf;
	capa = *n;

	if (b == QSE_NULL)
	{
		capa = 256;
	#if (defined(vms) || defined(__vms)) && (QSE_SIZEOF_VOID_P >= 8)
		b = (qse_char_t*) _malloc32 (sizeof(qse_char_t)*(capa+1));
	#else
		b = (qse_char_t*) malloc (sizeof(qse_char_t)*(capa+1));
	#endif
		if (b == QSE_NULL) return -2;
	}

	if (qse_feof(fp))
	{
		len = (qse_size_t)-1;
		goto exit_task;
	}

	while (1)
	{
		qse_cint_t c = qse_fgetc(fp);
		if (c == QSE_CHAR_EOF)
		{
			if (qse_ferror(fp)) 
			{
				len = (qse_size_t)-2;
				goto exit_task;
			}
			if (len == 0)
			{
				len = (qse_size_t)-1;
				goto exit_task;
			}

			break;
		}

		if (len+1 >= capa)
		{
			qse_size_t ncapa = capa + 256;
			qse_char_t* nb;

			nb = realloc (b, ncapa*sizeof(qse_char_t));
			if (nb == QSE_NULL)
			{
				len = (qse_size_t)-2;
				goto exit_task;
			}

			b = nb;
			capa = ncapa;
		}

		b[len++] = c;

		x = fn (b, len, fnarg);
		if (x < 0)
		{
			len = (qse_size_t)-3;
			goto exit_task;
		}
		if (x > 0) break;
	}
	b[len] = QSE_T('\0');

exit_task:
	*buf = b;
	*n = capa;

	return (qse_ssize_t)len;
}
