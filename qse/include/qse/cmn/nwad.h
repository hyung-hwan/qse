/*
 * $Id$
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

#ifndef _QSE_CMN_NWAD_H_
#define _QSE_CMN_NWAD_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/ipad.h>

enum qse_nwad_type_t
{
	QSE_NWAD_NX, /* non-existent */
	QSE_NWAD_IN4,
	QSE_NWAD_IN6
};
typedef enum qse_nwad_type_t qse_nwad_type_t;

typedef struct qse_nwad_t qse_nwad_t;

struct qse_nwad_t
{
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

typedef struct qse_skad_t qse_skad_t;

struct qse_skad_t
{
	/* TODO: is this large enough?? */
#if (QSE_SIZEOF_STRUCT_SOCKADDR_IN > 0) && \
    (QSE_SIZEOF_STRUCT_SOCKADDR_IN >= QSE_SIZEOF_STRUCT_SOCKADDR_IN6)
	qse_uint8_t data[QSE_SIZEOF_STRUCT_SOCKADDR_IN];
#elif (QSE_SIZEOF_STRUCT_SOCKADDR_IN6 > 0) && \
      (QSE_SIZEOF_STRUCT_SOCKADDR_IN6 >= QSE_SIZEOF_STRUCT_SOCKADDR_IN)
	qse_uint8_t data[QSE_SIZEOF_STRUCT_SOCKADDR_IN6];
#else
	/* no sockaddr_xxx is available */
	qse_uint8_t data[QSE_SIZEOF(qse_nwad_t)];
#endif
};

#ifdef __cplusplus
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

QSE_EXPORT int qse_skadtonwad (
	const qse_skad_t* skad,
	qse_nwad_t*       nwad
);

QSE_EXPORT int qse_nwadtoskad (
	const qse_nwad_t* nwad,
	qse_skad_t*       skad
);

QSE_EXPORT int qse_skadfamily (
	const qse_skad_t* skad
);

#ifdef __cplusplus
}
#endif

#endif
