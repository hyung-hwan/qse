/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
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

/* 
 * This file holds url rewriting support code and is included by httpd-std.c
 */

#define URS_SEQ_RANGE_SIZE (QSE_TYPE_MAX(qse_uint16_t) - 2)
#define URS_MAX_URL_LEN 50000

typedef struct urs_hdr_t urs_hdr_t;
typedef struct urs_pkt_t urs_pkt_t;
typedef struct urs_ctx_t urs_ctx_t;
typedef struct urs_req_t urs_req_t;

#include <qse/pack1.h>
struct urs_hdr_t
{
	qse_uint16_t seq;    /* in network-byte order */
	qse_uint16_t rcode;  /* response code */
	qse_uint32_t urlsum; /* checksum of url in the request */
	qse_uint16_t pktlen; /* packet header size + url length */
};

struct urs_pkt_t
{
	struct urs_hdr_t hdr;
	qse_mchar_t url[1];
};
#include <qse/unpack.h>

struct urs_ctx_t
{
	qse_httpd_t* httpd;
	qse_httpd_urs_t* urs;

	qse_skad_t skad;
	int skadlen;
	int urs_socket; /* default urs socket to use */

	qse_uint16_t seq; /* TODO: change to uint32_t??? */
	urs_req_t* reqs[1024]; /* TOOD: choose the right size */
	qse_uint16_t req_count;

	qse_uint8_t rcvbuf[QSE_SIZEOF(urs_hdr_t) + URS_MAX_URL_LEN + 1];
	qse_uint8_t fmtbuf[QSE_SIZEOF(urs_hdr_t) + URS_MAX_URL_LEN + 1];

#if defined(AF_UNIX)
	struct sockaddr_un unix_bind_addr;
#endif
};

struct urs_req_t
{
	qse_uint16_t seq; /* in host-byte order */
	qse_uint32_t pktlen;
	urs_pkt_t* pkt;

	qse_httpd_rewrite_t rewrite;
	void* ctx;

	urs_ctx_t* dc;
	qse_skad_t urs_skad;
	int urs_skadlen;
	int urs_socket;
	int urs_retries;
	qse_ntime_t urs_tmout;

	qse_tmr_index_t tmr_tmout;

	urs_req_t* prev;
	urs_req_t* next;
};


