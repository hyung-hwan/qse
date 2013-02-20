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
#include <qse/cmn/fmt.h>

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
	n = httpd->opt.scb.client.send (httpd, client, ctx->ptr, count);
	if (n <= -1) return -1;

	ctx->left -= n;
	if (ctx->left <= 0) return 0;

	ctx->ptr += n;
	return 1; /* more work to do */
}


qse_httpd_task_t* qse_httpd_entask_text (
     qse_httpd_t* httpd,
	qse_httpd_client_t* client,
	qse_httpd_task_t* pred, 
	const qse_mchar_t* ptr,
	qse_size_t len)
{
	qse_httpd_task_t task;
	task_text_t data;

	QSE_MEMSET (&data, 0, QSE_SIZEOF(data));
	data.ptr = ptr;
	data.left = len;

	QSE_MEMSET (&task, 0, QSE_SIZEOF(task));
	task.init = task_init_text;
	task.main = task_main_text;
	task.ctx = &data;

	return qse_httpd_entask (
		httpd, client, pred, 
		&task, QSE_SIZEOF(data) + data.left);
}

qse_httpd_task_t* qse_httpd_entasktext (
     qse_httpd_t* httpd,
	qse_httpd_client_t* client,
	qse_httpd_task_t* pred, 
	const qse_mchar_t* text,
	const qse_mchar_t* mime,
	qse_htre_t* req)
{
	qse_size_t tlen;
	qse_mchar_t b_tlen[64];
	qse_http_version_t* version;

	version = qse_htre_getversion (req);

	tlen = qse_mbslen(text);
	qse_fmtuintmaxtombs (b_tlen, QSE_COUNTOF(b_tlen), tlen, 10, -1, QSE_MT('\0'), QSE_NULL);

	pred = qse_httpd_entaskformat (
		httpd, client, pred,
		QSE_MT("HTTP/%d.%d 200 OK\r\nServer: %s\r\nDate: %s\r\nConnection: %s\r\nContent-Type: %s\r\nContent-Length: %s\r\n\r\n"),
		version->major, version->minor,
		qse_httpd_getname (httpd),
		qse_httpd_fmtgmtimetobb (httpd, QSE_NULL, 0),
		((req->attr.flags & QSE_HTRE_ATTR_KEEPALIVE)? QSE_MT("keep-alive"): QSE_MT("close")),
		mime, b_tlen
	);
	if (pred == QSE_NULL) return QSE_NULL;

	return qse_httpd_entask_text (httpd, client, pred, text, tlen);
}
