
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

typedef struct xreq_t xreq_t;
struct xreq_t
{
	qse_skad_t from;
	qse_sck_len_t fromlen;
	qse_uint8_t* data;
	xreq_t* next;
};

typedef struct xpio_t xpio_t;
struct xpio_t
{
	qse_pio_t* pio;
	int busy;

	struct
	{
		qse_skad_t from;
		qse_sck_len_t fromlen;
		qse_uint8_t buf[65535];
	} req;

	xpio_t* prev;
	xpio_t* next;
};

typedef struct ursd_t ursd_t;
struct ursd_t
{
	qse_size_t total;
	qse_size_t nfree;

	xpio_t* xpios;
	xpio_t* free_xpio;
	xpio_t* busy_xpio;

	qse_sck_hnd_t sck;
	qse_mux_t* mux;

	xreq_t* head;
	xreq_t* tail;
};


#define TYPE_SOCKET 0
#define TYPE_PIO    1

#define MAKE_MUX_DATA(type,index)   ((qse_uintptr_t)type | ((qse_uintptr_t)index << 4))
#define GET_TYPE_FROM_MUX_DATA(md)  ((md) & 0xF)
#define GET_INDEX_FROM_MUX_DATA(md) ((md) >> 4)

struct mux_xtn_t
{
	ursd_t* ursd;
};
typedef struct mux_xtn_t mux_xtn_t;

static void chain_to_free_list (ursd_t* ursd, xpio_t* xpio)
{
	xpio->busy = 0;
	xpio->prev = QSE_NULL;
	xpio->next = ursd->free_xpio;
	if (ursd->free_xpio) ursd->free_xpio->prev = xpio;
	ursd->free_xpio = xpio;
	ursd->nfree++;
}

static xpio_t* dechain_from_free_list (ursd_t* ursd, xpio_t* xpio)
{
	if (xpio->next) xpio->next->prev = xpio->prev;
	if (xpio == ursd->free_xpio) ursd->free_xpio = xpio->next;
	else xpio->prev->next = xpio->next;
	ursd->nfree--;
	return xpio;
}

static void chain_to_busy_list (ursd_t* ursd, xpio_t* xpio)
{
	xpio->busy = 1;
	xpio->prev = QSE_NULL;
	xpio->next = ursd->busy_xpio;
	if (ursd->busy_xpio) ursd->busy_xpio->prev = xpio;
	ursd->busy_xpio = xpio;
}

static xpio_t* dechain_from_busy_list (ursd_t* ursd, xpio_t* xpio)
{
	if (xpio->next) xpio->next->prev = xpio->prev;

	if (xpio == ursd->busy_xpio) ursd->busy_xpio = xpio->next;
	else xpio->prev->next = xpio->next;
	
	return xpio;
}

static xreq_t* enqueue_request (ursd_t* ursd, urs_pkt_t* pkt)
{
	return QSE_NULL;
}

static int insert_to_mux (qse_mux_t* mux, qse_mux_hnd_t handle, int type, int index)
{
	qse_mux_evt_t evt;

	memset (&evt, 0, QSE_SIZEOF(evt));
	evt.hnd = handle;
	evt.mask = QSE_MUX_IN;
	evt.data = MAKE_MUX_DATA(type, index);
	return qse_mux_insert (mux, &evt);
}

static int delete_from_mux (qse_mux_t* mux, qse_mux_hnd_t handle, int type, int index)
{
	qse_mux_evt_t evt;

	memset (&evt, 0, QSE_SIZEOF(evt));
	evt.hnd = handle;
	evt.mask = QSE_MUX_IN;
	evt.data = MAKE_MUX_DATA(type, index);
	return qse_mux_delete (mux, &evt);
}

static qse_sck_hnd_t open_server_socket (int proto, const qse_nwad_t* bindnwad)
{
	qse_sck_hnd_t s = QSE_INVALID_SCKHND;
	qse_skad_t skad;
	qse_sck_len_t skad_len;
	int family, type, flag;

	skad_len = qse_nwadtoskad (bindnwad, &skad);
	family = qse_skadfamily(&skad);
	type = (proto == IPPROTO_SCTP)? SOCK_SEQPACKET: SOCK_DGRAM;

	s = socket (family, type, proto);
	if (!qse_isvalidsckhnd(s))
	{
		fprintf (stderr, "cannot create a socket\n");
		goto oops;
	}

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
		fprintf (stderr, "cannot bind a socket\n");
		goto oops;
		
	}

