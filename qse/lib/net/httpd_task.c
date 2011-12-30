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

#if defined(_WIN32) || defined(__DOS__) || defined(__OS2__)
/* UNSUPPORTED YET..  */
/* TODO: IMPLEMENT THIS */
#else

#include "httpd.h"
#include "../cmn/mem.h"
#include <qse/cmn/str.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/pio.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>

#define MAX_SEND_SIZE 4096

#ifdef HAVE_SYS_SENDFILE_H
#	include <sys/sendfile.h>
#else
static qse_ssize_t sendfile (
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

qse_printf (QSE_T("SEND: [%.*S]\n"), (int)l, buf);
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

qse_printf (QSE_T("opending file %hs\n"), data->name);
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
    			QSE_MT("HTTP/%d.%d 206 Partial content\r\n%s%s%sContent-Length: %llu\r\nContent-Location: %s\r\nContent-Range: bytes %llu-%llu/%llu\r\n\r\n"), 
			data->version.major,
			data->version.minor,
			(mime_type? QSE_MT("Content-Type: "): QSE_MT("")),
			(mime_type? mime_type: QSE_MT("")),
			(mime_type? QSE_MT("\r\n"): QSE_MT("")),
			(unsigned long long)(data->range.to - data->range.from + 1),
			data->name,
			(unsigned long long)data->range.from,
			(unsigned long long)data->range.to,
			(unsigned long long)st.st_size
		);
#else
		x = qse_httpd_entaskformat (
			httpd, client, x,
    			QSE_MT("HTTP/%d.%d 206 Partial content\r\n%s%s%sContent-Length: %lu\r\nContent-Location: %s\r\nContent-Range: bytes %lu-%lu/%lu\r\n\r\n"), 
			data->version.major,
			data->version.minor,
			(mime_type? QSE_MT("Content-Type: "): QSE_MT("")),
			(mime_type? mime_type: QSE_MT("")),
			(mime_type? QSE_MT("\r\n"): QSE_MT("")),
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
    			QSE_MT("HTTP/%d.%d 200 OK\r\n%s%s%sContent-Length: %llu\r\nContent-Location: %s\r\n\r\n"), 
			data->version.major,
			data->version.minor,
			(mime_type? QSE_MT("Content-Type: "): QSE_MT("")),
			(mime_type? mime_type: QSE_MT("")),
			(mime_type? QSE_MT("\r\n"): QSE_MT("")),
			(unsigned long long)st.st_size,
			data->name
		);
