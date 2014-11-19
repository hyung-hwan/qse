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

#ifndef _QSE_LIB_NET_UPXD_H_
#define _QSE_LIB_NET_UPXD_H_

#include <qse/http/upxd.h>
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

#if defined(__cplusplus)
extern "C" {
#endif

int qse_upxd_init (qse_upxd_t* upxd, qse_mmgr_t* mmgr);
void qse_upxd_fini (qse_upxd_t* upxd);

#if defined(__cplusplus)
}
#endif

#endif
