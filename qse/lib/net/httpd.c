/*
 * $Id$
 * 
    Copyright 2006-2011 Chung, Hyung-Hwan.
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

#if defined(_WIN32) || defined(__DOS__) || defined(__OS2__)
/* UNSUPPORTED YET..  */ 
/* TODO: IMPLEMENT THIS */
#else

#include "httpd.h"
#include "../cmn/mem.h"
#include "../cmn/syscall.h"
#include <qse/cmn/chr.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mbwc.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#if defined(HAVE_PTHREAD)
#include <pthread.h>
#endif

#if 0
#include <openssl.h>
#endif

#include <qse/cmn/stdio.h>

typedef struct htrd_xtn_t htrd_xtn_t;

struct htrd_xtn_t
{
	qse_size_t      client_index; 
	qse_httpd_t*    httpd;
};

QSE_IMPLEMENT_COMMON_FUNCTIONS (httpd)

#define DEFAULT_PORT        80
#define DEFAULT_SECURE_PORT 443

static void free_listener_list (qse_httpd_t* httpd, listener_t* l);

qse_httpd_t* qse_httpd_open (qse_mmgr_t* mmgr, qse_size_t xtnsize)
{
	qse_httpd_t* httpd;

	httpd = (qse_httpd_t*) QSE_MMGR_ALLOC (
		mmgr, QSE_SIZEOF(*httpd) + xtnsize
	);
	if (httpd == QSE_NULL) return QSE_NULL;

	if (qse_httpd_init (httpd, mmgr) <= -1)
	{
		QSE_MMGR_FREE (httpd->mmgr, httpd);
		return QSE_NULL;
	}

	return httpd;
}

void qse_httpd_close (qse_httpd_t* httpd)
{
	qse_httpd_fini (httpd);
	QSE_MMGR_FREE (httpd->mmgr, httpd);
}

int qse_httpd_init (qse_httpd_t* httpd, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (httpd, 0, QSE_SIZEOF(*httpd));
	httpd->mmgr = mmgr;
	httpd->listener.max = -1;

#if defined(HAVE_PTHREAD)
	pthread_mutex_init (&httpd->listener.mutex, QSE_NULL);
#endif

	return 0;
}

void qse_httpd_fini (qse_httpd_t* httpd)
{
	/* TODO */
#if defined(HAVE_PTHREAD)
	pthread_mutex_destroy (&httpd->listener.mutex);
#endif
	free_listener_list (httpd, httpd->listener.list);
	httpd->listener.list = QSE_NULL;
}

void qse_httpd_stop (qse_httpd_t* httpd)
{
	httpd->stopreq = 1;
}

int qse_httpd_getoption (qse_httpd_t* httpd)
{
	return httpd->option;
}

void qse_httpd_setoption (qse_httpd_t* httpd, int option)
{
	httpd->option = option;
}

const qse_httpd_cbs_t* qse_httpd_getcbs (qse_httpd_t* httpd)
{
	return httpd->cbs;
}

void qse_httpd_setcbs (qse_httpd_t* httpd, qse_httpd_cbs_t* cbs)
{
	httpd->cbs = cbs;
}

QSE_INLINE void* qse_httpd_allocmem (qse_httpd_t* httpd, qse_size_t size)
{
	void* ptr = QSE_MMGR_ALLOC (httpd->mmgr, size);
	if (ptr == QSE_NULL)  httpd->errnum = QSE_HTTPD_ENOMEM;
	return ptr;
}

QSE_INLINE void* qse_httpd_reallocmem (
	qse_httpd_t* httpd, void* ptr, qse_size_t size)
{
	void* nptr = QSE_MMGR_REALLOC (httpd->mmgr, ptr, size);
	if (nptr == QSE_NULL)  httpd->errnum = QSE_HTTPD_ENOMEM;
	return nptr;
}

QSE_INLINE void qse_httpd_freemem (qse_httpd_t* httpd, void* ptr)
{
	QSE_MMGR_FREE (httpd->mmgr, ptr);
}

static qse_httpd_task_t* enqueue_task_unlocked (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	const qse_httpd_task_t* pred, const qse_httpd_task_t* task,
	qse_size_t xtnsize)
{
	task_queue_node_t* node;

/* TODO: limit check 
	if (client->task.queue.count >= httpd->limit.client_task_queue)
	{
		httpd->errnum = QSE_HTTPD_ETASK;
		return -1;
	}
*/
	node = (task_queue_node_t*)
		qse_httpd_allocmem (httpd, QSE_SIZEOF(*node) + xtnsize);
	if (node == QSE_NULL) return QSE_NULL;

	node->task = *task;

	if (task->init)
	{
		httpd->errnum = QSE_HTTPD_ENOERR;
		if (task->init (httpd, client, &node->task) <= -1)
		{
			if (httpd->errnum == QSE_HTTPD_ENOERR) 
				httpd->errnum = QSE_HTTPD_ETASK;
			qse_httpd_freemem (httpd, node);
			return QSE_NULL;	
		}
	}

	if (pred)
	{
		task_queue_node_t* prev;

		/* TODO: confirm if this calculation works all the time,
		 *       especially regarding structure alignment */
		prev = (task_queue_node_t*) 
			((qse_byte_t*)pred - (QSE_SIZEOF(*prev) - QSE_SIZEOF(*pred)));

		node->next = prev->next;
		node->prev = prev;

		if (prev->next) prev->next->prev = node;
		else client->task.queue.tail = node;
		prev->next = node;
	}
	else
	{
		node->next = QSE_NULL;
		node->prev = client->task.queue.tail;
		if (client->task.queue.tail) 
			client->task.queue.tail->next = node;
		else 
			client->task.queue.head = node;
		client->task.queue.tail = node;
	}
	client->task.queue.count++;

	return &node->task;
}

