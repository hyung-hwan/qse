/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

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

#include "mod-sys.h"
#include "val.h" /* QSE_AWK_QUICKINT_MAX.. need to exclude this line if the items gets available in qse/awk/awk.h */
#include <qse/cmn/str.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/time.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/rbt.h>
#include <qse/si/nwad.h>
#include <qse/si/nwif.h>
#include <qse/si/sio.h>
#include "../cmn/mem-prv.h"

#if defined(_WIN32)
#	include <windows.h>
#	include <process.h>
#	include <tchar.h>
#elif defined(__OS2__)
#	define INCL_DOSPROCESS
#	define INCL_DOSEXCEPTIONS
#	define INCL_ERRORS
#	include <os2.h>
#elif defined(__DOS__)
#	include <dos.h>
#else
#	include "../cmn/syscall.h"
#	if defined(HAVE_SYS_SYSCALL_H)
#		include <sys/syscall.h>
#	endif

#	define ENABLE_SYSLOG
#	include <syslog.h>

#	include <sys/socket.h>

#endif

#include <stdlib.h> /* getenv, system */
#include <time.h>
#include <errno.h>
#include <string.h>


#define DEFAULT_MODE (0777)
/* ------------------------------------------------------------------------ */

typedef enum syslog_type_t syslog_type_t;
enum syslog_type_t
{
	SYSLOG_LOCAL,
	SYSLOG_REMOTE
};

struct mod_ctx_t
{
	qse_rbt_t* rtxtab;

	struct
	{
		syslog_type_t type;
		char* ident;
		qse_skad_t skad;
		int syslog_opened; // has openlog() been called?
		int opt;
		int fac;
		int sck;
		qse_mbs_t* dmsgbuf;
	} log;
};
typedef struct mod_ctx_t mod_ctx_t;

/* ------------------------------------------------------------------------ */

struct sys_node_data_t
{
	int fd;
};
typedef struct sys_node_data_t sys_node_data_t;

struct sys_list_data_t
{
	qse_char_t errmsg[256];
	qse_mchar_t* readbuf;
	qse_size_t readbuf_capa;
};
typedef struct sys_list_data_t sys_list_data_t;

#define __IMAP_NODE_T_DATA sys_node_data_t ctx;
#define __IMAP_LIST_T_DATA sys_list_data_t ctx;
#define __IMAP_LIST_T sys_list_t
#define __IMAP_NODE_T sys_node_t
#define __MAKE_IMAP_NODE __new_sys_node
#define __FREE_IMAP_NODE __free_sys_node
#include "imap-imp.h"

struct rtx_data_t
{
	sys_list_t sys_list;
};
typedef struct rtx_data_t rtx_data_t;

/* ------------------------------------------------------------------------ */

static void set_error_on_sys_list (qse_awk_rtx_t* rtx, sys_list_t* sys_list, const qse_char_t* errfmt, ...)
{
	if (errfmt)
	{
		va_list ap;
		va_start (ap, errfmt);
		qse_strxvfmt (sys_list->ctx.errmsg, QSE_COUNTOF(sys_list->ctx.errmsg), errfmt, ap);
		va_end (ap);
	}
	else
	{
		qse_strxcpy (sys_list->ctx.errmsg, QSE_COUNTOF(sys_list->ctx.errmsg), qse_awk_rtx_geterrmsg(rtx));
	}
}

static QSE_INLINE void set_error_on_sys_list_with_syserr (qse_awk_rtx_t* rtx, sys_list_t* sys_list)
{
	set_error_on_sys_list (rtx, sys_list, QSE_T("%hs"), strerror(errno));
}

/* ------------------------------------------------------------------------ */

static sys_node_t* new_sys_node (qse_awk_rtx_t* rtx, sys_list_t* list, int fd)
{
	sys_node_t* node;

	node = __new_sys_node(rtx, list);
	if (!node) return QSE_NULL;

	node->ctx.fd = fd;
	return node;
}

static void free_sys_node (qse_awk_rtx_t* rtx, sys_list_t* list, sys_node_t* node)
{
	if (node->ctx.fd >= 0) 
	{
		close (node->ctx.fd);
		node->ctx.fd = -1;
	}
	__free_sys_node (rtx, list, node);
}

/* ------------------------------------------------------------------------ */

static QSE_INLINE sys_list_t* rtx_to_sys_list (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	mod_ctx_t* mctx = (mod_ctx_t*)fi->mod->ctx;
	qse_rbt_pair_t* pair;

	pair = qse_rbt_search(mctx->rtxtab, &rtx, QSE_SIZEOF(rtx));
	QSE_ASSERT (pair != QSE_NULL);
	return (sys_list_t*)QSE_RBT_VPTR(pair);
}

static QSE_INLINE sys_node_t* get_sys_list_node (sys_list_t* sys_list, qse_awk_int_t id)
{
	if (id < 0 || id >= sys_list->map.high || !sys_list->map.tab[id]) return QSE_NULL;
	return sys_list->map.tab[id];
}

/* ------------------------------------------------------------------------ */


static sys_node_t* get_sys_list_node_with_arg (qse_awk_rtx_t* rtx, sys_list_t* sys_list, qse_awk_val_t* arg)
{
	qse_awk_int_t id;
	sys_node_t* sys_node;

	if (qse_awk_rtx_valtoint(rtx, arg, &id) <= -1)
	{
		set_error_on_sys_list (rtx, sys_list, QSE_T("illegal instance id"));
		return QSE_NULL;
	}
	else if (!(sys_node = get_sys_list_node(sys_list, id)))
	{
		set_error_on_sys_list (rtx, sys_list, QSE_T("invalid instance id - %zd"), (qse_size_t)id);
		return QSE_NULL;
	}

	return sys_node;
}

static int fnc_close (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	sys_list_t* sys_list;
	sys_node_t* sys_node;
	int ret = -1;

	sys_list = rtx_to_sys_list(rtx, fi);
	sys_node = get_sys_list_node_with_arg(rtx, sys_list, qse_awk_rtx_getarg(rtx, 0));
	if (sys_node)
	{
		free_sys_node (rtx, sys_list, sys_node);
		ret = 0;
	}

	qse_awk_rtx_setretval (rtx, qse_awk_rtx_makeintval(rtx, ret));
	return 0;
}

/*
  BEGIN {
     f = sys::open ("/tmp/test.txt", sys::O_RDONLY);
     while (sys::read(f, x, 10) > 0) printf (B"%s", x);
     sys::close (f);
  }
*/

static int fnc_open (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	sys_list_t* sys_list;
	sys_node_t* sys_node = QSE_NULL;
	qse_awk_int_t ret = -1, flags = 0, mode = DEFAULT_MODE;
	int fd = -1;
	qse_mchar_t* pstr;
	qse_size_t plen;
	qse_awk_val_t* a0;
	qse_awk_val_t* retv;

	sys_list = rtx_to_sys_list(rtx, fi);

	if (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 1), &flags) <= -1 || flags < 0) flags = O_RDONLY;
	if (qse_awk_rtx_getnargs(rtx) >= 3 && (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 2), &mode) <= -1 || mode < 0)) mode = DEFAULT_MODE;

#if defined(O_LARGEFILE)
	flags |= O_LARGEFILE;
#endif

	a0 = qse_awk_rtx_getarg(rtx, 0);
	pstr = qse_awk_rtx_getvalmbs(rtx, a0, &plen);
	if (!pstr) goto fail;
	fd = open(pstr, flags, mode);
	qse_awk_rtx_freevalmbs (rtx, a0, pstr);

	if (fd >= 0)
	{
		sys_node = new_sys_node(rtx, sys_list, fd);
		if (sys_node) 
		{
			ret = sys_node->id;
		}
		else 
		{
		fail:
			set_error_on_sys_list (rtx, sys_list, QSE_NULL);
		}
	}
	else
	{
		set_error_on_sys_list_with_syserr (rtx, sys_list);
	}

	/* ret may not be a statically managed number. 
	 * error checking is required */
	retv = qse_awk_rtx_makeintval(rtx, ret);
	if (retv == QSE_NULL)
	{
		if (sys_node) free_sys_node (rtx, sys_list, sys_node);
		return -1;
	}

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_read (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	sys_list_t* sys_list;
	sys_node_t* sys_node;
	qse_awk_int_t ret = -1;
	qse_awk_int_t reqsize = 8192;

	if (qse_awk_rtx_getnargs(rtx) >= 3 && (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 2), &reqsize) <= -1 || reqsize <= 0)) reqsize = 8192;
	if (reqsize > QSE_AWK_QUICKINT_MAX) reqsize = QSE_AWK_QUICKINT_MAX;

	sys_list = rtx_to_sys_list(rtx, fi);
	sys_node = get_sys_list_node_with_arg(rtx, sys_list, qse_awk_rtx_getarg(rtx, 0));
	if (sys_node)
	{
		if (reqsize > sys_list->ctx.readbuf_capa)
		{
			qse_mchar_t* tmp = qse_awk_rtx_reallocmem(rtx, sys_list->ctx.readbuf, reqsize);
			if (!tmp)
			{
				set_error_on_sys_list (rtx, sys_list, QSE_NULL);
				goto done;
			}
			sys_list->ctx.readbuf = tmp;
			sys_list->ctx.readbuf_capa = reqsize;
		}

		ret = read(sys_node->ctx.fd, sys_list->ctx.readbuf, reqsize);
		if (ret <= 0) 
		{
			if (ret <= -1) 
			{
				if (errno == EINTR) ret = -3;
			#if defined(EAGAIN) && defined(EWOULDBLOCK) && (EAGAIN == EWOULDBLOCK)
				else if (errno == EAGAIN) ret = -2;
			#elif defined(EAGAIN) && defined(EWOULDBLOCK) && (EAGAIN != EWOULDBLOCK)
				else if (errno == EAGAIN || errno == EWOULDBLOCK) ret = -2;
			#elif defined(EAGAIN)
				else if (errno == EAGAIN) ret = -2;
			#elif defined(EWOULDBLOCK)
				else if (errno == EWOULDBLOCK) ret = -2;
			#endif
				set_error_on_sys_list_with_syserr(rtx, sys_list);
			}
			goto done;
		}

		if (ret > 0)
		{
			qse_awk_val_t* sv;
			int x;

			sv = qse_awk_rtx_makembsval(rtx, sys_list->ctx.readbuf, ret);
			if (!sv) return -1;

			qse_awk_rtx_refupval (rtx, sv);
			x = qse_awk_rtx_setrefval(rtx, (qse_awk_val_ref_t*)qse_awk_rtx_getarg(rtx, 1), sv);
			qse_awk_rtx_refdownval (rtx, sv);
			if (x <= -1) return -1;
		}
	}

