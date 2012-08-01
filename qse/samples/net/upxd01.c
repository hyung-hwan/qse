#include <qse/net/upxd.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/time.h>
#include <qse/cmn/sio.h>

#include <signal.h>
#include <locale.h>
#include <string.h>
#include <stdlib.h>

#if defined(_WIN32)
#	include <windows.h>
#else
#	include <unistd.h>
#	include <errno.h>
#	include <fcntl.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <net/if.h>
#endif

#if defined(HAVE_EPOLL)
#	if defined(HAVE_SYS_EPOLL_H)
#		include <sys/epoll.h>
#	endif
#endif

/* ------------------------------------------------------------------- */
static qse_upxd_errnum_t syserr_to_errnum (int e)
{
	switch (e)
	{
		case ENOMEM:
			return QSE_UPXD_ENOMEM;

		case EINVAL:
			return QSE_UPXD_EINVAL;

		case EACCES:
		case ECONNREFUSED:
			return QSE_UPXD_EACCES;

		case ENOENT:
			return QSE_UPXD_ENOENT;

		case EEXIST:
			return QSE_UPXD_EEXIST;

		case EINTR:
			return QSE_UPXD_EINTR;

		case EAGAIN:
		/*case EWOULDBLOCK:*/
			return QSE_UPXD_EAGAIN;

		default:
			return QSE_UPXD_ESYSERR;
	}
}

/* ------------------------------------------------------------------- */

static int sockaddr_to_nwad (
	const struct sockaddr_storage* addr, qse_nwad_t* nwad)
{
	int addrsize = -1;

	switch (addr->ss_family)
	{
		case AF_INET:
		{
			struct sockaddr_in* in;
			in = (struct sockaddr_in*)addr;
			addrsize = QSE_SIZEOF(*in);

			memset (nwad, 0, QSE_SIZEOF(*nwad));
			nwad->type = QSE_NWAD_IN4;
			nwad->u.in4.addr.value = in->sin_addr.s_addr;
			nwad->u.in4.port = in->sin_port;
			break;
		}

#if defined(AF_INET6)
		case AF_INET6:
		{
			struct sockaddr_in6* in;
			in = (struct sockaddr_in6*)addr;
			addrsize = QSE_SIZEOF(*in);

			memset (nwad, 0, QSE_SIZEOF(*nwad));
			nwad->type = QSE_NWAD_IN6;
			memcpy (&nwad->u.in6.addr, &in->sin6_addr, QSE_SIZEOF(nwad->u.in6.addr));
			nwad->u.in6.scope = in->sin6_scope_id;
			nwad->u.in6.port = in->sin6_port;
			break;
		}
#endif
	}

	return addrsize;
}

static int nwad_to_sockaddr (
	const qse_nwad_t* nwad, struct sockaddr_storage* addr)
{
	int addrsize = -1;

	switch (nwad->type)
	{
		case QSE_NWAD_IN4:
		{
			struct sockaddr_in* in;

			in = (struct sockaddr_in*)addr;
			addrsize = QSE_SIZEOF(*in);
			memset (in, 0, addrsize);

			in->sin_family = AF_INET;
			in->sin_addr.s_addr = nwad->u.in4.addr.value;
			in->sin_port = nwad->u.in4.port;
			break;
		}

		case QSE_NWAD_IN6:
		{
#if defined(AF_INET6)
			struct sockaddr_in6* in;

			in = (struct sockaddr_in6*)addr;
			addrsize = QSE_SIZEOF(*in);
			memset (in, 0, addrsize);

			in->sin6_family = AF_INET6;
			memcpy (&in->sin6_addr, &nwad->u.in6.addr, QSE_SIZEOF(nwad->u.in6.addr));
			in->sin6_scope_id = nwad->u.in6.scope;
			in->sin6_port = nwad->u.in6.port;
#endif
			break;
		}
	}

	return addrsize;
}

/* ------------------------------------------------------------------- */

