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
	int urs_socket;

	qse_uint16_t seq; /* TODO: change to uint32_t??? */
	urs_req_t* reqs[1024]; /* TOOD: choose the right size */
	qse_uint16_t req_count;

	qse_uint8_t rcvbuf[QSE_SIZEOF(urs_hdr_t) + URS_MAX_URL_LEN + 1];
	qse_uint8_t fmtbuf[QSE_SIZEOF(urs_hdr_t) + URS_MAX_URL_LEN + 1];
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
#if defined(__DOS__)
	qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
	return -1;
#else
	qse_nwad_t nwad;
	urs_ctx_t* dc;
	httpd_xtn_t* httpd_xtn;
	int type, proto = IPPROTO_UDP; //IPPROTO_SCTP;

	httpd_xtn = qse_httpd_getxtn (httpd);

	dc = (urs_ctx_t*) qse_httpd_callocmem (httpd, QSE_SIZEOF(urs_ctx_t));
	if (dc == NULL) goto oops;

	dc->httpd = httpd;
	dc->urs = urs;

	nwad = httpd_xtn->urs.nwad;
	if (nwad.type != QSE_NWAD_NX && qse_getnwadport(&nwad) == 0) 
		qse_setnwadport (&nwad, qse_hton16(QSE_HTTPD_URSSTD_DEFAULT_PORT));

	if (httpd->opt.trait & QSE_HTTPD_LOGACT)
	{
		qse_httpd_act_t msg;
		qse_size_t pos;
		msg.code = QSE_HTTPD_CATCH_MDBGMSG;
		pos = qse_mbsxcpy (msg.u.mdbgmsg, QSE_COUNTOF(msg.u.mdbgmsg), "default ursserver set to ");
		qse_nwadtombs (&nwad, &msg.u.mdbgmsg[pos], QSE_COUNTOF(msg.u.mdbgmsg) - pos, QSE_NWADTOMBS_ALL);
		httpd->opt.rcb.logact (httpd, &msg);
	}

#if defined(IPPROTO_SCTP)
	type = (proto == IPPROTO_SCTP)? SOCK_SEQPACKET: SOCK_DGRAM;
#else
	type = SOCK_DGRAM;
#endif
	
	urs->handle[0].i = open_udp_socket (httpd, AF_INET, type, proto);
#if defined(AF_INET6)
	urs->handle[1].i = open_udp_socket (httpd, AF_INET6, type, proto);
#endif

	if (!qse_isvalidsckhnd(urs->handle[0].i) && !qse_isvalidsckhnd(urs->handle[1].i))
	{
		goto oops;
	}

	/* carry on regardless of success or failure */
	dc->skadlen = qse_nwadtoskad (&nwad, &dc->skad);

	/* determine which socket to use when sending a request to the server */
	if (dc->skadlen >= 0)
	{
		if (nwad.type == QSE_NWAD_IN4)
			dc->urs_socket = urs->handle[0].i;
		else
			dc->urs_socket = urs->handle[1].i;
	}
	else
	{
		dc->urs_socket = QSE_INVALID_SCKHND;
	}

#if 0
	if (proto == IPPROTO_SCTP)
	{
/* TODO: error ahndleing */
		if (qse_isvalidsckhnd(urs->handle[0].i)) listen (urs->handle[0].i, 99);
		if (qse_isvalidsckhnd(urs->handle[1].i)) listen (urs->handle[1].i, 99);
	}
#endif
	urs->handle_count = 2;

	urs->ctx = dc;
	return 0;

oops:
	if (qse_isvalidsckhnd(urs->handle[0].i)) qse_closesckhnd (urs->handle[0].i);
	if (qse_isvalidsckhnd(urs->handle[1].i)) qse_closesckhnd (urs->handle[1].i);
	if (dc) qse_httpd_freemem (httpd, dc);
	return -1;

#endif
}