#else
		x = qse_httpd_entaskformat (
			httpd, client, x,
    			QSE_MT("HTTP/%d.%d 200 OK\r\n%s%s%sContent-Length: %lu\r\nContent-Location: %s\r\n\r\n"), 
			data->version.major,
			data->version.minor,
			(mime_type? QSE_MT("Content-Type: "): QSE_MT("")),
			(mime_type? mime_type: QSE_MT("")),
			(mime_type? QSE_MT("\r\n"): QSE_MT("")),
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

typedef struct task_cgi_arg_t task_cgi_arg_t;
struct task_cgi_arg_t 
{
	const qse_mchar_t* path;
	const qse_htre_t* req;
};

typedef struct task_cgi_t task_cgi_t;
struct task_cgi_t
{
	int init_failed;

	const qse_mchar_t* path;
	qse_http_version_t version;

	qse_env_t* env;
	qse_pio_t* pio;
	qse_htrd_t* htrd;

	qse_mbs_t*   res;
	qse_mchar_t* res_ptr;
	qse_size_t   res_left;	

	/* if true, close connection after response is sent out */
	int disconnect;
	/* if true, the content of response is chunked */
	int content_chunked;
	/* if true, content_length is set. */
	int content_length_set;
	/* content-length that CGI returned */
	qse_size_t content_length;
	/* the number of octets in the contents received */
	qse_size_t content_received; 

	qse_mchar_t buf[MAX_SEND_SIZE];
	qse_size_t  buflen;
};

typedef struct cgi_htrd_xtn_t cgi_htrd_xtn_t;
struct cgi_htrd_xtn_t
{
	task_cgi_t* cgi;
};

int walk_cgi_headers (qse_htre_t* req, const qse_mchar_t* key, const qse_mchar_t* val, void* ctx)
{
	task_cgi_t* cgi = (task_cgi_t*)ctx;

	if (qse_mbscmp (key, "Status") != 0)
	{
		if (qse_mbs_cat (cgi->res, key) == (qse_size_t)-1) return -1;
		if (qse_mbs_cat (cgi->res, QSE_MT(": ")) == (qse_size_t)-1) return -1;
		if (qse_mbs_cat (cgi->res, val) == (qse_size_t)-1) return -1;
		if (qse_mbs_cat (cgi->res, QSE_MT("\r\n")) == (qse_size_t)-1) return -1;
	}

	return 0;
}

static int cgi_htrd_handle_request (qse_htrd_t* htrd, qse_htre_t* req)
{
	cgi_htrd_xtn_t* xtn = (cgi_htrd_xtn_t*) qse_htrd_getxtn (htrd);
	task_cgi_t* cgi = xtn->cgi;
	const qse_mchar_t* status;
	static qse_http_version_t v11 = { 1, 1 };

	QSE_ASSERT (req->attr.hurried);

	status = qse_htre_getheaderval (req, QSE_MT("Status"));
	if (status)
	{
		qse_mchar_t buf[128];
		int nstatus;
		qse_mchar_t* endptr;

/* TODO: check the syntax of status value??? */

		QSE_MSTRTONUM (nstatus,status,&endptr,10);

		snprintf (buf, QSE_COUNTOF(buf), 	
			QSE_MT("HTTP/%d.%d %d "),
			cgi->version.major, 
			cgi->version.minor, 
			nstatus
		);

		/* 
		Would it need this kind of extra work?
		while (QSE_ISMSPACE(*endptr)) endptr++;
		if (*endptr == QSE_MT('\0')) ....
		*/

		if (qse_mbs_cat (cgi->res, buf) == (qse_size_t)-1) return -1;
		if (qse_mbs_cat (cgi->res, endptr) == (qse_size_t)-1) return -1;
		if (qse_mbs_cat (cgi->res, QSE_MT("\r\n")) == (qse_size_t)-1) return -1;
	}
	else
	{
		qse_mchar_t buf[128];
		snprintf (buf, QSE_COUNTOF(buf), 	
			QSE_MT("HTTP/%d.%d 200 OK\r\n"),
			cgi->version.major, cgi->version.minor
		);
		if (qse_mbs_cat (cgi->res, buf) == (qse_size_t)-1) return -1;
	}

	if (req->attr.content_length_set) 
	{
		cgi->content_length_set = 1;
		cgi->content_length = req->attr.content_length;
	}
	else
	{
		/* no Content-Length returned by CGI */
		if (qse_comparehttpversions (&cgi->version, &v11) >= 0) 
			cgi->content_chunked = 1;
		else cgi->disconnect = 1;
	}

	if (cgi->content_chunked)
	{
		if (qse_mbs_cat (cgi->res, QSE_MT("Transfer-Encoding: chunked\r\n")) == (qse_size_t)-1) return -1;
	}

	if (qse_htre_walkheaders (req, walk_cgi_headers, cgi) <= -1) return -1;
	if (qse_mbs_ncat (cgi->res, QSE_MT("\r\n"), 2) == (qse_size_t)-1) return -1;

	cgi->content_received = qse_htre_getcontentlen(req);
	if (cgi->content_length_set && 
	    cgi->content_received > cgi->content_length)
	{
/* TODO: cgi returning too much data... something is wrong in CGI */
		return -1;
	}

	if (cgi->content_received > 0)
	{
		if (cgi->content_chunked)
		{
			qse_mchar_t buf[64];
			snprintf (buf, QSE_COUNTOF(buf), QSE_MT("%lX\r\n"), (unsigned long)cgi->content_received);
			if (qse_mbs_cat (cgi->res, buf) == (qse_size_t)-1) return -1;
		}

		if (qse_mbs_ncat (cgi->res, qse_htre_getcontentptr(req), qse_htre_getcontentlen(req)) == (qse_size_t)-1) return -1;

		if (cgi->content_chunked)
		{
			if (qse_mbs_ncat (cgi->res, QSE_MT("\r\n"), 2) == (qse_size_t)-1) return -1;
		}
	}

	return 0;
}

static qse_htrd_recbs_t cgi_htrd_cbs =
{
	cgi_htrd_handle_request,
	QSE_NULL, /* not needed for CGI */
     QSE_NULL  /* not needed for CGI */
};

static qse_env_t* makecgienv (
	qse_httpd_t* httpd, qse_httpd_client_t* client, const qse_htre_t* req)
{
/* TODO: error check */
	qse_env_t* env;

	env = qse_env_open (httpd->mmgr, 0, 0);
	if (env == QSE_NULL) goto oops;

#ifdef _WIN32
	qse_env_insertsys (env, QSE_T("PATH"));
#else
	qse_env_insertsysm (env, QSE_MT("LANG"));
	qse_env_insertsysm (env, QSE_MT("PATH"));
	//qse_env_insertm (env, QSE_MT("SERVER_PORT"), );

	{
		qse_mchar_t port[16];
		snprintf (port, QSE_COUNTOF(port), 
			"%d", (int)ntohs(client->addr.in4.sin_port));
		qse_env_insertm (env, QSE_MT("REMOTE_PORT"), port);
	}
	//qse_env_insertm (env, QSE_MT("REMOTE_ADDR"), QSE_MT("what the hell"));
#endif

#if 0
	qse_env_insertm (env, "SERVER_NAME",
	qse_env_insertm (env, "SERVER_ROOT", 
	qse_env_insertm (env, "DOCUMENT_ROOT", 
	qse_env_insertm (env, "REMOTE_PORT", 
	qse_env_insertm (env, "REQUEST_URI", 
#endif
	return env;

oops:
	if (env) qse_env_close (env);
	return QSE_NULL;
}

static int task_init_cgi (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_cgi_t* xtn = (task_cgi_t*)qse_httpd_gettaskxtn (httpd, task);
	task_cgi_arg_t* arg = (task_cgi_arg_t*)task->ctx;

	QSE_MEMSET (xtn, 0, QSE_SIZEOF(*xtn));
	qse_mbscpy ((qse_mchar_t*)(xtn + 1), arg->path);
	xtn->path = (qse_mchar_t*)(xtn + 1);
	xtn->version = *qse_htre_getversion(arg->req);

	xtn->env = makecgienv (httpd, client, arg->req);
	if (xtn->env == QSE_NULL) xtn->init_failed = 1;

	task->ctx = xtn;
	return 0;
}

static void task_fini_cgi (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_cgi_t* cgi = (task_cgi_t*)task->ctx;
	if (cgi->env) qse_env_close (cgi->env);
	if (cgi->pio) qse_pio_close (cgi->pio);
	if (cgi->res) qse_mbs_close (cgi->res);
	if (cgi->htrd) qse_htrd_close (cgi->htrd);
qse_printf (QSE_T("task_fini_cgi\n"));
}

static int task_main_cgi_5 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_cgi_t* cgi = (task_cgi_t*)task->ctx;
	qse_ssize_t n;

	QSE_ASSERT (cgi->pio != QSE_NULL);

qse_printf (QSE_T("task_main_cgi_5\n"));

/* TODO: check if cgi outputs more than content-length if it is set... */
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

static int task_main_cgi_4 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_cgi_t* cgi = (task_cgi_t*)task->ctx;
	qse_ssize_t n;
	
	QSE_ASSERT (cgi->pio != QSE_NULL);

	/* this function assumes that the chunk length does not exceeded 
	 * 4 hexadecimal digits. */
	QSE_ASSERT (QSE_SIZEOF(cgi->buf) <= 0xFFFF);

qse_printf (QSE_T("task_main_cgi_4\n"));

	if (cgi->content_chunked)
	{
		qse_size_t count, extra;
		qse_mchar_t chunklen[7];

qse_printf (QSE_T("READING CHUNKED MODE...\n"));
		extra = (QSE_SIZEOF(chunklen) - 1) + 2;
		count = QSE_SIZEOF(cgi->buf) - cgi->buflen;
		if (count > extra)
		{

/* TODO: check if cgi outputs more than content-length if it is set... */
	 /* <- can i make it non-block?? or use select??? pio_tryread()? */

			n = qse_pio_read (
				cgi->pio, QSE_PIO_OUT,
				&cgi->buf[cgi->buflen + QSE_SIZEOF(chunklen) - 1], 
				count - extra
			);
			if (n <= -1)
			{
				/* can't return internal server error any more... */
/* TODO: logging ... */
				return -1;
			}
			if (n == 0) 
			{
				cgi->buf[cgi->buflen++] = QSE_MT('0');
				cgi->buf[cgi->buflen++] = QSE_MT('\r');
				cgi->buf[cgi->buflen++] = QSE_MT('\n');
				cgi->buf[cgi->buflen++] = QSE_MT('\r');
				cgi->buf[cgi->buflen++] = QSE_MT('\n');

				task->main = task_main_cgi_5;
				return task_main_cgi_5 (httpd, client, task);
			}

			/* set the chunk length */
			snprintf (chunklen, QSE_COUNTOF(chunklen), QSE_MT("%-4lX\r\n"), n);
			QSE_MEMCPY (&cgi->buf[cgi->buflen], chunklen, QSE_SIZEOF(chunklen) - 1);
			cgi->buflen += QSE_SIZEOF(chunklen) - 1 + n;
	
			/* set the trailing CR & LF for a chunk */
			cgi->buf[cgi->buflen++] = QSE_MT('\r');
			cgi->buf[cgi->buflen++] = QSE_MT('\n');

			cgi->content_received += n;
		}
	}
	else
	{
qse_printf (QSE_T("READING IN NON-CHUNKED MODE...\n"));
		n = qse_pio_read (
			cgi->pio, QSE_PIO_OUT,
			&cgi->buf[cgi->buflen], 
			QSE_SIZEOF(cgi->buf) - cgi->buflen
		);
		if (n <= -1)
		{
			/* can't return internal server error any more... */
/* TODO: loggig ... */
			return -1;
		}
		if (n == 0)
		{
			task->main = task_main_cgi_5;
			return task_main_cgi_5 (httpd, client, task);
		}

		cgi->buflen += n;
		cgi->content_received += n;
	}

	if (cgi->content_length_set && 
	    cgi->content_received > cgi->content_length)
	{
/* TODO: cgi returning too much data... something is wrong in CGI */
qse_printf (QSE_T("CGI FUCKED UP...RETURNING TOO MUCH DATA\n"));
		return -1;
	}

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

static int task_main_cgi_3 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	/* send the http initial line and headers built using the headers
	 * returned by CGI. it may include some contents as well */

	task_cgi_t* cgi = (task_cgi_t*)task->ctx;
	qse_ssize_t n;
	qse_size_t count;

qse_printf (QSE_T("task_main_cgi_3\n"));
	count = MAX_SEND_SIZE;
	if (count >= cgi->res_left) count = cgi->res_left;

	n = send (
		client->handle.i,
		cgi->res_ptr,
		count,
		0
	);

	if (n <= -1) return -1;

	cgi->res_left -= n;
	if (cgi->res_left <= 0) 
	{
		task->main = task_main_cgi_4;
		return task_main_cgi_4 (httpd, client, task);
	}

	cgi->res_ptr += n;
	return 1; /* more work to do */
}

static int task_main_cgi_2 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	/* several calls to this function will read output from the cgi 
	 * until the end of header is reached. when the end is reached, 
	 * it is possible that some contents are also read in.
	 * The callback function to qse_htrd_feed() handles this also.
	 */

	task_cgi_t* cgi = (task_cgi_t*)task->ctx;
	qse_ssize_t n;
	
	QSE_ASSERT (cgi->pio != QSE_NULL);

qse_printf (QSE_T("[cgi_2 ]\n"));
	 /* <- can i make it non-block?? or use select??? pio_tryread()? */
	n = qse_pio_read (
		cgi->pio, QSE_PIO_OUT,
		&cgi->buf[cgi->buflen], 
		QSE_SIZEOF(cgi->buf) - cgi->buflen
	);
	if (n <= -1)
	{
		/* can't return internal server error any more... */
/* TODO: logging ... */
		return -1;
	}
	if (n == 0) 
	{
		/* end of output from cgi before it has seen a header.
		 * the cgi script must be crooked. */
/* TODO: logging */
		qse_pio_kill (cgi->pio);
		return -1;
	}
			
	cgi->buflen += n;

	if (qse_htrd_feed (cgi->htrd, cgi->buf, cgi->buflen) <= -1)
	{
/* TODO: logging */
		return -1;
	}

	cgi->buflen = 0;

	if (QSE_MBS_LEN(cgi->res) > 0)
	{
		/* the htrd handler composed some response.
		 * this means that at least it finished processing CGI headers.
		 * some contents could be in cgi->res, though.
		 */

		if (cgi->disconnect && 
		    qse_httpd_entaskdisconnect (httpd, client, task) == QSE_NULL) return -1;

		cgi->res_ptr = QSE_MBS_PTR(cgi->res);
		cgi->res_left = QSE_MBS_LEN(cgi->res);

		task->main = task_main_cgi_3;
		return task_main_cgi_3 (httpd, client, task);
	}

	/* complete headers not seen yet. i need to be called again */
	return 1;
}

static int task_main_cgi (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_cgi_t* cgi = (task_cgi_t*)task->ctx;
	cgi_htrd_xtn_t* xtn;
/* TODO: get the message using callback */
	const qse_mchar_t* msg = "Internal server error has occurred";

	if (cgi->init_failed) goto oops;

	cgi->htrd = qse_htrd_open (httpd->mmgr, QSE_SIZEOF(cgi_htrd_xtn_t));
	if (cgi->htrd == QSE_NULL) goto oops;
	xtn = (cgi_htrd_xtn_t*) qse_htrd_getxtn (cgi->htrd);
	xtn->cgi = cgi;
	qse_htrd_setrecbs (cgi->htrd, &cgi_htrd_cbs);
	qse_htrd_setoption (
		cgi->htrd, 
		QSE_HTRD_SKIPINITIALLINE | 
		QSE_HTRD_HURRIED | 
		QSE_HTRD_REQUEST
	);

	cgi->res = qse_mbs_open (httpd->mmgr, 0, 256);
	if (cgi->res == QSE_NULL) goto oops;

	cgi->pio = qse_pio_open (
		httpd->mmgr, 0, (const qse_char_t*)cgi->path, cgi->env,
		QSE_PIO_READOUT | QSE_PIO_WRITEIN | QSE_PIO_ERRTONUL | QSE_PIO_MBSCMD
	);
	if (cgi->pio == QSE_NULL) goto oops;
	
	task->main = task_main_cgi_2; /* cause this function to be called subsequently */
	return task_main_cgi_2 (httpd, client, task); /* let me call it here once */

oops:
	if (cgi->res) qse_mbs_close (cgi->res);
	if (cgi->htrd) qse_htrd_close (cgi->htrd);

	qse_httpd_entaskformat (
		httpd, client, task,
    		QSE_MT("HTTP/%d.%d 500 Internal Server Error\r\nContent-Type: text/plain\r\nContent-Length: %lu\r\n\r\n%s"), 
		cgi->version.major,
		cgi->version.minor,
		(unsigned long)qse_mbslen(msg),
		msg
	);
/* TODO: can i return something else if this fails... */

	return 0;
}

qse_httpd_task_t* qse_httpd_entaskcgi (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client,
	const qse_httpd_task_t* pred, 
	const qse_mchar_t* path,
	const qse_htre_t* req)
{
	qse_httpd_task_t task;
	task_cgi_arg_t arg;

	arg.path = path;
	arg.req = req;

	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = task_init_cgi;
	task.fini = task_fini_cgi;
	task.main = task_main_cgi;
	task.ctx = &arg;

	return qse_httpd_entask (
		httpd, client, pred, &task, 
		QSE_SIZEOF(task_cgi_t) + ((qse_mbslen(path) + 1) * QSE_SIZEOF(*path))
	);
}


/*------------------------------------------------------------------------*/

/* 
typedef struct task_proxy_t task_proxy_t;
struct task_proxy_t
{
}

qse_httpd_task_t* qse_httpd_entaskproxy (...) 
{
}
*/

/*------------------------------------------------------------------------*/

#endif
