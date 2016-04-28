/*
 * $Id$
 *
    Copyright (c) 2006-2016 Chung, Hyung-Hwan. All rights reserved.

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

#include "aio-prv.h"
  
#if defined(HAVE_SYS_EPOLL_H)
#	include <sys/epoll.h>
#	define USE_EPOLL
#elif defined(HAVE_SYS_POLL_H)
#	include <sys/poll.h>
#	define USE_POLL
#else
#	error NO SUPPORTED MULTIPLEXER
#endif

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#define DEV_CAPA_ALL_WATCHED (QSE_AIO_DEV_CAPA_IN_WATCHED | QSE_AIO_DEV_CAPA_OUT_WATCHED | QSE_AIO_DEV_CAPA_PRI_WATCHED)

static int schedule_kill_zombie_job (qse_aio_dev_t* dev);
static int kill_and_free_device (qse_aio_dev_t* dev, int force);

#define APPEND_DEVICE_TO_LIST(list,dev) do { \
	if ((list)->tail) (list)->tail->dev_next = (dev); \
	else (list)->head = (dev); \
	(dev)->dev_prev = (list)->tail; \
	(dev)->dev_next = QSE_NULL; \
	(list)->tail = (dev); \
} while(0)

#define UNLINK_DEVICE_FROM_LIST(list,dev) do { \
	if ((dev)->dev_prev) (dev)->dev_prev->dev_next = (dev)->dev_next; \
	else (list)->head = (dev)->dev_next; \
	if ((dev)->dev_next) (dev)->dev_next->dev_prev = (dev)->dev_prev; \
	else (list)->tail = (dev)->dev_prev; \
} while (0)



/* ========================================================================= */
#if defined(USE_POLL)

#define MUX_CMD_INSERT 1
#define MUX_CMD_UPDATE 2
#define MUX_CMD_DELETE 3

#define MUX_INDEX_INVALID QSE_AIO_TYPE_MAX(qse_size_t)

struct qse_aio_mux_t
{
	struct
	{
		qse_size_t* ptr;
		qse_size_t  size;
		qse_size_t  capa;
	} map; /* handle to index */

	struct
	{
		struct pollfd* pfd;
		qse_aio_dev_t** dptr;
		qse_size_t size;
		qse_size_t capa;
	} pd; /* poll data */
};


static int mux_open (qse_aio_t* aio)
{
	qse_aio_mux_t* mux;

	mux = QSE_MMGR_ALLOC (aio->mmgr, QSE_SIZEOF(*mux));
	if (!mux)
	{
		aio->errnum = QSE_AIO_ENOMEM;
		return -1;
	}

	QSE_MEMSET (mux, 0, QSE_SIZEOF(*mux));

	aio->mux = mux;
	return 0;
}

static void mux_close (qse_aio_t* aio)
{
	if (aio->mux)
	{
		QSE_MMGR_FREE (aio->mmgr, aio->mux);
		aio->mux = QSE_NULL;
	}
}

static int mux_control (qse_aio_dev_t* dev, int cmd, qse_aio_syshnd_t hnd, int dev_capa)
{
	qse_aio_t* aio;
	qse_aio_mux_t* mux;
	qse_size_t idx;

	aio = dev->aio;
	mux = (qse_aio_mux_t*)aio->mux;

	if (hnd >= mux->map.capa)
	{
		qse_size_t new_capa;
		qse_size_t* tmp;

		if (cmd != MUX_CMD_INSERT)
		{
			aio->errnum = QSE_AIO_ENOENT;
			return -1;
		}

		new_capa = QSE_AIO_ALIGNTO_POW2((hnd + 1), 256);

		tmp = QSE_MMGR_REALLOC (aio->mmgr, mux->map.ptr, new_capa * QSE_SIZEOF(*tmp));
		if (!tmp)
		{
			aio->errnum = QSE_AIO_ENOMEM;
			return -1;
		}

		for (idx = mux->map.capa; idx < new_capa; idx++) 
			tmp[idx] = MUX_INDEX_INVALID;

		mux->map.ptr = tmp;
		mux->map.capa = new_capa;
	}

	idx = mux->map.ptr[hnd];
	if (idx != MUX_INDEX_INVALID)
	{
		if (cmd == MUX_CMD_INSERT)
		{
			aio->errnum = QSE_AIO_EEXIST;
			return -1;
		}
	}
	else
	{
		if (cmd != MUX_CMD_INSERT)
		{
			aio->errnum = QSE_AIO_ENOENT;
			return -1;
		}
	}

	switch (cmd)
	{
		case MUX_CMD_INSERT:

			if (mux->pd.size >= mux->pd.capa)
			{
				qse_size_t new_capa;
				struct pollfd* tmp1;
				qse_aio_dev_t** tmp2;

				new_capa = QSE_AIO_ALIGNTO_POW2(mux->pd.size + 1, 256);

				tmp1 = QSE_MMGR_REALLOC (aio->mmgr, mux->pd.pfd, new_capa * QSE_SIZEOF(*tmp1));
				if (!tmp1)
				{
					aio->errnum = QSE_AIO_ENOMEM;
					return -1;
				}

				tmp2 = QSE_MMGR_REALLOC (aio->mmgr, mux->pd.dptr, new_capa * QSE_SIZEOF(*tmp2));
				if (!tmp2)
				{
					QSE_MMGR_FREE (aio->mmgr, tmp1);
					aio->errnum = QSE_AIO_ENOMEM;
					return -1;
				}

				mux->pd.pfd = tmp1;
				mux->pd.dptr = tmp2;
				mux->pd.capa = new_capa;
			}

			idx = mux->pd.size++;

			mux->pd.pfd[idx].fd = hnd;
			mux->pd.pfd[idx].events = 0;
			if (dev_capa & QSE_AIO_DEV_CAPA_IN_WATCHED) mux->pd.pfd[idx].events |= POLLIN;
			if (dev_capa & QSE_AIO_DEV_CAPA_OUT_WATCHED) mux->pd.pfd[idx].events |= POLLOUT;
			mux->pd.pfd[idx].revents = 0;
			mux->pd.dptr[idx] = dev;

			mux->map.ptr[hnd] = idx;

			return 0;

		case MUX_CMD_UPDATE:
			QSE_ASSERT (mux->pd.dptr[idx] == dev);
			mux->pd.pfd[idx].events = 0;
			if (dev_capa & QSE_AIO_DEV_CAPA_IN_WATCHED) mux->pd.pfd[idx].events |= POLLIN;
			if (dev_capa & QSE_AIO_DEV_CAPA_OUT_WATCHED) mux->pd.pfd[idx].events |= POLLOUT;
			return 0;

		case MUX_CMD_DELETE:
			QSE_ASSERT (mux->pd.dptr[idx] == dev);
			mux->map.ptr[hnd] = MUX_INDEX_INVALID;

			/* TODO: speed up deletion. allow a hole in the array.
			 *       delay array compaction if there is a hole.
			 *       set fd for the hole to -1 such that poll()
			 *       ignores it. compact the array if another deletion 
			 *       is requested when there is an existing hole. */
			idx++;
			while (idx < mux->pd.size)
			{
				int fd;

				mux->pd.pfd[idx - 1] = mux->pd.pfd[idx];
				mux->pd.dptr[idx - 1] = mux->pd.dptr[idx];

				fd = mux->pd.pfd[idx].fd;
				mux->map.ptr[fd] = idx - 1;

				idx++;
			}

			mux->pd.size--;

			return 0;

		default:
			aio->errnum = QSE_AIO_EINVAL;
			return -1;
	}
}

