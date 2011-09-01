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
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include "scm.h"

QSE_IMPLEMENT_COMMON_FUNCTIONS (scm)

static int qse_scm_init (
	qse_scm_t*  scm,
	qse_mmgr_t* mmgr,
	qse_size_t  mem_ubound,
	qse_size_t  mem_ubound_inc
);

static void qse_scm_fini (
	qse_scm_t* scm
);

qse_scm_t* qse_scm_open (
	qse_mmgr_t* mmgr, qse_size_t xtnsize,
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

	scm = (qse_scm_t*) QSE_MMGR_ALLOC (
		mmgr, QSE_SIZEOF(qse_scm_t) + xtnsize
	);
	if (scm == QSE_NULL) return QSE_NULL;

	if (qse_scm_init (scm, mmgr, mem_ubound, mem_ubound_inc) <= -1)
	{
		QSE_MMGR_FREE (scm->mmgr, scm);
		return QSE_NULL;
	}

	return scm;
}

void qse_scm_close (qse_scm_t* scm)
{
	qse_scm_fini (scm);
	QSE_MMGR_FREE (scm->mmgr, scm);
}

static QSE_INLINE void delete_all_entity_blocks (qse_scm_t* scm)
{
	while (scm->mem.ebl)
	{
		qse_scm_enb_t* enb = scm->mem.ebl;
		scm->mem.ebl = scm->mem.ebl->next;
		QSE_MMGR_FREE (scm->mmgr, ((void**)enb)[-1]);
	}
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
			qse_scm_seterror (scm, QSE_SCM_EIO, QSE_NULL, QSE_NULL);
		io->in (scm, QSE_SCM_IO_CLOSE, &scm->io.arg.in, QSE_NULL, 0);
		return -1;
	}

	scm->io.fns = *io;
	scm->r.curc = QSE_CHAR_EOF;
	scm->r.curloc.line = 1;
	scm->r.curloc.colm = 0;

	return 0;
}

#define MAKE_SYNTAX_ENTITY(scm,name,uptr) QSE_BLOCK( \
	if (qse_scm_makesyntent (scm, name, uptr) == QSE_NULL) return -1; \
)

static int build_syntax_entities (qse_scm_t* scm)
{
	qse_scm_ent_t* v;

	v = qse_scm_makesyntent (scm, QSE_T("lambda"), qse_scm_dolambda);
	if (v == QSE_NULL) return -1;
	scm->lambda = v;

	v = qse_scm_makesyntent (scm, QSE_T("quote"), qse_scm_doquote);
	if (v == QSE_NULL) return -1;
	scm->quote = v;

	MAKE_SYNTAX_ENTITY (scm, QSE_T("define"), qse_scm_dodefine);
	MAKE_SYNTAX_ENTITY (scm, QSE_T("begin"),  qse_scm_dobegin);
	MAKE_SYNTAX_ENTITY (scm, QSE_T("if"),     qse_scm_doif);

	return 0;
}

static qse_scm_t* qse_scm_init (
	qse_scm_t* scm, qse_mmgr_t* mmgr, 
	qse_size_t mem_ubound, qse_size_t mem_ubound_inc)
{
	static qse_scm_ent_t static_values[3] =
	{
		/* dswcount, mark, atom, synt, type */

		/* nil */
		{ 0, 1, 1, 0, QSE_SCM_ENT_NIL },
		/* f */
		{ 0, 1, 1, 0, QSE_SCM_ENT_T }, 
		/* t */
		{ 0, 1, 1, 0, QSE_SCM_ENT_F }
	};

	if (mmgr == QSE_NULL) mmgr = QSE_MMGR_GETDFL();

	QSE_MEMSET (scm, 0, QSE_SIZEOF(*scm));
	scm->mmgr = mmgr;

	/* set the default error string function */
	scm->err.str = qse_scm_dflerrstr;

	/* initialize error data */
	scm->err.num = QSE_SCM_ENOERR;
	scm->err.msg[0] = QSE_T('\0');

	/* initialize read data */
	scm->r.curc = QSE_CHAR_EOF;
	scm->r.curloc.line = 1;
	scm->r.curloc.colm = 0;
	if (qse_str_init(&scm->r.t.name, mmgr, 256) <= -1) return -1;

	/* initialize common values */
	scm->nil    = &static_values[0];
	scm->t      = &static_values[1];
	scm->f      = &static_values[2];
	scm->lambda = scm->nil;
	scm->quote  = scm->nil;

	/* initialize entity block list */
	scm->mem.ebl = QSE_NULL;
	scm->mem.free = scm->nil;

	/* initialize all the key data to nil before qse_scm_makepairent()
	 * below. qse_scm_makepairent() calls alloc_entity() that invokes
	 * gc() as this is the first time. As gc() marks all the key data,
	 * we need to initialize these to nil. */
	scm->symtab = scm->nil;
	scm->gloenv = scm->nil;

	scm->r.s    = scm->nil;
	scm->r.e    = scm->nil;
	scm->p.s    = scm->nil;
	scm->p.e    = scm->nil;
	scm->e.arg  = scm->nil;
	scm->e.dmp  = scm->nil;
	scm->e.cod  = scm->nil;
	scm->e.env  = scm->nil;

	/* build the global environment entity as a pair */
	scm->gloenv = qse_scm_makepairent (scm, scm->nil, scm->nil);
	if (scm->gloenv == QSE_NULL) goto oops;

	/* update the current environment to the global environment */
	scm->e.env = scm->gloenv;

	if (build_syntax_entities (scm) <= -1) goto oops;
	return 0;

oops:
	delete_all_entity_blocks (scm);
	qse_str_fini (&scm->r.t.name);
	return -1;
}

static void qse_scm_fini (qse_scm_t* scm)
{
	delete_all_entity_blocks (scm);
	qse_str_fini (&scm->r.t.name);
}


