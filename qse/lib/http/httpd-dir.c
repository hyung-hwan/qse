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

#include <stdio.h> /* TODO: remove this */
#if defined(_MSC_VER) || defined(__BORLANDC__) || (defined(__WATCOMC__) && (__WATCOMC__ < 1200))
#	define snprintf _snprintf 
#endif

typedef struct task_dir_t task_dir_t;
struct task_dir_t
{
	qse_mcstr_t        path;
	qse_mcstr_t        qpath;
	qse_mcstr_t        head;
	qse_mcstr_t        foot;
	qse_http_version_t version;
	int                keepalive;
	int                method;
	qse_ubi_t          handle;
};

typedef struct task_dseg_t task_dseg_t;
struct task_dseg_t
{
	qse_http_version_t version;
	int                keepalive;
	int                chunked;
	
	qse_mcstr_t path;
	qse_mcstr_t qpath;
	qse_mcstr_t head;
	qse_mcstr_t foot;
	qse_ubi_t handle;
	qse_httpd_dirent_t dent;

#define HEADER_ADDED   (1 << 0)
#define FOOTER_ADDED   (1 << 1)
#define FOOTER_PENDING (1 << 2)
#define DIRENT_PENDING (1 << 3)
#define DIRENT_NOMORE  (1 << 4)
	int state;

	qse_size_t  tcount; /* total directory entries */
	qse_size_t  dcount; /* the number of items in the buffer */

	qse_mchar_t buf[4096*2]; /* is this large enough? */
	int         bufpos; 
	int         buflen;
	int         bufrem;
	qse_size_t  chunklen;
};

static int task_init_dseg (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_dseg_t* xtn = qse_httpd_gettaskxtn (httpd, task);
	task_dseg_t* arg = (task_dseg_t*)task->ctx;

	QSE_MEMCPY (xtn, arg, QSE_SIZEOF(*xtn));

	xtn->path.ptr = (qse_mchar_t*)(xtn + 1);
	qse_mbscpy ((qse_mchar_t*)xtn->path.ptr, arg->path.ptr);
	xtn->qpath.ptr = xtn->path.ptr + xtn->path.len + 1;
	qse_mbscpy ((qse_mchar_t*)xtn->qpath.ptr, arg->qpath.ptr);

	xtn->head.ptr = xtn->qpath.ptr + xtn->qpath.len + 1;
	qse_mbscpy ((qse_mchar_t*)xtn->head.ptr, arg->head.ptr);
	xtn->foot.ptr = xtn->head.ptr + xtn->head.len + 1;
	qse_mbscpy ((qse_mchar_t*)xtn->foot.ptr, arg->foot.ptr);

	task->ctx = xtn;

	return 0;
}

static void task_fini_dseg (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_dseg_t* ctx = (task_dseg_t*)task->ctx;
	httpd->opt.scb.dir.close (httpd, ctx->handle);
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
	x = qse_fmtuintmaxtombs (
		ctx->buf, (SIZE_CHLEN + SIZE_CHLENCRLF) - 1, 
		(ctx->chunklen - (SIZE_CHLEN + SIZE_CHLENCRLF)), /* value to convert */
		16 | QSE_FMTUINTMAXTOMBS_UPPERCASE, /* uppercase hexadecimal letters */
		-1, /* no particular precision - want space filled if less than 4 digits */
		QSE_MT(' '),  /* fill with space */
		QSE_NULL
	);
	/* i don't check the buffer size error because i've secured the 
	 * suffient space for the chunk length at the beginning of the buffer */
	
	/* CHLENCRLF */
	ctx->buf[x] = QSE_MT('\r');
	ctx->buf[x+1] = QSE_MT('\n');

	/* skip leading space padding */
	QSE_ASSERT (ctx->bufpos == 0);
	while (ctx->buf[ctx->bufpos] == QSE_MT(' ')) ctx->bufpos++;
}

