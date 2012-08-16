/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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
#include <qse/cmn/mbwc.h>
#include "mem.h"

#include <wchar.h>
#include <stdlib.h>
#include <limits.h>

#ifndef PATH_MAX
#	define PATH_MAX 2048
#endif

static qse_char_t* __adjust_format (const qse_char_t* format);

int qse_vfprintf (QSE_FILE *stream, const qse_char_t* fmt, va_list ap)
{
	int n;
	qse_char_t* nf = __adjust_format (fmt);
	if (nf == NULL) return -1;

	#if defined(QSE_CHAR_IS_MCHAR)
	n = vfprintf (stream, nf, ap);
	#else
	n = vfwprintf (stream, nf, ap);
	#endif
	QSE_MMGR_FREE (QSE_MMGR_GETDFL(), nf);
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

int qse_vsprintf (qse_char_t* buf, qse_size_t size, const qse_char_t* fmt, va_list ap)
{
	int n;
	qse_char_t* nf = __adjust_format (fmt);
	if (nf == NULL) return -1;

	#if defined(QSE_CHAR_IS_MCHAR)
		#if defined(_WIN32) && !defined(__WATCOMC__)
			n = _vsnprintf (buf, size, nf, ap);
		#else
			n = vsnprintf (buf, size, nf, ap);
		#endif
	#else
		#if defined(_WIN32) && !defined(__WATCOMC__)
			n = _vsnwprintf (buf, size, nf, ap);
		#else
			n = vswprintf (buf, size, nf, ap);
		#endif
	#endif

	if (n < 0 || (size_t)n >= size)
	{
		if (size > 0) buf[size-1] = QSE_T('\0');
		n = -1;
	}

	QSE_MMGR_FREE (QSE_MMGR_GETDFL(), nf);
	return n;
}

int qse_sprintf (qse_char_t* buf, qse_size_t size, const qse_char_t* fmt, ...)
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
			tmp = (qse_char_t*) QSE_MMGR_REALLOC ( \
				QSE_MMGR_GETDFL(), buf.ptr, \
				QSE_SIZEOF(qse_char_t) * (buf.cap+256+1)); \
			if (tmp == NULL) \
			{ \
				QSE_MMGR_FREE (QSE_MMGR_GETDFL(), buf.ptr); \
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

	buf.ptr = (qse_char_t*) QSE_MMGR_ALLOC (
		QSE_MMGR_GETDFL(), QSE_SIZEOF(qse_char_t) * (buf.cap+1));
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
		switch (ch)
		{
			case QSE_T('\0'):
				goto done;

			case QSE_T('%'):
			{
				ADDC (buf, ch);
				break;
			}

			case QSE_T('c'):
			case QSE_T('s'):
			{
				if (modifier == MOD_SHORT)
				{
					/* always multibyte */
				#if defined(QSE_CHAR_IS_MCHAR)
					goto mchar_multi;
				#else
					ch = QSE_TOUPPER(ch);
					goto wchar_multi;
				#endif
				}
				else if (modifier == MOD_LONG)
				{
					/* always wide-character */
				#if defined(QSE_CHAR_IS_MCHAR)
					ch = QSE_TOUPPER(ch);
					goto mchar_wide;
				#else
					goto wchar_wide;
				#endif
				}
				else
				{
				#if defined(QSE_CHAR_IS_MCHAR) 
				mchar_multi:
					#if defined(_WIN32) && !defined(__WATCOMC__)
					ADDC (buf, ch);
					#else
					ADDC (buf, ch);
					#endif
				#else
				wchar_wide:
					#if defined(_WIN32) && !defined(__WATCOMC__)
					ADDC (buf, ch);
					#else
					ADDC (buf, QSE_WT('l'));
					ADDC (buf, ch);
					#endif
				#endif
				}
				break;
			}

			case QSE_T('C'):
			case QSE_T('S'):
			{
			#if defined(QSE_CHAR_IS_MCHAR)
			mchar_wide:
				#if defined(_WIN32) && !defined(__WATCOMC__)
				ADDC (buf, ch);
				#else		
				ADDC (buf, QSE_MT('l'));
				ADDC (buf, QSE_TOLOWER(ch));
				#endif
			#else
			wchar_multi:
				#if defined(_WIN32) && !defined(__WATCOMC__)
				ADDC (buf, ch);
				#else
				ADDC (buf, QSE_MT('h'));
				ADDC (buf, QSE_TOLOWER(ch));
				#endif
			#endif
	
				break;
			}

			case QSE_T('d'):
			case QSE_T('i'):
			case QSE_T('o'):
			case QSE_T('u'):
			case QSE_T('x'):
			case QSE_T('X'):
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
				break;
			}

			default:
			{
				ADDC (buf, ch);
				break;
			}
		}
	}

