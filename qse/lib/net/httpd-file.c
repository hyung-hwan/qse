/*
 * $Id$
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
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
#include "../cmn/syscall.h"
#include <qse/cmn/str.h>
#include <qse/cmn/fmt.h>
#include <qse/cmn/path.h>
#include <qse/cmn/time.h>
#include <qse/cmn/stdio.h> /* TODO: remove this */

#define ETAG_LEN_MAX 127

typedef struct task_file_t task_file_t;
struct task_file_t
{
	qse_mcstr_t path;
	qse_mcstr_t mime;

	qse_http_range_t   range;
	qse_mchar_t        if_none_match[ETAG_LEN_MAX + 1];
	qse_ntime_t        if_modified_since;
	qse_http_version_t version;
	int                keepalive;
};

typedef struct task_fseg_t task_fseg_t;
struct task_fseg_t
{
	qse_ubi_t handle;
	qse_foff_t left;
	qse_foff_t offset;
};

static int task_init_fseg (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_fseg_t* xtn = qse_httpd_gettaskxtn (httpd, task);
	QSE_MEMCPY (xtn, task->ctx, QSE_SIZEOF(*xtn));
	task->ctx = xtn;
	return 0;
}

static void task_fini_fseg (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_fseg_t* ctx = (task_fseg_t*)task->ctx;
	httpd->scb->file.close (httpd, ctx->handle);
}

