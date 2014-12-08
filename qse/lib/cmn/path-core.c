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

#include <qse/cmn/path.h>


qse_mchar_t* qse_mbspathcore (const qse_mchar_t* path)
{
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	if (QSE_ISPATHMBDRIVE(path)) return (qse_mchar_t*)path + 2;
	#if defined(_WIN32)
	else if (QSE_ISPATHMBSEP(*path) && QSE_ISPATHMBSEP(*(path + 1)) && !QSE_ISPATHMBSEPORNIL(*(path + 2)))
	{
		/* UNC Path */
		path += 2;
		do { path++; } while (!QSE_ISPATHMBSEPORNIL(*path));
		if (QSE_ISPATHMBSEP(*path)) return (qse_mchar_t*)path;
	}
	#endif
/* TOOD: \\server\XXX \\.\XXX \\?\XXX \\?\UNC\server\XXX */
	
#endif
	return (qse_mchar_t*)path;
}

qse_wchar_t* qse_wcspathcore (const qse_wchar_t* path)
{
#if defined(_WIN32) || defined(__OS2__) || defined(__DOS__)
	if (QSE_ISPATHWCDRIVE(path)) return (qse_wchar_t*)path + 2;
	#if defined(_WIN32)
	else if (QSE_ISPATHWCSEP(*path) && QSE_ISPATHWCSEP(*(path + 1)) && !QSE_ISPATHWCSEPORNIL(*(path + 2)))
	{
		/* UNC Path */
		path += 2;
		do { path++; } while (!QSE_ISPATHWCSEPORNIL(*path));
		if (QSE_ISPATHWCSEP(*path)) return (qse_wchar_t*)path;
	}
	#endif
/* TOOD: \\server\XXX \\.\XXX \\?\XXX \\?\UNC\server\XXX */
#endif
	return (qse_wchar_t*)path;
}

