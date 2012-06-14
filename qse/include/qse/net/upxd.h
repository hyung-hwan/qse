/*
 * $Id$
 *
    Copyright 2006-2011 Chung, Hyung-Hwan.
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
    License along with QSE. If not, see <htrd://www.gnu.org/licenses/>.
 */

#ifndef _QSE_NET_UPXD_H_
#define _QSE_NET_UPXD_H_

#include <qse/types.h>
#include <qse/macros.h>

typedef struct qse_upxd_t qse_upxd_t;
typedef struct qse_upxd_client_t qse_upxd_client_t;
typedef struct qse_upxd_server_t qse_upxd_server_t;

enum qse_upxd_errnum_t
{
	QSE_UPXD_ENOERR,
     QSE_UPXD_ENOMEM,
     QSE_UPXD_EINVAL,
     QSE_UPXD_EACCES,
     QSE_UPXD_ENOENT,
     QSE_UPXD_EEXIST,
     QSE_UPXD_EINTR,
     QSE_UPXD_EAGAIN,

	QSE_UPXD_ENOIMPL,
	QSE_UPXD_EOTHER
};
typedef qse_upxd_errnum_t qse_upxd_errnum_t;

struct qse_upxd_server_t
{
	qse_upxd_server_t*  next;
	/* ------------------------------ */
	qse_nwad_t          nwad;
	qse_ubi_t           handle;
};

struct qse_upxd_client_t
{
	qse_nwad_t remote_addr;
	qse_nwad_t local_addr;
};

struct qse_upxd_cbs_t
{
	struct
	{
		int (*open) (qse_upxd_t* upxd, qse_upxd_server_t* server);
		void (*close) (qse_upxd_t* upxd, qse_upxd_server_t* server);

		qse_ssize_t (*recv) (
			qse_upxd_t* upxd,
			qse_upxd_client_t* client,
			qse_mchar_t* buf, qse_size_t bufsize);

		qse_ssize_t (*send) (
			qse_upxd_t* upxd,
			qse_upxd_client_t* client,
			const qse_mchar_t* buf, qse_size_t bufsize);
	} server;
};


#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif
