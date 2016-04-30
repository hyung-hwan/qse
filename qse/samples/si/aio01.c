/*
 * $Id$
 *
    Copyright (c) 2015-2016 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WAfRRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <qse/si/aio.h>
#include <qse/si/aio-sck.h>
#include <qse/si/aio-pro.h>
#include <qse/cmn/hton.h>
#include <qse/cmn/mem.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#include <net/if.h>

#include <assert.h>

#if defined(HAVE_OPENSSL_SSL_H) && defined(HAVE_SSL)
#	include <openssl/ssl.h>
#	if defined(HAVE_OPENSSL_ERR_H)
#		include <openssl/err.h>
#	endif
#	if defined(HAVE_OPENSSL_ENGINE_H)
#		include <openssl/engine.h>
#	endif
#	define USE_SSL
#endif

/* ========================================================================= */

/* ========================================================================= */

#if defined(USE_SSL)
static void cleanup_openssl ()
{
	/* ERR_remove_state() should be called for each thread if the application is thread */
	ERR_remove_state (0);
#if defined(HAVE_ENGINE_CLEANUP)
	ENGINE_cleanup ();
#endif
	ERR_free_strings ();
	EVP_cleanup ();
#if defined(HAVE_CRYPTO_CLEANUP_ALL_EX_DATA)
	CRYPTO_cleanup_all_ex_data ();
#endif
}
#endif

struct tcp_server_t
{
	int tally;
};
typedef struct tcp_server_t tcp_server_t;

static void tcp_sck_on_disconnect (qse_aio_dev_sck_t* tcp)
{
	switch (QSE_AIO_DEV_SCK_GET_PROGRESS(tcp))
	{
		case QSE_AIO_DEV_SCK_CONNECTING:
			printf ("OUTGOING SESSION DISCONNECTED - FAILED TO CONNECT (%d) TO REMOTE SERVER\n", (int)tcp->sck);
			break;

		case QSE_AIO_DEV_SCK_CONNECTING_SSL:
			printf ("OUTGOING SESSION DISCONNECTED - FAILED TO SSL-CONNECT (%d) TO REMOTE SERVER\n", (int)tcp->sck);
			break;

		case QSE_AIO_DEV_SCK_LISTENING:
			printf ("SHUTTING DOWN THE SERVER SOCKET(%d)...\n", (int)tcp->sck);
			break;

		case QSE_AIO_DEV_SCK_CONNECTED:
			printf ("OUTGOING CLIENT CONNECTION GOT TORN DOWN(%d).......\n", (int)tcp->sck);
			break;

		case QSE_AIO_DEV_SCK_ACCEPTING_SSL:
			printf ("INCOMING SSL-ACCEPT GOT DISCONNECTED(%d) ....\n", (int)tcp->sck);
			break;

		case QSE_AIO_DEV_SCK_ACCEPTED:
			printf ("INCOMING CLIENT BEING SERVED GOT DISCONNECTED(%d).......\n", (int)tcp->sck);
			break;

		default:
			printf ("SOCKET DEVICE DISCONNECTED (%d - %x)\n", (int)tcp->sck, (unsigned int)tcp->state);
			break;
	}
}
static int tcp_sck_on_connect (qse_aio_dev_sck_t* tcp)
{

	qse_aio_sckfam_t fam;
	qse_aio_scklen_t len;
	qse_mchar_t buf1[128], buf2[128];

	memset (buf1, 0, QSE_SIZEOF(buf1));
	memset (buf2, 0, QSE_SIZEOF(buf2));

	qse_aio_getsckaddrinfo (tcp->aio, &tcp->localaddr, &len, &fam);
	inet_ntop (fam, tcp->localaddr.data, buf1, QSE_COUNTOF(buf1));

	qse_aio_getsckaddrinfo (tcp->aio, &tcp->remoteaddr, &len, &fam);
	inet_ntop (fam, tcp->remoteaddr.data, buf2, QSE_COUNTOF(buf2));

	if (tcp->state & QSE_AIO_DEV_SCK_CONNECTED)
	{

printf ("device connected to a remote server... LOCAL %s:%d REMOTE %s:%d.", buf1, qse_aio_getsckaddrport(&tcp->localaddr), buf2, qse_aio_getsckaddrport(&tcp->remoteaddr));

	}
	else if (tcp->state & QSE_AIO_DEV_SCK_ACCEPTED)
	{
printf ("device accepted client device... .LOCAL %s:%d REMOTE %s:%d\n", buf1, qse_aio_getsckaddrport(&tcp->localaddr), buf2, qse_aio_getsckaddrport(&tcp->remoteaddr));
	}

	return qse_aio_dev_sck_write  (tcp, "hello", 5, QSE_NULL, QSE_NULL);
}

