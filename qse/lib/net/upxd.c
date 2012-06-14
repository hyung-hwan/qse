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

#include "upxd.h"

qse_upxd_t* qse_upxd_open (qse_mmgr_t* mmgr, qse_size_t xtnsize)
{
	qse_upxd_t* upxd;

	upxd = (qse_upxd_t*) QSE_MMGR_ALLOC (
		mmgr, QSE_SIZEOF(*upxd) + xtnsize
	);
	if (upxd == QSE_NULL) return QSE_NULL;

	if (qse_upxd_init (upxd, mmgr) <= -1)
	{
		QSE_MMGR_FREE (upxd->mmgr, upxd);
		return QSE_NULL;
	}

	return upxd;
}

void qse_upxd_close (qse_upxd_t* upxd)
{
	qse_upxd_fini (upxd);
	QSE_MMGR_FREE (upxd->mmgr, upxd);
}

int qse_upxd_init (qse_upxd_t* upxd, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (upxd, 0, QSE_SIZEOF(*upxd));
	upxd->mmgr = mmgr;
	return 0;
}

void qse_upxd_fini (qse_upxd_t* upxd)
{
	free_server_list (upxd, upxd->server.list);
	QSE_ASSERT (upxd->server.navail == 0);
	upxd->server.list = QSE_NULL;
}

void qse_upxd_stop (qse_upxd_t* upxd)
{
	upxd->stopreq = 1;
}
 
qse_upxd_errnum_t qse_upxd_geterrnum (qse_upxd_t* upxd)
{
	return upxd->errnum;
}

void qse_upxd_seterrnum (qse_upxd_t* upxd, qse_upxd_errnum_t errnum)
{
	upxd->errnum = errnum;
}

int qse_upxd_addserver (qse_upxd_t* upxd, const qse_nwad_t* nwad)
{
	qse_upxd_server_t* server;

	server = QSE_MMGR_ALLOC (upxd->mmgr, QSE_SIZEOF(*server));
	if (server == QSE_NULL)
	{
		upxd->errnum = QSE_UPXD_ENOMEM;
		return -1;
	}

	QSE_MEMSET (server, 0, QSE_SIZEOF(*server));
	server->nwad = *nwad;

	if (upxd->cbs->server.open (upxd, server) <= -1)
	{
		QSE_MMGR_FREE (upxd->mmgr, server);
		return -1;
	}

	server->next = upxd->server.list;	
	upxd->server.list = server;
}

int qse_upxd_loop (qse_upxd_t* upxd, qse_upxd_cbls_t* cbs)
{
	upxd->stopreq = 0;

	while (!upxd->stopreq)
	{
	}

	return 0;
}
