#include <qse/utl/http.h>
#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/cmn/stdio.h>

#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <sys/sendfile.h>
#include <sys/stat.h>

typedef struct client_t client_t;
typedef struct client_array_t client_array_t;

struct client_t
{
	int                fd;
	struct sockaddr_in addr;
	qse_http_t*        http;

	pending_requests???
	action_t           actions[10];
};

struct client_array_t
{
	int capa;
	int size;
	client_t* data;
};

typedef struct http_xtn_t http_xtn_t;

struct http_xtn_t
{
	client_array_t* array;
	qse_size_t      index; 
};

int handle_request (qse_http_t* http, qse_http_req_t* req)
{
	http_xtn_t* xtn = (http_xtn_t*) qse_http_getxtn (http);
	qse_printf (QSE_T("got a request... %S\n"), req->path.ptr);

	if (req->method == QSE_HTTP_REQ_GET)
	{
	#if 0
		/* determine what to do with the request 
		 * and set the right action for it */
		xtn->action.type = SENDFILE;
		xtn->action.data.sendfile.path = filename;
		xtn->action.data.sendfile.ifnewer = xxxx;
	#endif

	#if 0
		int fd = open (req->path.ptr, O_RDONLY);
		if (fd <= -1)
		{
		}
		else
		{
			struct stat st;

			fstat (fd, &st);
			sendfile (xtn->array->data[xtn->index].fd, fd, NULL, st.st_size);
			close (fd);
		}	
	#endif
	}

	return 0;
}

#if 0
int handle_action (qse_http_t* http, qse_http_req_t* req)
{
	http_xtn_t* xtn = (http_xtn_t*) qse_http_getxtn (http);

	switch (xtn->action.type)
	{
		case SENDFILE:
		{
			off_t offset = 0;	

			fstat (fd, &st);
			sendfile (
				xtn->array->data[xtn->index].fd,  /* socket */
				fd, /* input file descriptor */
				&offset, 
				st.st_size - offset
			);

			if (offset >= st.st_size)
			{
				/* done */
				xtn->action.type = NONE;
			}
		}
	}

	return 0;
}
#endif

qse_http_reqcbs_t http_reqcbs =
{
	handle_request
};

int mkserver (const char* portstr)
{
	int s, flag, port = atoi(portstr);
	struct sockaddr_in addr;
	
	s = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s <= -1) return -1;

	flag = 1;
	setsockopt (s, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

	memset (&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl (INADDR_ANY);
	addr.sin_port = htons (port);

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
}

static void delete_from_client_array (client_array_t* array, int fd)
{
	if (array->data[fd].http)
	{
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
}

static client_t* insert_into_client_array (
	client_array_t* array, int fd, struct sockaddr_in* addr)
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

	xtn = (http_xtn_t*)qse_http_getxtn (array->data[fd].http);	
	xtn->array = array;
	xtn->index = fd; 

	qse_http_setreqcbs (array->data[fd].http, &http_reqcbs);
	array->size++;
	return &array->data[fd];
}

static int make_fd_set_from_client_array (client_array_t* ca, int s, fd_set* r, fd_set* w)
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
			if (r) FD_SET (ca->data[fd].fd, r);
			if (w) FD_SET (ca->data[fd].fd, w);
			if (ca->data[fd].fd > max) max = ca->data[fd].fd;
		}
	}

	return max;
}

static int quit = 0;

void sigint (int sig)
{
	quit = 1;
}

int main (int argc, char* argv[])
{
	int s;
	client_array_t ca;

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

	init_client_array (&ca);

	while (!quit)
	{
		int n, max, fd;
		fd_set r, w;
		struct timeval tv;

	#if 0
		if (ca->need_cpu)
		{
			tv.tv_sec = 0;
			tv.tv_usec = 0;
		}
		else
		{
	#endif
			tv.tv_sec = 1;
			tv.tv_usec = 0;
	#if 0
		}
	#endif

		max = make_fd_set_from_client_array (&ca, s, &r, &w);
		n = select (max + 1, &r, &w, NULL, &tv);
		if (n <= -1)
		{
			if (errno == EINTR) continue;

			qse_fprintf (QSE_STDERR, QSE_T("Error: select returned failure\n"));
			break;
		}
		if (n == 0) 
		{
	#if 0
			/* no input and no writable sockets.
			 */
			for (fd = 0; fd < ca.capa; fd++)
			{
				/* process internal things */
				qse_http_pump (ca.data[fd].http);
			}
	#endif
			continue;
		}

		if (FD_ISSET(s, &r))
		{
			int flag;
			struct sockaddr_in addr;
			socklen_t addrlen = sizeof(addr);
			int c = accept (s, (struct sockaddr*)&addr, &addrlen);
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

			if (insert_into_client_array (&ca, c, &addr) == QSE_NULL)
			{
				close (c);
				qse_fprintf (QSE_STDERR, QSE_T("Error: failed to add a client\n"));
				continue;
			}
		}

		for (fd = 0; fd < ca.capa; fd++)
		{
			client_t* client = &ca.data[fd];

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
						delete_from_client_array (&ca, fd);	
						qse_fprintf (QSE_STDERR, QSE_T("Error: failed to read from a client %d\n"), fd);
					}
					goto reread;
				}
				else if (m == 0)
				{
					delete_from_client_array (&ca, fd);	
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

					delete_from_client_array (&ca, fd);	
					qse_fprintf (QSE_STDERR, QSE_T("Error: http error while processing \n"));
					continue;
				}
			}

		#if 0
			if (FD_ISSET(client->fd, &w) && client->has_data_to_send)
			{
				/* ready to send output */
				if (handle_output (xxxxx) <= -1)
				{
					delete_from_client_array (&ca, fd);	
					qse_fprintf (QSE_STDERR, QSE_T("Error: output error\n"));
				}
			}
		#endif
		}
	}

	fini_client_array (&ca);

	close (s);
	return 0;
}
