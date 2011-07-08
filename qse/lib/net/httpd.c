#include <qse/net/httpd.h>
#include <qse/net/htrd.h>

#include "../cmn/mem.h"
#include <pthread.h>

QSE_IMPLEMENT_COMMON_FUNCTIONS (httpd)


typedef struct client_t client_t;

struct client_t
{
	int                     fd;
#if 0
	struct sockaddr_storage addr;
	qse_htrd_t*             htrd;

	pthread_mutex_t action_mutex;
	struct
	{
		int             offset;
		int             count;
		client_action_t target[32];
	} action;
#endif
};

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
	return httpd;
}

void qse_httpd_fini (qse_httpd_t* httpd)
{
}

int qse_httpd_exec (qse_httpd_t* httpd)
{
#if 0
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
#endif
	return 0;
}

int qse_httpd_loop (qse_httpd_t* httpd)
{
	httpd->stopreq = 0;

	while (!httpd->stopreq)
	{
		if (qse_httpd_exec (httpd) <= -1) return -1;
	}

	return 0;
}

void qse_httpd_stop (qse_httpd_t* httpd)
{
	httpd->stopreq = 1;	
}
