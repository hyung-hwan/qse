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
#include "../cmn/syscall.h"
#include <qse/cmn/str.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/pio.h>
#include <qse/cmn/fmt.h>

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <dirent.h>

#define MAX_SEND_SIZE 4096

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
	n = httpd->cbs->client.send (httpd, client, task->ctx, count);
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
	n = httpd->cbs->client.send (httpd, client, ctx->ptr, count);
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

	n = httpd->cbs->client.send (httpd, client, ctx->ptr, count);
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

qse_printf (QSE_T("SEND: [%.*hs]\n"), (int)l, buf);
	return qse_httpd_entask (
		httpd, client, pred, &task, QSE_SIZEOF(data));
}

/* TODO: send wide character string when QSE_CHAR_IS_WCHAR */

/*------------------------------------------------------------------------*/

static qse_httpd_task_t* entask_error (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	const qse_httpd_task_t* task, int code, 
	const qse_http_version_t* version, int keepalive)
{
	const qse_mchar_t* smsg;
	const qse_mchar_t* lmsg;

	switch (code)
	{
		case 403:
			smsg = QSE_MT("Forbidden");
			lmsg = QSE_MT("<html><head><title>Forbidden</title></head><body><b>FORBIDDEN<b></body></html>");
			break;

		case 404:
			smsg = QSE_MT("Not Found");
			lmsg = QSE_MT("<html><head><title>Not Found</title></head><body><b>REQUESTED PATH NOT FOUND<b></body></html>");
			break;

		case 405:
			smsg = QSE_MT("Method Not Allowed");
			lmsg = QSE_MT("<html><head><title>Method Not Allowed</title></head><body><b>REQUESTED METHOD NOT ALLOWED<b></body></html>");
			break;

		case 411:
			smsg = QSE_MT("Length Required");
			lmsg = QSE_MT("<html><head><title>Length Required</title></head><body><b>LENGTH REQUIRED<b></body></html>");
			break;

		case 416:
			smsg = QSE_MT("Requested Range Not Satisfiable");
			lmsg = QSE_MT("<html><head><title>Requested Range Not Satsfiable</title></head><body><b>REQUESTED RANGE NOT SATISFIABLE<b></body></html>");
			break;

		case 417:
			smsg = QSE_MT("Expectation Failed");
			lmsg = QSE_MT("<html><head><title>Expectation Failed</title></head><body><b>EXPECTATION FAILED<b></body></html>");
			break;

		case 500:
			smsg = QSE_MT("Internal Server Error");
			lmsg = QSE_MT("<html><head><title>Internal Server Error</title></head><body><b>INTERNAL SERVER ERROR<b></body></html>");
			break;

		case 501:
			smsg = QSE_MT("Not Implemented");
			lmsg = QSE_MT("<html><head><title>Not Implemented</title></head><body><b>NOT IMPLEMENTED<b></body></html>");
			break;

		case 502:
			smsg = QSE_MT("Bad Gateway");
			lmsg = QSE_MT("<html><head><title>Bad Gateway</title></head><body><b>BAD GATEWAY<b></body></html>");
			break;

		case 503:
			smsg = QSE_MT("Service Unavailable");
			lmsg = QSE_MT("<html><head><title>Service Unavailable</title></head><body><b>SERVICE UNAVAILABLE<b></body></html>");
			break;
		
		default:
			smsg = QSE_MT("Unknown");
			lmsg = QSE_MT("<html><head><title>Unknown Error</title></head><body><b>UNKNOWN ERROR<b></body></html>");
			break;
	}

	return qse_httpd_entaskformat (
		httpd, client, task,
		QSE_MT("HTTP/%d.%d %d %s\r\nConnection: %s\r\nContent-Type: text/html;charset=utf-8\r\nContent-Length: %d\r\n\r\n%s\r\n\r\n"), 
		version->major, version->minor, code, smsg,
		(keepalive? QSE_MT("keep-alive"): QSE_MT("close")),
		(int)qse_mbslen(lmsg) + 4, lmsg
	);
}

qse_httpd_task_t* qse_httpd_entaskerror (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	const qse_httpd_task_t* task, int code, const qse_htre_t* req)
{
	return entask_error (httpd, client, task, code, qse_htre_getversion(req), req->attr.keepalive);
}

/*------------------------------------------------------------------------*/
qse_httpd_task_t* qse_httpd_entaskcontinue (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	const qse_httpd_task_t* task, const qse_htre_t* req)
{
	qse_http_version_t* version = qse_htre_getversion(req);
	return qse_httpd_entaskformat (
		httpd, client, task,
		QSE_MT("HTTP/%d.%d 100 Continue\r\n\r\n"),
		version->major, version->minor);
}

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
qse_printf (QSE_T("closing file %d\n"), ctx->handle.i);
	QSE_CLOSE (ctx->handle.i);
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
	n = httpd->cbs->client.sendfile (
		httpd, client, ctx->handle, &ctx->offset, count);
	if (n <= -1) 
	{
// HANDLE EGAIN specially???
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

qse_printf (QSE_T("Debug: entasking file (%d)\n"), client->handle.i);
	return qse_httpd_entask (httpd, client, pred, &task, QSE_SIZEOF(data));
}

/*------------------------------------------------------------------------*/
typedef struct task_dir_t task_dir_t;
struct task_dir_t
{
	qse_ubi_t handle;

	int header_added;
	int footer_pending;
	struct dirent* dent;

