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

#ifndef _QSE_CMN_STDIO_H_
#define _QSE_CMN_STDIO_H_

/** @file
 * This file defines stdio wrapper functions for #qse_char_t.
 */

#include <qse/types.h>
#include <qse/macros.h>

#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>

#if defined(_WIN32) && !defined(__WATCOMC__)
	#include <tchar.h>
	#define qse_fgets(x,y,s) _fgetts(x,y,s)
	#define qse_fgetc(x) _fgettc(x)
	#define qse_fputs(x,s) _fputts(x,s)
	#define qse_fputc(x,s) _fputtc(x,s)
#elif defined(QSE_CHAR_IS_MCHAR)
	#define qse_fgets(x,y,s) fgets(x,y,s)
	#define qse_fgetc(x) fgetc(x)
	#define qse_fputs(x,s) fputs(x,s)
	#define qse_fputc(x,s) fputc(x,s)
#else
	#define qse_fgets(x,y,s) fgetws(x,y,s)
	#define qse_fgetc(x) fgetwc(x)
	#define qse_fputs(x,s) fputws(x,s)
	#define qse_fputc(x,s) fputwc(x,s)
#endif

#define QSE_FILE        FILE
#define QSE_STDIN       stdin
#define QSE_STDOUT      stdout
#define QSE_STDERR      stderr

typedef int (*qse_getdelim_t) (const qse_char_t* ptr,qse_size_t len,void* arg);

#ifdef __cplusplus
extern "C" {
#endif

QSE_EXPORT int qse_vsprintf (
	qse_char_t*       buf,
	qse_size_t        size,
	const qse_char_t* fmt,
	va_list           ap
);

QSE_EXPORT int qse_sprintf (
	qse_char_t* buf,
	qse_size_t size,
	const qse_char_t* fmt,
	...
);

QSE_EXPORT int qse_vfprintf (
	QSE_FILE *stream, const qse_char_t* fmt, va_list ap);
QSE_EXPORT int qse_vprintf (
	const qse_char_t* fmt, va_list ap);
QSE_EXPORT int qse_fprintf (
	QSE_FILE* file, const qse_char_t* fmt, ...);
QSE_EXPORT int qse_printf (
	const qse_char_t* fmt, ...);

QSE_EXPORT int qse_dprintf (
	const qse_char_t* fmt, ...);
QSE_EXPORT QSE_FILE* qse_fopen (
	const qse_char_t* path, const qse_char_t* mode);

QSE_EXPORT void qse_fclose (QSE_FILE* fp);
QSE_EXPORT int qse_fflush (QSE_FILE* fp);
QSE_EXPORT void qse_clearerr (QSE_FILE* fp);
QSE_EXPORT int qse_feof (QSE_FILE* fp);
QSE_EXPORT int qse_ferror (QSE_FILE* fp);

/**
 * The qse_getline() function read a line from a file pointer @a fp
 * until a new line character is met.
 *
 * @return -2 on error, -1 on eof, length of data read on success 
 */
QSE_EXPORT qse_ssize_t qse_getline (qse_char_t **buf, qse_size_t *n, QSE_FILE *fp);

/**
 * The qse_getdelim() function reads characters from a file pointer @a fp 
 * until a certain condition is met as defined by @a fn and @a fnarg. 
 * 
 * @return -3 on line breaker error, -2 on error, -1 on eof, 
 *         length of data read on success 
 */
QSE_EXPORT qse_ssize_t qse_getdelim (
	qse_char_t **buf, qse_size_t *n, 
	qse_getdelim_t fn, void* fnarg, QSE_FILE *fp);

#ifdef __cplusplus
}
#endif

#endif