done:
	/* the value in 'ret' never exceeds QSE_AWK_QUICKINT_MAX as 'reqsize' has been limited to
	 * it before the call to 'read'. so it's safe not to check the result of qse_awk_rtx_makeintval() */
	qse_awk_rtx_setretval (rtx, qse_awk_rtx_makeintval(rtx, ret));
	return 0;
}

static int fnc_write (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	sys_list_t* sys_list;
	sys_node_t* sys_node;
	qse_awk_int_t ret = -1;

	sys_list = rtx_to_sys_list(rtx, fi);
	sys_node = get_sys_list_node_with_arg(rtx, sys_list, qse_awk_rtx_getarg(rtx, 0));
	if (sys_node)
	{
		qse_mchar_t* dptr;
		qse_size_t dlen;
		qse_awk_val_t* a1;

		a1 = qse_awk_rtx_getarg(rtx, 1);
		dptr = qse_awk_rtx_getvalmbs(rtx, a1, &dlen);
		if (dptr)
		{
			ret = write(sys_node->ctx.fd, dptr, dlen);
			if (ret <= -1) 
			{
				if (errno == EINTR) ret = -3;
			#if defined(EAGAIN) && defined(EWOULDBLOCK) && (EAGAIN == EWOULDBLOCK)
				else if (errno == EAGAIN) ret = -2;
			#elif defined(EAGAIN) && defined(EWOULDBLOCK) && (EAGAIN != EWOULDBLOCK)
				else if (errno == EAGAIN || errno == EWOULDBLOCK) ret = -2;
			#elif defined(EAGAIN)
				else if (errno == EAGAIN) ret = -2;
			#elif defined(EWOULDBLOCK)
				else if (errno == EWOULDBLOCK) ret = -2;
			#endif
				set_error_on_sys_list_with_syserr(rtx, sys_list);
			}
			qse_awk_rtx_freevalmbs (rtx, a1, dptr);
		}
		else
		{
			set_error_on_sys_list (rtx, sys_list, QSE_NULL);
		}
	}

	qse_awk_rtx_setretval (rtx, qse_awk_rtx_makeintval(rtx, ret));
	return 0;
}

/* ------------------------------------------------------------------------ */

/*
	##if (sys::pipe(p0, p1) <= -1)
	if (sys::pipe(p0, p1, sys::O_NONBLOCK | sys::O_CLOEXEC) <= -1)
	{
		 print "pipe error";
		 return -1;
	}
	a = sys::fork();
	if (a <= -1)
	{
		print "fork error";
		sys::close (p0);
		sys::close (p1);
	}
	else if (a == 0)
	{
		## child
		printf ("child.... %d %d %d\n", sys::getpid(), p0, p1);
		sys::close (p1);
		while (1)
		{
			n = sys::read (p0, k, 3);
			if (n <= 0)
			{
				if (n == -2) continue; ## nonblock but data not available
				if (n <= -1) print "ERROR: " sys::errmsg();
				break;
			}
			print k;
		}
		sys::close (p0);
	}
	else
	{
		## parent
		printf ("parent.... %d %d %d\n", sys::getpid(), p0, p1);
		sys::close (p0);
		sys::write (p1, B"hello");
		sys::write (p1, B"world");
		sys::close (p1);
		sys::wait(a);
	}
*/

static int fnc_pipe (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	/* create low-level pipes */

	sys_list_t* sys_list;
	sys_node_t* sys_node;
	int ret = -1;
	int fds[2];
	qse_awk_int_t flags = 0;

	sys_list = rtx_to_sys_list(rtx, fi);

	if (qse_awk_rtx_getnargs(rtx) >= 3 && (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 2), &flags) <= -1 || flags < 0)) flags = 0;

#if defined(HAVE_PIPE2)
	if (pipe2(fds, flags) >= 0)
#else
	if (pipe(fds) >= 0)
#endif
	{
		sys_node_t* node1, * node2;

	#if defined(HAVE_PIPE2)
		/* do nothing extra */
	#else
		if (flags > 0)
		{
			int nflags = 0;

			/* needs translation from O_XXXX to FD_XXXX */
		#if defined(O_CLOEXEC) && defined(FD_CLOEXEC)
			if (flags & O_CLOEXEC) nflags |= FD_CLOEXEC;
		#endif
		#if defined(O_NONBLOCK) && defined(FD_NONBLOCK)
			if (flags & O_NONBLOCK) nflags |= FD_NONBLOCK;
		#endif

			if (nflags > 0)
			{
				fcntl (fds[0], F_SETFD, nflags);
				fcntl (fds[1], F_SETFD, nflags);
			}
		}
	#endif
		node1 = new_sys_node(rtx, sys_list, fds[0]);
		node2 = new_sys_node(rtx, sys_list, fds[1]);
		if (node1 && node2)
		{
			qse_awk_val_t* v;
			int x;

			v = qse_awk_rtx_makeintval(rtx, node1->id);
			qse_awk_rtx_refupval (rtx, v);
			x = qse_awk_rtx_setrefval (rtx, (qse_awk_val_ref_t*)qse_awk_rtx_getarg(rtx, 0), v);
			qse_awk_rtx_refdownval (rtx, v);
			if (x <= -1) return -1;

			v = qse_awk_rtx_makeintval(rtx, node2->id);
			qse_awk_rtx_refupval (rtx, v);
			x = qse_awk_rtx_setrefval (rtx, (qse_awk_val_ref_t*)qse_awk_rtx_getarg(rtx, 1), v);
			qse_awk_rtx_refdownval (rtx, v);
			if (x <= -1) return -1;

			ret = 0;
		}
		else
		{
			set_error_on_sys_list (rtx, sys_list, QSE_NULL);
			if (node2) free_sys_node (rtx, sys_list, node2);
			if (node1) free_sys_node (rtx, sys_list, node1);
		}
	}
	else
	{
		set_error_on_sys_list_with_syserr (rtx, sys_list);
	}

	qse_awk_rtx_setretval (rtx, qse_awk_rtx_makeintval(rtx, ret));
	return 0;
}

/* ------------------------------------------------------------------------ */

static int fnc_errmsg (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	sys_list_t* sys_list;
	qse_awk_val_t* retv;

	sys_list = rtx_to_sys_list(rtx, fi);
	retv = qse_awk_rtx_makestrvalwithstr(rtx, sys_list->ctx.errmsg);
	if (!retv) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

/* ------------------------------------------------------------------------ */

static int fnc_fork (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t pid;
	qse_awk_val_t* retv;

#if defined(_WIN32)
	/* TOOD: implement this*/
	pid = -1;
	set_error_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), QSE_T("not implemented"));
#elif defined(__OS2__)
	/* TOOD: implement this*/
	pid = -1;
	set_error_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), QSE_T("not implemented"));
	
#elif defined(__DOS__)
	/* TOOD: implement this*/
	pid = -1;
	set_error_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), QSE_T("not implemented"));

#else
	pid = fork ();
	if (pid <= -1) set_error_on_sys_list_with_syserr (rtx, rtx_to_sys_list(rtx, fi));
#endif

	retv = qse_awk_rtx_makeintval(rtx, pid);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_wait (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t pid;
	qse_awk_val_t* retv;
	int rx;
	qse_size_t nargs;
	qse_awk_int_t opts = 0;
	int status;

	nargs = qse_awk_rtx_getnargs(rtx);
	if (nargs >= 3)
	{
		if (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 2), &opts) <= -1) return -1;
	}

	rx = qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 0), &pid);
	if (rx >= 0)
	{
#if defined(_WIN32)
		/* TOOD: implement this*/
		rx = -1;
		status = 0;
		set_error_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), QSE_T("not implemented"));
#elif defined(__OS2__)
		/* TOOD: implement this*/
		rx = -1;
		status = 0;
		set_error_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), QSE_T("not implemented"));
#elif defined(__DOS__)
		/* TOOD: implement this*/
		rx = -1;
		status = 0;
		set_error_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), QSE_T("not implemented"));
#else
		rx = waitpid(pid, &status, opts);
		if (rx <= -1) set_error_on_sys_list_with_syserr (rtx, rtx_to_sys_list(rtx, fi));