static int urs_open (qse_httpd_t* httpd, qse_httpd_urs_t* urs)
{
	qse_nwad_t nwad;
	urs_ctx_t* dc;
	httpd_xtn_t* httpd_xtn;
	int type, proto = 0; /*IPPROTO_UDP;*/ /*IPPROTO_SCTP*/

	httpd_xtn = qse_httpd_getxtn (httpd);

	urs->handle[0] = QSE_INVALID_SCKHND;
	urs->handle[1] = QSE_INVALID_SCKHND;
	urs->handle[2] = QSE_INVALID_SCKHND;

	dc = (urs_ctx_t*) qse_httpd_callocmem (httpd, QSE_SIZEOF(urs_ctx_t));
	if (dc == NULL) goto oops;

	dc->httpd = httpd;
	dc->urs = urs;

	nwad = httpd_xtn->urs.nwad;
	if (nwad.type != QSE_NWAD_NX && qse_getnwadport(&nwad) == 0) 
		qse_setnwadport (&nwad, qse_hton16(QSE_HTTPD_URSSTD_DEFAULT_PORT));

#if defined(QSE_HTTPD_DEBUG)
	{
		qse_mchar_t tmp[128];
		qse_nwadtombs (&nwad, tmp, QSE_COUNTOF(tmp), QSE_NWADTOMBS_ALL);
		HTTPD_DBGOUT1 ("Default URS server set to [%hs]\n", tmp);
	}
#endif

#if defined(IPPROTO_SCTP)
	type = (proto == IPPROTO_SCTP)? SOCK_SEQPACKET: SOCK_DGRAM;
#else
	type = SOCK_DGRAM;
#endif

	urs->handle[0] = open_client_socket (httpd, AF_INET, type, proto);
#if defined(AF_INET6)
	urs->handle[1] = open_client_socket (httpd, AF_INET6, type, proto);
#endif
#if defined(AF_UNIX)
	urs->handle[2] = open_client_socket (httpd, AF_UNIX, type, 0);
#endif

	if (qse_isvalidsckhnd(urs->handle[2]))
	{
	#if defined(AF_UNIX)
		qse_ntime_t now;

		qse_gettime (&now);

		QSE_MEMSET (&dc->unix_bind_addr, 0, QSE_SIZEOF(dc->unix_bind_addr));
		dc->unix_bind_addr.sun_family = AF_UNIX;

		/* TODO: make the location(/tmp) or the prefix(.urs-) of the socket file configurable??? */
		qse_mbsxfmt (
			dc->unix_bind_addr.sun_path, 
			QSE_COUNTOF(dc->unix_bind_addr.sun_path),
			QSE_MT("/tmp/.urs-%x-%lu"), (int)QSE_GETPID(), (unsigned long int)urs->handle[2]);
		QSE_UNLINK (dc->unix_bind_addr.sun_path);
		if (bind (urs->handle[2], (struct sockaddr*)&dc->unix_bind_addr, QSE_SIZEOF(dc->unix_bind_addr)) <= -1)
		{
			qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
			qse_closesckhnd (urs->handle[2]);
			urs->handle[2] = QSE_INVALID_SCKHND;
		}
	#endif
	}

	if (!qse_isvalidsckhnd(urs->handle[0]) && 
	    !qse_isvalidsckhnd(urs->handle[1]) &&
	    !qse_isvalidsckhnd(urs->handle[2]))
	{
		/* don't set the error number here.
		 * open_client_socket() or bind() above should set the error number */
		goto oops;
	}

	/* carry on regardless of success or failure */
	dc->skadlen = qse_nwadtoskad (&nwad, &dc->skad);

	/* determine which socket to use when sending a request to the default server */
	if (dc->skadlen >= 0)
	{
		switch (nwad.type)
		{
			case QSE_NWAD_IN4:
				dc->urs_socket = urs->handle[0];
				break;
			case QSE_NWAD_IN6:
				dc->urs_socket = urs->handle[1];
				break;
			case QSE_NWAD_LOCAL:
				dc->urs_socket = urs->handle[2];
				break;
			default:
				/* unsupported address for the default server */
				dc->urs_socket = QSE_INVALID_SCKHND;
				break;
		}
	}
	else
	{
		dc->urs_socket = QSE_INVALID_SCKHND;
	}

#if 0
	if (proto == IPPROTO_SCTP)
	{
/* TODO: error handling */
		if (qse_isvalidsckhnd(urs->handle[0])) listen (urs->handle[0], 99);
		if (qse_isvalidsckhnd(urs->handle[1])) listen (urs->handle[1], 99);
		/* handle[2] is a unix socket. no special handling for SCTP */
	}
#endif

	urs->handle_count = 3;
	if (qse_isvalidsckhnd(urs->handle[0])) urs->handle_mask |= (1 << 0);
	if (qse_isvalidsckhnd(urs->handle[1])) urs->handle_mask |= (1 << 1);
	if (qse_isvalidsckhnd(urs->handle[2])) urs->handle_mask |= (1 << 2);

	urs->ctx = dc;
	return 0;

oops:
	if (qse_isvalidsckhnd(urs->handle[0])) qse_closesckhnd (urs->handle[0]);
	if (qse_isvalidsckhnd(urs->handle[1])) qse_closesckhnd (urs->handle[1]);
	if (qse_isvalidsckhnd(urs->handle[2])) 
	{
		qse_closesckhnd (urs->handle[2]);
	#if defined(AF_UNIX)
		QSE_UNLINK (dc->unix_bind_addr.sun_path);
	#endif
	}
	if (dc) qse_httpd_freemem (httpd, dc);
	return -1;
}