static qse_httpd_task_t* enqueue_task_locked (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	const qse_httpd_task_t* pred, const qse_httpd_task_t* task,
	qse_size_t xtnsize)
{
#if defined(HAVE_PTHREAD)
	if (httpd->threaded)
	{
		qse_httpd_task_t* ret;
		pthread_mutex_lock (&client->task.mutex);
		ret = enqueue_task_unlocked (httpd, client, pred, task, xtnsize);
		pthread_mutex_unlock (&client->task.mutex);
		return ret;
	}
	else
	{
#endif
		return enqueue_task_unlocked (httpd, client, pred, task, xtnsize);
#if defined(HAVE_PTHREAD)
	}
#endif
}

static QSE_INLINE int dequeue_task_unlocked (
	qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	task_queue_node_t* node;

	if (client->task.queue.count <= 0) return -1;

	node = client->task.queue.head;

	if (node == client->task.queue.tail)
	{
		client->task.queue.head = QSE_NULL;
		client->task.queue.tail = QSE_NULL;
	}
	else
	{
		node->next->prev = QSE_NULL;
		client->task.queue.head = node->next;
	}
	client->task.queue.count--;

	if (node->task.fini) node->task.fini (httpd, client, &node->task);
	qse_httpd_freemem (httpd, node);

	return 0;
}

static int dequeue_task_locked (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
#if defined(HAVE_PTHREAD)
	if (httpd->threaded)
	{
		int ret;
		pthread_mutex_lock (&client->task.mutex);
		ret = dequeue_task_unlocked (httpd, client);
		pthread_mutex_unlock (&client->task.mutex);
		return ret;
	}
	else
	{
#endif
		return dequeue_task_unlocked (httpd, client);
#if defined(HAVE_PTHREAD)
	}
#endif
}

static void purge_tasks_locked (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
#if defined(HAVE_PTHREAD)
	if (httpd->threaded) pthread_mutex_lock (&client->task.mutex);
#endif
	while (dequeue_task_unlocked (httpd, client) == 0);
#if defined(HAVE_PTHREAD)
	if (httpd->threaded) pthread_mutex_unlock (&client->task.mutex);
#endif
}

static int htrd_peek_request (qse_htrd_t* htrd, qse_htre_t* req)
{
	htrd_xtn_t* xtn = (htrd_xtn_t*) qse_htrd_getxtn (htrd);
	qse_httpd_client_t* client = 
		&xtn->httpd->client.array.data[xtn->client_index];
	return xtn->httpd->cbs->peek_request (xtn->httpd, client, req);
}

static int htrd_handle_request (qse_htrd_t* htrd, qse_htre_t* req)
{
	htrd_xtn_t* xtn = (htrd_xtn_t*) qse_htrd_getxtn (htrd);
	qse_httpd_client_t* client = 
		&xtn->httpd->client.array.data[xtn->client_index];
	return xtn->httpd->cbs->handle_request (xtn->httpd, client, req);
}

static qse_htrd_recbs_t htrd_recbs =
{
	htrd_peek_request,
	htrd_handle_request
};

static void deactivate_listener (qse_httpd_t* httpd, listener_t* l)
{
	QSE_ASSERT (l->handle >= 0);

/* TODO: callback httpd->cbs.listener_deactivated (httpd, l);*/
/* TODO: suport https... */
	close (l->handle);
	l->handle = -1;
}

static int get_listener_sockaddr (const listener_t* l, sockaddr_t* addr)
{
	int addrsize;

	QSE_MEMSET (addr, 0, QSE_SIZEOF(*addr));

	switch (l->family)
	{
		case AF_INET:
		{
			addr->in4.sin_family = l->family;	
			addr->in4.sin_addr = l->addr.in4;
			addr->in4.sin_port = htons (l->port);
			addrsize = QSE_SIZEOF(addr->in4);
			break;
		}

#ifdef AF_INET6
		case AF_INET6:
		{
			addr->in6.sin6_family = l->family;	
			addr->in6.sin6_addr = l->addr.in6;
			addr->in6.sin6_port = htons (l->port);
			/* TODO: addr.in6.sin6_scope_id  */
			addrsize = QSE_SIZEOF(addr->in6);
			break;
		}
#endif

		default:
		{
			QSE_ASSERT (!"This should never happen");
		}
	}

	return addrsize;
}

static int activate_listener (qse_httpd_t* httpd, listener_t* l)
{
/* TODO: suport https... */
	int s = -1, flag;
	sockaddr_t addr;
	int addrsize;

	QSE_ASSERT (l->handle <= -1);

	s = socket (l->family, SOCK_STREAM, IPPROTO_TCP);
	if (s <= -1) goto oops_esocket;

	flag = 1;
	setsockopt (s, SOL_SOCKET, SO_REUSEADDR, &flag, QSE_SIZEOF(flag));

	addrsize = get_listener_sockaddr (l, &addr);

	/* Solaris 8 returns EINVAL if QSE_SIZEOF(addr) is passed in as the 
	 * address size for AF_INET. */
	/*if (bind (s, (struct sockaddr*)&addr, QSE_SIZEOF(addr)) <= -1) goto oops_esocket;*/
	if (bind (s, (struct sockaddr*)&addr, addrsize) <= -1) goto oops_esocket;
	if (listen (s, 10) <= -1) goto oops_esocket;
	
	flag = fcntl (s, F_GETFL);
	if (flag >= 0) fcntl (s, F_SETFL, flag | O_NONBLOCK);
	fcntl (s, F_SETFD, FD_CLOEXEC);

	l->handle = s;
	s = -1;

	return 0;

oops_esocket:
	httpd->errnum = QSE_HTTPD_ESOCKET;
	if (s >= 0) close (s);
	return -1;
}

