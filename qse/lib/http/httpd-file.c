/*
 * $Id$
 *
    Copyright 2006-2014 Chung, Hyung-Hwan.
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
#include <qse/cmn/fmt.h>

#define ETAG_LEN_MAX 127

#define PUTFILE_INIT_FAILED  (1 << 0)
#define PUTFILE_WRITE_FAILED (1 << 1)

typedef struct task_file_t task_file_t;
struct task_file_t
{
	qse_mcstr_t path;

	qse_http_version_t version;
	int                keepalive;
	int                method;

	union
	{
		struct
		{
			qse_mcstr_t mime;
			qse_mchar_t if_none_match[ETAG_LEN_MAX + 1];
			qse_ntime_t if_modified_since;
			qse_http_range_t range;
		} get;
		struct
		{
			qse_httpd_t* httpd;
			int flags;
			int status;
			qse_htre_t* req;
			qse_httpd_hnd_t handle;
		} put;
	} u;
};

/*------------------------------------------------------------------------*/

typedef struct task_getfseg_t task_getfseg_t;
struct task_getfseg_t
{
	qse_httpd_hnd_t handle;
	qse_foff_t left;
	qse_foff_t offset;
};

static int task_init_getfseg (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_getfseg_t* xtn = qse_httpd_gettaskxtn (httpd, task);
	QSE_MEMCPY (xtn, task->ctx, QSE_SIZEOF(*xtn));
	task->ctx = xtn;
	return 0;
}

static void task_fini_getfseg (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_getfseg_t* ctx = (task_getfseg_t*)task->ctx;
	httpd->opt.scb.file.close (httpd, ctx->handle);
}

static int task_main_getfseg (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	qse_ssize_t n;
	qse_size_t count;
	task_getfseg_t* ctx = (task_getfseg_t*)task->ctx;

	count = MAX_SEND_SIZE;
	if (count >= ctx->left) count = ctx->left;

	httpd->errnum = QSE_HTTPD_ENOERR;
	n = httpd->opt.scb.client.sendfile (
		httpd, client, ctx->handle, &ctx->offset, count);
	if (n <= -1) 
	{
/* HANDLE EGAIN specially??? */
		if (httpd->errnum != QSE_HTTPD_EAGAIN)
		{
			/* TODO: logging */
			return -1; 
		}

		goto more_work;
	}

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

more_work:
	return 1; /* more work to do */
}

static qse_httpd_task_t* entask_getfseg (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_httpd_task_t* pred,
	qse_httpd_hnd_t handle, qse_foff_t offset, qse_foff_t size)
{
	qse_httpd_task_t task;
	task_getfseg_t data;

	QSE_MEMSET (&data, 0, QSE_SIZEOF(data));
	data.handle = handle;
	data.offset = offset;
	data.left = size;

	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = task_init_getfseg;
	task.main = task_main_getfseg;
	task.fini = task_fini_getfseg;
	task.ctx = &data;

	return qse_httpd_entask (httpd, client, pred, &task, QSE_SIZEOF(data));
}

/*------------------------------------------------------------------------*/


static int task_init_getfile (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_file_t* file = qse_httpd_gettaskxtn (httpd, task);
	task_file_t* arg = (task_file_t*)task->ctx;

	QSE_MEMCPY (file, arg, QSE_SIZEOF(*file));

	file->path.ptr = (qse_mchar_t*)(file + 1);
	qse_mbscpy ((qse_mchar_t*)file->path.ptr, arg->path.ptr);
	if (arg->u.get.mime.ptr)
	{
		file->u.get.mime.ptr = file->path.ptr + file->path.len + 1;
		qse_mbscpy ((qse_mchar_t*)file->u.get.mime.ptr, arg->u.get.mime.ptr);
	}

	task->ctx = file;
	return 0;
}