#elif defined(USE_EPOLL)

#define MUX_CMD_INSERT EPOLL_CTL_ADD
#define MUX_CMD_UPDATE EPOLL_CTL_MOD
#define MUX_CMD_DELETE EPOLL_CTL_DEL

struct qse_aio_mux_t
{
	int hnd;
	struct epoll_event revs[100]; /* TODO: is it a good size? */
};

static int mux_open (qse_aio_t* aio)
{
	qse_aio_mux_t* mux;

	mux = QSE_MMGR_ALLOC (aio->mmgr, QSE_SIZEOF(*mux));
	if (!mux)
	{
		aio->errnum = QSE_AIO_ENOMEM;
		return -1;
	}

	QSE_MEMSET (mux, 0, QSE_SIZEOF(*mux));

	mux->hnd = epoll_create (1000);
	if (mux->hnd == -1)
	{
		aio->errnum = qse_aio_syserrtoerrnum(errno);
		QSE_MMGR_FREE (aio->mmgr, mux);
		return -1;
	}

	aio->mux = mux;
	return 0;
}

static void mux_close (qse_aio_t* aio)
{
	if (aio->mux)
	{
		close (aio->mux->hnd);
		QSE_MMGR_FREE (aio->mmgr, aio->mux);
		aio->mux = QSE_NULL;
	}
}


static QSE_INLINE int mux_control (qse_aio_dev_t* dev, int cmd, qse_aio_syshnd_t hnd, int dev_capa)
{
	struct epoll_event ev;

	ev.data.ptr = dev;
	ev.events = EPOLLHUP | EPOLLERR /*| EPOLLET*/;

	if (dev_capa & QSE_AIO_DEV_CAPA_IN_WATCHED) 
	{
		ev.events |= EPOLLIN;
	#if defined(EPOLLRDHUP)
		ev.events |= EPOLLRDHUP;
	#endif
		if (dev_capa & QSE_AIO_DEV_CAPA_PRI_WATCHED) ev.events |= EPOLLPRI;
	}

	if (dev_capa & QSE_AIO_DEV_CAPA_OUT_WATCHED) ev.events |= EPOLLOUT;

	if (epoll_ctl (dev->aio->mux->hnd, cmd, hnd, &ev) == -1)
	{
		dev->aio->errnum = qse_aio_syserrtoerrnum(errno);
		return -1;
	}

	return 0;
}
#endif

/* ========================================================================= */

qse_aio_t* qse_aio_open (qse_mmgr_t* mmgr, qse_size_t xtnsize, qse_size_t tmrcapa, qse_aio_errnum_t* errnum)
{
	qse_aio_t* aio;

	aio = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_aio_t) + xtnsize);
	if (aio)
	{
		if (qse_aio_init (aio, mmgr, tmrcapa) <= -1)
		{
			if (errnum) *errnum = aio->errnum;
			QSE_MMGR_FREE (mmgr, aio);
			aio = QSE_NULL;
		}
		else QSE_MEMSET (aio + 1, 0, xtnsize);
	}
	else
	{
		if (errnum) *errnum = QSE_AIO_ENOMEM;
	}

	return aio;
}

void qse_aio_close (qse_aio_t* aio)
{
	qse_aio_fini (aio);
	QSE_MMGR_FREE (aio->mmgr, aio);
}

int qse_aio_init (qse_aio_t* aio, qse_mmgr_t* mmgr, qse_size_t tmrcapa)
{
	QSE_MEMSET (aio, 0, QSE_SIZEOF(*aio));
	aio->mmgr = mmgr;

	/* intialize the multiplexer object */

	if (mux_open (aio) <= -1) return -1;

	/* initialize the timer object */
	if (tmrcapa <= 0) tmrcapa = 1;
	aio->tmr.jobs = QSE_MMGR_ALLOC (aio->mmgr, tmrcapa * QSE_SIZEOF(qse_aio_tmrjob_t));
	if (!aio->tmr.jobs) 
	{
		aio->errnum = QSE_AIO_ENOMEM;
		mux_close (aio);
		return -1;
	}
	aio->tmr.capa = tmrcapa;

	return 0;
}

