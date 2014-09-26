
#include <qse/cmn/main.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/time.h>
#include <qse/cmn/path.h>
#include <qse/cmn/opt.h>
#include <qse/cmn/htb.h>
#include <qse/cmn/fmt.h>
#include <qse/cmn/hton.h>
#include <qse/cmn/pio.h>
#include <qse/cmn/mux.h>
#include <qse/cmn/sck.h>
#include <qse/cmn/nwad.h>
#include <qse/cmn/sio.h>

#include <signal.h>
#include <locale.h>

#if defined(_WIN32)
#	include <winsock2.h>
#	include <windows.h>
#	include <tchar.h>
#	include <process.h>
#elif defined(__OS2__)
#	define INCL_DOSPROCESS
#	define INCL_DOSEXCEPTIONS
#	define INCL_ERRORS
#	include <os2.h>
#	if defined(TCPV40HDRS)
#		define  BSD_SELECT
#	endif
#	include <types.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <sys/ioctl.h>
#	include <nerrno.h>
#	if defined(TCPV40HDRS)
#		define  USE_SELECT
#		include <sys/select.h>
#	else
#		include <unistd.h>
#	endif
#elif defined(__DOS__)
#	include <dos.h>
#else
#	include <unistd.h>
#	include <errno.h>
#	include <fcntl.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#	if defined(HAVE_NETINET_SCTP_H)
#		include <netinet/sctp.h>
#	endif
#endif

#if defined(HAVE_SYS_PRCTL_H)
#	include <sys/prctl.h>
#endif

#if defined(HAVE_SYS_RESOURCE_H)
#	include <sys/resource.h>
#endif

typedef struct urs_hdr_t urs_hdr_t;
typedef struct urs_pkt_t urs_pkt_t;

#define URS_RCODE_OK 0
#define URS_RCODE_ERROR 1

#include <qse/pack1.h>
struct urs_hdr_t
{
	qse_uint16_t seq; /* in network-byte order */
	qse_uint16_t rcode; /* response code */
	qse_uint32_t urlsum;/* checksum of url in the request */
	qse_uint16_t pktlen; /* url length in network-byte order */
};

struct urs_pkt_t
{
	struct urs_hdr_t hdr;
	qse_mchar_t url[1];
};
#include <qse/unpack.h>

#define MAX_PACKET_SIZE 65535
#define XREQ_BLOCK_SIZE 2048
#define XREQ_MAX_BLOCKS 32 

typedef struct xreq_t xreq_t;
struct xreq_t
{
	qse_ntime_t timestamp;

	qse_skad_t from;
	qse_sck_len_t fromlen;
	urs_pkt_t* pkt;

	xreq_t* next;
};

typedef struct rewriter_t rewriter_t;
struct rewriter_t
{
	int index; /* index in ursd->rewriters */
	qse_pio_t* pio;

	/* ------------------ */

	unsigned int free: 1;
	unsigned int busy: 1;
	unsigned int faulty: 1;
	unsigned int pio_in_in_mux: 1;

	struct
	{
		qse_skad_t from;
		qse_sck_len_t fromlen;
		qse_uint16_t urllen;
		qse_uint16_t urlpos;
		qse_uint8_t buf[MAX_PACKET_SIZE];
		qse_uint32_t outlen; /* length of output read from the rewriter */
	} req;

	rewriter_t* prev;
	rewriter_t* next;
};

typedef struct ursd_t ursd_t;
struct ursd_t
{
	qse_mmgr_t* mmgr;
	qse_char_t* cmdline;

	qse_size_t total_rewriter_count;
	qse_size_t free_rewriter_count;

	rewriter_t* rewriters;
	rewriter_t* free_rewriter;
	rewriter_t* busy_rewriter;

	qse_sck_hnd_t sck;
	qse_mux_t* mux;

	
	struct
	{
		qse_uint8_t buf[MAX_PACKET_SIZE]; /* temporary buffer */

		qse_size_t count; /* number of packets in the request queue */
		xreq_t* head; /* head of the request queue */
		xreq_t* tail; /* tail of the request queue */

		xreq_t* free[XREQ_MAX_BLOCKS]; /* xreq avaialble chains per block size */
	} xreq; /* request queue */

};


#define TYPE_SOCKET  0
#define TYPE_PIO_OUT 1
#define TYPE_PIO_IN  2