static QSE_INLINE int task_main_getfile (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_file_t* file;
	qse_httpd_task_t* x;
	qse_httpd_hnd_t handle;
	int fileopen = 0;
	qse_httpd_stat_t st;

	file = (task_file_t*)task->ctx;
	x = task;

/* TODO: if you should deal with files on a network-mounted drive,
         setting a trigger or non-blocking I/O are needed. */

	httpd->errnum = QSE_HTTPD_ENOERR;
	if (httpd->opt.scb.file.stat (httpd, file->path.ptr, &st) <= -1)
	{
		int http_errnum;
		http_errnum = (httpd->errnum == QSE_HTTPD_ENOENT)? 404:
		              (httpd->errnum == QSE_HTTPD_EACCES)? 403: 500;
		x = qse_httpd_entask_error (
			httpd, client, x, http_errnum, 
			file->method, &file->version, file->keepalive);
		goto no_file_send;
	}

	httpd->errnum = QSE_HTTPD_ENOERR;
	if (httpd->opt.scb.file.ropen (httpd, file->path.ptr, &handle) <= -1)
	{
		int http_errnum;
printf ("ropen failure...\n");
		http_errnum = (httpd->errnum == QSE_HTTPD_ENOENT)? 404:
		              (httpd->errnum == QSE_HTTPD_EACCES)? 403: 500;
		x = qse_httpd_entask_error (
			httpd, client, x, http_errnum, 
			file->method, &file->version, file->keepalive);
		goto no_file_send;
	}
	fileopen = 1;

printf ("ropen ok...\n");
	if (file->u.get.range.type != QSE_HTTP_RANGE_NONE)
	{ 
		qse_mchar_t tmp[4][64];
		qse_mchar_t etag[ETAG_LEN_MAX + 1];
		qse_size_t etag_len;

		if (file->u.get.range.type == QSE_HTTP_RANGE_SUFFIX)
		{
			if (file->u.get.range.to > st.size) file->u.get.range.to = st.size;
			file->u.get.range.from = st.size - file->u.get.range.to;
			file->u.get.range.to = file->u.get.range.to + file->u.get.range.from;
			if (st.size > 0) file->u.get.range.to--;
		}

		if (file->u.get.range.from >= st.size)
		{
			x = qse_httpd_entask_error (
				httpd, client, x, 416, file->method, &file->version, file->keepalive);
			goto no_file_send;
		}

		if (file->u.get.range.to >= st.size) file->u.get.range.to = st.size - 1;

		qse_fmtuintmaxtombs (tmp[0], QSE_COUNTOF(tmp[0]), (file->u.get.range.to - file->u.get.range.from + 1), 10, -1, QSE_MT('\0'), QSE_NULL);
		qse_fmtuintmaxtombs (tmp[1], QSE_COUNTOF(tmp[1]), file->u.get.range.from, 10, -1, QSE_MT('\0'), QSE_NULL);
		qse_fmtuintmaxtombs (tmp[2], QSE_COUNTOF(tmp[2]), file->u.get.range.to, 10, -1, QSE_MT('\0'), QSE_NULL);
		qse_fmtuintmaxtombs (tmp[3], QSE_COUNTOF(tmp[3]), st.size, 10, -1, QSE_MT('\0'), QSE_NULL);

		etag_len = qse_fmtuintmaxtombs (&etag[0], QSE_COUNTOF(etag), st.mtime.sec, 16, -1, QSE_MT('\0'), QSE_NULL);
		etag[etag_len++] = QSE_MT('-');
		etag_len += qse_fmtuintmaxtombs (&etag[etag_len], QSE_COUNTOF(etag), st.mtime.nsec, 16, -1, QSE_MT('\0'), QSE_NULL);
		etag[etag_len++] = QSE_MT('-');
		etag_len += qse_fmtuintmaxtombs (&etag[etag_len], QSE_COUNTOF(etag) - etag_len, st.size, 16, -1, QSE_MT('\0'), QSE_NULL);
		etag[etag_len++] = QSE_MT('-');
		etag_len += qse_fmtuintmaxtombs (&etag[etag_len], QSE_COUNTOF(etag) - etag_len, st.ino, 16, -1, QSE_MT('\0'), QSE_NULL);
		etag[etag_len++] = QSE_MT('-');
		etag_len += qse_fmtuintmaxtombs (&etag[etag_len], QSE_COUNTOF(etag) - etag_len, st.dev, 16, -1, QSE_MT('\0'), QSE_NULL);

		x = qse_httpd_entaskformat (
			httpd, client, x,
    			QSE_MT("HTTP/%d.%d 206 Partial Content\r\nServer: %s\r\nDate: %s\r\nConnection: %s\r\n%s%s%sContent-Length: %s\r\nContent-Range: bytes %s-%s/%s\r\nAccept-Ranges: bytes\r\nLast-Modified: %s\r\nETag: %s\r\n\r\n"), 
			file->version.major, file->version.minor,
			qse_httpd_getname (httpd),
			qse_httpd_fmtgmtimetobb (httpd, QSE_NULL, 0),
			(file->keepalive? QSE_MT("keep-alive"): QSE_MT("close")),
			(file->u.get.mime.len > 0? QSE_MT("Content-Type: "): QSE_MT("")),
			(file->u.get.mime.len > 0? file->u.get.mime.ptr: QSE_MT("")),
			(file->u.get.mime.len > 0? QSE_MT("\r\n"): QSE_MT("")),
			((file->method == QSE_HTTP_HEAD)? QSE_MT("0"): tmp[0]),
			tmp[1], tmp[2], tmp[3],
			qse_httpd_fmtgmtimetobb (httpd, &st.mtime, 1),
			etag
		);
		if (x)
		{
			if (file->method == QSE_HTTP_HEAD) goto no_file_send;
			x = entask_getfseg (
				httpd, client, x, handle, 
				file->u.get.range.from, 
				(file->u.get.range.to - file->u.get.range.from + 1)
			);
		}
	}
	else
	{
		qse_mchar_t b_fsize[64];
		qse_mchar_t etag[ETAG_LEN_MAX + 1];
		qse_size_t etag_len;

		etag_len = qse_fmtuintmaxtombs (&etag[0], QSE_COUNTOF(etag), st.mtime.sec, 16, -1, QSE_MT('\0'), QSE_NULL);
		etag[etag_len++] = QSE_MT('-');
		etag_len += qse_fmtuintmaxtombs (&etag[etag_len], QSE_COUNTOF(etag), st.mtime.nsec, 16, -1, QSE_MT('\0'), QSE_NULL);
		etag[etag_len++] = QSE_MT('-');
		etag_len += qse_fmtuintmaxtombs (&etag[etag_len], QSE_COUNTOF(etag) - etag_len, st.size, 16, -1, QSE_MT('\0'), QSE_NULL);
		etag[etag_len++] = QSE_MT('-');
		etag_len += qse_fmtuintmaxtombs (&etag[etag_len], QSE_COUNTOF(etag) - etag_len, st.ino, 16, -1, QSE_MT('\0'), QSE_NULL);
		etag[etag_len++] = QSE_MT('-');
		etag_len += qse_fmtuintmaxtombs (&etag[etag_len], QSE_COUNTOF(etag) - etag_len, st.dev, 16, -1, QSE_MT('\0'), QSE_NULL);

		if ((file->u.get.if_none_match[0] != QSE_MT('\0') && qse_mbscmp (etag, file->u.get.if_none_match) == 0) ||
		    (file->u.get.if_modified_since.sec > 0 && st.mtime.sec <= file->u.get.if_modified_since.sec)) 
		{
			/* i've converted milliseconds to seconds before timestamp comparison
			 * because st.mtime has the actual milliseconds less than 1 second
			 * while if_modified_since doesn't have such small milliseconds */
			x = qse_httpd_entask_nomod (httpd, client, x, file->method, &file->version, file->keepalive);
			goto no_file_send;
		}

		qse_fmtuintmaxtombs (b_fsize, QSE_COUNTOF(b_fsize), st.size, 10, -1, QSE_MT('\0'), QSE_NULL);

		/* wget 1.8.2 set 'Connection: keep-alive' in the http 1.0 header.
		 * if the reply doesn't contain 'Connection: keep-alive', it didn't
		 * close connection.*/

		x = qse_httpd_entaskformat (
			httpd, client, x,
    			QSE_MT("HTTP/%d.%d 200 OK\r\nServer: %s\r\nDate: %s\r\nConnection: %s\r\n%s%s%sContent-Length: %s\r\nAccept-Ranges: bytes\r\nLast-Modified: %s\r\nETag: %s\r\n\r\n"), 
			file->version.major, file->version.minor,
			qse_httpd_getname (httpd),
			qse_httpd_fmtgmtimetobb (httpd, QSE_NULL, 0),
			(file->keepalive? QSE_MT("keep-alive"): QSE_MT("close")),
			(file->u.get.mime.len > 0? QSE_MT("Content-Type: "): QSE_MT("")),
			(file->u.get.mime.len > 0? file->u.get.mime.ptr: QSE_MT("")),
			(file->u.get.mime.len > 0? QSE_MT("\r\n"): QSE_MT("")),
			((file->method == QSE_HTTP_HEAD)? QSE_MT("0"): b_fsize),
			qse_httpd_fmtgmtimetobb (httpd, &st.mtime, 1),
			etag
		);
		if (x) 
		{
			if (file->method == QSE_HTTP_HEAD) goto no_file_send;
			x = entask_getfseg (httpd, client, x, handle, 0, st.size);
		}
	}

	if (x) return 0;
	httpd->opt.scb.file.close (httpd, handle);
	return -1;

no_file_send:
	if (fileopen) httpd->opt.scb.file.close (httpd, handle);
	return (x == QSE_NULL)? -1: 0;
}

