#include "httpd.h"
#include "../cmn/mem.h"
#include <qse/cmn/str.h>
#include <qse/cmn/pio.h>

#include <fcntl.h>
#include <unistd.h>

#ifdef HAVE_SYS_SENDFILE_H
#include <sys/sendfile.h>
#endif

#include <stdarg.h>
#include <stdio.h>

#define MAX_SENDFILE_SIZE 4096

/*------------------------------------------------------------------------*/

static int task_main_disconnect (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	shutdown (client->fd, SHUT_RDWR);
	return 0;
}

int qse_httpd_entaskdisconnect (qse_httpd_t* httpd, qse_httpd_client_t* client)
{
	qse_httpd_task_t task;
	
	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.main = task_main_disconnect;

	return qse_httpd_entask (httpd, client, &task, 0);
}

/*------------------------------------------------------------------------*/

typedef struct task_sendtext_t task_sendtext_t;
struct task_sendtext_t
{
	const qse_mchar_t* ptr;
	qse_size_t         left;
};

static int httpd_init_sendtext (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_sendtext_t* xtn = qse_httpd_gettaskxtn (httpd, task);

	QSE_MEMCPY (xtn, task->ctx, QSE_SIZEOF(*xtn));
	QSE_MEMCPY (xtn + 1, xtn->ptr, xtn->left);
	xtn->ptr = (qse_mchar_t*)(xtn + 1);

	task->ctx = xtn;
	return 0;
}

static int httpd_main_sendtext (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	ssize_t n;
	size_t count;
	task_sendtext_t* ctx = (task_sendtext_t*)task->ctx;

	count = MAX_SENDFILE_SIZE;
	if (count >= ctx->left) count = ctx->left;

	n = send (
		client->fd,
		ctx->ptr,
		count,
		0
	);

	if (n <= -1) return -1;

	ctx->left -= n;
	if (ctx->left <= 0) return 0;

	ctx->ptr += n;
	return 1; /* more work to do */
}

int qse_httpd_entasksendtext (
     qse_httpd_t* httpd, qse_httpd_client_t* client, const qse_mchar_t* text)
{
	qse_httpd_task_t task;
	task_sendtext_t data;

	QSE_MEMSET (&data, 0, QSE_SIZEOF(data));
	data.ptr = text;
	data.left = qse_mbslen(text);

	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = httpd_init_sendtext;
	task.main = httpd_main_sendtext;
	task.ctx = &data;

	return qse_httpd_entask (
		httpd, client, &task, QSE_SIZEOF(data) + data.left);
}

/*------------------------------------------------------------------------*/

/* TODO: send wide character string when QSE_CHAR_IS_WCHAR */

/*------------------------------------------------------------------------*/

typedef struct task_sendfmt_t task_sendfmt_t;
struct task_sendfmt_t
{
	qse_mchar_t*       org;
	const qse_mchar_t* ptr;
	qse_size_t         left;
};

static int httpd_init_sendfmt (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_sendfmt_t* xtn = qse_httpd_gettaskxtn (httpd, task);

	QSE_MEMCPY (xtn, task->ctx, QSE_SIZEOF(*xtn));
	task->ctx = xtn;
	return 0;
}

static void httpd_fini_sendfmt (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_sendfmt_t* ctx = (task_sendfmt_t*)task->ctx;
	qse_httpd_freemem (httpd, ctx->org);
}

static int httpd_main_sendfmt (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	ssize_t n;
	size_t count;
	task_sendfmt_t* ctx = (task_sendfmt_t*)task->ctx;

	count = MAX_SENDFILE_SIZE;
	if (count >= ctx->left) count = ctx->left;

	n = send (
		client->fd,
		ctx->ptr,
		count,
		0
	);

	if (n <= -1) return -1;

	ctx->left -= n;
	if (ctx->left <= 0) return 0;

	ctx->ptr += n;
	return 1; /* more work to do */
}