void qse_aio_fini (qse_aio_t* aio)
{
	qse_aio_dev_t* dev, * next_dev;
	struct
	{
		qse_aio_dev_t* head;
		qse_aio_dev_t* tail;
	} diehard;

	/* kill all registered devices */
	while (aio->actdev.head)
	{
		qse_aio_killdev (aio, aio->actdev.head);
	}

	while (aio->hltdev.head)
	{
		qse_aio_killdev (aio, aio->hltdev.head);
	}

	/* clean up all zombie devices */
	QSE_MEMSET (&diehard, 0, QSE_SIZEOF(diehard));
	for (dev = aio->zmbdev.head; dev; )
	{
		kill_and_free_device (dev, 1);
		if (aio->zmbdev.head == dev) 
		{
			/* the deive has not been freed. go on to the next one */
			next_dev = dev->dev_next;

			/* remove the device from the zombie device list */
			UNLINK_DEVICE_FROM_LIST (&aio->zmbdev, dev);
			dev->dev_capa &= ~QSE_AIO_DEV_CAPA_ZOMBIE;

			/* put it to a private list for aborting */
			APPEND_DEVICE_TO_LIST (&diehard, dev);

			dev = next_dev;
		}
		else dev = aio->zmbdev.head;
	}

	while (diehard.head)
	{
		/* if the kill method returns failure, it can leak some resource
		 * because the device is freed regardless of the failure when 2 
		 * is given to kill_and_free_device(). */
		dev = diehard.head;
		QSE_ASSERT (!(dev->dev_capa & (QSE_AIO_DEV_CAPA_ACTIVE | QSE_AIO_DEV_CAPA_HALTED | QSE_AIO_DEV_CAPA_ZOMBIE)));
		UNLINK_DEVICE_FROM_LIST (&diehard, dev);
		kill_and_free_device (dev, 2);
	}

	/* purge scheduled timer jobs and kill the timer */
	qse_aio_cleartmrjobs (aio);
	QSE_MMGR_FREE (aio->mmgr, aio->tmr.jobs);

	/* close the multiplexer */
	mux_close (aio);
}


int qse_aio_prologue (qse_aio_t* aio)
{
	/* TODO: */
	return 0;
}

void qse_aio_epilogue (qse_aio_t* aio)
{
	/* TODO: */
}

static QSE_INLINE void unlink_wq (qse_aio_t* aio, qse_aio_wq_t* q)
{
	if (q->tmridx != QSE_AIO_TMRIDX_INVALID)
	{
		qse_aio_deltmrjob (aio, q->tmridx);
		QSE_ASSERT (q->tmridx == QSE_AIO_TMRIDX_INVALID);
	}
	QSE_AIO_WQ_UNLINK (q);
}

static QSE_INLINE void handle_event (qse_aio_dev_t* dev, int events, int rdhup)
{
	qse_aio_t* aio;

	aio = dev->aio;
	aio->renew_watch = 0;

	QSE_ASSERT (aio == dev->aio);

	if (dev->dev_evcb->ready)
	{
		int x, xevents;

		xevents = events;
		if (rdhup) xevents |= QSE_AIO_DEV_EVENT_HUP;

		/* return value of ready()
		 *   <= -1 - failure. kill the device.
		 *   == 0 - ok. but don't invoke recv() or send().
		 *   >= 1 - everything is ok. */
		x = dev->dev_evcb->ready (dev, xevents);
		if (x <= -1)
		{
			qse_aio_dev_halt (dev);
			return;
		}
		else if (x == 0) goto skip_evcb;
	}

	if (dev && (events & QSE_AIO_DEV_EVENT_PRI))
	{
		/* urgent data */
		/* TODO: implement urgent data handling */
		/*x = dev->dev_mth->urgread (dev, aio->bugbuf, &len);*/
	}

	if (dev && (events & QSE_AIO_DEV_EVENT_OUT))
	{
		/* write pending requests */
		while (!QSE_AIO_WQ_ISEMPTY(&dev->wq))
		{
			qse_aio_wq_t* q;
			const qse_uint8_t* uptr;
			qse_aio_iolen_t urem, ulen;
			int x;

			q = QSE_AIO_WQ_HEAD(&dev->wq);

			uptr = q->ptr;
			urem = q->len;

		send_leftover:
			ulen = urem;
			x = dev->dev_mth->write (dev, uptr, &ulen, &q->dstaddr);
			if (x <= -1)
			{
				qse_aio_dev_halt (dev);
				dev = QSE_NULL;
				break;
			}
			else if (x == 0)
			{
				/* keep the left-over */
				QSE_MEMMOVE (q->ptr, uptr, urem);
				q->len = urem;
				break;
			}
			else
			{
				uptr += ulen;
				urem -= ulen;

				if (urem <= 0)
				{
					/* finished writing a single write request */
					int y, out_closed = 0;

					if (q->len <= 0 && (dev->dev_capa & QSE_AIO_DEV_CAPA_STREAM)) 
					{
						/* it was a zero-length write request. 
						 * for a stream, it is to close the output. */
						dev->dev_capa |= QSE_AIO_DEV_CAPA_OUT_CLOSED;
						aio->renew_watch = 1;
						out_closed = 1;
					}

					unlink_wq (aio, q);
					y = dev->dev_evcb->on_write (dev, q->olen, q->ctx, &q->dstaddr);
					QSE_MMGR_FREE (aio->mmgr, q);

					if (y <= -1)
					{
						qse_aio_dev_halt (dev);
						dev = QSE_NULL;
						break;
					}

					if (out_closed)
					{
						/* drain all pending requests. 
						 * callbacks are skipped for drained requests */
						while (!QSE_AIO_WQ_ISEMPTY(&dev->wq))
						{
							q = QSE_AIO_WQ_HEAD(&dev->wq);
							unlink_wq (aio, q);
							QSE_MMGR_FREE (dev->aio->mmgr, q);
						}
						break;
					}
				}
				else goto send_leftover;
			}
		}

		if (dev && QSE_AIO_WQ_ISEMPTY(&dev->wq))
		{
			/* no pending request to write */
			if ((dev->dev_capa & QSE_AIO_DEV_CAPA_IN_CLOSED) &&
			    (dev->dev_capa & QSE_AIO_DEV_CAPA_OUT_CLOSED))
			{
				qse_aio_dev_halt (dev);
				dev = QSE_NULL;
			}
			else
			{
				aio->renew_watch = 1;
			}
		}
	}

	if (dev && (events & QSE_AIO_DEV_EVENT_IN))
	{
		qse_aio_devaddr_t srcaddr;
		qse_aio_iolen_t len;
		int x;

		/* the devices are all non-blocking. read as much as possible
		 * if on_read callback returns 1 or greater. read only once
		 * if the on_read calllback returns 0. */
		while (1)
		{
			len = QSE_COUNTOF(aio->bigbuf);
			x = dev->dev_mth->read (dev, aio->bigbuf, &len, &srcaddr);
			if (x <= -1)
			{
				qse_aio_dev_halt (dev);
				dev = QSE_NULL;
				break;
			}
			else if (x == 0)
			{
				/* no data is available - EWOULDBLOCK or something similar */
				break;
			}
			else if (x >= 1)
			{
				if (len <= 0 && (dev->dev_capa & QSE_AIO_DEV_CAPA_STREAM)) 
				{
					/* EOF received. for a stream device, a zero-length 
					 * read is interpreted as EOF. */
					dev->dev_capa |= QSE_AIO_DEV_CAPA_IN_CLOSED;
					aio->renew_watch = 1;

					/* call the on_read callback to report EOF */
					if (dev->dev_evcb->on_read (dev, aio->bigbuf, len, &srcaddr) <= -1 ||
					    (dev->dev_capa & QSE_AIO_DEV_CAPA_OUT_CLOSED))
					{
						/* 1. input ended and its reporting failed or 
						 * 2. input ended and no writing is possible */
						qse_aio_dev_halt (dev);
						dev = QSE_NULL;
					}

					/* since EOF is received, reading can't be greedy */
					break;
				}
				else
				{
					int y;
		/* TODO: for a stream device, merge received data if bigbuf isn't full and fire the on_read callback
		 *        when x == 0 or <= -1. you can  */

					/* data available */
					y = dev->dev_evcb->on_read (dev, aio->bigbuf, len, &srcaddr);
					if (y <= -1)
					{
						qse_aio_dev_halt (dev);
						dev = QSE_NULL;
						break;
					}
					else if (y == 0)
					{
						/* don't be greedy. read only once 
						 * for this loop iteration */
						break;
					}
				}
			}
		}
	}

	if (dev)
	{
		if (events & (QSE_AIO_DEV_EVENT_ERR | QSE_AIO_DEV_EVENT_HUP))
		{ 
			/* if error or hangup has been reported on the device,
			 * halt the device. this check is performed after
			 * EPOLLIN or EPOLLOUT check because EPOLLERR or EPOLLHUP
			 * can be set together with EPOLLIN or EPOLLOUT. */
			dev->dev_capa |= QSE_AIO_DEV_CAPA_IN_CLOSED | QSE_AIO_DEV_CAPA_OUT_CLOSED;
			aio->renew_watch = 1;
		}
		else if (dev && rdhup) 
		{
			if (events & (QSE_AIO_DEV_EVENT_IN | QSE_AIO_DEV_EVENT_OUT | QSE_AIO_DEV_EVENT_PRI))
			{
				/* it may be a half-open state. don't do anything here
				 * to let the next read detect EOF */
			}
			else
			{
				dev->dev_capa |= QSE_AIO_DEV_CAPA_IN_CLOSED | QSE_AIO_DEV_CAPA_OUT_CLOSED;
				aio->renew_watch = 1;
			}
		}

		if ((dev->dev_capa & QSE_AIO_DEV_CAPA_IN_CLOSED) &&
		    (dev->dev_capa & QSE_AIO_DEV_CAPA_OUT_CLOSED))
		{
			qse_aio_dev_halt (dev);
			dev = QSE_NULL;
		}
	}

skip_evcb:
	if (dev && aio->renew_watch && qse_aio_dev_watch (dev, QSE_AIO_DEV_WATCH_RENEW, 0) <= -1)
	{
		qse_aio_dev_halt (dev);
		dev = QSE_NULL;
	}
}