static int sock_open (qse_upxd_t* upxd, qse_upxd_sock_t* sock)
{
	int fd = -1, flag;
	int syserr = 1;

	struct sockaddr_storage addr;
	int addrsize;

	addrsize = nwad_to_sockaddr (&sock->bind, &addr);
	if (addrsize <= -1)
	{
		qse_upxd_seterrnum (upxd, QSE_UPXD_ENOIMPL);
		syserr = 0;
		goto oops;
	}

/* TODO: if AF_INET6 is not defined sockaddr_storage is not available...
 * create your own union or somehting similar... */

	fd = socket (addr.ss_family, SOCK_DGRAM, IPPROTO_UDP);
	if (fd <= -1) goto oops;

	flag = fcntl (fd, F_GETFD);
	if (flag >= 0) fcntl (fd, F_SETFD, flag | FD_CLOEXEC);

	if (bind (fd, (struct sockaddr*)&addr, addrsize) <= -1)
	{
#if defined(IPV6_V6ONLY)
		if (errno == EADDRINUSE && addr.ss_family == AF_INET6)
		{
			int on = 1;
			setsockopt (fd, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on));
			if (bind (fd, (struct sockaddr*)&addr, addrsize) <= -1)  goto oops;
		}
		else goto oops;
#else
		goto oops;
#endif
	}

	if (sock->dev)
	{
#if defined(SO_BINDTODEVICE)
		struct ifreq ifr;
		qse_size_t wsz, msz;

		memset (&ifr, 0, sizeof(ifr));

#if defined(QSE_CHAR_IS_MCHAR)
		qse_mbscpy (ifr.ifr_name, sock->dev, QSE_COUNTOF(ifr.ifr_name));
#else
		msz = QSE_COUNTOF(ifr.ifr_name);
		if (qse_wcstombs (sock->dev, &wsz, ifr.ifr_name, &msz) <= -1)
		{
			qse_upxd_seterrnum (upxd, QSE_UPXD_EINVAL);
			syserr = 0;
			goto oops;
		}
#endif
		if (setsockopt (fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, QSE_SIZEOF(ifr)) <= -1) goto oops;
#endif
	}

	flag = fcntl (fd, F_GETFL);
	if (flag >= 0) fcntl (fd, F_SETFL, flag | O_NONBLOCK);

	sock->handle.i = fd;
	return 0;

oops:
	if (syserr) qse_upxd_seterrnum (upxd, syserr_to_errnum(errno));
	if (fd >= 0) close (fd);
	return -1;
}

static void sock_close (qse_upxd_t* upxd, qse_upxd_sock_t* sock)
{
	close (sock->handle.i);
}

static qse_ssize_t sock_recv (
	qse_upxd_t* upxd, qse_upxd_sock_t* sock, void* buf, qse_size_t bufsize)
{
	ssize_t ret;
	struct sockaddr_storage addr;
	socklen_t addrsize;

	addrsize = QSE_SIZEOF(addr);
	ret = recvfrom (sock->handle.i, buf, bufsize, 0, (struct sockaddr*)&addr, &addrsize);
	if (ret <= -1) qse_upxd_seterrnum (upxd, syserr_to_errnum(errno));
	else
	{
		if (sockaddr_to_nwad (&addr, &sock->from) <= -1)
		{
			qse_upxd_seterrnum (upxd, QSE_UPXD_ENOIMPL);
			ret = -1;
		}
	}

	return ret;
}

static qse_ssize_t sock_send (
	qse_upxd_t* upxd, qse_upxd_sock_t* sock, const void* buf, qse_size_t bufsize)
{
	struct sockaddr_storage addr;
	int addrsize;
	ssize_t ret;

	addrsize = nwad_to_sockaddr (&sock->to, &addr);
	if (addrsize <= -1)
	{
		qse_upxd_seterrnum (upxd, QSE_UPXD_ENOIMPL);
		return -1;
	}

	ret = sendto (sock->handle.i, buf, bufsize,
	              0, (struct sockaddr*)&addr, addrsize);
	if (ret <= -1) qse_upxd_seterrnum (upxd, syserr_to_errnum(errno));
	return ret;
}
/* ------------------------------------------------------------------- */

static int session_config (
	qse_upxd_t* upxd, qse_upxd_session_t* session)
{
	/* you can check the source address (session->from).
	 * you can set the destination address.
	 * you can set the binding address.
	 * you can set the binding interface.
	 */
	qse_strtonwad (QSE_T("127.0.0.1:9991"), &session->config.peer);
	qse_strtonwad (QSE_T("0.0.0.0:0"), &session->config.bind);
	/*qse_strxcpy (session->config.dev, QSE_COUNTOF(session->config.dev), QSE_T("eth1"));*/
	session->config.dormancy = 10000;
	return 0;
}

