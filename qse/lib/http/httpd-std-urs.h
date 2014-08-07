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
	qse_uint16_t seq; /* in network-byte order */
	qse_uint16_t rcode; /* response code */
	qse_uint32_t qusum;/* checksum of url in the request */
	qse_uint16_t len; /* url length in network-byte order */
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

	qse_uint16_t seq; /* TODO: change to uint32_t??? */
	urs_req_t* reqs[1024]; /* TOOD: choose the right size */
	qse_uint16_t req_count;

	qse_uint8_t rcvbuf[URS_MAX_URL_LEN + QSE_SIZEOF(urs_pkt_t)];
	qse_uint8_t fmtbuf[URS_MAX_URL_LEN + QSE_SIZEOF(urs_pkt_t)];
};

struct urs_req_t
{
	qse_uint16_t seq; /* in host-byte order */
	qse_uint32_t pktlen;
	urs_pkt_t* pkt;

	qse_httpd_rewrite_t rewrite;
	void* ctx;

	urs_ctx_t* dc;
	qse_tmr_index_t tmr_tmout;
	int resends;

	urs_req_t* prev;
	urs_req_t* next;
};


static int urs_open (qse_httpd_t* httpd, qse_httpd_urs_t* urs)
{
#if defined(__DOS__)
	qse_httpd_seterrnum (httpd, QSE_HTTPD_ENOIMPL);
	return -1;
#else
	sock_t fd = SOCK_INIT; 
	int flag;
	qse_nwad_t nwad;
	urs_ctx_t* dc;
	httpd_xtn_t* httpd_xtn;

	httpd_xtn = qse_httpd_getxtn (httpd);

	dc = (urs_ctx_t*) qse_httpd_callocmem (httpd, QSE_SIZEOF(urs_ctx_t));
	if (dc == NULL) goto oops;

	dc->httpd = httpd;
	dc->urs = urs;

	nwad = httpd_xtn->urs.nwad;
	if (nwad.type == QSE_NWAD_NX)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_EINVAL);
		goto oops;
	}

	if (qse_getnwadport(&nwad) == 0) 
		qse_setnwadport (&nwad, qse_hton16(QSE_HTTPD_URSSTD_DEFAULT_PORT));

	dc->skadlen = qse_nwadtoskad (&nwad, &dc->skad);
	if (dc->skadlen <= -1)
	{
		qse_httpd_seterrnum (httpd, QSE_HTTPD_EINVAL);
		goto oops;
	}

	if (httpd->opt.trait & QSE_HTTPD_LOGACT)
	{
		qse_httpd_act_t msg;
		qse_size_t pos;
		msg.code = QSE_HTTPD_CATCH_MDBGMSG;
		pos = qse_mbsxcpy (msg.u.mdbgmsg, QSE_COUNTOF(msg.u.mdbgmsg), "ursserver set to ");
		qse_nwadtombs (&nwad, &msg.u.mdbgmsg[pos], QSE_COUNTOF(msg.u.mdbgmsg) - pos, QSE_NWADTOMBS_ALL);
		httpd->opt.rcb.logact (httpd, &msg);
	}

	fd = socket (qse_skadfamily(&dc->skad), SOCK_DGRAM, IPPROTO_UDP);
	if (!is_valid_socket(fd)) 
	{
		qse_httpd_seterrnum (httpd, SKERR_TO_ERRNUM());
		goto oops;
	}

/* TODO: set socket send/recv buffer size. it's needed as urs may be long */

	#if defined(FD_CLOEXEC)
	flag = fcntl (fd, F_GETFD);
	if (flag >= 0) fcntl (fd, F_SETFD, flag | FD_CLOEXEC);
	#endif

	#if defined(SO_REUSEADDR)
	flag = 1;
	setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, (void*)&flag, QSE_SIZEOF(flag));
	#endif
	
	#if defined(SO_REUSEPORT)
	flag = 1;
	setsockopt (fd, SOL_SOCKET, SO_REUSEPORT, (void*)&flag, QSE_SIZEOF(flag));
	#endif

	if (set_socket_nonblock (httpd, fd, 1) <= -1) goto oops;
	
	urs->handle.i = fd;
	urs->ctx = dc;
	return 0;

oops:
	if (is_valid_socket(fd)) close_socket (fd);
	if (dc) qse_httpd_freemem (httpd, dc);
	return -1;

#endif
}

static void urs_remove_tmr_tmout (urs_req_t* req)
{
	if (req->tmr_tmout != QSE_TMR_INVALID_INDEX)
	{
		qse_httpd_removetimerevent (req->dc->httpd, req->tmr_tmout);
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
			urs_remove_tmr_tmout (dc->reqs[i]);
			qse_httpd_freemem (httpd, dc->reqs[i]);
			dc->reqs[i] = next_req;
			dc->req_count--;
		}
	}

	QSE_ASSERT (dc->req_count == 0);

	close_socket (urs->handle.i);
	qse_httpd_freemem (httpd, urs->ctx);
}


