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
#include <qse/cmn/path.h>
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
	qse_http_version_t version;
	int                keepalive;
	int                chunked;

	const qse_mchar_t* path;
	qse_dir_t* handle;
	qse_dirent_t* dent;

#define HEADER_ADDED   (1 << 0)
#define FOOTER_ADDED   (1 << 1)
#define FOOTER_PENDING (1 << 2)
#define DIRENT_PENDING (1 << 3)
	int state;

	qse_size_t  tcount; /* total directory entries */
	qse_size_t  dcount; /* the number of items in the buffer */

	qse_mchar_t buf[4096];
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

#define SIZE_CHLEN     4 /* the space size to hold the hexadecimal chunk length */
#define SIZE_CHLENCRLF 2 /* the space size to hold CRLF after the chunk length */
#define SIZE_CHENDCRLF 2 /* the sapce size to hold CRLF after the chunk data */

static QSE_INLINE void close_chunk_data (task_dseg_t* ctx, qse_size_t len)
{
	ctx->chunklen = len;

	/* CHENDCRLF - there is always space for these two. 
	 * reserved with SIZE_CHENDCRLF */
	ctx->buf[ctx->buflen++] = QSE_MT('\r');
	ctx->buf[ctx->buflen++] = QSE_MT('\n');
}

static void fill_chunk_length (task_dseg_t* ctx)
{
	int x;

	/* right alignment with space padding on the left */
/* TODO: change snprintf to qse_fmtuintmaxtombs() */
	x = snprintf (
		ctx->buf, (SIZE_CHLEN + SIZE_CHLENCRLF) - 1, 
		QSE_MT("%*lX"), (int)(SIZE_CHLEN + SIZE_CHLENCRLF - 2), 
		(unsigned long)(ctx->chunklen - (SIZE_CHLEN + SIZE_CHLENCRLF)));	

	/* i don't check the error of snprintf because i've secured the 
	 * suffient space for the chunk length at the beginning of the buffer */
	
	/* CHLENCRLF */
	ctx->buf[x] = QSE_MT('\r');
	ctx->buf[x+1] = QSE_MT('\n');

	/* skip leading space padding */
	QSE_ASSERT (ctx->bufpos == 0);
	while (ctx->buf[ctx->bufpos] == QSE_MT(' ')) ctx->bufpos++;
}

static int add_footer (task_dseg_t* ctx)
{
	int x;

	if (ctx->chunked)
	{
		x = snprintf (
			&ctx->buf[ctx->buflen], ctx->bufrem,
			QSE_MT("</ul>Total %lu entries</body></html>\r\n0\r\n"), (unsigned long)ctx->tcount);
	}
	else
	{
		x = snprintf (
			&ctx->buf[ctx->buflen], ctx->bufrem,
			QSE_MT("</ul>Total %lu entries</body></html>"), (unsigned long)ctx->tcount);
	}
	
	if (x == -1 || x >= ctx->bufrem) 
	{
		/* return an error if the buffer is too small to hold the 
		 * trailing footer. you need to increate the buffer size */
		return -1;
	}

	ctx->buflen += x;
	ctx->bufrem -= x;

	/* -5 for \r\n0\r\n added above */
	if (ctx->chunked) close_chunk_data (ctx, ctx->buflen - 5);

	return 0;
}

