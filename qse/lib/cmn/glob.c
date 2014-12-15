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

#include <qse/cmn/glob.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/path.h>
#include <qse/cmn/dir.h>
#include "mem.h"

#if defined(_WIN32) 
#	include <windows.h>
#elif defined(__OS2__) 
#	define INCL_DOSFILEMGR
#	define INCL_ERRORS
#	include <os2.h>
#elif defined(__DOS__) 
#	include <dos.h>
#	include <errno.h>
#elif defined(macintosh)
#	include <Files.h>
#else
#	include "syscall.h"
#endif

#define NO_RECURSION 1

enum segment_type_t
{
	NONE,
	ROOT,
	NORMAL
};
typedef enum segment_type_t segment_type_t;

#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	/* i don't support escaping in these systems */
#	define IS_ESC_MBS(c) (0)
#	define IS_ESC_WCS(c) (0)
#else
#	define IS_ESC_MBS(c) ((c) == QSE_MT('\\'))
#	define IS_ESC_WCS(c) ((c) == QSE_WT('\\'))
#endif

#define IS_NIL_MBS(c) ((c) == QSE_MT('\0'))
#define IS_NIL_WCS(c) ((c) == QSE_WT('\0'))

/* this macro only checks for top-level wild-cards among these.
 *  *, ?, [], !, -  
 * see str-fnmat.c for more wild-card letters
 */
#define IS_WILD_MBS(c) ((c) == QSE_MT('*') || (c) == QSE_MT('?') || (c) == QSE_MT('['))
#define IS_WILD_WCS(c) ((c) == QSE_WT('*') || (c) == QSE_WT('?') || (c) == QSE_WT('['))



/* -------------------------------------------------------------------- */

#define glob qse_globmbs
#define cbimpl_t qse_glob_mbscbimpl_t
#define glob_t mbs_glob_t
#define segment_t mbs_segment_t
#define stack_node_t mbs_stack_node_t
#define char_t qse_mchar_t
#define cstr_t qse_mcstr_t
#define T(x) QSE_MT(x)
#define IS_ESC(x) IS_ESC_MBS(x)
#define IS_DRIVE(x) QSE_ISPATHMBDRIVE(x)
#define IS_SEP(x) QSE_ISPATHMBSEP(x)
#define IS_SEP_OR_NIL(x) QSE_ISPATHMBSEPORNIL(x)
#define IS_NIL(x) IS_NIL_MBS(x)
#define IS_WILD(x) IS_WILD_MBS(x)
#define str_t qse_mbs_t
#define str_open qse_mbs_open
#define str_close qse_mbs_close
#define str_init qse_mbs_init
#define str_fini qse_mbs_fini
#define str_cat qse_mbs_cat
#define str_ccat qse_mbs_ccat
#define str_ncat qse_mbs_ncat
#define str_setcapa qse_mbs_setcapa
#define str_setlen qse_mbs_setlen
#define STR_CAPA(x) QSE_MBS_CAPA(x)
#define STR_LEN(x) QSE_MBS_LEN(x)
#define STR_PTR(x) QSE_MBS_PTR(x)
#define STR_XSTR(x) QSE_MBS_XSTR(x)
#define STR_CPTR(x,y) QSE_MBS_CPTR(x,y)
#define strnfnmat qse_mbsnfnmat
#define DIR_CHAR_FLAGS QSE_DIR_MBSPATH
#define path_exists mbs_path_exists
#define search      mbs_search
#define get_next_segment mbs_get_next_segment
#define handle_non_wild_segments mbs_handle_non_wild_segments
#define CHAR_IS_MCHAR 
#undef INCLUDE_MBUF
#include "glob.h"

/* -------------------------------------------------------------------- */

#undef glob
#undef cbimpl_t
#undef glob_t
#undef segment_t
#undef stack_node_t
#undef char_t
#undef cstr_t
#undef T
#undef IS_ESC
#undef IS_DRIVE
#undef IS_SEP
#undef IS_SEP_OR_NIL
#undef IS_NIL
#undef IS_WILD
#undef str_t
#undef str_open
#undef str_close
#undef str_init
#undef str_fini
#undef str_cat
#undef str_ccat
#undef str_ncat
#undef str_setcapa
#undef str_setlen
#undef STR_CAPA
#undef STR_LEN
#undef STR_PTR
#undef STR_XSTR
#undef STR_CPTR
#undef strnfnmat
#undef DIR_CHAR_FLAGS
#undef path_exists
#undef search
#undef get_next_segment
#undef handle_non_wild_segments
#undef CHAR_IS_MCHAR 
#undef INCLUDE_MBUF

/* -------------------------------------------------------------------- */

#define glob qse_globwcs
#define cbimpl_t qse_glob_wcscbimpl_t
#define glob_t wcs_glob_t
#define segment_t wcs_segment_t
#define stack_node_t wcs_stack_node_t
#define char_t qse_wchar_t
#define cstr_t qse_wcstr_t
#define T(x) QSE_WT(x)
#define IS_ESC(x) IS_ESC_WCS(x)
#define IS_DRIVE(x) QSE_ISPATHWCDRIVE(x)
#define IS_SEP(x) QSE_ISPATHWCSEP(x)
#define IS_SEP_OR_NIL(x) QSE_ISPATHWCSEPORNIL(x)
#define IS_NIL(x) IS_NIL_WCS(x)
#define IS_WILD(x) IS_WILD_WCS(x)
#define str_t qse_wcs_t
#define str_open qse_wcs_open
#define str_close qse_wcs_close
#define str_init qse_wcs_init
#define str_fini qse_wcs_fini
#define str_cat qse_wcs_cat
#define str_ccat qse_wcs_ccat
#define str_ncat qse_wcs_ncat
#define str_setcapa qse_wcs_setcapa
#define str_setlen qse_wcs_setlen
#define STR_CAPA(x) QSE_WCS_CAPA(x)
#define STR_LEN(x) QSE_WCS_LEN(x)
#define STR_PTR(x) QSE_WCS_PTR(x)
#define STR_XSTR(x) QSE_WCS_XSTR(x)
#define STR_CPTR(x,y) QSE_WCS_CPTR(x,y)

#define strnfnmat qse_wcsnfnmat
#define DIR_CHAR_FLAGS QSE_DIR_WCSPATH
#define path_exists wcs_path_exists
#define search      wcs_search
#define get_next_segment wcs_get_next_segment
#define handle_non_wild_segments wcs_handle_non_wild_segments
#undef CHAR_IS_MCHAR 
#if !defined(_WIN32) 
#	define INCLUDE_MBUF 1
#endif
#include "glob.h"