#endif
	}

	retv = qse_awk_rtx_makeintval(rtx, rx);
	if (!retv) return -1;

	if (nargs >= 2)
	{
		qse_awk_val_t* sv;
		int x;

		sv = qse_awk_rtx_makeintval(rtx, status);
		if (!sv) return -1;

		qse_awk_rtx_refupval (rtx, sv);
		x = qse_awk_rtx_setrefval(rtx, (qse_awk_val_ref_t*)qse_awk_rtx_getarg(rtx, 1), sv);
		qse_awk_rtx_refdownval (rtx, sv);
		if (x <= -1)  
		{
			qse_awk_rtx_freemem (rtx, retv);
			return -1;
		}
	}

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_wifexited (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t wstatus;
	qse_awk_val_t* retv;
	if (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 0), &wstatus) <= -1) return -1;
	retv = qse_awk_rtx_makeintval(rtx, WIFEXITED(wstatus));
	if (!retv) return -1;
	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_wexitstatus (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t wstatus;
	qse_awk_val_t* retv;
	if (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 0), &wstatus) <= -1) return -1;
	retv = qse_awk_rtx_makeintval(rtx, WEXITSTATUS(wstatus));
	if (!retv) return -1;
	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_wifsignaled (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t wstatus;
	qse_awk_val_t* retv;
	if (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 0), &wstatus) <= -1) return -1;
	retv = qse_awk_rtx_makeintval(rtx, WIFSIGNALED(wstatus));
	if (!retv) return -1;
	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_wtermsig (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t wstatus;
	qse_awk_val_t* retv;
	if (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 0), &wstatus) <= -1) return -1;
	retv = qse_awk_rtx_makeintval(rtx, WTERMSIG(wstatus));
	if (!retv) return -1;
	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_wcoredump (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t wstatus;
	qse_awk_val_t* retv;
	if (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 0), &wstatus) <= -1) return -1;
	retv = qse_awk_rtx_makeintval(rtx, WCOREDUMP(wstatus));
	if (!retv) return -1;
	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_kill (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t pid, sig;
	qse_awk_val_t* retv;
	int rx;

	if (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg (rtx, 0), &pid) <= -1 ||
	    qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg (rtx, 1), &sig) <= -1)
	{
		rx = -1;
	}
	else
	{
#if defined(_WIN32)
		/* TOOD: implement this*/
		rx = -1;
		set_error_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), QSE_T("not implemented"));
#elif defined(__OS2__)
		/* TOOD: implement this*/
		rx = -1;
		set_error_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), QSE_T("not implemented"));
#elif defined(__DOS__)
		/* TOOD: implement this*/
		rx = -1;
		set_error_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), QSE_T("not implemented"));
#else
		rx = kill(pid, sig);
		if (rx <= -1) set_error_on_sys_list_with_syserr (rtx, rtx_to_sys_list(rtx, fi));
#endif
	}

	retv = qse_awk_rtx_makeintval (rtx, rx);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_getpgid (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t pid;
	qse_awk_val_t* retv;

#if defined(_WIN32)
	/* TOOD: implement this*/
	pid = -1;
	set_error_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), QSE_T("not implemented"));
#elif defined(__OS2__)
	/* TOOD: implement this*/
	pid = -1;
	set_error_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), QSE_T("not implemented"));
#elif defined(__DOS__)
	/* TOOD: implement this*/
	pid = -1;
	set_error_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), QSE_T("not implemented"));
#else
	/* TODO: support specifing calling process id other than 0 */
	#if defined(HAVE_GETPGID)
	pid = getpgid(0);
	if (pid <= -1) set_error_on_sys_list_with_syserr (rtx, rtx_to_sys_list(rtx, fi));
	#elif defined(HAVE_GETPGRP)
	pid = getpgrp();
	if (pid <= -1) set_error_on_sys_list_with_syserr (rtx, rtx_to_sys_list(rtx, fi));
	#else
	pid = -1;
	set_error_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), QSE_T("not supported"));
	#endif