static void urs_remove_tmr_tmout (qse_httpd_t* httpd, urs_req_t* req)
{
	if (req->tmr_tmout != QSE_TMR_INVALID_INDEX)
	{
		qse_httpd_remove_timer_event (httpd, req->tmr_tmout);
		req->tmr_tmout = QSE_TMR_INVALID_INDEX;
	}
}

static void urs_close (qse_httpd_t* httpd, qse_httpd_urs_t* urs)
{
	urs_ctx_t* dc = (urs_ctx_t*)urs->ctx;
	qse_size_t i;

	for (i = 0; i < QSE_COUNTOF(dc->reqs); i++)
	{
		urs_req_t* next_req;
		while (dc->reqs[i])
		{
			next_req = dc->reqs[i]->next;
			urs_remove_tmr_tmout (httpd, dc->reqs[i]);
			qse_httpd_freemem (httpd, dc->reqs[i]);
			dc->reqs[i] = next_req;
			dc->req_count--;
		}
	}

	QSE_ASSERT (dc->req_count == 0);

	if (qse_isvalidsckhnd(urs->handle[0])) qse_closesckhnd (urs->handle[0]);
	if (qse_isvalidsckhnd(urs->handle[1])) qse_closesckhnd (urs->handle[1]);
	if (qse_isvalidsckhnd(urs->handle[2])) 
	{
		qse_closesckhnd (urs->handle[2]);
	#if defined(AF_UNIX)
		QSE_UNLINK (dc->unix_bind_addr.sun_path);
	#endif
	}
	qse_httpd_freemem (httpd, urs->ctx);
}


static int urs_recv (qse_httpd_t* httpd, qse_httpd_urs_t* urs, qse_httpd_hnd_t handle)
{
	urs_ctx_t* dc = (urs_ctx_t*)urs->ctx;
	httpd_xtn_t* httpd_xtn;

	qse_skad_t fromaddr;
	qse_sck_len_t fromlen;

	qse_uint16_t xid;
	qse_ssize_t len, url_len;
	urs_pkt_t* pkt;
	urs_req_t* req;
	qse_mchar_t* spc;

printf ("URS_RECV............................................\n");

	httpd_xtn = qse_httpd_getxtn (httpd);

/* TODO: use recvmsg with MSG_ERRQUEUE... set socket option IP_RECVERR... */
	fromlen = QSE_SIZEOF(fromaddr);
	len = recvfrom (handle, dc->rcvbuf, QSE_SIZEOF(dc->rcvbuf) - 1, 0, (struct sockaddr*)&fromaddr, &fromlen);

/* TODO: check if fromaddr matches the dc->skad... */

	pkt = (urs_pkt_t*)dc->rcvbuf;
	if (len >= QSE_SIZEOF(urs_hdr_t))
	{
		pkt->hdr.pktlen = qse_ntoh16(pkt->hdr.pktlen);
		if (len == pkt->hdr.pktlen)
		{
			url_len = pkt->hdr.pktlen - QSE_SIZEOF(urs_hdr_t);
			xid = qse_ntoh16(pkt->hdr.seq) % QSE_COUNTOF(dc->reqs);

			for (req = dc->reqs[xid]; req; req = req->next)
			{
				if (req->pkt->hdr.seq == pkt->hdr.seq && req->pkt->hdr.urlsum == pkt->hdr.urlsum)
				{
					/* null-terminate the url for easier processing */
					pkt->url[url_len] = QSE_MT('\0');

					/* drop trailers starting from the first space onwards */
					spc = qse_mbschr (pkt->url, QSE_MT(' '));
					if (spc) *spc = QSE_MT('\0');

					urs_remove_tmr_tmout (httpd, req);
					req->rewrite (httpd, req->pkt->url, pkt->url, req->ctx);

					/* detach the request off dc->reqs */
					if (req == dc->reqs[xid]) dc->reqs[xid] = req->next;
					else req->prev->next = req->next;
					if (req->next) req->next->prev = req->prev;

					qse_httpd_freemem (httpd, req);
					dc->req_count--;

					break;
				}
			}
		}
	}

	return 0;
}

