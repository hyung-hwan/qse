/*
 * $Id: stdio.h,v 1.1 2007-02-20 14:04:22 bacon Exp $
 */

#ifndef _ASE_STDIO_H_
#define _ASE_STDIO_H_

#include <ase/types.h>
#include <ase/macros.h>

#include <stdio.h>
#include <stdarg.h>

#if defined(_WIN32)
	#include <tchar.h>

	#define ase_printf   _tprintf
	#define ase_vprintf  _vtprintf
	#define ase_fprintf  _ftprintf
	#define ase_vfprintf _vftprintf

	#define awk_fgets _fgetts
	#define awk_fgetc _fgettc
#elif defined(ASE_CHAR_IS_MCHAR)
	#define awk_fgets fgets
	#define awk_fgetc fgetc
#else
	#define awk_fgets fgetws
	#define awk_fgetc fgetwc
#endif

#ifdef __cplusplus
extern "C" {
#endif

int ase_vsprintf (ase_char_t* buf, size_t size, const ase_char_t* fmt, va_list ap);
int ase_sprintf (ase_char_t* buf, size_t size, const ase_char_t* fmt, ...);

#if !defined(WIN32)
int ase_vfprintf (FILE *stream, const ase_char_t* fmt, va_list ap);
int ase_vprintf (const ase_char_t* fmt, va_list ap);
int ase_fprintf (FILE* file, const ase_char_t* fmt, ...);
int ase_printf (const ase_char_t* fmt, ...);
#endif

FILE* ase_fopen (const ase_char_t* path, const ase_char_t* mode);
FILE* ase_popen (const ase_char_t* cmd, const ase_char_t* mode);

#ifdef __cplusplus
}
#endif

#endif