static void deactivate_listeners (qse_httpd_t* httpd)
{
	listener_t* l;

	for (l = httpd->listener.list; l; l = l->next)
	{
		if (l->handle >= 0) deactivate_listener (httpd, l);
	}

	FD_ZERO (&httpd->listener.set);
	httpd->listener.max = -1;
}

static int activate_listeners (qse_httpd_t* httpd)
{
	listener_t* l;
	fd_set listener_set;
	int listener_max = -1;

	FD_ZERO (&listener_set);	
	for (l = httpd->listener.list; l; l = l->next)
	{
		if (l->handle <= -1)
		{
			if (activate_listener (httpd, l) <= -1) goto oops;
/*TODO: callback httpd->cbs.listener_activated (httpd, l);*/
		}

		FD_SET (l->handle, &listener_set);
		if (l->handle >= listener_max) listener_max = l->handle;
	}

	httpd->listener.set = listener_set;
	httpd->listener.max = listener_max;
	return 0;

oops:
	deactivate_listeners (httpd);
	return -1;
}

static void init_client_array (qse_httpd_t* httpd)
{
	client_array_t* array = &httpd->client.array;
	array->capa = 0;
	array->size = 0;
	array->data = QSE_NULL;
}

static void delete_from_client_array (qse_httpd_t* httpd, int fd)
{
	client_array_t* array = &httpd->client.array;
	if (array->data[fd].htrd)
	{
		purge_tasks_locked (httpd, &array->data[fd]);
#if defined(HAVE_PTHREAD)
		if (httpd->threaded) 
			pthread_mutex_destroy (&array->data[fd].task.mutex);
#endif

		qse_htrd_close (array->data[fd].htrd);
		array->data[fd].htrd = QSE_NULL;	
qse_fprintf (QSE_STDERR, QSE_T("Debug: closing socket %d\n"), array->data[fd].handle.i);

		/* note that client.closed is not a counterpart to client.accepted. 
		 * so it is called even if client.closed failed. */
		if (httpd->cbs->client.closed)
			httpd->cbs->client.closed (httpd, &array->data[fd]);
		
		close (array->data[fd].handle.i);
		array->size--;
	}
}

static void fini_client_array (qse_httpd_t* httpd)
{
	client_array_t* array = &httpd->client.array;
	if (array->data) 
	{
		int fd;

		for (fd = 0; fd < array->capa; fd++)
			delete_from_client_array (httpd, fd);

		qse_httpd_freemem (httpd, array->data);
		array->capa = 0;
		array->size = 0;
		array->data = QSE_NULL;
	}
}

static qse_httpd_client_t* insert_into_client_array (
	qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	htrd_xtn_t* xtn;
	client_array_t* array = &httpd->client.array;
	int opt, fd = client->handle.i;

/* TODO: is an array is the best??? 
 *       i do use an array for direct access by fd. */
	if (fd >= array->capa)
	{
	#define ALIGN 512
		qse_httpd_client_t* tmp;
		qse_size_t capa = ((fd + ALIGN) / ALIGN) * ALIGN;

		tmp = qse_httpd_reallocmem (
			httpd, array->data, capa * QSE_SIZEOF(*tmp));
		if (tmp == QSE_NULL) return QSE_NULL;

		QSE_MEMSET (&tmp[array->capa], 0,
			QSE_SIZEOF(*tmp) * (capa - array->capa));

		array->data = tmp;
		array->capa = capa;
	}

	QSE_ASSERT (array->data[fd].htrd == QSE_NULL);

	array->data[fd].htrd = qse_htrd_open (httpd->mmgr, QSE_SIZEOF(*xtn));
	if (array->data[fd].htrd == QSE_NULL) return QSE_NULL;
	opt = qse_htrd_getoption (array->data[fd].htrd);
	opt |= QSE_HTRD_REQUEST;
	opt &= ~QSE_HTRD_RESPONSE;
	qse_htrd_setoption (array->data[fd].htrd, opt);

	array->data[fd].ready = httpd->cbs->client.accepted? 0 : 1;
	array->data[fd].bad = 0;
	array->data[fd].secure = client->secure;
	array->data[fd].handle = client->handle;
	array->data[fd].handle2 = client->handle2;
	array->data[fd].local_addr = client->local_addr;
	array->data[fd].remote_addr = client->remote_addr;

#if defined(HAVE_PTHREAD)
	if (httpd->threaded) 
		pthread_mutex_init (&array->data[fd].task.mutex, QSE_NULL);
#endif

	xtn = (htrd_xtn_t*)qse_htrd_getxtn (array->data[fd].htrd);	
	xtn->client_index = fd; 
	xtn->httpd = httpd;

	qse_htrd_setrecbs (array->data[fd].htrd, &htrd_recbs);
	array->size++;
	return &array->data[fd];
}