static int format_dirent (
	qse_httpd_t* httpd, 
	qse_httpd_client_t* client, 
	const qse_httpd_dirent_t* dirent,
	qse_mchar_t* buf, int bufsz)
{
/* TODO: page encoding?? utf-8??? or derive name from cmgr or current locale??? */
	int n;

	qse_mchar_t* encname;
	qse_mchar_t* escname;
	qse_btime_t bt;
	qse_mchar_t tmbuf[32];
	qse_mchar_t fszbuf[64];

	/* TODO: better buffer management in case there are 
	 *       a lot of file names to escape. */

	/* perform percent-encoding for the anchor */
	encname = qse_perenchttpstrdup (dirent->name, httpd->mmgr);
	if (encname == QSE_NULL)
	{
		httpd->errnum = QSE_HTTPD_ENOMEM;
		return -1;
	}

	/* perform html escaping for the text */
	escname = qse_httpd_escapehtml (httpd, dirent->name);
	if (escname == QSE_NULL) 
	{
		if (encname != dirent->name) QSE_MMGR_FREE (httpd->mmgr, encname);
		return -1;
	}

	qse_localtime (&dirent->stat.mtime, &bt);
	snprintf (tmbuf, QSE_COUNTOF(tmbuf),
		QSE_MT("%04d-%02d-%02d %02d:%02d:%02d"),
        		bt.year + QSE_BTIME_YEAR_BASE, bt.mon + 1, bt.mday,
		bt.hour, bt.min, bt.sec);

	if (dirent->stat.isdir)
	{
		fszbuf[0] = QSE_MT('\0');
	}
	else
	{
		qse_fmtuintmaxtombs (
			fszbuf, QSE_COUNTOF(fszbuf),
			dirent->stat.size, 10, -1, QSE_MT('\0'), QSE_NULL
		);
	}

	n = snprintf (
		buf, bufsz,
		QSE_MT("<tr><td class='name'><a href='%s%s' class='%s'>%s%s</a></td><td class='time'>%s</td><td class='size'>%s</td></tr>\n"),
		encname,
		(dirent->stat.isdir? QSE_MT("/"): QSE_MT("")),
		(dirent->stat.isdir? QSE_MT("dir"): QSE_MT("file")),
		escname,
		(dirent->stat.isdir? QSE_MT("/"): QSE_MT("")),
		tmbuf, fszbuf
	);

	if (escname != dirent->name) qse_httpd_freemem (httpd, escname);
	if (encname != dirent->name) QSE_MMGR_FREE (httpd->mmgr, encname);

	if (n <= -1 || n >= bufsz)
	{
		httpd->errnum = QSE_HTTPD_ENOBUF;
		return -1;
	}

	return n;
}

