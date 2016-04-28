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

#ifndef _QSE_CMN_NWIF_H_
#define _QSE_CMN_NWIF_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/nwad.h>

typedef struct qse_nwifcfg_t qse_nwifcfg_t;

enum qse_nwifcfg_flag_t
{
	QSE_NWIFCFG_UP       = (1 << 0),
	QSE_NWIFCFG_RUNNING  = (1 << 1),
	QSE_NWIFCFG_BCAST    = (1 << 2),
	QSE_NWIFCFG_PTOP     = (1 << 3),
	QSE_NWIFCFG_LINKUP   = (1 << 4),
	QSE_NWIFCFG_LINKDOWN = (1 << 5)
};

enum qse_nwifcfg_type_t
{
	QSE_NWIFCFG_IN4 = QSE_NWAD_IN4,
	QSE_NWIFCFG_IN6 = QSE_NWAD_IN6
};

typedef enum qse_nwifcfg_type_t qse_nwifcfg_type_t;
struct qse_nwifcfg_t
{
	qse_nwifcfg_type_t type;     /* in */
	qse_char_t         name[64]; /* in/out */
	unsigned int       index;    /* in/out */

	/* ---------------- */
	int                flags;    /* out */
	int                mtu;      /* out */

	qse_nwad_t         addr;     /* out */
	qse_nwad_t         mask;     /* out */
	qse_nwad_t         ptop;     /* out */
	qse_nwad_t         bcast;    /* out */

	/* ---------------- */

	/* TODO: add hwaddr?? */	
	/* i support ethernet only currently */
	qse_uint8_t        ethw[6];  /* out */
};

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT int qse_nwifmbstoindex (
	const qse_mchar_t* ptr,
	unsigned int*      index
);

QSE_EXPORT int qse_nwifwcstoindex (
	const qse_wchar_t* ptr,
	unsigned int*      index
);

QSE_EXPORT int qse_nwifmbsntoindex (
	const qse_mchar_t* ptr,
	qse_size_t         len,
	unsigned int*      index
);

QSE_EXPORT int qse_nwifwcsntoindex (
	const qse_wchar_t* ptr,
	qse_size_t         len,
	unsigned int*      index
);

QSE_EXPORT int qse_nwifindextombs (
	unsigned int index,
	qse_mchar_t* buf,
	qse_size_t   len
);

QSE_EXPORT int qse_nwifindextowcs (
	unsigned int index,
	qse_wchar_t* buf,
	qse_size_t   len
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_nwifstrtoindex(ptr,index)      qse_nwifmbstoindex(ptr,index)
#	define qse_nwifstrntoindex(ptr,len,index) qse_nwifmbsntoindex(ptr,len,index)
#	define qse_nwifindextostr(index,buf,len)  qse_nwifindextombs(index,buf,len)
#else
#	define qse_nwifstrtoindex(ptr,index)      qse_nwifwcstoindex(ptr,index)
#	define qse_nwifstrntoindex(ptr,len,index) qse_nwifwcsntoindex(ptr,len,index)
#	define qse_nwifindextostr(index,buf,len)  qse_nwifindextowcs(index,buf,len)
#endif

QSE_EXPORT int qse_getnwifcfg (
	qse_nwifcfg_t* cfg
);

#if defined(__cplusplus)
}
#endif

#endif