static int accept_client_from_listener (qse_httpd_t* httpd, listener_t* l)
{
	int flag;

#ifdef HAVE_SOCKLEN_T
	socklen_t addrlen;
#else
	int addrlen;
#endif
	qse_httpd_client_t clibuf;
	qse_httpd_client_t* client;

	QSE_MEMSET (&clibuf, 0, QSE_SIZEOF(clibuf));
	clibuf.secure = l->secure;

	addrlen = QSE_SIZEOF(clibuf.remote_addr);
	clibuf.handle.i = accept (
		l->handle, (struct sockaddr*)&clibuf.remote_addr, &addrlen);
	if (clibuf.handle.i <= -1)
	{
		httpd->errnum = QSE_HTTPD_ESOCKET;
qse_fprintf (QSE_STDERR, QSE_T("Error: accept returned failure\n"));
		goto oops;
	}

	/* select() uses a fixed-size array so the file descriptor can not
	 * exceeded FD_SETSIZE */
	if (clibuf.handle.i >= FD_SETSIZE)
	{
		close (clibuf.handle.i);
qse_fprintf (QSE_STDERR, QSE_T("Error: too many client?\n"));
		/* httpd->errnum = QSE_HTTPD_EOVERFLOW; */
		goto oops;
	}

	addrlen = QSE_SIZEOF(clibuf.local_addr);
	if (getsockname (clibuf.handle.i, (struct sockaddr*)&clibuf.local_addr, &addrlen) <= -1)
		get_listener_sockaddr (l, &clibuf.local_addr);

	/* set the nonblock flag in case read() after select() blocks
	 * for various reasons - data received may be dropped after 
	 * arrival for wrong checksum, for example. */
	flag = fcntl (clibuf.handle.i, F_GETFL);
	if (flag >= 0) fcntl (clibuf.handle.i, F_SETFL, flag | O_NONBLOCK);

	flag = fcntl (clibuf.handle.i, F_GETFD);
	if (flag >= 0) fcntl (clibuf.handle.i, F_SETFD, flag | FD_CLOEXEC);

#if defined(HAVE_PTHREAD)
	if (httpd->threaded) pthread_mutex_lock (&httpd->client.mutex);
#endif
	client = insert_into_client_array (httpd, &clibuf);
#if defined(HAVE_PTHREAD)
	if (httpd->threaded) pthread_mutex_unlock (&httpd->client.mutex);
#endif

	if (client == QSE_NULL)
	{
		close (clibuf.handle.i);
		goto oops;
	}

qse_printf (QSE_T("connection %d accepted\n"), clibuf.handle.i);

	return 0;

oops:
	return -1;
}

static void accept_client_from_listeners (qse_httpd_t* httpd, fd_set* r)
{
	listener_t* l;

	for (l = httpd->listener.list; l; l = l->next)
	{
		if (FD_ISSET(l->handle, r))
		{
			accept_client_from_listener (httpd, l);
/* TODO: if (accept_client_from_listener (httpd, l) <= -1)
httpd->cbs.on_error (httpd, l).... */
		}
	}
}

static int make_fd_set_from_client_array (
	qse_httpd_t* httpd, fd_set* r, fd_set* w, int for_rdwr)
{
	/* qse_http_loop() sets for_rdwr to true.
	 * response_thread() sets for_rdwr to false.
	 * 
	 * qse_http_loop() 
	 *  - accepts a new client connection
	 *  - reads a client request
	 *  - writes back a response to a client request if not threaded.
	 *
	 * response_thread()
	 *  - writes back a response to a client request if threaded.
	 */

	int fd, max = -1;
	client_array_t* ca = &httpd->client.array;

	if (for_rdwr)
	{
		/* qse_http_loop() needs to monitor listner handles
		 * to handle a new client connection. */
		max = httpd->listener.max;
		*r = httpd->listener.set;
	}
	else 
	{
		FD_ZERO (r);

#if defined(HAVE_PTHREAD)
		/* select() in response_thread() needs to be aborted
		 * if it's blocking on a fd_set previously composed
		 * when a new task is enqueued. it can select() on new
		 * fd_set quickly.  
		 */
		QSE_ASSERT (httpd->threaded);
		FD_SET (httpd->client.pfd[0], r);
		max = httpd->client.pfd[0];
#endif
	}
	FD_ZERO (w);

	for (fd = 0; fd < ca->capa; fd++)
	{
		if (ca->data[fd].htrd) 
		{
			if (!ca->data[fd].bad) 
			{
				if (for_rdwr)
				{
					/* add a client-side handle to the read set
					 * only for qse_httpd_loop(). */
					FD_SET (ca->data[fd].handle.i, r);
					if (ca->data[fd].handle.i > max) max = ca->data[fd].handle.i;
				}

				if (!httpd->threaded || !for_rdwr)
				{
					/* trigger[0] is a handle to monitor to check
					 * if there is data avaiable to read to write back to 
					 * the client. if it is not threaded, qse_httpd_loop() 
					 * needs to monitor trigger handles. if it is threaded, 
					 * response_thread() needs to monitor these handles.
					 *
					 * trigger[1] is a user-defined handle to monitor to 
					 * check if httpd can post data to. but this is not 
					 * a client-side handle.
					 */
					if (ca->data[fd].task.queue.head)
					{
						qse_httpd_task_t* task = &ca->data[fd].task.queue.head->task;
						if (task->trigger_mask & QSE_HTTPD_TASK_TRIGGER_READ)
						{
							/* if a trigger is available, add it to the read set also. */
qse_printf (QSE_T(">>>>ADDING TRIGGER[0] %d\n"), task->trigger[0].i);
							FD_SET (task->trigger[0].i, r);
							if (task->trigger[0].i > max) max = task->trigger[0].i;
						}
						if (task->trigger_mask & QSE_HTTPD_TASK_TRIGGER_WRITE)
						{
							/* if a trigger is available, add it to the read set also. */
qse_printf (QSE_T(">>>>ADDING TRIGGER[1] %d\n"), task->trigger[1].i);
							FD_SET (task->trigger[1].i, w);
							if (task->trigger[1].i > max) max = task->trigger[1].i;
						}
					}
				}
			}

			if (ca->data[fd].bad ||
			    (ca->data[fd].task.queue.head && !(ca->data[fd].task.queue.head->task.trigger_mask & QSE_HTTPD_TASK_TRIGGER_READ)))
			{
				/* add a client-side handle to the write set 
				 * if the client is already marked bad or
				 * the current task enqueued didn't specify a trigger.
				 *
				 * if the task doesn't have a trigger, i perform
				 * the task so long as the client side-handle is
				 * available for writing in the main loop.
				 */
				FD_SET (ca->data[fd].handle.i, w);
				if (ca->data[fd].handle.i > max) max = ca->data[fd].handle.i;
			}
		}
	}

	return max;
}

