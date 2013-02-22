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

#if defined(_WIN32) || defined(__DOS__) || defined(__OS2__)
/* UNSUPPORTED YET..  */
/* TODO: IMPLEMENT THIS */
#else

#include "httpd.h"
#include "../cmn/mem.h"

typedef struct task_resol_arg_t task_resol_arg_t;
struct task_resol_arg_t 
{
	const qse_mchar_t* path;
	qse_htre_t* req;
	int nph;
};

typedef struct task_resol_t task_resol_t;
struct task_resol_t
{
	int init_failed;
	qse_httpd_t* httpd;

	const qse_mchar_t* path;
	qse_http_version_t version;
	int keepalive; /* taken from the request */
};

static int task_init_resol (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_resol_t* resol;

	resol = (task_resol_t*)qse_httpd_gettaskxtn (httpd, task);

	QSE_MEMSET (resol, 0, QSE_SIZEOF(*resol));
	resol->httpd = httpd;

	task->ctx = resol;
	return 0;
}

static void task_fini_resol (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	task_resol_t* resol = (task_resol_t*)task->ctx;
}

static int task_main_resol (
	qse_httpd_t* httpd, qse_httpd_client_t* client, qse_httpd_task_t* task)
{
	return 0;
}

qse_httpd_task_t* qse_httpd_entaskresol (
	qse_httpd_t* httpd,
	qse_httpd_client_t* client,
	qse_httpd_task_t* pred, 
	const qse_mchar_t* host)
{
	qse_httpd_task_t task;
	task_resol_arg_t arg;

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
