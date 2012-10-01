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
#include <qse/cmn/str.h>
#include <qse/cmn/fmt.h>
#include "../cmn/mem.h"


#include <stdarg.h>
#include <qse/cmn/stdio.h> /* TODO: remove this */


/* TODO:
 * many functions in this file use qse_size_t.
 * so the size data transfers is limited by this type.
 * break this barrier... */

/*------------------------------------------------------------------------*/

static int task_main_disconnect (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	httpd->scb->client.shutdown (httpd, client);
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

	n = httpd->scb->client.send (httpd, client, ctx->ptr, count);
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
		QSE_MT("HTTP/%d.%d %d %s\r\nServer: %s\r\nDate: %s\r\nConnection: %s\r\nContent-Type: text/html\r\nContent-Length: %lu\r\n\r\n%s\r\n\r\n"), 
		version->major, version->minor, code, smsg,
		qse_httpd_getname (httpd),
		qse_httpd_fmtgmtimetobb (httpd, QSE_NULL, 0),
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
		QSE_MT("HTTP/%d.%d 401 Unauthorized\r\nServer: %s\r\nDate: %s\r\nConnection: %s\r\nWWW-Authenticate: Basic realm=\"%s\"\r\nContent-Type: text/html\r\nContent-Length: %lu\r\n\r\n%s\r\n\r\n"),
		version->major, version->minor, 
		qse_httpd_getname (httpd),
		qse_httpd_fmtgmtimetobb (httpd, QSE_NULL, 0),
		((req->attr.flags & QSE_HTRE_ATTR_KEEPALIVE)? QSE_MT("keep-alive"): QSE_MT("close")),
		realm, (unsigned long)qse_mbslen(lmsg) + 4, lmsg);
}

/*------------------------------------------------------------------------*/

qse_httpd_task_t* qse_httpd_entaskreloc (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_httpd_task_t* pred, const qse_mchar_t* dst, qse_htre_t* req)
{
	const qse_http_version_t* version;

	version = qse_htre_getversion(req);

	return qse_httpd_entaskformat (
		httpd, client, pred,
		QSE_MT("HTTP/%d.%d 301 Moved Permanently\r\nServer: %s\r\nDate: %s\r\nContent-Length: 0\r\nConnection: %s\r\nLocation: %s\r\n\r\n"),
		version->major, version->minor,
		qse_httpd_getname (httpd),
		qse_httpd_fmtgmtimetobb (httpd, QSE_NULL, 0),
		((req->attr.flags & QSE_HTRE_ATTR_KEEPALIVE)? QSE_MT("keep-alive"): QSE_MT("close")),
		dst);
}

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


qse_httpd_task_t* qse_httpd_entaskrsrc (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client,
	qse_httpd_task_t* pred, 
	qse_httpd_rsrc_t* rsrc,
	qse_htre_t* req)
{
	qse_httpd_task_t* task;

	switch (rsrc->type)
	{
		case QSE_HTTPD_RSRC_AUTH:
			task = qse_httpd_entaskauth (httpd, client, QSE_NULL, rsrc->u.auth.realm, req);
			break;

		case QSE_HTTPD_RSRC_CGI:
			task = qse_httpd_entaskcgi (
				httpd, client, QSE_NULL, rsrc->u.cgi.path,
				rsrc->u.cgi.script, rsrc->u.cgi.suffix, 
				rsrc->u.cgi.docroot, rsrc->u.cgi.nph, req);
			break;

		case QSE_HTTPD_RSRC_DIR:
			qse_httpd_discardcontent (httpd, req);
			task = qse_httpd_entaskdir (httpd, client, QSE_NULL, rsrc->u.dir.path, req);
			break;

		case QSE_HTTPD_RSRC_ERROR:
			qse_httpd_discardcontent (httpd, req);
			task = qse_httpd_entaskerror (httpd, client, QSE_NULL, rsrc->u.error.code, req);
			break;

		case QSE_HTTPD_RSRC_FILE:
			qse_httpd_discardcontent (httpd, req);
			task = qse_httpd_entaskfile (httpd, client, QSE_NULL, rsrc->u.file.path, rsrc->u.file.mime, req);
			break;
	
		case QSE_HTTPD_RSRC_PROXY:
			task = qse_httpd_entaskproxy (httpd, client, QSE_NULL, &rsrc->u.proxy.dst, &rsrc->u.proxy.src, req);
			break;

		case QSE_HTTPD_RSRC_RELOC:
			task = qse_httpd_entaskreloc (httpd, client, QSE_NULL, rsrc->u.reloc.dst, req);
			break;

		case QSE_HTTPD_RSRC_TEXT:
			task = qse_httpd_entasktext (httpd, client, QSE_NULL, rsrc->u.text.ptr, rsrc->u.text.mime, req);
			break;

		default:
			qse_httpd_discardcontent (httpd, req);
			task = QSE_NULL;
			httpd->errnum = QSE_HTTPD_EINVAL;
			break;
	}

	return task;
}