static QSE_INLINE int __exec (qse_aio_t* aio)
{
	qse_ntime_t tmout;

#if defined(_WIN32)
	ULONG nentries, i;
#else
	int nentries, i;
	qse_aio_mux_t* mux;
#endif

	/*if (!aio->actdev.head) return 0;*/

	/* execute the scheduled jobs before checking devices with the 
	 * multiplexer. the scheduled jobs can safely destroy the devices */
	qse_aio_firetmrjobs (aio, QSE_NULL, QSE_NULL);

	if (qse_aio_gettmrtmout (aio, QSE_NULL, &tmout) <= -1)
	{
		/* defaults to 1 second if timeout can't be acquired */
		tmout.sec = 1; /* TODO: make the default timeout configurable */
		tmout.nsec = 0;
	}

#if defined(_WIN32)
/*
	if (GetQueuedCompletionStatusEx (aio->iocp, aio->ovls, QSE_COUNTOF(aio->ovls), &nentries, timeout, FALSE) == FALSE)
	{
		// TODO: set errnum 
		return -1;
	}

	for (i = 0; i < nentries; i++)
	{
	}
*/
#elif defined(USE_POLL)

	mux = (qse_aio_mux_t*)aio->mux;

	nentries = poll (mux->pd.pfd, mux->pd.size, QSE_SECNSEC_TO_MSEC(tmout.sec, tmout.nsec));
	if (nentries == -1)
	{
		if (errno == EINTR) return 0;
		aio->errnum = qse_aio_syserrtoerrnum(errno);
		return -1;
	}

	for (i = 0; i < mux->pd.size; i++)
	{
		if (mux->pd.pfd[i].fd >= 0 && mux->pd.pfd[i].revents)
		{
			int events = 0;
			qse_aio_dev_t* dev;

			dev = mux->pd.dptr[i];

			QSE_ASSERT (!(mux->pd.pfd[i].revents & POLLNVAL));
			if (mux->pd.pfd[i].revents & POLLIN) events |= QSE_AIO_DEV_EVENT_IN;
			if (mux->pd.pfd[i].revents & POLLOUT) events |= QSE_AIO_DEV_EVENT_OUT;
			if (mux->pd.pfd[i].revents & POLLPRI) events |= QSE_AIO_DEV_EVENT_PRI;
			if (mux->pd.pfd[i].revents & POLLERR) events |= QSE_AIO_DEV_EVENT_ERR;
			if (mux->pd.pfd[i].revents & POLLHUP) events |= QSE_AIO_DEV_EVENT_HUP;

			handle_event (dev, events, 0);
		}
	}

#elif defined(USE_EPOLL)

	mux = (qse_aio_mux_t*)aio->mux;

	nentries = epoll_wait (mux->hnd, mux->revs, QSE_COUNTOF(mux->revs), QSE_SECNSEC_TO_MSEC(tmout.sec, tmout.nsec));
	if (nentries == -1)
	{
		if (errno == EINTR) return 0; /* it's actually ok */
		/* other errors are critical - EBADF, EFAULT, EINVAL */
		aio->errnum = qse_aio_syserrtoerrnum(errno);
		return -1;
	}

	/* TODO: merge events??? for the same descriptor */
	
	for (i = 0; i < nentries; i++)
	{
		int events = 0, rdhup = 0;
		qse_aio_dev_t* dev;

		dev = mux->revs[i].data.ptr;

		if (mux->revs[i].events & EPOLLIN) events |= QSE_AIO_DEV_EVENT_IN;
		if (mux->revs[i].events & EPOLLOUT) events |= QSE_AIO_DEV_EVENT_OUT;
		if (mux->revs[i].events & EPOLLPRI) events |= QSE_AIO_DEV_EVENT_PRI;
		if (mux->revs[i].events & EPOLLERR) events |= QSE_AIO_DEV_EVENT_ERR;
		if (mux->revs[i].events & EPOLLHUP) events |= QSE_AIO_DEV_EVENT_HUP;
	#if defined(EPOLLRDHUP)
		else if (mux->revs[i].events & EPOLLRDHUP) rdhup = 1;
	#endif
		handle_event (dev, events, rdhup);
	}

#else

#	error NO SUPPORTED MULTIPLEXER
#endif

	/* kill all halted devices */
	while (aio->hltdev.head) 
	{
printf (">>>>>>>>>>>>>> KILLING HALTED DEVICE %p\n", aio->hltdev.head);
		qse_aio_killdev (aio, aio->hltdev.head);
	}
	QSE_ASSERT (aio->hltdev.tail == QSE_NULL);

	return 0;
}