static void perform_task (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	task_queue_node_t* node;
	int n;

	QSE_ASSERT (client->task.queue.count > 0);
	QSE_ASSERT (client->task.queue.head != QSE_NULL);
	node = client->task.queue.head;
	
	n = node->task.main (httpd, client, &node->task);
	if (n <= -1)
	{
		dequeue_task_locked (httpd, client);
		/*shutdown (client->handle.i, SHUT_RDWR);*/
		client->bad = 1;
	}
	else if (n == 0)
	{
		dequeue_task_locked (httpd, client);
	}
}

#if defined(HAVE_PTHREAD)
static void* response_thread (void* arg)
{
	qse_httpd_t* httpd = (qse_httpd_t*)arg;

	while (!httpd->stopreq)
	{
		int n, max, fd;
		fd_set r, w;
		struct timeval tv;

		pthread_mutex_lock (&httpd->client.mutex);
		max = make_fd_set_from_client_array (httpd, &r, &w, 0);
		pthread_mutex_unlock (&httpd->client.mutex);

		while (max == -1 && !httpd->stopreq)
		{
			struct timeval now;
			struct timespec timeout;

			pthread_mutex_lock (&httpd->client.mutex);

			gettimeofday (&now, QSE_NULL);
			timeout.tv_sec = now.tv_sec + 1;
			timeout.tv_nsec = now.tv_usec * 1000;

			pthread_cond_timedwait (
				&httpd->client.cond, &httpd->client.mutex, &timeout);
			max = make_fd_set_from_client_array (httpd, &r, &w, 0);

			pthread_mutex_unlock (&httpd->client.mutex);
		}

		if (httpd->stopreq) break;

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		n = select (max + 1, &r, &w, QSE_NULL, &tv);
		if (n <= -1)
		{
			/*if (errno == EINTR) continue; */
qse_fprintf (QSE_STDERR, QSE_T("Error: select returned failure - %hs\n"), strerror(errno));
			/* break; */
			continue;
		}
		if (n == 0) 
		{
			continue;
		}

		if (FD_ISSET (httpd->client.pfd[0], &r))
		{
			qse_mchar_t dummy;
			QSE_READ (httpd->client.pfd[0], &dummy, 1);
		}

		for (fd = 0; fd < httpd->client.array.capa; fd++)
		{
			qse_httpd_client_t* client = &httpd->client.array.data[fd];

			if (!client->htrd) continue;

			if (client->bad)
			{
				/*shutdown (client->handle.i, SHUT_RDWR);*/
				pthread_mutex_lock (&httpd->client.mutex);
				delete_from_client_array (httpd, fd);     
				pthread_mutex_unlock (&httpd->client.mutex);
			}
			else if (client->task.queue.head)
			{
				qse_httpd_task_t* task;
				int perform = 0;

				task = &client->task.queue.head->task;

				task->trigger_mask &=
					~(QSE_HTTPD_TASK_TRIGGER_READABLE | 
					  QSE_HTTPD_TASK_TRIGGER_WRITABLE);

				if (!(task->trigger_mask & QSE_HTTPD_TASK_TRIGGER_READ) &&
				    !(task->trigger_mask & QSE_HTTPD_TASK_TRIGGER_WRITE))
				{
					/* no trigger set. set the flag to 
					 * non-readable and non-writable */
					perform = 1;
				}
				else 
				{
				     if ((task->trigger_mask & QSE_HTTPD_TASK_TRIGGER_READ) &&
					    FD_ISSET(task->trigger[0].i, &r))
					{
						/* set the flag to readable */
						task->trigger_mask |= QSE_HTTPD_TASK_TRIGGER_READABLE;
						perform = 1;
					}

				     if ((task->trigger_mask & QSE_HTTPD_TASK_TRIGGER_WRITE) &&
					    FD_ISSET(task->trigger[1].i, &w))
					{
						/* set the flag to writable */
						task->trigger_mask |= QSE_HTTPD_TASK_TRIGGER_WRITABLE;
						perform = 1;
					}
				}
				
				if (perform)
				{
					tv.tv_sec = 0;
					tv.tv_usec = 0;
					FD_ZERO (&w);
					FD_SET (client->handle.i, &w);
					n = select (client->handle.i + 1, QSE_NULL, &w, QSE_NULL, &tv);
					if (n > 0 && FD_ISSET(client->handle.i, &w)) 
					{
						perform_task (httpd, client);
					}
				}
			}
		}
	}

	pthread_exit (QSE_NULL);
	return QSE_NULL;
}
#endif