static int tcp_sck_on_write (qse_aio_dev_sck_t* tcp, qse_aio_iolen_t wrlen, void* wrctx, const qse_aio_sckaddr_t* dstaddr)
{
	tcp_server_t* ts;

if (wrlen <= -1)
{
printf ("SEDING TIMED OUT...........\n");
	qse_aio_dev_sck_halt(tcp);
}
else
{
	ts = (tcp_server_t*)(tcp + 1);
	printf (">>> SENT MESSAGE %d of length %ld\n", ts->tally, (long int)wrlen);

	ts->tally++;
//	if (ts->tally >= 2) qse_aio_dev_sck_halt (tcp);

printf ("ENABLING READING..............................\n");
	qse_aio_dev_sck_read (tcp, 1);

	//qse_aio_dev_sck_timedread (tcp, 1, 1000);
}
	return 0;
}

static int tcp_sck_on_read (qse_aio_dev_sck_t* tcp, const void* buf, qse_aio_iolen_t len, const qse_aio_sckaddr_t* srcaddr)
{
	int n;

	if (len <= 0)
	{
		printf ("STREAM DEVICE: EOF RECEIVED...\n");
		/* no outstanding request. but EOF */
		qse_aio_dev_sck_halt (tcp);
		return 0;
	}

printf ("on read %d\n", (int)len);

{
qse_ntime_t tmout;

static char a ='A';
char* xxx = malloc (1000000);
memset (xxx, a++ ,1000000);

	//return qse_aio_dev_sck_write  (tcp, "HELLO", 5, QSE_NULL);
	qse_inittime (&tmout, 5, 0);
	n = qse_aio_dev_sck_timedwrite  (tcp, xxx, 1000000, &tmout, QSE_NULL, QSE_NULL);
free (xxx);


	if (n <= -1) return -1;
}


printf ("DISABLING READING..............................\n");
	qse_aio_dev_sck_read (tcp, 0);

	/* post the write finisher */
	n = qse_aio_dev_sck_write  (tcp, QSE_NULL, 0, QSE_NULL, QSE_NULL);
	if (n <= -1) return -1;

	return 0;

/* return 1; let the main loop to read more greedily without consulting the multiplexer */
}

/* ========================================================================= */

static void pro_on_close (qse_aio_dev_pro_t* dev, qse_aio_dev_pro_sid_t sid)
{
printf (">>>>>>>>>>>>> ON CLOSE OF SLAVE %d.\n", sid);
}

static int pro_on_read (qse_aio_dev_pro_t* dev, const void* data, qse_aio_iolen_t dlen, qse_aio_dev_pro_sid_t sid)
{
printf ("PROCESS READ DATA on SLAVE[%d]... [%.*s]\n", (int)sid, (int)dlen, (char*)data);
	return 0;
}


static int pro_on_write (qse_aio_dev_pro_t* dev, qse_aio_iolen_t wrlen, void* wrctx)
{
printf ("PROCESS WROTE DATA...\n");
	return 0;
}

/* ========================================================================= */