static int task_main_dseg (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_dseg_t* ctx = (task_dseg_t*)task->ctx;
	qse_ssize_t n;
	int x;

	if (ctx->bufpos < ctx->buflen) 
	{
		/* buffer contents not fully sent yet */
		goto send_dirlist;
	}

	/* the buffer size is fixed to QSE_COUNTOF(ctx->buf).
	 * 
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
	
	/* initialize buffer */
	ctx->dcount = 0; /* reset the entry counter */
	ctx->bufpos = 0;
	if (ctx->chunked)
	{
		/* reserve space to fill with the chunk length
		 * 4 for the actual chunk length and +2 for \r\n */
		ctx->buflen = SIZE_CHLEN + SIZE_CHLENCRLF; 
		/* free space remaing in the buffer for the chunk data */
		ctx->bufrem = QSE_COUNTOF(ctx->buf) - ctx->buflen - SIZE_CHENDCRLF; 
	}
	else
	{
		ctx->buflen = 0;
		ctx->bufrem = QSE_COUNTOF(ctx->buf);
	}

	if (ctx->state & FOOTER_PENDING)
	{
		/* only footers yet to be sent */
		if (add_footer (ctx) <= -1)
		{
			/* return an error if the buffer is too small to hold the 
			 * trailing footer. you need to increate the buffer size */
			httpd->errnum = QSE_HTTPD_EINTERN;
			return -1;
		}

		ctx->state &= ~FOOTER_PENDING;
		ctx->state |= FOOTER_ADDED;

		if (ctx->chunked) fill_chunk_length (ctx);
		goto send_dirlist;
	}

	if (!(ctx->state & HEADER_ADDED))
	{
		int is_root;

		is_root = (qse_mbscmp (ctx->path, QSE_MT("/")) == 0);

		/* compose the header since this is the first time. */
/* TODO: page encoding?? utf-8??? or derive name from cmgr or current locale??? */
		x = snprintf (
			&ctx->buf[ctx->buflen], ctx->bufrem,
			QSE_MT("<html><head></head><body><b>%s</b><ul>%s"), 
			ctx->path, (is_root? QSE_MT(""): QSE_MT("<li><a href='../'>..</a></li>"))
		);
		if (x == -1 || x >= ctx->bufrem) 
		{
			/* return an error if the buffer is too small to hold the header.
			 * you need to increate the buffer size. or i have make the buffer 
			 * dynamic. */
			httpd->errnum = QSE_HTTPD_EINTERN;
			return -1;
		}

		ctx->buflen += x;
		ctx->bufrem -= x;

		ctx->state |= HEADER_ADDED;
		ctx->dcount++;  
	}

	/*if (!ctx->dent) ctx->dent = QSE_READDIR (ctx->handle); */
	if (ctx->state & DIRENT_PENDING) 
		ctx->state &= ~DIRENT_PENDING;
	else 
		ctx->dent = QSE_READDIR (ctx->handle);

	do
	{
		if (!ctx->dent)
		{
			/* TODO: check if errno has changed from before QSE_READDIR().
			         and return -1 if so. */ 
			if (add_footer (ctx) <= -1) 
			{
				/* failed to add the footer part */
				if (ctx->chunked)
				{
					close_chunk_data (ctx, ctx->buflen);
					fill_chunk_length (ctx);
				}
				ctx->state |= FOOTER_PENDING;
			}
			else if (ctx->chunked) fill_chunk_length (ctx);
			break;	
		}
		else if (qse_mbscmp (ctx->dent->d_name, QSE_MT(".")) != 0 &&
		         qse_mbscmp (ctx->dent->d_name, QSE_MT("..")) != 0)
		{
			qse_mchar_t* encname;

			/* TODO: better buffer management in case there are 
			 *       a lot of file names to escape. */
			encname = qse_perenchttpstrdup (ctx->dent->d_name, httpd->mmgr);
			if (encname == QSE_NULL)
			{
				httpd->errnum = QSE_HTTPD_ENOMEM;
				return -1;
			}

			x = snprintf (
				&ctx->buf[ctx->buflen], 
				ctx->bufrem,
				QSE_MT("<li><a href='%s%s'>%s%s</a></li>"),
				encname,
				(ctx->dent->d_type == DT_DIR? QSE_MT("/"): QSE_MT("")),
				ctx->dent->d_name,
				(ctx->dent->d_type == DT_DIR? QSE_MT("/"): QSE_MT(""))
			);

			if (encname != ctx->dent->d_name) QSE_MMGR_FREE (httpd->mmgr, encname);

			if (x == -1 || x >= ctx->bufrem)
			{
				/* buffer not large enough to hold this entry */
				if (ctx->dcount <= 0) 
				{
					/* neither directory entry nor the header 
					 * has been added to the buffer so far. and 
					 * this attempt has failed. the buffer size must 
					 * be too small. you must increase it */
					httpd->errnum = QSE_HTTPD_EINTERN;
					return -1;
				}

				if (ctx->chunked)
				{
					close_chunk_data (ctx, ctx->buflen);
					fill_chunk_length (ctx);
				}

				ctx->state |= DIRENT_PENDING;
				break;
			}
			else
			{
				ctx->buflen += x;
				ctx->bufrem -= x;
				ctx->dcount++;
				ctx->tcount++;
			}
		}

		ctx->dent = QSE_READDIR (ctx->handle);
	}
	while (1);


send_dirlist:
	httpd->errnum = QSE_HTTPD_ENOERR;
	n = httpd->scb->client.send (
		httpd, client, &ctx->buf[ctx->bufpos], ctx->buflen - ctx->bufpos);
	if (n <= -1) return -1;

	/* NOTE if (n == 0), it will enter an infinite loop */
		
	ctx->bufpos += n;
	return (ctx->bufpos < ctx->buflen || (ctx->state & FOOTER_PENDING) || ctx->dent)? 1: 0;
}

static qse_httpd_task_t* entask_directory_segment (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_httpd_task_t* pred, qse_dir_t* handle, task_dir_t* dir)
{
	qse_httpd_task_t task;
	task_dseg_t data;
	
	QSE_MEMSET (&data, 0, QSE_SIZEOF(data));
	data.handle = handle;
	data.version = dir->version;
	data.keepalive = dir->keepalive;
	data.chunked = dir->keepalive;
	data.path = dir->path;

	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = task_init_dseg;
	task.main = task_main_dseg;
	task.fini = task_fini_dseg;
	task.ctx = &data;

qse_printf (QSE_T("Debug: entasking directory segment (%d)\n"), client->handle.i);
	return qse_httpd_entask (httpd, client, pred, &task, QSE_SIZEOF(data) + qse_mbslen(data.path) + 1);
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
    				QSE_MT("HTTP/%d.%d 200 OK\r\nServer: %s\r\nDate: %s\r\nConnection: %s\r\nContent-Type: text/html\r\n%s\r\n"), 
				dir->version.major, dir->version.minor,
				qse_httpd_getname (httpd),
				qse_httpd_fmtgmtimetobb (httpd, QSE_NULL, 0),
				(dir->keepalive? QSE_MT("keep-alive"): QSE_MT("close")),
				(dir->keepalive? QSE_MT("Transfer-Encoding: chunked\r\n"): QSE_MT(""))
			);
			if (x) x = entask_directory_segment (httpd, client, x, handle, dir);
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
			QSE_MT("HTTP/%d.%d 301 Moved Permanently\r\nServer: %s\r\nDate: %s\r\nContent-Length: 0\r\nConnection: %s\r\nLocation: %s/\r\n\r\n"),
			dir->version.major, dir->version.minor,
			qse_httpd_getname (httpd),
			qse_httpd_fmtgmtimetobb (httpd, QSE_NULL, 0),
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