static int read_from_client (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	qse_mchar_t buf[1024];
	qse_ssize_t m;

	QSE_ASSERT (httpd->cbs->client.recv != QSE_NULL);

reread:
	httpd->errnum = QSE_HTTPD_ENOERR;
	m = httpd->cbs->client.recv (httpd, client, buf, QSE_SIZEOF(buf));
	if (m <= -1)
	{
// TODO: handle errno in the callback... and devise a new return value
//       to indicate no data at this momemnt (EAGAIN, EWOULDBLOCK)...
//       EINTR to be hnalded inside callback if needed...
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			/* nothing to read yet. */
qse_fprintf (QSE_STDERR, QSE_T("Warning: Nothing to read from a client %d\n"), client->handle.i);
			return 0; /* return ok */
		}
		else if (errno != EINTR)
		{
			httpd->errnum = QSE_HTTPD_ESOCKET;
qse_fprintf (QSE_STDERR, QSE_T("Error: failed to read from a client %d\n"), client->handle.i);
			return -1;
		}
		goto reread;
	}
	else if (m == 0)
	{
		httpd->errnum = QSE_HTTPD_EDISCON;
qse_fprintf (QSE_STDERR, QSE_T("Debug: connection closed %d\n"), client->handle.i);
		return -1;
	}

	/* feed may have called the request callback multiple times... 
 	 * that's because we don't know how many valid requests
	 * are included in 'buf' */ 
qse_fprintf (QSE_STDERR, QSE_T("Debug: read from a client %d\n"), client->handle.i);

	httpd->errnum = QSE_HTTPD_ENOERR;
	if (qse_htrd_feed (client->htrd, buf, m) <= -1)
	{
		if (httpd->errnum == QSE_HTTPD_ENOERR)
		{
			if (client->htrd->errnum == QSE_HTRD_EBADRE || 
			    client->htrd->errnum == QSE_HTRD_EBADHDR)
				httpd->errnum = QSE_HTTPD_EBADREQ;
			else httpd->errnum = QSE_HTTPD_ENOMEM; /* TODO: better translate error code */
		}

qse_fprintf (QSE_STDERR, QSE_T("Error: http error while processing \n"));
		return -1;
	}

	return 0;
}

int qse_httpd_loop (qse_httpd_t* httpd, qse_httpd_cbs_t* cbs, int threaded)
{
#if defined(HAVE_PTHREAD)
	pthread_t response_thread_id;
#endif

	httpd->threaded = 0;
	httpd->stopreq = 0;
	httpd->cbs = cbs;

	QSE_ASSERTX (httpd->listener.list != QSE_NULL,
		"Add listeners before calling qse_httpd_loop()"
	);	

	QSE_ASSERTX (httpd->cbs != QSE_NULL,
		"Set httpd callbacks before calling qse_httpd_loop()"
	);	

	if (httpd->listener.list == QSE_NULL)
	{
		/* no listener specified */
		httpd->errnum = QSE_HTTPD_EINVAL;
		return -1;
	}

	if (activate_listeners (httpd) <= -1) return -1;

	init_client_array (httpd);

#if defined(HAVE_PTHREAD)
	/* start the response sender as a thread */
	if (threaded)
	{
		if (QSE_PIPE(httpd->client.pfd) == 0)
		{
			int i;
			for (i = 0; i < 2; i++)
			{
				int flags = QSE_FCNTL (httpd->client.pfd[i], F_GETFD, 0);
				if (flags >= 0)
					QSE_FCNTL (httpd->client.pfd[i], F_SETFD, flags | FD_CLOEXEC);
			}
		
			pthread_mutex_init (&httpd->client.mutex, QSE_NULL);
			pthread_cond_init (&httpd->client.cond, QSE_NULL);

			/* set this before creating a thread
			 * because this is accessed in a thread.
			 * if i set this after pthread_create, a thread
			 * function may still see 0. */
			httpd->threaded = 1; 

			if (pthread_create (
				&response_thread_id, QSE_NULL,
				response_thread, httpd) != 0) 
			{
				httpd->threaded = 0;
				pthread_cond_destroy (&httpd->client.cond);
				pthread_mutex_destroy (&httpd->client.mutex);
				QSE_CLOSE (httpd->client.pfd[1]);
				QSE_CLOSE (httpd->client.pfd[0]);
			}
		}
	}
#endif

	while (!httpd->stopreq)
	{
		int n, max, fd;
		fd_set r;
		fd_set w;
		struct timeval tv;

		tv.tv_sec = 1;
		tv.tv_usec = 0;

#if defined(HAVE_PTHREAD)
		if (httpd->threaded) pthread_mutex_lock (&httpd->client.mutex);
#endif
		max = make_fd_set_from_client_array (httpd, &r, &w, 1);
#if defined(HAVE_PTHREAD)
		if (httpd->threaded) pthread_mutex_unlock (&httpd->client.mutex);
#endif

		n = select (max + 1, &r, &w, QSE_NULL, &tv);
		if (n <= -1)
		{
			httpd->errnum = QSE_HTTPD_EIOMUX;
/* TODO: call user callback for this multiplexer error */
			/*if (errno == EINTR) continue;*/
qse_fprintf (QSE_STDERR, QSE_T("Error: select returned failure\n"));
			/* break; */
			continue;
		}
		if (n == 0) 
		{
			continue;
		}

		/* check the listener activity */
		accept_client_from_listeners (httpd, &r);

		/* check the client activity */
		for (fd = 0; fd < httpd->client.array.capa; fd++)
		{
			qse_httpd_client_t* client = &httpd->client.array.data[fd];

			if (!client->htrd) continue;

			if (FD_ISSET(client->handle.i, &r))
			{
				/* got input */
				if (!client->ready)
				{
					/* if client.accepted() returns 0, it is called
					 * again next time. */
					QSE_ASSERT (httpd->cbs->client.accepted != QSE_NULL);
					int x = httpd->cbs->client.accepted (httpd, client); /* is this correct???? what if ssl handshaking got stalled because writing failed in SSL_accept()? */
					if (x >= 1) client->ready = 1;
					else if (x <= -1) goto bad_client;
				}
				else
				{
					if (read_from_client (httpd, client) <= -1)
					{
					bad_client:
						if (httpd->threaded)
						{
							/* let the writing part handle it,  
							 * probably in the next iteration */
							qse_httpd_markbadclient (httpd, client);
							shutdown (client->handle.i, SHUT_RDWR);
						}
						else
						{
							/*pthread_mutex_lock (&httpd->client.mutex);*/
							delete_from_client_array (httpd, fd);     
							/*pthread_mutex_unlock (&httpd->client.mutex);*/
							continue; /* don't need to go to the writing part */		
						}
					}
				}
			}

			if (!httpd->threaded)
			{
				if (client->bad)
				{
					/*shutdown (client->handle.i, SHUT_RDWR);*/
					/*pthread_mutex_lock (&httpd->client.mutex);*/
					delete_from_client_array (httpd, fd);     
					/*pthread_mutex_unlock (&httpd->client.mutex);*/
				}
				else if (client->task.queue.head)
				{
					qse_httpd_task_t* task;
					int perform = 0;

					task = &client->task.queue.head->task;
					task->trigger_mask &=
						~(QSE_HTTPD_TASK_TRIGGER_READABLE | 
						  QSE_HTTPD_TASK_TRIGGER_WRITABLE);

					if (!(task->trigger_mask & QSE_HTTPD_TASK_TRIGGER_READ) &&
					    !(task->trigger_mask & QSE_HTTPD_TASK_TRIGGER_WRITE))
					{
						/* no trigger set. set the flag to 
						 * non-readable and non-writable */
						perform = 1;
					}
					else 
					{
					     if ((task->trigger_mask & QSE_HTTPD_TASK_TRIGGER_READ) &&
						    FD_ISSET(task->trigger[0].i, &r))
						{
							/* set the flag to readable */
							task->trigger_mask |= QSE_HTTPD_TASK_TRIGGER_READABLE;
							perform = 1;
						}
	
					     if ((task->trigger_mask & QSE_HTTPD_TASK_TRIGGER_WRITE) &&
						    FD_ISSET(task->trigger[1].i, &w))
						{
							/* set the flag to writable */
							task->trigger_mask |= QSE_HTTPD_TASK_TRIGGER_WRITABLE;
							perform = 1;
						}
					}
	
					if (perform)
					{
						tv.tv_sec = 0;
						tv.tv_usec = 0;
						FD_ZERO (&w);
						FD_SET (client->handle.i, &w);
						n = select (client->handle.i + 1, QSE_NULL, &w, QSE_NULL, &tv);

						/* TODO: logging if n == -1 */

						if (n > 0 && FD_ISSET(client->handle.i, &w))
							perform_task (httpd, client);
					}
				}
			}
		}
	}

#if defined(HAVE_PTHREAD)
	if (httpd->threaded) 
	{
		pthread_join (response_thread_id, QSE_NULL);
		pthread_cond_destroy (&httpd->client.cond);
		pthread_mutex_destroy (&httpd->client.mutex);
		QSE_CLOSE (httpd->client.pfd[1]);
		QSE_CLOSE (httpd->client.pfd[0]);
	}
#endif

	fini_client_array (httpd);
	deactivate_listeners (httpd);
	return 0;
}