static int add_footer (qse_httpd_t* httpd, qse_httpd_client_t* client, task_dseg_t* ctx)
{
	int x, rem;

	rem = ctx->chunked? (ctx->buflen - 5): ctx->buflen;
	if (rem < 1)
	{
		httpd->errnum = QSE_HTTPD_ENOBUF;
		return -1;
	}

	x = snprintf (&ctx->buf[ctx->buflen], rem, QSE_MT("</table></div>\n<div class='footer'>%s</div>\n</body></html>"), ctx->foot.ptr);
	if (x <= -1 || x >= rem) 
	{
		httpd->errnum = QSE_HTTPD_ENOBUF;
		return -1;
	}

	QSE_ASSERT (x < rem);

	ctx->buflen += x;
	ctx->bufrem -= x;

	if (ctx->chunked)
	{
		qse_mbscpy (&ctx->buf[ctx->buflen], QSE_MT("\r\n0\r\n"));
		ctx->buflen += 5;
		ctx->bufrem -= 5;

		/* -5 for \r\n0\r\n added above */
		if (ctx->chunked) close_chunk_data (ctx, ctx->buflen - 5);
	}

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
		if (add_footer (httpd, client, ctx) <= -1)
		{
			/* return an error if the buffer is too small to hold the 
			 * trailing footer. you need to increase the buffer size */
			return -1;
		}

		ctx->state &= ~FOOTER_PENDING;
		ctx->state |= FOOTER_ADDED;

		if (ctx->chunked) fill_chunk_length (ctx);
		goto send_dirlist;
	}

	if (!(ctx->state & HEADER_ADDED))
	{
		/* compose the header since this is the first time. */
		int is_root;
		qse_mchar_t* qpath_esc;

		is_root = (qse_mbscmp (ctx->qpath.ptr, QSE_MT("/")) == 0);

		qpath_esc = qse_httpd_escapehtml (httpd, ctx->qpath.ptr);
		if (qpath_esc == QSE_NULL) return -1;

		x = snprintf (&ctx->buf[ctx->buflen], ctx->bufrem,
			QSE_MT("<html><head>%s</head>\n<body>\n<div class='header'>%s</div>\n<div class='body'><table>%s"), ctx->head.ptr, qpath_esc,
			(is_root? QSE_MT(""): QSE_MT("<tr><td class='name'><a href='../' class='dir'>..</a></td><td class='time'></td><td class='size'></td></tr>\n"))
		);

		if (qpath_esc != ctx->qpath.ptr) qse_httpd_freemem (httpd, qpath_esc);

#if 0
		if (x <= -1)
#endif
		if (x <= -1 || x >= ctx->bufrem)
		{
			/* return an error if the buffer is too small to 
			 * hold the header(httpd->errnum == QSE_HTTPD_ENOBUF).
			 * i need to increase the buffer size. or i have make
			 * the buffer dynamic. */
			httpd->errnum = QSE_HTTPD_ENOBUF;
			return -1;
		}

		QSE_ASSERT (x < ctx->bufrem);

		ctx->buflen += x;
		ctx->bufrem -= x;

		ctx->state |= HEADER_ADDED;
		ctx->dcount++;  
	}

	if (ctx->state & DIRENT_PENDING) 
	{
		ctx->state &= ~DIRENT_PENDING;
	}
	else 
	{
		if (httpd->opt.scb.dir.read (httpd, ctx->handle, &ctx->dent) <= 0)
			ctx->state |= DIRENT_NOMORE;	
	}

	do
	{
		if (ctx->state & DIRENT_NOMORE)
		{
			/* no more directory entry */

			if (add_footer (httpd, client, ctx) <= -1) 
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


		if (qse_mbscmp (ctx->dent.name, QSE_MT(".")) != 0 &&
		    qse_mbscmp (ctx->dent.name, QSE_MT("..")) != 0)
		{
			x = format_dirent (httpd, client, &ctx->dent, &ctx->buf[ctx->buflen], ctx->bufrem); 
			if (x <= -1)
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
				QSE_ASSERT (x < ctx->bufrem);

				ctx->buflen += x;
				ctx->bufrem -= x;
				ctx->dcount++;
				ctx->tcount++;
			}
		}

		if (httpd->opt.scb.dir.read (httpd, ctx->handle, &ctx->dent) <= 0)
			ctx->state |= DIRENT_NOMORE;
	}
	while (1);


send_dirlist:
	httpd->errnum = QSE_HTTPD_ENOERR;
	n = httpd->opt.scb.client.send (
		httpd, client, &ctx->buf[ctx->bufpos], ctx->buflen - ctx->bufpos);
	if (n <= -1) return -1;

	/* NOTE if (n == 0), it will enter an infinite loop */
		
	ctx->bufpos += n;
	return (ctx->bufpos < ctx->buflen || (ctx->state & FOOTER_PENDING) || !(ctx->state & DIRENT_NOMORE))? 1: 0;
}