static void tmr_urs_tmout_update (qse_tmr_t* tmr, qse_tmr_index_t old_index, qse_tmr_index_t new_index, qse_tmr_event_t* evt)
{
	urs_req_t* req = (urs_req_t*)evt->ctx;

	QSE_ASSERT (req->tmr_tmout == old_index);
	req->tmr_tmout = new_index;
}

static void tmr_urs_tmout_handle (qse_tmr_t* tmr, const qse_ntime_t* now, qse_tmr_event_t* evt)
{
	/* destory the unanswered request if timed out */

	urs_req_t* req = (urs_req_t*)evt->ctx;
	urs_ctx_t* dc = req->dc;
	qse_uint16_t xid;

	/* when this handler is called, the event should be removed from the timer */
	QSE_ASSERT (req->tmr_tmout == QSE_TMR_INVALID_INDEX);

	/* ---------------------------------------------------------------
	 * resend 
	 *---------------------------------------------------------------- */
	if (req->urs_retries > 0)
	{
		httpd_xtn_t* httpd_xtn;
		qse_tmr_event_t tmout_event;

		httpd_xtn = qse_httpd_getxtn (dc->httpd);

		QSE_MEMSET (&tmout_event, 0, QSE_SIZEOF(tmout_event));
		qse_gettime (&tmout_event.when);
		qse_addtime (&tmout_event.when, &req->urs_tmout, &tmout_event.when);
		tmout_event.ctx = req;
		tmout_event.handler = tmr_urs_tmout_handle;
		tmout_event.updater = tmr_urs_tmout_update;

		if (sendto (req->urs_socket, (void*)req->pkt, req->pktlen, 0, (struct sockaddr*)&req->urs_skad, req->urs_skadlen) != req->pktlen)
		{
			/* error. fall thru */
			qse_httpd_seterrnum (dc->httpd, SKERR_TO_ERRNUM());

			/* Unix datagram socket seems to fail with EAGAIN often 
			 * even with increased SO_SNDBUF size. */
			if (dc->httpd->errnum == QSE_HTTPD_EAGAIN && req->urs_retries > 1)
			{
				/* TODO: check writability of req->urs_socket instead of just retrying... */
				goto send_ok;
			}
		}
		else
		{
		send_ok:
			QSE_ASSERT (tmr == dc->httpd->tmr);
			if (qse_httpd_insert_timer_event (dc->httpd, &tmout_event, &req->tmr_tmout) >= 0)
			{
				req->urs_retries--;
				return; /* resend ok */
			}
		}
	}

printf ("urs timed out....\n");
	/* ---------------------------------------------------------------
	 * timed out + no resend 
	 *---------------------------------------------------------------- */
	xid = req->seq % QSE_COUNTOF(dc->reqs);

	/* detach the request off dc->reqs */
	if (req == dc->reqs[xid]) dc->reqs[xid] = req->next;
	else req->prev->next = req->next;
	if (req->next) req->next->prev = req->prev;

	/* urs timed out. report that name resolution failed */
	req->rewrite (dc->httpd, req->pkt->url, QSE_NULL, req->ctx);

	/* i don't cache the items that have timed out */
	qse_httpd_freemem (dc->httpd, req);
	dc->req_count--;
}

