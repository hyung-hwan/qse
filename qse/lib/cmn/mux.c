/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qse/cmn/mux.h>
#include "mem.h"

#if defined(_WIN32)
#	define FD_SETSIZE 4096 /* what is the best value??? */
#	include <winsock2.h>
#	include <windows.h>
#	define USE_SELECT
#elif defined(__OS2__)
#	include <types.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <tcpustd.h>
#	include <sys/ioctl.h>
#	include <nerrno.h>
#	define INCL_DOSERRORS
#	include <os2.h>
#	pragma library("tcpip32.lib")

#elif defined(__DOS__)
#	include <errno.h>
#else
#	include <unistd.h>
#	include <fcntl.h>
#	include <errno.h>
#	if defined(HAVE_SYS_TIME_H)
#		include <sys/time.h>
#	endif
#	if defined(HAVE_SYS_EPOLL_H)
#		include <sys/epoll.h>
#		if defined(HAVE_EPOLL_CREATE)
#			define USE_EPOLL
#		endif
#	elif defined(HAVE_POLL_H)
#		define USE_POLL
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
#endif
};

int qse_mux_init (qse_mux_t* mux, qse_mmgr_t* mmgr, qse_mux_evtfun_t evtfun, qse_size_t capahint);
void qse_mux_fini (qse_mux_t* mux);

#if defined(_WIN32)
static qse_mux_errnum_t syserr_to_errnum (DWORD e)
{

	switch (e)
	{
		case ERROR_NOT_ENOUGH_MEMORY:
		case ERROR_OUTOFMEMORY:
			return QSE_MUX_ENOMEM;

		case ERROR_INVALID_PARAMETER:
		case ERROR_INVALID_HANDLE:
		case ERROR_INVALID_NAME:
			return QSE_MUX_EINVAL;

		case ERROR_ACCESS_DENIED:
			return QSE_MUX_EACCES;

		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			return QSE_MUX_ENOENT;

		case ERROR_ALREADY_EXISTS:
		case ERROR_FILE_EXISTS:
			return QSE_MUX_EEXIST;

		default:
			return QSE_MUX_ESYSERR;
	}
}
#elif defined(__OS2__)
static qse_mux_errnum_t syserr_to_errnum (APIRET e)
{
	switch (e)
	{
		case ERROR_NOT_ENOUGH_MEMORY:
			return QSE_MUX_ENOMEM;

		case ERROR_INVALID_PARAMETER:
		case ERROR_INVALID_HANDLE:
		case ERROR_INVALID_NAME:
			return QSE_MUX_EINVAL;

		case ERROR_ACCESS_DENIED:
			return QSE_MUX_EACCES;

		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			return QSE_MUX_ENOENT;

		case ERROR_ALREADY_EXISTS:
			return QSE_MUX_EEXIST;

		default:
			return QSE_MUX_ESYSERR;
	}
}
#elif defined(__DOS__)
static qse_mux_errnum_t syserr_to_errnum (int e)
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
	
		default:
			return QSE_MUX_ESYSERR;
	}
}

#else
static qse_mux_errnum_t syserr_to_errnum (int e)
{
	switch (e)
	{
		case ENOMEM:
			return QSE_MUX_ENOMEM;

		case EINVAL:
			return QSE_MUX_EINVAL;

		case ENOENT:
			return QSE_MUX_ENOENT;

		case EACCES:
			return QSE_MUX_EACCES;

		case EEXIST:
			return QSE_MUX_EEXIST;
	
		case EINTR:
			return QSE_MUX_EINTR;

		default:
			return QSE_MUX_ESYSERR;
	}
}
#endif

qse_mux_t* qse_mux_open (qse_mmgr_t* mmgr, qse_size_t xtnsize, qse_mux_evtfun_t evtfun, qse_size_t capahint)
{
	qse_mux_t* mux;
	
	mux = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(*mux) + xtnsize);
	if (mux)
	{
		if (qse_mux_init (mux, mmgr, evtfun, capahint) <= -1)
		{
			QSE_MMGR_FREE (mmgr, mux);
			mux = QSE_NULL;
		}
		else QSE_MEMSET (mux + 1, 0, xtnsize);
	}

	return mux;
}

