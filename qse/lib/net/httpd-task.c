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
#include <qse/cmn/fmt.h>

#include <stdarg.h>
#include <stdio.h>
#include <dirent.h>

#define MAX_SEND_SIZE 4096

/*------------------------------------------------------------------------*/

static int task_main_disconnect (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	httpd->cbs->client.shutdown (httpd, client);
	return 0;
}

qse_httpd_task_t* qse_httpd_entaskdisconnect (
	qse_httpd_t* httpd, 
	qse_httpd_client_t* client,
	qse_httpd_task_t* pred)
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
	qse_httpd_task_t* pred, 
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
	qse_httpd_task_t* pred, 
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
	qse_httpd_task_t* pred,
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

		buf = (qse_mchar_t*) qse_httpd_allocmem (
			httpd, (capa + 1) * QSE_SIZEOF(*buf));
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
				if (buf == QSE_NULL) return QSE_NULL;
			}
			else break;
		}
	}
	else 
	{
		/* vsnprintf returns the number of characters that would 
		 * have been written not including the terminating '\0' 
		 * if the _data buffer were large enough */
		buf = (qse_mchar_t*) qse_httpd_allocmem (
			httpd, (bytes_req + 1) * QSE_SIZEOF(*buf));
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
	qse_httpd_task_t* pred, int code, 
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
		httpd, client, pred,
		QSE_MT("HTTP/%d.%d %d %s\r\nConnection: %s\r\nContent-Type: text/html\r\nContent-Length: %lu\r\n\r\n%s\r\n\r\n"), 
		version->major, version->minor, code, smsg,
		(keepalive? QSE_MT("keep-alive"): QSE_MT("close")),
		(unsigned long)qse_mbslen(lmsg) + 4, lmsg
	);
}

qse_httpd_task_t* qse_httpd_entaskerror (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_httpd_task_t* pred, int code, qse_htre_t* req)
{
	return entask_error (
		httpd, client, pred, code,
		qse_htre_getversion(req), req->attr.keepalive);
}

/*------------------------------------------------------------------------*/
qse_httpd_task_t* qse_httpd_entaskcontinue (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_httpd_task_t* pred, qse_htre_t* req)
{
	const qse_http_version_t* version = qse_htre_getversion(req);
	return qse_httpd_entaskformat (
		httpd, client, pred,
		QSE_MT("HTTP/%d.%d 100 Continue\r\n\r\n"),
		version->major, version->minor);
}

/*------------------------------------------------------------------------*/

qse_httpd_task_t* qse_httpd_entaskauth (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_httpd_task_t* pred, const qse_mchar_t* realm, qse_htre_t* req)
{
	const qse_http_version_t* version;
	const qse_mchar_t* lmsg;

	version = qse_htre_getversion(req);
	lmsg = QSE_MT("<html><head><title>Unauthorized</title></head><body><b>UNAUTHORIZED<b></body></html>");

	return qse_httpd_entaskformat (
		httpd, client, pred,
		QSE_MT("HTTP/%d.%d 401 Unauthorized\r\nConnection: %s\r\nWWW-Authenticate: Basic realm=\"%s\"\r\nContent-Type: text/html\r\nContent-Length: %lu\r\n\r\n%s\r\n\r\n"),
		version->major, version->minor, 
		(req->attr.keepalive? QSE_MT("keep-alive"): QSE_MT("close")),
		realm, (unsigned long)qse_mbslen(lmsg) + 4, lmsg);
}

/*------------------------------------------------------------------------*/

