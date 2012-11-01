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
    License along with QSE. If not, see <htrd://www.gnu.org/licenses/>.
 */

#ifndef _QSE_LIB_NET_UPXD_H_
#define _QSE_LIB_NET_UPXD_H_

#include <qse/net/upxd.h>
#include "../cmn/mem.h"

typedef struct qse_upxd_server_session_t qse_upxd_server_session_t;

struct qse_upxd_server_t
{
	qse_upxd_server_t* next;
	qse_upxd_server_t* prev;

#define QSE_UPXD_SERVER_ENABLED (1 << 0)
	int flags;
	
	/* the socket can be bound to this interface.
	 * sock->dev points to this buffer when necessary. */
	qse_char_t dev[QSE_UPXD_SESSION_DEV_LEN + 1];

	/* list of sessions beloning to this server */
	struct
	{
		qse_upxd_server_session_t* list;
		qse_size_t count;
	} session;

	qse_upxd_sock_t local;

	/* user-defined context data that can be set
	 * with qse_upxd_setserverctx() */
	void* ctx;
};

struct qse_upxd_server_session_t
{
	/* internal fields */
	qse_upxd_server_session_t* next;
	qse_upxd_server_session_t* prev;

	/* timestamps for housekeeping */
	qse_ntime_t created;
	qse_ntime_t modified;

	/* socket used to talk with a peer */
	qse_upxd_sock_t peer;

	/* exposed to a caller via callbacks */
	qse_upxd_session_t inner;
};

struct qse_upxd_t
{
	qse_mmgr_t* mmgr;
	qse_upxd_errnum_t errnum;

	int stopreq;
	qse_upxd_cbs_t* cbs;

	struct
	{
		qse_upxd_server_t* list;
		qse_size_t nactive;
	} server;

	void* mux;
	qse_uint8_t rbuf[65535];
};

#ifdef __cplusplus
extern "C" {
#endif

int qse_upxd_init (qse_upxd_t* upxd, qse_mmgr_t* mmgr);
void qse_upxd_fini (qse_upxd_t* upxd);

#ifdef __cplusplus
}
#endif

#endif