static void session_error (
	qse_upxd_t* upxd, qse_upxd_session_t* session)
{
	if (session->server)
	{
	}
	else
	{
		/* session->local.nwad is not associated with a session. */
	}

	qse_printf (QSE_T("ERROR IN SESSION COMMUNICATION\n"));

}
/* ------------------------------------------------------------------- */

struct mux_ev_t
{
	qse_ubi_t handle;
	int reqmask;
	qse_upxd_muxcb_t cbfun;
	void* cbarg;
};

struct mux_t
{
	int fd;

	struct
	{
		struct epoll_event* ptr;
		qse_size_t len;
		qse_size_t capa;
	} ee;

	struct
	{
		struct mux_ev_t** ptr;
		qse_size_t        capa;
	} mev;
};

#define MUX_EV_ALIGN 64

static void* mux_open (qse_upxd_t* upxd)
{
	struct mux_t* mux;

	mux = qse_upxd_allocmem (upxd, QSE_SIZEOF(*mux));
	if (mux == QSE_NULL) return QSE_NULL;

	memset (mux, 0, QSE_SIZEOF(*mux));

#if defined(HAVE_EPOLL_CREATE1) && defined(O_CLOEXEC)
	mux->fd = epoll_create1 (O_CLOEXEC);
#else
	mux->fd = epoll_create (100);
#endif
	if (mux->fd <= -1)
	{
		qse_upxd_freemem (upxd, mux);
		qse_upxd_seterrnum (upxd, syserr_to_errnum(errno));
		return QSE_NULL;
	}

#if defined(HAVE_EPOLL_CREATE1) && defined(O_CLOEXEC)
	/* nothing else to do */
#else
	{
		int flag = fcntl (mux->fd, F_GETFD);
		if (flag >= 0) fcntl (mux->fd, F_SETFD, flag | FD_CLOEXEC);
	}
#endif

	return mux;
}

static void mux_close (qse_upxd_t* upxd, void* vmux)
{
	struct mux_t* mux = (struct mux_t*)vmux;
	if (mux->ee.ptr) qse_upxd_freemem (upxd, mux->ee.ptr);
	if (mux->mev.ptr)
	{
		qse_size_t i;
		for (i = 0; i < mux->mev.capa; i++)
			if (mux->mev.ptr[i]) qse_upxd_freemem (upxd, mux->mev.ptr[i]);
		qse_upxd_freemem (upxd, mux->mev.ptr);
	}
	close (mux->fd);
	qse_upxd_freemem (upxd, mux);
}

static int mux_addhnd (
	qse_upxd_t* upxd, void* vmux, qse_ubi_t handle,
	qse_upxd_muxcb_t cbfun, void* cbarg)
{
	struct mux_t* mux = (struct mux_t*)vmux;
	struct epoll_event ev;
	struct mux_ev_t* mev;

	ev.events = EPOLLIN; /* inspect IN and HUP only */

	if (handle.i >= mux->mev.capa)
	{
		struct mux_ev_t** tmp;
		qse_size_t tmpcapa, i;

		tmpcapa = (((handle.i + MUX_EV_ALIGN) / MUX_EV_ALIGN) * MUX_EV_ALIGN);

		tmp = (struct mux_ev_t**) qse_upxd_reallocmem (
			upxd, mux->mev.ptr,
			QSE_SIZEOF(*mux->mev.ptr) * tmpcapa);
		if (tmp == QSE_NULL) return -1;

		for (i = mux->mev.capa; i < tmpcapa; i++) tmp[i] = QSE_NULL;
		mux->mev.ptr = tmp;
		mux->mev.capa = tmpcapa;
	}

	if (mux->mev.ptr[handle.i] == QSE_NULL)
	{
		/* the location of the data passed to epoll_ctl()
		 * must not change unless i update the info with epoll()
		 * whenever there is reallocation. so i simply
		 * make mux-mev.ptr reallocatable but auctual
		 * data fixed once allocated. */
		mux->mev.ptr[handle.i] = qse_upxd_allocmem (
			upxd, QSE_SIZEOF(*mux->mev.ptr[handle.i]));
		if (mux->mev.ptr[handle.i] == QSE_NULL) return -1;
	}

	if (mux->ee.len >= mux->ee.capa)
	{
		struct epoll_event* tmp;

		tmp = qse_upxd_reallocmem (
			upxd, mux->ee.ptr,
			QSE_SIZEOF(*mux->ee.ptr) * (mux->ee.capa + 1) * 2);
		if (tmp == QSE_NULL) return -1;

		mux->ee.ptr = tmp;
		mux->ee.capa = (mux->ee.capa + 1) * 2;
	}

	mev = mux->mev.ptr[handle.i];
	mev->handle = handle;
	mev->cbfun = cbfun;
	mev->cbarg = cbarg;

	ev.data.ptr = mev;

	if (epoll_ctl (mux->fd, EPOLL_CTL_ADD, handle.i, &ev) <= -1)
	{
		/* don't rollback ee.ptr */
		qse_upxd_seterrnum (upxd, syserr_to_errnum(errno));
		return -1;
	}

	mux->ee.len++;
	return 0;
}