#define MAKE_MUX_DATA(type,index)   ((qse_uintptr_t)type | ((qse_uintptr_t)index << 4))
#define GET_TYPE_FROM_MUX_DATA(md)  ((md) & 0xF)
#define GET_INDEX_FROM_MUX_DATA(md) ((md) >> 4)

struct mux_xtn_t
{
	ursd_t* ursd;
};
typedef struct mux_xtn_t mux_xtn_t;

static qse_sck_hnd_t open_server_socket (int proto, const qse_nwad_t* bindnwad)
{
	qse_sck_hnd_t s = QSE_INVALID_SCKHND;
	qse_skad_t skad;
	qse_sck_len_t skad_len;
	int family, type, flag;

	skad_len = qse_nwadtoskad (bindnwad, &skad);
	family = qse_skadfamily(&skad);

#if defined(IPPROTO_SCTP)
	type = (proto == IPPROTO_SCTP)? SOCK_SEQPACKET: SOCK_DGRAM;
#else
	type = SOCK_DGRAM;
#endif

	if (bindnwad->type == QSE_NWAD_LOCAL) 
	{
		proto = 0;
		/* TODO: delete sun_path */
	}

	s = socket (family, type, proto);
	if (!qse_isvalidsckhnd(s))
	{
		qse_fprintf (QSE_STDERR, QSE_T("cannot create a socket\n"));
		goto oops;
	}

/* TODO: increase the socket buffer size, especially the output buffer size */

	#if defined(FD_CLOEXEC)
	flag = fcntl (s, F_GETFD);
	if (flag >= 0) fcntl (s, F_SETFD, flag | FD_CLOEXEC);
	#endif

	#if defined(SO_REUSEADDR)
	flag = 1;
	setsockopt (s, SOL_SOCKET, SO_REUSEADDR, (void*)&flag, QSE_SIZEOF(flag));
	#endif

	if (bind (s, (struct sockaddr*)&skad, skad_len) <= -1) 
	{
	#if defined(IPV6_V6ONLY) && defined(EADDRINUSE)
		if (errno == EADDRINUSE && family == AF_INET6)
		{
			int on = 1;
			setsockopt (s, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on));
			if (bind (s, (struct sockaddr*)&skad, skad_len) == 0) goto bind_ok;
		}
		
	#endif
		qse_fprintf (QSE_STDERR, QSE_T("cannot bind a socket\n"));
		goto oops;
		
	}

bind_ok:
	#if defined(IPPROTO_SCTP)
	if (proto == IPPROTO_SCTP)
	{
		#if defined(SOL_SCTP)
		struct sctp_initmsg im;
		struct sctp_paddrparams hb;

		qse_memset (&im, 0, QSE_SIZEOF(im));
		im.sinit_num_ostreams = 1;
		im.sinit_max_instreams = 1;
		im.sinit_max_attempts = 1;

		if (setsockopt (s, SOL_SCTP, SCTP_INITMSG, &im, QSE_SIZEOF(im)) <= -1) 
		{
			qse_fprintf (QSE_STDERR, QSE_T("cannot set sctp initmsg option\n"));
			goto oops;
		}

		qse_memset (&hb, 0, QSE_SIZEOF(hb));
		hb.spp_flags = SPP_HB_ENABLE;
		hb.spp_hbinterval = 5000;
		hb.spp_pathmaxrxt = 1;

		if (setsockopt (s, SOL_SCTP, SCTP_PEER_ADDR_PARAMS, &hb, QSE_SIZEOF(hb)) <= -1) goto oops;
		#endif

		if (listen (s, 99) <= -1)
		{
			qse_fprintf (QSE_STDERR, QSE_T("cannot set listen on sctp socket\n"));
			goto oops;
		}
	}
	#endif

	return s;

oops:
	if (qse_isvalidsckhnd(s)) qse_closesckhnd (s);
	return QSE_INVALID_SCKHND;
}

static int insert_to_mux (qse_mux_t* mux, qse_mux_hnd_t handle, int type, int index)
{
	qse_mux_evt_t evt;

	qse_memset (&evt, 0, QSE_SIZEOF(evt));
	evt.hnd = handle;
	evt.mask = (type == TYPE_PIO_IN? QSE_MUX_OUT: QSE_MUX_IN);
	evt.data = MAKE_MUX_DATA(type, index);
	return qse_mux_insert (mux, &evt);
}