/*------------------------------------------------------------------------*/

static int write_file (qse_httpd_t* httpd, qse_httpd_hnd_t handle, const qse_mchar_t* ptr, qse_size_t len)
{
	qse_ssize_t n;
	qse_size_t pos = 0;

	/* this implementation assumes that file writing will never get 
	 * blocked in practice. so no i/o multiplexing is performed over
	 * file descriptors */
	while (pos < len)
	{
		n = httpd->opt.scb.file.write (httpd, handle, &ptr[pos], len - pos);
		if (n <= 0) return -1;
		pos += n;
	}

	return 0;
}

static int putfile_snatch_client_input (
	qse_htre_t* req, const qse_mchar_t* ptr, qse_size_t len, void* ctx)
{
	qse_httpd_task_t* task;
	task_file_t* file; 

	task = (qse_httpd_task_t*)ctx;
	file = (task_file_t*)task->ctx;

	if (ptr == QSE_NULL)
	{
		/*
		 * this callback is called with ptr of QSE_NULL 
		 * when the request is completed or discarded. 
		 * and this indicates that there's nothing more to read
		 * from the client side. this can happen when the end of
		 * a request is seen or when an error occurs 
		 */
		QSE_ASSERT (len == 0);

		/* mark the there's nothing to read form the client side */
		qse_htre_unsetconcb (file->u.put.req);
		file->u.put.req = QSE_NULL; 

		/* since there is no more to read from the client side.
		 * the trigger is not needed any more. */
		task->trigger.v[0].mask = 0;
	}
	else if (!(file->u.put.flags & PUTFILE_WRITE_FAILED))
	{
		if (write_file (file->u.put.httpd, file->u.put.handle, ptr, len) <= -1)
		{
			file->u.put.flags |= PUTFILE_WRITE_FAILED;
			file->u.put.status = 500;
		}
	}

	return 0;
}

