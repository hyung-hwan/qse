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
    License along with QSE. If not, see <htrd://www.gnu.org/licenses/>.
 */

#include "httpd.h"
#include "../cmn/mem.h"
#include <qse/cmn/str.h>
#include <qse/cmn/pio.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>

#define MAX_SEND_SIZE 4096

#ifdef HAVE_SYS_SENDFILE_H
#	include <sys/sendfile.h>
#else
qse_ssize_t sendfile (
	int out_fd, int in_fd, qse_foff_t* offset, qse_size_t count)
{
	qse_mchar_t buf[MAX_SEND_SIZE];
	qse_ssize_t n;

	if (offset && lseek (in_fd, *offset, SEEK_SET) != *offset) 
		return (qse_ssize_t)-1;

	if (count > QSE_COUNTOF(buf)) count = QSE_COUNTOF(buf);
	n = read (in_fd, buf, count);
	if (n == (qse_ssize_t)-1 || n == 0) return n;

	n = send (out_fd, buf, n, 0);
	if (n > 0 && offset) *offset = *offset + n;

	return n;
}
#endif



/*------------------------------------------------------------------------*/

static int task_main_disconnect (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	shutdown (client->handle.i, SHUT_RDWR);
	return 0;
}

qse_httpd_task_t* qse_httpd_entaskdisconnect (
	qse_httpd_t* httpd, 
	qse_httpd_client_t* client,
	const qse_httpd_task_t* pred)
{
	qse_httpd_task_t task;
	
	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.main = task_main_disconnect;

	return qse_httpd_entask (httpd, client, pred, &task, 0);
}

/*------------------------------------------------------------------------*/

static int task_main_statictext (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	qse_ssize_t n;
	qse_size_t count = 0;
	const qse_mchar_t* ptr = (const qse_mchar_t*)task->ctx;

	while (*ptr != QSE_MT('\0') && count < MAX_SEND_SIZE)
	{
		ptr++; count++;
	}

/* TODO: do i need to add code to skip this send if count is 0? */
	n = send (
		client->handle.i,
		task->ctx,
		count,
		0
	);

	if (n <= -1) return -1;

	ptr = (const qse_mchar_t*)task->ctx + n;
	if (*ptr == QSE_MT('\0')) return 0;

	task->ctx = (void*)ptr;
	return 1; /* more work to do */
}

qse_httpd_task_t* qse_httpd_entaskstatictext (
     qse_httpd_t* httpd,
	qse_httpd_client_t* client,
	const qse_httpd_task_t* pred, 
	const qse_mchar_t* text)
{
	qse_httpd_task_t task;

	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.main = task_main_statictext;
	task.ctx = (void*)text;

	return qse_httpd_entask (httpd, client, pred, &task, 0);
}

/*------------------------------------------------------------------------*/

typedef struct task_text_t task_text_t;
struct task_text_t
{
	const qse_mchar_t* ptr;
	qse_size_t         left;
};

static int task_init_text (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_text_t* xtn = qse_httpd_gettaskxtn (httpd, task);

	QSE_MEMCPY (xtn, task->ctx, QSE_SIZEOF(*xtn));
	QSE_MEMCPY (xtn + 1, xtn->ptr, xtn->left);
	xtn->ptr = (qse_mchar_t*)(xtn + 1);

	task->ctx = xtn;
	return 0;
}