static int arp_sck_on_read (qse_aio_dev_sck_t* dev, const void* data, qse_aio_iolen_t dlen, const qse_aio_sckaddr_t* srcaddr)
{
	qse_aio_etharp_pkt_t* eap;


	if (dlen < QSE_SIZEOF(*eap)) return 0; /* drop */

	eap = (qse_aio_etharp_pkt_t*)data;

	printf ("ARP ON IFINDEX %d OPCODE: %d", qse_aio_getsckaddrifindex(srcaddr), ntohs(eap->arphdr.opcode));

	printf (" SHA: %02X:%02X:%02X:%02X:%02X:%02X", eap->arppld.sha[0], eap->arppld.sha[1], eap->arppld.sha[2], eap->arppld.sha[3], eap->arppld.sha[4], eap->arppld.sha[5]);
	printf (" SPA: %d.%d.%d.%d", eap->arppld.spa[0], eap->arppld.spa[1], eap->arppld.spa[2], eap->arppld.spa[3]);
	printf (" THA: %02X:%02X:%02X:%02X:%02X:%02X", eap->arppld.tha[0], eap->arppld.tha[1], eap->arppld.tha[2], eap->arppld.tha[3], eap->arppld.tha[4], eap->arppld.tha[5]);
	printf (" TPA: %d.%d.%d.%d", eap->arppld.tpa[0], eap->arppld.tpa[1], eap->arppld.tpa[2], eap->arppld.tpa[3]);
	printf ("\n");
	return 0;
}

static int arp_sck_on_write (qse_aio_dev_sck_t* dev, qse_aio_iolen_t wrlen, void* wrctx, const qse_aio_sckaddr_t* dstaddr)
{
	return 0;
}

static void arp_sck_on_disconnect (qse_aio_dev_sck_t* dev)
{
printf ("SHUTTING DOWN ARP SOCKET %d...\n", dev->sck);
}

static int setup_arp_tester (qse_aio_t* aio)
{
	qse_aio_sckaddr_t ethdst;
	qse_aio_etharp_pkt_t etharp;
	qse_aio_dev_sck_make_t sck_make;
	qse_aio_dev_sck_t* sck;

	memset (&sck_make, 0, QSE_SIZEOF(sck_make));
	sck_make.type = QSE_AIO_DEV_SCK_ARP;
	//sck_make.type = QSE_AIO_DEV_SCK_ARP_DGRAM;
	sck_make.on_write = arp_sck_on_write;
	sck_make.on_read = arp_sck_on_read;
	sck_make.on_disconnect = arp_sck_on_disconnect;
	sck = qse_aio_dev_sck_make (aio, 0, &sck_make);
	if (!sck)
	{
		printf ("Cannot make socket device\n");
		return -1;
	}

	//qse_aio_sckaddr_initforeth (&ethdst, if_nametoindex("enp0s25.3"), (qse_aio_ethaddr_t*)"\xFF\xFF\xFF\xFF\xFF\xFF");
	qse_aio_sckaddr_initforeth (&ethdst, if_nametoindex("enp0s25.3"), (qse_aio_ethaddr_t*)"\xAA\xBB\xFF\xCC\xDD\xFF");

	memset (&etharp, 0, sizeof(etharp));

	memcpy (etharp.ethhdr.source, "\xB8\x6B\x23\x9C\x10\x76", QSE_AIO_ETHADDR_LEN);
	//memcpy (etharp.ethhdr.dest, "\xFF\xFF\xFF\xFF\xFF\xFF", QSE_AIO_ETHADDR_LEN);
	memcpy (etharp.ethhdr.dest, "\xAA\xBB\xFF\xCC\xDD\xFF", QSE_AIO_ETHADDR_LEN);
	etharp.ethhdr.proto = QSE_CONST_HTON16(QSE_AIO_ETHHDR_PROTO_ARP);

	etharp.arphdr.htype = QSE_CONST_HTON16(QSE_AIO_ARPHDR_HTYPE_ETH);
	etharp.arphdr.ptype = QSE_CONST_HTON16(QSE_AIO_ARPHDR_PTYPE_IP4);
	etharp.arphdr.hlen = QSE_AIO_ETHADDR_LEN;
	etharp.arphdr.plen = QSE_AIO_IP4ADDR_LEN;
	etharp.arphdr.opcode = QSE_CONST_HTON16(QSE_AIO_ARPHDR_OPCODE_REQUEST);

	memcpy (etharp.arppld.sha, "\xB8\x6B\x23\x9C\x10\x76", QSE_AIO_ETHADDR_LEN);

	if (qse_aio_dev_sck_write (sck, &etharp, sizeof(etharp), NULL, &ethdst) <= -1)
	//if (qse_aio_dev_sck_write (sck, &etharp.arphdr, sizeof(etharp) - sizeof(etharp.ethhdr), NULL, &ethaddr) <= -1)
	{
		printf ("CANNOT WRITE ARP...\n");
	}


	return 0;
}