static int task_init_putfile (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_file_t* file = qse_httpd_gettaskxtn (httpd, task);
	task_file_t* arg = (task_file_t*)task->ctx;
	qse_httpd_stat_t st;

	/* zero out the task's extension area */
	QSE_MEMCPY (file, arg, QSE_SIZEOF(*file));
	file->u.put.req = QSE_NULL;
	
	/* copy in the path name to the area */
	file->path.ptr = (qse_mchar_t*)(file + 1);
	qse_mbscpy ((qse_mchar_t*)file->path.ptr, arg->path.ptr);
	file->u.put.status = 204; /* 200 should also be ok to indicate success. */
	task->ctx = file; /* switch the task context to the extension area */

	httpd->errnum = QSE_HTTPD_ENOERR;
	if (httpd->opt.scb.file.stat (httpd, file->path.ptr, &st) <= -1)
	{
		if (httpd->errnum == QSE_HTTPD_ENOENT) 
		{
			/* stat found no such file. so if the request is achived
			 * successfully, it should send '201 Created' */
			file->u.put.status = 201;
		}
	}

	httpd->errnum = QSE_HTTPD_ENOERR;
	if (httpd->opt.scb.file.wopen (httpd, file->path.ptr, &file->u.put.handle) <= -1) 
	{
		file->u.put.status = (httpd->errnum == QSE_HTTPD_EACCES)? 403: 500;
		goto oops;
	}

	if (write_file (httpd, file->u.put.handle, qse_htre_getcontentptr(arg->u.put.req), qse_htre_getcontentlen(arg->u.put.req)) <= -1) 
	{
		httpd->opt.scb.file.close (httpd, file->u.put.handle);
		file->u.put.status = 500;
		goto oops;
	}

	if (!(arg->u.put.req->state & QSE_HTRE_DISCARDED) &&
	    !(arg->u.put.req->state & QSE_HTRE_COMPLETED))
	{
		/* set up a callback to be called when the request content
		 * is fed to the htrd reader. qse_htre_addcontent() that 
		 * htrd calls invokes this callback. */
		file->u.put.req = arg->u.put.req;
		qse_htre_setconcb (file->u.put.req, putfile_snatch_client_input, task);
	}

	/* no triggers yet since the main loop doesn't allow me to set 
	 * triggers in the task initializer. however the main task handler
	 * will be invoked so long as the client handle is writable by
	 * the main loop. */
	return 0;

oops:
	/* since a new task can't be added in the initializer,
	 * i mark that initialization failed and let task_main_putfile()
	 * add an error task */
	qse_htre_discardcontent (arg->u.put.req);
	file->u.put.flags |= PUTFILE_INIT_FAILED;
	return 0;
}