done:
	buf.ptr[buf.len] = QSE_T('\0');
	return buf.ptr;
}

int qse_dprintf (const qse_char_t* fmt, ...)
{
	int n;
	va_list ap;

	va_start (ap, fmt);
	n = qse_vfprintf (QSE_STDERR, fmt, ap);
	va_end (ap);
	return n;
}

QSE_FILE* qse_fopen (const qse_char_t* path, const qse_char_t* mode)
{
#if defined(QSE_CHAR_IS_MCHAR)
	return fopen (path, mode);
#elif defined(_WIN32) || defined(__OS2__)
	return _wfopen (path, mode);
#else

	QSE_FILE* fp = QSE_NULL;
	qse_mchar_t* path_mb;
	qse_mchar_t* mode_mb;

	path_mb = qse_wcstombsdup (path, QSE_MMGR_GETDFL());
	mode_mb = qse_wcstombsdup (mode, QSE_MMGR_GETDFL());

	if (path_mb && mode_mb)
	{
		fp = fopen (path_mb, mode_mb);
	}

	if (mode_mb) QSE_MMGR_FREE (QSE_MMGR_GETDFL(), mode_mb);
	if (path_mb) QSE_MMGR_FREE (QSE_MMGR_GETDFL(), path_mb);

	return fp;

#endif
}

QSE_FILE* qse_popen (const qse_char_t* cmd, const qse_char_t* mode)
{
#if defined(QSE_CHAR_IS_MCHAR)

	#if defined(__OS2__)
	return _popen (cmd, mode);
	#elif defined(__DOS__)
	return QSE_NULL;
	#else
	return popen (cmd, mode);
	#endif

#elif defined(_WIN32) || defined(__OS2__) 
	return _wpopen (cmd, mode);

#elif defined(__DOS__)
	return QSE_NULL;

#else

	QSE_FILE* fp = QSE_NULL;
	qse_mchar_t* cmd_mb;

	cmd_mb = qse_wcstombsdup (cmd, QSE_MMGR_GETDFL());
	if (cmd_mb)
	{
		char mode_mb[3];
		int mode_mb_len;
		int mode_flag = 0;

		fp = popen (cmd_mb, mode_mb);

		while (*mode)
		{
			if (*mode == QSE_T('r')) mode_flag |= 1;
			else if (*mode == QSE_T('w')) mode_flag |= 2;
			mode++;
		}

		mode_mb_len = 0;
		if (mode_flag & 1) mode_mb[mode_mb_len++] = QSE_MT('r');
		if (mode_flag & 2) mode_mb[mode_mb_len++] = QSE_MT('w');
		mode_mb[mode_mb_len++] = QSE_MT('\0');
	
		QSE_MMGR_FREE (QSE_MMGR_GETDFL(), cmd_mb);
	}

	return fp;

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
		b = (qse_char_t*) QSE_MMGR_ALLOC (
			QSE_MMGR_GETDFL(), QSE_SIZEOF(qse_char_t)*(capa+1));
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

			nb = QSE_MMGR_REALLOC (
				QSE_MMGR_GETDFL(), b, ncapa * QSE_SIZEOF(qse_char_t));
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