static int delete_from_mux (qse_mux_t* mux, qse_mux_hnd_t handle, int type, int index)
{
	qse_mux_evt_t evt;

	qse_memset (&evt, 0, QSE_SIZEOF(evt));
	evt.hnd = handle;
	evt.mask = (type == TYPE_PIO_IN? QSE_MUX_OUT: QSE_MUX_IN);
	evt.data = MAKE_MUX_DATA(type, index);
	return qse_mux_delete (mux, &evt);
}

static void chain_rewriter_to_free_list (ursd_t* ursd, rewriter_t* rewriter)
{
	/* attach a rewriter to the head of the list */
	rewriter->free = 1;
	rewriter->prev = QSE_NULL;
	rewriter->next = ursd->free_rewriter;
	if (ursd->free_rewriter) ursd->free_rewriter->prev = rewriter;
	ursd->free_rewriter = rewriter;
	ursd->free_rewriter_count++;
}

static rewriter_t* dechain_rewriter_from_free_list (ursd_t* ursd, rewriter_t* rewriter)
{
	if (rewriter->next) rewriter->next->prev = rewriter->prev;

	if (rewriter == ursd->free_rewriter) ursd->free_rewriter = rewriter->next;
	else rewriter->prev->next = rewriter->next;

	rewriter->free = 0;
	ursd->free_rewriter_count--;
	return rewriter;
}

static void chain_rewriter_to_busy_list (ursd_t* ursd, rewriter_t* rewriter)
{
	rewriter->busy = 1;
	rewriter->prev = QSE_NULL;
	rewriter->next = ursd->busy_rewriter;
	if (ursd->busy_rewriter) ursd->busy_rewriter->prev = rewriter;
	ursd->busy_rewriter = rewriter;
}

static rewriter_t* dechain_rewriter_from_busy_list (ursd_t* ursd, rewriter_t* rewriter)
{
	if (rewriter->next) rewriter->next->prev = rewriter->prev;

	if (rewriter == ursd->busy_rewriter) ursd->busy_rewriter = rewriter->next;
	else rewriter->prev->next = rewriter->next;

	rewriter->busy = 0;
	return rewriter;
}

static void start_rewriter (ursd_t* ursd, rewriter_t* rewriter)
{
	QSE_ASSERT (rewriter->pio == QSE_NULL);
	
	rewriter->pio = qse_pio_open (
		ursd->mmgr, 0, ursd->cmdline, QSE_NULL, 
		QSE_PIO_WRITEIN | QSE_PIO_READOUT | QSE_PIO_ERRTONUL | 
		QSE_PIO_INNOBLOCK | QSE_PIO_OUTNOBLOCK
	);

	if (rewriter->pio)
	{
		if (insert_to_mux (ursd->mux, qse_pio_gethandle(rewriter->pio, QSE_PIO_OUT), TYPE_PIO_OUT, rewriter->index) <= -1) 
		{
			/* error logging */
			qse_pio_kill (rewriter->pio);
			qse_pio_close (rewriter->pio);
			rewriter->pio = QSE_NULL;
		}
	}
}

static void stop_rewriter (ursd_t* ursd, rewriter_t* rewriter)
{
	if (rewriter->pio)
	{
		if (rewriter->pio_in_in_mux)
		{
			delete_from_mux (ursd->mux, qse_pio_gethandle(rewriter->pio, QSE_PIO_IN), TYPE_PIO_IN, rewriter->index);
			rewriter->pio_in_in_mux = 0;
		}

		delete_from_mux (ursd->mux, qse_pio_gethandle(rewriter->pio, QSE_PIO_OUT), TYPE_PIO_OUT, rewriter->index);

		qse_pio_kill (rewriter->pio);
		qse_pio_close (rewriter->pio);
		rewriter->pio = QSE_NULL;
	}
}

static void restart_rewriter (ursd_t* ursd, rewriter_t* rewriter)
{
	stop_rewriter (ursd, rewriter);
	start_rewriter (ursd, rewriter);
}

static void reset_rewriter_data (rewriter_t* rewriter)
{
	int index = rewriter->index;
	qse_pio_t* pio = rewriter->pio;

	qse_memset (rewriter, 0, QSE_SIZEOF(*rewriter));

	rewriter->index = index;
	rewriter->pio = pio;
}

