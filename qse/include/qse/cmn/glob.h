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

#ifndef _QSE_CMN_GLOB_H_
#define _QSE_CMN_GLOB_H_

#include <qse/types.h>
#include <qse/macros.h>

/** \file
 * This file provides functions, types, macros for wildcard expansion
 * in a path name.
 */

typedef int (*qse_glob_cbimpl_t) (
	const qse_cstr_t* path,
	void*             cbctx
);

enum qse_glob_flags_t
{
	/** Don't use the backslash as an escape charcter.
	 *  This option is on in Win32/OS2/DOS. */
	QSE_GLOB_NOESCAPE   = (1 << 0),

	/** Match a leading period explicitly by a literal period in the pattern */
	QSE_GLOB_PERIOD     = (1 << 1),

	/** Perform case-insensitive matching. 
	 *  This option is always on in Win32/OS2/DOS. */
	QSE_GLOB_IGNORECASE = (1 << 2),

	/** Make the function to be more fault-resistent */
	QSE_GLOB_TOLERANT   = (1 << 3),

	/** Exclude special entries from matching. 
	  * Special entries include . and .. */
	QSE_GLOB_SKIPSPCDIR  = (1 << 4)
};

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * The qse_glob() function finds path names matchin the \a pattern.
 * It calls the call-back function \a cbimpl for each path name found.
 * 
 * \return -1 on failure, 0 on no match, 1 if matches are found.
 */
QSE_EXPORT int qse_glob (
	const qse_char_t*  pattern,
	qse_glob_cbimpl_t  cbimpl,
	void*              cbctx,
	int                flags,
	qse_mmgr_t*        mmgr,
	qse_cmgr_t*        cmgr
);

#if defined(__cplusplus)
}
#endif

#endif