static int mux_delhnd (qse_upxd_t* upxd, void* vmux, qse_ubi_t handle)
{
	struct mux_t* mux = (struct mux_t*)vmux;

	if (epoll_ctl (mux->fd, EPOLL_CTL_DEL, handle.i, QSE_NULL) <= -1)
	{
		qse_upxd_seterrnum (upxd, syserr_to_errnum(errno));
		return -1;
	}

	mux->ee.len--;
	return 0;
}

static int mux_poll (qse_upxd_t* upxd, void* vmux, qse_ntime_t timeout)
{
	struct mux_t* mux = (struct mux_t*)vmux;
	struct mux_ev_t* mev;
	int nfds, i;

	if (mux->ee.len < 0)
	{
		/* nothing to monitor yet */
		sleep (timeout / 1000);
	}
	else
	{
		nfds = epoll_wait (mux->fd, mux->ee.ptr, mux->ee.len, timeout);
		if (nfds <= -1)
		{
			qse_upxd_seterrnum (upxd, syserr_to_errnum(errno));
			return -1;
		}

		for (i = 0; i < nfds; i++)
		{
			mev = mux->ee.ptr[i].data.ptr;

			if (mux->ee.ptr[i].events & (EPOLLIN | EPOLLHUP))
				mev->cbfun (upxd, mux, mev->handle, mev->cbarg);
		}
	}
	
	return 0;
}

/* ------------------------------------------------------------------- */
static qse_upxd_cbs_t upxd_cbs =
{
	/* socket */
	{ sock_open, sock_close, sock_recv, sock_send },

	/* session */
	{ session_config, session_error },

	/* multiplexer */
	{ mux_open, mux_close, mux_addhnd, mux_delhnd, mux_poll }
};


/* ------------------------------------------------------------------- */

typedef struct tr_t tr_t;
struct tr_t
{
	unsigned int line;
	qse_sio_t* sio;
	qse_str_t* t;
	unsigned int tl;
	qse_char_t last;
};

tr_t* tr_open (const qse_char_t* name)
{
	tr_t* tr;

	tr = malloc (QSE_SIZEOF(*tr));
	if (tr == QSE_NULL) return QSE_NULL;

	memset (tr, 0, sizeof(*tr));
	tr->line = 1;
	
	tr->sio = qse_sio_open (QSE_MMGR_GETDFL(), 0, name, QSE_SIO_READ);
	if (tr->sio == QSE_NULL) 
	{
		free (tr);
		return QSE_NULL;
	}

	tr->t = qse_str_open (QSE_MMGR_GETDFL(), 0, 128);
	if (tr->t == QSE_NULL)
	{
		qse_sio_close (tr->sio);
		free (tr);
		return QSE_NULL;
	}

	return tr;
}

void tr_close (tr_t* tr)
{
	qse_str_close (tr->t);
	qse_sio_close (tr->sio);
	free (tr);	
}