static rewriter_t* get_free_rewriter (ursd_t* ursd)
{
	rewriter_t* rewriter;

	rewriter = ursd->free_rewriter;

	if (rewriter)
	{
		QSE_ASSERT (!rewriter->busy);
		QSE_ASSERT (!rewriter->faulty);
		QSE_ASSERT (!rewriter->pio_in_in_mux);

		if (!rewriter->pio) start_rewriter (ursd, rewriter); 

		dechain_rewriter_from_free_list (ursd, rewriter);
		reset_rewriter_data (rewriter);
	}

	return rewriter;
}

static void release_rewriter (ursd_t* ursd, rewriter_t* rewriter, int send_empty_response)
{
	if (send_empty_response)
	{
		urs_pkt_t* pkt = (urs_pkt_t*)rewriter->req.buf;
		pkt->hdr.pktlen = qse_ntoh16(QSE_SIZEOF(urs_hdr_t));
		sendto (ursd->sck, pkt, QSE_SIZEOF(urs_hdr_t), 0, (struct sockaddr*)&rewriter->req.from, rewriter->req.fromlen); 
		/* TOOD: error logging. if this fails, the client side should resend a request or just time out. */
	}

	if (rewriter->pio_in_in_mux)
	{
		QSE_ASSERT (rewriter->pio);
		delete_from_mux (ursd->mux, qse_pio_gethandle(rewriter->pio, QSE_PIO_IN), TYPE_PIO_IN, rewriter->index);
		rewriter->pio_in_in_mux = 0;
	}

	if (rewriter->busy) dechain_rewriter_from_busy_list (ursd, rewriter);

	if (rewriter->faulty || !rewriter->pio) 
	{
		restart_rewriter (ursd, rewriter);

		/* NOTE: start may fail in restart_rewriter(), 
		 *       meaning rewrite->pio can still be null. */

		rewriter->faulty = 0;
	}

	chain_rewriter_to_free_list (ursd, rewriter);
}


static void seize_rewriter (ursd_t* ursd, rewriter_t* rewriter)
{
	/* call this function once the request from the socket has been
	 * fully written to the rewriter. */

	if (rewriter->pio_in_in_mux)
	{
		delete_from_mux (ursd->mux, qse_pio_gethandle(rewriter->pio, QSE_PIO_IN), TYPE_PIO_IN, rewriter->index);
		rewriter->pio_in_in_mux = 0;
	}

	QSE_ASSERT (!rewriter->faulty);
	QSE_ASSERT (!rewriter->busy);

	chain_rewriter_to_busy_list (ursd, rewriter);
}

static int feed_rewriter (ursd_t* ursd, rewriter_t* rewriter)
{
	qse_ssize_t x;
	urs_pkt_t* pkt = (urs_pkt_t*)rewriter->req.buf;

	while (rewriter->req.urlpos < rewriter->req.urllen)
	{
		x = qse_pio_write (rewriter->pio, QSE_PIO_IN, &pkt->url[rewriter->req.urlpos], rewriter->req.urllen - rewriter->req.urlpos);
		if (x <= -1)
		{
			if (qse_pio_geterrnum(rewriter->pio) == QSE_PIO_EAGAIN)
			{
				if (rewriter->pio_in_in_mux || 
				    insert_to_mux (ursd->mux, qse_pio_gethandle(rewriter->pio, QSE_PIO_IN), TYPE_PIO_IN, rewriter->index) >= 0)
				{
					/* this is partial success. the request has not 
					   been passed to the rewriter in its entirety yet. */
					rewriter->pio_in_in_mux = 1;
					return 0; 
				}
			}

			/* reclaim the rewriter since it seems faulty */
			rewriter->faulty = 1;
			release_rewriter (ursd, rewriter, 1);
			return -1;
		}

		rewriter->req.urlpos += x;
	}

	/* full feeding is completed - the entire request has 
	 * been passed to the rewriter */
	seize_rewriter (ursd, rewriter);
	return 1; /* the full url has been passed to the rewriter */
}