bind_ok:
	if (proto == IPPROTO_SCTP)
	{
#if 1
		struct sctp_initmsg im;
		struct sctp_paddrparams hb;

		qse_memset (&im, 0, QSE_SIZEOF(im));
		im.sinit_num_ostreams = 1;
		im.sinit_max_instreams = 1;
		im.sinit_max_attempts = 1;

		if (setsockopt (s, SOL_SCTP, SCTP_INITMSG, &im, QSE_SIZEOF(im)) <= -1) 
		{
			fprintf (stderr, "cannot set sctp initmsg option\n");
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
			fprintf (stderr, "cannot set listen on sctp socket\n");
			goto oops;
		}
	}

	return s;

oops:
	if (qse_isvalidsckhnd(s)) qse_closesckhnd (s);
	return QSE_INVALID_SCKHND;
}

static void schedule_request (ursd_t* ursd, urs_pkt_t* pkt, const qse_skad_t* skad, qse_sck_len_t skadlen)
{
	if (ursd->free_xpio)
	{
		xpio_t* xpio = dechain_from_free_list (ursd, ursd->free_xpio);

		xpio->req.from = *skad;
		xpio->req.fromlen = skadlen;
		qse_memcpy (xpio->req.buf, pkt, QSE_SIZEOF(urs_hdr_t)); /* copy header */

printf ("XPIO WRITNG TO PIPE %p %d\n", xpio, qse_skadfamily(skad));
		qse_pio_write (xpio->pio, QSE_PIO_IN, pkt->url, pkt->hdr.pktlen - QSE_SIZEOF(urs_hdr_t)); /* TODO: error handling */
		chain_to_busy_list (ursd, xpio);
	}
	else
	{
		/* queue up in the internal queue... */

		/* TODO */
	}
}

static void dispatch_mux_event (qse_mux_t* mux, const qse_mux_evt_t* evt)
{
	mux_xtn_t* mux_xtn;
	int type, index;
	qse_skad_t skad;
	qse_sck_len_t skad_len;
	
	unsigned char buf[65535];

	mux_xtn = (mux_xtn_t*)qse_mux_getxtn(mux);

	type = GET_TYPE_FROM_MUX_DATA((qse_uintptr_t)evt->data);
	index = GET_INDEX_FROM_MUX_DATA((qse_uintptr_t)evt->data);

	if (type == TYPE_SOCKET)
	{
		ssize_t x;

		skad_len = QSE_SIZEOF(skad);
		x = recvfrom (evt->hnd, buf, QSE_SIZEOF(buf), 0, (struct sockaddr*)&skad, &skad_len);

printf ("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXx\n");
/* TODO: error handling */
		if (x >= QSE_SIZEOF(urs_hdr_t))
		{
			urs_pkt_t* pkt = (urs_pkt_t*)buf;

			pkt->hdr.pktlen = qse_ntoh16(pkt->hdr.pktlen); /* change the byte order */
			if (pkt->hdr.pktlen == x)
			{
				printf ("%d [[[%.*s]]]\n", (int)x, (int)(pkt->hdr.pktlen - QSE_SIZEOF(urs_hdr_t)), pkt->url);
				schedule_request (mux_xtn->ursd, pkt, &skad, skad_len);
			}
		}
	}
	else
	{
		qse_ssize_t x;
		urs_pkt_t* pkt;
		xpio_t* xpio = &mux_xtn->ursd->xpios[index];

		if (xpio->busy)
		{
			pkt = (urs_pkt_t*)xpio->req.buf;

			x = qse_pio_read (xpio->pio, QSE_PIO_OUT, pkt->url, QSE_SIZEOF(xpio->req.buf) - QSE_SIZEOF(urs_hdr_t));
printf ("READ %d bytes from pipes [%.*s]\n", (int)x, (int)x, pkt->url);

			x += QSE_SIZEOF(urs_hdr_t); /* add up the header size */
			if (x > QSE_TYPE_MAX(qse_uint16_t))
			{
				/* ERROR HANDLING - it's returning too long data */
			}

			pkt->hdr.pktlen = qse_hton16(x); /* change the byte order */
			sendto (mux_xtn->ursd->sck, pkt, x, 0, (struct sockaddr*)&xpio->req.from, xpio->req.fromlen);

	/* TODO: error handling */

			/* TODO: if there is a pending request, use this xpio to send request... */

			dechain_from_busy_list (mux_xtn->ursd, xpio);
			chain_to_free_list (mux_xtn->ursd, xpio);
		}
		else 
		{
			/* something is wrong. if the child process writes something 
			 * while it's not given any input. restart this process */

			/* TODO: */
		}
	}
}