#endif

	retv = qse_awk_rtx_makeintval (rtx, pid);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_getpid (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t pid;
	qse_awk_val_t* retv;

#if defined(_WIN32)
	pid = GetCurrentProcessId();

#elif defined(__OS2__)
	PTIB tib;
	PPIB pib;

	pid = (DosGetInfoBlocks(&tib, &pib) == NO_ERROR)? pib->pib_ulpid: -1;

#elif defined(__DOS__)
	/* TOOD: implement this*/
	pid = -1;
	set_error_on_sys_list (rtx, rtx_to_sys_list(rtx, fi), QSE_T("not implemented"));

#else
	pid = getpid ();
	/* getpid() never fails */
#endif

	retv = qse_awk_rtx_makeintval (rtx, pid);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_gettid (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_intptr_t pid;
	qse_awk_val_t* retv;

#if defined(_WIN32)
	pid = GetCurrentThreadId();
	
#elif defined(__OS2__)
	PTIB tib;
	PPIB pib;

	pid = (DosGetInfoBlocks (&tib, &pib) == NO_ERROR && tib->tib_ptib2)?
		 tib->tib_ptib2->tib2_ultid: -1;
	
#elif defined(__DOS__)
	/* TOOD: implement this*/
	pid = -1;

#else
	#if defined(SYS_gettid) && defined(QSE_SYSCALL0)
	QSE_SYSCALL0 (pid, SYS_gettid);
	#elif defined(SYS_gettid)
	pid = syscall(SYS_gettid);
	#else
	pid = -1;
	#endif
#endif

	retv = qse_awk_rtx_makeintval (rtx, (qse_awk_int_t)pid);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_getppid (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t pid;
	qse_awk_val_t* retv;

#if defined(_WIN32)
	/* TOOD: implement this*/
	pid = -1;
	
#elif defined(__OS2__)
	/* TOOD: implement this*/
	pid = -1;
	
#elif defined(__DOS__)
	/* TOOD: implement this*/
	pid = -1;

#else
	pid = getppid ();
#endif

	retv = qse_awk_rtx_makeintval (rtx, pid);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_getuid (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t uid;
	qse_awk_val_t* retv;

#if defined(_WIN32)
	/* TOOD: implement this*/
	uid = -1;
	
#elif defined(__OS2__)
	/* TOOD: implement this*/
	uid = -1;
	
#elif defined(__DOS__)
	/* TOOD: implement this*/
	uid = -1;

#else
	uid = getuid ();
#endif

	retv = qse_awk_rtx_makeintval (rtx, uid);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_getgid (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t gid;
	qse_awk_val_t* retv;

#if defined(_WIN32)
	/* TOOD: implement this*/
	gid = -1;
	
#elif defined(__OS2__)
	/* TOOD: implement this*/
	gid = -1;

#elif defined(__DOS__)
	/* TOOD: implement this*/
	gid = -1;

#else
	gid = getgid ();
#endif

	retv = qse_awk_rtx_makeintval (rtx, gid);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_geteuid (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t uid;
	qse_awk_val_t* retv;

#if defined(_WIN32)
	/* TOOD: implement this*/
	uid = -1;
	
#elif defined(__OS2__)
	/* TOOD: implement this*/
	uid = -1;
	
#elif defined(__DOS__)
	/* TOOD: implement this*/
	uid = -1;

#else
	uid = geteuid ();
#endif

	retv = qse_awk_rtx_makeintval (rtx, uid);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_getegid (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t gid;
	qse_awk_val_t* retv;

#if defined(_WIN32)
	/* TOOD: implement this*/
	gid = -1;
	
#elif defined(__OS2__)
	/* TOOD: implement this*/
	gid = -1;
	
#elif defined(__DOS__)
	/* TOOD: implement this*/
	gid = -1;

#else
	gid = getegid ();
#endif

	retv = qse_awk_rtx_makeintval (rtx, gid);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_sleep (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_int_t lv;
	qse_awk_flt_t fv;
	qse_awk_val_t* retv;
	int rx;

	rx = qse_awk_rtx_valtonum(rtx, qse_awk_rtx_getarg (rtx, 0), &lv, &fv);
	if (rx == 0)
	{
#if defined(_WIN32)
		Sleep ((DWORD)QSE_SEC_TO_MSEC(lv));
		rx = 0;
#elif defined(__OS2__)
		DosSleep ((ULONG)QSE_SEC_TO_MSEC(lv));
		rx = 0;
#elif defined(__DOS__)
		#if (defined(__WATCOMC__) && (__WATCOMC__ < 1200))
			sleep (lv);
			rx = 0;
		#else
			rx = sleep (lv);
		#endif
#elif defined(HAVE_NANOSLEEP)
		struct timespec req;
		req.tv_sec = lv;
		req.tv_nsec = 0;
		rx = nanosleep(&req, QSE_NULL);
#else
		rx = sleep(lv);
#endif
	}
	else if (rx >= 1)
	{
#if defined(_WIN32)
		Sleep ((DWORD)QSE_SEC_TO_MSEC(fv));
		rx = 0;
#elif defined(__OS2__)
		DosSleep ((ULONG)QSE_SEC_TO_MSEC(fv));
		rx = 0;
#elif defined(__DOS__)
		/* no high-resolution sleep() is available */
		#if (defined(__WATCOMC__) && (__WATCOMC__ < 1200))
			sleep ((qse_awk_int_t)fv);
			rx = 0;
		#else
			rx = sleep((qse_awk_int_t)fv);
		#endif;
#elif defined(HAVE_NANOSLEEP)
		struct timespec req;
		req.tv_sec = (qse_awk_int_t)fv;
		req.tv_nsec = QSE_SEC_TO_NSEC(fv - req.tv_sec);
		rx = nanosleep(&req, QSE_NULL);
#elif defined(HAVE_SELECT)
		struct timeval req;
		req.tv_sec = (qse_awk_int_t)fv;
		req.tv_usec = QSE_SEC_TO_USEC(fv - req.tv_sec);
		rx = select(0, QSE_NULL, QSE_NULL, QSE_NULL, &req);
#else
		/* no high-resolution sleep() is available */
		rx = sleep((qse_awk_int_t)fv);
#endif
	}

	retv = qse_awk_rtx_makeintval(rtx, rx);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_gettime (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_val_t* retv;
	qse_ntime_t now;

	if (qse_gettime (&now) <= -1) now.sec = 0;

	retv = qse_awk_rtx_makeintval (rtx, now.sec);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_settime (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_val_t* retv;
	qse_ntime_t now;
	qse_awk_int_t tmp;
	int rx;

	now.nsec = 0;

	if (qse_awk_rtx_valtoint (rtx, qse_awk_rtx_getarg (rtx, 0), &tmp) <= -1) rx = -1;
	else
	{
		now.sec = tmp;
		if (qse_settime (&now) <= -1) rx = -1;
		else rx = 0;
	}

	retv = qse_awk_rtx_makeintval (rtx, rx);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_mktime (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_ntime_t nt;
	qse_size_t nargs;
	qse_awk_val_t* retv;

	nargs = qse_awk_rtx_getnargs(rtx);
	if (nargs >= 1)
	{
		int sign;
		qse_char_t* str, * p, * end;
		qse_size_t len;
		qse_awk_val_t* a0;
		qse_btime_t bt;

		a0 = qse_awk_rtx_getarg (rtx, 0);
		str = qse_awk_rtx_getvalstr (rtx, a0, &len);
		if (str == QSE_NULL) return -1;

		/* the string must be of the format  YYYY MM DD HH MM SS[ DST] */
		p = str;
		end = str + len;
		QSE_MEMSET (&bt, 0, QSE_SIZEOF(bt));

		sign = 1;
		if (p < end && *p == QSE_T('-')) { sign = -1; p++; }
		while (p < end && QSE_ISDIGIT(*p)) bt.year = bt.year * 10 + (*p++ - QSE_T('0'));
		bt.year *= sign;
		bt.year -= 1900;
		while (p < end && (QSE_ISSPACE(*p) || *p == QSE_T('\0'))) p++;

		sign = 1;
		if (p < end && *p == QSE_T('-')) { sign = -1; p++; }
		while (p < end && QSE_ISDIGIT(*p)) bt.mon = bt.mon * 10 + (*p++ - QSE_T('0'));
		bt.mon *= sign;
		bt.mon -= 1;
		while (p < end && (QSE_ISSPACE(*p) || *p == QSE_T('\0'))) p++;

		sign = 1;
		if (p < end && *p == QSE_T('-')) { sign = -1; p++; }
		while (p < end && QSE_ISDIGIT(*p)) bt.mday = bt.mday * 10 + (*p++ - QSE_T('0'));
		bt.mday *= sign;
		while (p < end && (QSE_ISSPACE(*p) || *p == QSE_T('\0'))) p++;

		sign = 1;
		if (p < end && *p == QSE_T('-')) { sign = -1; p++; }
		while (p < end && QSE_ISDIGIT(*p)) bt.hour = bt.hour * 10 + (*p++ - QSE_T('0'));
		bt.hour *= sign;
		while (p < end && (QSE_ISSPACE(*p) || *p == QSE_T('\0'))) p++;

		sign = 1;
		if (p < end && *p == QSE_T('-')) { sign = -1; p++; }
		while (p < end && QSE_ISDIGIT(*p)) bt.min = bt.min * 10 + (*p++ - QSE_T('0'));
		bt.min *= sign;
		while (p < end && (QSE_ISSPACE(*p) || *p == QSE_T('\0'))) p++;

		sign = 1;
		if (p < end && *p == QSE_T('-')) { sign = -1; p++; }
		while (p < end && QSE_ISDIGIT(*p)) bt.sec = bt.sec * 10 + (*p++ - QSE_T('0'));
		bt.sec *= sign;
		while (p < end && (QSE_ISSPACE(*p) || *p == QSE_T('\0'))) p++;

		sign = 1;
		if (p < end && *p == QSE_T('-')) { sign = -1; p++; }
		while (p < end && QSE_ISDIGIT(*p)) bt.isdst = bt.isdst * 10 + (*p++ - QSE_T('0'));
		bt.isdst *= sign;
		while (p < end && (QSE_ISSPACE(*p) || *p == QSE_T('\0'))) p++;

		qse_awk_rtx_freevalstr (rtx, a0, str);
		qse_timelocal (&bt, &nt);
	}
	else
	{
		/* get the current time when no argument is given */
		qse_gettime (&nt);
	}

	retv = qse_awk_rtx_makeintval(rtx, nt.sec);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}


#define STRFTIME_UTC (1 << 0)

static int fnc_strftime (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{

	/*
	sys::strftime("%Y-%m-%d %H:%M:%S %z", sys::gettime()); 
	sys::strftime("%Y-%m-%d %H:%M:%S %z", sys::gettime(), sys::STRFTIME_UTC);
	*/

	qse_mchar_t* fmt;
	qse_size_t len;
	qse_awk_val_t* retv;

	fmt = qse_awk_rtx_valtombsdup(rtx, qse_awk_rtx_getarg(rtx, 0), &len);
	if (fmt) 
	{
		qse_ntime_t nt;
		qse_btime_t bt;
		qse_awk_int_t tmpsec, flags = 0;

		nt.nsec = 0;
		if (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 1), &tmpsec) <= -1) 
		{
			nt.sec = 0;
		}
		else
		{
			nt.sec = tmpsec;
		}

		
		if (qse_awk_rtx_getnargs(rtx) >= 3 && (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 2), &flags) <= -1 || flags < 0)) flags = 0;

		if (((flags & STRFTIME_UTC)? qse_gmtime(&nt, &bt): qse_localtime(&nt, &bt)) >= 0)
		{
			qse_mchar_t tmpbuf[64], * tmpptr;
			struct tm tm;
			qse_size_t sl;

			QSE_MEMSET (&tm, 0, QSE_SIZEOF(tm));
			tm.tm_year = bt.year;
			tm.tm_mon = bt.mon;
			tm.tm_mday = bt.mday;
			tm.tm_hour = bt.hour;
			tm.tm_min = bt.min;
			tm.tm_sec = bt.sec;
			tm.tm_isdst = bt.isdst;
		#if defined(HAVE_STRUCT_TM_TM_GMTOFF)
			tm.tm_gmtoff = bt.gmtoff;
		#elif defined(HAVE_STRUCT_TM___TM_GMTOFF)
			tm.__tm_gmtoff = bt.gmtoff;
		#endif
			if (flags & STRFTIME_UTC)
			{
			#if defined(HAVE_STRUCT_TM_TM_ZONE)
				tm.tm_zone = "GMT";
			#elif defined(HAVE_STRUCT_TM___TM_ZONE)
				tm.__tm_zone = "GMT";
			#endif
			}

			sl = strftime(tmpbuf, QSE_COUNTOF(tmpbuf), fmt, &tm);
			if (sl <= 0 || sl >= QSE_COUNTOF(tmpbuf))
			{
				/* buffer too small */
				qse_mchar_t* tmp;
				qse_size_t tmpcapa, i, count = 0;

/*
man strftime >>>

RETURN VALUE
       The strftime() function returns the number of bytes placed in the array s, not including the  terminating  null  byte,  provided  the
       string,  including  the  terminating  null  byte,  fits.  Otherwise, it returns 0, and the contents of the array is undefined.  (This
       behavior applies since at least libc 4.4.4; very old versions of libc, such as libc 4.4.1, would return max  if  the  array  was  too
       small.)

       Note that the return value 0 does not necessarily indicate an error; for example, in many locales %p yields an empty string.
 
--------------------------------------------------------------------------------------
* 
I use 'count' to limit the maximum number of retries when 0 is returned.
*/

				for (i = 0; i < len;)
				{
					if (fmt[i] == QSE_MT('%')) 
					{
						count++; /* the nubmer of % specifier */
						i++;
						if (i < len) i++;
					}
					else i++;
				}

				tmpptr = QSE_NULL;
				tmpcapa = QSE_COUNTOF(tmpbuf);
				if (tmpcapa < len) tmpcapa = len;

				do
				{
					if (count <= 0) 
					{
						if (tmpptr) qse_awk_rtx_freemem (rtx, tmpptr);
						tmpbuf[0] = QSE_MT('\0');
						tmpptr = tmpbuf;
						break;
					}
					count--;

					tmpcapa *= 2;
					tmp = (qse_mchar_t*)qse_awk_rtx_reallocmem(rtx, tmpptr, tmpcapa * QSE_SIZEOF(*tmpptr));
					if (!tmp) 
					{
						if (tmpptr) qse_awk_rtx_freemem (rtx, tmpptr);
						tmpbuf[0] = QSE_MT('\0');
						tmpptr = tmpbuf;
						break;
					}

					tmpptr = tmp;
					sl = strftime(tmpptr, tmpcapa, fmt, &tm);
				}
				while (sl <= 0 || sl >= tmpcapa);
			}
			else
			{
				tmpptr = tmpbuf;
			}
			qse_awk_rtx_freemem (rtx, fmt);

			retv = qse_awk_rtx_makestrvalwithmbs(rtx, tmpptr);
			if (tmpptr && tmpptr != tmpbuf) qse_awk_rtx_freemem (rtx, tmpptr);
			if (retv == QSE_NULL) return -1;

			qse_awk_rtx_setretval (rtx, retv);
		}
		else
		{
			qse_awk_rtx_freemem (rtx, fmt);
		}
	}
	return 0;
}

static int fnc_getenv (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_mchar_t* var;
	qse_size_t len;
	qse_awk_val_t* retv;

	var = qse_awk_rtx_valtombsdup(
		rtx, qse_awk_rtx_getarg (rtx, 0), &len);
	if (var)
	{
		qse_mchar_t* val;

		val = getenv (var);	
		if (val) 
		{
			retv = qse_awk_rtx_makestrvalwithmbs (rtx, val);
			if (retv == QSE_NULL) return -1;

			qse_awk_rtx_setretval (rtx, retv);
		}

		qse_awk_rtx_freemem (rtx, var);
	}

	return 0;
}

static int fnc_getnwifcfg (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_nwifcfg_t cfg;
	qse_awk_rtx_valtostr_out_t out;
	int ret = -1;

	out.type = QSE_AWK_RTX_VALTOSTR_CPLCPY;
	out.u.cplcpy.ptr = cfg.name;
	out.u.cplcpy.len = QSE_COUNTOF(cfg.name);
	if (qse_awk_rtx_valtostr(rtx, qse_awk_rtx_getarg(rtx, 0), &out) >= 0)
	{
		qse_awk_int_t type;
		int rx;

		rx = qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 1), &type);
		if (rx >= 0)
		{
			cfg.type = type;

			if (qse_getnwifcfg(&cfg) >= 0)
			{
				/* make a map value containg configuration */
				qse_awk_int_t index, mtu;
				qse_char_t addr[128];
				qse_char_t mask[128];
				qse_char_t ethw[32];
				qse_awk_val_map_data_t md[7];
				qse_awk_val_t* tmp;

				QSE_MEMSET (md, 0, QSE_SIZEOF(md));

				md[0].key.ptr = QSE_T("index");
				md[0].key.len = 5;
				md[0].type = QSE_AWK_VAL_MAP_DATA_INT;
				index = cfg.index;
				md[0].vptr = &index;

				md[1].key.ptr = QSE_T("mtu");
				md[1].key.len = 3;
				md[1].type = QSE_AWK_VAL_MAP_DATA_INT;
				mtu = cfg.mtu;
				md[1].vptr = &mtu;
		
				md[2].key.ptr = QSE_T("addr");
				md[2].key.len = 4;
				md[2].type = QSE_AWK_VAL_MAP_DATA_STR;
				qse_nwadtostr (&cfg.addr, addr, QSE_COUNTOF(addr), QSE_NWADTOSTR_ADDR);
				md[2].vptr = addr;

				md[3].key.ptr = QSE_T("mask");
				md[3].key.len = 4;
				md[3].type = QSE_AWK_VAL_MAP_DATA_STR;
				qse_nwadtostr (&cfg.mask, mask, QSE_COUNTOF(mask), QSE_NWADTOSTR_ADDR);
				md[3].vptr = mask;

				md[4].key.ptr = QSE_T("ethw");
				md[4].key.len = 4;
				md[4].type = QSE_AWK_VAL_MAP_DATA_STR;
				qse_strxfmt (ethw, QSE_COUNTOF(ethw), QSE_T("%02X:%02X:%02X:%02X:%02X:%02X"), 
					cfg.ethw[0], cfg.ethw[1], cfg.ethw[2], cfg.ethw[3], cfg.ethw[4], cfg.ethw[5]);
				md[4].vptr = ethw;

				if (cfg.flags & (QSE_NWIFCFG_LINKUP | QSE_NWIFCFG_LINKDOWN))
				{
					md[5].key.ptr = QSE_T("link");
					md[5].key.len = 4;
					md[5].type = QSE_AWK_VAL_MAP_DATA_STR;
					md[5].vptr = (cfg.flags & QSE_NWIFCFG_LINKUP)? QSE_T("up"): QSE_T("down");
				}

				tmp = qse_awk_rtx_makemapvalwithdata (rtx, md);
				if (tmp)
				{
					int x;
					qse_awk_rtx_refupval (rtx, tmp);
					x = qse_awk_rtx_setrefval (rtx, (qse_awk_val_ref_t*)qse_awk_rtx_getarg(rtx, 2), tmp);
					qse_awk_rtx_refdownval (rtx, tmp);
					if (x <= -1) return -1;
					ret = 0;
				}
			}
		}
	}

	/* no error check for qse_awk_rtx_makeintval() since ret is 0 or -1 */
	qse_awk_rtx_setretval (rtx, qse_awk_rtx_makeintval (rtx, ret));
	return 0;
}
/* ------------------------------------------------------------ */

static int fnc_system (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_val_t* v, * a0;
	qse_char_t* str;
	qse_size_t len;
	int n = 0;

	a0 = qse_awk_rtx_getarg (rtx, 0);
	str = qse_awk_rtx_getvalstr (rtx, a0, &len);
	if (str == QSE_NULL) return -1;

	/* the target name contains a null character.
	 * make system return -1 */
	if (qse_strxchr (str, len, QSE_T('\0')))
	{
		n = -1;
		goto skip_system;
	}

#if defined(_WIN32)
	n = _tsystem (str);
#elif defined(QSE_CHAR_IS_MCHAR)
	n = system (str);
#else

	{
		qse_mchar_t* mbs;
		mbs = qse_wcstombsdupwithcmgr(str, QSE_NULL, qse_awk_rtx_getmmgr(rtx), qse_awk_rtx_getcmgr(rtx));
		if (mbs == QSE_NULL) 
		{
			n = -1;
			goto skip_system;
		}
		n = system (mbs);
		qse_awk_rtx_freemem (rtx, mbs);
	}

#endif

skip_system:
	qse_awk_rtx_freevalstr (rtx, a0, str);

	v = qse_awk_rtx_makeintval (rtx, (qse_awk_int_t)n);
	if (v == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, v);
	return 0;
}

/* ------------------------------------------------------------ */
static int fnc_chmod (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_val_t* v, * a0;
	qse_char_t* str;
	qse_size_t len;
	int n = 0;
	qse_awk_int_t mode;

	a0 = qse_awk_rtx_getarg (rtx, 0);
	str = qse_awk_rtx_getvalstr (rtx, a0, &len);
	if (!str) return -1;

	/* the target name contains a null character.
	 * make system return -1 */
	if (qse_strxchr(str, len, QSE_T('\0')))
	{
		n = -1;
		goto skip_mkdir;
	}

	if (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 1), &mode) <= -1 || mode < 0) mode = DEFAULT_MODE;

#if defined(_WIN32)
	n = _tchmod(str, mode);
#elif defined(QSE_CHAR_IS_MCHAR)
	n = chmod(str, mode);
#else

	{
		qse_mchar_t* mbs;
		mbs = qse_wcstombsdupwithcmgr(str, QSE_NULL, qse_awk_rtx_getmmgr(rtx), qse_awk_rtx_getcmgr(rtx));
		if (mbs == QSE_NULL) 
		{
			n = -1;
			goto skip_mkdir;
		}
		n = chmod(mbs, mode);
		qse_awk_rtx_freemem (rtx, mbs);
	}

#endif

skip_mkdir:
	qse_awk_rtx_freevalstr (rtx, a0, str);

	v = qse_awk_rtx_makeintval (rtx, (qse_awk_int_t)n);
	if (v == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, v);
	return 0;
}

static int fnc_mkdir (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_val_t* v, * a0;
	qse_char_t* str;
	qse_size_t len;
	int n = 0;
	qse_awk_int_t mode;

	a0 = qse_awk_rtx_getarg (rtx, 0);
	str = qse_awk_rtx_getvalstr (rtx, a0, &len);
	if (!str) return -1;

	/* the target name contains a null character.
	 * make system return -1 */
	if (qse_strxchr(str, len, QSE_T('\0')))
	{
		n = -1;
		goto skip_mkdir;
	}

	if (qse_awk_rtx_getnargs(rtx) >= 2 && (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 1), &mode) <= -1 || mode < 0)) mode = DEFAULT_MODE;

#if defined(_WIN32)
	n = _tmkdir(str);
#elif defined(QSE_CHAR_IS_MCHAR)
	n = mkdir(str, mode);
#else

	{
		qse_mchar_t* mbs;
		mbs = qse_wcstombsdupwithcmgr(str, QSE_NULL, qse_awk_rtx_getmmgr(rtx), qse_awk_rtx_getcmgr(rtx));
		if (mbs == QSE_NULL) 
		{
			n = -1;
			goto skip_mkdir;
		}
		n = mkdir(mbs, mode);
		qse_awk_rtx_freemem (rtx, mbs);
	}

#endif

skip_mkdir:
	qse_awk_rtx_freevalstr (rtx, a0, str);

	v = qse_awk_rtx_makeintval (rtx, (qse_awk_int_t)n);
	if (v == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, v);
	return 0;
}

static int fnc_unlink (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	qse_awk_val_t* v, * a0;
	qse_char_t* str;
	qse_size_t len;
	int n = 0;

	a0 = qse_awk_rtx_getarg (rtx, 0);
	str = qse_awk_rtx_getvalstr(rtx, a0, &len);
	if (!str) return -1;

	/* the target name contains a null character.
	 * make system return -1 */
	if (qse_strxchr(str, len, QSE_T('\0')))
	{
		n = -1;
		goto skip_unlink;
	}

#if defined(_WIN32)
	n = _tunlink(str);
#elif defined(QSE_CHAR_IS_MCHAR)
	n = unlink(str);
#else

	{
		qse_mchar_t* mbs;
		mbs = qse_wcstombsdupwithcmgr(str, QSE_NULL, qse_awk_rtx_getmmgr(rtx), qse_awk_rtx_getcmgr(rtx));
		if (mbs == QSE_NULL) 
		{
			n = -1;
			goto skip_unlink;
		}
		n = unlink(mbs);
		qse_awk_rtx_freemem (rtx, mbs);
	}

#endif

skip_unlink:
	qse_awk_rtx_freevalstr (rtx, a0, str);

	v = qse_awk_rtx_makeintval (rtx, (qse_awk_int_t)n);
	if (v == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, v);
	return 0;
}

/* ------------------------------------------------------------ */
static void open_remote_log_socket (qse_awk_rtx_t* rtx, mod_ctx_t* mctx)
{
#if defined(_WIN32)
	/* TODO: implement this */
#else
	int sck, flags;
	int domain = qse_skadfamily(&mctx->log.skad);
	int type = SOCK_DGRAM;

	QSE_ASSERT (mctx->log.sck <= -1);


#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
	type |= SOCK_NONBLOCK;
	type |= SOCK_CLOEXEC;
open_socket:
#endif
	sck = socket(domain, type, 0); 
	if (sck <= -1)
	{
	#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
		if (errno == EINVAL && (type & (SOCK_NONBLOCK | SOCK_CLOEXEC)))
		{
			type &= ~(SOCK_NONBLOCK | SOCK_CLOEXEC);
			goto open_socket;
		}
	#endif
		return;
	}
	else
	{
	#if defined(SOCK_NONBLOCK) && defined(SOCK_CLOEXEC)
		if (type & (SOCK_NONBLOCK | SOCK_CLOEXEC)) goto done;
	#endif
	}

	flags = fcntl(sck, F_GETFD, 0);
	if (flags <= -1) return;
#if defined(FD_CLOEXEC)
	flags |= FD_CLOEXEC;
#endif
#if defined(O_NONBLOCK)
	flags |= O_NONBLOCK;
#endif
	if (fcntl(sck, F_SETFD, flags) <= -1) return;

done:
	mctx->log.sck = sck;

#endif
}

static int fnc_openlog (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	int rx = -1;
	qse_awk_int_t opt, fac;
	qse_awk_val_t* retv;
	qse_char_t* ident = QSE_NULL, * actual_ident;
	qse_size_t ident_len;
	qse_mchar_t* mbs_ident;
	mod_ctx_t* mctx = (mod_ctx_t*)fi->mod->ctx;
	qse_nwad_t nwad;
	syslog_type_t log_type = SYSLOG_LOCAL;


	ident = qse_awk_rtx_getvalstr(rtx, qse_awk_rtx_getarg(rtx, 0), &ident_len);
	if (!ident) goto done;

	/* the target name contains a null character.
	 * make system return -1 */
	if (qse_strxchr(ident, ident_len, QSE_T('\0'))) goto done;

	if (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 1), &opt) <= -1) goto done;
	if (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 2), &fac) <= -1) goto done;

	if (qse_strbeg(ident, QSE_T("remote://")))
	{
		qse_char_t* slash;
		/* "udp://remote-addr:remote-port/syslog-identifier" */

		log_type = SYSLOG_REMOTE;
		actual_ident = ident + 9;
		slash = qse_strchr(actual_ident, QSE_T('/'));
		if (!slash) goto done;
		if (qse_strntonwad(actual_ident, slash - actual_ident, &nwad) <= -1) goto done;
		actual_ident = slash + 1;
	}
	else if (qse_strbeg(ident, QSE_T("local://")))
	{
		/* "local://syslog-identifier" */
		actual_ident = ident + 8;
	}
	else
	{
		actual_ident = ident;
	}