static int urs_send (qse_httpd_t* httpd, qse_httpd_urs_t* urs, const qse_mchar_t* url, qse_httpd_rewrite_t rewrite, const qse_httpd_urs_server_t* urs_server, void* ctx)
{
	urs_ctx_t* dc = (urs_ctx_t*)urs->ctx;
	httpd_xtn_t* httpd_xtn;

	qse_uint16_t xid;
	qse_uint32_t seq;
	urs_req_t* req = QSE_NULL;
	qse_size_t url_len;
	qse_tmr_event_t tmout_event;

	httpd_xtn = qse_httpd_getxtn (httpd);

	if (dc->req_count >= QSE_COUNTOF(dc->reqs))
	{
		/* too many pending requests */
		qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOBUF);
		goto oops;
	}

	url_len = qse_mbslen(url);
	if (url_len > URS_MAX_URL_LEN) /* TODO: change the limit */
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_EINVAL);
		goto oops;
	}

	seq = ((qse_uint32_t)dc->seq + 1) % URS_SEQ_RANGE_SIZE;
	dc->seq = seq;

	xid = seq % QSE_COUNTOF(dc->reqs); 

	req = qse_httpd_callocmem (httpd, QSE_SIZEOF(*req) + QSE_SIZEOF(urs_hdr_t) + url_len + 1);
	if (req == QSE_NULL) goto oops;

	req->tmr_tmout = QSE_TMR_INVALID_INDEX;
	req->seq = seq;
	req->pkt = (urs_pkt_t*)(req + 1);
	req->pktlen = QSE_SIZEOF(urs_hdr_t) + url_len;

	req->pkt->hdr.seq = qse_hton16(seq);
	req->pkt->hdr.pktlen = qse_hton16(req->pktlen);
	req->pkt->hdr.urlsum = hash_string (url);
	qse_mbscpy (req->pkt->url, url);

	req->rewrite = rewrite;
	req->ctx = ctx;

	req->urs_retries = httpd_xtn->urs.retries;
	req->urs_tmout = httpd_xtn->urs.tmout;

	if (urs_server)
	{
		if (urs_server->retries >= 0) req->urs_retries = urs_server->retries;
		if (urs_server->tmout.sec >= 0) req->urs_tmout = urs_server->tmout;

		req->urs_skadlen = qse_nwadtoskad (&urs_server->nwad, &req->urs_skad);
		if (req->urs_skadlen <= -1) goto default_urs_server;

		switch (urs_server->nwad.type)
		{
			case QSE_NWAD_IN4:
				req->urs_socket = urs->handle[0];
				break;
			case QSE_NWAD_IN6:
				req->urs_socket = urs->handle[1];
				break;
			case QSE_NWAD_LOCAL:
				req->urs_socket = urs->handle[2];
				break;
			default:
				/* TODO: should it return failure with QSE_HTTPD_EINVAL? */
				goto default_urs_server;
		}
	}
	else
	{
	default_urs_server:
		if (dc->skadlen >= 0)
		{
			/* the default url rewrite server address set in urs_open
			 * is valid. */
			req->urs_skad = dc->skad;
			req->urs_skadlen = dc->skadlen;
			req->urs_socket = dc->urs_socket;
		}
		else
		{
			qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOURS);
			goto oops;
		}
	}

	QSE_MEMSET (&tmout_event, 0, QSE_SIZEOF(tmout_event));
	qse_gettime (&tmout_event.when);
	qse_addtime (&tmout_event.when, &req->urs_tmout, &tmout_event.when);
	tmout_event.ctx = req;
	tmout_event.handler = tmr_urs_tmout_handle;
	tmout_event.updater = tmr_urs_tmout_update;
	if (qse_httpd_insert_timer_event (httpd, &tmout_event, &req->tmr_tmout) <= -1) goto oops;

