#include <qse/net/htrd.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/cmn/stdio.h>

#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>

#include <sys/sendfile.h>
#include <sys/stat.h>

#define MAX_SENDFILE_SIZE 4096
//#define MAX_SENDFILE_SIZE 64

typedef struct client_t client_t;
typedef struct client_array_t client_array_t;

typedef struct client_action_t client_action_t;
struct client_action_t
{
	enum
	{
		ACTION_SENDTEXT,
		ACTION_SENDTEXTDUP,
		ACTION_SENDFILE,
		ACTION_DISCONNECT
	} type;

	union
	{
		struct
		{
			const qse_htoc_t* ptr;
			qse_size_t        left;
		} sendtext;
		struct
		{
			qse_htoc_t* ptr;
			qse_size_t  left;
		} sendtextdup;
		struct
		{
			int   fd;
			off_t left;
			off_t offset;
		} sendfile;
	} u;
};

struct client_t
{
	int                     fd;
	struct sockaddr_storage addr;
	qse_htrd_t*             http;

	pthread_mutex_t action_mutex;
	struct
	{
		int             offset;
		int             count;
		client_action_t target[32];
	} action;
};

struct client_array_t
{
	int       capa;
	int       size;
	client_t* data;
	pthread_cond_t cond;
};

typedef struct appdata_t appdata_t;

struct appdata_t
{
	client_array_t ca;
	pthread_mutex_t camutex;
	pthread_cond_t cacond;
};

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