/* ========================================================================= */

struct icmpxtn_t
{
	qse_uint16_t icmp_seq;
	qse_aio_tmridx_t tmout_jobidx;
	int reply_received;
};

typedef struct icmpxtn_t icmpxtn_t;

static int schedule_icmp_wait (qse_aio_dev_sck_t* dev);

static void send_icmp (qse_aio_dev_sck_t* dev, qse_uint16_t seq)
{
	qse_aio_sckaddr_t dstaddr;
	qse_aio_ip4addr_t ia;
	qse_aio_icmphdr_t* icmphdr;
	qse_uint8_t buf[512];

	inet_pton (AF_INET, "192.168.1.131", &ia);
	qse_aio_sckaddr_initforip4 (&dstaddr, 0, &ia);

	memset(buf, 0, QSE_SIZEOF(buf));
	icmphdr = (qse_aio_icmphdr_t*)buf;
	icmphdr->type = QSE_AIO_ICMP_ECHO_REQUEST;
	icmphdr->u.echo.id = QSE_CONST_HTON16(100);
	icmphdr->u.echo.seq = qse_hton16(seq);

	memset (&buf[QSE_SIZEOF(*icmphdr)], 'A', QSE_SIZEOF(buf) - QSE_SIZEOF(*icmphdr));
	icmphdr->checksum = qse_aio_checksumip (icmphdr, QSE_SIZEOF(buf));

	if (qse_aio_dev_sck_write (dev, buf, QSE_SIZEOF(buf), NULL, &dstaddr) <= -1)
	{
		printf ("CANNOT WRITE ICMP...\n");
		qse_aio_dev_sck_halt (dev);
	}

	if (schedule_icmp_wait (dev) <= -1)
	{
		printf ("CANNOT SCHEDULE ICMP WAIT...\n");
		qse_aio_dev_sck_halt (dev);
	}
}

static void on_icmp_due (qse_aio_t* aio, const qse_ntime_t* now, qse_aio_tmrjob_t* tmrjob)
{
	qse_aio_dev_sck_t* dev;
	icmpxtn_t* icmpxtn;

	dev = tmrjob->ctx;
	icmpxtn = (icmpxtn_t*)(dev + 1);

	if (icmpxtn->reply_received)
		icmpxtn->reply_received = 0;
	else
		printf ("NO ICMP REPLY RECEIVED....\n");

	send_icmp (dev, ++icmpxtn->icmp_seq);
}

static int schedule_icmp_wait (qse_aio_dev_sck_t* dev)
{
	icmpxtn_t* icmpxtn;
	qse_aio_tmrjob_t tmrjob;
	qse_ntime_t fire_after;

	icmpxtn = (icmpxtn_t*)(dev + 1);
	qse_inittime (&fire_after, 2, 0);

	memset (&tmrjob, 0, QSE_SIZEOF(tmrjob));
	tmrjob.ctx = dev;
	qse_gettime (&tmrjob.when);
	qse_addtime (&tmrjob.when, &fire_after, &tmrjob.when);
	tmrjob.handler = on_icmp_due;
	tmrjob.idxptr = &icmpxtn->tmout_jobidx;

	assert (icmpxtn->tmout_jobidx == QSE_AIO_TMRIDX_INVALID);

	return (qse_aio_instmrjob (dev->aio, &tmrjob) == QSE_AIO_TMRIDX_INVALID)? -1: 0;
}