static void task_fini_putfile (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_file_t* file = (task_file_t*)task->ctx;

	if (!(file->u.put.flags & PUTFILE_INIT_FAILED))
		httpd->opt.scb.file.close (httpd, file->u.put.handle);
}

static int task_main_putfile_2 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_file_t* file = (task_file_t*)task->ctx;

	if (file->u.put.req)
	{
		/* file->u.put.req is set to a non-NULL value if snatching
		 * is needed in the task_init_putfile(). and it's reset to
		 * QSE_NULL when snatching is over in putfile_snatch_client_input().
		 * i set a trigger so that the task is executed
		 * while there is input from the client side  */
		/*task->trigger.v[0].mask = QSE_HTTPD_TASK_TRIGGER_READ;
		task->trigger.v[0].handle = client->handle;*/
		task->trigger.cmask = QSE_HTTPD_TASK_TRIGGER_READ;
		return 1; /* trigger me when a client sends data  */
	}

	/* snatching is over. writing error may have occurred as well.
	 * file->u.put.status should hold a proper status code */
	if (qse_httpd_entask_error (
		httpd, client, task, file->u.put.status,
		file->method, &file->version, file->keepalive) == QSE_NULL) return -1;
	return 0; /* no more data to read. task over */
}

static int task_main_putfile (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_file_t* file = (task_file_t*)task->ctx;

	if (!(file->u.put.flags & PUTFILE_INIT_FAILED) && file->u.put.req)
	{
		/* initialization was successful and snatching is required. 
		 * switch to the next phase. */
		task->main = task_main_putfile_2;
		return task_main_putfile_2 (httpd, client, task);
	}

	/* snatching is not required or initialization error has occurred.
	 * file->u.put.status should hold a proper status code.
	 *
	 * note: if initialization error occurred and there is contents for the
	 * client to send, this reply may get to the client before it finishes 
	 * sending the contents. */
	if (qse_httpd_entask_error (
		httpd, client, task, file->u.put.status,
		file->method, &file->version, file->keepalive) == QSE_NULL) return -1;
	return 0; /* task over */
}

/*------------------------------------------------------------------------*/