int qse_aio_exec (qse_aio_t* aio)
{
	int n;

	aio->in_exec = 1;
	n = __exec (aio);
	aio->in_exec = 0;

	return n;
}

void qse_aio_stop (qse_aio_t* aio, qse_aio_stopreq_t stopreq)
{
	aio->stopreq = stopreq;
}

int qse_aio_loop (qse_aio_t* aio)
{
	if (!aio->actdev.head) return 0;

	aio->stopreq = QSE_AIO_STOPREQ_NONE;
	aio->renew_watch = 0;

	if (qse_aio_prologue (aio) <= -1) return -1;

	while (aio->stopreq == QSE_AIO_STOPREQ_NONE && aio->actdev.head)
	{
		if (qse_aio_exec (aio) <= -1) break;
		/* you can do other things here */
	}

	qse_aio_epilogue (aio);
	return 0;
}

qse_aio_dev_t* qse_aio_makedev (qse_aio_t* aio, qse_size_t dev_size, qse_aio_dev_mth_t* dev_mth, qse_aio_dev_evcb_t* dev_evcb, void* make_ctx)
{
	qse_aio_dev_t* dev;

	if (dev_size < QSE_SIZEOF(qse_aio_dev_t)) 
	{
		aio->errnum = QSE_AIO_EINVAL;
		return QSE_NULL;
	}

	dev = QSE_MMGR_ALLOC (aio->mmgr, dev_size);
	if (!dev)
	{
		aio->errnum = QSE_AIO_ENOMEM;
		return QSE_NULL;
	}

	QSE_MEMSET (dev, 0, dev_size);
	dev->aio = aio;
	dev->dev_size = dev_size;
	/* default capability. dev->dev_mth->make() can change this.
	 * qse_aio_dev_watch() is affected by the capability change. */
	dev->dev_capa = QSE_AIO_DEV_CAPA_IN | QSE_AIO_DEV_CAPA_OUT;
	dev->dev_mth = dev_mth;
	dev->dev_evcb = dev_evcb;
	QSE_AIO_WQ_INIT(&dev->wq);

	/* call the callback function first */
	aio->errnum = QSE_AIO_ENOERR;
	if (dev->dev_mth->make (dev, make_ctx) <= -1)
	{
		if (aio->errnum == QSE_AIO_ENOERR) aio->errnum = QSE_AIO_EDEVMAKE;
		goto oops;
	}

	/* the make callback must not change these fields */
	QSE_ASSERT (dev->dev_mth == dev_mth);
	QSE_ASSERT (dev->dev_evcb == dev_evcb);
	QSE_ASSERT (dev->dev_prev == QSE_NULL);
	QSE_ASSERT (dev->dev_next == QSE_NULL);

	/* set some internal capability bits according to the capabilities 
	 * removed by the device making callback for convenience sake. */
	if (!(dev->dev_capa & QSE_AIO_DEV_CAPA_IN)) dev->dev_capa |= QSE_AIO_DEV_CAPA_IN_CLOSED;
	if (!(dev->dev_capa & QSE_AIO_DEV_CAPA_OUT)) dev->dev_capa |= QSE_AIO_DEV_CAPA_OUT_CLOSED;

#if defined(_WIN32)
	if (CreateIoCompletionPort ((HANDLE)dev->dev_mth->getsyshnd(dev), aio->iocp, QSE_AIO_IOCP_KEY, 0) == NULL)
	{
		/* TODO: set errnum from GetLastError()... */
		goto oops_after_make;
	}
#else
	if (qse_aio_dev_watch (dev, QSE_AIO_DEV_WATCH_START, 0) <= -1) goto oops_after_make;
#endif

	/* and place the new device object at the back of the active device list */
	APPEND_DEVICE_TO_LIST (&aio->actdev, dev);
	dev->dev_capa |= QSE_AIO_DEV_CAPA_ACTIVE;

	return dev;

oops_after_make:
	if (kill_and_free_device (dev, 0) <= -1)
	{
		/* schedule a timer job that reattempts to destroy the device */
		if (schedule_kill_zombie_job (dev) <= -1) 
		{
			/* job scheduling failed. i have no choice but to
			 * destroy the device now.
			 * 
			 * NOTE: this while loop can block the process
			 *       if the kill method keep returning failure */
			while (kill_and_free_device (dev, 1) <= -1)
			{
				if (aio->stopreq != QSE_AIO_STOPREQ_NONE) 
				{
					/* i can't wait until destruction attempt gets
					 * fully successful. there is a chance that some
					 * resources can leak inside the device */
					kill_and_free_device (dev, 2);
					break;
				}
			}
		}

		return QSE_NULL;
	}

oops:
	QSE_MMGR_FREE (aio->mmgr, dev);
	return QSE_NULL;
}