#if 0
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
	qse_httpd_task_t* pred,
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
    					QSE_MT("HTTP/%d.%d 200 OK\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nTransfer-Encoding: chunked\r\n\r\n"), 
					data->version.major, data->version.minor
				);
				if (x) x = qse_httpd_entaskdir (httpd, client, x, handle, data->keepalive);
			}
			else
			{
				x = qse_httpd_entaskformat (
					httpd, client, x,
    					QSE_MT("HTTP/%d.%d 200 OK\r\nConnection: close\r\nContent-Type: text/html\r\n\r\n"), 
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
	qse_httpd_task_t* pred,
	const qse_mchar_t* name,
	qse_htre_t* req)
{
	qse_httpd_task_t task;
	task_path_t data;
	const qse_mchar_t* tmp;

	QSE_MEMSET (&data, 0, QSE_SIZEOF(data));
	data.name = name;
	data.version = *qse_htre_getversion(req);
	data.keepalive = req->attr.keepalive;

	tmp = qse_htre_getheaderval(req, QSE_MT("Range"));
	if (tmp) 
	{
		if (qse_parsehttprange (tmp, &data.range) <= -1)
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
#endif

typedef struct task_file_t task_file_t;
struct task_file_t
{
	const qse_mchar_t* path;
	qse_http_range_t   range;
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
	httpd->cbs->file.close (httpd, ctx->handle);
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
	n = httpd->cbs->client.sendfile (
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
	task_file_t* xtn = qse_httpd_gettaskxtn (httpd, task);
	QSE_MEMCPY (xtn, task->ctx, QSE_SIZEOF(*xtn));
	qse_mbscpy ((qse_mchar_t*)(xtn + 1), xtn->path);
	xtn->path = (qse_mchar_t*)(xtn + 1);
	task->ctx = xtn;
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
	if (httpd->cbs->file.stat (httpd, file->path, &st) <= -1)
	{
		int http_errnum;
		http_errnum = (httpd->errnum == QSE_HTTPD_ENOENT)? 404:
		              (httpd->errnum == QSE_HTTPD_EACCES)? 403: 500;
		x = entask_error (
			httpd, client, x, http_errnum, 
			&file->version, file->keepalive);
		goto no_file_send;
	}

	httpd->errnum = QSE_HTTPD_ENOERR;
	if (httpd->cbs->file.ropen (httpd, file->path, &handle) <= -1)
	{
		int http_errnum;
		http_errnum = (httpd->errnum == QSE_HTTPD_ENOENT)? 404:
		              (httpd->errnum == QSE_HTTPD_EACCES)? 403: 500;
		x = entask_error (
			httpd, client, x, http_errnum, 
			&file->version, file->keepalive);
		goto no_file_send;
	}	
	fileopen = 1;

	if (file->range.type != QSE_HTTP_RANGE_NONE)
	{ 
		if (file->range.type == QSE_HTTP_RANGE_SUFFIX)
		{
			if (file->range.to > st.size) file->range.to = st.size;
			file->range.from = st.size - file->range.to;
			file->range.to = file->range.to + file->range.from;
			if (st.size > 0) file->range.to--;
		}

		if (file->range.from >= st.size)
		{
			x = entask_error (
				httpd, client, x, 416, &file->version, file->keepalive);
			goto no_file_send;
		}

		if (file->range.to >= st.size) file->range.to = st.size - 1;

#if (QSE_SIZEOF_LONG_LONG > 0)
		x = qse_httpd_entaskformat (
			httpd, client, x,
    			QSE_MT("HTTP/%d.%d 206 Partial Content\r\nConnection: %s\r\n%s%s%sContent-Length: %llu\r\nContent-Range: bytes %llu-%llu/%llu\r\n\r\n"), 
			file->version.major,
			file->version.minor,
			(file->keepalive? QSE_MT("keep-alive"): QSE_MT("close")),
			(st.mime? QSE_MT("Content-Type: "): QSE_MT("")),
			(st.mime? st.mime: QSE_MT("")),
			(st.mime? QSE_MT("\r\n"): QSE_MT("")),
			(unsigned long long)(file->range.to - file->range.from + 1),
			(unsigned long long)file->range.from,
			(unsigned long long)file->range.to,
			(unsigned long long)st.size
		);
#else
		x = qse_httpd_entaskformat (
			httpd, client, x,
    			QSE_MT("HTTP/%d.%d 206 Partial Content\r\nConnection: %s\r\n%s%s%sContent-Length: %lu\r\nContent-Range: bytes %lu-%lu/%lu\r\n\r\n"), 
			file->version.major,
			file->version.minor,
			(file->keepalive? QSE_MT("keep-alive"): QSE_MT("close")),
			(st.mime? QSE_MT("Content-Type: "): QSE_MT("")),
			(st.mime? st.mime: QSE_MT("")),
			(st.mime? QSE_MT("\r\n"): QSE_MT("")),
			(unsigned long)(file->range.to - file->range.from + 1),
			(unsigned long)file->range.from,
			(unsigned long)file->range.to,
			(unsigned long)st.size
		);
#endif
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
/* TODO: int64 format.... don't hard code it llu */
		/* wget 1.8.2 set 'Connection: keep-alive' in the http 1.0 header.
		 * if the reply doesn't contain 'Connection: keep-alive', it didn't
		 * close connection.*/
#if (QSE_SIZEOF_LONG_LONG > 0)
		x = qse_httpd_entaskformat (
			httpd, client, x,
    			QSE_MT("HTTP/%d.%d 200 OK\r\nConnection: %s\r\n%s%s%sContent-Length: %llu\r\n\r\n"), 
			file->version.major, file->version.minor,
			(file->keepalive? QSE_MT("keep-alive"): QSE_MT("close")),
			(st.mime? QSE_MT("Content-Type: "): QSE_MT("")),
			(st.mime? st.mime: QSE_MT("")),
			(st.mime? QSE_MT("\r\n"): QSE_MT("")),
			(unsigned long long)st.size
		);
#else
		x = qse_httpd_entaskformat (
			httpd, client, x,
    			QSE_MT("HTTP/%d.%d 200 OK\r\nConnection: %s\r\n%s%s%sContent-Length: %lu\r\n\r\n"), 
			file->version.major,
			file->version.minor,
			(file->keepalive? QSE_MT("keep-alive"): QSE_MT("close")),
			(st.mime? QSE_MT("Content-Type: "): QSE_MT("")),
			(st.mime? st.mime: QSE_MT("")),
			(st.mime? QSE_MT("\r\n"): QSE_MT("")),
			(unsigned long)st.size
		);
#endif
		if (x)
		{
			x = entask_file_segment (httpd, client, x, handle, 0, st.size);
		}
	}

	return (x == QSE_NULL)? -1: 0;

no_file_send:
	if (fileopen) httpd->cbs->file.close (httpd, handle);
	return (x == QSE_NULL)? -1: 0;
}

qse_httpd_task_t* qse_httpd_entaskfile (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client, 
	qse_httpd_task_t* pred,
	const qse_mchar_t* path,
	qse_htre_t* req)
{
	qse_httpd_task_t task;
	task_file_t data;
	const qse_mchar_t* tmp;

	QSE_MEMSET (&data, 0, QSE_SIZEOF(data));
	data.path = path;
	data.version = *qse_htre_getversion(req);
	data.keepalive = req->attr.keepalive;

	tmp = qse_htre_getheaderval(req, QSE_MT("Range"));
	if (tmp) 
	{
		if (qse_parsehttprange (tmp, &data.range) <= -1)
		{
			return qse_httpd_entaskerror (httpd, client, pred, 416, req);
		}
	}
	else 
	{
		data.range.type = QSE_HTTP_RANGE_NONE;
	}

/*
TODO: If-Modified-Since...
	tmp = qse_htre_getheaderval(req, QSE_MT("If-Modified-Since"));
	if (tmp)
	{
	}
*/
	
	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = task_init_file;
	task.main = task_main_file;
	task.ctx = &data;

	return qse_httpd_entask (httpd, client, pred, &task, 
		QSE_SIZEOF(task_file_t) + qse_mbslen(path) + 1);
}

/*------------------------------------------------------------------------*/

typedef struct task_cgi_arg_t task_cgi_arg_t;
struct task_cgi_arg_t 
{
	const qse_mchar_t* path;
	qse_htre_t* req;
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

	qse_htrd_t* htrd;
	qse_env_t* env;
	qse_pio_t pio;
	int pio_inited;

	qse_htre_t*  req; /* original request associated with this */
	qse_mbs_t*   reqfwdbuf; /* content from the request */
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
		const qse_mchar_t* location;
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
	const qse_htre_t* req, const qse_mchar_t* path, qse_size_t content_length)
{
/* TODO: error check */
	qse_env_t* env;

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

	{
		qse_mchar_t tmp[64];
		qse_fmtuintmaxtombs (
			tmp, QSE_COUNTOF(tmp), content_length, 10, 
			-1, QSE_MT('\0'), QSE_NULL);
		qse_env_insertmbs (env, QSE_MT("CONTENT_LENGTH"), tmp);
	}

	{
		qse_mchar_t addr[128];
		qse_nwadtombs (&client->local_addr, 
			addr, QSE_COUNTOF(addr), QSE_NWADTOMBS_PORT);
		qse_env_insertmbs (env, QSE_MT("SERVER_PORT"), addr);
		qse_nwadtombs (&client->local_addr, 
			addr, QSE_COUNTOF(addr), QSE_NWADTOMBS_ADDR);
		qse_env_insertmbs (env, QSE_MT("SERVER_ADDR"), addr);
		qse_nwadtombs (&client->remote_addr,
			addr, QSE_COUNTOF(addr), QSE_NWADTOMBS_PORT);
		qse_env_insertmbs (env, QSE_MT("REMOTE_PORT"), addr);
		qse_nwadtombs (&client->remote_addr,
			addr, QSE_COUNTOF(addr), QSE_NWADTOMBS_ADDR);
		qse_env_insertmbs (env, QSE_MT("REMOTE_ADDR"), addr);
	}

	{
		qse_mchar_t proto[32];
		const qse_http_version_t* v = qse_htre_getversion(req);
		snprintf (proto, QSE_COUNTOF(proto), 
			QSE_MT("HTTP/%d.%d"), (int)v->major, (int)v->minor);
		qse_env_insertmbs (env, QSE_MT("SERVER_PROTOCOL"), proto);
	}
// TODO: HTTP_ headers.
// TODO: REMOTE_USER ...

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
	qse_httpd_task_t* task;
	task_cgi_t* cgi; 

	task = (qse_httpd_task_t*)ctx;
	cgi = (task_cgi_t*)task->ctx;

if (ptr) qse_printf (QSE_T("!!!CGI SNATCHING [%.*hs]\n"), len, ptr);
else qse_printf (QSE_T("!!!CGI SNATCHING DONE\n"));

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
		cgi->req = QSE_NULL; 

		/* since there is no more to read from the client side.
		 * the relay trigger is not needed any more. */
		task->trigger[2].mask = 0;

		if (QSE_MBS_LEN(cgi->reqfwdbuf) > 0 && cgi->pio_inited &&
		    !(task->trigger[1].mask & QSE_HTTPD_TASK_TRIGGER_WRITE))
		{
			/* there's nothing more to read from the client side.
			 * there's something to forward in the forwarding buffer.
			 * but no write trigger is set. add the write trigger 
			 * for task invocation. */
			task->trigger[1].mask = QSE_HTTPD_TASK_TRIGGER_WRITE;
			task->trigger[1].handle = qse_pio_gethandleasubi (&cgi->pio, QSE_PIO_IN);
		}
	}
	else if (!cgi->reqfwderr)
	{
		/* we can write to the child process if a forwarding error 
		 * didn't occur previously. we store data from the client side
		 * to the forwaring buffer only if there's no such previous
		 * error. if an error occurred, we simply drop the data. */
		if (qse_mbs_ncat (cgi->reqfwdbuf, ptr, len) == (qse_size_t)-1)
		{
			return -1;
		}
qse_printf (QSE_T("!!!CGI SNATCHED [%.*hs]\n"), len, ptr);
	}

	return 0;
}

static void cgi_forward_content (
	qse_httpd_t* httpd, qse_httpd_task_t* task, int writable)
{
	task_cgi_t* cgi = (task_cgi_t*)task->ctx;

	QSE_ASSERT (cgi->reqfwdbuf != QSE_NULL);

	if (QSE_MBS_LEN(cgi->reqfwdbuf) > 0)
	{
		/* there is something to forward in the forwarding buffer. */

		if (cgi->reqfwderr) 
		{
			/* a forwarding error has occurred previously.
			 * clear the forwarding buffer */
qse_printf (QSE_T("FORWARD: CLEARING REQCON FOR ERROR\n"));
			qse_mbs_clear (cgi->reqfwdbuf);
		}
		else
		{
			/* normal forwarding */
			qse_ssize_t n;

			if (writable) goto forward;

			n = httpd->cbs->mux.writable (
				httpd, qse_pio_gethandleasubi (&cgi->pio, QSE_PIO_IN), 0);
if (n == 0) qse_printf (QSE_T("FORWARD: @@@@@@@@@NOT WRITABLE\n"));
			if (n >= 1)
			{
			forward:
				/* writable */
qse_printf (QSE_T("FORWARD: @@@@@@@@@@WRITING[%.*hs]\n"),
	(int)QSE_MBS_LEN(cgi->reqfwdbuf),
	QSE_MBS_PTR(cgi->reqfwdbuf));
				n = qse_pio_write (
					&cgi->pio, QSE_PIO_IN,
					QSE_MBS_PTR(cgi->reqfwdbuf),
					QSE_MBS_LEN(cgi->reqfwdbuf)
				);
/* TODO: improve performance.. instead of copying the remaing part 
to the head all the time..  grow the buffer to a certain limit. */
				if (n > 0) qse_mbs_del (cgi->reqfwdbuf, 0, n);
			}

			if (n <= -1)
			{
qse_printf (QSE_T("FORWARD: @@@@@@@@WRITE TO CGI FAILED\n"));
/* TODO: logging ... */
				cgi->reqfwderr = 1;
				qse_mbs_clear (cgi->reqfwdbuf); 
				if (cgi->req) 
				{
					qse_htre_discardcontent (cgi->req);
					/* NOTE: cgi->req may be set to QSE_NULL
					 *       in cgi_snatch_content() triggered by
					 *       qse_htre_discardcontent() */
				}
			}
		}
	}
	else if (cgi->req == QSE_NULL)
	{
		/* there is nothing to read from the client side and
		 * there is nothing more to forward in the forwarding buffer.
		 * clear the relay and write triggers.
		 */
qse_printf (QSE_T("FORWARD: @@@@@@@@NOTHING MORE TO WRITE TO CGI\n"));
		task->trigger[1].mask = 0;
		task->trigger[2].mask = 0;
	}
}

static int task_init_cgi (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_cgi_t* cgi;
	task_cgi_arg_t* arg;
	qse_size_t content_length;
	qse_size_t len;
	const qse_mchar_t* ptr;

	cgi = (task_cgi_t*)qse_httpd_gettaskxtn (httpd, task);
	arg = (task_cgi_arg_t*)task->ctx;

/* TODO: can content length be a different type???
 *  maybe qse_uintmax_t.... it thinks the data size can be larger than the max pointer size 
 * qse_htre_t and qse_htrd_t also needs changes to support it
 */

	QSE_MEMSET (cgi, 0, QSE_SIZEOF(*cgi));
	qse_mbscpy ((qse_mchar_t*)(cgi + 1), arg->path);
	cgi->path = (qse_mchar_t*)(cgi + 1);
	cgi->version = *qse_htre_getversion(arg->req);
	cgi->keepalive = arg->req->attr.keepalive;
	cgi->nph = arg->nph;

	if (arg->req->state & QSE_HTRE_DISCARDED) 
	{
		content_length = 0;
		goto done;
	}

	len = qse_htre_getcontentlen(arg->req);
	if ((arg->req->state & QSE_HTRE_COMPLETED) && len <= 0)
	{
		/* the content part is completed and no content 
		 * in the content buffer. there is nothing to forward */
		content_length = 0;
		goto done;
	}

	if (!(arg->req->state & QSE_HTRE_COMPLETED) &&
	    !arg->req->attr.content_length_set)
	{
		/* if the request is not completed and doesn't have
		 * content-length set, it's not really possible to
		 * pass the content. this function, however, allows
		 * such a request to entask a cgi script dropping the
		 * content */
		qse_htre_discardcontent (arg->req);
		content_length = 0;		
	}
	else
	{	
		/* create a buffer to hold request content from the client
		 * and copy content received already */
		cgi->reqfwdbuf = qse_mbs_open (httpd->mmgr, 0, (len < 512? 512: len));
		if (cgi->reqfwdbuf == QSE_NULL) goto oops;

		ptr = qse_htre_getcontentptr(arg->req);
		if (qse_mbs_ncpy (cgi->reqfwdbuf, ptr, len) == (qse_size_t)-1) 
		{
			qse_mbs_close (cgi->reqfwdbuf);
			cgi->reqfwdbuf = QSE_NULL;
			goto oops;
		}

		if (arg->req->state & QSE_HTRE_COMPLETED)
		{
			/* no furthur forwarding is needed. 
			 * even a chunked request entaksed when completed 
			 * should reach here. if content-length is set
			 * the length should match len. */
			QSE_ASSERT (len > 0);
			QSE_ASSERT (!arg->req->attr.content_length_set ||
			            (arg->req->attr.content_length_set && arg->req->attr.content_length == len));
			content_length = len;
		}
		else
		{
			/* CGI entasking is invoked probably from the peek handler
			 * that was triggered after the request header is received.
			 * you can know this because the request is not completed.
			 * In this case, arrange to forward content
			 * bypassing the buffer in the request object itself. */

/* TODO: callback chain instead of a single pointer??? 
       if the request is already set up with a callback, something will go wrong.
*/
			/* set up a callback to be called when the request content
			 * is fed to the htrd reader. qse_htre_addcontent() that 
			 * htrd calls invokes this callback. */
			cgi->req = arg->req;
			qse_htre_setconcb (cgi->req, cgi_snatch_content, task);

			QSE_ASSERT (arg->req->attr.content_length_set);
			content_length = arg->req->attr.content_length;
		}
	}

done:
	cgi->env = makecgienv (httpd, client, arg->req, arg->path, content_length);
	if (cgi->env == QSE_NULL) goto oops;

	/* no triggers yet since the main loop doesn't allow me to set 
	 * triggers in the task initializer. however the main task handler
	 * will be invoked so long as the client handle is writable by
	 * the main loop. */

	task->ctx = cgi;
	return 0;

oops:
	/* since a new task can't be added in the initializer,
	 * i mark that initialization failed and let task_main_cgi()
	 * add an error task */
	cgi->init_failed = 1;
	task->ctx = cgi;
	return 0;
}

static void task_fini_cgi (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_cgi_t* cgi = (task_cgi_t*)task->ctx;
	if (cgi->env) qse_env_close (cgi->env);
	if (cgi->pio_inited) 
	{
		/* kill cgi in case it is still alive.
		 * qse_pio_wait() in qse_pio_fini() can block. */
		qse_pio_kill (&cgi->pio); 
		qse_pio_fini (&cgi->pio);
	}
	if (cgi->res) qse_mbs_close (cgi->res);
	if (cgi->htrd) qse_htrd_close (cgi->htrd);
	if (cgi->reqfwdbuf) qse_mbs_close (cgi->reqfwdbuf);
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

	QSE_ASSERT (cgi->pio_inited);

	if (task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		cgi_forward_content (httpd, task, 0);
	}
	else if (task->trigger[1].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		cgi_forward_content (httpd, task, 1);
	}

qse_printf (QSE_T("task_main_cgi_5\n"));
	if (cgi->buflen > 0)
	{
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
	}

	/* if forwarding didn't finish, something is not really right... 
	 * so long as the output from CGI is finished, no more forwarding
	 * is performed */
	return (cgi->buflen > 0 || cgi->req ||
	        (cgi->reqfwdbuf && QSE_MBS_LEN(cgi->reqfwdbuf) > 0))? 1: 0;
}

static int task_main_cgi_4 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_cgi_t* cgi = (task_cgi_t*)task->ctx;
	qse_ssize_t n;
	
	QSE_ASSERT (cgi->pio_inited);

qse_printf (QSE_T("task_main_cgi_4 trigger[0].mask=%d trigger[1].mask=%d trigger[2].mask=%d\n"), 
	task->trigger[0].mask, task->trigger[1].mask, task->trigger[2].mask);

	if (task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		cgi_forward_content (httpd, task, 0);
	}
	else if (task->trigger[1].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		cgi_forward_content (httpd, task, 1);
	}

	if (task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
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
					&cgi->pio, QSE_PIO_OUT,
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
					task->trigger[0].mask = 0;

					cgi->buf[cgi->buflen++] = QSE_MT('0');
					cgi->buf[cgi->buflen++] = QSE_MT('\r');
					cgi->buf[cgi->buflen++] = QSE_MT('\n');
					cgi->buf[cgi->buflen++] = QSE_MT('\r');
					cgi->buf[cgi->buflen++] = QSE_MT('\n');
	
					task->main = task_main_cgi_5;
					/* ok to chain-call since this task is called
					 * if the client-side is writable */
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
				&cgi->pio, QSE_PIO_OUT,
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
				task->trigger[0].mask = 0;
				task->main = task_main_cgi_5;
				/* ok to chain-call since this task is called
				 * if the client-side is writable */
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
	qse_printf (QSE_T("CGI SEND FAILURE\n"));
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

	if (task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		cgi_forward_content (httpd, task, 0);
	}
	else if (task->trigger[1].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		cgi_forward_content (httpd, task, 1);
	}

	if (task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		count = MAX_SEND_SIZE;
		if (count >= cgi->res_left) count = cgi->res_left;

qse_printf (QSE_T("[cgi_3 sending %d bytes]\n"), (int)count);
		httpd->errnum = QSE_HTTPD_ENOERR;
		n = httpd->cbs->client.send (httpd, client, cgi->res_ptr, count);

		if (n <= -1) 
		{
qse_printf (QSE_T("[cgi-3 send failure....\n"));
			return -1;
		}

		cgi->res_left -= n;
		if (cgi->res_left <= 0) 
		{
qse_printf (QSE_T("[switching to cgi-4....\n"));
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
	
	QSE_ASSERT (cgi->pio_inited);

{
qse_ntime_t now;
qse_gettime(&now);
qse_printf (QSE_T("[cgi_2 at %lld]\n"), now);
}
	if (task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
qse_printf (QSE_T("[cgi_2 write]\n"));
		cgi_forward_content (httpd, task, 0);
	}
	else if (task->trigger[1].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		cgi_forward_content (httpd, task, 1);
	}

	if (task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
qse_printf (QSE_T("[cgi_2 read]\n"));
		 /* <- can i make it non-block?? or use select??? pio_tryread()? */
		n = qse_pio_read (
			&cgi->pio, QSE_PIO_OUT,
			&cgi->buf[cgi->buflen], 
			QSE_SIZEOF(cgi->buf) - cgi->buflen
		);
		if (n <= -1)
		{
			/* can't return internal server error any more... */
/* TODO: logging ... */
			goto oops;
		}
		if (n == 0) 
		{
			/* end of output from cgi before it has seen a header.
			 * the cgi script must be crooked. */
/* TODO: logging */
qse_printf (QSE_T("#####PREMATURE EOF FROM CHILD\n"));
			goto oops;
		}
			
		cgi->buflen += n;

		if (qse_htrd_feed (cgi->htrd, cgi->buf, cgi->buflen) <= -1)
		{
/* TODO: logging */
qse_printf (QSE_T("#####INVALID HEADER FROM FROM CHILD [%.*hs]\n"), (int)cgi->buflen, cgi->buf);
			goto oops;
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
			    qse_httpd_entaskdisconnect (httpd, client, task) == QSE_NULL) 
			{
				goto oops;
			}

			cgi->res_ptr = QSE_MBS_PTR(cgi->res);
			cgi->res_left = QSE_MBS_LEN(cgi->res);

qse_printf (QSE_T("TRAILING DATA=[%.*hs]\n"), (int)QSE_MBS_LEN(cgi->res), QSE_MBS_PTR(cgi->res));
			task->main = task_main_cgi_3;
			/* ok to chain-call since this task is called
			 * only if the client-side is writable */
			return task_main_cgi_3 (httpd, client, task);
		}

	}

	/* complete headers not seen yet. i need to be called again */
	return 1;

oops:
	return (entask_error (httpd, client, task, 500, &cgi->version, cgi->keepalive) == QSE_NULL)? -1: 0;
}

static int task_main_cgi (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_cgi_t* cgi = (task_cgi_t*)task->ctx;
	int pio_options;
	int http_errnum = 500;

	if (cgi->init_failed) goto oops;

	if (cgi->nph)
	{
		/* i cannot know how long the content will be
		 * since i don't parse the header. so i have to close
		 * the connection regardless of content-length or transfer-encoding
		 * in the actual header. */
		if (qse_httpd_entaskdisconnect (
			httpd, client, task) == QSE_NULL) goto oops;
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

	pio_options = QSE_PIO_READOUT | QSE_PIO_WRITEIN | QSE_PIO_MBSCMD;
	if (httpd->option & QSE_HTTPD_CGIERRTONUL)
		pio_options |= QSE_PIO_ERRTONUL;
	else
		pio_options |= QSE_PIO_ERRTOOUT;
	if (httpd->option & QSE_HTTPD_CGINOCLOEXEC) 
		pio_options |= QSE_PIO_NOCLOEXEC;

	if (qse_pio_init (
		&cgi->pio, httpd->mmgr, (const qse_char_t*)cgi->path, 
		cgi->env, pio_options) <= -1)
	{
		qse_pio_errnum_t errnum;

		errnum = qse_pio_geterrnum (&cgi->pio);

		if (errnum == QSE_PIO_ENOENT) http_errnum = 404;
		else if (errnum == QSE_PIO_EACCES) http_errnum = 403;

		goto oops;
	}
	cgi->pio_inited = 1;
	
	/* set the trigger that the main loop can use this
	 * handle for multiplexing 
	 *
	 * it the output from the child is available, this task
	 * writes it back to the client. so add a trigger for
	 * checking the data availability from the child process */
	task->trigger[0].mask = QSE_HTTPD_TASK_TRIGGER_READ;
	task->trigger[0].handle = qse_pio_gethandleasubi (&cgi->pio, QSE_PIO_OUT);
	if (cgi->reqfwdbuf)
	{
		/* the existence of the forwarding buffer leads to a trigger
		 * for checking data availiability from the client side. */

		if (cgi->req)
		{
			/* there are still things to forward from the client-side. 
			 * i can rely on this relay trigger for task invocation. */
			task->trigger[2].mask = QSE_HTTPD_TASK_TRIGGER_READ;
			task->trigger[2].handle = client->handle;
		}
		else if (QSE_MBS_LEN(cgi->reqfwdbuf) > 0)
		{
			/* there's nothing more to read from the client side but
			 * some contents are already read into the forwarding buffer.
			 * this is possible because the main loop can still read 
			 * between the initializer function (task_init_cgi()) and 
			 * this function. so let's forward it initially. */
qse_printf (QSE_T("FORWARDING INITIAL PART OF CONTENT...\n"));
			cgi_forward_content (httpd, task, 0);

			/* if the initial forwarding clears the forwarding 
			 * buffer, there is nothing more to forward. 
			 * (nothing more to read from the client side, nothing 
			 * left in the forwarding buffer). if not, this task should
			 * still be invoked for forwarding.
			 */
			if (QSE_MBS_LEN(cgi->reqfwdbuf) > 0)
			{
				/* since the buffer is not cleared, this task needs
				 * a trigger for invocation. ask the main loop to
				 * invoke this task so long as it is able to write
				 * to the child process */
				task->trigger[1].mask = QSE_HTTPD_TASK_TRIGGER_WRITE;
				task->trigger[1].handle = qse_pio_gethandleasubi (&cgi->pio, QSE_PIO_IN);
			}
		}
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

	return (entask_error (
		httpd, client, task, http_errnum, 
		&cgi->version, cgi->keepalive) == QSE_NULL)? -1: 0;
}

/* TODO: global option or individual paramter for max cgi lifetime 
*        non-blocking pio read ...
*/

static QSE_INLINE qse_httpd_task_t* entask_cgi (
	qse_httpd_t* httpd, qse_httpd_client_t* client,
	qse_httpd_task_t* pred, const qse_mchar_t* path,
	qse_htre_t* req, int nph)
{
	qse_httpd_task_t task;
	task_cgi_arg_t arg;

	arg.path = path;
	arg.req = req;
	arg.nph = nph;

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

qse_httpd_task_t* qse_httpd_entaskcgi (
	qse_httpd_t* httpd, qse_httpd_client_t* client,
	qse_httpd_task_t* pred, const qse_mchar_t* path, qse_htre_t* req)
{
	return entask_cgi (httpd, client, pred, path, req, 0);
}

qse_httpd_task_t* qse_httpd_entasknph (
	qse_httpd_t* httpd, qse_httpd_client_t* client,
	qse_httpd_task_t* pred, const qse_mchar_t* path, qse_htre_t* req)
{
	return entask_cgi (httpd, client, pred, path, req, 1);
}

/*------------------------------------------------------------------------*/

typedef struct task_proxy_arg_t task_proxy_arg_t;
struct task_proxy_arg_t 
{
	qse_nwad_t peer_nwad;
	qse_htre_t* req;
	int nph;
};

typedef struct task_proxy_t task_proxy_t;
struct task_proxy_t
{
	int init_failed;

	const qse_mchar_t* host;
	qse_http_version_t version;
	int keepalive; /* taken from the request */

	qse_htrd_t* htrd;

	qse_httpd_peer_t peer;
#define PEER_OPEN      (1 << 0)
#define PEER_CONNECTED (1 << 1)
	int peer_status;

	qse_htre_t*  req; /* original request associated with this */
	qse_mbs_t*   reqfwdbuf; /* content from the request */
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

typedef struct proxy_htrd_xtn_t proxy_htrd_xtn_t;
struct proxy_htrd_xtn_t
{
	task_proxy_t* proxy;
};

static int proxy_snatch_content (
	qse_htre_t* req, const qse_mchar_t* ptr, qse_size_t len, void* ctx)
{
	qse_httpd_task_t* task;
	task_proxy_t* proxy; 

	task = (qse_httpd_task_t*)ctx;
	proxy = (task_proxy_t*)task->ctx;

if (ptr) qse_printf (QSE_T("!!!PROXY SNATCHING [%.*hs]\n"), len, ptr);
else qse_printf (QSE_T("!!!PROXY SNATCHING DONE\n"));

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
		proxy->req = QSE_NULL; 

		/* since there is no more to read from the client side.
		 * the relay trigger is not needed any more. */
		task->trigger[2].mask = 0;

		if (QSE_MBS_LEN(proxy->reqfwdbuf) > 0 && 
		    (proxy->peer_status & PEER_CONNECTED) &&
		    !(task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_WRITE))
		{
			/* there's nothing more to read from the client side.
			 * there's something to forward in the forwarding buffer.
			 * but no write trigger is set. add the write trigger 
			 * for task invocation. */
			task->trigger[0].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
		}
	}
	else if (!proxy->reqfwderr)
	{
		/* we can write to the peer if a forwarding error 
		 * didn't occur previously. we store data from the client side
		 * to the forwaring buffer only if there's no such previous
		 * error. if an error occurred, we simply drop the data. */
		if (qse_mbs_ncat (proxy->reqfwdbuf, ptr, len) == (qse_size_t)-1)
		{
			return -1;
		}
qse_printf (QSE_T("!!!PROXY SNATCHED [%.*hs]\n"), len, ptr);
	}

	return 0;
}

static int proxy_htrd_peek_request (qse_htrd_t* htrd, qse_htre_t* req)
{
	proxy_htrd_xtn_t* xtn = (proxy_htrd_xtn_t*) qse_htrd_getxtn (htrd);
	task_proxy_t* proxy = xtn->proxy;

	/* add initial line and headers to proxy->res */
	if (qse_mbs_cat (proxy->res, QSE_MT("\r\n")) == (qse_size_t)-1) return -1; 

	/* add any contents received so far to cgi->res */
	if (qse_mbs_ncat (proxy->res, qse_htre_getcontentptr(req), qse_htre_getcontentlen(req)) == (qse_size_t)-1) return -1;

	return 0;
}

static qse_htrd_recbs_t proxy_htrd_cbs =
{
	proxy_htrd_peek_request,
	QSE_NULL /* not needed for proxy */
};

static void proxy_forward_content (
	qse_httpd_t* httpd, qse_httpd_task_t* task, int writable)
{
	task_proxy_t* proxy = (task_proxy_t*)task->ctx;

	QSE_ASSERT (proxy->reqfwdbuf != QSE_NULL);

	if (QSE_MBS_LEN(proxy->reqfwdbuf) > 0)
	{
		/* there is something to forward in the forwarding buffer. */

		if (proxy->reqfwderr) 
		{
			/* a forwarding error has occurred previously.
			 * clear the forwarding buffer */
qse_printf (QSE_T("FORWARD: CLEARING REQCON FOR ERROR\n"));
			qse_mbs_clear (proxy->reqfwdbuf);
		}
		else
		{
			/* normal forwarding */
			qse_ssize_t n;

			if (writable) goto forward;

			n = httpd->cbs->mux.writable (httpd, proxy->peer.handle, 0);
if (n == 0) qse_printf (QSE_T("PROXY FORWARD: @@@@@@@@@NOT WRITABLE\n"));
			if (n >= 1)
			{
			forward:
				/* writable */
qse_printf (QSE_T("PROXY FORWARD: @@@@@@@@@@WRITING[%.*hs]\n"),
	(int)QSE_MBS_LEN(proxy->reqfwdbuf),
	QSE_MBS_PTR(proxy->reqfwdbuf));
				n = httpd->cbs->peer.send (
					httpd, &proxy->peer,
					QSE_MBS_PTR(proxy->reqfwdbuf),
					QSE_MBS_LEN(proxy->reqfwdbuf)
				);

/* TODO: improve performance.. instead of copying the remaing part 
to the head all the time..  grow the buffer to a certain limit. */
				if (n > 0) qse_mbs_del (proxy->reqfwdbuf, 0, n);
			}

			if (n <= -1)
			{
qse_printf (QSE_T("PROXY FORWARD: @@@@@@@@WRITE TO PROXY FAILED\n"));
/* TODO: logging ... */
				proxy->reqfwderr = 1;
				qse_mbs_clear (proxy->reqfwdbuf); 
				if (proxy->req) 
				{
					qse_htre_discardcontent (proxy->req);
					/* NOTE: proxy->req may be set to QSE_NULL
					 *       in proxy_snatch_content() triggered by
					 *       qse_htre_discardcontent() */
				}
			}
		}
	}
	else if (proxy->req == QSE_NULL)
	{
		/* there is nothing to read from the client side and
		 * there is nothing more to forward in the forwarding buffer.
		 * clear the relay and write triggers.
		 */
qse_printf (QSE_T("FORWARD: @@@@@@@@NOTHING MORE TO WRITE TO PROXY\n"));
		task->trigger[2].mask = 0;
	}
}


static int task_init_proxy (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_proxy_t* proxy;
	task_proxy_arg_t* arg;
	qse_size_t content_length;
	qse_size_t len;
	const qse_mchar_t* ptr;

	proxy = (task_proxy_t*)qse_httpd_gettaskxtn (httpd, task);
	arg = (task_proxy_arg_t*)task->ctx;

/* TODO: can content length be a different type???
 *  maybe qse_uintmax_t.... it thinks the data size can be larger than the max pointer size 
 * qse_htre_t and qse_htrd_t also needs changes to support it
 */

	QSE_MEMSET (proxy, 0, QSE_SIZEOF(*proxy));
	proxy->version = *qse_htre_getversion(arg->req);
	proxy->keepalive = arg->req->attr.keepalive;
	proxy->peer.nwad = arg->peer_nwad;

/* -------------------------------------------------------------------- 
 * TODO: compose headers to send to peer and push them to fwdbuf... 
 * TODO: also change the content length check logic below...
 * -------------------------------------------------------------------- */

	if (arg->req->state & QSE_HTRE_DISCARDED) 
	{
		content_length = 0;
		goto done;
	}

	len = qse_htre_getcontentlen(arg->req);
	if ((arg->req->state & QSE_HTRE_COMPLETED) && len <= 0)
	{
		/* the content part is completed and no content 
		 * in the content buffer. there is nothing to forward */
		content_length = 0;
		goto done;
	}

	if (!(arg->req->state & QSE_HTRE_COMPLETED) &&
	    !arg->req->attr.content_length_set)
	{
		/* if the request is not completed and doesn't have
		 * content-length set, it's not really possible to
		 * pass the content. this function, however, allows
		 * such a request to entask a proxy script dropping the
		 * content */
		qse_htre_discardcontent (arg->req);
		content_length = 0;		
	}
	else
	{	
		/* create a buffer to hold request content from the client
		 * and copy content received already */
		proxy->reqfwdbuf = qse_mbs_open (httpd->mmgr, 0, (len < 512? 512: len));
		if (proxy->reqfwdbuf == QSE_NULL) goto oops;

		ptr = qse_htre_getcontentptr(arg->req);
		if (qse_mbs_ncpy (proxy->reqfwdbuf, ptr, len) == (qse_size_t)-1) 
		{
			qse_mbs_close (proxy->reqfwdbuf);
			proxy->reqfwdbuf = QSE_NULL;
			goto oops;
		}


		if (arg->req->state & QSE_HTRE_COMPLETED)
		{
			/* no furthur forwarding is needed. 
			 * even a chunked request entaksed when completed 
			 * should reach here. if content-length is set
			 * the length should match len. */
			QSE_ASSERT (len > 0);
			QSE_ASSERT (!arg->req->attr.content_length_set ||
			            (arg->req->attr.content_length_set && arg->req->attr.content_length == len));
			content_length = len;
		}
		else
		{
			/* CGI entasking is invoked probably from the peek handler
			 * that was triggered after the request header is received.
			 * you can know this because the request is not completed.
			 * In this case, arrange to forward content
			 * bypassing the buffer in the request object itself. */

/* TODO: callback chain instead of a single pointer??? 
       if the request is already set up with a callback, something will go wrong.
*/
			/* set up a callback to be called when the request content
			 * is fed to the htrd reader. qse_htre_addcontent() that 
			 * htrd calls invokes this callback. */
			proxy->req = arg->req;
			qse_htre_setconcb (proxy->req, proxy_snatch_content, task);

			QSE_ASSERT (arg->req->attr.content_length_set);
			content_length = arg->req->attr.content_length;
		}
	}

done:
	/* no triggers yet since the main loop doesn't allow me to set 
	 * triggers in the task initializer. however the main task handler
	 * will be invoked so long as the client handle is writable by
	 * the main loop. */

	task->ctx = proxy;
	return 0;

oops:
	/* since a new task can't be added in the initializer,
	 * i mark that initialization failed and let task_main_proxy()
	 * add an error task */
	proxy->init_failed = 1;
	task->ctx = proxy;
	return 0;
}

static void task_fini_proxy (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_proxy_t* proxy = (task_proxy_t*)task->ctx;

	if (proxy->peer_status & PEER_OPEN) 
		httpd->cbs->peer.close (httpd, &proxy->peer);
	if (proxy->res) qse_mbs_close (proxy->res);
	if (proxy->htrd) qse_htrd_close (proxy->htrd);
	if (proxy->reqfwdbuf) qse_mbs_close (proxy->reqfwdbuf);
	if (proxy->req) qse_htre_unsetconcb (proxy->req);
}

static int task_main_proxy_5 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_proxy_t* proxy = (task_proxy_t*)task->ctx;
	qse_ssize_t n;

	QSE_ASSERT (proxy->pio_inited);

	if (task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		proxy_forward_content (httpd, task, 0);
	}
	else if (task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		proxy_forward_content (httpd, task, 1);
	}

qse_printf (QSE_T("task_main_proxy_5\n"));
	if (proxy->buflen > 0)
	{
/* TODO: check if proxy outputs more than content-length if it is set... */
		httpd->errnum = QSE_HTTPD_ENOERR;
		n = httpd->cbs->client.send (httpd, client, proxy->buf, proxy->buflen);
		if (n <= -1)
		{
		/* can't return internal server error any more... */
/* TODO: logging ... */
			return -1;
		}

		QSE_MEMCPY (&proxy->buf[0], &proxy->buf[n], proxy->buflen - n);
		proxy->buflen -= n;
	}

	/* if forwarding didn't finish, something is not really right... 
	 * so long as the output from CGI is finished, no more forwarding
	 * is performed */
	return (proxy->buflen > 0 || proxy->req ||
	        (proxy->reqfwdbuf && QSE_MBS_LEN(proxy->reqfwdbuf) > 0))? 1: 0;
}

static int task_main_proxy_4 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_proxy_t* proxy = (task_proxy_t*)task->ctx;
	qse_ssize_t n;
	
	QSE_ASSERT (proxy->pio_inited);

qse_printf (QSE_T("task_main_proxy_4 trigger[0].mask=%d trigger[1].mask=%d trigger[2].mask=%d\n"), 
	task->trigger[0].mask, task->trigger[1].mask, task->trigger[2].mask);

	if (task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		proxy_forward_content (httpd, task, 0);
	}
	else if (task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		proxy_forward_content (httpd, task, 1);
	}

	if (task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		/* this function assumes that the chunk length does not exceeded 
		 * 4 hexadecimal digits. */
		QSE_ASSERT (QSE_SIZEOF(proxy->buf) <= 0xFFFF);
	
	qse_printf (QSE_T("task_main_proxy_4\n"));
	
		n = httpd->cbs->peer.recv (
			httpd, &proxy->peer,
			&proxy->buf[proxy->buflen], 
			QSE_SIZEOF(proxy->buf) - proxy->buflen
		);
		if (n <= -1)
		{
			/* can't return internal server error any more... */
	/* TODO: loggig ... */
			return -1;
		}
		if (n == 0)
		{
			task->trigger[0].mask = 0;
			task->main = task_main_proxy_5;
			/* ok to chain-call since this task is called
			 * if the client-side is writable */
			return task_main_proxy_5 (httpd, client, task);
		}

		proxy->buflen += n;
		proxy->content_received += n;
	
		if (proxy->content_length_set && 
		    proxy->content_received > proxy->content_length)
		{
	/* TODO: proxy returning too much data... something is wrong in CGI */
	qse_printf (QSE_T("CGI FUCKED UP...RETURNING TOO MUCH DATA\n"));
			return -1;
		}
	
#if 0
qse_printf (QSE_T("CGI_4 SEND [%.*hs]\n"), (int)proxy->buflen, proxy->buf);
#endif
		httpd->errnum = QSE_HTTPD_ENOERR;
		n = httpd->cbs->client.send (httpd, client, proxy->buf, proxy->buflen);
		if (n <= -1)
		{
			/* can't return internal server error any more... */
	/* TODO: logging ... */
	qse_printf (QSE_T("CGI SEND FAILURE\n"));
			return -1;
		}
	
		QSE_MEMCPY (&proxy->buf[0], &proxy->buf[n], proxy->buflen - n);
		proxy->buflen -= n;
	
#if 0
qse_printf (QSE_T("CGI SEND DONE\n"));
#endif
	}

	return 1;
}

static int task_main_proxy_3 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	/* send the http initial line and headers built using the headers
	 * returned by peer. it may include some contents as well */

	task_proxy_t* proxy = (task_proxy_t*)task->ctx;
	qse_ssize_t n;
	qse_size_t count;

	if (task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		proxy_forward_content (httpd, task, 0);
	}
	else if (task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		proxy_forward_content (httpd, task, 1);
	}

	if (task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		count = MAX_SEND_SIZE;
		if (count >= proxy->res_left) count = proxy->res_left;

qse_printf (QSE_T("[proxy_3 sending %d bytes]\n"), (int)count);
		httpd->errnum = QSE_HTTPD_ENOERR;
		n = httpd->cbs->client.send (httpd, client, proxy->res_ptr, count);

		if (n <= -1) 
		{
qse_printf (QSE_T("[proxy-3 send failure....\n"));
			return -1;
		}

		proxy->res_left -= n;
		if (proxy->res_left <= 0) 
		{
qse_printf (QSE_T("[switching to proxy-4....\n"));
			task->main = task_main_proxy_4;
			/* don't chain-call task_main_proxy_4 since it has another send
			 * and it has already been sent here. so the writability must
			 * be checked again in the main loop.
			 * => return task_main_proxy_4 (httpd, client, task);*/
			return 1;
		}

		proxy->res_ptr += n;
	}

	return 1; /* more work to do */
}


static int task_main_proxy_2 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_proxy_t* proxy = (task_proxy_t*)task->ctx;
	qse_ssize_t n;
	qse_size_t count;

	if (task->trigger[2].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		proxy_forward_content (httpd, task, 0);
	}
	else if (task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		proxy_forward_content (httpd, task, 1);
	}

	if (task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_READABLE)
	{
		/* there is something to read from peer */
		n = httpd->cbs->peer.recv (
			httpd, &proxy->peer,
			&proxy->buf[proxy->buflen], 
			QSE_SIZEOF(proxy->buf) - proxy->buflen
		);
		if (n <= -1)
		{
			/* can't return internal server error any more... */
/* TODO: logging ... */
			goto oops;
		}
		if (n == 0) 
		{
			/* end of output from peer before it has seen a header.
			 * the proxy script must be crooked. */
/* TODO: logging */
qse_printf (QSE_T("#####PREMATURE EOF FROM PEER\n"));
			goto oops;
		}
			
		proxy->buflen += n;

		if (qse_htrd_feed (proxy->htrd, proxy->buf, proxy->buflen) <= -1)
		{
/* TODO: logging */
qse_printf (QSE_T("#####INVALID HEADER FROM FROM PEER [%.*hs]\n"), (int)proxy->buflen, proxy->buf);
			goto oops;
		}

		proxy->buflen = 0;


		if (QSE_MBS_LEN(proxy->res) > 0)
		{
			if (proxy->disconnect &&
			    qse_httpd_entaskdisconnect (httpd, client, task) == QSE_NULL) 
			{
				goto oops;
			}

			proxy->res_ptr = QSE_MBS_PTR(proxy->res);
			proxy->res_left = QSE_MBS_LEN(proxy->res);

qse_printf (QSE_T("TRAILING DATA=[%.*hs]\n"), (int)QSE_MBS_LEN(proxy->res), QSE_MBS_PTR(proxy->res));
			task->main = task_main_proxy_3;
			/* ok to chain-call since this task is called
			 * only if the client-side is writable */
			return task_main_proxy_3 (httpd, client, task);
		}
	}

	/* complete headers not seen yet. i need to be called again */
	return 1;

oops:
	return (entask_error (httpd, client, task, 500, &proxy->version, proxy->keepalive) == QSE_NULL)? -1: 0;
}

static int task_main_proxy_0 (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_proxy_t* proxy = (task_proxy_t*)task->ctx;
	int n;

	/* wait for peer to get connected */

	if (task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_READABLE ||
	    task->trigger[0].mask & QSE_HTTPD_TASK_TRIGGER_WRITABLE)
	{
		n = httpd->cbs->peer.connected (httpd, &proxy->peer);
		if (n <= -1) return -1;
		if (n >= 1) 
		{
			proxy->peer_status |= PEER_CONNECTED;
			task->trigger[0].mask &= ~QSE_HTTPD_TASK_TRIGGER_WRITE;
			if (proxy->reqfwdbuf)
			{
				if (proxy->req)
				{
					task->trigger[2].mask = QSE_HTTPD_TASK_TRIGGER_READ;
					task->trigger[2].handle = client->handle;
				}
				else if (QSE_MBS_LEN(proxy->reqfwdbuf) > 0)
				{
					proxy_forward_content (httpd, task, 0);
					if (QSE_MBS_LEN(proxy->reqfwdbuf) > 0)
					{
						task->trigger[0].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
					}
				}
			}
qse_printf (QSE_T("FINALLY connected to peer ...............................\n"));
qse_printf (QSE_T("FINALLY connected to peer ...............................\n"));
qse_printf (QSE_T("FINALLY connected to peer ...............................\n"));
qse_printf (QSE_T("FINALLY connected to peer ...............................\n"));
			task->main = task_main_proxy_2;
		}
	}

	return 1;
}

static int task_main_proxy (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_proxy_t* proxy = (task_proxy_t*)task->ctx;
	proxy_htrd_xtn_t* xtn;
	int http_errnum = 500;
	int n;

	if (proxy->init_failed) goto oops;

	proxy->htrd = qse_htrd_open (httpd->mmgr, QSE_SIZEOF(proxy_htrd_xtn_t));
	if (proxy->htrd == QSE_NULL) goto oops;
	xtn = (proxy_htrd_xtn_t*) qse_htrd_getxtn (proxy->htrd);
	xtn->proxy = proxy;
	qse_htrd_setrecbs (proxy->htrd, &proxy_htrd_cbs);
	qse_htrd_setoption (
		proxy->htrd, 
		QSE_HTRD_PEEKONLY | 
		QSE_HTRD_REQUEST
	);

	proxy->res = qse_mbs_open (httpd->mmgr, 0, 256);
	if (proxy->res == QSE_NULL) goto oops;

	httpd->errnum = QSE_HTTPD_ENOERR;
	n = httpd->cbs->peer.open (httpd, &proxy->peer);
	if (n <= -1)
	{
/* TODO: translate error code to http error... */
		if (httpd->errnum == QSE_HTTPD_ENOENT) http_errnum = 404;
		else if (httpd->errnum == QSE_HTTPD_EACCES) http_errnum = 403;
		goto oops;
	}

	proxy->peer_status |= PEER_OPEN;
	task->trigger[0].mask = QSE_HTTPD_TASK_TRIGGER_READ;
	task->trigger[0].handle = proxy->peer.handle;

	if (n == 0)
	{
		/* peer not connected yet */
		task->trigger[0].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
		task->main = task_main_proxy_0;
	}
	else
	{
		/* peer connected already */
		proxy->peer_status |= PEER_CONNECTED;
		if (proxy->reqfwdbuf)
		{
			if (proxy->req)
			{
				task->trigger[2].mask = QSE_HTTPD_TASK_TRIGGER_READ;
				task->trigger[2].handle = client->handle;
			}
			else if (QSE_MBS_LEN(proxy->reqfwdbuf) > 0)
			{
				proxy_forward_content (httpd, task, 0);
			
				if (QSE_MBS_LEN(proxy->reqfwdbuf) > 0)
				{
					task->trigger[0].mask |= QSE_HTTPD_TASK_TRIGGER_WRITE;
				}
			}
		}
		task->main = task_main_proxy_2;
	}

	return 1;

oops:
	if (proxy->res) 
	{
		qse_mbs_close (proxy->res);
		proxy->res = QSE_NULL;
	}
	if (proxy->htrd) 
	{
		qse_htrd_close (proxy->htrd);
		proxy->htrd = QSE_NULL;
	}

	return (entask_error (
		httpd, client, task, http_errnum, 
		&proxy->version, proxy->keepalive) == QSE_NULL)? -1: 0;
}

qse_httpd_task_t* qse_httpd_entaskproxy (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client,
	const qse_httpd_task_t* pred, 
	const qse_nwad_t* nwad,
	const qse_htre_t* req)
{
	qse_httpd_task_t task;
	task_proxy_arg_t arg;

	arg.peer_nwad = *nwad;
	arg.req = req;

	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = task_init_proxy;
	task.fini = task_fini_proxy;
	task.main = task_main_proxy;
	task.ctx = &arg;

	return qse_httpd_entask (
		httpd, client, pred, &task, QSE_SIZEOF(task_proxy_t)
	);
}

/*------------------------------------------------------------------------*/

#endif
