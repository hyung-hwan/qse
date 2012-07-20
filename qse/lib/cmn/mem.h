/*
 * $Id: mem.h 556 2011-08-31 15:43:46Z hyunghwan.chung $
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _QSE_LIB_CMN_MEM_H_
#define _QSE_LIB_CMN_MEM_H_

#include <qse/cmn/mem.h>

#ifdef MINIMIZE_PLATFORM_DEPENDENCY
#	define QSE_MEMCPY(dst,src,len)  qse_memcpy(dst,src,len)
#	define QSE_MEMCMP(p1,p2,len)    qse_memcmp(p1,p2,len)
#	define QSE_MEMSET(dst,val,len)  qse_memset(dst,val,len)
#	define QSE_MEMBYTE(s,val,len)   qse_membyte(s,val,len)
#	define QSE_MEMRBYTE(s,val,len)  qse_memrbyte(s,val,len)
#	define QSE_MEMMEM(hs,hl,nd,nl)  qse_memmem(hs,hl,nd,nl)
#	define QSE_MEMRMEM(hs,hl,nd,nl) qse_memrmem(hs,hl,nd,nl)
#else
#	include <string.h>
#	define QSE_MEMCPY(dst,src,len)  memcpy(dst,src,len)
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
