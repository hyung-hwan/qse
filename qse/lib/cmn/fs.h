/* 
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <qse/cmn/fs.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/glob.h>
#include <qse/cmn/dir.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/cmn/path.h>

#if defined(_WIN32)
#	include <windows.h>
	typedef DWORD qse_fs_syserr_t;
#elif defined(__OS2__)
#	define INCL_DOSERRORS
#	define INCL_DOSFILEMGR
#	include <os2.h>
	typedef APIRET qse_fs_syserr_t;
#elif defined(__DOS__)
#	include <errno.h>
#	include <io.h>
#	include <stdio.h> /* for rename() */
	typedef int qse_fs_syserr_t;
#else
#	include "syscall.h"
	typedef int qse_fs_syserr_t;
#endif

#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
#	define DEFAULT_GLOB_FLAGS (QSE_GLOB_PERIOD | QSE_GLOB_LIMITED | QSE_GLOB_NOESCAPE | QSE_GLOB_IGNORECASE)
#	define DEFAULT_PATH_SEPARATOR QSE_T("\\")
#else
#	define DEFAULT_GLOB_FLAGS (QSE_GLOB_PERIOD | QSE_GLOB_LIMITED)
#	define DEFAULT_PATH_SEPARATOR QSE_T("/")
#endif

#define IS_CURDIR(x) ((x)[0] == QSE_T('.') && (x)[1] == QSE_T('\0'))
#define IS_PREVDIR(x) ((x)[0] == QSE_T('.') && (x)[1] == QSE_T('.') && (x)[2] == QSE_T('\0'))

#if defined(QSE_CHAR_IS_MCHAR)
#	define make_str_with_wcs(fs,wcs) qse_wcstombsdupwithcmgr(wcs,QSE_NULL,(fs)->mmgr,(fs)->cmgr)
#	define make_str_with_mbs(fs,mbs) (mbs)
#	define free_str_with_wcs(fs,wcs,str) QSE_MMGR_FREE((fs)->mmgr,str)
#	define free_str_with_mbs(fs,mbs,str) 
#else
#	define make_str_with_wcs(fs,wcs) (wcs)
#	define make_str_with_mbs(fs,mbs) qse_mbstowcsdupwithcmgr(mbs,QSE_NULL,(fs)->mmgr,(fs)->cmgr)
#	define free_str_with_wcs(fs,wcs,str)
#	define free_str_with_mbs(fs,mbs,str) QSE_MMGR_FREE((fs)->mmgr,str)
#endif

#if defined(QSE_FS_CHAR_IS_MCHAR)
#	define canon_fspath(path,canon,flags) qse_canonmbspath(path,canon,flags)
#	define merge_fspath_dup(dir,file,mmgr) qse_mergembspathdup(dir,file,mmgr)
#	define get_fspath_core(fspath) qse_mbspathcore(fspath)
#	define get_fspath_base(fspath) qse_mbsbasename(fspath)
#	define IS_FSPATHSEP(x) QSE_ISPATHMBSEP(x)
#	define QSE_FS_T(x) QSE_MT(x)
#else
#	define canon_fspath(fspath,canon,flags) qse_canonwcspath(fspath,canon,flags)
#	define merge_fspath_dup(dir,file,mmgr) qse_mergewcspathdup(dir,file,mmgr)
#	define get_fspath_core(fspath) qse_wcspathcore(fspath)
#	define get_fspath_base(fspath) qse_wcsbasename(fspath)
#	define IS_FSPATHSEP(x) QSE_ISPATHWCSEP(x)
#	define QSE_FS_T(x) QSE_WT(x)
#endif

#if defined(__cplusplus)
extern "C" {
#endif

qse_fs_errnum_t qse_fs_syserrtoerrnum (
	qse_fs_t*       fs,
	qse_fs_syserr_t e
);

qse_fs_errnum_t qse_fs_direrrtoerrnum (
	qse_fs_t*        fs,
	qse_dir_errnum_t e
);

qse_fs_char_t* qse_fs_makefspathformbs (
	qse_fs_t*          fs,
	const qse_mchar_t* path
);

qse_fs_char_t* qse_fs_makefspathforwcs (
	qse_fs_t*          fs,
	const qse_wchar_t* path
);

qse_fs_char_t* qse_fs_dupfspathformbs (
	qse_fs_t*          fs,
	const qse_mchar_t* path
);

qse_fs_char_t* qse_fs_dupfspathforwcs (
	qse_fs_t*          fs,
	const qse_wchar_t* path
);

void qse_fs_freefspathformbs (
	qse_fs_t*          fs, 
	const qse_mchar_t* path,
	qse_fs_char_t*     fspath
);

void qse_fs_freefspathforwcs (
	qse_fs_t*          fs, 
	const qse_wchar_t* path,
	qse_fs_char_t*     fspath
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_fs_makefspath(fs,path) qse_fs_makefspathformbs(fs,path)
#	define qse_fs_freefspath(fs,path,fspath) qse_fs_freefspathformbs(fs,path,fspath);
#else
#	define qse_fs_makefspath(fs,path) qse_fs_makefspathforwcs(fs,path)
#	define qse_fs_freefspath(fs,path,fspath) qse_fs_freefspathforwcs(fs,path,fspath);
#endif


int qse_fs_getattr (
	qse_fs_t* fs,
	const qse_fs_char_t* fspath,
	qse_fs_attr_t* attr
);

#if defined(__cplusplus)
}
#endif