void qse_mux_close (qse_mux_t* mux)
{
	qse_mux_fini (mux);
	QSE_MMGR_FREE (mux->mmgr, mux);
}

int qse_mux_init (qse_mux_t* mux, qse_mmgr_t* mmgr, qse_mux_evtfun_t evtfun, qse_size_t capahint)
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

#elif defined(USE_EPOLL) 
	#if defined(HAVE_EPOLL_CREATE1) && defined(O_CLOEXEC)
	mux->fd = epoll_create1 (O_CLOEXEC);
	#else
	mux->fd = epoll_create (capahint);
	#endif
	if (mux->fd <= -1) 
	{
		mux->errnum = syserr_to_errnum (errno);
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

#define ALIGN_TO(num,align) ((((num) + (align) - 1) / (align)) * (align))

int qse_mux_insert (qse_mux_t* mux, const qse_mux_evt_t* evt)
{
#if defined(USE_SELECT)

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

#elif defined(USE_EPOLL)
	struct epoll_event ev;

	QSE_MEMSET (&ev, 0, QSE_SIZEOF(ev));
	if (evt->mask & QSE_MUX_IN) ev.events |= EPOLLIN;
	if (evt->mask & QSE_MUX_OUT) ev.events |= EPOLLOUT;

	if (ev.events == 0 || evt->hnd < 0)
	{
		mux->errnum = QSE_MUX_EINVAL;
		return -1;
	}

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
		mux->errnum = syserr_to_errnum (errno);
		return -1;
	}

	*mux->me.ptr[evt->hnd] = *evt;
	mux->ee.len++;
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
	if (mevt->hnd != evt->hnd) 
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
		mux->errnum = syserr_to_errnum(errno);
		return -1;
	}

	mux->ee.len--;
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
		mux->errnum = syserr_to_errnum(WSAGetLastError());
	#else
		mux->errnum = syserr_to_errnum(errno);
	#endif
		return -1;
	}

	if (n > 0)
	{
		qse_mux_hnd_t i;
		qse_mux_evt_t* evt, xevt;

		for (i = 0 ; i <= mux->maxhnd; i++)
		{
			evt = mux->me.ptr[i];
			if (!evt || evt->hnd != i) continue;

			QSE_MEMCPY (&xevt, evt, QSE_SIZEOF(xevt));

			xevt.mask = 0;
			if ((evt->mask & QSE_MUX_IN) && FD_ISSET(evt->hnd, &mux->tmprset)) xevt.mask |= QSE_MUX_IN;
			if ((evt->mask & QSE_MUX_OUT) && FD_ISSET(evt->hnd, &mux->tmpwset)) xevt.mask |= QSE_MUX_OUT;

			if (xevt.mask > 0) mux->evtfun (mux, &xevt);
		}
	}

	return n;

#elif defined(USE_EPOLL)
	int nfds, i;
	qse_mux_evt_t* evt, xevt;

	nfds = epoll_wait (mux->fd, mux->ee.ptr, mux->ee.len, QSE_SECNSEC_TO_MSEC(tmout->sec,tmout->nsec));
	if (nfds <= -1)
	{
		mux->errnum = syserr_to_errnum(errno);
		return -1;
	}

	for (i = 0; i < nfds; i++) 
	{
		/*int hnd = mux->ee.ptr[i].data.fd;
		evt = mux->me.ptr[hnd];
		QSE_ASSERT (evt->hnd == hnd); */

		evt = mux->ee.ptr[i].data.ptr;

		QSE_MEMCPY (&xevt, evt, QSE_SIZEOF(xevt));

		xevt.mask = 0;
		if (mux->ee.ptr[i].events & EPOLLIN) xevt.mask |= QSE_MUX_IN;
		if (mux->ee.ptr[i].events & EPOLLOUT) xevt.mask |= QSE_MUX_OUT;

		if (mux->ee.ptr[i].events & EPOLLHUP)
		{
			if (evt->mask & QSE_MUX_IN) xevt.mask |= QSE_MUX_IN;
			if (evt->mask & QSE_MUX_OUT) xevt.mask |= QSE_MUX_OUT;
		}

		mux->evtfun (mux, &xevt);
	}

	return nfds;

#else
	/* TODO */
	mux->errnum = QSE_MUX_ENOIMPL;
	return -1;
#endif

}