static qse_httpd_task_t* entask_directory_segment (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_httpd_task_t* pred, qse_ubi_t handle, task_dir_t* dir)
{
	qse_httpd_task_t task;
	task_dseg_t data;
	
	QSE_MEMSET (&data, 0, QSE_SIZEOF(data));
	data.handle = handle;
	data.version = dir->version;
	data.keepalive = dir->keepalive;
	data.chunked = dir->keepalive;
	data.path = dir->path;
	data.qpath = dir->qpath;
	data.head = dir->head;
	data.foot = dir->foot;

	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = task_init_dseg;
	task.main = task_main_dseg;
	task.fini = task_fini_dseg;
	task.ctx = &data;

	return qse_httpd_entask (httpd, client, pred, &task, QSE_SIZEOF(data) + data.path.len + 1 + data.qpath.len + 1 + data.head.len + 1 + data.foot.len + 1);
}

/*------------------------------------------------------------------------*/

static int task_init_getdir (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_dir_t* xtn = qse_httpd_gettaskxtn (httpd, task);
	task_dir_t* arg = (task_dir_t*)task->ctx;

	/* deep-copy the context data to the extension area */
	QSE_MEMCPY (xtn, arg, QSE_SIZEOF(*xtn));

	xtn->path.ptr = (qse_mchar_t*)(xtn + 1);
	qse_mbscpy ((qse_mchar_t*)xtn->path.ptr, arg->path.ptr);
	xtn->qpath.ptr = xtn->path.ptr + xtn->path.len + 1;
	qse_mbscpy ((qse_mchar_t*)xtn->qpath.ptr, arg->qpath.ptr);

	xtn->head.ptr = xtn->qpath.ptr + xtn->qpath.len + 1;
	qse_mbscpy ((qse_mchar_t*)xtn->head.ptr, arg->head.ptr);
	xtn->foot.ptr = xtn->head.ptr + xtn->head.len + 1;
	qse_mbscpy ((qse_mchar_t*)xtn->foot.ptr, arg->foot.ptr);

	/* switch the context to the extension area */
	task->ctx = xtn;
	return 0;
}

static QSE_INLINE int task_main_getdir (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_dir_t* dir;
	qse_httpd_task_t* x;

	dir = (task_dir_t*)task->ctx;
	x = task;

	/* arrange to return the header part first */
	if (dir->method == QSE_HTTP_HEAD)
	{
		x = qse_httpd_entaskformat (
			httpd, client, x,
    			QSE_MT("HTTP/%d.%d 200 OK\r\nServer: %s\r\nDate: %s\r\nConnection: %s\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n"), 
			dir->version.major, dir->version.minor,
			qse_httpd_getname (httpd),
			qse_httpd_fmtgmtimetobb (httpd, QSE_NULL, 0),
			(dir->keepalive? QSE_MT("keep-alive"): QSE_MT("close"))
		);

		httpd->opt.scb.dir.close (httpd, dir->handle);
		return (x == QSE_NULL)? -1: 0;
	}
	else
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
		if (x) 
		{
#if 0
			if (httpd->opt.trait & QSE_HTTPD_LOGACC)
			{
				qse_httpd_reqsum_t reqsum;

				acc.remote = remote;
				acc.qpath = qpath;
				acc.status = 200;
				acc.version =  ...;
				acc.method =  ...;

				httpd->opt.rcb.logacc (httpd, &reqsum);
			}
#endif

			/* arrange to send the actual directory contents */
			x = entask_directory_segment (httpd, client, x, dir->handle, dir);
			if (x) return 0;
		}

		httpd->opt.scb.dir.close (httpd, dir->handle);
		return -1;
	}
}