/*
  {
  struct msghdr msg;
	struct iovec iov;
	QSE_MEMSET (&msg, 0, QSE_SIZEOF(msg));
	msg.msg_name = &req->urs_skad;
	msg.msg_namelen = req->urs_skadlen;
	iov.iov_base = req->pkt;
	iov.iov_len = req->pktlen;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	if (sendmsg (req->urs_socket, &msg, 0) != req->pktlen) 
}
*/

	if (sendto (req->urs_socket, (void*)req->pkt, req->pktlen, 0, (struct sockaddr*)&req->urs_skad, req->urs_skadlen) != req->pktlen)
	{
		qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
printf ("URS SENDTO FAILURE........................\n"); /* TODO: logging */

		/* Unix datagram socket seems to fail with EAGAIN often 
		 * even with increased SO_SNDBUF size. */
		if (httpd->errnum == QSE_HTTPD_EAGAIN && req->urs_retries > 0)
		{
			/* TODO: check writability of req->urs_socket instead of just retrying... */
			goto send_ok;
		}

		goto oops;
	}

send_ok:
	req->dc = dc;

	/* link the request to the front of the chain */
	if (dc->reqs[xid]) dc->reqs[xid]->prev = req;
	req->next = dc->reqs[xid];
	dc->reqs[xid] = req;

	/* increment the number of pending requests */
	dc->req_count++;
	return 0;

oops:
	if (req)
	{
		urs_remove_tmr_tmout (httpd, req);
		qse_httpd_freemem (httpd, req);
	}
	return -1;
}

static int urs_prerewrite (qse_httpd_t* httpd, qse_httpd_client_t* client, qse_htre_t* req, const qse_mchar_t* host, qse_mchar_t** url)
{
	const qse_mchar_t* qpath;
	int mtype;
	const qse_mchar_t* mname;
	const qse_mchar_t* quest;
	const qse_mchar_t* qparam;
	const qse_mchar_t* proto;

	const qse_mchar_t* host_ptr = QSE_NULL;
	qse_mchar_t cliaddrbuf[MAX_NWAD_TEXT_SIZE];
	qse_size_t total_len;
	qse_mchar_t* url_to_rewrite;

	qpath = qse_htre_getqpath(req);
	qparam = qse_htre_getqparam(req);
	mtype = qse_htre_getqmethodtype(req);
	mname = qse_htre_getqmethodname(req);

	total_len = qse_mbslen(qpath) + qse_mbslen(mname);
	if (qparam) 
	{
		quest = QSE_MT("?");
		total_len = total_len + 1 + qse_mbslen(qparam);
	}
	else 
	{
		qparam = QSE_MT("");
		quest = QSE_MT("");
	}

	if (host)
	{
		/* use the host name set explicitly by the caller */
		host_ptr = host;
		total_len += qse_mbslen(host_ptr);
	}
	else 
	{
		/* find the host name in the http header */
		const qse_htre_hdrval_t* hosthv;
		hosthv = qse_htre_getheaderval(req, QSE_MT("Host"));
		if (hosthv)
		{
			/* the first host header only */
			host_ptr = hosthv->ptr;
			total_len += hosthv->len;
		}
	}

	total_len += qse_nwadtombs (&client->remote_addr, cliaddrbuf, QSE_COUNTOF(cliaddrbuf), QSE_NWADTOMBS_ADDR);
	total_len += 128; /* extra space */

	url_to_rewrite = qse_httpd_allocmem (httpd, total_len);
	if (url_to_rewrite == QSE_NULL) return -1;

	if (mtype == QSE_HTTP_CONNECT || !host_ptr) 
	{
		host_ptr = QSE_MT("");
		proto = QSE_MT("");
	}
	else if (client->status & QSE_HTTPD_CLIENT_SECURE)
	{
		proto = QSE_MT("https://");
	}
	else
	{
		proto = QSE_MT("http://");
	}

	/* URL client-ip/client-fqdn ident method  */
	qse_mbsxfmt (url_to_rewrite, total_len, QSE_MT("%s%s%s%s%s %s/- - %s"), proto, host_ptr, qpath, quest, qparam, cliaddrbuf, mname);

	*url = url_to_rewrite;
	return 1;
}