#if defined(QSE_CHAR_IS_MCHAR)
	mbs_ident = qse_mbsdup(actual_ident, qse_awk_rtx_getmmgr(rtx));
#else
	mbs_ident = qse_wcstombsdupwithcmgr(actual_ident, QSE_NULL, qse_awk_rtx_getmmgr(rtx), qse_awk_rtx_getcmgr(rtx));
#endif
	if (!mbs_ident) goto done;

	if (mctx->log.ident) qse_awk_rtx_freemem (rtx, mctx->log.ident);
	mctx->log.ident = mbs_ident;

#if defined(ENABLE_SYSLOG)
	if (mctx->log.syslog_opened)
	{
		closelog ();
		mctx->log.syslog_opened = 0;
	}
#endif
	if (mctx->log.sck >= 0)
	{
	#if defined(_WIN32)
		/* TODO: impelement this */
	#else
		close (mctx->log.sck);
	#endif
		mctx->log.sck = -1;
	}

	mctx->log.type = log_type;
	mctx->log.opt = opt;
	mctx->log.fac = fac;
	if (mctx->log.type == SYSLOG_LOCAL)
	{
	#if defined(ENABLE_SYSLOG)
		openlog(mbs_ident, opt, fac);
		mctx->log.syslog_opened = 1;
	#endif
	}
	else if (mctx->log.type == SYSLOG_REMOTE)
	{
		qse_nwadtoskad (&nwad, &mctx->log.skad);
		if ((opt & LOG_NDELAY) && mctx->log.sck <= -1) open_remote_log_socket (rtx, mctx);
	}

	rx = 0;

