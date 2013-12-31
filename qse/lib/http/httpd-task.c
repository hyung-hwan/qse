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
#include <qse/cmn/str.h>
#include "../cmn/mem.h"

/* TODO:
 * many functions in this file use qse_size_t.
 * so the size data transfers is limited by this type.
 * break this barrier... */

/*------------------------------------------------------------------------*/

static int task_main_disconnect (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	httpd->opt.scb.client.shutdown (httpd, client);
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

	n = httpd->opt.scb.client.send (httpd, client, ctx->ptr, count);
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
	qse_mchar_t* buf;
	int bytes_req, l;

	va_start (ap, fmt);
	bytes_req = qse_mbsxvfmt (QSE_NULL, 0, fmt, ap);
	va_end (ap);

	buf = (qse_mchar_t*) qse_httpd_allocmem (
		httpd, (bytes_req + 1) * QSE_SIZEOF(*buf));
	if (buf == QSE_NULL) return QSE_NULL;

	va_start (ap, fmt);
	l = qse_mbsxvfmt (buf, bytes_req + 1, fmt, ap);
	va_end (ap);

	if (l != bytes_req) 
	{
		/* something got wrong ... */
		qse_httpd_freemem (httpd, buf);
		httpd->errnum = QSE_HTTPD_EINTERN;
		return QSE_NULL;
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

	return qse_httpd_entask (
		httpd, client, pred, &task, QSE_SIZEOF(data));
}

/* TODO: send wide character string when QSE_CHAR_IS_WCHAR */

/*------------------------------------------------------------------------*/

typedef struct status_reloc_t status_reloc_t;
struct status_reloc_t
{
	const qse_mchar_t* dst;
	int redir;
};

static qse_httpd_task_t* entask_status (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_httpd_task_t* pred, int code, void* extra,
	qse_http_method_t method, const qse_http_version_t* version, 
	int keepalive)
{
	const qse_mchar_t* msg;

	const qse_mchar_t* extrapre = QSE_MT(""); 
	const qse_mchar_t* extrapst = QSE_MT("");
	const qse_mchar_t* extraval = QSE_MT("");

	qse_mchar_t text[1024]; /* TODO: make this buffer dynamic or scalable */

	text[0] = QSE_MT('\0');
	
	msg = qse_httpstatustombs (code);
	switch (code)
	{
		case 301:
		case 307:
		{
			status_reloc_t* reloc;
			reloc = (status_reloc_t*)extra;
			extrapre = QSE_MT("Location: ");
			extrapst = reloc->redir? QSE_MT("/\r\n"): QSE_MT("\r\n");
			extraval = reloc->dst;
			break;
		}

		case 304:
		case 200:
		case 201:
		case 202:
		case 203:
		case 204:
		case 205:
		case 206:
			/* nothing to do */
			break;	

		default:
			if (method != QSE_HTTP_HEAD &&
			    httpd->opt.rcb.fmterr (httpd, client, code, text, QSE_COUNTOF(text)) <= -1) return QSE_NULL;

			if (code == 401)
			{
				extrapre = QSE_MT("WWW-Authenticate: Basic realm=\"");
				extrapst = QSE_MT("\"\r\n");
				extraval = (const qse_mchar_t*)extra;
			}
			break;
	}

	return qse_httpd_entaskformat (
		httpd, client, pred,
		QSE_MT("HTTP/%d.%d %d %s\r\nServer: %s\r\nDate: %s\r\nConnection: %s\r\nContent-Type: text/html\r\nContent-Length: %u\r\n%s%s%s\r\n%s"),
		version->major, version->minor, 
		code, msg, qse_httpd_getname (httpd),
		qse_httpd_fmtgmtimetobb (httpd, QSE_NULL, 0),
		(keepalive? QSE_MT("keep-alive"): QSE_MT("close")),
		(unsigned int)qse_mbslen(text), /* unsigned int should be large enough since text is small */
		extrapre, extraval, extrapst, text);
}
/*------------------------------------------------------------------------*/

qse_httpd_task_t* qse_httpd_entask_err (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_httpd_task_t* pred, int code,
	qse_http_method_t method, const qse_http_version_t* version, int keepalive)
{
	return entask_status (httpd, client, pred, code, QSE_NULL, method, version, keepalive);
}

qse_httpd_task_t* qse_httpd_entaskerr (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_httpd_task_t* pred, int code, qse_htre_t* req)
{
	return entask_status (
		httpd, client, pred, code, QSE_NULL, 
		qse_htre_getqmethodtype(req), 
		qse_htre_getversion(req), 
		(req->attr.flags & QSE_HTRE_ATTR_KEEPALIVE)
	);
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
	return entask_status (
		httpd, client, pred, 401, (void*)realm, 
		qse_htre_getqmethodtype(req),
		qse_htre_getversion(req), 
		(req->attr.flags & QSE_HTRE_ATTR_KEEPALIVE));
}


/*------------------------------------------------------------------------*/

qse_httpd_task_t* qse_httpd_entaskreloc (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_httpd_task_t* pred, const qse_mchar_t* dst, qse_htre_t* req)
{
	status_reloc_t reloc;

	reloc.dst = dst;
	reloc.redir = 0;

	return entask_status (
		httpd, client, pred, 301, (void*)&reloc,
		qse_htre_getqmethodtype(req), 
		qse_htre_getversion(req), 
		(req->attr.flags & QSE_HTRE_ATTR_KEEPALIVE));
}

qse_httpd_task_t* qse_httpd_entaskredir (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_httpd_task_t* pred, const qse_mchar_t* dst, qse_htre_t* req)
{
	status_reloc_t reloc;

	reloc.dst = dst;
	reloc.redir = 1;

	return entask_status (
		httpd, client, pred, 301, (void*)&reloc,
		qse_htre_getqmethodtype(req), 
		qse_htre_getversion(req), 
		(req->attr.flags & QSE_HTRE_ATTR_KEEPALIVE));
}

/*------------------------------------------------------------------------*/

qse_httpd_task_t* qse_httpd_entask_nomod (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* pred, 
	qse_http_method_t method, const qse_http_version_t* version, int keepalive)
{
	return entask_status (
		httpd, client, pred, 304, QSE_NULL, method, version, keepalive);
}

qse_httpd_task_t* qse_httpd_entasknomod (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_httpd_task_t* pred, qse_htre_t* req)
{
	return entask_status (
		httpd, client, pred, 304, QSE_NULL, 
		qse_htre_getqmethodtype(req), 
		qse_htre_getversion(req), 
		(req->attr.flags & QSE_HTRE_ATTR_KEEPALIVE));
}

/*------------------------------------------------------------------------*/

qse_httpd_task_t* qse_httpd_entaskallow (
	qse_httpd_t* httpd, qse_httpd_client_t* client, 
	qse_httpd_task_t* pred, const qse_mchar_t* allow, qse_htre_t* req)
{
	int code = 200;
	const qse_mchar_t* msg;
	const qse_http_version_t* version;
	int keepalive;

	msg = qse_httpstatustombs (code);
	version = qse_htre_getversion(req);
	keepalive = (req->attr.flags & QSE_HTRE_ATTR_KEEPALIVE);
	return qse_httpd_entaskformat (
		httpd, client, pred,
		QSE_MT("HTTP/%d.%d %d %s\r\nServer: %s\r\nDate: %s\r\nConnection: %s\r\nAllow: %s\r\nContent-Length: 0\r\n\r\n"),
		version->major, version->minor, 
		code, msg, qse_httpd_getname (httpd),
		qse_httpd_fmtgmtimetobb (httpd, QSE_NULL, 0),
		(keepalive? QSE_MT("keep-alive"): QSE_MT("close")),
		allow
	);
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
			task = qse_httpd_entaskauth (httpd, client, pred, rsrc->u.auth.realm, req);
			break;

		case QSE_HTTPD_RSRC_CGI:
			task = qse_httpd_entaskcgi (httpd, client, pred, &rsrc->u.cgi, req);
			break;

		case QSE_HTTPD_RSRC_DIR:
			task = qse_httpd_entaskdir (httpd, client, pred, &rsrc->u.dir, req);
			break;

		case QSE_HTTPD_RSRC_ERR:
			task = qse_httpd_entaskerr (httpd, client, pred, rsrc->u.err.code, req);
			break;

		case QSE_HTTPD_RSRC_FILE:
			task = qse_httpd_entaskfile (httpd, client, pred, rsrc->u.file.path, rsrc->u.file.mime, req);
			break;
	
		case QSE_HTTPD_RSRC_PROXY:
			task = qse_httpd_entaskproxy (httpd, client, pred, &rsrc->u.proxy.dst, &rsrc->u.proxy.src, req);
			break;

		case QSE_HTTPD_RSRC_RELOC:
			task = qse_httpd_entaskreloc (httpd, client, pred, rsrc->u.reloc.dst, req);
			break;

		case QSE_HTTPD_RSRC_REDIR:
			task = qse_httpd_entaskredir (httpd, client, pred, rsrc->u.redir.dst, req);
			break;

		case QSE_HTTPD_RSRC_TEXT:
			task = qse_httpd_entasktext (httpd, client, pred, rsrc->u.text.ptr, rsrc->u.text.mime, req);
			break;

		default:
			qse_httpd_discardcontent (httpd, req);
			task = QSE_NULL;
			httpd->errnum = QSE_HTTPD_EINVAL;
			break;
	}

	return task;
}
