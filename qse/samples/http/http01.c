#include <qse/http/http.h>
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
		ACTION_SENDFILE
	} type;

	union
	{
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
	qse_http_t*             http;

	pthread_mutex_t action_mutex;
	struct
	{
		int             offset;
		int             count;
		client_action_t target[10];
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
	if (client->action.count <= 0) return -1;

	if (action) *action = client->action.target[client->action.offset];
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
	while (dequeue_client_action_unlocked (client, &action) == 0)
	{
		if (action.type == ACTION_SENDFILE) close (action.u.sendfile.fd);
	}
	pthread_mutex_unlock (&client->action_mutex);
}

static int handle_request (qse_http_t* http, qse_http_req_t* req)
{
	http_xtn_t* xtn = (http_xtn_t*) qse_http_getxtn (http);
	qse_printf (QSE_T("got a request... %S\n"), req->path.ptr);

	if (req->method == QSE_HTTP_REQ_GET)
	{
		int fd = open (req->path.ptr, O_RDONLY);
		if (fd <= -1)
		{
qse_printf (QSE_T("open failure....\n"));
			/*
			qse_http_addtext (http, 
				"<html><title>FILE NOT FOUND</title><body></body></html>");
			*/
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
				qse_http_req_t* rep = qse_http_newreply (http);
				if (req == QSE_NULL)
				{
					/* hard failure... can't answer */
					/* arrange to close connection */
				}


				ptr = qse_http_emitreply (http, rep, &len);
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
				client_t* client = &xtn->array->data[xtn->index];
				client_action_t action;

				memset (&action, 0, sizeof(action));
				action.type = ACTION_SENDFILE;
				action.u.sendfile.fd = fd;
				action.u.sendfile.left = st.st_size;;

				if (enqueue_client_action_locked (client, &action) <= -1)
				{
	/* TODO: close??? send error page ...*/
qse_printf (QSE_T("failed to push action....\n"));
				}
			}
		}	
	}

	pthread_cond_signal (&xtn->array->cond);
	return 0;
}

qse_http_reqcbs_t http_reqcbs =
{
	handle_request
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
	inet_pton (AF_INET6, "::1", &addr.sin6_addr);

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

		qse_http_close (array->data[fd].http);
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
	array->data[fd].http = qse_http_open (QSE_MMGR_GETDFL(), QSE_SIZEOF(*xtn));
	if (array->data[fd].http == QSE_NULL) return QSE_NULL;
	pthread_mutex_init (&array->data[fd].action_mutex, NULL);

	xtn = (http_xtn_t*)qse_http_getxtn (array->data[fd].http);	
	xtn->array = array;
	xtn->index = fd; 

	qse_http_setreqcbs (array->data[fd].http, &http_reqcbs);
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
				/* add it to the set if it has a reply to send */
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
			break;
		}

		case ACTION_SENDFILE:
		{
			ssize_t n;
			size_t count;

			count = MAX_SENDFILE_SIZE;
			if (count >= action->u.sendfile.left)
				count = action->u.sendfile.left;

			n = sendfile (
				client->fd, 
				action->u.sendfile.fd,
				&action->u.sendfile.offset,
				count
			);

			if (n <= -1) 
			{
qse_printf (QSE_T("sendfile failure... arrange to close this connection....\n"));
				close (action->u.sendfile.fd);
				dequeue_client_action_locked (client, NULL);
			}
			else
			{
				action->u.sendfile.left -= n;

				if (action->u.sendfile.left <= 0)
				{
qse_printf (QSE_T("finished sending...\n"));
					close (action->u.sendfile.fd);
					dequeue_client_action_locked (client, NULL);
				}
			}

			break;
		}
	}

	return 0;
}

static void* replier_thread (void* arg)
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
			qse_fprintf (QSE_STDERR, QSE_T("Error: select returned failure\n"));
			break;
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
	pthread_t replier_thread_id;
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

	/* start the reply sender as a thread */
	pthread_create (&replier_thread_id, NULL, replier_thread, &appdata);

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
			break;
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

				qse_byte_t buf[1024];
				ssize_t m;

			reread:
				m = read (client->fd, buf, sizeof(buf));
				if (m <= -1)
				{
					if (errno != EINTR)
					{
						pthread_mutex_lock (&appdata.camutex);
						delete_from_client_array (&appdata.ca, fd);	
						pthread_mutex_unlock (&appdata.camutex);
						qse_fprintf (QSE_STDERR, QSE_T("Error: failed to read from a client %d\n"), fd);
					}
					goto reread;
				}
				else if (m == 0)
				{
					pthread_mutex_lock (&appdata.camutex);
					delete_from_client_array (&appdata.ca, fd);	
					pthread_mutex_unlock (&appdata.camutex);
					qse_fprintf (QSE_STDERR, QSE_T("Debug: connection closed %d\n"), fd);
					continue;
				}

				/* feed may have called the request callback multiple times... 
				 * that's because we don't know how many valid requests
				 * are included in 'buf' */ 
				n = qse_http_feed (client->http, buf, m);
				if (n <= -1)
				{
					if (client->http->errnum == QSE_HTTP_EBADREQ)
					{
						/* TODO: write a reply to indicate bad request... */	
					}

					pthread_mutex_lock (&appdata.camutex);
					delete_from_client_array (&appdata.ca, fd);	
					pthread_mutex_unlock (&appdata.camutex);
					qse_fprintf (QSE_STDERR, QSE_T("Error: http error while processing \n"));
					continue;
				}
			}
		}
	}

	pthread_join (replier_thread_id, NULL);

	fini_client_array (&appdata.ca);
	pthread_mutex_destroy (&appdata.camutex);

	close (s);
	return 0;
}