static int capture_param (void* ctx, const qse_mcstr_t* key, const qse_mcstr_t* val)
{
qse_printf (QSE_T("PARAM [%.*S] => [%.*S] \n"), (int)key->len, key->ptr, (int)val->len, val->ptr);
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
if (qse_htre_getqparamstrlen(req) > 0)
{
qse_printf (QSE_T("PARAMS ==> [%S]\n"), qse_htre_getqparamstrptr(req));
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
		qse_mchar_t* paramstr;

qse_printf (QSE_T("BEGIN SCANNING PARAM STR\n"));

		paramstr = qse_htre_getqparamstrptr(req); /* if it is null, ? wasn't even provided */
		if (paramstr != QSE_NULL && qse_scanhttpparamstr (paramstr, capture_param, client) <= -1)
		{
qse_printf (QSE_T("END SCANNING PARAM STR WITH ERROR\n"));
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

qse_printf (QSE_T("END SCANNING PARAM STR WITH SUCCESS\n"));

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
	"HTTP/%d.%d 200 OK\r\nContent-Length: %llu\r\n\r\n", 
	qse_htre_getmajorversion(req),
	qse_htre_getminorversion(req),
	(unsigned long long)st.st_size);

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
	handle_expect_continue
};

int mkserver (const char* portstr)
{
	int s, flag, port = atoi(portstr);
	struct sockaddr_in6 addr;
	
	s = socket (PF_INET6, SOCK_STREAM, IPPROTO_TCP);
	if (s <= -1) return -1;

	flag = 1;
	setsockopt (s, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

	memset (&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(port);
	//inet_pton (AF_INET6, "::1", &addr.sin6_addr);

	if (bind (s, (struct sockaddr*)&addr, sizeof(addr)) <= -1)
	{
		close (s);
		return -1;
	}

	if (listen (s, 10) <= -1)
	{
		close (s);
		return -1;
	}

	flag = fcntl (s, F_GETFL);
	if (flag >= 0) fcntl (s, F_SETFL, flag | O_NONBLOCK);
		
	return s;
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
	client_array_t* array, int fd, struct sockaddr_storage* addr)
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

static int make_fd_set_from_client_array (
	client_array_t* ca, int s, fd_set* r, fd_set* w)
{
	int fd, max = s;

	if (r)
	{
		FD_ZERO (r);
		FD_SET (s, r);
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

static int quit = 0;

static void sigint (int sig)
{
	quit = 1;
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
	appdata_t* appdata = (appdata_t*)arg;

	while (!quit)
	{
		int n, max, fd;
		fd_set w;
		struct timeval tv;

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		pthread_mutex_lock (&appdata->camutex);
		max = make_fd_set_from_client_array (&appdata->ca, -1, NULL, &w);
		pthread_mutex_unlock (&appdata->camutex);

		while (max == -1 && !quit)
		{
			struct timeval now;
			struct timespec timeout;

			pthread_mutex_lock (&appdata->camutex);

			gettimeofday (&now, NULL);
			timeout.tv_sec = now.tv_sec + 2;
			timeout.tv_nsec = now.tv_usec * 1000;

			pthread_cond_timedwait (&appdata->ca.cond, &appdata->camutex, &timeout);
			max = make_fd_set_from_client_array (&appdata->ca, -1, NULL, &w);

			pthread_mutex_unlock (&appdata->camutex);
		}

		if (quit) break;

		n = select (max + 1, NULL, &w, NULL, &tv);
		if (n <= -1)
		{
			if (errno == EINTR) continue;
			qse_fprintf (QSE_STDERR, QSE_T("Error: select returned failure - %S\n"), strerror(errno));
			/* break; */
			continue;
		}
		if (n == 0) continue;

		for (fd = 0; fd < appdata->ca.capa; fd++)
		{
			client_t* client = &appdata->ca.data[fd];

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

int main (int argc, char* argv[])
{
	int s;
	pthread_t response_thread_id;
	appdata_t appdata;

	if (argc != 2)
	{
	#ifdef QSE_CHAR_IS_MCHAR
		qse_fprintf (QSE_STDERR, QSE_T("Usage: %s <port>\n"), argv[0]);
	#else
		qse_fprintf (QSE_STDERR, QSE_T("Usage: %S <port>\n"), argv[0]);
	#endif
		return -1;
	}

	signal (SIGINT, sigint);
	signal (SIGPIPE, SIG_IGN);

	if ((s = mkserver (argv[1])) <= -1) 
	{
		qse_fprintf (QSE_STDERR, QSE_T("Error: failed to make a server socket\n"));
		return -1;
	}

	/* data receiver main logic */
	init_client_array (&appdata.ca);
	pthread_mutex_init (&appdata.camutex, NULL);

	/* start the response sender as a thread */
	pthread_create (&response_thread_id, NULL, response_thread, &appdata);

	while (!quit)
	{
		int n, max, fd;
		fd_set r;
		struct timeval tv;

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		max = make_fd_set_from_client_array (&appdata.ca, s, &r, NULL);
		n = select (max + 1, &r, NULL, NULL, &tv);
		if (n <= -1)
		{
			if (errno == EINTR) continue;
			qse_fprintf (QSE_STDERR, QSE_T("Error: select returned failure\n"));
			/* break; */
			continue;
		}
		if (n == 0) continue;

		if (FD_ISSET(s, &r))
		{
			int flag, c;
			struct sockaddr_storage addr;
			socklen_t addrlen = sizeof(addr);
			client_t* client;

			c = accept (s, (struct sockaddr*)&addr, &addrlen);
			if (c <= -1)
			{
				if (errno != EINTR) 
					qse_fprintf (QSE_STDERR, QSE_T("Error: accept returned failure\n"));
				continue;
			}

			/* select() uses a fixed-size array so the file descriptor can not
			 * exceeded FD_SETSIZE */
			if (c >= FD_SETSIZE)
			{
				close (c);
				qse_fprintf (QSE_STDERR, QSE_T("Error: socket descriptor too high. probably too many clients\n"));
				continue;
			}

			/* set the nonblock flag in case read() after select() blocks
			 * for various reasons - data received may be dropped after 
			 * arrival for wrong checksum, for example. */
			flag = fcntl (c, F_GETFL);
			if (flag >= 0) fcntl (c, F_SETFL, flag | O_NONBLOCK);

			pthread_mutex_lock (&appdata.camutex);
			client = insert_into_client_array (&appdata.ca, c, &addr);
			pthread_mutex_unlock (&appdata.camutex);
			if (client == QSE_NULL)
			{
				close (c);
				qse_fprintf (QSE_STDERR, QSE_T("Error: failed to add a client\n"));
				continue;
			}
qse_printf (QSE_T("connection %d accepted\n"), c);
		}

		for (fd = 0; fd < appdata.ca.capa; fd++)
		{
			client_t* client = &appdata.ca.data[fd];

			if (!client->http) continue;

			if (FD_ISSET(client->fd, &r)) 
			{
				/* got input */
				if (qse_htrd_read (client->http) <= -1)
				{
					int errnum = client->http->errnum;
					if (errnum != 99999)
					{
						pthread_mutex_lock (&appdata.camutex);
						delete_from_client_array (&appdata.ca, fd);	
						pthread_mutex_unlock (&appdata.camutex);
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

	fini_client_array (&appdata.ca);
	pthread_mutex_destroy (&appdata.camutex);

	close (s);
	return 0;
}
