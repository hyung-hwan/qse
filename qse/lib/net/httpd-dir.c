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
#include <qse/cmn/stdio.h> /* TODO: remove this */

typedef struct task_dir_t task_dir_t;
struct task_dir_t
{
	const qse_mchar_t* path;
	qse_http_version_t version;
	int                keepalive;
};

typedef struct task_dseg_t task_dseg_t;
struct task_dseg_t
{
	const qse_mchar_t* path;
	qse_dir_t* handle;
	qse_dirent_t* dent;

	int header_added;
	int footer_pending;

	/*qse_mchar_t buf[4096];*/
	qse_mchar_t buf[512]; /* TOOD: increate size */
	qse_size_t  bufpos;
	qse_size_t  buflen;
	qse_size_t  bufrem;
	qse_size_t  chunklen;
};

static int task_init_dseg (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_dseg_t* xtn = qse_httpd_gettaskxtn (httpd, task);

	QSE_MEMCPY (xtn, task->ctx, QSE_SIZEOF(*xtn));
	qse_mbscpy ((qse_mchar_t*)(xtn + 1), xtn->path);
	xtn->path = (qse_mchar_t*)(xtn + 1);
	task->ctx = xtn;

	return 0;
}

static void task_fini_dseg (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_dseg_t* ctx = (task_dseg_t*)task->ctx;
	QSE_CLOSEDIR (ctx->handle);
}

static int task_main_dseg_chunked (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_dseg_t* ctx = (task_dseg_t*)task->ctx;
	qse_ssize_t n;
	int x;

	if (ctx->bufpos < ctx->buflen) goto send_dirlist;

	/* the buffer size is fixed to QSE_COUNTOF(ctx->buf).
	 * the number of digits need to hold the the size converted to
	 * a hexadecimal notation is roughly (log16(QSE_COUNTOF(ctx->buf) + 1).
	 * it should be safter to use ceil(log16(QSE_COUNTOF(ctx->buf)) + 1
	 * for precision issues. 
	 *
	 *  16**X = QSE_COUNTOF(ctx->buf). 
	 *  X = log16(QSE_COUNTOF(ctx->buf).
	 *  X + 1 is a required number of digits.
	 *
	 * Since log16 is not provided, we should use a natural log function
	 * whose base is the constant e (2.718).
	 * 
	 *  log16(n) = log(n) / log(16)
	 *
	 * The final fomula is here.
	 *
	 *  X = ceil((log(QSE_COUNTOF(ctx->buf)) / log(16))) + 1;
	 *  
 	 * However, i won't use these floating-point opertions.
	 * instead i'll reserve a hardcoded size. so when you change
	 * the size of the buffer arrray, you should check this size. 
	 */
	
#define SIZE_CHLEN     4
#define SIZE_CHLENCRLF 2
#define SIZE_CHENDCRLF 2

	/* reserve space to fill with the chunk length
	 * 4 for the actual chunk length and +2 for \r\n */
	ctx->buflen = SIZE_CHLEN + SIZE_CHLENCRLF; 

	/* free space remaing in the buffer for the chunk data */
	ctx->bufrem = QSE_COUNTOF(ctx->buf) - ctx->buflen - SIZE_CHENDCRLF; 

	if (ctx->footer_pending)
	{
		x = snprintf (
			&ctx->buf[ctx->buflen], 
			ctx->bufrem,
			QSE_MT("</ul></body></html>\r\n0\r\n"));
		if (x == -1 || x >= ctx->bufrem) 
		{
			/* return an error if the buffer is too small to hold the 
			 * trailing footer. you need to increate the buffer size */
			return -1;
		}

		ctx->buflen += x;
		ctx->chunklen = ctx->buflen - 5; /* -5 for \r\n0\r\n added above */

		/* CHENDCRLF */
		ctx->buf[ctx->buflen++] = QSE_MT('\r');
		ctx->buf[ctx->buflen++] = QSE_MT('\n');

		goto set_chunklen;
	}

	if (!ctx->header_added)
	{
		/* compose the header since this is the first time. */

		x = snprintf (
			&ctx->buf[ctx->buflen], 
			ctx->bufrem,
			QSE_MT("<html><head><title>Directory Listing</title></head><body><b>%s</b><ul><li><a href='../'>..</a></li>"),
			ctx->path
		);
		if (x == -1 || x >= ctx->bufrem) 
		{
			/* return an error if the buffer is too small to hold the header.
			 * you need to increate the buffer size */
			return -1;
		}

		ctx->buflen += x;
		ctx->bufrem -= x;

		ctx->header_added = 1;
	}

	if (!ctx->dent)
		ctx->dent = QSE_READDIR (ctx->handle);

	do
	{
		if (!ctx->dent)
		{
			// TODO: check if errno has changed from before QSE_READDIR().
			//       and return -1 if so.
			x = snprintf (
				&ctx->buf[ctx->buflen], 
				ctx->bufrem,
				QSE_MT("</ul></body></html>\r\n0\r\n"));
			if (x == -1 || x >= ctx->bufrem) 
			{
				ctx->footer_pending = 1;
				ctx->chunklen = ctx->buflen;

				/* CHENDCRLF */
				ctx->buf[ctx->buflen++] = QSE_MT('\r');
				ctx->buf[ctx->buflen++] = QSE_MT('\n');
			}
			else
			{
				ctx->buflen += x;
				ctx->chunklen = ctx->buflen - 5;

				/* CHENDCRLF */
				ctx->buf[ctx->buflen++] = QSE_MT('\r');
				ctx->buf[ctx->buflen++] = QSE_MT('\n');
			}
			break;	
		}
		else if (qse_mbscmp (ctx->dent->d_name, QSE_MT(".")) != 0 &&
		         qse_mbscmp (ctx->dent->d_name, QSE_MT("..")) != 0)
		{
			x = snprintf (
				&ctx->buf[ctx->buflen], 
				ctx->bufrem,
				QSE_MT("<li><a href='%s%s'>%s%s</a></li>"),
				ctx->dent->d_name,
				(ctx->dent->d_type == DT_DIR? QSE_MT("/"): QSE_MT("")),
				ctx->dent->d_name,
				(ctx->dent->d_type == DT_DIR? QSE_MT("/"): QSE_MT(""))
			);
			if (x == -1 || x >= ctx->bufrem)
			{
				/* buffer not large enough to hold this entry */
				ctx->chunklen = ctx->buflen;

				/* CHENDCRLF */
				ctx->buf[ctx->buflen++] = QSE_MT('\r');
				ctx->buf[ctx->buflen++] = QSE_MT('\n');
				break;
			}
			else
			{
				ctx->buflen += x;
				ctx->bufrem -= x;
			}
		}

		ctx->dent = QSE_READDIR (ctx->handle);
	}
	while (1);

set_chunklen:
	/* right alignment with space padding on the left */
/* TODO: change snprintf to qse_fmtuintmaxtombs() */
	x = snprintf (
		ctx->buf, (SIZE_CHLEN + SIZE_CHLENCRLF) - 1, 
		QSE_MT("%*lX"), (int)(SIZE_CHLEN + SIZE_CHLENCRLF - 2), 
		(unsigned long)(ctx->chunklen - (SIZE_CHLEN + SIZE_CHLENCRLF)));

	/* CHLENCRLF */
	ctx->buf[x] = QSE_MT('\r');
	ctx->buf[x+1] = QSE_MT('\n');

	/* skip leading space padding */
	for (x = 0; ctx->buf[x] == QSE_MT(' '); x++) ctx->buflen--;
	ctx->bufpos = x;

send_dirlist:
	httpd->errnum = QSE_HTTPD_ENOERR;
	n = httpd->cbs->client.send (
		httpd, client, &ctx->buf[ctx->bufpos], ctx->buflen);
	if (n <= -1) return -1;

	/* NOTE if (n == 0), it will enter an infinite loop */
		
	ctx->bufpos += n;
	ctx->buflen -= n;
	return (ctx->bufpos < ctx->buflen || ctx->footer_pending || ctx->dent)? 1: 0;
}

