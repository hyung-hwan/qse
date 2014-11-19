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

#ifndef _QSE_CMN_NWAD_H_
#define _QSE_CMN_NWAD_H_

/** \file
 * This file defines functions and data types for handling
 * a network address and a socket address.
 */

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/ipad.h>

enum qse_nwad_type_t
{
	QSE_NWAD_NX, /* non-existent */
	QSE_NWAD_IN4,
	QSE_NWAD_IN6,
	QSE_NWAD_LOCAL
};
typedef enum qse_nwad_type_t qse_nwad_type_t;

typedef struct qse_nwad_t qse_nwad_t;

#define QSE_NWAD_LOCAL_MAX_PATH 128

/** 
 * The qse_nwad_t type defines a structure to hold a network address.
 */
struct qse_nwad_t
{
	/** network address type */
	qse_nwad_type_t type;

	union
	{
		struct
		{
			qse_uint16_t port;
			qse_ip4ad_t  addr;
		} in4;

		struct
		{
			qse_uint16_t port;
			qse_ip6ad_t  addr;
			qse_uint32_t scope;
		} in6;

		struct
		{
			/* no port number. path is the address */
 
			/* note: 128 is chosen based on common path length in existing
			 *       systems. most systems have different sizes. some 
			 *       trailers may get truncated, when itconverted to skad. */
			qse_char_t path[QSE_NWAD_LOCAL_MAX_PATH]; 
		} local;
	} u;
};

enum qse_nwadtostr_flag_t
{
	QSE_NWADTOSTR_ADDR = (1 << 0),
#define QSE_NWADTOMBS_ADDR QSE_NWADTOSTR_ADDR
#define QSE_NWADTOWCS_ADDR QSE_NWADTOSTR_ADDR

	QSE_NWADTOSTR_PORT = (1 << 1),
#define QSE_NWADTOMBS_PORT QSE_NWADTOSTR_PORT
#define QSE_NWADTOWCS_PORT QSE_NWADTOSTR_PORT

	QSE_NWADTOSTR_ALL  = (QSE_NWADTOSTR_ADDR | QSE_NWADTOSTR_PORT)
#define QSE_NWADTOMBS_ALL  QSE_NWADTOSTR_ALL
#define QSE_NWADTOWCS_ALL  QSE_NWADTOSTR_ALL
};
typedef enum qse_nwadtostr_flag_t qse_nwadtostr_flag_t;

typedef struct qse_skad_t qse_skad_t;

/** 
 * The qse_skad_t type defines a structure large enough to hold a socket
 * address.
 */
struct qse_skad_t
{
#define QSE_SKAD_DATA_SIZE 0

#if (QSE_SIZEOF_STRUCT_SOCKADDR_IN > QSE_SKAD_DATA_SIZE)
#	undef QSE_SKAD_DATA_SIZE
#	define QSE_SKAD_DATA_SIZE QSE_SIZEOF_STRUCT_SOCKADDR_IN
#endif
#if (QSE_SIZEOF_STRUCT_SOCKADDR_IN6 > QSE_SKAD_DATA_SIZE)
#	undef QSE_SKAD_DATA_SIZE
#	define QSE_SKAD_DATA_SIZE QSE_SIZEOF_STRUCT_SOCKADDR_IN6
#endif
#if (QSE_SIZEOF_STRUCT_SOCKADDR_UN > QSE_SKAD_DATA_SIZE)
#	undef QSE_SKAD_DATA_SIZE
#	define QSE_SKAD_DATA_SIZE QSE_SIZEOF_STRUCT_SOCKADDR_UN
#endif

#if (QSE_SKAD_DATA_SIZE == 0)
#	undef QSE_SKAD_DATA_SIZE
#	define QSE_SKAD_DATA_SIZE QSE_SIZEOF(qse_nwad_t)
#endif
	/* TODO: is this large enough?? */
	qse_uint8_t data[QSE_SKAD_DATA_SIZE];

	/* dummy member to secure extra space and force structure alignment */
	qse_uintptr_t dummy; 
};

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT int qse_nwadequal (
	const qse_nwad_t* x,
	const qse_nwad_t* y
);

/**
 * The qse_clearnwad() function zeros out the address
 * for the address @a type. 
 */
QSE_EXPORT void qse_clearnwad (
	qse_nwad_t*     nwad,
	qse_nwad_type_t type
);

/**
 * The qse_setnwadport() function changes the port number field of a 
 * network saddress.
 */
QSE_EXPORT void qse_setnwadport (
	qse_nwad_t*  nwad,
	qse_uint16_t port
);

/**
 * The qse_getnwadport() function returns the port number field of a
 * network address.
 */
QSE_EXPORT qse_uint16_t qse_getnwadport (
	qse_nwad_t*   nwad
);

/**
 * The qse_mbstonwad() function converts a multi-byte string \a mbs to a 
 * network address and stores it to memory pointed to by \a nwad.
 */
QSE_EXPORT int qse_mbstonwad (
	const qse_mchar_t* mbs,
	qse_nwad_t*        nwad
);

QSE_EXPORT int qse_mbsntonwad (
	const qse_mchar_t* mbs,
	qse_size_t         len,
	qse_nwad_t*        nwad
);

QSE_EXPORT int qse_wcstonwad (
	const qse_wchar_t* wcs,
	qse_nwad_t*        nwad
);

QSE_EXPORT int qse_wcsntonwad (
	const qse_wchar_t* wcs,
	qse_size_t         len,
	qse_nwad_t*        nwad
);

QSE_EXPORT qse_size_t qse_nwadtombs (
	const qse_nwad_t* nwad,
	qse_mchar_t*      mbs,
	qse_size_t        len,
	int               flags
);

QSE_EXPORT qse_size_t qse_nwadtowcs (
	const qse_nwad_t* nwad,
	qse_wchar_t*      wcs,
	qse_size_t        len,
	int               flags
);

#if defined(QSE_CHAR_IS_MCHAR)
#	define qse_strtonwad(ptr,nwad)           qse_mbstonwad(ptr,nwad)
#	define qse_strntonwad(ptr,len,nwad)      qse_mbsntonwad(ptr,len,nwad)
#	define qse_nwadtostr(nwad,ptr,len,flags) qse_nwadtombs(nwad,ptr,len,flags)
#else
#	define qse_strtonwad(ptr,nwad)           qse_wcstonwad(ptr,nwad)
#	define qse_strntonwad(ptr,len,nwad)      qse_wcsntonwad(ptr,len,nwad)
#	define qse_nwadtostr(nwad,ptr,len,flags) qse_nwadtowcs(nwad,ptr,len,flags)
#endif

/**
 * The qse_skadtonwad() function converts a socket address \a skad
 * to a network address and stroes it to memory pointed to by \a nwad.
 * \return -1 upon failure, size of the socket address in success.
 */
QSE_EXPORT int qse_skadtonwad (
	const qse_skad_t* skad,
	qse_nwad_t*       nwad
);

/**
 * The qse_nwadtoskad() function converts a network address \a nwad
 * to a socket address and stores it to memory pointed to by \a skad.
 * \return -1 upon failure, size of the socket address in success.
 */
QSE_EXPORT int qse_nwadtoskad (
	const qse_nwad_t* nwad,
	qse_skad_t*       skad
);

/**
 * The qse_skadfamily() function returns the socket address family
 * of a given address \a skad.
 */
QSE_EXPORT int qse_skadfamily (
	const qse_skad_t* skad
);

#if defined(__cplusplus)
}
#endif

#endif