static void free_listener (qse_httpd_t* httpd, listener_t* l)
{
	if (l->host) qse_httpd_freemem (httpd, l->host);
	if (l->handle >= 0) close (l->handle);
	qse_httpd_freemem (httpd, l);
}

static void free_listener_list (qse_httpd_t* httpd, listener_t* l)
{
	while (l)
	{
		listener_t* cur = l;
		l = l->next;
		free_listener (httpd, cur);
	}
}

static listener_t* parse_listener_uri (
	qse_httpd_t* httpd, const qse_char_t* uri)
{
	const qse_char_t* p = uri;
	listener_t* ltmp = QSE_NULL;
	qse_cstr_t tmp;
	qse_mchar_t* host;
	int x;

	ltmp = qse_httpd_allocmem (httpd, QSE_SIZEOF(*ltmp));
	if (ltmp == QSE_NULL) goto oops; /* alloc set error number. so goto oops */

	QSE_MEMSET (ltmp, 0, QSE_SIZEOF(*ltmp));
	ltmp->handle = -1;

	/* check the protocol part */
	tmp.ptr = p;
	while (*p != QSE_T(':')) 
	{
		if (*p == QSE_T('\0')) goto oops_einval;
		p++;
	}
	tmp.len = p - tmp.ptr;
	if (qse_strxcmp (tmp.ptr, tmp.len, QSE_T("http")) == 0) 
	{
		ltmp->secure = 0;
		ltmp->port = DEFAULT_PORT;
	}
	else if (qse_strxcmp (tmp.ptr, tmp.len, QSE_T("https")) == 0) 
	{
		ltmp->secure = 1;
		ltmp->port = DEFAULT_SECURE_PORT;
	}
	else goto oops;
	
	p++; /* skip : */ 
	if (*p != QSE_T('/')) goto oops_einval;
	p++; /* skip / */
	if (*p != QSE_T('/')) goto oops_einval;
	p++; /* skip / */

#ifdef AF_INET6
	if (*p == QSE_T('['))	
	{
		/* IPv6 address */
		p++; /* skip [ */

		for (tmp.ptr = p; *p != QSE_T(']'); p++)
		{
			if (*p == QSE_T('\0')) goto oops_einval;
		}

		tmp.len = p - tmp.ptr;
		ltmp->family = AF_INET6;

		p++; /* skip ] */
		if (*p != QSE_T(':') && *p != QSE_T('\0'))  goto oops_einval;
	}
	else
	{
#endif
		/* host name or IPv4 address */
		for (tmp.ptr = p; ; p++)
		{
			if (*p == QSE_T(':') || *p == QSE_T('\0')) break;
		}
		tmp.len = p - tmp.ptr;
		ltmp->family = AF_INET;
#ifdef AF_INET6
	}
#endif

	ltmp->host = qse_strxdup (tmp.ptr, tmp.len, httpd->mmgr);
	if (ltmp->host == QSE_NULL) goto oops_enomem;

#ifdef QSE_CHAR_IS_WCHAR
	host = qse_wcstombsdup (ltmp->host, httpd->mmgr);
	if (host == QSE_NULL) goto oops_enomem;
#else
	host = ltmp->host;
#endif

	x = inet_pton (ltmp->family, host, &ltmp->addr);
#ifdef QSE_CHAR_IS_WCHAR
	qse_httpd_freemem (httpd, host);
#endif
	if (x != 1)
	{
		/* TODO: need to support host names??? 
		if (getaddrinfo... )....
or CALL a user callback for name resolution? 
		if (httpd->cbs.resolve_hostname (httpd, ltmp->host) <= -1) must call this with host before freeing it up????
		 */
		goto oops_einval;
	}

	if (*p == QSE_T(':')) 
	{
		unsigned int port = 0;

		/* port number */
		p++;

		for (tmp.ptr = p; QSE_ISDIGIT(*p); p++)
			port = port * 10 + (*p - QSE_T('0'));

		tmp.len = p - tmp.ptr;
		if (tmp.len <= 0 ||
		    tmp.len >= 6 || 
		    port > QSE_TYPE_MAX(unsigned short)) goto oops_einval;
		ltmp->port = port;
	}

	/* skip spaces */
	while (QSE_ISSPACE(*p)) p++;

	return ltmp;

oops_einval:
	httpd->errnum = QSE_HTTPD_EINVAL;
	goto oops;

oops_enomem:
	httpd->errnum = QSE_HTTPD_ENOMEM;
	goto oops;

oops:
	if (ltmp) free_listener (httpd, ltmp);
	return QSE_NULL;
}