static int init_ursd (ursd_t* ursd, int npios, const qse_char_t* cmdline, const qse_char_t* bindaddr)
{
	qse_size_t i;
	qse_nwad_t bindnwad;
	mux_xtn_t* mux_xtn;

	memset (ursd, 0, sizeof(*ursd));

	ursd->mux = qse_mux_open (QSE_MMGR_GETDFL(), QSE_SIZEOF(mux_xtn_t), dispatch_mux_event, 100, QSE_NULL);
	if (ursd->mux == QSE_NULL)
	{
		fprintf (stderr, "cannot create a multiplexer\n");
		goto oops;
	}

	if (qse_strtonwad (bindaddr, &bindnwad) <= -1)
	{
		fprintf (stderr, "invalid binding address\n");
		goto oops;
	}

	ursd->sck = open_server_socket (/*IPPROTO_SCTP*/IPPROTO_UDP, &bindnwad);

	ursd->xpios = calloc (npios, QSE_SIZEOF(xpio_t));
	if (ursd->xpios == QSE_NULL) 
	{
		fprintf (stderr, "cannot callocate pipes\n");
		goto oops;
	}

	for (i = 0; i < npios; i++)
	{
		qse_pio_t* pio;

		pio = qse_pio_open (
			QSE_MMGR_GETDFL(), 0, cmdline, QSE_NULL, 
			QSE_PIO_WRITEIN | QSE_PIO_READOUT | QSE_PIO_ERRTONUL | 
			QSE_PIO_INNOBLOCK | QSE_PIO_OUTNOBLOCK
		);
		if (pio == QSE_NULL) goto oops;

		ursd->xpios[i].pio = pio;
	}

	if (insert_to_mux (ursd->mux, ursd->sck, TYPE_SOCKET, 0) <= -1) 
	{
		fprintf (stderr, "cannot add socket to multiplexer\n");
		goto oops;
	}
	for (i = 0; i < npios; i++)
	{
		if (insert_to_mux (ursd->mux, qse_pio_gethandle(ursd->xpios[i].pio, QSE_PIO_OUT), TYPE_PIO, i) <= -1) 
		{
			fprintf (stderr, "cannot add pipe to multiplexer\n");
			goto oops;
		}
	}

	for (i = 0; i < npios; i++) chain_to_free_list (ursd, &ursd->xpios[i]);

	ursd->total = npios;

	mux_xtn = qse_mux_getxtn (ursd->mux);
	mux_xtn->ursd = ursd;
	return 0;

oops:
	if (ursd->mux) qse_mux_close (ursd->mux);

	for (i = 0; i < npios; i++)
	{
		if (ursd->xpios[i].pio) qse_pio_close (ursd->xpios[i].pio);
	}
	if (ursd->xpios) free (ursd->xpios);

	return -1;
}

static void fini_ursd (ursd_t* ursd)
{
	qse_size_t i;

	/* destroy the multiplex first. i don't want to delete handles explicitly */
	qse_mux_close (ursd->mux);

	for (i = 0; i < ursd->total; i++)
	{
		if (ursd->xpios[i].pio) qse_pio_close (ursd->xpios[i].pio);
	}
	if (ursd->xpios) free (ursd->xpios);

	qse_closesckhnd (ursd->sck);
}

static int httpd_main (int argc, qse_char_t* argv[])
{
	ursd_t ursd;
	int ursd_inited = 0;

	if (init_ursd (&ursd, 10, QSE_T("/bin/cat"), QSE_T("[::]:97]")) <= -1) goto oops;
	ursd_inited = 1;

	while (1)
	{
		qse_ntime_t tmout;
		qse_cleartime (&tmout);
		tmout.sec += 1;
		qse_mux_poll (ursd.mux, &tmout);
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
		qse_fprintf (QSE_STDERR, QSE_T("Failed to start up winsock\n"));
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
