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

#include <qse/cmn/mux.h>
#include "mem.h"

#if defined(_WIN32)
#	define FD_SETSIZE 4096 /* what is the best value??? */
#	include <winsock2.h>
#	include <windows.h>
#	define USE_SELECT
#elif defined(__OS2__)
#	if defined(TCPV40HDRS)
#		define BSD_SELECT
#	endif
#	include <types.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <sys/ioctl.h>
#	include <nerrno.h>
#	if defined(TCPV40HDRS)
#		define USE_SELECT
#		include <sys/select.h>
#	else
#		include <unistd.h>
#	endif
#	define INCL_DOSERRORS
#	include <os2.h>

#elif defined(__DOS__)
#	include <errno.h>
#	include <tcp.h> /* watt-32 */
#	define select select_s
#	define USE_SELECT

#else
#	include <unistd.h>
#	include <fcntl.h>
#	include <errno.h>
#	if defined(HAVE_SYS_TIME_H)
#		include <sys/time.h>
#	endif

#	if defined(QSE_MUX_USE_SELECT)
		/* you can set QSE_MUX_USE_SELECT to force using select() */
#		define USE_SELECT
#	elif defined(HAVE_SYS_EVENT_H) && defined(HAVE_KQUEUE) && defined(HAVE_KEVENT)
#		include <sys/event.h>
#		define USE_KQUEUE
#	elif defined(HAVE_SYS_EPOLL_H)
#		include <sys/epoll.h>
#		if defined(HAVE_EPOLL_CREATE)
#			define USE_EPOLL
#		endif
/*
#	elif defined(HAVE_POLL_H)
		TODO: IMPLEMENT THIS
#		define USE_POLL
*/
#	else
#		define USE_SELECT
#	endif
#endif

struct qse_mux_t
{
	qse_mmgr_t*      mmgr;
	qse_mux_errnum_t errnum;
	qse_mux_evtfun_t evtfun;

#if defined(USE_SELECT)
	fd_set rset;
	fd_set wset;
	fd_set tmprset;
	fd_set tmpwset;
	int size;
	int maxhnd;
	struct
	{
		qse_mux_evt_t** ptr;
		int ubound;
	} me;

#elif defined(USE_KQUEUE)

	int kq;

	/* kevent() places the events into the event list up to the limit specified.
	 * this implementation passes the 'evlist' array to kevent() upon polling.
	 * what is the best array size?
	 * TODO: find the optimal size or make it auto-scalable. */
	struct kevent evlist[512]; 
	int size;
	struct
	{
		qse_mux_evt_t** ptr;
		int ubound;
	} me;

#elif defined(USE_EPOLL)
	int fd;
	struct
	{
		struct epoll_event* ptr;
		qse_size_t len;
		qse_size_t capa;
	} ee;
	struct
	{
		qse_mux_evt_t** ptr;
		int ubound;
	} me;
#elif defined(__OS2__)
	int* fdarr;
	int  size;
	struct
	{
		qse_mux_evt_t** ptr;
		int ubound;
	} me;
#endif
};

int qse_mux_init (
	qse_mux_t*       mux,
	qse_mmgr_t*      mmgr,
	qse_mux_evtfun_t evtfun,
	qse_size_t       capahint
);
void qse_mux_fini (qse_mux_t* mux);

#if defined(_WIN32)
static qse_mux_errnum_t skerr_to_errnum (DWORD e)
{
	switch (e)
	{
		case WSA_NOT_ENOUGH_MEMORY:
			return QSE_MUX_ENOMEM;

		case WSA_INVALID_PARAMETER:
		case WSA_INVALID_HANDLE:
			return QSE_MUX_EINVAL;

		case WSAEACCES:
			return QSE_MUX_EACCES;

		case WSAEINTR:
			return QSE_MUX_EINTR;

		default:
			return QSE_MUX_ESYSERR;
	}
}
#elif defined(__OS2__)
static qse_mux_errnum_t skerr_to_errnum (int e)
{
	switch (e)
	{
	#if defined(SOCENOMEM)
		case SOCENOMEM:
			return QSE_MUX_ENOMEM;
	#endif

		case SOCEINVAL:
			return QSE_MUX_EINVAL;

		case SOCEACCES:
			return QSE_MUX_EACCES;

	#if defined(SOCENOENT)
		case SOCENOENT:
			return QSE_MUX_ENOENT;
	#endif

	#if defined(SOCEXIST)
		case SOCEEXIST:
			return QSE_MUX_EEXIST;
	#endif
	
		case SOCEINTR:
			return QSE_MUX_EINTR;

		case SOCEPIPE:
			return QSE_MUX_EPIPE;

		default:
			return QSE_MUX_ESYSERR;
	}
}

