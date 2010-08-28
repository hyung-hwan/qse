/*
 * $Id$
 *
    Copyright 2006-2009 Chung, Hyung-Hwan.
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
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include "scm.h"

QSE_IMPLEMENT_COMMON_FUNCTIONS (scm)

static qse_scm_t* qse_scm_init (
	qse_scm_t*           scm,
	qse_mmgr_t*          mmgr,
	const qse_scm_prm_t* prm,
	qse_size_t           mem_ubound,
	qse_size_t mem_ubound_inc
);

static void qse_scm_fini (qse_scm_t* scm);

qse_scm_t* qse_scm_open (
	qse_mmgr_t* mmgr, qse_size_t xtnsize, const qse_scm_prm_t* prm, 
	qse_size_t mem_ubound, qse_size_t mem_ubound_inc)
{
	qse_scm_t* scm;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	scm = (qse_scm_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_scm_t) + xtnsize);
	if (scm == QSE_NULL) return QSE_NULL;

	if (qse_scm_init (scm, mmgr, prm, mem_ubound, mem_ubound_inc) == QSE_NULL)
	{
		QSE_MMGR_FREE (scm->mmgr, scm);
		return QSE_NULL;
	}

	return scm;
}

void qse_scm_close (qse_scm_t* scm)
{
	qse_scm_fini (scm);
	QSE_SCM_FREE (scm, scm);
}

static qse_scm_t* qse_scm_init (
	qse_scm_t* scm, qse_mmgr_t* mmgr, const qse_scm_prm_t* prm,
	qse_size_t mem_ubound, qse_size_t mem_ubound_inc)
{
	if (mmgr == QSE_NULL) mmgr = QSE_MMGR_GETDFL();

	QSE_MEMSET (scm, 0, QSE_SIZEOF(*scm));

	scm->mmgr = mmgr;
	scm->prm = *prm;

	/* set the default error string function */
	scm->err.str = qse_scm_dflerrstr;

	/* initialize error data */
	scm->err.num = QSE_SCM_ENOERR;
	scm->err.msg[0] = QSE_T('\0');

	/* initialize read data */
	scm->r.curc = QSE_CHAR_EOF;
	scm->r.curloc.line = 1;
	scm->r.curloc.colm = 0;

	if (qse_str_init(&scm->r.t.name, mmgr, 256) == QSE_NULL) 
	{
		QSE_SCM_FREE (scm, scm);
		return QSE_NULL;
	}

	/* initialize memory manager */
#if  0
	if (qse_scm_initmem (
		&scm->mem, scm, mem_ubound, mem_ubound_inc) == QSE_NULL)
	{
		qse_str_fini (&scm->r.t.name);
		QSE_SCM_FREE (scm, scm);
		return QSE_NULL;
	}
#endif

	return scm;
}

static void qse_scm_fini (qse_scm_t* scm)
{
#if 0
	qse_scm_finimem (&scm->mem);
#endif
	qse_str_fini (&scm->r.t.name);
}

void qse_scm_detachio (qse_scm_t* scm)
{
	if (scm->io.fns.out)
	{
		scm->io.fns.out (scm, QSE_SCM_IO_CLOSE, &scm->io.arg.out, QSE_NULL, 0);
		scm->io.fns.out = QSE_NULL;
	}

	if (scm->io.fns.in)
	{
		scm->io.fns.in (scm, QSE_SCM_IO_CLOSE, &scm->io.arg.in, QSE_NULL, 0);
		scm->io.fns.in = QSE_NULL;

		scm->r.curc = QSE_CHAR_EOF; /* TODO: needed??? */
	}
}

int qse_scm_attachio (qse_scm_t* scm, qse_scm_io_t* io)
{
	qse_scm_detachio(scm);

	QSE_ASSERT (scm->io.fns.in == QSE_NULL);
	QSE_ASSERT (scm->io.fns.out == QSE_NULL);

	scm->err.num = QSE_SCM_ENOERR;
	if (io->in (scm, QSE_SCM_IO_OPEN, &scm->io.arg.in, QSE_NULL, 0) <= -1)
	{
		if (scm->err.num == QSE_SCM_ENOERR)
			qse_scm_seterror (scm, QSE_SCM_EIO, QSE_NULL, QSE_NULL);
		return -1;
	}

	scm->err.num = QSE_SCM_ENOERR;
	if (io->out (scm, QSE_SCM_IO_OPEN, &scm->io.arg.out, QSE_NULL, 0) <= -1)
	{
		if (scm->err.num == QSE_SCM_ENOERR)
			qse_scm_seterror (scm,QSE_SCM_EIO, QSE_NULL, QSE_NULL);
		io->in (scm, QSE_SCM_IO_CLOSE, &scm->io.arg.in, QSE_NULL, 0);
		return -1;
	}

	scm->io.fns = *io;
	scm->r.curc = QSE_CHAR_EOF;
	scm->r.curloc.line = 1;
	scm->r.curloc.colm = 0;

	return 0;
}

