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
#include <qse/cmn/fmt.h>

#include <qse/cmn/stdio.h> /* TODO: remove this */

#include <stdarg.h>
#include <stdio.h>
#include <dirent.h>

/* TODO:
 * many functions in this file use qse_size_t.
 * so the size data transfers is limited by this type.
 * break this barrier... */

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

qse_httpd_task_t* qse_httpd_entask_error (
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

		case 504:
			smsg = QSE_MT("Gateway Timeout");
			lmsg = QSE_MT("<html><head><title>Gateway Timeout</title></head><body><b>GATEWAY TIMEOUT<b></body></html>");
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
	return qse_httpd_entask_error (
		httpd, client, pred, code,
		qse_htre_getversion(req), 
		(req->attr.flags & QSE_HTRE_ATTR_KEEPALIVE));
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
		((req->attr.flags & QSE_HTRE_ATTR_KEEPALIVE)? QSE_MT("keep-alive"): QSE_MT("close")),
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
			x = qse_httpd_entask_error (httpd, client, x, 403, &data->version, data->keepalive);
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
		return (qse_httpd_entask_error (httpd, client, task, 404, &data->version, data->keepalive) == QSE_NULL)? -1: 0;
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
	const qse_htre_hdrval_t* tmp;

	QSE_MEMSET (&data, 0, QSE_SIZEOF(data));
	data.name = name;
	data.version = *qse_htre_getversion(req);
	data.keepalive = req->attr.keepalive;

	tmp = qse_htre_getheaderval(req, QSE_MT("Range"));
	if (tmp) 
	{
		while (tmp->next) tmp = tmp->next; /* get the last value */
		if (qse_parsehttprange (tmp->ptr, &data.range) <= -1)
		{
			return qse_httpd_entask_error (httpd, client, pred, 416, &data.version, data.keepalive);
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
		x = qse_httpd_entask_error (
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
		x = qse_httpd_entask_error (
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
			x = qse_httpd_entask_error (
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
	const qse_htre_hdrval_t* tmp;

	QSE_MEMSET (&data, 0, QSE_SIZEOF(data));
	data.path = path;
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

#if 0
qse_httpd_task_t* qse_httpd_entaskresol (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client,
	qse_httpd_task_t* pred, 
	const qse_mchar_t* host,
	qse_htre_t* req)
{
	qse_httpd_task_t task;
	task_resol_arg_t arg;

	arg.peer_nwad = *nwad;
	arg.req = req;

	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = task_init_resol;
	task.fini = task_fini_resol;
	task.main = task_main_resol;
	task.ctx = &arg;

	return qse_httpd_entask (
		httpd, client, pred, &task, QSE_SIZEOF(task_resol_t)
	);
}
#endif

/*------------------------------------------------------------------------*/

#if 0
qse_httpd_task_t* qse_httpd_entaskconnect (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client,
	qse_httpd_task_t* pred, 
	const qse_nwad_t* nwad,
	qse_htre_t* req)
{
	return -1;
}
#endif

#endif
