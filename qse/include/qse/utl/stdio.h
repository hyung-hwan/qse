/*
 * $Id: stdio.h 419 2008-10-13 11:32:58Z baconevi $
 * 
 * {License}
 */

#ifndef _QSE_UTL_STDIO_H_
#define _QSE_UTL_STDIO_H_

#include <qse/types.h>
#include <qse/macros.h>

#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>

#if defined(_WIN32)
	#include <tchar.h>

	#define qse_printf   _tprintf
	#define qse_vprintf  _vtprintf
	#define qse_fprintf  _ftprintf
	#define qse_vfprintf _vftprintf

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

#define qse_feof(s)     feof(s)
#define qse_ferror(s)   ferror(s)
#define qse_clearerr(s) clearerr(s)
#define qse_fflush(s)   fflush(s)
#define qse_fclose(s)   fclose(s)

#define QSE_FILE        FILE
#define QSE_STDIN       stdin
#define QSE_STDOUT      stdout
#define QSE_STDERR      stderr

typedef int (*qse_getdelim_t) (const qse_char_t* ptr,qse_size_t len, void* arg);

#ifdef __cplusplus
extern "C" {
#endif

int qse_vsprintf (qse_char_t* buf, size_t size, const qse_char_t* fmt, va_list ap);
int qse_sprintf (qse_char_t* buf, size_t size, const qse_char_t* fmt, ...);

#if !defined(_WIN32)
int qse_vfprintf (QSE_FILE *stream, const qse_char_t* fmt, va_list ap);
int qse_vprintf (const qse_char_t* fmt, va_list ap);
int qse_fprintf (QSE_FILE* file, const qse_char_t* fmt, ...);
int qse_printf (const qse_char_t* fmt, ...);
#endif

int qse_dprintf (const qse_char_t* fmt, ...);
QSE_FILE* qse_fopen (const qse_char_t* path, const qse_char_t* mode);
QSE_FILE* qse_popen (const qse_char_t* cmd, const qse_char_t* mode);

/**
 * returns -2 on error, -1 on eof, length of data read on success 
 */
qse_ssize_t qse_getline (qse_char_t **buf, qse_size_t *n, QSE_FILE *fp);
/**
 * returns -3 on line breaker error, -2 on error, -1 on eof, 
 * length of data read on success 
 */
qse_ssize_t qse_getdelim (
	qse_char_t **buf, qse_size_t *n, 
	qse_getdelim_t fn, void* fnarg, QSE_FILE *fp);

#ifdef __cplusplus
}
#endif

#endif
