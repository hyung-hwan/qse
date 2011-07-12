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

#include "httpd.h"
#include "../cmn/mem.h"
#include <qse/cmn/chr.h>

#if 0
#include <openssl.h>
#endif

QSE_IMPLEMENT_COMMON_FUNCTIONS (httpd)

#define DEFAULT_PORT        80
#define DEFAULT_SECURE_PORT 443

static void free_listener_list (qse_httpd_t* httpd, listener_t* l);

qse_httpd_t* qse_httpd_open (qse_mmgr_t* mmgr, qse_size_t xtnsize)
{
	qse_httpd_t* httpd;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	httpd = (qse_httpd_t*) QSE_MMGR_ALLOC (
		mmgr, QSE_SIZEOF(*httpd) + xtnsize
	);
	if (httpd == QSE_NULL) return QSE_NULL;

	if (qse_httpd_init (httpd, mmgr) == QSE_NULL)
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

qse_httpd_t* qse_httpd_init (qse_httpd_t* httpd, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (httpd, 0, QSE_SIZEOF(*httpd));
	httpd->mmgr = mmgr;
	httpd->listener.max = -1;

	pthread_mutex_init (&httpd->listener.mutex, QSE_NULL);

	return httpd;
}

void qse_httpd_fini (qse_httpd_t* httpd)
{
	/* TODO */
	pthread_mutex_destroy (&httpd->listener.mutex);
	free_listener_list (httpd, httpd->listener.list);
	httpd->listener.list = QSE_NULL;
}

void qse_httpd_stop (qse_httpd_t* httpd)
{
	httpd->stopreq = 1;
}

#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

#include <sys/sendfile.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdio.h>

#include <qse/cmn/stdio.h>

#define MAX_SENDFILE_SIZE 4096
//#define MAX_SENDFILE_SIZE 64

typedef struct http_xtn_t http_xtn_t;

struct http_xtn_t
{
	client_array_t* array;
	qse_size_t      index; 
};

static int enqueue_client_action_unlocked (client_t* client, const client_action_t* action)
{
	int index;

	if (client->action.count >= QSE_COUNTOF(client->action.target)) return -1;
	
	index = (client->action.offset + client->action.count) % 
	        QSE_COUNTOF(client->action.target);
	client->action.target[index] = *action;
	client->action.count++;
	return 0;
}

static int enqueue_client_action_locked (client_t* client, const client_action_t* action)
{
	int ret;
	pthread_mutex_lock (&client->action_mutex);
	ret = enqueue_client_action_unlocked (client, action);
	pthread_mutex_unlock (&client->action_mutex);
	return ret;
}

static int dequeue_client_action_unlocked (client_t* client, client_action_t* action)
{
	client_action_t* actp;

	if (client->action.count <= 0) return -1;

	actp = &client->action.target[client->action.offset];
	if (actp->type == ACTION_SENDFILE) close (actp->u.sendfile.fd);
	else if (actp->type == ACTION_SENDTEXTDUP) free (actp->u.sendtextdup.ptr);

	if (action) *action = *actp;
	client->action.offset = (client->action.offset + 1) % QSE_COUNTOF(client->action.target);
	client->action.count--;
	return 0;
}

static int dequeue_client_action_locked (client_t* client, client_action_t* action)
{
	int ret;
	pthread_mutex_lock (&client->action_mutex);
	ret = dequeue_client_action_unlocked (client, action);
	pthread_mutex_unlock (&client->action_mutex);
	return ret;
}

static void purge_client_actions_locked (client_t* client)
{
	client_action_t action;
	pthread_mutex_lock (&client->action_mutex);
	while (dequeue_client_action_unlocked (client, &action) == 0);
	pthread_mutex_unlock (&client->action_mutex);
}

static int enqueue_sendtext_locked (client_t* client, const char* text)
{
	client_action_t action;

	memset (&action, 0, sizeof(action));
	action.type = ACTION_SENDTEXT;
	action.u.sendtext.ptr = text;
	action.u.sendtext.left = strlen(text);

	return enqueue_client_action_locked (client, &action);
}

static int format_and_do (int (*task) (void* arg, char* text), void* arg, const char* fmt, ...)
{
	va_list ap;
	char n[2];
	char* buf;
	int bytes_req;

	va_start (ap, fmt);
#if defined(_WIN32) && defined(_MSC_VER)
	bytes_req = _vsnprintf (n, 1, fmt, ap);
#else
	bytes_req = vsnprintf (n, 1, fmt, ap);
#endif
	va_end (ap);
	if (bytes_req == -1) 
	{
		qse_size_t capa = 256;
		buf = (char*) malloc (capa + 1);

		/* an old vsnprintf behaves differently from C99 standard.
		 * thus, it returns -1 when it can't write all the input given. */
		for (;;) 
		{
			int l;
			va_start (ap, fmt);
#if defined(_WIN32) && defined(_MSC_VER)
			if ((l = _vsnprintf (buf, capa + 1, fmt, ap)) == -1) 
#else
			if ((l = vsnprintf (buf, capa + 1, fmt, ap)) == -1) 
#endif
			{
				va_end (ap);

				free (buf);
				capa = capa * 2;
				buf = (char*)malloc (capa + 1);
				if (buf == NULL) return -1;

				continue;
			}

			va_end (ap);
			break;
		}
	}
	else 
	{
		/* vsnprintf returns the number of characters that would 
		 * have been written not including the terminating '\0' 
		 * if the _data buffer were large enough */
		buf = (char*)malloc (bytes_req + 1);
		if (buf == NULL) return -1;

		va_start (ap, fmt);
#if defined(_WIN32) && defined(_MSC_VER)
		_vsnprintf (buf, bytes_req + 1, fmt, ap);
#else
		vsnprintf (buf, bytes_req + 1, fmt, ap);
#endif

		//_data[_size] = '\0';
		va_end (ap);
	}

	return task (arg, buf);
}

static int enqueue_format (void* arg, char* text)
{
	client_t* client = (client_t*)arg;

	client_action_t action;

	memset (&action, 0, sizeof(action));
	action.type = ACTION_SENDTEXTDUP;
	action.u.sendtextdup.ptr = text;
	action.u.sendtextdup.left = strlen(text);

	if (enqueue_client_action_locked (client, &action) <= -1)
	{
		free (text);
		return -1;
	}

	return 0;
}

static int enqueue_sendtextdup_locked (client_t* client, const char* text)
{
	client_action_t action;
	char* textdup;

	textdup = strdup (text);
	if (textdup == NULL) return -1;

	memset (&action, 0, sizeof(action));
	action.type = ACTION_SENDTEXTDUP;
	action.u.sendtextdup.ptr = textdup;
	action.u.sendtextdup.left = strlen(textdup);

	if (enqueue_client_action_locked (client, &action) <= -1)
	{
		free (textdup);
		return -1;
	}

	return 0;
}

static int enqueue_sendfile_locked (client_t* client, int fd)
{
	client_action_t action;
	struct stat st;

	if (fstat (fd, &st) <= -1) return -1;

	memset (&action, 0, sizeof(action));
	action.type = ACTION_SENDFILE;
	action.u.sendfile.fd = fd;
	action.u.sendfile.left = st.st_size;;

	return enqueue_client_action_locked (client, &action);
}

static int enqueue_disconnect (client_t* client)
{
	client_action_t action;

	memset (&action, 0, sizeof(action));
	action.type = ACTION_DISCONNECT;

	return enqueue_client_action_locked (client, &action);
}

static qse_htb_walk_t walk (qse_htb_t* htb, qse_htb_pair_t* pair, void* ctx)
{
qse_printf (QSE_T("HEADER OK %d[%S] %d[%S]\n"),  (int)QSE_HTB_KLEN(pair), QSE_HTB_KPTR(pair), (int)QSE_HTB_VLEN(pair), QSE_HTB_VPTR(pair));
     return QSE_HTB_WALK_FORWARD;
}

static int capture_param (qse_htrd_t* http, const qse_mcstr_t* key, const qse_mcstr_t* val)
{
qse_printf (QSE_T("PARAM %d[%S] => %d[%S] \n"), (int)key->len, key->ptr, (int)val->len, val->ptr);
	return 0;
}

static int handle_request (qse_htrd_t* http, qse_htre_t* req)
{
	http_xtn_t* xtn = (http_xtn_t*) qse_htrd_getxtn (http);
	client_t* client = &xtn->array->data[xtn->index];
	int method;

qse_printf (QSE_T("================================\n"));
qse_printf (QSE_T("REQUEST ==> [%S] version[%d.%d] method[%d]\n"), 
	qse_htre_getqpathptr(req),
	qse_htre_getmajorversion(req),
	qse_htre_getminorversion(req),
	qse_htre_getqmethod(req)
);
if (qse_htre_getqparamlen(req) > 0)
{
qse_printf (QSE_T("PARAMS ==> [%S]\n"), qse_htre_getqparamptr(req));
}

qse_htb_walk (&http->re.hdrtab, walk, QSE_NULL);
if (QSE_MBS_LEN(&http->re.content) > 0)
{
qse_printf (QSE_T("content = [%.*S]\n"),
		(int)QSE_MBS_LEN(&http->re.content),
		QSE_MBS_PTR(&http->re.content));
}

	method = qse_htre_getqmethod (req);

	if (method == QSE_HTTP_GET || method == QSE_HTTP_POST)
	{
		int fd;

		//if (qse_htrd_scanqparam (http, qse_htre_getqparamcstr(req)) <= -1)
		if (qse_htrd_scanqparam (http, QSE_NULL) <= -1)
		{
const char* msg = "<html><head><title>INTERNAL SERVER ERROR</title></head><body><b>INTERNAL SERVER ERROR</b></body></html>";
if (format_and_do (enqueue_format, client, 
	"HTTP/%d.%d 500 Internal Server Error\r\nContent-Length: %d\r\n\r\n%s\r\n\r\n", 
	req->version.major, 
	req->version.minor,
	(int)strlen(msg) + 4, msg) <= -1)
{
qse_printf (QSE_T("failed to push action....\n"));
	return -1;
}
		}

		if (method == QSE_HTTP_POST)
		{
qse_printf (QSE_T("BEGIN FORM FIELDS=============\n"));
			if (qse_htrd_scanqparam (http, qse_htre_getcontentcstr(req)) <= -1)
			{
			}
qse_printf (QSE_T("END FORM FIELDS=============\n"));
		}

		fd = open (qse_htre_getqpathptr(req), O_RDONLY);
		if (fd <= -1)
		{
const char* msg = "<html><head><title>NOT FOUND</title></head><body><b>REQUESTD FILE NOT FOUND</b></body></html>";
if (format_and_do (enqueue_format, client, 
	"HTTP/%d.%d 404 Not found\r\nContent-Length: %d\r\n\r\n%s\r\n\r\n", 
	req->version.major, 
	req->version.minor,
	(int)strlen(msg) + 4, msg) <= -1)
{
qse_printf (QSE_T("failed to push action....\n"));
	return -1;
}
		}
		else
		{
			struct stat st;
			if (fstat (fd, &st) <= -1)
			{
				close (fd);

qse_printf (QSE_T("fstat failure....\n"));
			}
			else if (st.st_size <= 0)
			{
				close (fd);

qse_printf (QSE_T("empty file....\n"));
#if 0
				qse_htre_t* res = qse_htrd_newresponse (http);
				if (req == QSE_NULL)
				{
					/* hard failure... can't answer */
					/* arrange to close connection */
				}


				ptr = qse_htrd_emitresponse (http, res, &len);
				if (ptr == QSE_NULL)
				{
					/* hard failure... can't answer */
					/* arrange to close connection */
				}

				action.type = ACTION_SENDTEXT;
				action.u.sendtext.ptr = ptr;
				action.u.sendtext.len = len;
#endif
			}
			else
			{

char text[128];
snprintf (text, sizeof(text),
	"HTTP/%d.%d 200 OK\r\nContent-Length: %llu\r\nContent-Location: %s\r\n\r\n", 
	qse_htre_getmajorversion(req),
	qse_htre_getminorversion(req),
	(unsigned long long)st.st_size,
	qse_htre_getqpathptr(req)
);

#if 0
				if (enqueue_sendfmt_locked (client,
					"HTTP/%d.%d 200 OK\r\nContent-Length: %llu\r\n\r\n", 
					req->version.major, 
					req->version.minor, 
					(unsigned long long)st.st_size) <= -1)
				{
				}
#endif

				if (enqueue_sendtextdup_locked (client, text) <= -1)
				{
qse_printf (QSE_T("failed to push action....\n"));
					return -1;
				}

				if (enqueue_sendfile_locked (client, fd) <= -1)
				{
	/* TODO: close??? just close....??? */
qse_printf (QSE_T("failed to push action....\n"));
					return -1;
				}

				if (req->attr.connection_close)
				{
					if (enqueue_disconnect (client) <= -1)
					{
qse_printf (QSE_T("failed to push action....\n"));
						return -1;
					}
				}
			}
		}	
	}
	else
	{
char text[256];
const char* msg = "<html><head><title>Method not allowed</title></head><body><b>METHOD NOT ALLOWED</b></body></html>";
snprintf (text, sizeof(text),
	"HTTP/%d.%d 405 Method not allowed\r\nContent-Length: %d\r\n\r\n%s\r\n\r\n", 
	qse_htre_getmajorversion(req),
	qse_htre_getminorversion(req),
	(int)strlen(msg)+4, msg);
if (enqueue_sendtextdup_locked (client, text) <= -1)
{
qse_printf (QSE_T("failed to push action....\n"));
return -1;
}

	}

	pthread_cond_signal (&xtn->array->cond);
	return 0;
}

static int handle_expect_continue (qse_htrd_t* http, qse_htre_t* req)
{
	http_xtn_t* xtn = (http_xtn_t*) qse_htrd_getxtn (http);
	client_t* client = &xtn->array->data[xtn->index];

	if (qse_htre_getqmethod(req) == QSE_HTTP_POST)
	{
		char text[32];

		snprintf (text, sizeof(text),
			"HTTP/%d.%d 100 OK\r\n\r\n", 
			req->version.major, req->version.minor);

		if (enqueue_sendtextdup_locked (client, text) <= -1)
		{
			return -1;
		}
	}
	else
	{
		char text[32];

		qse_htre_setdiscard (req, 1);

		snprintf (text, sizeof(text),
			"HTTP/%d.%d 404 Not found\r\n\r\n", 
			req->version.major, req->version.minor);

		if (enqueue_sendtextdup_locked (client, text) <= -1)
		{
			return -1;
		}
	}

	return 0;
}

static int handle_response (qse_htrd_t* http, qse_htre_t* res)
{
#if 0
	if (res->code >= 100 && res->code <= 199)
	{
		/* informational response */
	}
#endif

	qse_printf (QSE_T("response received... HTTP/%d.%d %d %.*S\n"), 
		qse_htre_getmajorversion(res),
		qse_htre_getminorversion(res),
		qse_htre_getsstatus(res),
		(int)qse_htre_getsmessagelen(res),
		qse_htre_getsmessageptr(res)
	);
	return -1;
}

static qse_ssize_t receive_octets (qse_htrd_t* http, qse_htoc_t* buf, qse_size_t size)
{
	http_xtn_t* xtn = (http_xtn_t*) qse_htrd_getxtn (http);
	client_t* client = &xtn->array->data[xtn->index];
	ssize_t n;

	n = recv (client->fd, buf, size, 0);
	if (n <= -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
		http->errnum = 99999;	

	return n;
}

qse_htrd_recbs_t http_recbs =
{
	receive_octets,
	handle_request,
	handle_response,
	handle_expect_continue,
	capture_param
};


static void deactivate_listener (qse_httpd_t* httpd, listener_t* l)
{
	QSE_ASSERT (l->handle >= 0);

/* TODO: callback httpd->cbs.listener_deactivated (httpd, l);*/
/* TODO: suport https... */
	close (l->handle);
	l->handle = -1;
}

static int activate_listner (qse_httpd_t* httpd, listener_t* l)
{
/* TODO: suport https... */
	sockaddr_t addr;
	int s = -1, flag;

	QSE_ASSERT (l->handle <= -1);

	s = socket (l->family, SOCK_STREAM, IPPROTO_TCP);
	if (s <= -1) goto oops_esocket;

	flag = 1;
	setsockopt (s, SOL_SOCKET, SO_REUSEADDR, &flag, QSE_SIZEOF(flag));

	QSE_MEMSET (&addr, 0, QSE_SIZEOF(addr));
	switch (l->family)
	{
		case AF_INET:
		{
			addr.in4.sin_family = l->family;	
			addr.in4.sin_addr = l->addr.in4;
			addr.in4.sin_port = htons (l->port);
			break;
		}

		case AF_INET6:
		{
			addr.in6.sin6_family = l->family;	
			addr.in6.sin6_addr = l->addr.in6;
			addr.in6.sin6_port = htons (l->port);
			/* TODO: addr.in6.sin6_scope_id  */
			break;
		}

		default:
		{
			QSE_ASSERT (!"This should never happen");
		}
	}

	if (bind (s, (struct sockaddr*)&addr, QSE_SIZEOF(addr)) <= -1) goto oops_esocket;
	if (listen (s, 10) <= -1) goto oops_esocket;
	
	flag = fcntl (s, F_GETFL);
	if (flag >= 0) fcntl (s, F_SETFL, flag | O_NONBLOCK);
	fcntl (s, F_SETFD, FD_CLOEXEC);

#if 0
/* TODO: */
	if (l->secure)
	{
		SSL_CTX* ctx;
		SSL* ssl;

		ctx = SSL_ctx_new (SSLv3_method());
		ssl =  SSL_new (ctx);
	}
#endif

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

static int activate_listners (qse_httpd_t* httpd)
{
	listener_t* l;
	fd_set listener_set;
	int listener_max = -1;

	FD_ZERO (&listener_set);	
	for (l = httpd->listener.list; l; l = l->next)
	{
		if (l->handle <= -1)
		{
			if (activate_listner (httpd, l) <= -1) goto oops;
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

static void init_client_array (client_array_t* array)
{
	array->capa = 0;
	array->size = 0;
	array->data = QSE_NULL;
	pthread_cond_init (&array->cond, NULL);
}

static void delete_from_client_array (client_array_t* array, int fd)
{
	if (array->data[fd].http)
	{
		purge_client_actions_locked (&array->data[fd]);
		pthread_mutex_destroy (&array->data[fd].action_mutex);

		qse_htrd_close (array->data[fd].http);
		array->data[fd].http = QSE_NULL;	
		close (array->data[fd].fd);
		array->size--;
	}
}

static void fini_client_array (client_array_t* array)
{
	if (array->data) 
	{
		int fd;

		for (fd = 0; fd < array->capa; fd++)
			delete_from_client_array (array, fd);

		free (array->data);
		array->capa = 0;
		array->size = 0;
		array->data = QSE_NULL;
	}
	pthread_cond_destroy (&array->cond);
}

static client_t* insert_into_client_array (
	client_array_t* array, int fd, sockaddr_t* addr)
{
	http_xtn_t* xtn;

	if (fd >= array->capa)
	{
	#define ALIGN 512
		client_t* tmp;
		qse_size_t capa = ((fd + ALIGN) / ALIGN) * ALIGN;

		tmp = realloc (array->data, capa * QSE_SIZEOF(client_t));
		if (tmp == QSE_NULL) return QSE_NULL;

		memset (&tmp[array->capa], 0,
			QSE_SIZEOF(client_t) * (capa - array->capa));

		array->data = tmp;
		array->capa = capa;
	}

	QSE_ASSERT (array->data[fd].http == QSE_NULL);

	array->data[fd].fd = fd;	
	array->data[fd].addr = *addr;
	array->data[fd].http = qse_htrd_open (QSE_MMGR_GETDFL(), QSE_SIZEOF(*xtn));
	if (array->data[fd].http == QSE_NULL) return QSE_NULL;
	pthread_mutex_init (&array->data[fd].action_mutex, NULL);

	xtn = (http_xtn_t*)qse_htrd_getxtn (array->data[fd].http);	
	xtn->array = array;
	xtn->index = fd; 

	qse_htrd_setrecbs (array->data[fd].http, &http_recbs);
	array->size++;
	return &array->data[fd];
}

static int accept_client_from_listener (qse_httpd_t* httpd, listener_t* l)
{
	int flag, c;
	sockaddr_t addr;
	socklen_t addrlen = QSE_SIZEOF(addr);
	client_t* client;

/* TODO:
	if (l->secure)
	{
SSL_Accept
	....
	}
*/

	c = accept (l->handle, (struct sockaddr*)&addr, &addrlen);
	if (c <= -1)
	{
		httpd->errnum = QSE_HTTPD_ESOCKET;
qse_fprintf (QSE_STDERR, QSE_T("Error: accept returned failure\n"));
		goto oops;
	}

	/* select() uses a fixed-size array so the file descriptor can not
	 * exceeded FD_SETSIZE */
	if (c >= FD_SETSIZE)
	{
		close (c);

qse_fprintf (QSE_STDERR, QSE_T("Error: too many client?\n"));
		/* httpd->errnum = QSE_HTTPD_EOVERFLOW; */
		goto oops;
	}

	/* set the nonblock flag in case read() after select() blocks
	 * for various reasons - data received may be dropped after 
	 * arrival for wrong checksum, for example. */
	flag = fcntl (c, F_GETFL);
	if (flag >= 0) fcntl (c, F_SETFL, flag | O_NONBLOCK);
	fcntl (c, F_SETFD, FD_CLOEXEC);

	pthread_mutex_lock (&httpd->camutex);
	client = insert_into_client_array (&httpd->ca, c, &addr);
	pthread_mutex_unlock (&httpd->camutex);
	if (client == QSE_NULL)
	{
		close (c);
		goto oops;
	}

qse_printf (QSE_T("connection %d accepted\n"), c);

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


static int make_fd_set_from_client_array (qse_httpd_t* httpd, fd_set* r, fd_set* w)
{
	int fd, max = -1;
	client_array_t* ca = &httpd->ca;

	if (r)
	{
		max = httpd->listener.max;
		*r = httpd->listener.set;
	}
	if (w)
	{
		FD_ZERO (w);
	}

	for (fd = 0; fd < ca->capa; fd++)
	{
		if (ca->data[fd].http) 
		{
			if (r) 
			{
				FD_SET (ca->data[fd].fd, r);
				if (ca->data[fd].fd > max) max = ca->data[fd].fd;
			}
			if (w && ca->data[fd].action.count > 0)
			{
				/* add it to the set if it has a response to send */
				FD_SET (ca->data[fd].fd, w);
				if (ca->data[fd].fd > max) max = ca->data[fd].fd;
			}
		}
	}

	return max;
}

static int take_client_action (client_t* client)
{
	client_action_t* action;

	action = &client->action.target[client->action.offset];

	switch (action->type)
	{
		case ACTION_SENDTEXT:
		{
			ssize_t n;
			size_t count;

			count = MAX_SENDFILE_SIZE;
			if (count >= action->u.sendtext.left)
				count = action->u.sendtext.left;

			n = send (client->fd, action->u.sendtext.ptr, count, 0);
			if (n <= -1) 
			{
qse_printf (QSE_T("send text failure... arrange to close this connection....\n"));
				dequeue_client_action_locked (client, NULL);
				shutdown (client->fd, SHUT_RDWR);
			}
			else
			{
/* TODO: what if n is 0???? does it mean EOF? */
				action->u.sendtext.left -= n;

				if (action->u.sendtext.left <= 0)
				{
qse_printf (QSE_T("finished sending text ...\n"));
					dequeue_client_action_locked (client, NULL);
				}
			}

			break;
		}

		case ACTION_SENDTEXTDUP:
		{
			ssize_t n;
			size_t count;

			count = MAX_SENDFILE_SIZE;
			if (count >= action->u.sendtextdup.left)
				count = action->u.sendtextdup.left;

			n = send (client->fd, action->u.sendtextdup.ptr, count, 0);
			if (n <= -1) 
			{
qse_printf (QSE_T("send text dup failure... arrange to close this connection....\n"));
				dequeue_client_action_locked (client, NULL);
				shutdown (client->fd, SHUT_RDWR);
			}
			else
			{
/* TODO: what if n is 0???? does it mean EOF? */
				action->u.sendtextdup.left -= n;

				if (action->u.sendtextdup.left <= 0)
				{
qse_printf (QSE_T("finished sending text dup...\n"));
					dequeue_client_action_locked (client, NULL);
qse_printf (QSE_T("finished sending text dup dequed...\n"));
				}
			}

			break;
		}

		case ACTION_SENDFILE:
		{
			ssize_t n;
			size_t count;

			count = MAX_SENDFILE_SIZE;
			if (count >= action->u.sendfile.left)
				count = action->u.sendfile.left;

//n = qse_htrd_write (client->http, sendfile, joins);


			n = sendfile (
				client->fd, 
				action->u.sendfile.fd,
				&action->u.sendfile.offset,
				count
			);

			if (n <= -1) 
			{
qse_printf (QSE_T("sendfile failure... arrange to close this connection....\n"));
				dequeue_client_action_locked (client, NULL);
				shutdown (client->fd, SHUT_RDWR);
			}
			else
			{
/* TODO: what if n is 0???? does it mean EOF? */
				action->u.sendfile.left -= n;

				if (action->u.sendfile.left <= 0)
				{
qse_printf (QSE_T("finished sending...\n"));
					dequeue_client_action_locked (client, NULL);
				}
			}

			break;
		}

		case ACTION_DISCONNECT:
		{
			shutdown (client->fd, SHUT_RDWR);
			break;
		}
	}

	return 0;
}

static void* response_thread (void* arg)
{
	qse_httpd_t* httpd = (qse_httpd_t*)arg;

	while (!httpd->stopreq)
	{
		int n, max, fd;
		fd_set w;
		struct timeval tv;

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		pthread_mutex_lock (&httpd->camutex);
		max = make_fd_set_from_client_array (httpd, NULL, &w);
		pthread_mutex_unlock (&httpd->camutex);

		while (max == -1 && !httpd->stopreq)
		{
			struct timeval now;
			struct timespec timeout;

			pthread_mutex_lock (&httpd->camutex);

			gettimeofday (&now, NULL);
			timeout.tv_sec = now.tv_sec + 2;
			timeout.tv_nsec = now.tv_usec * 1000;

			pthread_cond_timedwait (&httpd->ca.cond, &httpd->camutex, &timeout);
			max = make_fd_set_from_client_array (httpd, NULL, &w);

			pthread_mutex_unlock (&httpd->camutex);
		}

		if (httpd->stopreq) break;

		n = select (max + 1, NULL, &w, NULL, &tv);
		if (n <= -1)
		{
			if (errno == EINTR) continue;
			qse_fprintf (QSE_STDERR, QSE_T("Error: select returned failure - %S\n"), strerror(errno));
			/* break; */
			continue;
		}
		if (n == 0) continue;

		for (fd = 0; fd < httpd->ca.capa; fd++)
		{
			client_t* client = &httpd->ca.data[fd];

			if (!client->http) continue;

			if (FD_ISSET(client->fd, &w)) 
			{
				if (client->action.count > 0) take_client_action (client);
			}
		
		}
	}

	pthread_exit (NULL);
	return NULL;
}

int qse_httpd_loop (qse_httpd_t* httpd)
{
	pthread_t response_thread_id;

	httpd->stopreq = 0;


	QSE_ASSERTX (httpd->listener.list != QSE_NULL,
		"Add listeners before calling qse_httpd_loop()"
	);	

	if (httpd->listener.list == QSE_NULL)
	{
		httpd->errnum = QSE_HTTPD_EINVAL;
		goto oops;
	}

	if (activate_listners (httpd) <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Error: failed to make a server socket\n"));
		goto oops;
	}

	/* data receiver main logic */
	init_client_array (&httpd->ca);
	pthread_mutex_init (&httpd->camutex, NULL);

	/* start the response sender as a thread */
	pthread_create (&response_thread_id, NULL, response_thread, httpd);

	while (!httpd->stopreq)
	{

		int n, max, fd;
		fd_set r;
		struct timeval tv;

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		max = make_fd_set_from_client_array (httpd, &r, QSE_NULL);
		n = select (max + 1, &r, NULL, NULL, &tv);
		if (n <= -1)
		{
/*
httpd->errnum = QSE_HTTPD_EMUX;
*/
/* TODO: user callback for an error */

			if (errno == EINTR) continue;
qse_fprintf (QSE_STDERR, QSE_T("Error: select returned failure\n"));
			/* break; */
			continue;
		}
		if (n == 0) continue;

		/* check the listener activity */
		accept_client_from_listeners (httpd, &r);

		/* check the client activity */
		for (fd = 0; fd < httpd->ca.capa; fd++)
		{
			client_t* client = &httpd->ca.data[fd];

			if (!client->http) continue;

			if (FD_ISSET(client->fd, &r)) 
			{
				/* got input */
				if (qse_htrd_read (client->http) <= -1)
				{
					int errnum = client->http->errnum;
					if (errnum != 99999)
					{
						pthread_mutex_lock (&httpd->camutex);
						delete_from_client_array (&httpd->ca, fd);	
						pthread_mutex_unlock (&httpd->camutex);
						if (errnum == QSE_HTRD_EDISCON)
							qse_fprintf (QSE_STDERR, QSE_T("Debug: connection closed %d\n"), client->fd);
						else
							qse_fprintf (QSE_STDERR, QSE_T("Error: failed to read/process a request from a client %d\n"), client->fd);
					}
					continue;
				}
			}
		}
	}

	pthread_join (response_thread_id, NULL);

	fini_client_array (&httpd->ca);
	pthread_mutex_destroy (&httpd->camutex);

	deactivate_listeners (httpd);
	return 0;

oops:
	deactivate_listeners (httpd);
	return -1;
}

static void free_listener (qse_httpd_t* httpd, listener_t* l)
{
	if (l->host) QSE_MMGR_FREE (httpd->mmgr, l->host);
	if (l->handle >= 0) close (l->handle);
	QSE_MMGR_FREE (httpd->mmgr, l);
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

static listener_t* parse_listener_string (
	qse_httpd_t* httpd, const qse_char_t* str, listener_t** ltail)
{
	const qse_char_t* p = str;
	listener_t* lhead = QSE_NULL, * ltmp = QSE_NULL, * lend = QSE_NULL;

	do
	{
		qse_cstr_t tmp;
		qse_mchar_t* host;
		int x;

		/* skip spaces */
		while (QSE_ISSPACE(*p)) p++;

		ltmp = QSE_MMGR_ALLOC (httpd->mmgr, QSE_SIZEOF(*ltmp));
		if (ltmp == QSE_NULL) goto oops_enomem;

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

		if (*p == QSE_T('['))	
		{
			/* IPv6 address */
			p++; /* skip [ */
			tmp.ptr = p;
			while (*p != QSE_T(']')) 
			{
				if (*p == QSE_T('\0')) goto oops_einval;
				p++;
			}
			tmp.len = p - tmp.ptr;

			ltmp->family = AF_INET6;
			p++; /* skip ] */
		}
		else
		{
			/* host name or IPv4 address */
			tmp.ptr = p;
			while (!QSE_ISSPACE(*p) &&
			       *p != QSE_T(':') && 
			       *p != QSE_T('\0')) p++;
			tmp.len = p - tmp.ptr;
			ltmp->family = AF_INET;
		}

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
		QSE_MMGR_FREE (httpd->mmgr, host);
#endif
		if (x != 1)
		{
			/* TODO: need to support host names??? 
			if (getaddrinfo... )....
			 */
			goto oops_einval;
		}

		if (*p == QSE_T(':')) 
		{
			unsigned int port = 0;
			/* port number */
			p++;

			tmp.ptr = p;
			while (QSE_ISDIGIT(*p)) 
			{
				port = port * 10 + (*p - QSE_T('0'));
				p++;
			}
			tmp.len = p - tmp.ptr;
			if (tmp.len > 5 || port > 65535) goto oops_einval;
			ltmp->port = port;
		}

		/* skip spaces */
		while (QSE_ISSPACE(*p)) p++;

		if (lhead == QSE_NULL) lend = ltmp;
		ltmp->next = lhead;
		lhead = ltmp;
		ltmp = QSE_NULL;
	} 
	while (*p != QSE_T('\0'));

	if (ltail) *ltail = lend;
	return lhead;

oops_einval:
	httpd->errnum = QSE_HTTPD_EINVAL;
	goto oops;

oops_enomem:
	httpd->errnum = QSE_HTTPD_ENOMEM;
	goto oops;

oops:
	if (ltmp) free_listener (httpd, ltmp);
	if (lhead) free_listener_list (httpd, lhead);
	return QSE_NULL;
}

static int add_listeners (qse_httpd_t* httpd, const qse_char_t* uri)
{
	listener_t* lh, * lt;

	lh = parse_listener_string (httpd, uri, &lt);
	if (lh == QSE_NULL) return -1;

	lt->next = httpd->listener.list;
	httpd->listener.list = lh;
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

	lh = parse_listener_string (httpd, uri, QSE_NULL);
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

int qse_httpd_addlisteners (qse_httpd_t* httpd, const qse_char_t* uri)
{
	int n;
	pthread_mutex_lock (&httpd->listener.mutex);
	n = add_listeners (httpd, uri);
	pthread_mutex_unlock (&httpd->listener.mutex);
	return n;
}

#if 0
int qse_httpd_dellisteners (qse_httpd_t* httpd, const qse_char_t* uri)
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
	httpd->listener.list = NULL;
	pthread_mutex_unlock (&httpd->listener.mutex);
}
#endif
