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

#ifndef _QSE_LIB_CMN_MEM_H_
#define _QSE_LIB_CMN_MEM_H_

#include <qse/cmn/mem.h>

#define MINIMIZE_PLATFORM_DEPENDENCY

#if defined(MINIMIZE_PLATFORM_DEPENDENCY)
#	define QSE_MEMCPY(dst,src,len)  qse_memcpy(dst,src,len)
#	define QSE_MEMMOVE(dst,src,len) qse_memmove(dst,src,len)
#	define QSE_MEMCMP(p1,p2,len)    qse_memcmp(p1,p2,len)
#	define QSE_MEMSET(dst,val,len)  qse_memset(dst,val,len)
#	define QSE_MEMBYTE(s,val,len)   qse_membyte(s,val,len)
#	define QSE_MEMRBYTE(s,val,len)  qse_memrbyte(s,val,len)
#	define QSE_MEMMEM(hs,hl,nd,nl)  qse_memmem(hs,hl,nd,nl)
#	define QSE_MEMRMEM(hs,hl,nd,nl) qse_memrmem(hs,hl,nd,nl)
#else
#	include <string.h>
#	define QSE_MEMCPY(dst,src,len)  memcpy(dst,src,len)
#	define QSE_MEMMOVE(dst,src,len) memmove(dst,src,len)
#	define QSE_MEMCMP(p1,p2,len)    memcmp(p1,p2,len)
#	define QSE_MEMSET(dst,val,len)  memset(dst,val,len)
#	define QSE_MEMBYTE(s,val,len)   memchr(s,val,len)
#	define QSE_MEMRBYTE(s,val,len)  memrchr(s,val,len)
#	define QSE_MEMMEM(hs,hl,nd,nl)  memmem(hs,hl,nd,nl)
#	define QSE_MEMRMEM(hs,hl,nd,nl) qse_memrmem(hs,hl,nd,nl)
#endif

#define QSE_MALLOC(mmgr,size) QSE_MMGR_ALLOC(mmgr,size)
#define QSE_REALLOC(mmgr,ptr,size) QSE_MMGR_REALLOC(mmgr,ptr,size)
#define QSE_FREE(mmgr,ptr) QSE_MMGR_FREE(mmgr,ptr)

#endif