static int kill_and_free_device (qse_aio_dev_t* dev, int force)
{
	qse_aio_t* aio;

	QSE_ASSERT (!(dev->dev_capa & QSE_AIO_DEV_CAPA_ACTIVE));
	QSE_ASSERT (!(dev->dev_capa & QSE_AIO_DEV_CAPA_HALTED));

	aio = dev->aio;

	if (dev->dev_mth->kill(dev, force) <= -1) 
	{
		if (force >= 2) goto free_device;

		if (!(dev->dev_capa & QSE_AIO_DEV_CAPA_ZOMBIE))
		{
			APPEND_DEVICE_TO_LIST (&aio->zmbdev, dev);
			dev->dev_capa |= QSE_AIO_DEV_CAPA_ZOMBIE;
		}

		return -1;
	}

free_device:
	if (dev->dev_capa & QSE_AIO_DEV_CAPA_ZOMBIE)
	{
		/* detach it from the zombie device list */
		UNLINK_DEVICE_FROM_LIST (&aio->zmbdev, dev);
		dev->dev_capa &= ~QSE_AIO_DEV_CAPA_ZOMBIE;
	}

	QSE_MMGR_FREE (aio->mmgr, dev);
	return 0;
}

static void kill_zombie_job_handler (qse_aio_t* aio, const qse_ntime_t* now, qse_aio_tmrjob_t* job)
{
	qse_aio_dev_t* dev = (qse_aio_dev_t*)job->ctx;

	QSE_ASSERT (dev->dev_capa & QSE_AIO_DEV_CAPA_ZOMBIE);

	if (kill_and_free_device (dev, 0) <= -1)
	{
		if (schedule_kill_zombie_job (dev) <= -1)
		{
			/* i have to choice but to free up the devide by force */
			while (kill_and_free_device (dev, 1) <= -1)
			{
				if (aio->stopreq  != QSE_AIO_STOPREQ_NONE) 
				{
					/* i can't wait until destruction attempt gets
					 * fully successful. there is a chance that some
					 * resources can leak inside the device */
					kill_and_free_device (dev, 2);
					break;
				}
			}
		}
	}
}

static int schedule_kill_zombie_job (qse_aio_dev_t* dev)
{
	qse_aio_tmrjob_t kill_zombie_job;
	qse_ntime_t tmout;

	qse_inittime (&tmout, 3, 0); /* TODO: take it from configuration */

	QSE_MEMSET (&kill_zombie_job, 0, QSE_SIZEOF(kill_zombie_job));
	kill_zombie_job.ctx = dev;
	qse_gettime (&kill_zombie_job.when);
	qse_addtime (&kill_zombie_job.when, &tmout, &kill_zombie_job.when);
	kill_zombie_job.handler = kill_zombie_job_handler;
	/*kill_zombie_job.idxptr = &rdev->tmridx_kill_zombie;*/

	return qse_aio_instmrjob (dev->aio, &kill_zombie_job) == QSE_AIO_TMRIDX_INVALID? -1: 0;
}

void qse_aio_killdev (qse_aio_t* aio, qse_aio_dev_t* dev)
{
	QSE_ASSERT (aio == dev->aio);

	if (dev->dev_capa & QSE_AIO_DEV_CAPA_ZOMBIE)
	{
		QSE_ASSERT (QSE_AIO_WQ_ISEMPTY(&dev->wq));
		goto kill_device;
	}

	/* clear pending send requests */
	while (!QSE_AIO_WQ_ISEMPTY(&dev->wq))
	{
		qse_aio_wq_t* q;
		q = QSE_AIO_WQ_HEAD(&dev->wq);
		unlink_wq (aio, q);
		QSE_MMGR_FREE (aio->mmgr, q);
	}

	if (dev->dev_capa & QSE_AIO_DEV_CAPA_HALTED)
	{
		/* this device is in the halted state.
		 * unlink it from the halted device list */
		UNLINK_DEVICE_FROM_LIST (&aio->hltdev, dev);
		dev->dev_capa &= ~QSE_AIO_DEV_CAPA_HALTED;
	}
	else
	{
		QSE_ASSERT (dev->dev_capa & QSE_AIO_DEV_CAPA_ACTIVE);
		UNLINK_DEVICE_FROM_LIST (&aio->actdev, dev);
		dev->dev_capa &= ~QSE_AIO_DEV_CAPA_ACTIVE;
	}

	qse_aio_dev_watch (dev, QSE_AIO_DEV_WATCH_STOP, 0);

kill_device:
	if (kill_and_free_device(dev, 0) <= -1)
	{
		QSE_ASSERT (dev->dev_capa & QSE_AIO_DEV_CAPA_ZOMBIE);
		if (schedule_kill_zombie_job (dev) <= -1)
		{
			/* i have to choice but to free up the devide by force */
			while (kill_and_free_device (dev, 1) <= -1)
			{
				if (aio->stopreq  != QSE_AIO_STOPREQ_NONE) 
				{
					/* i can't wait until destruction attempt gets
					 * fully successful. there is a chance that some
					 * resources can leak inside the device */
					kill_and_free_device (dev, 2);
					break;
				}
			}
		}
	}
}

void qse_aio_dev_halt (qse_aio_dev_t* dev)
{
	if (dev->dev_capa & QSE_AIO_DEV_CAPA_ACTIVE)
	{
		qse_aio_t* aio;

		aio = dev->aio;

		/* delink the device object from the active device list */
		UNLINK_DEVICE_FROM_LIST (&aio->actdev, dev);
		dev->dev_capa &= ~QSE_AIO_DEV_CAPA_ACTIVE;

		/* place it at the back of the halted device list */
		APPEND_DEVICE_TO_LIST (&aio->hltdev, dev);
		dev->dev_capa |= QSE_AIO_DEV_CAPA_HALTED;
	}
}

int qse_aio_dev_ioctl (qse_aio_dev_t* dev, int cmd, void* arg)
{
	if (dev->dev_mth->ioctl) return dev->dev_mth->ioctl (dev, cmd, arg);
	dev->aio->errnum = QSE_AIO_ENOSUP; /* TODO: different error code ? */
	return -1;
}