done:
	if (ident) qse_awk_rtx_freevalstr(rtx, qse_awk_rtx_getarg(rtx, 0), ident);

	retv = qse_awk_rtx_makeintval(rtx, rx);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

static int fnc_closelog (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	int rx = -1;
	qse_awk_val_t* retv;
	mod_ctx_t* mctx = (mod_ctx_t*)fi->mod->ctx;

	switch (mctx->log.type)
	{
		case SYSLOG_LOCAL:
		#if defined(ENABLE_SYSLOG)
			closelog ();
			/* closelog() might be called without openlog(). so there is no 
			 * check if syslog_opened is true.
			 * it is just used as an indicator to decide wheter closelog()
			 * should be called upon module finalization(fini). */
			mctx->log.syslog_opened = 0;
		#endif
			break;

		case SYSLOG_REMOTE:
			if (mctx->log.sck >= 0)
			{
			#if defined(_WIN32)
				/* TODO: impelement this */
			#else
				close (mctx->log.sck);
			#endif
				mctx->log.sck = -1;
			}

			if (mctx->log.dmsgbuf)
			{
				qse_mbs_close (mctx->log.dmsgbuf);
				mctx->log.dmsgbuf = QSE_NULL;
			}

			break;
	}

	if (mctx->log.ident)
	{
		qse_awk_rtx_freemem (rtx, mctx->log.ident);
		mctx->log.ident = QSE_NULL;
	}

	/* back to the local syslog in case writelog() is called
	 * without another openlog() after this closelog() */
	mctx->log.type = SYSLOG_LOCAL;

	rx = 0;

	retv = qse_awk_rtx_makeintval(rtx, rx);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}


static int fnc_writelog (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi)
{
	int rx = -1;
	qse_awk_val_t* retv;
	qse_awk_int_t pri;
	qse_char_t* msg = QSE_NULL;
	qse_size_t msglen;
	mod_ctx_t* mctx = (mod_ctx_t*)fi->mod->ctx;

	if (qse_awk_rtx_valtoint(rtx, qse_awk_rtx_getarg(rtx, 0), &pri) <= -1) goto done;

	msg = qse_awk_rtx_getvalstr(rtx, qse_awk_rtx_getarg(rtx, 1), &msglen);
	if (!msg) goto done;

	if (qse_strxchr(msg, msglen, QSE_T('\0'))) goto done;

	if (mctx->log.type == SYSLOG_LOCAL)
	{
	#if defined(ENABLE_SYSLOG)
		#if defined(QSE_CHAR_IS_MCHAR)
		syslog(pri, "%s", msg);
		#else
		{
			qse_mchar_t* mbs;
			mbs = qse_wcstombsdupwithcmgr(msg, QSE_NULL, qse_awk_rtx_getmmgr(rtx), qse_awk_rtx_getcmgr(rtx));
			if (!mbs) goto done;
			syslog(pri, "%s", mbs);
			qse_awk_rtx_freemem (rtx, mbs);
		}
		#endif
	#endif
	}
	else if (mctx->log.type == SYSLOG_REMOTE)
	{
	#if defined(_WIN32)
		/* TODO: implement this */
	#else
		qse_ntime_t now;
		qse_btime_t cnow;

		static const qse_mchar_t* __syslog_month_names[] =
		{
			QSE_MT("Jan"),
			QSE_MT("Feb"),
			QSE_MT("Mar"),
			QSE_MT("Apr"),
			QSE_MT("May"),
			QSE_MT("Jun"),
			QSE_MT("Jul"),
			QSE_MT("Aug"),
			QSE_MT("Sep"),
			QSE_MT("Oct"),
			QSE_MT("Nov"),
			QSE_MT("Dec"),
		};

		if (mctx->log.sck <= -1) open_remote_log_socket (rtx, mctx);

		if (mctx->log.sck >= 0)
		{
			if (!mctx->log.dmsgbuf) mctx->log.dmsgbuf = qse_mbs_open(qse_awk_rtx_getmmgr(rtx), 0, 0);
			if (!mctx->log.dmsgbuf) goto done;

			if (qse_gettime(&now) || qse_localtime(&now, &cnow) <= -1) goto done;

			if (qse_mbs_fmt (
				mctx->log.dmsgbuf, QSE_MT("<%d>%s %02d %02d:%02d:%02d "), 
				(int)(mctx->log.fac | pri),
				__syslog_month_names[cnow.mon], cnow.mday, 
				cnow.hour, cnow.min, cnow.sec) == (qse_size_t)-1) goto done;

			if (mctx->log.ident || (mctx->log.opt & LOG_PID))
			{
				/* if the identifier is set or LOG_PID is set, the produced tag won't be empty.
				 * so appending ':' is kind of ok */

				if (qse_mbs_fcat(mctx->log.dmsgbuf, QSE_MT("%hs"), (mctx->log.ident? mctx->log.ident: QSE_MT(""))) == (qse_size_t)-1) goto done;

				if (mctx->log.opt & LOG_PID)
				{
					if (qse_mbs_fcat(mctx->log.dmsgbuf, QSE_MT("[%d]"), (int)QSE_GETPID()) == (qse_size_t)-1) goto done;
				}

				if (qse_mbs_fcat(mctx->log.dmsgbuf, QSE_MT(": ")) == (qse_size_t)-1) goto done;
			}

		#if defined(QSE_CHAR_IS_MCHAR)
			if (qse_mbs_fcat(mctx->log.dmsgbuf, QSE_MT("%hs"), msg) == (qse_size_t)-1) goto done;
		#else
			if (qse_mbs_fcat(mctx->log.dmsgbuf, QSE_MT("%ls"), msg) == (qse_size_t)-1) goto done;
		#endif

			/* don't care about output failure */
			sendto (mctx->log.sck, QSE_MBS_PTR(mctx->log.dmsgbuf), QSE_MBS_LEN(mctx->log.dmsgbuf),
			        0, (struct sockaddr*)&mctx->log.skad, qse_skadsize(&mctx->log.skad));
		}
	#endif
	}

	rx = 0;

done:
	if (msg) qse_awk_rtx_freevalstr(rtx, qse_awk_rtx_getarg(rtx, 1), msg);

	retv = qse_awk_rtx_makeintval(rtx, rx);
	if (retv == QSE_NULL) return -1;

	qse_awk_rtx_setretval (rtx, retv);
	return 0;
}