qse_httpd_task_t* qse_httpd_entaskdir (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client, 
	qse_httpd_task_t* pred,
	qse_httpd_rsrc_dir_t* dir,
	qse_htre_t* req)
{
	int method;

	method = qse_htre_getqmethodtype(req);

	/* i don't need contents for directories */
	qse_htre_discardcontent (req); 

	switch (method)
	{
		case QSE_HTTP_OPTIONS:
			return qse_httpd_entaskallow (httpd, client, pred, 
				QSE_MT("OPTIONS,GET,HEAD,POST,PUT,DELETE"), req);

		case QSE_HTTP_HEAD:
		case QSE_HTTP_GET:
		case QSE_HTTP_POST:
		{
			task_dir_t data;
			QSE_MEMSET (&data, 0, QSE_SIZEOF(data));

			/* check if the directory stream can be opened before
			 * creating an actual task. */
			if (httpd->opt.scb.dir.open (httpd, dir->path, &data.handle) <= -1)
			{
				/* arrange a status code to return */
				int status;
				status = (httpd->errnum == QSE_HTTPD_ENOENT)? 404:
					    (httpd->errnum == QSE_HTTPD_EACCES)? 403: 500;
				return qse_httpd_entaskerr (httpd, client, pred, status, req);
			}
			else
			{
				/* create a directory listing task */
				qse_httpd_task_t task, * x;

				data.path.ptr = (qse_mchar_t*)dir->path;
				data.path.len = qse_mbslen(data.path.ptr);
				data.qpath.ptr = qse_htre_getqpath(req);
				data.qpath.len = qse_mbslen(data.qpath.ptr);
				data.head.ptr = dir->head? (qse_mchar_t*)dir->head: QSE_MT("<style type='text/css'>body { background-color:#d0e4fe; font-size: 0.9em; } div.header { font-weight: bold; margin-bottom: 5px; } div.footer { border-top: 1px solid #99AABB; text-align: right; } table { font-size: 0.9em; } td { white-space: nowrap; } td.size { text-align: right; }</style>");
				data.head.len = qse_mbslen(data.head.ptr);
				data.foot.ptr = dir->foot? dir->foot: qse_httpd_getname(httpd);
				data.foot.len = qse_mbslen(data.foot.ptr);
				data.version = *qse_htre_getversion(req);
				data.keepalive = (req->attr.flags & QSE_HTRE_ATTR_KEEPALIVE);
				data.method = method;

				QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
				task.init = task_init_getdir;
				task.main = task_main_getdir;
				task.ctx = &data;

				x = qse_httpd_entask (httpd, client, pred, &task,
					QSE_SIZEOF(task_dir_t) + data.path.len + 1 + data.qpath.len + 1 + 
					                         data.head.len + 1 + data.foot.len + 1);
				if (x == QSE_NULL)
				{
					httpd->opt.scb.dir.close (httpd, data.handle);
				}
				return x;
			}
		}
			
		case QSE_HTTP_PUT:
		{
			int status = 201; /* 201 Created */

			if (httpd->opt.scb.dir.make (httpd, dir->path) <= -1)
			{
				if (httpd->errnum == QSE_HTTPD_EEXIST)
				{
					/* an entry with the same name exists.
					 * if it is a directory, it's considered ok.
					 * if not, send '403 forbidden' indicating you can't 
					 * change a file to a directory */
					qse_httpd_stat_t st;
					status = (httpd->opt.scb.dir.stat (httpd, dir->path, &st) <= -1)? 403: 204;
				}
				else
				{
					status = (httpd->errnum == QSE_HTTPD_ENOENT)? 404:
					         (httpd->errnum == QSE_HTTPD_EACCES)? 403: 500;
				}
			}
				
			return qse_httpd_entaskerr (httpd, client, pred, status, req);
		}


		case QSE_HTTP_DELETE:
		{
			int status = 200;

			if (httpd->opt.scb.dir.purge (httpd, dir->path) <= -1)
			{
				status = (httpd->errnum == QSE_HTTPD_ENOENT)? 404:
				         (httpd->errnum == QSE_HTTPD_EACCES)? 403: 500;
			}
				
			return qse_httpd_entaskerr (httpd, client, pred, status, req);
		}

		default:
			/* Method not allowed */
			return qse_httpd_entaskerr (httpd, client, pred, 405, req);
	}
}