int qse_httpd_entasksendfmt (
     qse_httpd_t* httpd, qse_httpd_client_t* client, const qse_mchar_t* fmt, ...)
{
	qse_httpd_task_t task;
	task_sendfmt_t data;

	va_list ap;
	qse_mchar_t n[2];
	qse_mchar_t* buf;
	int bytes_req, l;

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

		buf = (qse_mchar_t*) qse_httpd_allocmem (httpd, (capa + 1) * QSE_SIZEOF(*buf));
		if (buf == QSE_NULL) return -1;

		/* an old vsnprintf behaves differently from C99 standard.
		 * thus, it returns -1 when it can't write all the input given. */
		for (;;) 
		{
			va_start (ap, fmt);
#if defined(_WIN32) && defined(_MSC_VER)
			l = _vsnprintf (buf, capa + 1, fmt, ap);
#else
			l = vsnprintf (buf, capa + 1, fmt, ap);
#endif
			va_end (ap);

			if (l == -1)
			{
				qse_httpd_freemem (httpd, buf);

				capa = capa * 2;
				buf = (qse_mchar_t*) qse_httpd_allocmem (httpd, (capa + 1) * QSE_SIZEOF(*buf));
				if (buf == QSE_NULL) return  -1;
			}
			else break;
		}
	}
	else 
	{
		/* vsnprintf returns the number of characters that would 
		 * have been written not including the terminating '\0' 
		 * if the _data buffer were large enough */
		buf = (qse_mchar_t*) qse_httpd_allocmem (httpd, (bytes_req + 1) * QSE_SIZEOF(*buf));
		if (buf == QSE_NULL) return -1;

		va_start (ap, fmt);
#if defined(_WIN32) && defined(_MSC_VER)
		l = _vsnprintf (buf, bytes_req + 1, fmt, ap);
#else
		l = vsnprintf (buf, bytes_req + 1, fmt, ap);
#endif
		va_end (ap);

		if (l != bytes_req) 
		{
			/* something got wrong ... */
			qse_httpd_freemem (httpd, buf);

			httpd->errnum = QSE_HTTPD_EINTERN;
			return -1;
		}
	}

	QSE_MEMSET (&data, 0, QSE_SIZEOF(data));
	data.org = buf;
	data.ptr = buf;
	data.left = l;

	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = httpd_init_sendfmt;
	task.fini = httpd_fini_sendfmt;
	task.main = httpd_main_sendfmt;
	task.ctx = &data;

	return qse_httpd_entask (
		httpd, client, &task, QSE_SIZEOF(data));
}

/* TODO: send wide character string when QSE_CHAR_IS_WCHAR */

/*------------------------------------------------------------------------*/

typedef struct httpd_task_sendfile_t httpd_task_sendfile_t;
struct httpd_task_sendfile_t
{
	int fd;
	qse_foff_t left;
	qse_foff_t offset;
};

static int httpd_init_sendfile (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	httpd_task_sendfile_t* xtn = qse_httpd_gettaskxtn (httpd, task);
	QSE_MEMCPY (xtn, task->ctx, QSE_SIZEOF(*xtn));
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
		client->fd,
		ctx->fd,
		&ctx->offset,
		count
	);

	if (n <= -1) return -1;

	ctx->left -= n;
	if (ctx->left <= 0) return 0;

	return 1; /* more work to do */
}

int qse_httpd_entasksendfile (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	int fd, qse_foff_t offset, qse_foff_t size)
{
	qse_httpd_task_t task;
	httpd_task_sendfile_t data;
	
	QSE_MEMSET (&data, 0, QSE_SIZEOF(data));
	data.fd = fd;
	data.offset = offset;
	data.left = size;

	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = httpd_init_sendfile;
	task.main = httpd_main_sendfile;
	task.fini = httpd_fini_sendfile;
	task.ctx = &data;

	return qse_httpd_entask (httpd, client, &task, QSE_SIZEOF(data));
}

/*------------------------------------------------------------------------*/

#if 0
typedef struct httpd_task_cgi_t httpd_task_cgi_t;
struct httpd_task_cgi_t
{
	qse_pio_t* pio;
};

static int httpd_init_cgi (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	httpd_task_cgi_t* xtn = qse_httpd_gettaskxtn (httpd, task);

	QSE_MEMSET (xtn, 0, QSE_SIZEOF(*xtn));
	xtn->pio = qse_pio_open (httpd->mmgr, task->ctx);
	if (xtn->pio == QSE_NULL)
	{
		httpd->errnum = QSE_HTTPD_ECGI;
		return -1;
	}

	task->ctx = xtn;
	return 0;
}

static void httpd_fini_cgi (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	httpd_task_cgi_t* xtn = task->ctx;
	qse_pio_close (xtn->pio);
}

static void httpd_main_cgi (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
/* TODO */
	return -1;
}

int qse_httpd_entaskcgi (
	qse_httpd_t* httpd, qse_httpd_client_t* client, const qse_char_t* path)
{
	qse_httpd_task_t task;
	httpd_task_cgi_t data;
	
	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = httpd_init_cgi;
	task.main = httpd_main_cgi;
	task.fini = httpd_fini_cgi;
	task.ctx = path;

	return qse_httpd_entask (httpd, client, &task, QSE_SIZEOF(data));
}
#endif