#elif defined(__DOS__)
static qse_mux_errnum_t skerr_to_errnum (int e)
{
	/* TODO: */
	return QSE_MUX_ESYSERR;
}
#else
static qse_mux_errnum_t skerr_to_errnum (int e)
{
	switch (e)
	{
		case ENOMEM:
			return QSE_MUX_ENOMEM;

		case EINVAL:
			return QSE_MUX_EINVAL;

		case EACCES:
			return QSE_MUX_EACCES;

		case ENOENT:
			return QSE_MUX_ENOENT;

		case EEXIST:
			return QSE_MUX_EEXIST;
	
		case EINTR:
			return QSE_MUX_EINTR;

		case EPIPE:
			return QSE_MUX_EPIPE;

#if defined(EAGAIN) || defined(EWOULDBLOCK)
	#if defined(EAGAIN) && defined(EWOULDBLOCK)
		case EAGAIN:
		#if (EWOULDBLOCK != EAGAIN)
		case EWOULDBLOCK:
		#endif
	#elif defined(EAGAIN)
		case EAGAIN:
	#else
		case EWOULDBLOCK;
	#endif
			return QSE_MUX_EAGAIN;
#endif

		default:
			return QSE_MUX_ESYSERR;
	}
}
#endif

qse_mux_t* qse_mux_open (
	qse_mmgr_t* mmgr, qse_size_t xtnsize, 
	qse_mux_evtfun_t evtfun, qse_size_t capahint, 
	qse_mux_errnum_t* errnum)
{
	qse_mux_t* mux;
	
	mux = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(*mux) + xtnsize);
	if (mux)
	{
		if (qse_mux_init (mux, mmgr, evtfun, capahint) <= -1)
		{
			if (errnum) *errnum = qse_mux_geterrnum (mux);
			QSE_MMGR_FREE (mmgr, mux);
			mux = QSE_NULL;
		}
		else QSE_MEMSET (QSE_XTN(mux), 0, xtnsize);
	}
	else if (errnum) *errnum = QSE_MUX_ENOMEM;

	return mux;
}

void qse_mux_close (qse_mux_t* mux)
{
	qse_mux_fini (mux);
	QSE_MMGR_FREE (mux->mmgr, mux);
}

int qse_mux_init (
	qse_mux_t* mux, qse_mmgr_t* mmgr,
	qse_mux_evtfun_t evtfun, qse_size_t capahint)
{
	QSE_MEMSET (mux, 0, QSE_SIZEOF(*mux));
	mux->mmgr = mmgr;
	mux->evtfun = evtfun;

	/* epoll_create returns an error and set errno to EINVAL
	 * if size is 0. Having a positive size greater than 0
	 * also makes easier other parts like maintaining internal
	 * event buffers */
	if (capahint <= 0) capahint = 1;

#if defined(USE_SELECT)
	FD_ZERO (&mux->rset);
	FD_ZERO (&mux->wset);
	mux->maxhnd = -1;

#elif defined(USE_KQUEUE)

	#if defined(HAVE_KQUEUE1) && defined(O_CLOEXEC)
	mux->kq = kqueue1 (O_CLOEXEC);
	#else
	mux->kq = kqueue ();
	#endif
	if (mux->kq <= -1)
	{
		mux->errnum = skerr_to_errnum (errno);
		return -1;
	}

	#if defined(HAVE_KQUEUE1) && defined(O_CLOEXEC)
	/* nothing to do */
	#elif defined(FD_CLOEXEC)
	{
		int flag = fcntl (mux->kq, F_GETFD);
		if (flag >= 0) fcntl (mux->kq, F_SETFD, flag | FD_CLOEXEC);
	}
	#endif

#elif defined(USE_EPOLL) 
	#if defined(HAVE_EPOLL_CREATE1) && defined(O_CLOEXEC)
	mux->fd = epoll_create1 (O_CLOEXEC);
	#else
	mux->fd = epoll_create (capahint);
	#endif
	if (mux->fd <= -1) 
	{
		mux->errnum = skerr_to_errnum (errno);
		return -1;
	}

	#if defined(HAVE_EPOLL_CREATE1) && defined(O_CLOEXEC)
	/* nothing to do */
	#elif defined(FD_CLOEXEC)
	{
		int flag = fcntl (mux->fd, F_GETFD);
		if (flag >= 0) fcntl (mux->fd, F_SETFD, flag | FD_CLOEXEC);
	}
	#endif

#elif defined(__OS2__)

	/* nothing special to do */

#else
	/* TODO: */
	mux->errnum = QSE_MUX_ENOIMPL;
	return -1;
#endif

	return 0;
}