static int enqueue_request (ursd_t* ursd, urs_pkt_t* pkt, const qse_skad_t* from, qse_sck_len_t fromlen)
{
	qse_uint16_t blkidx;
	qse_size_t memsize;
	xreq_t* xreq;

	/* TODO: if (ursd->xreq.count > MAX_QUEUE_SIZE) return -1; */

	blkidx = (pkt->hdr.pktlen - 1) / XREQ_BLOCK_SIZE; /* 0 based */
	if (blkidx < XREQ_MAX_BLOCKS)
	{
		xreq = ursd->xreq.free[blkidx];
		if (xreq) 
		{
			ursd->xreq.free[blkidx] = xreq->next;
			goto copy_packet;
		}
		memsize = (blkidx + 1) * XREQ_BLOCK_SIZE;
	}
	else
	{
		memsize = pkt->hdr.pktlen;
	}

	xreq = QSE_MMGR_ALLOC (ursd->mmgr, QSE_SIZEOF(*xreq) + memsize);
	if (xreq == QSE_NULL) return -1;

copy_packet:
	/* TODOO: xreq->timestamp */
	xreq->next = QSE_NULL;
	xreq->from = *from;
	xreq->fromlen = fromlen;
	xreq->pkt = (urs_pkt_t*)(xreq + 1);
	qse_memcpy (xreq->pkt, pkt, pkt->hdr.pktlen);

	if (ursd->xreq.count > 0)
	{
		ursd->xreq.tail->next = xreq;
		ursd->xreq.tail = xreq;
	}
	else
	{
		ursd->xreq.head = xreq;
		ursd->xreq.tail = xreq;
	}

	ursd->xreq.count++;
	return 0;
}

static xreq_t* dequeue_request (ursd_t* ursd)
{
	xreq_t* xreq = QSE_NULL;

	if (ursd->xreq.count > 0)
	{
		xreq = ursd->xreq.head;

		ursd->xreq.head = xreq->next;
		if (ursd->xreq.count == 1)
			ursd->xreq.tail = ursd->xreq.head;

		ursd->xreq.count--;
	}

	return xreq;
}

static void release_request (ursd_t* ursd, xreq_t* xreq)
{
	qse_uint16_t blkidx;

	blkidx = (xreq->pkt->hdr.pktlen - 1) / XREQ_BLOCK_SIZE;
	if (blkidx < XREQ_MAX_BLOCKS)
	{
		xreq->next = ursd->xreq.free[blkidx];
		ursd->xreq.free[blkidx] = xreq;
	}
	else
	{
		QSE_MMGR_FREE (ursd->mmgr, xreq);
	}
}

static void handle_pending_requests (ursd_t* ursd)
{
	rewriter_t* rewriter;
	xreq_t* xreq;

	while (ursd->xreq.count > 0)
	{
		rewriter = get_free_rewriter (ursd);
		if (rewriter)
		{
	qse_printf (QSE_T("HANDLING PENDING REQUEST.... %d\n"), (int)ursd->xreq.count);
			xreq = dequeue_request (ursd);
			QSE_ASSERT (xreq);

			rewriter->req.fromlen = xreq->fromlen;
			rewriter->req.from = xreq->from;
			qse_memcpy (rewriter->req.buf, xreq->pkt, xreq->pkt->hdr.pktlen);
			rewriter->req.buf[xreq->pkt->hdr.pktlen] = QSE_MT('\n'); /* put a new line at the end */
			rewriter->req.urlpos = 0;
			rewriter->req.urllen = xreq->pkt->hdr.pktlen - QSE_SIZEOF(urs_hdr_t) + 1;  /* +1 for '\n' */

			release_request (ursd, xreq);

			if (rewriter->pio) feed_rewriter (ursd, rewriter);
			else release_rewriter (ursd, rewriter, 1);
		}
		else
		{
	qse_printf (QSE_T("NO REWRITER AVAILABLE FOR PENDING REQUEST....%d\n"), (int)ursd->xreq.count);
			break;
		}
	}
}