static int icmp_sck_on_read (qse_aio_dev_sck_t* dev, const void* data, qse_aio_iolen_t dlen, const qse_aio_sckaddr_t* srcaddr)
{
	icmpxtn_t* icmpxtn;
	qse_aio_iphdr_t* iphdr;
	qse_aio_icmphdr_t* icmphdr;

	/* when received, the data contains the IP header.. */
	icmpxtn = (icmpxtn_t*)(dev + 1);

	if (dlen < QSE_SIZEOF(*iphdr) + QSE_SIZEOF(*icmphdr))
	{
		printf ("INVALID ICMP PACKET.. TOO SHORT...%d\n", (int)dlen);
	}
	else
	{
		/* TODO: consider IP options... */
		iphdr = (qse_aio_iphdr_t*)data;

		if (iphdr->ihl * 4 + QSE_SIZEOF(*icmphdr) > dlen)
		{
			printf ("INVALID ICMP PACKET.. WRONG IHL...%d\n", (int)iphdr->ihl * 4);
		}
		else
		{
			icmphdr = (qse_aio_icmphdr_t*)((qse_uint8_t*)data + (iphdr->ihl * 4));

			/* TODO: check srcaddr against target */

			if (icmphdr->type == QSE_AIO_ICMP_ECHO_REPLY && 
			    qse_ntoh16(icmphdr->u.echo.seq) == icmpxtn->icmp_seq) /* TODO: more check.. echo.id.. */
			{
				icmpxtn->reply_received = 1;
				printf ("ICMP REPLY RECEIVED...ID %d SEQ %d\n", (int)qse_ntoh16(icmphdr->u.echo.id), (int)qse_ntoh16(icmphdr->u.echo.seq));
			}
			else
			{
				printf ("GARBAGE ICMP PACKET...LEN %d SEQ %d,%d\n", (int)dlen, (int)icmpxtn->icmp_seq, (int)qse_ntoh16(icmphdr->u.echo.seq));
			}
		}
	}
	return 0;
}


static int icmp_sck_on_write (qse_aio_dev_sck_t* dev, qse_aio_iolen_t wrlen, void* wrctx, const qse_aio_sckaddr_t* dstaddr)
{
	/*icmpxtn_t* icmpxtn;

	icmpxtn = (icmpxtn_t*)(dev + 1); */

	return 0;
}

static void icmp_sck_on_disconnect (qse_aio_dev_sck_t* dev)
{
	icmpxtn_t* icmpxtn;

	icmpxtn = (icmpxtn_t*)(dev + 1);

printf ("SHUTTING DOWN ICMP SOCKET %d...\n", dev->sck);
	if (icmpxtn->tmout_jobidx != QSE_AIO_TMRIDX_INVALID)
	{

		qse_aio_deltmrjob (dev->aio, icmpxtn->tmout_jobidx);
		icmpxtn->tmout_jobidx = QSE_AIO_TMRIDX_INVALID;
	}
}

static int setup_ping4_tester (qse_aio_t* aio)
{
	qse_aio_dev_sck_make_t sck_make;
	qse_aio_dev_sck_t* sck;
	icmpxtn_t* icmpxtn;

	memset (&sck_make, 0, QSE_SIZEOF(sck_make));
	sck_make.type = QSE_AIO_DEV_SCK_ICMP4;
	sck_make.on_write = icmp_sck_on_write;
	sck_make.on_read = icmp_sck_on_read;
	sck_make.on_disconnect = icmp_sck_on_disconnect;

	sck = qse_aio_dev_sck_make (aio, QSE_SIZEOF(icmpxtn_t), &sck_make);
	if (!sck)
	{
		printf ("Cannot make ICMP4 socket device\n");
		return -1;
	}

	icmpxtn = (icmpxtn_t*)(sck + 1);
	icmpxtn->tmout_jobidx = QSE_AIO_TMRIDX_INVALID;
	icmpxtn->icmp_seq = 0;

	/*TODO: qse_aio_dev_sck_setbroadcast (sck, 1);*/

	send_icmp (sck, ++icmpxtn->icmp_seq);

	return 0;
}


/* ========================================================================= */

static qse_aio_t* g_aio;

static void handle_signal (int sig)
{
	if (g_aio) qse_aio_stop (g_aio, QSE_AIO_STOPREQ_TERMINATION);
}