static int task_main_text (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	qse_ssize_t n;
	qse_size_t count;
	task_text_t* ctx = (task_text_t*)task->ctx;

	count = MAX_SEND_SIZE;
	if (count >= ctx->left) count = ctx->left;

/* TODO: do i need to add code to skip this send if count is 0? */
	n = send (
		client->handle.i,
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

qse_httpd_task_t* qse_httpd_entasktext (
     qse_httpd_t* httpd,
	qse_httpd_client_t* client,
	const qse_httpd_task_t* pred, 
	const qse_mchar_t* text)
{
	qse_httpd_task_t task;
	task_text_t data;

	QSE_MEMSET (&data, 0, QSE_SIZEOF(data));
	data.ptr = text;
	data.left = qse_mbslen(text);

	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = task_init_text;
	task.main = task_main_text;
	task.ctx = &data;

	return qse_httpd_entask (
		httpd, client, pred, &task, QSE_SIZEOF(data) + data.left);
}

/*------------------------------------------------------------------------*/

/* TODO: send wide character string when QSE_CHAR_IS_WCHAR */

/*------------------------------------------------------------------------*/

typedef struct task_format_t task_format_t;
struct task_format_t
{
	qse_mchar_t*       org;
	const qse_mchar_t* ptr;
	qse_size_t         left;
};

static int task_init_format (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_format_t* xtn = qse_httpd_gettaskxtn (httpd, task);

	QSE_MEMCPY (xtn, task->ctx, QSE_SIZEOF(*xtn));
	task->ctx = xtn;
	return 0;
}

static void task_fini_format (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_format_t* ctx = (task_format_t*)task->ctx;
	qse_httpd_freemem (httpd, ctx->org);
}

static int task_main_format (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	qse_ssize_t n;
	qse_size_t count;
	task_format_t* ctx = (task_format_t*)task->ctx;

	count = MAX_SEND_SIZE;
	if (count >= ctx->left) count = ctx->left;

	n = send (
		client->handle.i,
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

qse_httpd_task_t* qse_httpd_entaskformat (
     qse_httpd_t* httpd,
	qse_httpd_client_t* client, 
	const qse_httpd_task_t* pred,
	const qse_mchar_t* fmt, ...)
{
	qse_httpd_task_t task;
	task_format_t data;

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
		if (buf == QSE_NULL) return QSE_NULL;

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
				if (buf == QSE_NULL) return  QSE_NULL;
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
		if (buf == QSE_NULL) return QSE_NULL;

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
			return QSE_NULL;
		}
	}

	QSE_MEMSET (&data, 0, QSE_SIZEOF(data));
	data.org = buf;
	data.ptr = buf;
	data.left = l;

	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = task_init_format;
	task.fini = task_fini_format;
	task.main = task_main_format;
	task.ctx = &data;

	return qse_httpd_entask (
		httpd, client, pred, &task, QSE_SIZEOF(data));
}

/* TODO: send wide character string when QSE_CHAR_IS_WCHAR */

/*------------------------------------------------------------------------*/

typedef struct task_file_t task_file_t;
struct task_file_t
{
	qse_ubi_t handle;
	qse_foff_t left;
	qse_foff_t offset;
};

static int task_init_file (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_file_t* xtn = qse_httpd_gettaskxtn (httpd, task);
	QSE_MEMCPY (xtn, task->ctx, QSE_SIZEOF(*xtn));
	task->ctx = xtn;
	return 0;
}

static void task_fini_file (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_file_t* ctx = (task_file_t*)task->ctx;
	close (ctx->handle.i);
}

static int task_main_file (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	qse_ssize_t n;
	qse_size_t count;
	task_file_t* ctx = (task_file_t*)task->ctx;

	count = MAX_SEND_SIZE;
	if (count >= ctx->left) count = ctx->left;

/* TODO: more adjustment needed for OS with different sendfile semantics... */
	n = sendfile (
		client->handle.i,
		ctx->handle.i,
		&ctx->offset,
		count
	);

	if (n <= -1) return -1; /* TODO: any logging */

	if (n == 0 && count > 0)
	{
		/* The file could be truncated when this condition is set.
		 * The content-length sent in the header can't be fulfilled. 
		 * So let's return an error here so that the main loop abort 
		 * the connection. */
/* TODO: any logging....??? */
		return -1;	
	}

	ctx->left -= n;
	if (ctx->left <= 0) return 0;

	return 1; /* more work to do */
}

qse_httpd_task_t* qse_httpd_entaskfile (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client, 
	const qse_httpd_task_t* pred,
	qse_ubi_t handle,
	qse_foff_t offset,
	qse_foff_t size)
{
	qse_httpd_task_t task;
	task_file_t data;
	
	QSE_MEMSET (&data, 0, QSE_SIZEOF(data));
	data.handle = handle;
	data.offset = offset;
	data.left = size;

	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = task_init_file;
	task.main = task_main_file;
	task.fini = task_fini_file;
	task.ctx = &data;

	return qse_httpd_entask (httpd, client, pred, &task, QSE_SIZEOF(data));
}

/*------------------------------------------------------------------------*/

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

typedef struct task_path_t task_path_t;
struct task_path_t
{
	const qse_mchar_t* name;
	qse_http_range_t   range;
	qse_http_version_t version;
};

static int task_init_path (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_path_t* xtn = qse_httpd_gettaskxtn (httpd, task);
	QSE_MEMCPY (xtn, task->ctx, QSE_SIZEOF(*xtn));
	qse_mbscpy ((qse_mchar_t*)(xtn + 1), xtn->name);
	xtn->name = (qse_mchar_t*)(xtn + 1);
	task->ctx = xtn;
	return 0;
}

static int task_main_path (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_path_t* data = (task_path_t*)task->ctx;
	qse_ubi_t handle;
	struct stat st;
	qse_httpd_task_t* x = task;

qse_printf (QSE_T("opending file %S\n"), data->name);
	handle.i = open (data->name, O_RDONLY);
	if (handle.i <= -1)
	{
		const qse_mchar_t* msg = QSE_MT("<html><head><title>Not found</title></head><body><b>REQUESTED FILE NOT FOUND</b></body></html>");
		x = qse_httpd_entaskformat (
				httpd, client, x,
				QSE_MT("HTTP/%d.%d 404 Not found\r\nContent-Length: %d\r\n\r\n%s\r\n\r\n"), 
				data->version.major, data->version.minor,
				(int)qse_mbslen(msg) + 4, msg
		);
		goto no_file_send;
	}
	fcntl (handle.i, F_SETFD, FD_CLOEXEC);

	if (fstat (handle.i, &st) <= -1)
	{
		const qse_mchar_t* msg = QSE_MT("<html><head><title>Not found</title></head><body><b>REQUESTED FILE NOT FOUND</b></body></html>");

		x = qse_httpd_entaskformat (
			httpd, client, x,
			QSE_MT("HTTP/%d.%d 404 Not found\r\nContent-Length: %d\r\n\r\n%s\r\n\r\n"), 
			data->version.major, data->version.minor,
			(int)qse_mbslen(msg) + 4, msg
		);
		goto no_file_send;
	}

	if (S_ISDIR(st.st_mode))
	{
/* TODO: directory listing */
		const qse_mchar_t* msg = QSE_MT("<html><head><title>Directory Listing</title></head><body><li>file1<li>file2<li>file3</body></html>");

		x = qse_httpd_entaskformat (
			httpd, client, x,
			QSE_MT("HTTP/%d.%d 200 OK\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n%s\r\n\r\n"), 
			data->version.major, data->version.minor,
			(int)qse_mbslen(msg) + 4, msg
		);
		goto no_file_send;
	}

	if (st.st_size < 0) st.st_size = 0; /* can this happen? */

	if (data->range.type != QSE_HTTP_RANGE_NONE)
	{ 
		const qse_mchar_t* mime_type = QSE_NULL;

		if (data->range.type == QSE_HTTP_RANGE_SUFFIX)
		{
			if (data->range.to > st.st_size) data->range.to = st.st_size;
			data->range.from = st.st_size - data->range.to;
			data->range.to = data->range.to + data->range.from;
			if (st.st_size > 0) data->range.to--;
		}

		if (data->range.from >= st.st_size)
		{
			const qse_mchar_t* msg;

			msg = QSE_MT("<html><head><title>Requested range not satisfiable</title></head><body><b>REQUESTED RANGE NOT SATISFIABLE</b></body></html>");
			x = qse_httpd_entaskformat (
				httpd, client, x,
				QSE_MT("HTTP/%d.%d 416 Requested range not satisfiable\r\nContent-Length: %d\r\n\r\n%s\r\n\r\n"),
				data->version.major, data->version.minor,
				(int)qse_mbslen(msg) + 4, msg
			);
			goto no_file_send;
		}

		if (data->range.to >= st.st_size) data->range.to = st.st_size - 1;

		if (httpd->cbs->file.getmimetype)
		{
			httpd->errnum = QSE_HTTPD_ENOERR;
			mime_type = httpd->cbs->file.getmimetype (httpd, data->name);
			/*TODO: how to handle an error... */
		}

#if (QSE_SIZEOF_LONG_LONG > 0)
		x = qse_httpd_entaskformat (
			httpd, client, x,
    			QSE_MT("HTTP/%d.%d 206 Partial content\r\n%s%sContent-Length: %llu\r\nContent-Location: %s\r\nContent-Range: bytes %llu-%llu/%llu\r\n\r\n"), 
			data->version.major,
			data->version.minor,
			(mime_type? QSE_MT("\r\nContent-Type: "): QSE_MT("")),
			(mime_type? mime_type: QSE_MT("")),
			(unsigned long long)(data->range.to - data->range.from + 1),
			data->name,
			(unsigned long long)data->range.from,
			(unsigned long long)data->range.to,
			(unsigned long long)st.st_size
		);
#else
		x = qse_httpd_entaskformat (
			httpd, client, x,
    			QSE_MT("HTTP/%d.%d 206 Partial content\r\n%s%sContent-Length: %lu\r\nContent-Location: %s\r\nContent-Range: bytes %lu-%lu/%lu\r\n\r\n"), 
			data->version.major,
			data->version.minor,
			(mime_type? QSE_MT("\r\nContent-Type: "): QSE_MT("")),
			(mime_type? mime_type: QSE_MT("")),
			(unsigned long)(data->range.to - data->range.from + 1),
			data->name,
			(unsigned long)data->range.from,
			(unsigned long)data->range.to,
			(unsigned long)st.st_size
		);
#endif
		if (x)
		{
			x = qse_httpd_entaskfile (
					httpd, client, x,
					handle, 
					data->range.from, 
					(data->range.to - data->range.from + 1)
			);
		}
	}
	else
	{
/* TODO: int64 format.... don't hard code it llu */
		const qse_mchar_t* mime_type = QSE_NULL;

		if (httpd->cbs->file.getmimetype)
		{
			httpd->errnum = QSE_HTTPD_ENOERR;
			mime_type = httpd->cbs->file.getmimetype (httpd, data->name);
/*TODO: how to handle an error... */
		}

#if (QSE_SIZEOF_LONG_LONG > 0)
		x = qse_httpd_entaskformat (
			httpd, client, x,
    			QSE_MT("HTTP/%d.%d 200 OK\r\n%s%sContent-Length: %llu\r\nContent-Location: %s\r\n\r\n"), 
			data->version.major,
			data->version.minor,
			(mime_type? QSE_MT("\r\nContent-Type: "): QSE_MT("")),
			(mime_type? mime_type: QSE_MT("")),
			(unsigned long long)st.st_size,
			data->name
		);
#else
		x = qse_httpd_entaskformat (
			httpd, client, x,
    			QSE_MT("HTTP/%d.%d 200 OK\r\n%s%sContent-Length: %lu\r\nContent-Location: %s\r\n\r\n"), 
			data->version.major,
			data->version.minor,
			(mime_type? QSE_MT("\r\nContent-Type: "): QSE_MT("")),
			(mime_type? mime_type: QSE_MT("")),
			(unsigned long)st.st_size,
			data->name
		);
#endif
		if (x)
		{
			x = qse_httpd_entaskfile (
				httpd, client, x, handle, 0, st.st_size);
		}
	}

	return (x == QSE_NULL)? -1: 0;

no_file_send:
	if (handle.i >= 0) close (handle.i);
	return (x == QSE_NULL)? -1: 0;
}

qse_httpd_task_t* qse_httpd_entaskpath (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client, 
	const qse_httpd_task_t* pred,
	const qse_mchar_t* name,
	const qse_http_range_t* range, 
	const qse_http_version_t* verison)
{
	qse_httpd_task_t task;
	task_path_t data;

	QSE_MEMSET (&data, 0, QSE_SIZEOF(data));
	data.name = name;
	if (range) data.range = *range;
	else data.range.type = QSE_HTTP_RANGE_NONE;
	data.version = *verison;
	
	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = task_init_path;
	task.main = task_main_path;
	task.ctx = &data;

	return qse_httpd_entask (httpd, client, pred, &task, 
		QSE_SIZEOF(task_path_t) + qse_mbslen(name) + 1);
}

/*------------------------------------------------------------------------*/

typedef struct task_cgi_t task_cgi_t;
struct task_cgi_t
{
	const qse_char_t* path;
	qse_pio_t* pio;
	qse_mchar_t buf[MAX_SEND_SIZE];
	qse_size_t buflen;
};

static int task_init_cgi (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_cgi_t* xtn = (task_cgi_t*)qse_httpd_gettaskxtn (httpd, task);
	QSE_MEMSET (xtn, 0, QSE_SIZEOF(*xtn));
	qse_strcpy ((qse_char_t*)(xtn + 1), task->ctx);
	xtn->path = (qse_char_t*)(xtn + 1);
	task->ctx = xtn;
	return 0;
}

static void task_fini_cgi (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_cgi_t* cgi = (task_cgi_t*)task->ctx;
	if (cgi->pio) qse_pio_close (cgi->pio);
}
static int task_main_cgi_3 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_cgi_t* cgi = (task_cgi_t*)task->ctx;
	qse_ssize_t n;

	QSE_ASSERT (cgi->pio != QSE_NULL);

	n = send (client->handle.i, cgi->buf, cgi->buflen, 0);
	if (n <= -1)
	{
		/* can't return internal server error any more... */
/* TODO: logging ... */
		return -1;
	}

	QSE_MEMCPY (&cgi->buf[0], &cgi->buf[n], cgi->buflen - n);
	cgi->buflen -= n;

	return (cgi->buflen > 0)? 1: 0;
}

static int task_main_cgi_2 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_cgi_t* cgi = (task_cgi_t*)task->ctx;
	qse_ssize_t n;
	
	QSE_ASSERT (cgi->pio != QSE_NULL);

	 /* <- can i make it non-block?? or use select??? pio_tryread()? */
	n = qse_pio_read (
		cgi->pio, 
		&cgi->buf[cgi->buflen], 
		QSE_SIZEOF(cgi->buf) - cgi->buflen,
		QSE_PIO_OUT
	);
	if (n <= -1)
	{
		/* can't return internal server error any more... */
/* TODO: logging ... */
		return -1;
	}
	if (n == 0) 
	{
		if (cgi->buflen > 0)
		{
			task->main = task_main_cgi_3;
			return task_main_cgi_3 (httpd, client, task);
		}
		else return 0;
	}
			
	cgi->buflen += n;

	n = send (client->handle.i, cgi->buf, cgi->buflen, 0);
	if (n <= -1)
	{
		/* can't return internal server error any more... */
/* TODO: logging ... */
		return -1;
	}

	QSE_MEMCPY (&cgi->buf[0], &cgi->buf[n], cgi->buflen - n);
	cgi->buflen -= n;

	return 1;
}

static int task_main_cgi (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_cgi_t* cgi = (task_cgi_t*)task->ctx;

qse_printf (QSE_T("[pip open for %s]\n"), cgi->path);
	cgi->pio = qse_pio_open (httpd->mmgr, 0, cgi->path, QSE_PIO_READOUT | QSE_PIO_WRITEIN);
	if (cgi->pio == QSE_NULL)
	{
		/* TODO: entask internal server errror */
qse_printf (QSE_T("internal server error....\n"));
		return 0;
	}

	task->main = task_main_cgi_2; /* cause this function to be called subsequently */

	return task_main_cgi_2 (httpd, client, task); /* let me call it here once */
}

qse_httpd_task_t* qse_httpd_entaskcgi (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client,
	const qse_httpd_task_t* pred, 
	const qse_char_t* path)
{
	qse_httpd_task_t task;
	
	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = task_init_cgi;
	task.fini = task_fini_cgi;
	task.main = task_main_cgi;
	task.ctx = (void*)path;

	return qse_httpd_entask (
		httpd, client, pred, &task, 
		QSE_SIZEOF(task_cgi_t) + ((qse_strlen(path) + 1) * QSE_SIZEOF(*path))
	);
}