void qse_mux_fini (qse_mux_t* mux)
{

#if defined(USE_SELECT)
	FD_ZERO (&mux->rset);
	FD_ZERO (&mux->wset);

	if (mux->me.ptr)
	{
		int i;

		for (i = 0; i < mux->me.ubound; i++)
		{
			if (mux->me.ptr[i]) 
				QSE_MMGR_FREE (mux->mmgr, mux->me.ptr[i]);
		}

		QSE_MMGR_FREE (mux->mmgr, mux->me.ptr);
		mux->me.ubound = 0;
		mux->maxhnd = -1;
	}

#elif defined(USE_KQUEUE)
	close (mux->kq);

	if (mux->me.ptr)
	{
		int i;

		for (i = 0; i < mux->me.ubound; i++)
		{
			if (mux->me.ptr[i]) 
				QSE_MMGR_FREE (mux->mmgr, mux->me.ptr[i]);
		}

		QSE_MMGR_FREE (mux->mmgr, mux->me.ptr);
		mux->me.ubound = 0;
	}

#elif defined(USE_EPOLL)
	close (mux->fd);

	if (mux->ee.ptr) 
	{
		QSE_MMGR_FREE (mux->mmgr, mux->ee.ptr);
		mux->ee.len = 0;
		mux->ee.capa = 0;
	}

	if (mux->me.ptr)
	{
		int i;

		for (i = 0; i < mux->me.ubound; i++)
		{
			if (mux->me.ptr[i]) 
				QSE_MMGR_FREE (mux->mmgr, mux->me.ptr[i]);
		}

		QSE_MMGR_FREE (mux->mmgr, mux->me.ptr);
		mux->me.ubound = 0;
	}

#elif defined(__OS2__)
	if (mux->me.ptr)
	{
		int i;

		for (i = 0; i < mux->me.ubound; i++)
		{
			if (mux->me.ptr[i]) 
				QSE_MMGR_FREE (mux->mmgr, mux->me.ptr[i]);
		}

		QSE_MMGR_FREE (mux->mmgr, mux->me.ptr);
		mux->me.ubound = 0;
	}

	if (mux->fdarr) QSE_MMGR_FREE (mux->mmgr, mux->fdarr);
#endif
}

qse_mmgr_t* qse_mux_getmmgr (qse_mux_t* mux)
{
	return mux->mmgr;
}

void* qse_mux_getxtn (qse_mux_t* mux)
{
	return QSE_XTN (mux);
}

qse_mux_errnum_t qse_mux_geterrnum (qse_mux_t* mux)
{
	return mux->errnum;
}

#define ALIGN_TO(num,align) ((((num) + (align) - 1) / (align)) * (align))

