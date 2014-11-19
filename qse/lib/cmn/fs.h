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

#if defined(_WIN32)
#	include <windows.h>
	typedef DWORD qse_fs_syserr_t;
#elif defined(__OS2__)
#	define INCL_DOSERRORS
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

#if defined(__cplusplus)
extern "C" {
#endif

qse_fs_errnum_t qse_fs_syserrtoerrnum (
	qse_fs_t*       fs,
	qse_fs_syserr_t e
);

#if defined(__cplusplus)
}
#endif