int main ()
{
	int i;

	qse_aio_t* aio;
	qse_aio_dev_sck_t* tcp[3];

	struct sigaction sigact;
	qse_aio_dev_sck_connect_t tcp_conn;
	qse_aio_dev_sck_listen_t tcp_lstn;
	qse_aio_dev_sck_bind_t tcp_bind;
	qse_aio_dev_sck_make_t tcp_make;

	tcp_server_t* ts;

#if defined(USE_SSL)
	SSL_load_error_strings ();
	SSL_library_init ();
#endif

	aio = qse_aio_open (QSE_MMGR_GETDFL(), 0, 512, QSE_NULL);
	if (!aio)
	{
		printf ("Cannot open aio\n");
		return -1;
	}

	g_aio = aio;

	memset (&sigact, 0, QSE_SIZEOF(sigact));
	sigact.sa_flags = SA_RESTART;
	sigact.sa_handler = handle_signal;
	sigaction (SIGINT, &sigact, QSE_NULL);

	memset (&sigact, 0, QSE_SIZEOF(sigact));
	sigact.sa_handler = SIG_IGN;
	sigaction (SIGPIPE, &sigact, QSE_NULL);

/*
	memset (&sigact, 0, QSE_SIZEOF(sigact));
	sigact.sa_handler = SIG_IGN;
	sigaction (SIGCHLD, &sigact, QSE_NULL);
*/

	/*memset (&sin, 0, QSE_SIZEOF(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(1234); */
/*
	udp = (qse_aio_dev_udp_t*)qse_aio_makedev (aio, QSE_SIZEOF(*udp), &udp_mth, &udp_evcb, &sin);
	if (!udp)
	{
		printf ("Cannot make udp\n");
		goto oops;
	}
*/

	memset (&tcp_make, 0, QSE_SIZEOF(&tcp_make));
	tcp_make.type = QSE_AIO_DEV_SCK_TCP4;
	tcp_make.on_write = tcp_sck_on_write;
	tcp_make.on_read = tcp_sck_on_read;
	tcp_make.on_disconnect = tcp_sck_on_disconnect;
	tcp[0] = qse_aio_dev_sck_make (aio, QSE_SIZEOF(tcp_server_t), &tcp_make);
	if (!tcp[0])
	{
		printf ("Cannot make tcp\n");
		goto oops;
	}

	ts = (tcp_server_t*)(tcp[0] + 1);
	ts->tally = 0;


	memset (&tcp_conn, 0, QSE_SIZEOF(tcp_conn));
{
	in_addr_t ia = inet_addr("192.168.1.119");
	qse_aio_sckaddr_initforip4 (&tcp_conn.remoteaddr, 9999, (qse_aio_ip4addr_t*)&ia);
}

	qse_inittime (&tcp_conn.connect_tmout, 5, 0);
	tcp_conn.on_connect = tcp_sck_on_connect;
	tcp_conn.options = QSE_AIO_DEV_SCK_CONNECT_SSL;
	if (qse_aio_dev_sck_connect (tcp[0], &tcp_conn) <= -1)
	{
		printf ("qse_aio_dev_sck_connect() failed....\n");
		/* carry on regardless of failure */
	}

	/* -------------------------------------------------------------- */
	memset (&tcp_make, 0, QSE_SIZEOF(&tcp_make));
	tcp_make.type = QSE_AIO_DEV_SCK_TCP4;
	tcp_make.on_write = tcp_sck_on_write;
	tcp_make.on_read = tcp_sck_on_read;
	tcp_make.on_disconnect = tcp_sck_on_disconnect;

	tcp[1] = qse_aio_dev_sck_make (aio, QSE_SIZEOF(tcp_server_t), &tcp_make);
	if (!tcp[1])
	{
		printf ("Cannot make tcp\n");
		goto oops;
	}
	ts = (tcp_server_t*)(tcp[1] + 1);
	ts->tally = 0;

	memset (&tcp_bind, 0, QSE_SIZEOF(tcp_bind));
	qse_aio_sckaddr_initforip4 (&tcp_bind.localaddr, 1234, QSE_NULL);
	tcp_bind.options = QSE_AIO_DEV_SCK_BIND_REUSEADDR;

	if (qse_aio_dev_sck_bind (tcp[1],&tcp_bind) <= -1)
	{
		printf ("qse_aio_dev_sck_bind() failed....\n");
		goto oops;
	}


	tcp_lstn.backlogs = 100;
	tcp_lstn.on_connect = tcp_sck_on_connect;
	if (qse_aio_dev_sck_listen (tcp[1], &tcp_lstn) <= -1)
	{
		printf ("qse_aio_dev_sck_listen() failed....\n");
		goto oops;
	}

	/* -------------------------------------------------------------- */
	memset (&tcp_make, 0, QSE_SIZEOF(&tcp_make));
	tcp_make.type = QSE_AIO_DEV_SCK_TCP4;
	tcp_make.on_write = tcp_sck_on_write;
	tcp_make.on_read = tcp_sck_on_read;
	tcp_make.on_disconnect = tcp_sck_on_disconnect;

	tcp[2] = qse_aio_dev_sck_make (aio, QSE_SIZEOF(tcp_server_t), &tcp_make);
	if (!tcp[2])
	{
		printf ("Cannot make tcp\n");
		goto oops;
	}
	ts = (tcp_server_t*)(tcp[2] + 1);
	ts->tally = 0;

	memset (&tcp_bind, 0, QSE_SIZEOF(tcp_bind));
	qse_aio_sckaddr_initforip4 (&tcp_bind.localaddr, 1235, QSE_NULL);
	tcp_bind.options = QSE_AIO_DEV_SCK_BIND_REUSEADDR | /*QSE_AIO_DEV_SCK_BIND_REUSEPORT |*/ QSE_AIO_DEV_SCK_BIND_SSL; 
	tcp_bind.ssl_certfile = QSE_MT("localhost.crt");
	tcp_bind.ssl_keyfile = QSE_MT("localhost.key");
	qse_inittime (&tcp_bind.accept_tmout, 5, 1);

	if (qse_aio_dev_sck_bind (tcp[2],&tcp_bind) <= -1)
	{
		printf ("qse_aio_dev_sck_bind() failed....\n");
		goto oops;
	}

	tcp_lstn.backlogs = 100;
	tcp_lstn.on_connect = tcp_sck_on_connect;
	if (qse_aio_dev_sck_listen (tcp[2], &tcp_lstn) <= -1)
	{
		printf ("qse_aio_dev_sck_listen() failed....\n");
		goto oops;
	}

	//qse_aio_dev_sck_sendfile (tcp[2], fd, offset, count);

	if (setup_arp_tester(aio) <= -1) goto oops;
	if (setup_ping4_tester(aio) <= -1) goto oops;


for (i = 0; i < 5; i++)
{
	qse_aio_dev_pro_t* pro;
	qse_aio_dev_pro_make_t pro_make;

	memset (&pro_make, 0, QSE_SIZEOF(pro_make));
	pro_make.flags = QSE_AIO_DEV_PRO_READOUT | QSE_AIO_DEV_PRO_READERR | QSE_AIO_DEV_PRO_WRITEIN /*| QSE_AIO_DEV_PRO_FORGET_CHILD*/;
	//pro_make.cmd = "/bin/ls -laF /usr/bin";
	//pro_make.cmd = "/bin/ls -laF";
	pro_make.cmd = "./a";
	pro_make.on_read = pro_on_read;
	pro_make.on_write = pro_on_write;
	pro_make.on_close = pro_on_close;

	pro = qse_aio_dev_pro_make (aio, 0, &pro_make);
	if (!pro)
	{
		printf ("CANNOT CREATE PROCESS PIPE\n");
		goto oops;
	}

	qse_aio_dev_pro_write (pro, "MY AIO LIBRARY\n", 16, QSE_NULL);
//qse_aio_dev_pro_killchild (pro); 
//qse_aio_dev_pro_close (pro, QSE_AIO_DEV_PRO_IN); 
//qse_aio_dev_pro_close (pro, QSE_AIO_DEV_PRO_OUT); 
//qse_aio_dev_pro_close (pro, QSE_AIO_DEV_PRO_ERR); 
}

	qse_aio_loop (aio);

	g_aio = QSE_NULL;
	qse_aio_close (aio);
#if defined(USE_SSL)
	cleanup_openssl ();
#endif

	return 0;

oops:
	g_aio = QSE_NULL;
	qse_aio_close (aio);
#if defined(USE_SSL)
	cleanup_openssl ();
#endif
	return -1;
}