static void receive_request_from_socket (ursd_t* ursd, const qse_mux_evt_t* evt)
{
	qse_ssize_t x;
	urs_pkt_t* pkt;
	rewriter_t* rewriter;

	rewriter = get_free_rewriter (ursd);
	if (rewriter)
	{
		rewriter->req.fromlen = QSE_SIZEOF(rewriter->req.from);
		x = recvfrom (evt->hnd, rewriter->req.buf, QSE_SIZEOF(rewriter->req.buf) - 1, 0, (struct sockaddr*)&rewriter->req.from, &rewriter->req.fromlen);
		/*TODO: improve error handling */
		if (x < QSE_SIZEOF(urs_hdr_t)) 
		{
			/* TODO: message logging */
			return;
		}

		pkt = (urs_pkt_t*)rewriter->req.buf;
		pkt->hdr.pktlen = qse_ntoh16(pkt->hdr.pktlen); /* change the byte order */
		if (pkt->hdr.pktlen != x)
		{
			/* TOOD: message logging */
			return;
		}

		if (rewriter->pio)
		{
			rewriter->req.buf[x] = QSE_MT('\n'); /* put a new line at the end */
			rewriter->req.urlpos = 0;
			rewriter->req.urllen = pkt->hdr.pktlen - QSE_SIZEOF(urs_hdr_t) + 1; /* +1 for '\n' */

qse_printf (QSE_T("%d [[[%.*hs]]]\n"), (int)x, (int)rewriter->req.urllen, pkt->url);
			feed_rewriter (ursd, rewriter); 
		}
		else
		{
			/* the actual rewriter is defunct. */
			/* TODO: error logging */
qse_printf (QSE_T("rewriter->pio is NULL. ....\n"));
			release_rewriter (ursd, rewriter, 1);
		}
	}
	else
	{
		qse_sck_len_t fromlen;
		qse_skad_t from;
		urs_pkt_t* pkt;

		fromlen = QSE_SIZEOF(from);
		x = recvfrom (evt->hnd, ursd->xreq.buf, QSE_SIZEOF(ursd->xreq.buf) - 1, 0, (struct sockaddr*)&from, &fromlen);

		if (x < QSE_SIZEOF(urs_hdr_t)) 
		{
			/* TODO: message logging */
			return;
		}

		pkt = (urs_pkt_t*)ursd->xreq.buf;
		pkt->hdr.pktlen = qse_ntoh16(pkt->hdr.pktlen); /* change the byte order */
		if (pkt->hdr.pktlen != x)
		{
			/* TOOD: message logging */
			return;
		}

		if (enqueue_request (ursd, pkt, &from, fromlen) <= -1)
		{
			/* TODO: error logging for failed enqueuing */

			/* if enqueuing fails, send an empty response */
			pkt->hdr.rcode = qse_hton16(URS_RCODE_ERROR);
			pkt->hdr.pktlen = qse_hton16(QSE_SIZEOF(urs_hdr_t));
			sendto (evt->hnd, pkt, QSE_SIZEOF(urs_hdr_t), 0, (struct sockaddr*)&from, fromlen);
			/* TODO: error logging for sendto failure */
		}
	}
}

static void dispatch_mux_event (qse_mux_t* mux, const qse_mux_evt_t* evt)
{
	mux_xtn_t* mux_xtn;
	int type, index;

	mux_xtn = (mux_xtn_t*)qse_mux_getxtn(mux);

	type = GET_TYPE_FROM_MUX_DATA((qse_uintptr_t)evt->data);
	index = GET_INDEX_FROM_MUX_DATA((qse_uintptr_t)evt->data);

	switch (type)
	{
		case TYPE_SOCKET:
		{
			receive_request_from_socket (mux_xtn->ursd, evt);
			break;
		}

		case TYPE_PIO_OUT:
		{
			/* the rewriter has produced some data */

			qse_ssize_t x;
			urs_pkt_t* pkt;
			qse_size_t maxoutlen;

			rewriter_t* rewriter = &mux_xtn->ursd->rewriters[index];

			if (rewriter->busy)
			{
				pkt = (urs_pkt_t*)rewriter->req.buf;
				maxoutlen = QSE_SIZEOF(rewriter->req.buf) - QSE_SIZEOF(urs_hdr_t);

				x = qse_pio_read (rewriter->pio, QSE_PIO_OUT, &pkt->url[rewriter->req.outlen], maxoutlen - rewriter->req.outlen);
				if (x <= 0)
				{
					/* read failure or end of input */
					rewriter->faulty = 1;
					release_rewriter (mux_xtn->ursd, rewriter, 1);
					/* TODO: error logging */
				}
				else
				{
					rewriter->req.outlen += x;

qse_printf (QSE_T("READ %d, %d bytes from pipes [%.*hs]\n"), (int)x, (int)rewriter->req.outlen, (int)rewriter->req.outlen, pkt->url);

					if (rewriter->req.outlen > maxoutlen)
					{
						/* the rewriter returns too long a result */
						rewriter->faulty = 1;
						release_rewriter (mux_xtn->ursd, rewriter, 1);
						/* TODO: error logging */
					}
					else if (pkt->url[rewriter->req.outlen - 1] == QSE_MT('\n'))
					{
						/* the last byte is a new line. i don't really care about
						 * new lines in the middle of data. the rewriter must 
						 * keep to the protocol. */

						/* add up the header size. -1 to exclude '\n' */
						rewriter->req.outlen += QSE_SIZEOF(urs_hdr_t) - 1;

						pkt->hdr.pktlen = qse_hton16(rewriter->req.outlen); /* change the byte order */
						sendto (mux_xtn->ursd->sck, pkt, rewriter->req.outlen, 0, (struct sockaddr*)&rewriter->req.from, rewriter->req.fromlen);
						/* TODO: error logging */
						/* sendto() to the socket can be blocking. if the socket side is too busy, there's no reason for rewriter to be as busy.
						 * it can wait a while. think about it. */

						release_rewriter (mux_xtn->ursd, rewriter, 0);
					}
				}

/* TODO: is the complete output is not received within time, some actions must be taken. timer based... rewrite timeout */
			}
			else 
			{
				/* something is wrong. if the rewriter process writes something 
				 * while it's not given the full input. reclaim it */
				rewriter->faulty = 1;
				release_rewriter (mux_xtn->ursd, rewriter, 1);
			}

			break;
		}

		case TYPE_PIO_IN:
		{
			/* the pipe to the rewriter is writable. 
			 * pass the leftover to the rewriter */

			rewriter_t* rewriter = &mux_xtn->ursd->rewriters[index];

			QSE_ASSERT (rewriter->pio_in_in_mux);
			QSE_ASSERT (!rewriter->free && !rewriter->busy);

			feed_rewriter (mux_xtn->ursd, rewriter);
			break;
		}
	}
}