static int task_main_fseg (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	qse_ssize_t n;
	qse_size_t count;
	task_fseg_t* ctx = (task_fseg_t*)task->ctx;

	count = MAX_SEND_SIZE;
	if (count >= ctx->left) count = ctx->left;

/* TODO: more adjustment needed for OS with different sendfile semantics... */
	n = httpd->scb->client.sendfile (
		httpd, client, ctx->handle, &ctx->offset, count);
	if (n <= -1) 
	{
/* HANDLE EGAIN specially??? */
		return -1; /* TODO: any logging */
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

	return 1; /* more work to do */
}

static qse_httpd_task_t* entask_file_segment (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_httpd_task_t* pred,
	qse_ubi_t handle, qse_foff_t offset, qse_foff_t size)
{
	qse_httpd_task_t task;
	task_fseg_t data;
	
	QSE_MEMSET (&data, 0, QSE_SIZEOF(data));
	data.handle = handle;
	data.offset = offset;
	data.left = size;

	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = task_init_fseg;
	task.main = task_main_fseg;
	task.fini = task_fini_fseg;
	task.ctx = &data;

qse_printf (QSE_T("Debug: entasking file segment (%d)\n"), client->handle.i);
	return qse_httpd_entask (httpd, client, pred, &task, QSE_SIZEOF(data));
}

/*------------------------------------------------------------------------*/


static int task_init_file (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_file_t* file = qse_httpd_gettaskxtn (httpd, task);
	task_file_t* arg = (task_file_t*)task->ctx;

	QSE_MEMCPY (file, arg, QSE_SIZEOF(*file));

	file->path.ptr = (qse_mchar_t*)(file + 1);
     qse_mbscpy ((qse_mchar_t*)file->path.ptr, arg->path.ptr);
	if (arg->mime.ptr)
	{
		file->mime.ptr = file->path.ptr + file->path.len + 1;
		qse_mbscpy ((qse_mchar_t*)file->mime.ptr, arg->mime.ptr);
	}

	task->ctx = file;
	return 0;
}

static QSE_INLINE int task_main_file (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_file_t* file;
	qse_httpd_task_t* x;
	qse_ubi_t handle;
	int fileopen = 0;
	qse_httpd_stat_t st;

	file = (task_file_t*)task->ctx;
	x = task;

/* TODO: if you should deal with files on a network-mounted drive,
         setting a trigger or non-blocking I/O are needed. */

qse_printf (QSE_T("opening file %hs\n"), file->path);
		
	httpd->errnum = QSE_HTTPD_ENOERR;
	if (httpd->scb->file.stat (httpd, file->path.ptr, &st) <= -1)
	{
		int http_errnum;
		http_errnum = (httpd->errnum == QSE_HTTPD_ENOENT)? 404:
		              (httpd->errnum == QSE_HTTPD_EACCES)? 403: 500;
		x = qse_httpd_entask_error (
			httpd, client, x, http_errnum, 
			&file->version, file->keepalive);
		goto no_file_send;
	}

	httpd->errnum = QSE_HTTPD_ENOERR;
	if (httpd->scb->file.ropen (httpd, file->path.ptr, &handle) <= -1)
	{
		int http_errnum;
		http_errnum = (httpd->errnum == QSE_HTTPD_ENOENT)? 404:
		              (httpd->errnum == QSE_HTTPD_EACCES)? 403: 500;
		x = qse_httpd_entask_error (
			httpd, client, x, http_errnum, 
			&file->version, file->keepalive);
		goto no_file_send;
	}	
	fileopen = 1;

	if (file->range.type != QSE_HTTP_RANGE_NONE)
	{ 
		qse_mchar_t tmp[4][64];
		qse_mchar_t etag[ETAG_LEN_MAX + 1];
		qse_size_t etag_len;

		if (file->range.type == QSE_HTTP_RANGE_SUFFIX)
		{
			if (file->range.to > st.size) file->range.to = st.size;
			file->range.from = st.size - file->range.to;
			file->range.to = file->range.to + file->range.from;
			if (st.size > 0) file->range.to--;
		}

		if (file->range.from >= st.size)
		{
			x = qse_httpd_entask_error (
				httpd, client, x, 416, &file->version, file->keepalive);
			goto no_file_send;
		}

		if (file->range.to >= st.size) file->range.to = st.size - 1;

		qse_fmtuintmaxtombs (tmp[0], QSE_COUNTOF(tmp[0]), (file->range.to - file->range.from + 1), 10, -1, QSE_MT('\0'), QSE_NULL);
		qse_fmtuintmaxtombs (tmp[1], QSE_COUNTOF(tmp[1]), file->range.from, 10, -1, QSE_MT('\0'), QSE_NULL);
		qse_fmtuintmaxtombs (tmp[2], QSE_COUNTOF(tmp[2]), file->range.to, 10, -1, QSE_MT('\0'), QSE_NULL);
		qse_fmtuintmaxtombs (tmp[3], QSE_COUNTOF(tmp[3]), st.size, 10, -1, QSE_MT('\0'), QSE_NULL);

		etag_len = qse_fmtuintmaxtombs (&etag[0], QSE_COUNTOF(etag), st.mtime, 16, -1, QSE_MT('\0'), QSE_NULL);
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
			(file->mime.len > 0? QSE_MT("Content-Type: "): QSE_MT("")),
			(file->mime.len > 0? file->mime.ptr: QSE_MT("")),
			(file->mime.len > 0? QSE_MT("\r\n"): QSE_MT("")),
			tmp[0], tmp[1], tmp[2], tmp[3], etag
		);
		if (x)
		{
			x = entask_file_segment (
				httpd, client, x,
				handle, 
				file->range.from, 
				(file->range.to - file->range.from + 1)
			);
		}
	}
	else
	{
		qse_mchar_t b_fsize[64];
		qse_mchar_t etag[ETAG_LEN_MAX + 1];
		qse_size_t etag_len;

		etag_len = qse_fmtuintmaxtombs (&etag[0], QSE_COUNTOF(etag), st.mtime, 16, -1, QSE_MT('\0'), QSE_NULL);
		etag[etag_len++] = QSE_MT('-');
		etag_len += qse_fmtuintmaxtombs (&etag[etag_len], QSE_COUNTOF(etag) - etag_len, st.size, 16, -1, QSE_MT('\0'), QSE_NULL);
		etag[etag_len++] = QSE_MT('-');
		etag_len += qse_fmtuintmaxtombs (&etag[etag_len], QSE_COUNTOF(etag) - etag_len, st.ino, 16, -1, QSE_MT('\0'), QSE_NULL);
		etag[etag_len++] = QSE_MT('-');
		etag_len += qse_fmtuintmaxtombs (&etag[etag_len], QSE_COUNTOF(etag) - etag_len, st.dev, 16, -1, QSE_MT('\0'), QSE_NULL);

		if ((file->if_none_match[0] != QSE_MT('\0') && qse_mbscmp (etag, file->if_none_match) == 0) ||
		    (file->if_modified_since > 0 && QSE_MSEC_TO_SEC(st.mtime) <= QSE_MSEC_TO_SEC(file->if_modified_since))) 
		{
			/* i've converted milliseconds to seconds before timestamp comparison
			 * because st.mtime has the actual milliseconds less than 1 second
			 * while if_modified_since doesn't have such small milliseconds */

			x = qse_httpd_entaskformat (
				httpd, client, x,
				QSE_MT("HTTP/%d.%d 304 Not Modified\r\nServer: %s\r\nDate: %s\r\nConnection: %s\r\nContent-Length: 0\r\n\r\n"),
				file->version.major, file->version.minor,
				qse_httpd_getname (httpd),
				qse_httpd_fmtgmtimetobb (httpd, QSE_NULL, 0),
				(file->keepalive? QSE_MT("keep-alive"): QSE_MT("close"))
			);
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
			(file->mime.len > 0? QSE_MT("Content-Type: "): QSE_MT("")),
			(file->mime.len > 0? file->mime.ptr: QSE_MT("")),
			(file->mime.len > 0? QSE_MT("\r\n"): QSE_MT("")),
			b_fsize,
			qse_httpd_fmtgmtimetobb (httpd, &st.mtime, 1),
			etag
		);
		if (x) x = entask_file_segment (httpd, client, x, handle, 0, st.size);
	}

	if (x) return 0;
	httpd->scb->file.close (httpd, handle);
	return -1;

no_file_send:
	if (fileopen) httpd->scb->file.close (httpd, handle);
	return (x == QSE_NULL)? -1: 0;
}

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

	QSE_MEMSET (&data, 0, QSE_SIZEOF(data));
	data.path.ptr = path;
	data.path.len = qse_mbslen(path);
	if (mime)
	{
		data.mime.ptr = mime;
		data.mime.len = qse_mbslen(mime);
	}
	data.version = *qse_htre_getversion(req);
	data.keepalive = (req->attr.flags & QSE_HTRE_ATTR_KEEPALIVE);

	tmp = qse_htre_getheaderval(req, QSE_MT("Range"));
	if (tmp) 
	{
		while (tmp->next) tmp = tmp->next; /* get the last value */
		if (qse_parsehttprange (tmp->ptr, &data.range) <= -1)
		{
			return qse_httpd_entaskerror (httpd, client, pred, 416, req);
		}
	}
	else 
	{
		data.range.type = QSE_HTTP_RANGE_NONE;
	}

	tmp = qse_htre_getheaderval(req, QSE_MT("If-None-Match"));
	if (tmp)
	{
		while (tmp->next) tmp = tmp->next; /* get the last value */
		qse_mbsxcpy (data.if_none_match, QSE_COUNTOF(data.if_none_match), tmp->ptr);
	}
	if (data.if_none_match[0] == QSE_MT('\0'))
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
			while (tmp->next) tmp = tmp->next; /* get the last value */
			if (qse_parsehttptime (tmp->ptr, &data.if_modified_since) <= -1)
				data.if_modified_since = 0;
		}
	}
	
	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = task_init_file;
	task.main = task_main_file;
	task.ctx = &data;

	return qse_httpd_entask (httpd, client, pred, &task, 
		QSE_SIZEOF(task_file_t) + data.path.len + 1 + data.mime.len + 1);
}