int qse_aio_dev_watch (qse_aio_dev_t* dev, qse_aio_dev_watch_cmd_t cmd, int events)
{
	int mux_cmd;
	int dev_capa;

	/* the virtual device doesn't perform actual I/O.
	 * it's different from not hanving QSE_AIO_DEV_CAPA_IN and QSE_AIO_DEV_CAPA_OUT.
	 * a non-virtual device without the capabilities still gets attention
	 * of the system multiplexer for hangup and error. */
	if (dev->dev_capa & QSE_AIO_DEV_CAPA_VIRTUAL) return 0;

	/*ev.data.ptr = dev;*/
	switch (cmd)
	{
		case QSE_AIO_DEV_WATCH_START:
			/* upon start, only input watching is requested */
			events = QSE_AIO_DEV_EVENT_IN; 
			mux_cmd = MUX_CMD_INSERT;
			break;

		case QSE_AIO_DEV_WATCH_RENEW:
			/* auto-renwal mode. input watching is requested all the time.
			 * output watching is requested only if there're enqueued 
			 * data for writing. */
			events = QSE_AIO_DEV_EVENT_IN;
			if (!QSE_AIO_WQ_ISEMPTY(&dev->wq)) events |= QSE_AIO_DEV_EVENT_OUT;
			/* fall through */
		case QSE_AIO_DEV_WATCH_UPDATE:
			/* honor event watching requests as given by the caller */
			mux_cmd = MUX_CMD_UPDATE;
			break;

		case QSE_AIO_DEV_WATCH_STOP:
			events = 0; /* override events */
			mux_cmd = MUX_CMD_DELETE;
			break;

		default:
			dev->aio->errnum = QSE_AIO_EINVAL;
			return -1;
	}

	dev_capa = dev->dev_capa;
	dev_capa &= ~(DEV_CAPA_ALL_WATCHED);

	/* this function honors QSE_AIO_DEV_EVENT_IN and QSE_AIO_DEV_EVENT_OUT only
	 * as valid input event bits. it intends to provide simple abstraction
	 * by reducing the variety of event bits that the caller has to handle. */

	if ((events & QSE_AIO_DEV_EVENT_IN) && !(dev->dev_capa & (QSE_AIO_DEV_CAPA_IN_CLOSED | QSE_AIO_DEV_CAPA_IN_DISABLED)))
	{
		if (dev->dev_capa & QSE_AIO_DEV_CAPA_IN) 
		{
			if (dev->dev_capa & QSE_AIO_DEV_CAPA_PRI) dev_capa |= QSE_AIO_DEV_CAPA_PRI_WATCHED;
			dev_capa |= QSE_AIO_DEV_CAPA_IN_WATCHED;
		}
	}

	if ((events & QSE_AIO_DEV_EVENT_OUT) && !(dev->dev_capa & QSE_AIO_DEV_CAPA_OUT_CLOSED))
	{
		if (dev->dev_capa & QSE_AIO_DEV_CAPA_OUT) dev_capa |= QSE_AIO_DEV_CAPA_OUT_WATCHED;
	}

	if (mux_cmd == MUX_CMD_UPDATE && (dev_capa & DEV_CAPA_ALL_WATCHED) == (dev->dev_capa & DEV_CAPA_ALL_WATCHED))
	{
		/* no change in the device capacity. skip calling epoll_ctl */
	}
	else
	{
		if (mux_control (dev, mux_cmd, dev->dev_mth->getsyshnd(dev), dev_capa) <= -1) return -1;
	}

	dev->dev_capa = dev_capa;
	return 0;
}

int qse_aio_dev_read (qse_aio_dev_t* dev, int enabled)
{
	if (dev->dev_capa & QSE_AIO_DEV_CAPA_IN_CLOSED)
	{
		dev->aio->errnum = QSE_AIO_ENOCAPA;
		return -1;
	}

	if (enabled)
	{
		dev->dev_capa &= ~QSE_AIO_DEV_CAPA_IN_DISABLED;
		if (!dev->aio->in_exec && (dev->dev_capa & QSE_AIO_DEV_CAPA_IN_WATCHED)) goto renew_watch_now;
	}
	else
	{
		dev->dev_capa |= QSE_AIO_DEV_CAPA_IN_DISABLED;
		if (!dev->aio->in_exec && !(dev->dev_capa & QSE_AIO_DEV_CAPA_IN_WATCHED)) goto renew_watch_now;
	}

	dev->aio->renew_watch = 1;
	return 0;

renew_watch_now:
	if (qse_aio_dev_watch (dev, QSE_AIO_DEV_WATCH_RENEW, 0) <= -1) return -1;
	return 0;
}

static void on_write_timeout (qse_aio_t* aio, const qse_ntime_t* now, qse_aio_tmrjob_t* job)
{
	qse_aio_wq_t* q;
	qse_aio_dev_t* dev;
	int x;

	q = (qse_aio_wq_t*)job->ctx;
	dev = q->dev;

	dev->aio->errnum = QSE_AIO_ETMOUT;
	x = dev->dev_evcb->on_write (dev, -1, q->ctx, &q->dstaddr); 

	QSE_ASSERT (q->tmridx == QSE_AIO_TMRIDX_INVALID);
	QSE_AIO_WQ_UNLINK(q);
	QSE_MMGR_FREE (aio->mmgr, q);

	if (x <= -1) qse_aio_dev_halt (dev);
}