	/*qse_mchar_t buf[4096];*/
	qse_mchar_t buf[512]; /* TOOD: increate size */
	qse_size_t  bufpos;
	qse_size_t  buflen;
	qse_size_t  bufrem;
	qse_size_t  chunklen;
};

static int task_init_dir (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_dir_t* xtn = qse_httpd_gettaskxtn (httpd, task);

	QSE_MEMSET (xtn, 0, QSE_SIZEOF(*xtn));
	xtn->handle = *(qse_ubi_t*)task->ctx;
	task->ctx = xtn;
	return 0;
}

static void task_fini_dir (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_dir_t* ctx = (task_dir_t*)task->ctx;
	closedir (ctx->handle.ptr);
}

static int task_main_dir (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_dir_t* ctx = (task_dir_t*)task->ctx;
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

/* TODO: get the actual path ... and use it in the body or title. */
		x = snprintf (
			&ctx->buf[ctx->buflen], 
			ctx->bufrem,
			QSE_MT("<html><head><title>Directory Listing</title></head><body>index of xxxx<ul>")
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
		ctx->dent = readdir (ctx->handle.ptr);

	do
	{
		if (ctx->dent == QSE_NULL)
		{
			// TODO: check if errno has changed from before readdir().
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
		else
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

		ctx->dent = readdir (ctx->handle.ptr);
	}
	while (1);

set_chunklen:
	/* right alignment with space padding on the left */
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

static int task_main_dir_nochunk (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_dir_t* ctx = (task_dir_t*)task->ctx;
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
			"<html><head><title>Directory Listing</title></head><body>index of xxxx<ul>"
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
		ctx->dent = readdir (ctx->handle.ptr);

	do
	{
		if (ctx->dent == QSE_NULL)
		{
			// TODO: check if errno has changed from before readdir().
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
		else
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

		ctx->dent = readdir (ctx->handle.ptr);
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

qse_httpd_task_t* qse_httpd_entaskdir (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client, 
	const qse_httpd_task_t* pred,
	qse_ubi_t handle, int chunked)
{
	qse_httpd_task_t task;

	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = task_init_dir;
	task.main = chunked? task_main_dir: task_main_dir_nochunk;
	task.fini = task_fini_dir;
	task.ctx = &handle;

	return qse_httpd_entask (httpd, client, pred, &task, QSE_SIZEOF(task_dir_t));
}

/*------------------------------------------------------------------------*/

typedef struct task_path_t task_path_t;
struct task_path_t
{
	const qse_mchar_t* name;
	qse_http_range_t   range;
	qse_http_version_t version;
	int                keepalive;
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

static QSE_INLINE int task_main_path_file (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task, qse_foff_t filesize)
{
	task_path_t* data = (task_path_t*)task->ctx;
	qse_httpd_task_t* x = task;
	qse_ubi_t handle;
	int oflags;

	/* when it comes to the file size, using fstat after opening 
	 * can be more accurate. but this function uses information
	 * set into the task context before the call to this function */

qse_printf (QSE_T("opening file %hs\n"), data->name);

	oflags = O_RDONLY;
#if defined(O_LARGEFILE)
	oflags |= O_LARGEFILE;
#endif
	handle.i = QSE_OPEN (data->name, oflags, 0);
	if (handle.i <= -1)
	{
		x = entask_error (httpd, client, x, 404, &data->version, data->keepalive);
		goto no_file_send;
	}
	oflags = QSE_FCNTL (handle.i, F_GETFD, 0);
	if (oflags >= 0) 
		QSE_FCNTL (handle.i, F_SETFD, oflags | FD_CLOEXEC);

	if (data->range.type != QSE_HTTP_RANGE_NONE)
	{ 
		const qse_mchar_t* mime_type = QSE_NULL;

		if (data->range.type == QSE_HTTP_RANGE_SUFFIX)
		{
			if (data->range.to > filesize) data->range.to = filesize;
			data->range.from = filesize - data->range.to;
			data->range.to = data->range.to + data->range.from;
			if (filesize > 0) data->range.to--;
		}

		if (data->range.from >= filesize)
		{
			x = entask_error (httpd, client, x, 416, &data->version, data->keepalive);
			goto no_file_send;
		}

		if (data->range.to >= filesize) data->range.to = filesize - 1;

		if (httpd->cbs->getmimetype)
		{
			httpd->errnum = QSE_HTTPD_ENOERR;
			mime_type = httpd->cbs->getmimetype (httpd, data->name);
			/*TODO: how to handle an error... */
		}

#if (QSE_SIZEOF_LONG_LONG > 0)
		x = qse_httpd_entaskformat (
			httpd, client, x,
    			QSE_MT("HTTP/%d.%d 206 Partial Content\r\nConnection: %s\r\n%s%s%sContent-Length: %llu\r\nContent-Range: bytes %llu-%llu/%llu\r\n\r\n"), 
			data->version.major,
			data->version.minor,
			(data->keepalive? QSE_MT("keep-alive"): QSE_MT("close")),
			(mime_type? QSE_MT("Content-Type: "): QSE_MT("")),
			(mime_type? mime_type: QSE_MT("")),
			(mime_type? QSE_MT("\r\n"): QSE_MT("")),
			(unsigned long long)(data->range.to - data->range.from + 1),
			(unsigned long long)data->range.from,
			(unsigned long long)data->range.to,
			(unsigned long long)filesize
		);
#else
		x = qse_httpd_entaskformat (
			httpd, client, x,
    			QSE_MT("HTTP/%d.%d 206 Partial Content\r\nConnection: %s\r\n%s%s%sContent-Length: %lu\r\nContent-Range: bytes %lu-%lu/%lu\r\n\r\n"), 
			data->version.major,
			data->version.minor,
			(data->keepalive? QSE_MT("keep-alive"): QSE_MT("close")),
			(mime_type? QSE_MT("Content-Type: "): QSE_MT("")),
			(mime_type? mime_type: QSE_MT("")),
			(mime_type? QSE_MT("\r\n"): QSE_MT("")),
			(unsigned long)(data->range.to - data->range.from + 1),
			(unsigned long)data->range.from,
			(unsigned long)data->range.to,
			(unsigned long)filesize
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

		if (httpd->cbs->getmimetype)
		{
			httpd->errnum = QSE_HTTPD_ENOERR;
			mime_type = httpd->cbs->getmimetype (httpd, data->name);
/*TODO: how to handle an error... */
		}

		/* wget 1.8.2 set 'Connection: keep-alive' in the http 1.0 header.
		 * if the reply doesn't contain 'Connection: keep-alive', it didn't
		 * close connection.*/
#if (QSE_SIZEOF_LONG_LONG > 0)
		x = qse_httpd_entaskformat (
			httpd, client, x,
    			QSE_MT("HTTP/%d.%d 200 OK\r\nConnection: %s\r\n%s%s%sContent-Length: %llu\r\n\r\n"), 
			data->version.major, data->version.minor,
			(data->keepalive? QSE_MT("keep-alive"): QSE_MT("close")),
			(mime_type? QSE_MT("Content-Type: "): QSE_MT("")),
			(mime_type? mime_type: QSE_MT("")),
			(mime_type? QSE_MT("\r\n"): QSE_MT("")),
			(unsigned long long)filesize
		);
#else
		x = qse_httpd_entaskformat (
			httpd, client, x,
    			QSE_MT("HTTP/%d.%d 200 OK\r\nConnection: %s\r\n%s%s%sContent-Length: %lu\r\n\r\n"), 
			data->version.major,
			data->version.minor,
			(data->keepalive? QSE_MT("keep-alive"): QSE_MT("close")),
			(mime_type? QSE_MT("Content-Type: "): QSE_MT("")),
			(mime_type? mime_type: QSE_MT("")),
			(mime_type? QSE_MT("\r\n"): QSE_MT("")),
			(unsigned long)filesize
		);
#endif
		if (x)
		{
			x = qse_httpd_entaskfile (
				httpd, client, x, handle, 0, filesize);
		}
	}

	return (x == QSE_NULL)? -1: 0;

no_file_send:
	if (handle.i >= 0) close (handle.i);
	return (x == QSE_NULL)? -1: 0;
}

static QSE_INLINE int task_main_path_dir (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_path_t* data = (task_path_t*)task->ctx;
	qse_httpd_task_t* x = task;
	qse_ubi_t handle;

	if (qse_mbsend (data->name, QSE_MT("/")))
	{
		handle.ptr = opendir (data->name);
		if (handle.ptr)
		{
			if (data->version.major < 1 ||
			    (data->version.major == 1 && data->version.minor == 0))
			{
				data->keepalive = 0;
			}

			if (data->keepalive)
			{
				x = qse_httpd_entaskformat (
					httpd, client, x,
    					QSE_MT("HTTP/%d.%d 200 OK\r\nConnection: keep-alive\r\nContent-Type: text/html;charset=utf-8\r\nTransfer-Encoding: chunked\r\n\r\n"), 
					data->version.major, data->version.minor
				);
				if (x) x = qse_httpd_entaskdir (httpd, client, x, handle, data->keepalive);
			}
			else
			{
				x = qse_httpd_entaskformat (
					httpd, client, x,
    					QSE_MT("HTTP/%d.%d 200 OK\r\nConnection: close\r\nContent-Type: text/html;charset=utf-8\r\n\r\n"), 
					data->version.major, data->version.minor
				);

				if (x) 
				{
					x = qse_httpd_entaskdir (httpd, client, x, handle, data->keepalive);
					if (x) x = qse_httpd_entaskdisconnect (httpd, client, x);
				}
			}

			if (!x) closedir (handle.ptr);
		}
		else
		{
			x = entask_error (httpd, client, x, 403, &data->version, data->keepalive);
		}
	}
	else
	{
		x = qse_httpd_entaskformat (
			httpd, client, x,
    			QSE_MT("HTTP/%d.%d 301 Moved Permanently\r\nContent-Length: 0\r\nConnection: %s\r\nLocation: %s/\r\n\r\n"), 
			data->version.major, data->version.minor,
			(data->keepalive? QSE_MT("keep-alive"): QSE_MT("close")),
			data->name	
		);
	}

	return (x == QSE_NULL)? -1: 0;
}

static int task_main_path (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_path_t* data = (task_path_t*)task->ctx;
	qse_lstat_t st;

	if (QSE_LSTAT (data->name, &st) <= -1)
	{
		return (entask_error (httpd, client, task, 404, &data->version, data->keepalive) == QSE_NULL)? -1: 0;
	}	

	if (S_ISDIR(st.st_mode))
	{
		return task_main_path_dir (httpd, client, task);
	}
	else
	{
		if (st.st_size < 0) st.st_size = 0; /* can this happen? */
		return task_main_path_file (httpd, client, task, st.st_size);
	}
}

qse_httpd_task_t* qse_httpd_entaskpath (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client, 
	const qse_httpd_task_t* pred,
	const qse_mchar_t* name,
	const qse_htre_t* req)
{
	qse_httpd_task_t task;
	task_path_t data;
	const qse_mchar_t* range;

	QSE_MEMSET (&data, 0, QSE_SIZEOF(data));
	data.name = name;
	data.version = *qse_htre_getversion(req);
	data.keepalive = req->attr.keepalive;

	range = qse_htre_getheaderval(req, QSE_MT("Range"));
	if (range) 
	{
		if (qse_parsehttprange (range, &data.range) <= -1)
		{
			return entask_error (httpd, client, pred, 416, &data.version, data.keepalive);
		}
	}
	else 
	{
		data.range.type = QSE_HTTP_RANGE_NONE;
	}
	
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
	int nph;
};

typedef struct task_cgi_t task_cgi_t;
struct task_cgi_t
{
	int init_failed;

	const qse_mchar_t* path;
	qse_http_version_t version;
	int keepalive; /* taken from the request */
	int nph;

	qse_env_t* env;
	qse_pio_t* pio;
	qse_htrd_t* htrd;

	qse_htre_t*  req; /* original request associated with this */
	qse_mbs_t*   reqcon; /* content from the request */
	int          reqfwderr;

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

typedef struct cgi_req_hdr_ctx_t cgi_req_hdr_ctx_t;
struct cgi_req_hdr_ctx_t
{
	qse_httpd_t* httpd;
	qse_env_t* env;
};


int walk_req_headers (qse_htre_t* req, const qse_mchar_t* key, const qse_mchar_t* val, void* ctx)
{
	cgi_req_hdr_ctx_t* cgi;
	qse_mchar_t* http_key;
	int ret;

	cgi = (cgi_req_hdr_ctx_t*)ctx;

/* convert value to uppercase, change - to _ */
	http_key = qse_mbsdup2 (QSE_MT("HTTP_"), key, req->mmgr);
	if (http_key == QSE_NULL)
	{
		cgi->httpd->errnum = QSE_HTTPD_ENOMEM;
		return -1;
	}

	ret = qse_env_insertmbs (cgi->env, http_key, val);
	QSE_MMGR_FREE (req->mmgr, http_key);
	return ret;
}

int walk_cgi_headers (qse_htre_t* req, const qse_mchar_t* key, const qse_mchar_t* val, void* ctx)
{
	task_cgi_t* cgi = (task_cgi_t*)ctx;

	if (qse_mbscmp (key, QSE_MT("Status")) != 0)
	{
		if (qse_mbs_cat (cgi->res, key) == (qse_size_t)-1) return -1;
		if (qse_mbs_cat (cgi->res, QSE_MT(": ")) == (qse_size_t)-1) return -1;
		if (qse_mbs_cat (cgi->res, val) == (qse_size_t)-1) return -1;
		if (qse_mbs_cat (cgi->res, QSE_MT("\r\n")) == (qse_size_t)-1) return -1;
	}

	return 0;
}

static int cgi_htrd_peek_request (qse_htrd_t* htrd, qse_htre_t* req)
{
	cgi_htrd_xtn_t* xtn = (cgi_htrd_xtn_t*) qse_htrd_getxtn (htrd);
	task_cgi_t* cgi = xtn->cgi;
	const qse_mchar_t* status;
	static qse_http_version_t v11 = { 1, 1 };

	status = qse_htre_getheaderval (req, QSE_MT("Status"));
	if (status)
	{
		qse_mchar_t buf[128];
		int nstatus;
		qse_mchar_t* endptr;

/* TODO: check the syntax of status value??? */
		QSE_MSTRTONUM (nstatus, status, &endptr, 10);

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
		qse_mchar_t* location;
		qse_mchar_t buf[128];

		location = qse_htre_getheaderval (req, QSE_MT("Location"));
		if (location)
		{
			snprintf (buf, QSE_COUNTOF(buf), 	
				QSE_MT("HTTP/%d.%d 301 Moved Permanently\r\n"),
				cgi->version.major, cgi->version.minor
			);
			if (qse_mbs_cat (cgi->res, buf) == (qse_size_t)-1) return -1;

			/* the actual Location hearer is added by 
			 * qse_htre_walkheaders() below */
		}
		else
		{
			snprintf (buf, QSE_COUNTOF(buf), 	
				QSE_MT("HTTP/%d.%d 200 OK\r\n"),
				cgi->version.major, cgi->version.minor
			);
			if (qse_mbs_cat (cgi->res, buf) == (qse_size_t)-1) return -1;
		}
	}

	if (req->attr.content_length_set) 
	{
		cgi->content_length_set = 1;
		cgi->content_length = req->attr.content_length;
	}
	else
	{
		/* no Content-Length returned by CGI. */
		if (qse_comparehttpversions (&cgi->version, &v11) >= 0) 
		{
			cgi->content_chunked = 1;
		}
		else 
		{
			/* If CGI doesn't specify Content-Length, i can't
			 * honor cgi->keepalive in HTTP/1.0 or earlier.
			 * Closing the connection is the only way to
			 * specify how long the content is */
			cgi->disconnect = 1;
		}
	}

	if (cgi->content_chunked &&
	    qse_mbs_cat (cgi->res, QSE_MT("Transfer-Encoding: chunked\r\n")) == (qse_size_t)-1) return -1;

	if (qse_htre_walkheaders (req, walk_cgi_headers, cgi) <= -1) return -1;
	/* end of header */
	if (qse_mbs_cat (cgi->res, QSE_MT("\r\n")) == (qse_size_t)-1) return -1; 

	/* content body begins here */
	cgi->content_received = qse_htre_getcontentlen(req);
	if (cgi->content_length_set && 
	    cgi->content_received > cgi->content_length)
	{
/* TODO: cgi returning too much data... something is wrong in CGI */
		return -1;
	}

	if (cgi->content_received > 0)
	{
		/* the initial part of the content body has been received 
		 * along with the header. it need to be added to the result 
		 * buffer. */
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
	cgi_htrd_peek_request,
	QSE_NULL /* not needed for CGI */
};

static qse_env_t* makecgienv (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	const qse_htre_t* req, const qse_mchar_t* path)
{
/* TODO: error check */
	qse_env_t* env;
	cgi_req_hdr_ctx_t ctx;

	env = qse_env_open (httpd->mmgr, 0, 0);
	if (env == QSE_NULL) goto oops;

#ifdef _WIN32
	qse_env_insert (env, QSE_T("PATH"), QSE_NULL);
#else
	qse_env_insertmbs (env, QSE_MT("LANG"), QSE_NULL);
	qse_env_insertmbs (env, QSE_MT("PATH"), QSE_NULL);
#endif

	qse_env_insertmbs (env, QSE_MT("GATEWAY_INTERFACE"), QSE_MT("CGI/1.1"));

{
/* TODO: corrent all these name???  */
qse_mchar_t tmp[1024];
qse_mbsxncpy (tmp, QSE_COUNTOF(tmp), qse_htre_getqpathptr(req), qse_htre_getqpathlen(req));
	//qse_env_insertmbs (env, QSE_MT("SCRIPT_NAME"), tmp);
	//qse_env_insertmbs (env, QSE_MT("PATH_INFO"), tmp);
	//qse_env_insertmbs (env, QSE_MT("PATH_TRANSLATED"), tmp);
	//qse_env_insertmbs (env, QSE_MT("DOCUMENT_ROOT"), QSE_MT("/"));
}


	//qse_env_insertmbs (env, QSE_MT("SCRIPT_FILENAME"), path);
	qse_env_insertmbs (env, QSE_MT("REQUEST_URI"), qse_htre_getqpathptr(req));

	{
		qse_mchar_t* tmp = qse_htre_getqparamptr(req);
		if (tmp) qse_env_insertmbs (env, QSE_MT("QUERY_STRING"), tmp);
	}

	qse_env_insertmbs (
		env, QSE_MT("REQUEST_METHOD"), qse_htre_getqmethodname(req));


	if (req->attr.content_length_set)
	{
		qse_mchar_t tmp[64];
		qse_fmtuintmaxtombs (
			tmp, QSE_COUNTOF(tmp), 
			req->attr.content_length, 10, 
			-1, QSE_MT('\0'), QSE_NULL);
		qse_env_insertmbs (env, QSE_MT("CONTENT_LENGTH"), tmp);
	}

	// TODO: SERVER_HOST, REMOTE_HOST,  
	{
		qse_mchar_t port[16];
		snprintf (port, QSE_COUNTOF(port), 
			QSE_MT("%d"), (int)ntohs(client->local_addr.in4.sin_port));
		qse_env_insertmbs (env, QSE_MT("SERVER_PORT"), port);

		snprintf (port, QSE_COUNTOF(port), 
			QSE_MT("%d"), (int)ntohs(client->remote_addr.in4.sin_port));
		qse_env_insertmbs (env, QSE_MT("REMOTE_PORT"), port);
	}

	if (client->local_addr.in4.sin_family == AF_INET)
	{
		qse_mchar_t ipaddr[128];
		inet_ntop (client->local_addr.in4.sin_family, &client->local_addr.in4.sin_addr, ipaddr, QSE_COUNTOF(ipaddr));
		qse_env_insertmbs (env, QSE_MT("SERVER_ADDR"), ipaddr);
	}
	else
	{
		qse_mchar_t ipaddr[128];
		inet_ntop (client->local_addr.in6.sin6_family, &client->local_addr.in6.sin6_addr, ipaddr, QSE_COUNTOF(ipaddr));
		qse_env_insertmbs (env, QSE_MT("SERVER_ADDR"), ipaddr);
	}

	if (client->remote_addr.in4.sin_family == AF_INET)
	{
		qse_mchar_t ipaddr[128];
		inet_ntop (client->remote_addr.in4.sin_family, &client->remote_addr.in4.sin_addr, ipaddr, QSE_COUNTOF(ipaddr));
		qse_env_insertmbs (env, QSE_MT("REMOTE_ADDR"), ipaddr);
	}
	else
	{
		qse_mchar_t ipaddr[128];
		inet_ntop (client->remote_addr.in6.sin6_family, &client->remote_addr.in6.sin6_addr, ipaddr, QSE_COUNTOF(ipaddr));
		qse_env_insertmbs (env, QSE_MT("REMOTE_ADDR"), ipaddr);
	}

	{
		qse_mchar_t proto[32];
		const qse_http_version_t* v = qse_htre_getversion(req);
		snprintf (proto, QSE_COUNTOF(proto), 
			QSE_MT("HTTP/%d.%d"), (int)v->major, (int)v->minor);
		qse_env_insertmbs (env, QSE_MT("SERVER_PROTOCOL"), proto);
	}
// TODO: HTTP_ headers.

#if 0
	qse_env_insertmbs (env, "SERVER_NAME",
	qse_env_insertmbs (env, "SERVER_ROOT", 
	qse_env_insertmbs (env, "DOCUMENT_ROOT", 
	qse_env_insertmbs (env, "REMOTE_PORT", 
	qse_env_insertmbs (env, "REQUEST_URI", 
#endif

#if 0
	ctx.httpd = httpd;
	ctx.env = env;
	if (qse_htre_walkheaders (req, walk_req_headers, &ctx) <= -1) return -1;
#endif

	{
		const qse_mchar_t* tmp;

		tmp = qse_htre_getheaderval(req, QSE_MT("Content-Type"));
		if (tmp) qse_env_insertmbs (env, QSE_MT("CONTENT_TYPE"), tmp);

		tmp = qse_htre_getheaderval(req, QSE_MT("Cookie"));
		if (tmp) qse_env_insertmbs (env, QSE_MT("HTTP_COOKIE"), tmp);

		tmp = qse_htre_getheaderval(req, QSE_MT("Host"));
		if (tmp) qse_env_insertmbs (env, QSE_MT("HTTP_HOST"), tmp);

		tmp = qse_htre_getheaderval(req, QSE_MT("Referer"));
		if (tmp) qse_env_insertmbs (env, QSE_MT("HTTP_REFERER"), tmp);

		tmp = qse_htre_getheaderval(req, QSE_MT("User-Agent"));
		if (tmp) qse_env_insertmbs (env, QSE_MT("HTTP_USER_AGENT"), tmp);
	}

	return env;

oops:
	if (env) qse_env_close (env);
	return QSE_NULL;
}

static int cgi_snatch_content (
	qse_htre_t* req, const qse_mchar_t* ptr, qse_size_t len, void* ctx)
{
	task_cgi_t* cgi = (task_cgi_t*)ctx;

	if (ptr == QSE_NULL)
	{
		/* request ended. this could be a real end or 
		 * abortion for an error */
		QSE_ASSERT (len == 0);
		cgi->req = QSE_NULL;
	}
	else
	{
		/* push the contents to the own buffer */
		if (qse_mbs_ncat (cgi->reqcon, ptr, len) == (qse_size_t)-1)
		{
			return -1;
		}
	}

	return 0;
}

static int cgi_forward_content (qse_httpd_task_t* task)
{
	task_cgi_t* cgi = (task_cgi_t*)task->ctx;

	QSE_ASSERT (cgi->reqcon != QSE_NULL);

	if (QSE_MBS_LEN(cgi->reqcon) > 0)
	{
		qse_ssize_t n;

		if (!cgi->reqfwderr)
		{
qse_printf (QSE_T("@@@@@@@WRITING %d bytes TO CGI\n"), (int)QSE_MBS_LEN(cgi->reqcon));
			n = qse_pio_write (
				cgi->pio, QSE_PIO_IN,
				QSE_MBS_PTR(cgi->reqcon),
				QSE_MBS_LEN(cgi->reqcon)
			);
			if (n <= -1)
			{
qse_printf (QSE_T("@@@@@@@@WRITE TO CGI FAILED\n"));
/* TODO: logging ... */
				cgi->reqfwderr = 1;
				if (cgi->req) cgi->req->flags |= QSE_HTRE_DISCARDED;	
			}
		}

/* TODO: performance improvement...
deleting keeps on moving contents to the head... 
can't we grow the buffer to a certain limit? 
if the limit is reached, copy the tail to the head... */
		/* can write return 0? */
		qse_mbs_del (cgi->reqcon, 0, n);
	}
	else if (cgi->req == QSE_NULL)
	{
		/* no more request content */
qse_printf (QSE_T("@@@@@@@@NOTHING MORE TO WRITE TO CGI\n"));
		task->trigger_mask &= 
			~(QSE_HTTPD_TASK_TRIGGER_WRITE |
			  QSE_HTTPD_TASK_TRIGGER_WRITABLE);
	}

	return 0;
}


static int task_init_cgi (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_cgi_t* cgi = (task_cgi_t*)qse_httpd_gettaskxtn (httpd, task);
	task_cgi_arg_t* arg = (task_cgi_arg_t*)task->ctx;

	QSE_MEMSET (cgi, 0, QSE_SIZEOF(*cgi));
	qse_mbscpy ((qse_mchar_t*)(cgi + 1), arg->path);
	cgi->path = (qse_mchar_t*)(cgi + 1);
	cgi->version = *qse_htre_getversion(arg->req);
	cgi->keepalive = arg->req->attr.keepalive;
	cgi->nph = arg->nph;

	if (!(arg->req->flags & QSE_HTRE_DISCARDED))
	{
		/* CGI entasking is invoked probably from the peek handler
		 * that was triggered after the request header is received 
		 * in principle. In that case, arrange to forward content
		 * bypassing the buffer in the request object itself. */
		const qse_mchar_t* ptr = qse_htre_getcontentptr(arg->req);
		qse_size_t len = qse_htre_getcontentlen(arg->req);

		/* create a buffer to hold request content from the client
		 * and copy content received already */
		cgi->reqcon = qse_mbs_open (httpd->mmgr, 0, (len < 512? 512: len));
		if (cgi->reqcon == QSE_NULL ||
		    qse_mbs_ncpy (cgi->reqcon, ptr, len) == (qse_size_t)-1)
		{
			cgi->init_failed = 1;
		}
		else if (!(arg->req->flags & QSE_HTRE_COMPLETED))
		{
/* TODO: callback chain instead of a single pointer??? 
if the request is already set up with a callback, something will go wrong.
 */
			/* set up a callback to be called when the request content
			 * is fed to the htrd reader */
			cgi->req = (qse_htre_t*)arg->req;
			qse_htre_setconcb (cgi->req, cgi_snatch_content, cgi);
		}
	}
	
	if (!cgi->init_failed)
	{
		cgi->env = makecgienv (httpd, client, arg->req, arg->path);
		if (cgi->env == QSE_NULL) cgi->init_failed = 1;
	}

	task->ctx = cgi;
	return 0;
}

static void task_fini_cgi (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_cgi_t* cgi = (task_cgi_t*)task->ctx;
	if (cgi->env) qse_env_close (cgi->env);
	if (cgi->pio) 
	{
		/* kill cgi in case it is still alive.
		 * qse_pio_wait() in qse_pio_close() can block. */
		qse_pio_kill (cgi->pio); 
		qse_pio_close (cgi->pio);
	}
	if (cgi->res) qse_mbs_close (cgi->res);
	if (cgi->htrd) qse_htrd_close (cgi->htrd);
	if (cgi->reqcon) qse_mbs_close (cgi->reqcon);
	if (cgi->req) 
	{
		/* this task is destroyed but the request
		 * associated is still alive. so clear the 
		 * callback to prevent the callback call. */
		qse_htre_unsetconcb (cgi->req);
	}
		
qse_printf (QSE_T("task_fini_cgi\n"));
}

static int task_main_cgi_5 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_cgi_t* cgi = (task_cgi_t*)task->ctx;
	qse_ssize_t n;

	QSE_ASSERT (cgi->pio != QSE_NULL);

	if (task->trigger_mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		if (cgi_forward_content (task) <= -1) return -1;
		/* if forwarding didn't finish, something is not really right... 
		 * so long as the output from CGI is finished, no more forwarding
		 * is performed */
	}

qse_printf (QSE_T("task_main_cgi_5\n"));
/* TODO: check if cgi outputs more than content-length if it is set... */
	httpd->errnum = QSE_HTTPD_ENOERR;
	n = httpd->cbs->client.send (httpd, client, cgi->buf, cgi->buflen);
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

	if (task->trigger_mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		if (cgi_forward_content (task) <= -1) return -1;
	}

	if (task->trigger_mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
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
					/* the cgi script closed the output */
					task->trigger_mask &= 
						~(QSE_HTTPD_TASK_TRIGGER_READ |
					       QSE_HTTPD_TASK_TRIGGER_READABLE);

					cgi->buf[cgi->buflen++] = QSE_MT('0');
					cgi->buf[cgi->buflen++] = QSE_MT('\r');
					cgi->buf[cgi->buflen++] = QSE_MT('\n');
					cgi->buf[cgi->buflen++] = QSE_MT('\r');
					cgi->buf[cgi->buflen++] = QSE_MT('\n');
	
					task->main = task_main_cgi_5;
					return task_main_cgi_5 (httpd, client, task);
				}
	
				/* set the chunk length */
				snprintf (chunklen, QSE_COUNTOF(chunklen), 
					QSE_MT("%-4lX\r\n"), (unsigned long)n);
				QSE_MEMCPY (&cgi->buf[cgi->buflen],
					chunklen, QSE_SIZEOF(chunklen) - 1);
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
				task->trigger_mask &= 
					~(QSE_HTTPD_TASK_TRIGGER_READ |
				       QSE_HTTPD_TASK_TRIGGER_READABLE);
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
	
	#if 0
	qse_printf (QSE_T("CGI_4 SEND [%.*hs]\n"), (int)cgi->buflen, cgi->buf);
	#endif
		httpd->errnum = QSE_HTTPD_ENOERR;
		n = httpd->cbs->client.send (httpd, client, cgi->buf, cgi->buflen);
		if (n <= -1)
		{
			/* can't return internal server error any more... */
	/* TODO: logging ... */
			return -1;
		}
	
		QSE_MEMCPY (&cgi->buf[0], &cgi->buf[n], cgi->buflen - n);
		cgi->buflen -= n;
	
	#if 0
	qse_printf (QSE_T("CGI SEND DONE\n"));
	#endif
	}

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

	if (task->trigger_mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		if (cgi_forward_content (task) <= -1) return -1;
	}

	if (task->trigger_mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		count = MAX_SEND_SIZE;
		if (count >= cgi->res_left) count = cgi->res_left;

qse_printf (QSE_T("[cgi_3 sending %d bytes]\n"), (int)count);
		httpd->errnum = QSE_HTTPD_ENOERR;
		n = httpd->cbs->client.send (httpd, client, cgi->res_ptr, count);

		if (n <= -1) return -1;

		cgi->res_left -= n;
		if (cgi->res_left <= 0) 
		{
			task->main = task_main_cgi_4;
			/* don't chain-call task_main_cgi_4 since it has another send
			 * and it has already been sent here. so the writability must
			 * be checked again in the main loop.
			 * => return task_main_cgi_4 (httpd, client, task);*/
			return 1;
		}

		cgi->res_ptr += n;
	}

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
	if (task->trigger_mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
qse_printf (QSE_T("[cgi_2 write]\n"));
		if (cgi_forward_content (task) <= -1) return -1;
	}

	if (task->trigger_mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
qse_printf (QSE_T("[cgi_2 read]\n"));
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
			task->trigger_mask &= 
				~(QSE_HTTPD_TASK_TRIGGER_READ |
				  QSE_HTTPD_TASK_TRIGGER_READABLE);
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
			 *
			 * qse_htrd_feed() must have executed the peek handler
			 * (cgi_htrd_peek_request()) which handled the request header.
			 * so i won't call qse_htrd_feed() any more. intead, i'll
			 * simply read directly from the pipe.
			 */

			if (cgi->disconnect && 
			    qse_httpd_entaskdisconnect (httpd, client, task) == QSE_NULL) return -1;

			cgi->res_ptr = QSE_MBS_PTR(cgi->res);
			cgi->res_left = QSE_MBS_LEN(cgi->res);

qse_printf (QSE_T("TRAILING DATA=[%.*hs]\n"), (int)QSE_MBS_LEN(cgi->res), QSE_MBS_PTR(cgi->res));
			task->main = task_main_cgi_3;
			return task_main_cgi_3 (httpd, client, task);
		}

	}

	/* complete headers not seen yet. i need to be called again */
	return 1;
}

static int task_main_cgi (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_cgi_t* cgi = (task_cgi_t*)task->ctx;
	int pio_options;
	int error_code = 500;

	if (cgi->init_failed) goto oops;

	if (QSE_ACCESS (cgi->path, X_OK) == -1) 
	{
		error_code = (errno == EACCES)? 403: 404;
		goto oops;
	}

	if (cgi->nph)
	{
		/* i cannot know how long the content will be
		 * since i don't parse the header. so i have to close
		 * the connection regardless of content-length or transfer-encoding
		 * in the actual header. */
		if (qse_httpd_entaskdisconnect (httpd, client, task) == QSE_NULL) return -1;
	}
	else
	{
		cgi_htrd_xtn_t* xtn;
		cgi->htrd = qse_htrd_open (httpd->mmgr, QSE_SIZEOF(cgi_htrd_xtn_t));
		if (cgi->htrd == QSE_NULL) goto oops;
		xtn = (cgi_htrd_xtn_t*) qse_htrd_getxtn (cgi->htrd);
		xtn->cgi = cgi;
		qse_htrd_setrecbs (cgi->htrd, &cgi_htrd_cbs);
		qse_htrd_setoption (
			cgi->htrd, 
			QSE_HTRD_SKIPINITIALLINE | 
			QSE_HTRD_PEEKONLY | 
			QSE_HTRD_REQUEST
		);

		cgi->res = qse_mbs_open (httpd->mmgr, 0, 256);
		if (cgi->res == QSE_NULL) goto oops;
	}

	pio_options = QSE_PIO_READOUT | QSE_PIO_WRITEIN | 
	              QSE_PIO_ERRTONUL | QSE_PIO_MBSCMD;
	if (httpd->option & QSE_HTTPD_CGINOCLOEXEC) 
		pio_options |= QSE_PIO_NOCLOEXEC;

	cgi->pio = qse_pio_open (
		httpd->mmgr, 0, (const qse_char_t*)cgi->path, 
		cgi->env, pio_options
	);
	if (cgi->pio == QSE_NULL) goto oops;
	
/* TODO: use a different field for different OS??? 
HANDLE for win32???
*/
	/* set the trigger that the main loop can use this
	 * handle for multiplexing */
	task->trigger_mask = QSE_HTTPD_TASK_TRIGGER_READ;
	task->trigger[0].i = qse_pio_gethandle (cgi->pio, QSE_PIO_OUT);
	if (cgi->reqcon)
	{
		task->trigger_mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
		task->trigger[1].i = qse_pio_gethandle (cgi->pio, QSE_PIO_IN);
	}

	task->main = cgi->nph? task_main_cgi_4: task_main_cgi_2;
	/* no chain call since readability and writability needs 
	 * to be checked in the main loop
	return task->main (httpd, client, task); */
	return 1;

oops:
	if (cgi->res) 
	{
		qse_mbs_close (cgi->res);
		cgi->res = QSE_NULL;
	}
	if (cgi->htrd) 
	{
		qse_htrd_close (cgi->htrd);
		cgi->htrd = QSE_NULL;
	}

	return (entask_error (httpd, client, task, error_code, &cgi->version, cgi->keepalive) == QSE_NULL)? -1: 0;
}

/* TODO: global option or individual paramter for max cgi lifetime 
*        non-blocking pio read ...
*/

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
	arg.nph = 0;

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

qse_httpd_task_t* qse_httpd_entasknph (
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
	arg.nph = 1;

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

#if 0
typedef struct task_proxy_t task_proxy_t;
struct task_proxy_t
{
}

qse_httpd_task_t* qse_httpd_entaskproxy (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client,
	const qse_httpd_task_t* pred, 
	const qse_mchar_t* host,  
	const qse_htre_t* req)
{
	
}
#endif

/*------------------------------------------------------------------------*/

#endif