static int urs_recv (qse_httpd_t* httpd, qse_httpd_urs_t* urs)
{
	urs_ctx_t* dc = (urs_ctx_t*)urs->ctx;
	httpd_xtn_t* httpd_xtn;

	qse_skad_t fromaddr;
	socklen_t fromlen;

	qse_uint16_t xid;
	qse_ssize_t len;
	urs_pkt_t* pkt;
	urs_req_t* req;

printf ("URS_RECV....\n");

	httpd_xtn = qse_httpd_getxtn (httpd);

	fromlen = QSE_SIZEOF(fromaddr);
	len = recvfrom (urs->handle.i, dc->rcvbuf, QSE_SIZEOF(dc->rcvbuf) - 1, 0, (struct sockaddr*)&fromaddr, &fromlen);

/* TODO: check if fromaddr matches the dc->skad... */

	pkt = (urs_pkt_t*)dc->rcvbuf;
	if (len >= QSE_SIZEOF(pkt->hdr) &&  len >= QSE_SIZEOF(pkt->hdr) + qse_ntoh16(pkt->hdr.len))
	{
		xid = qse_ntoh16(pkt->hdr.seq) % QSE_COUNTOF(dc->reqs);

		for (req = dc->reqs[xid]; req; req = req->next)
		{
			if (req->pkt->hdr.seq == pkt->hdr.seq && req->pkt->hdr.qusum == pkt->hdr.qusum)
			{
				pkt->url[qse_ntoh16(pkt->hdr.len)] = QSE_MT('\0');

				urs_remove_tmr_tmout (req);
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
	if (req->resends > 0)
	{
		httpd_xtn_t* httpd_xtn;
		qse_tmr_event_t tmout_event;

		httpd_xtn = qse_httpd_getxtn (dc->httpd);

		qse_gettime (&tmout_event.when);
		qse_addtime (&tmout_event.when, &httpd_xtn->urs.tmout, &tmout_event.when);
		tmout_event.ctx = req;
		tmout_event.handler = tmr_urs_tmout_handle;
		tmout_event.updater = tmr_urs_tmout_update;

		if (sendto (dc->urs->handle.i, req->pkt, req->pktlen, 0, (struct sockaddr*)&dc->skad, dc->skadlen) != req->pktlen)
		{
			/* error. fall thru */
		}
		else
		{
			QSE_ASSERT (tmr == dc->httpd->tmr);
			if (qse_httpd_inserttimerevent (dc->httpd, &tmout_event, &req->tmr_tmout) >= 0)
			{
				req->resends--;
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

static int urs_send (qse_httpd_t* httpd, qse_httpd_urs_t* urs, const qse_mchar_t* url, qse_httpd_rewrite_t rewrite, void* ctx)
{
	urs_ctx_t* dc = (urs_ctx_t*)urs->ctx;
	httpd_xtn_t* httpd_xtn;

	qse_uint16_t xid;
	qse_uint32_t seq;
	urs_req_t* req = QSE_NULL;
	qse_size_t url_len;
	qse_tmr_event_t tmout_event;

	httpd_xtn = qse_httpd_getxtn (httpd);

printf ("URS REALLY SENING>>>>>>>>>>>>>>>>>>>>>>>\n");

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

	req = qse_httpd_callocmem (httpd, QSE_SIZEOF(*req) + url_len + QSE_SIZEOF(urs_pkt_t));
	if (req == QSE_NULL) goto oops;

	req->tmr_tmout = QSE_TMR_INVALID_INDEX;
	req->seq = seq;
	req->pkt = (urs_pkt_t*)(req + 1);

	req->pkt->hdr.seq = qse_hton16(seq);
	req->pkt->hdr.len = qse_hton16(url_len);
	req->pkt->hdr.qusum = hash_string (url);
	qse_mbscpy (req->pkt->url, url);

	/* -1 to exclude the terminating '\0' as urs_pkt_t has url[1]. */
	req->pktlen = QSE_SIZEOF(urs_pkt_t) + url_len - 1; 

	req->rewrite = rewrite;
	req->ctx = ctx;
	req->resends = httpd_xtn->urs.resends;

	qse_gettime (&tmout_event.when);
	qse_addtime (&tmout_event.when, &httpd_xtn->urs.tmout, &tmout_event.when);
	tmout_event.ctx = req;
	tmout_event.handler = tmr_urs_tmout_handle;
	tmout_event.updater = tmr_urs_tmout_update;
	if (qse_httpd_inserttimerevent (httpd, &tmout_event, &req->tmr_tmout) <= -1) goto oops;

	if (sendto (urs->handle.i, req->pkt, req->pktlen, 0, (struct sockaddr*)&dc->skad, dc->skadlen) != req->pktlen)
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
		urs_remove_tmr_tmout (req);
		qse_httpd_freemem (httpd, req);
	}
	return -1;
}