static int task_main_dseg_nochunk (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_dseg_t* ctx = (task_dseg_t*)task->ctx;
	qse_ssize_t n;
	int x;

	if (ctx->bufpos < ctx->buflen) goto send_dirlist;

	ctx->bufpos = 0;
	ctx->buflen = 0;
	ctx->bufrem = QSE_COUNTOF(ctx->buf);

	if (ctx->footer_pending)
	{
		x = snprintf (
			&ctx->buf[ctx->buflen], 
			ctx->bufrem,
			"</ul></body></html>");
		if (x == -1 || x >= ctx->bufrem) 
		{
			/* return an error if the buffer is too small to hold the 
			 * trailing footer. you need to increate the buffer size */
			return -1;
		}

		ctx->buflen += x;
		goto send_dirlist;
	}

	if (!ctx->header_added)
	{
		/* compose the header since this is the first time. */
		x = snprintf (
			&ctx->buf[ctx->buflen], 
			ctx->bufrem,
			QSE_MT("<html><head><title>Directory Listing</title></head><body><b>%s</b><ul><li><a href='../'>..</a></li>"),
			ctx->path
		);
		if (x == -1 || x >= ctx->bufrem) 
		{
			/* return an error if the buffer is too small to hold the header.
			 * you need to increate the buffer size */
			return -1;
		}

		ctx->buflen += x;
		ctx->bufrem -= x;
		ctx->header_added = 1;
	}

	if (ctx->dent == QSE_NULL) 
		ctx->dent = QSE_READDIR (ctx->handle);

	do
	{
		if (ctx->dent == QSE_NULL)
		{
			// TODO: check if errno has changed from before QSE_READDIR().
			//       and return -1 if so.
			x = snprintf (
				&ctx->buf[ctx->buflen], 
				ctx->bufrem,
				"</ul></body></html>");
			if (x == -1 || x >= ctx->bufrem) 
			{
				ctx->footer_pending = 1;
			}
			else
			{
				ctx->buflen += x;
			}
			break;	
		}
		else if (qse_mbscmp (ctx->dent->d_name, QSE_MT(".")) != 0 &&
		         qse_mbscmp (ctx->dent->d_name, QSE_MT("..")) != 0)
		{
			x = snprintf (
				&ctx->buf[ctx->buflen], 
				ctx->bufrem,
				"<li><a href='%s%s'>%s%s</a></li>", 
				ctx->dent->d_name,
				(ctx->dent->d_type == DT_DIR? "/": ""),
				ctx->dent->d_name,
				(ctx->dent->d_type == DT_DIR? "/": "")
			);
			if (x == -1 || x >= ctx->bufrem)
			{
				/* buffer not large enough to hold this entry */
				break;
			}
			else
			{
				ctx->buflen += x;
				ctx->bufrem -= x;
			}
		}

		ctx->dent = QSE_READDIR (ctx->handle);
	}
	while (1);

send_dirlist:
	httpd->errnum = QSE_HTTPD_ENOERR;
	n = httpd->cbs->client.send (
		httpd, client, &ctx->buf[ctx->bufpos], ctx->buflen);
	if (n <= -1) return -1;

	ctx->bufpos += n;
	ctx->buflen -= n;
	return (ctx->bufpos < ctx->buflen || ctx->footer_pending || ctx->dent)? 1: 0;
}