qse_char_t* tr_getnext (tr_t* tr)
{
	qse_char_t c;

	qse_str_clear (tr->t);

	if (tr->last)
	{
		tr->tl = tr->line;
		if (qse_str_ccat (tr->t, tr->last) == (qse_ssize_t)-1) return QSE_NULL;
		tr->last = 0;
	}
	else
	{
		/* skip spaces */
		while (1)
		{
			if (qse_sio_getc (tr->sio, &c) <= -1) return QSE_NULL;
			if (c == QSE_CHAR_EOF) return QSE_NULL;
			if (c == QSE_T('\n')) tr->line++; 
			if (!QSE_ISSPACE(c)) break;
		}

		tr->tl = tr->line;
		
		if (c == QSE_T(';') || c == QSE_T('{') || c == QSE_T('}')) 
		{
			if (qse_str_ccat (tr->t, c) == (qse_ssize_t)-1) return QSE_NULL;
		}
		else
		{
			do
			{
				if (qse_str_ccat (tr->t, c) == (qse_ssize_t)-1) return QSE_NULL;
				if (qse_sio_getc (tr->sio, &c) <= -1) return QSE_NULL;
		
				if (c == QSE_CHAR_EOF) break;
				
				if (c == QSE_T('\n'))  tr->line++; 
				if (QSE_ISSPACE(c)) break;
				else if (c == QSE_T(';') || c == QSE_T('{') || c == QSE_T('}')) 
				{
					tr->last = c;
					break;
				}
			}
			while (1);
		}
	}

	return QSE_STR_PTR(tr->t);
}
 
/* ------------------------------------------------------------------- */

struct cfg_rule_t
{
	struct
	{
		qse_ipad_t ipad;
		qse_ipad_t mask;
	} src;

	int action; /* DROP, FORWARD */

	struct
	{
		qse_nwad_t via;
		qse_char_t via_dev[64];
		qse_nwad_t to_nwad;
	} fwd;
};

typedef struct cfg_listen_t cfg_listen_t;
struct cfg_listen_t
{
	qse_nwad_t nwad;
	qse_char_t dev[64];
	cfg_rule_t* rule;
};

typedef struct cfg_t cfg_t;
struct cfg_t
{
	cfg_listen_t* list;
};

cfg_t* cfg_open (void)
{
	cfg_t* cfg;
	
	cfg = malloc (QSE_SIZEOF(*cfg));
	if (cfg == QSE_NULL) return QSE_NULL;
	
	return cfg;
}

void cfg_close (cfg_t*)
{
	free (cfg);
}

cfg_listen_t* cfg_addlisten (cfg_t* cfg, qse_nwad_t nwad, const qse_char_t* dev)
{
	cfg_listen_t* ptr;
	
	ptr = malloc (QSE_SIZEOF(*ptr));
	if (ptr == QSE_NULL) return QSE_NULL;
	
	ptr->nwad = nwad;
	qse_strxcpy (ptr->dev, QSE_COUNTOF(ptr->dev), dev);
	ptr->rule = QSE_NULL;
	
	return ptr;
} 

static cfg_t* load_cfg (const qse_char_t* name)
{
	tr_t* tr;
	const qse_char_t* t;
	cfg_t* cfg;
	cfg_listen_t cfglis;
	qse_nwad_t nwad;

	cfg = malloc (QSE_SIZEOF(*cfg));
	if (cfg == QSE_NULL) return QSE_NULL;
	
	tr = tr_open (name);
	if (tr == QSE_NULL) goto oops;

	do
	{
		t = tr_getnext(tr);
		if (t == QSE_NULL) break;
		
		if (qse_strcmp (t, QSE_T("listen")) == 0)
		{
			tmp = tr_getnext(tr);
			if (tmp == QSE_NULL || qse_strtonwad (tmp, &nwad) <= -1)
			{
				qse_printf (QSE_T("line %u: ipaddr:port expected after 'listen'\n"), (unsigned int)tr->tl);
				goto oops;
			}
			
			tmp = tr_getnext (tr);
			if (tmp == QSE_NULL)
			{
				qse_printf (QSE_T("line %u: 'dev' or { expected\n"), (unsigned int)tr->tl);
				goto oops;
			}
			
			if (qse_strcmp (tmp, QSE_T("dev")) == 0)
			{
				tmp = tr_getnext (tr);
				if (tmp == QSE_NULL)
				{
					qse_printf (QSE_T("line %u: device name expected\n"), (unsigned int)tr->tl);
					goto oops;
				}
				
				if (cfg_addlisten (cfg, &nwad, tmp) <= -1)
				{
					qse_printf (QSE_T("line %u: failed to add a new service\n"), (unsigned int)tr->tl);
					goto oops;
				}
				
				tmp = tr_getnext (tr);
				if (tmp == QSE_NULL)
				{
					qse_printf (QSE_T("line %u: { expected\n"), (unsigned int)tr->tl);
					goto oops;
				}
			}
			
			if (qse_strcmp (tmp, QSE_T('{')) != 0)
			{
				qse_printf (QSE_T("line %u: { expected\n"), (unsigned int)tr->tl);
				goto oops;
			}


			while (1)
			{
				tmp = tr_getnext (tr);
				if (tmp == QSE_NULL)
				{
					qse_printf (QSE_T("line %u: } expected\n"), (unsigned int)tr->tl);
					goto oops;
				}

				if (qse_strcmp (tmp, QSE_T("}")) == 0) break;
				
				if (qse_strcmp (tmp, QSE_T("from")) != 0)
				{
					qse_printf (QSE_T("line %u: 'from' expected\n"), (unsigned int)tr->tl);
				}

				tmp = tr_getnext (tr);


				if (qse_strcmp (action, QSE_T("drop")) == 0)
				{
				}
				else if (qse_strcmp (action, QSE_T("forward")) == 0)
				{
				}
				else
				{
				}

				tmp = tr_getnext (tr);
				if (qse_strcmp (tmp, QSE_T(";")) != 0)
				{
				}
			}

			tmp = tr_getnext (tr);
			if (qse_strcmp (tmp, QSE_T("}")) != 0)
			{
			}
		}
		else 
		{
			qse_printf (QSE_T("line %u: 'listen' expected\n"), (unsigned int)tr->tl);
			goto oops;
		}
	}
	while (1);

	return cfg;
	
oops:
	if (tr) tr_close (tr);
	return QSE_NULL;
}