static int init_ursd (ursd_t* ursd, int npios, const qse_char_t* cmdline, const qse_char_t* bindaddr)
{
	qse_size_t i;
	qse_nwad_t bindnwad;
	mux_xtn_t* mux_xtn;

	if (npios <= 0) npios = 1;

	qse_memset (ursd, 0, sizeof(*ursd));
	ursd->mmgr = QSE_MMGR_GETDFL();
	
	ursd->cmdline = qse_strdup (cmdline, ursd->mmgr);
	if (ursd->cmdline == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("cannot duplicate cmdline\n"));
		goto oops;
	}

	ursd->mux = qse_mux_open (ursd->mmgr, QSE_SIZEOF(mux_xtn_t), dispatch_mux_event, 100, QSE_NULL);
	if (ursd->mux == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("cannot create a multiplexer\n"));
		goto oops;
	}

	if (qse_strtonwad (bindaddr, &bindnwad) <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("invalid binding address\n"));
		goto oops;
	}

	ursd->sck = open_server_socket (/*IPPROTO_SCTP*/IPPROTO_UDP, &bindnwad);
	if (ursd->sck == QSE_INVALID_SCKHND) goto oops;

	ursd->rewriters = QSE_MMGR_ALLOC (ursd->mmgr, npios * QSE_SIZEOF(rewriter_t));
	if (ursd->rewriters == QSE_NULL) 
	{
		qse_fprintf (QSE_STDERR, QSE_T("cannot allocate rewriters\n"));
		goto oops;
	}
	qse_memset (ursd->rewriters, 0, npios * QSE_SIZEOF(rewriter_t));

	for (i = 0; i < npios; i++) 
	{
		ursd->rewriters[i].index = i;
		start_rewriter (ursd, &ursd->rewriters[i]);
		release_rewriter (ursd, &ursd->rewriters[i], 0);
	}

	if (insert_to_mux (ursd->mux, ursd->sck, TYPE_SOCKET, 0) <= -1) 
	{
		qse_fprintf (QSE_STDERR, QSE_T("cannot add socket to multiplexer\n"));
		goto oops;
	}
	
	ursd->total_rewriter_count = npios;

	mux_xtn = qse_mux_getxtn (ursd->mux);
	mux_xtn->ursd = ursd;
	return 0;

oops:
	if (ursd->rewriters) 
	{
		for (i = 0; i < npios; i++) stop_rewriter (ursd, &ursd->rewriters[i]);
		QSE_MMGR_FREE (ursd->mmgr, ursd->rewriters);
	}
	if (qse_isvalidsckhnd(ursd->sck)) qse_closesckhnd (ursd->sck);
	if (ursd->mux) qse_mux_close (ursd->mux);
	if (ursd->cmdline) QSE_MMGR_FREE(ursd->mmgr, ursd->cmdline);

	return -1;
}

