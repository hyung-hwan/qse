/*
 * $Id: stdio.h,v 1.1 2007/03/28 14:05:30 bacon Exp $
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

	#define ase_fgets _fgetts
	#define ase_fgetc _fgettc
#elif defined(ASE_CHAR_IS_MCHAR)
	#define ase_fgets fgets
	#define ase_fgetc fgetc
#else
	#define ase_fgets fgetws
	#define ase_fgetc fgetwc
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

int ase_dprintf (const ase_char_t* fmt, ...);
FILE* ase_fopen (const ase_char_t* path, const ase_char_t* mode);
FILE* ase_popen (const ase_char_t* cmd, const ase_char_t* mode);

#ifdef __cplusplus
}
#endif

#endif