int qse_mux_insert (qse_mux_t* mux, const qse_mux_evt_t* evt)
{
#if defined(USE_SELECT)
	/* nothing */
#elif defined(USE_KQUEUE)
	struct kevent chlist[2];
	int count = 0;
#elif defined(USE_EPOLL)
	struct epoll_event ev;
#elif defined(__OS2__)
	/* nothing */
#else
	/* nothing */
#endif

	/* sanity check */
	if (!(evt->mask & (QSE_MUX_IN | QSE_MUX_OUT)) || evt->hnd < 0) 
	{
		mux->errnum = QSE_MUX_EINVAL;
		return -1;
	}

#if defined(USE_SELECT)

	/* TODO: windows seems to return a pretty high file descriptors
	 *       using an array to store information may not be so effcient.
	 *       devise a different way to maintain information */
	if (evt->hnd >= mux->me.ubound)
	{
		qse_mux_evt_t** tmp;
		int ubound;

		ubound = evt->hnd + 1;
		ubound = ALIGN_TO (ubound, 128);

		tmp = QSE_MMGR_REALLOC (mux->mmgr, mux->me.ptr, QSE_SIZEOF(*mux->me.ptr) * ubound);
		if (tmp == QSE_NULL)
		{
			mux->errnum = QSE_MUX_ENOMEM;
			return -1;
		}

		QSE_MEMSET (&tmp[mux->me.ubound], 0, QSE_SIZEOF(*mux->me.ptr) * (ubound - mux->me.ubound));
		mux->me.ptr = tmp;
		mux->me.ubound = ubound;
	}

	if (!mux->me.ptr[evt->hnd])
	{
		mux->me.ptr[evt->hnd] = QSE_MMGR_ALLOC (mux->mmgr, QSE_SIZEOF(*evt));
		if (!mux->me.ptr[evt->hnd])
		{
			mux->errnum = QSE_MUX_ENOMEM;
			return -1;
		}
	}

	if (evt->mask & QSE_MUX_IN) FD_SET (evt->hnd, &mux->rset);
	if (evt->mask & QSE_MUX_OUT) FD_SET (evt->hnd, &mux->wset);

	*mux->me.ptr[evt->hnd] = *evt;
	if (evt->hnd > mux->maxhnd) mux->maxhnd = evt->hnd;
	mux->size++;
	return 0;

#elif defined(USE_KQUEUE)
	/* TODO: study if it is better to put 'evt' to the udata 
	 *       field of chlist? */

	if (evt->mask & QSE_MUX_IN)
	{
		EV_SET (&chlist[count], evt->hnd, 
			EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
		count++;
	}

	if (evt->mask & QSE_MUX_OUT)
	{
		EV_SET (&chlist[count], evt->hnd,
			EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, 0);
		count++;
	}

	QSE_ASSERT (count > 0);

	if (evt->hnd >= mux->me.ubound)
	{
		qse_mux_evt_t** tmp;
		int ubound;

		ubound = evt->hnd + 1;
		ubound = ALIGN_TO (ubound, 128);

		tmp = QSE_MMGR_REALLOC (mux->mmgr, mux->me.ptr, QSE_SIZEOF(*mux->me.ptr) * ubound);
		if (tmp == QSE_NULL)
		{
			mux->errnum = QSE_MUX_ENOMEM;
			return -1;
		}

		QSE_MEMSET (&tmp[mux->me.ubound], 0, QSE_SIZEOF(*mux->me.ptr) * (ubound - mux->me.ubound));
		mux->me.ptr = tmp;
		mux->me.ubound = ubound;
	}

	if (!mux->me.ptr[evt->hnd])
	{
		mux->me.ptr[evt->hnd] = QSE_MMGR_ALLOC (mux->mmgr, QSE_SIZEOF(*evt));
		if (!mux->me.ptr[evt->hnd])
		{
			mux->errnum = QSE_MUX_ENOMEM;
			return -1;
		}
	}

	/* add the event */
	if (kevent (mux->kq, chlist, count, QSE_NULL, 0, QSE_NULL) <= -1)
	{
		mux->errnum = skerr_to_errnum (errno);
		return -1;
	}

	*mux->me.ptr[evt->hnd] = *evt;
	mux->size++;
	return 0;

#elif defined(USE_EPOLL)

	QSE_MEMSET (&ev, 0, QSE_SIZEOF(ev));
	if (evt->mask & QSE_MUX_IN) ev.events |= EPOLLIN;
	if (evt->mask & QSE_MUX_OUT) ev.events |= EPOLLOUT;

	QSE_ASSERT (ev.events != 0);

	if (evt->hnd >= mux->me.ubound)
	{
		qse_mux_evt_t** tmp;
		int ubound;

		ubound = evt->hnd + 1;
		ubound = ALIGN_TO (ubound, 128);

		tmp = QSE_MMGR_REALLOC (mux->mmgr, mux->me.ptr, QSE_SIZEOF(*mux->me.ptr) * ubound);
		if (tmp == QSE_NULL)
		{
			mux->errnum = QSE_MUX_ENOMEM;
			return -1;
		}

		QSE_MEMSET (&tmp[mux->me.ubound], 0, QSE_SIZEOF(*mux->me.ptr) * (ubound - mux->me.ubound));
		mux->me.ptr = tmp;
		mux->me.ubound = ubound;
	}

	if (!mux->me.ptr[evt->hnd])
	{
		mux->me.ptr[evt->hnd] = QSE_MMGR_ALLOC (mux->mmgr, QSE_SIZEOF(*evt));
		if (!mux->me.ptr[evt->hnd])
		{
			mux->errnum = QSE_MUX_ENOMEM;
			return -1;
		}
	}

	/*ev.data.fd = evt->hnd;*/
	ev.data.ptr = mux->me.ptr[evt->hnd];

	if (mux->ee.len >= mux->ee.capa)
	{
		struct epoll_event* tmp;
		qse_size_t newcapa;

		newcapa = (mux->ee.capa + 1) * 2;
		newcapa = ALIGN_TO (newcapa, 256);

		tmp = QSE_MMGR_REALLOC (
			mux->mmgr, mux->ee.ptr,
			QSE_SIZEOF(*mux->ee.ptr) * newcapa);
		if (tmp == QSE_NULL) 
		{
			mux->errnum = QSE_MUX_ENOMEM;
			return -1;
		}

		mux->ee.ptr = tmp;
		mux->ee.capa = newcapa;
	}

	if (epoll_ctl (mux->fd, EPOLL_CTL_ADD, evt->hnd, &ev) == -1) 
	{
		mux->errnum = skerr_to_errnum (errno);
		return -1;
	}

	*mux->me.ptr[evt->hnd] = *evt;
	mux->ee.len++;
	return 0;

#elif defined(__OS2__)

	if (evt->hnd >= mux->me.ubound)
	{
		qse_mux_evt_t** tmp;
		int* fatmp;
		int ubound;

		ubound = evt->hnd + 1;
		ubound = ALIGN_TO (ubound, 128);

		tmp = QSE_MMGR_REALLOC (mux->mmgr, mux->me.ptr, QSE_SIZEOF(*mux->me.ptr) * ubound);
		if (tmp == QSE_NULL)
		{
			mux->errnum = QSE_MUX_ENOMEM;
			return -1;
		}

		/* maintain this array double the size of the highest handle + 1 */
		fatmp = QSE_MMGR_REALLOC (mux->mmgr, mux->fdarr, QSE_SIZEOF(*mux->fdarr) * (ubound * 2));
		if (fatmp == QSE_NULL)
		{
			QSE_MMGR_FREE (mux->mmgr, tmp);
			mux->errnum = QSE_MUX_ENOMEM;
			return -1;
		}

		QSE_MEMSET (&tmp[mux->me.ubound], 0, QSE_SIZEOF(*mux->me.ptr) * (ubound - mux->me.ubound));
		mux->me.ptr = tmp;
		mux->me.ubound = ubound;
		mux->fdarr = fatmp;
	}

	if (!mux->me.ptr[evt->hnd])
	{
		mux->me.ptr[evt->hnd] = QSE_MMGR_ALLOC (mux->mmgr, QSE_SIZEOF(*evt));
		if (!mux->me.ptr[evt->hnd])
		{
			mux->errnum = QSE_MUX_ENOMEM;
			return -1;
		}
	}

	*mux->me.ptr[evt->hnd] = *evt;
	mux->size++;
	return 0;


#else
	/* TODO: */
	mux->errnum = QSE_MUX_ENOIMPL;
	return -1;
#endif
}

int qse_mux_delete (qse_mux_t* mux, const qse_mux_evt_t* evt)
{
#if defined(USE_SELECT)
	qse_mux_evt_t* mevt;

	if (mux->size <= 0 || evt->hnd < 0 || evt->hnd >= mux->me.ubound) 
	{
		mux->errnum = QSE_MUX_EINVAL;
		return -1;
	}

	mevt = mux->me.ptr[evt->hnd];
	if (!mevt || mevt->hnd != evt->hnd) 
	{
		/* already deleted??? */
		mux->errnum = QSE_MUX_EINVAL;
		return -1;
	}

	if (mevt->mask & QSE_MUX_IN) FD_CLR (evt->hnd, &mux->rset);
	if (mevt->mask & QSE_MUX_OUT) FD_CLR (evt->hnd, &mux->wset);

	if (mevt->hnd == mux->maxhnd)
	{
		qse_mux_hnd_t i;

		for (i = mevt->hnd; i > 0; )
		{
			i--;
			if (mux->me.ptr[i] && mux->me.ptr[i]->hnd >= 0)
			{
				QSE_ASSERT (i == mux->me.ptr[i]->hnd);
				mux->maxhnd = mux->me.ptr[i]->hnd;
				goto done;
			}
		}

		mux->maxhnd = -1;
		QSE_ASSERT (mux->size == 1);
	}

done:
	mevt->hnd = -1;
	mevt->mask = 0;
	mux->size--;
	return 0;	

#elif defined(USE_KQUEUE)

	qse_mux_evt_t* mevt;
	struct kevent chlist[2];
	int count = 0;

	if (mux->size <= 0 || evt->hnd < 0 || evt->hnd >= mux->me.ubound) 
	{
		mux->errnum = QSE_MUX_EINVAL;
		return -1;
	}

	mevt = mux->me.ptr[evt->hnd];
	if (!mevt || mevt->hnd != evt->hnd) 
	{
		/* already deleted??? */
		mux->errnum = QSE_MUX_EINVAL;
		return -1;
	}

	/* compose the change list */
	if (mevt->mask & QSE_MUX_IN)
	{
		EV_SET (&chlist[count], evt->hnd, 
			EVFILT_READ, EV_DELETE | EV_DISABLE, 0, 0, 0);
		count++;
	}
	if (mevt->mask & QSE_MUX_OUT) 
	{
		EV_SET (&chlist[count], evt->hnd,
			EVFILT_WRITE, EV_DELETE | EV_DISABLE, 0, 0, 0);
		count++;
	}

	/* delte the event by applying the change list */
	if (kevent (mux->kq, chlist, count, QSE_NULL, 0, QSE_NULL) <= -1)
	{
		mux->errnum = skerr_to_errnum (errno);
		return -1;
	}
	
	mux->size--;
	return 0;

#elif defined(USE_EPOLL)
	if (mux->ee.len <= 0) 
	{
		mux->errnum = QSE_MUX_EINVAL;
		return -1;
	}

	if (epoll_ctl (mux->fd, EPOLL_CTL_DEL, evt->hnd, QSE_NULL) <= -1)
	{
		mux->errnum = skerr_to_errnum(errno);
		return -1;
	}

	mux->ee.len--;
	return 0;

#elif defined(__OS2__)

	qse_mux_evt_t* mevt;

	if (mux->size <= 0 || evt->hnd < 0 || evt->hnd >= mux->me.ubound) 
	{
		mux->errnum = QSE_MUX_EINVAL;
		return -1;
	}

	mevt = mux->me.ptr[evt->hnd];
	if (!mevt || mevt->hnd != evt->hnd) 
	{
		/* already deleted??? */
		mux->errnum = QSE_MUX_EINVAL;
		return -1;
	}

	mevt->hnd = -1;
	mevt->mask = 0;
	mux->size--;
	return 0;	

#else
	/* TODO */
	mux->errnum = QSE_MUX_ENOIMPL;
	return -1;
#endif
}

int qse_mux_poll (qse_mux_t* mux, const qse_ntime_t* tmout)
{
#if defined(USE_SELECT)
	struct timeval tv;
	int n;

	tv.tv_sec = tmout->sec;
	tv.tv_usec = QSE_NSEC_TO_USEC (tmout->nsec);

	mux->tmprset = mux->rset;
	mux->tmpwset = mux->wset;

	n = select (mux->maxhnd + 1, &mux->tmprset, &mux->tmpwset, QSE_NULL, &tv); 
	if (n <= -1)
	{
	#if defined(_WIN32)
		mux->errnum = skerr_to_errnum(WSAGetLastError());
	#elif defined(__OS2__)
		mux->errnum = skerr_to_errnum(sock_errno());
	#else
		mux->errnum = skerr_to_errnum(errno);
	#endif
		return -1;
	}

	if (n > 0)
	{
		qse_mux_hnd_t i;
		qse_mux_evt_t* evt, xevt;

		for (i = 0; i <= mux->maxhnd; i++)
		{
			evt = mux->me.ptr[i];
			if (!evt || evt->hnd != i) continue;

			xevt = *evt;
			xevt.mask = 0;
			if ((evt->mask & QSE_MUX_IN) && 
			    FD_ISSET(evt->hnd, &mux->tmprset)) xevt.mask |= QSE_MUX_IN;
			if ((evt->mask & QSE_MUX_OUT) &&
			    FD_ISSET(evt->hnd, &mux->tmpwset)) xevt.mask |= QSE_MUX_OUT;

			if (xevt.mask > 0) mux->evtfun (mux, &xevt);
		}
	}

	return n;

#elif defined(USE_KQUEUE)
	int nevs;
	struct timespec ts;

	ts.tv_sec = tmout->sec;
	ts.tv_nsec = tmout->nsec;

	/* wait for events */
	nevs = kevent (mux->kq, QSE_NULL, 0, mux->evlist, QSE_COUNTOF(mux->evlist), &ts);
	if (nevs <= -1) 
	{
		mux->errnum = skerr_to_errnum(errno);
		return -1;
	}

	if (nevs > 0)
	{
		int i;
		qse_mux_hnd_t fd;
		qse_mux_evt_t* evt, xevt;

		for (i = 0; i < nevs; i++)
		{
			if (mux->evlist[i].flags & EV_ERROR) continue;

			fd = mux->evlist[i].ident;
			evt = mux->me.ptr[fd];
			if (!evt || evt->hnd != fd) continue;

			xevt = *evt;
			xevt.mask = 0;

			if ((evt->mask & QSE_MUX_IN) && 
			    mux->evlist[i].filter == EVFILT_READ) xevt.mask |= QSE_MUX_IN;
			if ((evt->mask & QSE_MUX_OUT) &&
			    mux->evlist[i].filter == EVFILT_WRITE) xevt.mask |= QSE_MUX_OUT;

			if (xevt.mask > 0) mux->evtfun (mux, &xevt);
		}
	}

	return nevs;

#elif defined(USE_EPOLL)
	int nfds, i;
	qse_mux_evt_t* evt, xevt;

	nfds = epoll_wait (
		mux->fd, mux->ee.ptr, mux->ee.len, 
		QSE_SECNSEC_TO_MSEC(tmout->sec,tmout->nsec)
	);
	if (nfds <= -1)
	{
		mux->errnum = skerr_to_errnum(errno);
		return -1;
	}

	for (i = 0; i < nfds; i++) 
	{
		/*int hnd = mux->ee.ptr[i].data.fd;
		evt = mux->me.ptr[hnd];
		QSE_ASSERT (evt->hnd == hnd); */

		evt = mux->ee.ptr[i].data.ptr;

		xevt = *evt;
		xevt.mask = 0;
		if (mux->ee.ptr[i].events & EPOLLIN) xevt.mask |= QSE_MUX_IN;
		if (mux->ee.ptr[i].events & EPOLLOUT) xevt.mask |= QSE_MUX_OUT;

		if (mux->ee.ptr[i].events & (EPOLLHUP | EPOLLERR))
		{
			if (evt->mask & QSE_MUX_IN) xevt.mask |= QSE_MUX_IN;
			if (evt->mask & QSE_MUX_OUT) xevt.mask |= QSE_MUX_OUT;
		}

		if (xevt.mask > 0) mux->evtfun (mux, &xevt);
	}

	return nfds;

#elif defined(__OS2__)

	qse_mux_evt_t* evt;
	long tv;
	int n, i, count, rcount, wcount;
	
	tv = QSE_SEC_TO_MSEC(tmout->sec) + QSE_NSEC_TO_MSEC (tmout->nsec);

	/* 
	 * be aware that reconstructing this array every time is pretty 
	 * inefficient.
	 */
	count = 0;
	for (i = 0; i < mux->me.ubound; i++)
	{
		evt = mux->me.ptr[i];
		if (evt && (evt->mask & QSE_MUX_IN)) mux->fdarr[count++] = evt->hnd;
	}
	rcount = count;
	for (i = 0; i < mux->me.ubound; i++)
	{
		evt = mux->me.ptr[i];
		if (evt && (evt->mask & QSE_MUX_OUT)) mux->fdarr[count++] = evt->hnd;
	}
	wcount = count - rcount;

	n = os2_select (mux->fdarr, rcount, wcount, 0, tv);
	if (n <= -1)
	{
		mux->errnum = skerr_to_errnum(sock_errno());
		return -1;
	}

	if (n >= 1)
	{
		qse_mux_evt_t xevt;

		for (i = 0; i < count; i++)
		{
			if (mux->fdarr[i] == -1) continue;

			evt = mux->me.ptr[mux->fdarr[i]];
			if (!evt || evt->hnd != mux->fdarr[i]) continue;

			xevt = *evt;

			/* due to the way i check 'fdarr' , it can't have
			 * both IN and OUT at the same time. they are 
			 * triggered separately */
			xevt.mask = (i < rcount)? QSE_MUX_IN: QSE_MUX_OUT;
			mux->evtfun (mux, &xevt);
		}
	}

	return n;


#else
	/* TODO */
	mux->errnum = QSE_MUX_ENOIMPL;
	return -1;
#endif

}