static void fini_ursd (ursd_t* ursd)
{
	qse_size_t i;
	xreq_t* xreq;

	for (i = 0; i < ursd->total_rewriter_count; i++) 
		stop_rewriter (ursd, &ursd->rewriters[i]);

	QSE_MMGR_FREE (ursd->mmgr, ursd->rewriters);

	delete_from_mux (ursd->mux, ursd->sck, TYPE_SOCKET, 0);
	qse_closesckhnd (ursd->sck);
	qse_mux_close (ursd->mux);
	QSE_MMGR_FREE (ursd->mmgr, ursd->cmdline);


	/* destroy the request queue */
	xreq = ursd->xreq.head;
	while (xreq)
	{
		xreq_t* next = xreq->next;
		QSE_MMGR_FREE (ursd->mmgr, xreq);
		xreq = next;
	}

	for (i = 0; i < QSE_COUNTOF(ursd->xreq.free); i++)
	{
		while (ursd->xreq.free[i])
		{
			xreq = ursd->xreq.free[i];
			ursd->xreq.free[i] = xreq->next;
			QSE_MMGR_FREE (ursd->mmgr, xreq);
		}
	}
}

static int g_stop_requested = 0;
static void handle_signal (int sig)
{
	switch (sig)
	{
		case SIGINT:
		case SIGTERM:
		case SIGHUP:
			g_stop_requested = 1;
			break;
	}
}

static void print_usage (qse_char_t* argv0)
{
	qse_fprintf (QSE_STDERR, QSE_T("Usage: %s rewriter-path rewriter-count binding-address\n"), qse_basename(argv0));
}

static int httpd_main (int argc, qse_char_t* argv[])
{
	ursd_t ursd;
	int ursd_inited = 0;


	if (argc < 4)
	{
		print_usage (argv[0]);
		return -1;
	}

	signal (SIGINT, handle_signal);
	signal (SIGTERM, handle_signal);
	signal (SIGHUP, handle_signal);
	signal (SIGPIPE, SIG_IGN);

	if (init_ursd (&ursd, qse_strtoi(argv[2], 10), argv[1], argv[3]) <= -1) goto oops;
	ursd_inited = 1;

	while (!g_stop_requested)
	{
		qse_ntime_t tmout;
		qse_cleartime (&tmout);

		/* if there are pending requests, use timeout of 0.
		 * this way, multiplexer events and pending requests can be
		 * handled together. note if there's no free rewriter while
		 * there's a pending request, the request can't be performed.
		 * the free rewriter check is simpler than the actual check
		 * in get_free_rewriter(). it's also possible that no free
		 * rewriter is available after qse_mux_poll(). this timeout
		 * calculation is on the best-effort basis. */
		if (ursd.xreq.count <= 0 || !ursd.free_rewriter) tmout.sec += 1;
/* TODO: add timer also... and consider that in calculating in tmout */

		qse_mux_poll (ursd.mux, &tmout);

		handle_pending_requests (&ursd);
	}

	fini_ursd (&ursd);
	return 0;

oops:
	if (ursd_inited) fini_ursd (&ursd);
	return -1;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	int ret;

#if defined(_WIN32)
	char locale[100];
	UINT codepage;
	WSADATA wsadata;
#else
	/* nothing */
#endif

#if defined(_WIN32)

	codepage = GetConsoleOutputCP();
	if (codepage == CP_UTF8)
	{
		/*SetConsoleOUtputCP (CP_UTF8);*/
		qse_setdflcmgrbyid (QSE_CMGR_UTF8);
	}
	else
	{
		/* .codepage */
		qse_fmtuintmaxtombs (locale, QSE_COUNTOF(locale),
			codepage, 10, -1, QSE_MT('\0'), QSE_MT("."));
		setlocale (LC_ALL, locale);
		qse_setdflcmgrbyid (QSE_CMGR_SLMB);
	}

	if (WSAStartup (MAKEWORD(2,0), &wsadata) != 0)
	{
		qse_fprintf (QSE_STDERR, QSE_T("failed to start up winsock\n"));
		return -1;
	}

#else
	setlocale (LC_ALL, "");
	qse_setdflcmgrbyid (QSE_CMGR_SLMB);
#endif

	qse_openstdsios ();
	ret = qse_runmain (argc, argv, httpd_main);
	qse_closestdsios ();


#if defined(_WIN32)
	WSACleanup ();
#endif

	return ret;
}