static void free_cfg (cfg_t* cfg)
{
}


/* ------------------------------------------- */


static qse_upxd_t* g_upxd = QSE_NULL;

static void sigint (int sig)
{
	if (g_upxd) qse_upxd_stop (g_upxd);
}

int upxd_main (int argc, qse_char_t* argv[])
{
	qse_upxd_t* upxd = QSE_NULL;
	cfg_t* cfg = QSE_NULL;
	int ret = -1, i;

	if (argc <= 1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Usage: %s <server-address> ...\n"), argv[0]);
		goto oops;
	}

	cfg = load_cfg (argv[1]);
	if (cfg == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Error: Cannot load %s\n"), argv[1]);
		goto oops;
	}

	upxd = qse_upxd_open (QSE_MMGR_GETDFL(), 0);
	if (upxd == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Cannot open upxd\n"));
		goto oops;
	}

	for (i = 1; i < argc; i++)
	{
		qse_nwad_t nwad;
		if (qse_strtonwad (argv[i], &nwad) <= -1)
		{
			qse_fprintf (QSE_STDERR,
				QSE_T("Wrong server - %s\n"), argv[i]);
			goto oops;
		}

		if (qse_upxd_addserver (upxd, &nwad, QSE_NULL) == QSE_NULL)
		{
			qse_fprintf (QSE_STDERR,
				QSE_T("Failed to add server - %s\n"), argv[i]);
			goto oops;
		}
	}

	g_upxd = upxd;
	signal (SIGINT, sigint);
	signal (SIGPIPE, SIG_IGN);

	qse_upxd_setcbs (upxd, &upxd_cbs);

	ret = qse_upxd_loop (upxd, 5000);

	signal (SIGINT, SIG_DFL);
	signal (SIGPIPE, SIG_DFL);
	g_upxd = QSE_NULL;

	if (ret <= -1) 
		qse_fprintf (QSE_STDERR, QSE_T("Error - %d\n"), (int)qse_upxd_geterrnum(upxd));

oops:
	if (upxd) qse_upxd_close (upxd);
	if (cfg) free_cfg (cfg);
	return ret;
}

int qse_main (int argc, qse_achar_t* argv[])
{
#if defined(_WIN32)
	char locale[100];
	UINT codepage = GetConsoleOutputCP();
	if (codepage == CP_UTF8)
	{
		/*SetConsoleOUtputCP (CP_UTF8);*/
		qse_setdflcmgr (qse_utf8cmgr);
	}
	else
	{
		sprintf (locale, ".%u", (unsigned int)codepage);
		setlocale (LC_ALL, locale);
		qse_setdflcmgr (qse_slmbcmgr);
	}
#else
	setlocale (LC_ALL, "");
	qse_setdflcmgr (qse_slmbcmgr);
#endif

	return qse_runmain (argc, argv, upxd_main);
}