/* ------------------------------------------------------------ */

typedef struct fnctab_t fnctab_t;
struct fnctab_t
{
	const qse_char_t* name;
	qse_awk_mod_sym_fnc_t info;
};

typedef struct inttab_t inttab_t;
struct inttab_t
{
	const qse_char_t* name;
	qse_awk_mod_sym_int_t info;
};

static fnctab_t fnctab[] =
{
	/* keep this table sorted for binary search in query(). */

	{ QSE_T("WCOREDUMP"),   { { 1, 1, QSE_NULL     }, fnc_wcoredump,   0  } },
	{ QSE_T("WEXITSTATUS"), { { 1, 1, QSE_NULL     }, fnc_wexitstatus, 0  } },
	{ QSE_T("WIFEXITED"),   { { 1, 1, QSE_NULL     }, fnc_wifexited,   0  } },
	{ QSE_T("WIFSIGNALED"), { { 1, 1, QSE_NULL     }, fnc_wifsignaled, 0  } },
	{ QSE_T("WTERMSIG"),    { { 1, 1, QSE_NULL     }, fnc_wtermsig,    0  } },
	{ QSE_T("chmod"),       { { 2, 2, QSE_NULL     }, fnc_chmod,       0  } },
	{ QSE_T("close"),       { { 1, 1, QSE_NULL     }, fnc_close,       0  } },
	{ QSE_T("closelog"),    { { 0, 0, QSE_NULL     }, fnc_closelog,    0  } },
	{ QSE_T("errmsg"),      { { 0, 0, QSE_NULL     }, fnc_errmsg,      0  } },
	{ QSE_T("fork"),        { { 0, 0, QSE_NULL     }, fnc_fork,        0  } },
	{ QSE_T("getegid"),     { { 0, 0, QSE_NULL     }, fnc_getegid,     0  } },
	{ QSE_T("getenv"),      { { 1, 1, QSE_NULL     }, fnc_getenv,      0  } },
	{ QSE_T("geteuid"),     { { 0, 0, QSE_NULL     }, fnc_geteuid,     0  } },
	{ QSE_T("getgid"),      { { 0, 0, QSE_NULL     }, fnc_getgid,      0  } },
	{ QSE_T("getnwifcfg"),  { { 3, 3, QSE_T("vvr") }, fnc_getnwifcfg,  0  } },
	{ QSE_T("getpgid"),     { { 0, 0, QSE_NULL     }, fnc_getpgid,     0  } },
	{ QSE_T("getpid"),      { { 0, 0, QSE_NULL     }, fnc_getpid,      0  } },
	{ QSE_T("getppid"),     { { 0, 0, QSE_NULL     }, fnc_getppid,     0  } },
	{ QSE_T("gettid"),      { { 0, 0, QSE_NULL     }, fnc_gettid,      0  } },
	{ QSE_T("gettime"),     { { 0, 0, QSE_NULL     }, fnc_gettime,     0  } },
	{ QSE_T("getuid"),      { { 0, 0, QSE_NULL     }, fnc_getuid,      0  } },
	{ QSE_T("kill"),        { { 2, 2, QSE_NULL     }, fnc_kill,        0  } },
	{ QSE_T("mkdir"),       { { 1, 2, QSE_NULL     }, fnc_mkdir,       0  } },
	{ QSE_T("mktime"),      { { 0, 1, QSE_NULL     }, fnc_mktime,      0  } },
	{ QSE_T("open"),        { { 2, 3, QSE_NULL     }, fnc_open,        0  } },
	{ QSE_T("openlog"),     { { 3, 3, QSE_NULL     }, fnc_openlog,     0  } },
	{ QSE_T("pipe"),        { { 2, 3, QSE_T("rrv") }, fnc_pipe,        0  } },
	{ QSE_T("read"),        { { 2, 3, QSE_T("vrv") }, fnc_read,        0  } },
	{ QSE_T("settime"),     { { 1, 1, QSE_NULL     }, fnc_settime,     0  } },
	{ QSE_T("sleep"),       { { 1, 1, QSE_NULL     }, fnc_sleep,       0  } },
	{ QSE_T("strftime"),    { { 2, 3, QSE_NULL     }, fnc_strftime,    0  } },
	{ QSE_T("system"),      { { 1, 1, QSE_NULL     }, fnc_system,      0  } },
	{ QSE_T("systime"),     { { 0, 0, QSE_NULL     }, fnc_gettime,     0  } }, /* alias to gettime() */
	{ QSE_T("unlink"),      { { 1, 1, QSE_NULL     }, fnc_unlink,      0  } },
	{ QSE_T("wait"),        { { 1, 3, QSE_T("vrv") }, fnc_wait,        0  } },
	{ QSE_T("write"),       { { 2, 2, QSE_NULL     }, fnc_write,       0  } },
	{ QSE_T("writelog"),    { { 2, 2, QSE_NULL     }, fnc_writelog,    0  } }
};

#if !defined(SIGHUP)
#	define SIGHUP 1
#endif
#if !defined(SIGINT)
#	define SIGINT 2
#endif
#if !defined(SIGQUIT)
#	define SIGQUIT 3
#endif
#if !defined(SIGABRT)
#	define SIGABRT 6
#endif
#if !defined(SIGKILL)
#	define SIGKILL 9
#endif
#if !defined(SIGSEGV)
#	define SIGSEGV 11
#endif
#if !defined(SIGALRM)
#	define SIGALRM 14
#endif
#if !defined(SIGTERM)
#	define SIGTERM 15
#endif

