
#include <qse/net/httpd.h>
#include <qse/cmn/stdio.h>
#include <qse/cmn/main.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>

#define MAX_SENDFILE_SIZE 4096
typedef struct httpd_xtn_t httpd_xtn_t;
struct httpd_xtn_t
{
	const qse_httpd_cbs_t* orgcbs;
};

typedef struct httpd_task_sendfile_t httpd_task_sendfile_t;
struct httpd_task_sendfile_t
{
	int fd;
	qse_foff_t left;
	qse_foff_t offset;
};

typedef struct httpd_task_sendtext_t httpd_task_sendtext_t;
struct httpd_task_sendtext_t
{
	const qse_mchar_t* ptr;
	qse_size_t left;
};

static int httpd_init_sendfile (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	httpd_task_sendfile_t* xtn = qse_httpd_gettaskxtn (httpd, task);
	memcpy (xtn, task->ctx, QSE_SIZEOF(*xtn));
	task->ctx = xtn;
	return 0;
}

static void httpd_fini_sendfile (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	httpd_task_sendfile_t* ctx = (httpd_task_sendfile_t*)task->ctx;
	close (ctx->fd);
}

static int httpd_main_sendfile (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	ssize_t n;
	size_t count;
	httpd_task_sendfile_t* ctx = (httpd_task_sendfile_t*)task->ctx;

	count = MAX_SENDFILE_SIZE;
	if (count >= ctx->left) count = ctx->left;

	n = sendfile (
	/*	TODO: client->fd, */ *(int*)client,
		ctx->fd,
		&ctx->offset,
		count
	);

	if (n <= -1) return -1;

	ctx->left -= n;
	if (ctx->left <= 0) return 0;

	return 1; /* more work to do */
}

static int entask_sendfile (
	qse_httpd_t* httpd, qse_httpd_client_t* client, int fd, qse_foff_t size)
{
	qse_httpd_task_t task;
	httpd_task_sendfile_t data;
	
	memset (&data, 0, QSE_SIZEOF(data));
	data.fd = fd;
	data.left = size;

	memset (&task, 0, QSE_SIZEOF(task));
	task.init = httpd_init_sendfile;
	task.main = httpd_main_sendfile;
	task.fini = httpd_fini_sendfile;
	task.ctx = &data;

	return qse_httpd_entask (httpd, client, &task, QSE_SIZEOF(data));
}


static qse_htb_walk_t walk (qse_htb_t* htb, qse_htb_pair_t* pair, void* ctx)
{
qse_printf (QSE_T("HEADER OK %d[%S] %d[%S]\n"),  (int)QSE_HTB_KLEN(pair), QSE_HTB_KPTR(pair), (int)QSE_HTB_VLEN(pair), QSE_HTB_VPTR(pair));
	return QSE_HTB_WALK_FORWARD;
}

static int handle_request (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_htre_t* req)
{
	int method;

#if 0
	httpd_xtn_t* xtn = (httpd_xtn_t*) qse_httpd_getxtn (httpd);
	return xtn->orgcbs->handle_request (httpd, client, req);
#endif

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

qse_htb_walk (&req->hdrtab, walk, QSE_NULL);
if (QSE_MBS_LEN(&req->content) > 0)
{
qse_printf (QSE_T("content = [%.*S]\n"),
          (int)QSE_MBS_LEN(&req->content),
          QSE_MBS_PTR(&req->content));
}


	method = qse_htre_getqmethod (req);

	if (method == QSE_HTTP_GET || method == QSE_HTTP_POST)
	{
		int fd;

		fd = open (qse_htre_getqpathptr(req), O_RDONLY);
		if (fd <= -1)
		{
			return -1;
		}
		else
		{
			struct stat st;
			if (fstat (fd, &st) <= -1)
			{
				close (fd);
				return -1;
			}
			else if (st.st_size <= 0)
			{
				close (fd);
				return -1;
			}
			else
			{
				int n;
#if 0
qse_mchar_t text[128];
snprintf (text, sizeof(text),
     "HTTP/%d.%d 200 OK\r\nContent-Length: %llu\r\nContent-Location: %s\r\n\r\n", 
     qse_htre_getmajorversion(req),
     qse_htre_getminorversion(req),
     (unsigned long long)st.st_size,
     qse_htre_getqpathptr(req)
);
				n = qse_httpd_entasksendfmt (httpd, client, text);
#endif
				n = qse_httpd_entasksendfmt (httpd, client,
     				"HTTP/%d.%d 200 OK\r\nContent-Length: %llu\r\nContent-Location: %s\r\n\r\n", 
					qse_htre_getmajorversion(req),
					qse_htre_getminorversion(req),
					(unsigned long long)st.st_size,
					qse_htre_getqpathptr(req)
				);
				if (n <= -1) return -1;

				if (entask_sendfile (httpd, client, fd, st.st_size) <= -1) return -1;
			}

		}
	}


	if (req->attr.connection_close)
	{
		if (qse_httpd_entaskdisconnect (httpd, client) <= -1) return -1;
	}

	return 0;
}

static int handle_expect_continue (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_htre_t* req)
{
	httpd_xtn_t* xtn = (httpd_xtn_t*) qse_httpd_getxtn (httpd);
	return xtn->orgcbs->handle_expect_continue (httpd, client, req);
}

static qse_httpd_t* httpd = NULL;

static void sigint (int sig)
{
	qse_httpd_stop (httpd);
}

static qse_httpd_cbs_t httpd_cbs =
{
	handle_request,
	handle_expect_continue
};

int httpd_main (int argc, qse_char_t* argv[])
{
	int n;
	httpd_xtn_t* xtn;

	if (argc != 2)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Usage: %s <listener_uri>\n"), argv[0]);
		return -1;
	}


	httpd = qse_httpd_open (QSE_NULL, QSE_SIZEOF(httpd_xtn_t));
	if (httpd == QSE_NULL)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Cannot open httpd\n"));
		return -1;
	}

	xtn = (httpd_xtn_t*)qse_httpd_getxtn (httpd);
	xtn->orgcbs = qse_httpd_getcbs (httpd);

	if (qse_httpd_addlisteners (httpd, argv[1]) <= -1)
	{
		qse_fprintf (QSE_STDERR, QSE_T("Failed to add httpd listeners\n"));
		qse_httpd_close (httpd);
		return -1;
	}

	qse_httpd_setcbs (httpd, &httpd_cbs);

	signal (SIGINT, sigint);
	signal (SIGPIPE, SIG_IGN);

	n = qse_httpd_loop (httpd);

	signal (SIGINT, SIG_DFL);
	signal (SIGPIPE, SIG_DFL);

	qse_httpd_close (httpd);
	return n;
}

int qse_main (int argc, qse_achar_t* argv[])
{
	return qse_runmain (argc, argv, httpd_main);
}