static int add_listener (qse_httpd_t* httpd, const qse_char_t* uri)
{
	listener_t* lsn;

	lsn = parse_listener_uri (httpd, uri);
	if (lsn == QSE_NULL) return -1;

	lsn->next = httpd->listener.list;
	httpd->listener.list = lsn;
/* 
TODO: mutex protection...
if in the activated state...
activate_additional_listeners (httpd, l) 
recalc httpd->listener.set.
recalc httpd->listener.max.
*/
	return 0;
}

#if 0
static int delete_listeners (qse_httpd_t* httpd, const qse_char_t* uri)
{
	listener_t* lh, * li, * hl;

	lh = parse_listener_uri (httpd, uri, QSE_NULL);
	if (lh == QSE_NULL) return -1;

	for (li = lh; li; li = li->next)
	{
		for (hl = httpd->listener.list; hl; hl = hl->next)
		{
			if (li->secure == hl->secuire &&
			    li->addr == hl->addr &&
			    li->port == hl->port)
			{
				mark_delete	
			}	
		}
	}
}
#endif

int qse_httpd_addlistener (qse_httpd_t* httpd, const qse_char_t* uri)
{
#if defined(HAVE_PTHREAD)
	int n;
	pthread_mutex_lock (&httpd->listener.mutex);
	n = add_listener (httpd, uri);
	pthread_mutex_unlock (&httpd->listener.mutex);
	return n;
#else
	return add_listener (httpd, uri);
#endif
}

#if 0
int qse_httpd_dellistener (qse_httpd_t* httpd, const qse_char_t* uri)
{
	int n;
	pthread_mutex_lock (&httpd->listener.mutex);
	n = delete_listeners (httpd, uri);
	pthread_mutex_unlock (&httpd->listener.mutex);
	return n;
}

void qse_httpd_clearlisteners (qse_httpd_t* httpd)
{
	pthread_mutex_lock (&httpd->listener.mutex);
	deactivate_listeners (httpd);
	free_listener_list (httpd, httpd->listener.list);
	httpd->listener.list = QSE_NULL;
	pthread_mutex_unlock (&httpd->listener.mutex);
}
#endif

qse_httpd_task_t* qse_httpd_entask (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	const qse_httpd_task_t* pred, const qse_httpd_task_t* task,
	qse_size_t xtnsize)
{
	qse_httpd_task_t* ret;
	ret = enqueue_task_locked (httpd, client, pred, task, xtnsize);
	if (ret == QSE_NULL) client->bad = 1; /* mark this client bad */
#if defined(HAVE_PTHREAD)
	else if (httpd->threaded) 
	{
		static qse_byte_t dummy = 0x01;
		/* write to the pipe to wake up select() in 
		 * the response thread if it was blocking. */
		QSE_WRITE (httpd->client.pfd[1], &dummy, 1); 

		pthread_cond_signal (&httpd->client.cond);
	}
#endif
	return ret;
}

void qse_httpd_markbadclient (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	/* mark that something is wrong in processing requests from this client.
	 * this client could be bad... or the system could encounter some errors
	 * like memory allocation failure */
	client->bad = 1;
}

void qse_httpd_discardcontent (qse_httpd_t* httpd, qse_htre_t* req)
{
	req->flags |= QSE_HTRE_DISCARDED;
	/* clear the content buffer in case it has received contents 
	 * partially already */
	qse_mbs_clear (&req->content);
}

#endif