static inttab_t inttab[] =
{
	/* keep this table sorted for binary search in query(). */
#if defined(ENABLE_SYSLOG)
	{ QSE_T("LOG_FAC_AUTH"),       { LOG_AUTH } },
	{ QSE_T("LOG_FAC_AUTHPRIV"),   { LOG_AUTHPRIV } },
	{ QSE_T("LOG_FAC_CRON"),       { LOG_CRON } },
	{ QSE_T("LOG_FAC_DAEMON"),     { LOG_DAEMON } },
	{ QSE_T("LOG_FAC_FTP"),        { LOG_FTP } },
	{ QSE_T("LOG_FAC_KERN"),       { LOG_KERN } },
	{ QSE_T("LOG_FAC_LOCAL0"),     { LOG_LOCAL0 } },
	{ QSE_T("LOG_FAC_LOCAL1"),     { LOG_LOCAL1 } },
	{ QSE_T("LOG_FAC_LOCAL2"),     { LOG_LOCAL2 } },
	{ QSE_T("LOG_FAC_LOCAL3"),     { LOG_LOCAL3 } },
	{ QSE_T("LOG_FAC_LOCAL4"),     { LOG_LOCAL4 } },
	{ QSE_T("LOG_FAC_LOCAL5"),     { LOG_LOCAL5 } },
	{ QSE_T("LOG_FAC_LOCAL6"),     { LOG_LOCAL6 } },
	{ QSE_T("LOG_FAC_LOCAL7"),     { LOG_LOCAL7 } },
	{ QSE_T("LOG_FAC_LPR"),        { LOG_LPR } },
	{ QSE_T("LOG_FAC_MAIL"),       { LOG_MAIL } },
	{ QSE_T("LOG_FAC_NEWS"),       { LOG_NEWS } },
	{ QSE_T("LOG_FAC_SYSLOG"),     { LOG_SYSLOG } },
	{ QSE_T("LOG_FAC_USER"),       { LOG_USER } },
	{ QSE_T("LOG_FAC_UUCP"),       { LOG_UUCP } },

	{ QSE_T("LOG_OPT_CONS"),       { LOG_CONS } },
	{ QSE_T("LOG_OPT_NDELAY"),     { LOG_NDELAY } },
	{ QSE_T("LOG_OPT_NOWAIT"),     { LOG_NOWAIT } },
	{ QSE_T("LOG_OPT_PID"),        { LOG_PID } },

	{ QSE_T("LOG_PRI_ALERT"),      { LOG_ALERT } },
	{ QSE_T("LOG_PRI_CRIT"),       { LOG_CRIT } },
	{ QSE_T("LOG_PRI_DEBUG"),      { LOG_DEBUG } },
	{ QSE_T("LOG_PRI_EMERG"),      { LOG_EMERG } },
	{ QSE_T("LOG_PRI_ERR"),        { LOG_ERR } },
	{ QSE_T("LOG_PRI_INFO"),       { LOG_INFO } },
	{ QSE_T("LOG_PRI_NOTICE"),     { LOG_NOTICE } },
	{ QSE_T("LOG_PRI_WARNING"),    { LOG_WARNING } },
#endif

	{ QSE_T("NWIFCFG_IN4"), { QSE_NWIFCFG_IN4 } },
	{ QSE_T("NWIFCFG_IN6"), { QSE_NWIFCFG_IN6 } },

#if defined(O_APPEND)
	{ QSE_T("O_APPEND"),    { O_APPEND } },
#endif
#if defined(O_ASYNC)
	{ QSE_T("O_ASYNC"),     { O_ASYNC } },
#endif
#if defined(O_CLOEXEC)
	{ QSE_T("O_CLOEXEC"),   { O_CLOEXEC } },
#endif
#if defined(O_CREAT)
	{ QSE_T("O_CREAT"),     { O_CREAT } },
#endif
#if defined(O_DIRECTORY)
	{ QSE_T("O_DIRECTORY"), { O_DIRECTORY } },
#endif
#if defined(O_DSYNC)
	{ QSE_T("O_DSYNC"),     { O_DSYNC } },
#endif
#if defined(O_EXCL)
	{ QSE_T("O_EXCL"),      { O_EXCL } },
#endif
#if defined(O_NOATIME)
	{ QSE_T("O_NOATIME"),   { O_NOATIME} },
#endif
#if defined(O_NOCTTY)
	{ QSE_T("O_NOCTTY"),    { O_NOCTTY} },
#endif
#if defined(O_NOFOLLOW)
	{ QSE_T("O_NOFOLLOW"),  { O_NOFOLLOW } },
#endif
#if defined(O_NONBLOCK)
	{ QSE_T("O_NONBLOCK"),  { O_NONBLOCK } },
#endif
#if defined(O_RDONLY)
	{ QSE_T("O_RDONLY"),    { O_RDONLY } },
#endif
#if defined(O_RDWR)
	{ QSE_T("O_RDWR"),      { O_RDWR } },
#endif
#if defined(O_SYNC)
	{ QSE_T("O_SYNC"),      { O_SYNC } },
#endif
#if defined(O_TRUNC)
	{ QSE_T("O_TRUNC"),     { O_TRUNC } },
#endif
#if defined(O_WRONLY)
	{ QSE_T("O_WRONLY"),    { O_WRONLY } },
#endif

	{ QSE_T("SIGABRT"), { SIGABRT } },
	{ QSE_T("SIGALRM"), { SIGALRM } },
	{ QSE_T("SIGHUP"),  { SIGHUP } },
	{ QSE_T("SIGINT"),  { SIGINT } },
	{ QSE_T("SIGKILL"), { SIGKILL } },
	{ QSE_T("SIGQUIT"), { SIGQUIT } },
	{ QSE_T("SIGSEGV"), { SIGSEGV } },
	{ QSE_T("SIGTERM"), { SIGTERM } },

	{ QSE_T("STRFTIME_UTC"), { STRFTIME_UTC } },

	{ QSE_T("WNOHANG"), { WNOHANG } }
};

static int query (qse_awk_mod_t* mod, qse_awk_t* awk, const qse_char_t* name, qse_awk_mod_sym_t* sym)
{
	qse_cstr_t ea;
	int left, right, mid, n;

	left = 0; right = QSE_COUNTOF(fnctab) - 1;

	while (left <= right)
	{
		mid = left + (right - left) / 2;

		n = qse_strcmp (fnctab[mid].name, name);
		if (n > 0) right = mid - 1; 
		else if (n < 0) left = mid + 1;
		else
		{
			sym->type = QSE_AWK_MOD_FNC;
			sym->u.fnc = fnctab[mid].info;
			return 0;
		}
	}

	left = 0; right = QSE_COUNTOF(inttab) - 1;
	while (left <= right)
	{
		mid = left + (right - left) / 2;

		n = qse_strcmp (inttab[mid].name, name);
		if (n > 0) right = mid - 1; 
		else if (n < 0) left = mid + 1;
		else
		{
			sym->type = QSE_AWK_MOD_INT;
			sym->u.in = inttab[mid].info;
			return 0;
		}
	}

	ea.ptr = (qse_char_t*)name;
	ea.len = qse_strlen(name);
	qse_awk_seterror (awk, QSE_AWK_ENOENT, &ea, QSE_NULL);
	return -1;
}

/* TODO: proper resource management */

static int init (qse_awk_mod_t* mod, qse_awk_rtx_t* rtx)
{
	mod_ctx_t* mctx = (mod_ctx_t*)mod->ctx;
	rtx_data_t data;

	mctx->log.type = SYSLOG_LOCAL;
	mctx->log.syslog_opened = 0;
	mctx->log.sck = -1;

	QSE_MEMSET (&data, 0, QSE_SIZEOF(data));
	if (qse_rbt_insert(mctx->rtxtab, &rtx, QSE_SIZEOF(rtx), &data, QSE_SIZEOF(data)) == QSE_NULL) 
	{
		qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOMEM, QSE_NULL);
		return -1;
	}

	return 0;
}

static void fini (qse_awk_mod_t* mod, qse_awk_rtx_t* rtx)
{
	/* TODO: 
	for (each pid for rtx) kill (pid, SIGKILL);
	for (each pid for rtx) waitpid (pid, QSE_NULL, 0);
	*/

	mod_ctx_t* mctx = (mod_ctx_t*)mod->ctx;
	qse_rbt_pair_t* pair;

	/* garbage clean-up */
	pair = qse_rbt_search(mctx->rtxtab, &rtx, QSE_SIZEOF(rtx));
	if (pair)
	{
		rtx_data_t* data;
		sys_node_t* sys_node, * sys_next;

		data = (rtx_data_t*)QSE_RBT_VPTR(pair);

		sys_node = data->sys_list.head;
		while (sys_node)
		{
			sys_next = sys_node->next;
			free_sys_node (rtx, &data->sys_list, sys_node);
			sys_node = sys_next;
		}

		if (data->sys_list.ctx.readbuf)
		{
			qse_awk_rtx_freemem (rtx, data->sys_list.ctx.readbuf);
			data->sys_list.ctx.readbuf = QSE_NULL;
			data->sys_list.ctx.readbuf_capa = 0;
		}
		qse_rbt_delete (mctx->rtxtab, &rtx, QSE_SIZEOF(rtx));
	}


#if defined(ENABLE_SYSLOG)
	if (mctx->log.syslog_opened) 
	{
		/* closelog() only if openlog() has been called explicitly.
		 * if you call writelog() functions without openlog() and
		 * end yoru program without closelog(), the program may leak
		 * some resources created by the writelog() function. (e.g.
		 * socket to /dev/log) */
		closelog ();
		mctx->log.syslog_opened = 0;
	}
#endif
	
	if (mctx->log.sck >= 0)
	{
	#if defined(_WIN32)
		/* TODO: implement this */
	#else
		close (mctx->log.sck);
	#endif
		mctx->log.sck = -1;
	}

	if (mctx->log.dmsgbuf)
	{
		qse_mbs_close (mctx->log.dmsgbuf);
		mctx->log.dmsgbuf = QSE_NULL;
	}

	if (mctx->log.ident) 
	{
		qse_awk_rtx_freemem (rtx, mctx->log.ident);
		mctx->log.ident = QSE_NULL;
	}
}

static void unload (qse_awk_mod_t* mod, qse_awk_t* awk)
{
	mod_ctx_t* mctx = (mod_ctx_t*)mod->ctx;

	QSE_ASSERT (QSE_RBT_SIZE(mctx->rtxtab) == 0);
	qse_rbt_close (mctx->rtxtab);

	qse_awk_freemem (awk, mctx);
}

int qse_awk_mod_sys (qse_awk_mod_t* mod, qse_awk_t* awk)
{
	qse_rbt_t* rbt;

	mod->query = query;
	mod->unload = unload;

	mod->init = init;
	mod->fini = fini;

	mod->ctx = qse_awk_callocmem(awk, QSE_SIZEOF(mod_ctx_t));
	if (!mod->ctx) return -1;

	rbt = qse_rbt_open(qse_awk_getmmgr(awk), 0, 1, 1);
	if (rbt == QSE_NULL) 
	{
		qse_awk_freemem (awk, mod->ctx);
		qse_awk_seterrnum (awk, QSE_AWK_ENOMEM, QSE_NULL);
		return -1;
	}
	qse_rbt_setstyle (rbt, qse_getrbtstyle(QSE_RBT_STYLE_INLINE_COPIERS));

	((mod_ctx_t*)mod->ctx)->rtxtab = rbt;
	return 0;
}