qse_httpd_task_t* qse_httpd_entaskfile (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client, 
	qse_httpd_task_t* pred,
	const qse_mchar_t* path,
	const qse_mchar_t* mime,
	qse_htre_t* req)
{
	qse_httpd_task_t task;
	task_file_t data;
	const qse_htre_hdrval_t* tmp;
	qse_size_t xtnsize;

	QSE_MEMSET (&data, 0, QSE_SIZEOF(data));
	data.path.ptr = (qse_mchar_t*)path;
	data.path.len = qse_mbslen(path);
	data.version = *qse_htre_getversion(req);
	data.keepalive = (req->flags & QSE_HTRE_ATTR_KEEPALIVE);
	data.method = qse_htre_getqmethodtype(req);

	xtnsize = QSE_SIZEOF(task_file_t) + data.path.len + 1;

	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));

	switch (data.method)
	{
		case QSE_HTTP_OPTIONS:
			qse_htre_discardcontent (req);
			return qse_httpd_entaskallow (httpd, client, pred, 
				QSE_MT("OPTIONS,GET,HEAD,POST,PUT,DELETE"), req);

		case QSE_HTTP_HEAD:
		case QSE_HTTP_GET:
		case QSE_HTTP_POST:
			qse_htre_discardcontent (req);

			if (mime)
			{
				data.u.get.mime.ptr = (qse_mchar_t*)mime;
				data.u.get.mime.len = qse_mbslen(mime);
				xtnsize += data.u.get.mime.len + 1;
			}

			tmp = qse_htre_getheaderval(req, QSE_MT("If-None-Match"));
			if (tmp)
			{
				/*while (tmp->next) tmp = tmp->next;*/ /* get the last value */
				qse_mbsxcpy (data.u.get.if_none_match, QSE_COUNTOF(data.u.get.if_none_match), tmp->ptr);
			}
			if (data.u.get.if_none_match[0] == QSE_MT('\0'))
			{
				/* Both ETag and Last-Modified are included in the reply.
				 * If the client understand ETag, it can choose to include 
				 * If-None-Match in the request. If it understands Last-Modified,
				 * it can choose to include If-Modified-Since. I don't care
				 * the client understands both and include both of them
				 * in the request.
				 *
				 * I check If-None-Match if it's included.
				 * I check If-Modified-Since if If-None-Match is not included.
				 */
				tmp = qse_htre_getheaderval(req, QSE_MT("If-Modified-Since"));
				if (tmp)
				{
					/*while (tmp->next) tmp = tmp->next;*/ /* get the last value */
					if (qse_parsehttptime (tmp->ptr, &data.u.get.if_modified_since) <= -1)
					{
						data.u.get.if_modified_since.sec = 0;
						data.u.get.if_modified_since.nsec = 0;
					}
				}
			}

			tmp = qse_htre_getheaderval(req, QSE_MT("Range"));
			if (tmp) 
			{
				/*while (tmp->next) tmp = tmp->next;*/ /* get the last value */
				if (qse_parsehttprange (tmp->ptr, &data.u.get.range) <= -1)
				{
					return qse_httpd_entaskerror (httpd, client, pred, 416, req);
				}
			}
			else 
			{
				data.u.get.range.type = QSE_HTTP_RANGE_NONE;
			}

			task.init = task_init_getfile;
			task.main = task_main_getfile;

			task.ctx = &data;
			return qse_httpd_entask (httpd, client, pred, &task, xtnsize);

		case QSE_HTTP_PUT:
			/* note that no partial update is supported for PUT */
			data.u.put.httpd = httpd;
			data.u.put.req = req;
			task.init = task_init_putfile;
			task.main = task_main_putfile;
			task.fini = task_fini_putfile;
			task.ctx = &data;
			return qse_httpd_entask (httpd, client, pred, &task, xtnsize);

		case QSE_HTTP_DELETE:
		{
			int status = 200;

			qse_htre_discardcontent (req);
			if (httpd->opt.scb.file.purge (httpd, path) <= -1)
			{
				status = (httpd->errnum == QSE_HTTPD_ENOENT)? 404:
				         (httpd->errnum == QSE_HTTPD_EACCES)? 403: 500;
			}
				
			return qse_httpd_entaskerror (httpd, client, pred, status, req);
		}

		default:
			/* Method not allowed */
			qse_htre_discardcontent (req);
			return qse_httpd_entaskerror (httpd, client, pred, 405, req);
	}
}

