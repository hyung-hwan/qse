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
	shutdown (client->handle.i, SHUT_RDWR);
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
	ssize_t n;
	size_t count;
	task_text_t* ctx = (task_text_t*)task->ctx;

	count = MAX_SENDFILE_SIZE;
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

int qse_httpd_entasktext (
     qse_httpd_t* httpd, qse_httpd_client_t* client, const qse_mchar_t* text)
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
		httpd, client, &task, QSE_SIZEOF(data) + data.left);
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
	ssize_t n;
	size_t count;
	task_format_t* ctx = (task_format_t*)task->ctx;

	count = MAX_SENDFILE_SIZE;
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

int qse_httpd_entaskformat (
     qse_httpd_t* httpd, qse_httpd_client_t* client, const qse_mchar_t* fmt, ...)
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
	task.init = task_init_format;
	task.fini = task_fini_format;
	task.main = task_main_format;
	task.ctx = &data;

	return qse_httpd_entask (
		httpd, client, &task, QSE_SIZEOF(data));
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
	ssize_t n;
	size_t count;
	task_file_t* ctx = (task_file_t*)task->ctx;

	count = MAX_SENDFILE_SIZE;
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

int qse_httpd_entaskfile (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_ubi_t handle, qse_foff_t offset, qse_foff_t size)
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

	return qse_httpd_entask (httpd, client, &task, QSE_SIZEOF(data));
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
	int x;

	handle.i = open (data->name, O_RDONLY);
	if (handle.i <= -1)
	{
		const qse_mchar_t* msg = QSE_MT("<html><head><title>Not found</title></head><body><b>REQUESTED FILE NOT FOUND</b></body></html>");
		x = qse_httpd_entaskformat (httpd, client,
				QSE_MT("HTTP/%d.%d 404 Not found\r\nContent-Length: %d\r\n\r\n%s\r\n\r\n"), 
				data->version.major, data->version.minor,
				(int)qse_mbslen(msg) + 4, msg
		);
		if (x <= -1) return -1;
	}

	if (fstat (handle.i, &st) <= -1)
	{
		const qse_mchar_t* msg = QSE_MT("<html><head><title>Not found</title></head><body><b>REQUESTED FILE NOT FOUND</b></body></html>");

		x = qse_httpd_entaskformat (httpd, client,
			QSE_MT("HTTP/%d.%d 404 Not found\r\nContent-Length: %d\r\n\r\n%s\r\n\r\n"), 
			data->version.major, data->version.minor,
			(int)qse_mbslen(msg) + 4, msg
		);
		if (x <= -1) goto oops;
	}

	if (st.st_size < 0) st.st_size = 0; /* can this happen? */

	if (data->range.type != QSE_HTTP_RANGE_NONE)
	{ 
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
			x = qse_httpd_entaskformat (httpd, client,
				QSE_MT("HTTP/%d.%d 416 Requested range not satisfiable\r\nContent-Length: %d\r\n\r\n%s\r\n\r\n"),
				data->version.major, data->version.minor,
				(int)qse_mbslen(msg) + 4, msg
			);
			if (x <= -1) goto oops;
		}

		if (data->range.to >= st.st_size) data->range.to = st.st_size - 1;

		x = qse_httpd_entaskformat (httpd, client,
    			QSE_MT("HTTP/%d.%d 206 Partial content\r\nContent-Length: %llu\r\nContent-Location: %s\r\nContent-Range: bytes %llu-%llu/%llu\r\n\r\n"), 
			data->version.major,
			data->version.minor,
			(unsigned long long)(data->range.to - data->range.from + 1),
			data->name,
			(unsigned long long)data->range.from,
			(unsigned long long)data->range.to,
			st.st_size
		);
		if (x <= -1) goto oops;

		x = qse_httpd_entaskfile (
				httpd, client, handle, 
				data->range.from, 
				(data->range.to - data->range.from + 1)
		);
		if (x <= -1) goto oops;
	}
	else
	{
/* TODO: int64 format.... don't hard code it llu */

		x = qse_httpd_entaskformat (httpd, client,
    			QSE_MT("HTTP/%d.%d 200 OK\r\nContent-Length: %llu\r\nContent-Location: %s\r\n\r\n"), 
			data->version.major,
			data->version.minor,
			(unsigned long long)st.st_size,
			data->name
		);
		if (x <= -1) goto oops;

		x = qse_httpd_entaskfile (httpd, client, handle, 0, st.st_size);
		if (x <= -1) goto oops;
	}

	return 0;

oops:
	close (handle.i);
	return -1;
}

int qse_httpd_entaskpath (
	qse_httpd_t* httpd, qse_httpd_client_t* client,
	const qse_mchar_t* name, const qse_http_range_t* range, 
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

	return qse_httpd_entask (httpd, client, &task, 
		QSE_SIZEOF(task_path_t) + qse_mbslen(name) + 1);
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
