/*
 * $Id: mem.h 97 2009-03-10 10:39:18Z hyunghwan.chung $
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#ifndef _QSE_LIB_CMN_MEM_H_
#define _QSE_LIB_CMN_MEM_H_

#include <qse/cmn/mem.h>

#ifdef USE_STDC

#include <string.h>
#define QSE_MEMCPY(dst,src,len) memcpy(dst,src,len)
#define QSE_MEMCMP(p1,p2,len) memcmp(p1,p2,len)
#define QSE_MEMSET(dst,val,len) memset(dst,val,len)
#define QSE_MEMBYTE(s,val,len) memchr(s,val,len)
#define QSE_MEMRBYTE(s,val,len) memrchr(s,val,len)
#define QSE_MEMMEM(hs,hl,nd,nl) memmem(hs,hl,nd,nl)
#define QSE_MEMRMEM(hs,hl,nd,nl) memrmem(hs,hl,nd,nl)

#else

#define QSE_MEMCPY(dst,src,len) qse_memcpy(dst,src,len)
#define QSE_MEMCMP(p1,p2,len) qse_memcmp(p1,p2,len)
#define QSE_MEMSET(dst,val,len) qse_memset(dst,val,len)
#define QSE_MEMBYTE(s,val,len) qse_membyte(s,val,len)
#define QSE_MEMRBYTE(s,val,len) qse_memrbyte(s,val,len)
#define QSE_MEMMEM(hs,hl,nd,nl) qse_memmem(hs,hl,nd,nl)
#define QSE_MEMRMEM(hs,hl,nd,nl) qse_memrmem(hs,hl,nd,nl)

#endif

#define QSE_MALLOC(mmgr,size) QSE_MMGR_ALLOC(mmgr,size)
#define QSE_REALLOC(mmgr,ptr,size) QSE_MMGR_REALLOC(mmgr,ptr,size)
#define QSE_FREE(mmgr,ptr) QSE_MMGR_FREE(mmgr,ptr)

#endif