static void urs_remove_tmr_tmout (qse_httpd_t* httpd, urs_req_t* req)
{
	if (req->tmr_tmout != QSE_TMR_INVALID_INDEX)
	{
		qse_httpd_removetimerevent (httpd, req->tmr_tmout);
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

	for (i = 0; i < urs->handle_count; i++) 
	{
		if (qse_isvalidsckhnd(urs->handle[i].i)) 
			qse_closesckhnd (urs->handle[i].i);
	}

	qse_httpd_freemem (httpd, urs->ctx);
}


static int urs_recv (qse_httpd_t* httpd, qse_httpd_urs_t* urs, qse_ubi_t handle)
{
	urs_ctx_t* dc = (urs_ctx_t*)urs->ctx;
	httpd_xtn_t* httpd_xtn;

	qse_skad_t fromaddr;
	socklen_t fromlen;

	qse_uint16_t xid;
	qse_ssize_t len, url_len;
	urs_pkt_t* pkt;
	urs_req_t* req;
	qse_mchar_t* spc;

printf ("URS_RECV............................................\n");

	httpd_xtn = qse_httpd_getxtn (httpd);

/* TODO: use recvmsg with MSG_ERRQUEUE... set socket option IP_RECVERR... */
	fromlen = QSE_SIZEOF(fromaddr);
	len = recvfrom (handle.i, dc->rcvbuf, QSE_SIZEOF(dc->rcvbuf) - 1, 0, (struct sockaddr*)&fromaddr, &fromlen);

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

static void tmr_urs_tmout_update (qse_tmr_t* tmr, qse_tmr_index_t old_index, qse_tmr_index_t new_index, void* ctx)
{
	urs_req_t* req = (urs_req_t*)ctx;

printf (">>tmr_urs_tmout_updated existing=%d old=%d new=%d\n", (int)req->tmr_tmout, (int)old_index, (int)new_index);
	QSE_ASSERT (req->tmr_tmout == old_index);
	req->tmr_tmout = new_index;
}

static void tmr_urs_tmout_handle (qse_tmr_t* tmr, const qse_ntime_t* now, void* ctx)
{
	/* destory the unanswered request if timed out */

	urs_req_t* req = (urs_req_t*)ctx;
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

		qse_gettime (&tmout_event.when);
		qse_addtime (&tmout_event.when, &req->urs_tmout, &tmout_event.when);
		tmout_event.ctx = req;
		tmout_event.handler = tmr_urs_tmout_handle;
		tmout_event.updater = tmr_urs_tmout_update;

		if (sendto (req->urs_socket, req->pkt, req->pktlen, 0, (struct sockaddr*)&req->urs_skad, req->urs_skadlen) != req->pktlen)
		{
			/* error. fall thru */
		}
		else
		{
			QSE_ASSERT (tmr == dc->httpd->tmr);
			if (qse_httpd_inserttimerevent (dc->httpd, &tmout_event, &req->tmr_tmout) >= 0)
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

static int urs_send (qse_httpd_t* httpd, qse_httpd_urs_t* urs, const qse_mchar_t* url, qse_httpd_rewrite_t rewrite, const qse_httpd_natr_t* urs_server, void* ctx)
{
	urs_ctx_t* dc = (urs_ctx_t*)urs->ctx;
	httpd_xtn_t* httpd_xtn;

	qse_uint16_t xid;
	qse_uint32_t seq;
	urs_req_t* req = QSE_NULL;
	qse_size_t url_len;
	qse_tmr_event_t tmout_event;
	

printf ("... URS_SEND.....................\n");
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

		if (urs_server->nwad.type == QSE_NWAD_IN4)
			req->urs_socket = urs->handle[0].i;
		else 
			req->urs_socket = urs->handle[1].i;
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

	qse_gettime (&tmout_event.when);
	qse_addtime (&tmout_event.when, &req->urs_tmout, &tmout_event.when);
	tmout_event.ctx = req;
	tmout_event.handler = tmr_urs_tmout_handle;
	tmout_event.updater = tmr_urs_tmout_update;
	if (qse_httpd_inserttimerevent (httpd, &tmout_event, &req->tmr_tmout) <= -1) goto oops;

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

	if (sendto (req->urs_socket, req->pkt, req->pktlen, 0, (struct sockaddr*)&req->urs_skad, req->urs_skadlen) != req->pktlen)
	{
		qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
		goto oops;
	}

	req->dc = dc;

	/* link the request to the front of the chain */
	if (dc->reqs[xid]) dc->reqs[xid]->prev = req;
	req->next = dc->reqs[xid];
	dc->reqs[xid] = req;

	/* increment the number of pending requests */
	dc->req_count++;

printf ("URS REALLY SENT>>>>>>>>>>>>>>>>>>>>>>>\n");
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

	const qse_mchar_t* host_ptr = QSE_NULL;
	qse_mchar_t cliaddrbuf[128];
	qse_size_t total_len;
	qse_mchar_t* url_to_rewrite;

	qpath = qse_htre_getqpath(req);
	mtype = qse_htre_getqmethodtype(req);
	mname = qse_htre_getqmethodname(req);

	total_len = qse_mbslen(qpath) + qse_mbslen(mname);

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

	/* URL client-ip/client-fqdn ident method  */
	if (mtype != QSE_HTTP_CONNECT && host_ptr)
		qse_mbsxfmt (url_to_rewrite, total_len, QSE_MT("http://%s%s %s/- - %s"), host_ptr, qpath, cliaddrbuf, mname);
	else
		qse_mbsxfmt (url_to_rewrite, total_len, QSE_MT("%s %s/- - %s"), qpath, cliaddrbuf, mname);

	*url = url_to_rewrite;
	return 1;
}