static qse_httpd_task_t* entask_directory_segment (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_httpd_task_t* pred, qse_dir_t* handle, const qse_mchar_t* path, int keepalive)
{
	qse_httpd_task_t task;
	task_dseg_t data;
	
	QSE_MEMSET (&data, 0, QSE_SIZEOF(data));
	data.handle = handle;
	data.path = path;

	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = task_init_dseg;
	task.main = (keepalive)? task_main_dseg_chunked: task_main_dseg_nochunk;
	task.fini = task_fini_dseg;
	task.ctx = &data;

qse_printf (QSE_T("Debug: entasking directory segment (%d)\n"), client->handle.i);
	return qse_httpd_entask (httpd, client, pred, &task, QSE_SIZEOF(data) + qse_mbslen(path) + 1);
}

/*------------------------------------------------------------------------*/

static int task_init_dir (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_dir_t* xtn = qse_httpd_gettaskxtn (httpd, task);

	/* deep-copy the context data to the extension area */
	QSE_MEMCPY (xtn, task->ctx, QSE_SIZEOF(*xtn));
	qse_mbscpy ((qse_mchar_t*)(xtn + 1), xtn->path);
	xtn->path = (qse_mchar_t*)(xtn + 1);

	/* switch the context to the extension area */
	task->ctx = xtn;

	return 0;
}

static QSE_INLINE int task_main_dir (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_dir_t* dir;
	qse_httpd_task_t* x;
	qse_dir_t* handle = QSE_NULL;

	dir = (task_dir_t*)task->ctx;
	x = task;

	if (qse_mbsend (dir->path, QSE_MT("/")))
	{
		handle = QSE_OPENDIR (dir->path);
		if (handle)
		{
			x = qse_httpd_entaskformat (
				httpd, client, x,
    				QSE_MT("HTTP/%d.%d 200 OK\r\nConnection: %s\r\nContent-Type: text/html\r\n%s\r\n"), 
				dir->version.major, dir->version.minor,
				(dir->keepalive? QSE_MT("keep-alive"): QSE_MT("close")),
				(dir->keepalive? QSE_MT("Transfer-Encoding: chunked\r\n"): QSE_MT(""))
			);
			if (x) x = entask_directory_segment (httpd, client, x, handle, dir->path, dir->keepalive);
			if (x) return 0;

			QSE_CLOSEDIR (handle);
			return -1;
		}
		else
		{
			int http_errnum;
			http_errnum = (errno == ENOENT)? 404:
					    (errno == EACCES)? 403: 500;
			x = qse_httpd_entask_error (
				httpd, client, x, http_errnum,
				&dir->version, dir->keepalive);
	
			QSE_CLOSEDIR (handle);
			return (x == QSE_NULL)? -1: 0;
		}
	}
	else
	{
		x = qse_httpd_entaskformat (
			httpd, client, x,
			QSE_MT("HTTP/%d.%d 301 Moved Permanently\r\nContent-Length: 0\r\nConnection: %s\r\nLocation: %s/\r\n\r\n"),
			dir->version.major, dir->version.minor,
			(dir->keepalive? QSE_MT("keep-alive"): QSE_MT("close")),
			dir->path
		);
		return (x == QSE_NULL)? -1: 0;
	}
}

qse_httpd_task_t* qse_httpd_entaskdir (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client, 
	qse_httpd_task_t* pred,
	const qse_mchar_t* path,
	qse_htre_t* req)
{
	qse_httpd_task_t task;
	task_dir_t data;

	QSE_MEMSET (&data, 0, QSE_SIZEOF(data));
	data.path = path;
	data.version = *qse_htre_getversion(req);
	data.keepalive = (req->attr.flags & QSE_HTRE_ATTR_KEEPALIVE);

	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = task_init_dir;
	task.main = task_main_dir;
	task.ctx = &data;

	return qse_httpd_entask (httpd, client, pred, &task,
		QSE_SIZEOF(task_dir_t) + qse_mbslen(path) + 1);
}

