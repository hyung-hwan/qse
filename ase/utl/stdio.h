/*
 * $Id: stdio.h 148 2008-03-18 08:26:52Z baconevi $
 * 
 * {License}
 */

#ifndef _ASE_UTL_STDIO_H_
#define _ASE_UTL_STDIO_H_

#include <ase/cmn/types.h>
#include <ase/cmn/macros.h>

#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>

#if defined(_WIN32)
	#include <tchar.h>

	#define ase_printf   _tprintf
	#define ase_vprintf  _vtprintf
	#define ase_fprintf  _ftprintf
	#define ase_vfprintf _vftprintf

	#define ase_fgets(x,y,s) _fgetts(x,y,s)
	#define ase_fgetc(x) _fgettc(x)
	#define ase_fputs(x,s) _fputts(x,s)
	#define ase_fputc(x,s) _fputtc(x,s)
#elif defined(ASE_CHAR_IS_MCHAR)
	#define ase_fgets(x,y,s) fgets(x,y,s)
	#define ase_fgetc(x) fgetc(x)
	#define ase_fputs(x,s) fputs(x,s)
	#define ase_fputc(x,s) fputc(x,s)
#else
	#define ase_fgets(x,y,s) fgetws(x,y,s)
	#define ase_fgetc(x) fgetwc(x)
	#define ase_fputs(x,s) fputws(x,s)
	#define ase_fputc(x,s) fputwc(x,s)
#endif

#define ase_feof(s)     feof(s)
#define ase_ferror(s)   ferror(s)
#define ase_clearerr(s) clearerr(s)
#define ase_fflush(s)   fflush(s)
#define ase_fclose(s)   fclose(s)

#define ASE_FILE FILE
#define ASE_STDIN stdin
#define ASE_STDOUT stdout
#define ASE_STDERR stderr

typedef int (*ase_getdelim_t) (const ase_char_t* ptr,ase_size_t len, void* arg);

#ifdef __cplusplus
extern "C" {
#endif

int ase_vsprintf (ase_char_t* buf, size_t size, const ase_char_t* fmt, va_list ap);
int ase_sprintf (ase_char_t* buf, size_t size, const ase_char_t* fmt, ...);

#if !defined(_WIN32)
int ase_vfprintf (ASE_FILE *stream, const ase_char_t* fmt, va_list ap);
int ase_vprintf (const ase_char_t* fmt, va_list ap);
int ase_fprintf (ASE_FILE* file, const ase_char_t* fmt, ...);
int ase_printf (const ase_char_t* fmt, ...);
#endif

int ase_dprintf (const ase_char_t* fmt, ...);
ASE_FILE* ase_fopen (const ase_char_t* path, const ase_char_t* mode);
ASE_FILE* ase_popen (const ase_char_t* cmd, const ase_char_t* mode);

/**
 * returns -2 on error, -1 on eof, length of data read on success 
 */
ase_ssize_t ase_getline (ase_char_t **buf, ase_size_t *n, ASE_FILE *fp);
/**
 * returns -3 on line breaker error, -2 on error, -1 on eof, 
 * length of data read on success 
 */
ase_ssize_t ase_getdelim (
	ase_char_t **buf, ase_size_t *n, 
	ase_getdelim_t fn, void* fnarg, ASE_FILE *fp);

#ifdef __cplusplus
}
#endif

#endif