static int __dev_write (qse_aio_dev_t* dev, const void* data, qse_aio_iolen_t len, const qse_ntime_t* tmout, void* wrctx, const qse_aio_devaddr_t* dstaddr)
{
	const qse_uint8_t* uptr;
	qse_aio_iolen_t urem, ulen;
	qse_aio_wq_t* q;
	int x;

	if (dev->dev_capa & QSE_AIO_DEV_CAPA_OUT_CLOSED)
	{
		dev->aio->errnum = QSE_AIO_ENOCAPA;
		return -1;
	}

	uptr = data;
	urem = len;

	if (!QSE_AIO_WQ_ISEMPTY(&dev->wq)) 
	{
		/* the writing queue is not empty. 
		 * enqueue this request immediately */
		goto enqueue_data;
	}

	if (dev->dev_capa & QSE_AIO_DEV_CAPA_STREAM)
	{
		/* use the do..while() loop to be able to send a zero-length data */
		do
		{
			ulen = urem;
			x = dev->dev_mth->write (dev, data, &ulen, dstaddr);
			if (x <= -1) return -1;
			else if (x == 0) 
			{
				/* [NOTE] 
				 * the write queue is empty at this moment. a zero-length 
				 * request for a stream device can still get enqueued  if the
				 * write callback returns 0 though i can't figure out if there
				 * is a compelling reason to do so 
				 */
				goto enqueue_data; /* enqueue remaining data */
			}
			else 
			{
				urem -= ulen;
				uptr += ulen;
			}
		}
		while (urem > 0);

		if (len <= 0) /* original length */
		{
			/* a zero-length writing request is to close the writing end */
			dev->dev_capa |= QSE_AIO_DEV_CAPA_OUT_CLOSED;
		}

		if (dev->dev_evcb->on_write (dev, len, wrctx, dstaddr) <= -1) return -1;
	}
	else
	{
		ulen = urem;

		x = dev->dev_mth->write (dev, data, &ulen, dstaddr);
		if (x <= -1) return -1;
		else if (x == 0) goto enqueue_data;

		/* partial writing is still considered ok for a non-stream device */
		if (dev->dev_evcb->on_write (dev, ulen, wrctx, dstaddr) <= -1) return -1;
	}

	return 1; /* written immediately and called on_write callback */

enqueue_data:
	if (!(dev->dev_capa & QSE_AIO_DEV_CAPA_OUT_QUEUED)) 
	{
		/* writing queuing is not requested. so return failure */
		dev->aio->errnum = QSE_AIO_ENOCAPA;
		return -1;
	}

	/* queue the remaining data*/
	q = (qse_aio_wq_t*)QSE_MMGR_ALLOC (dev->aio->mmgr, QSE_SIZEOF(*q) + (dstaddr? dstaddr->len: 0) + urem);
	if (!q)
	{
		dev->aio->errnum = QSE_AIO_ENOMEM;
		return -1;
	}

	q->tmridx = QSE_AIO_TMRIDX_INVALID;
	q->dev = dev;
	q->ctx = wrctx;

	if (dstaddr)
	{
		q->dstaddr.ptr = (qse_uint8_t*)(q + 1);
		q->dstaddr.len = dstaddr->len;
		QSE_MEMCPY (q->dstaddr.ptr, dstaddr->ptr, dstaddr->len);
	}
	else
	{
		q->dstaddr.len = 0;
	}

	q->ptr = (qse_uint8_t*)(q + 1) + q->dstaddr.len;
	q->len = urem;
	q->olen = len;
	QSE_MEMCPY (q->ptr, uptr, urem);

	if (tmout && qse_ispostime(tmout))
	{
		qse_aio_tmrjob_t tmrjob;

		QSE_MEMSET (&tmrjob, 0, QSE_SIZEOF(tmrjob));
		tmrjob.ctx = q;
		qse_gettime (&tmrjob.when);
		qse_addtime (&tmrjob.when, tmout, &tmrjob.when);
		tmrjob.handler = on_write_timeout;
		tmrjob.idxptr = &q->tmridx;

		q->tmridx = qse_aio_instmrjob (dev->aio, &tmrjob);
		if (q->tmridx == QSE_AIO_TMRIDX_INVALID) 
		{
			QSE_MMGR_FREE (dev->aio->mmgr, q);
			return -1;
		}
	}

	QSE_AIO_WQ_ENQ (&dev->wq, q);
	if (!dev->aio->in_exec && !(dev->dev_capa & QSE_AIO_DEV_CAPA_OUT_WATCHED))
	{
		/* if output is not being watched, arrange to do so */
		if (qse_aio_dev_watch (dev, QSE_AIO_DEV_WATCH_RENEW, 0) <= -1)
		{
			unlink_wq (dev->aio, q);
			QSE_MMGR_FREE (dev->aio->mmgr, q);
			return -1;
		}
	}
	else
	{
		dev->aio->renew_watch = 1;
	}

	return 0; /* request pused to a write queue. */
}

int qse_aio_dev_write (qse_aio_dev_t* dev, const void* data, qse_aio_iolen_t len, void* wrctx, const qse_aio_devaddr_t* dstaddr)
{
	return __dev_write (dev, data, len, QSE_NULL, wrctx, dstaddr);
}

int qse_aio_dev_timedwrite (qse_aio_dev_t* dev, const void* data, qse_aio_iolen_t len, const qse_ntime_t* tmout, void* wrctx, const qse_aio_devaddr_t* dstaddr)
{
	return __dev_write (dev, data, len, tmout, wrctx, dstaddr);
}

int qse_aio_makesyshndasync (qse_aio_t* aio, qse_aio_syshnd_t hnd)
{
#if defined(F_GETFL) && defined(F_SETFL) && defined(O_NONBLOCK)
	int flags;

	if ((flags = fcntl (hnd, F_GETFL)) <= -1 ||
	    (flags = fcntl (hnd, F_SETFL, flags | O_NONBLOCK)) <= -1)
	{
		aio->errnum = qse_aio_syserrtoerrnum (errno);
		return -1;
	}

	return 0;
#else
	aio->errnum = QSE_AIO_ENOSUP;
	return -1;
#endif
}

qse_aio_errnum_t qse_aio_syserrtoerrnum (int no)
{
	switch (no)
	{
		case ENOMEM:
			return QSE_AIO_ENOMEM;

		case EINVAL:
			return QSE_AIO_EINVAL;

		case EEXIST:
			return QSE_AIO_EEXIST;

		case ENOENT:
			return QSE_AIO_ENOENT;

		case EMFILE:
			return QSE_AIO_EMFILE;

	#if defined(ENFILE)
		case ENFILE:
			return QSE_AIO_ENFILE;
	#endif

	#if defined(EWOULDBLOCK) && defined(EAGAIN) && (EWOULDBLOCK != EAGAIN)
		case EAGAIN:
		case EWOULDBLOCK:
			return QSE_AIO_EAGAIN;
	#elif defined(EAGAIN)
		case EAGAIN:
			return QSE_AIO_EAGAIN;
	#elif defined(EWOULDBLOCK)
		case EWOULDBLOCK:
			return QSE_AIO_EAGAIN;
	#endif

	#if defined(ECONNREFUSED)
		case ECONNREFUSED:
			return QSE_AIO_ECONRF;
	#endif

	#if defined(ECONNRESETD)
		case ECONNRESET:
			return QSE_AIO_ECONRS;
	#endif

	#if defined(EPERM)
		case EPERM:
			return QSE_AIO_EPERM;
	#endif

		default:
			return QSE_AIO_ESYSERR;
	}
}
