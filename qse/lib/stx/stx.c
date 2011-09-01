/*
 * $Id: stx.c 118 2008-03-03 11:21:33Z baconevi $
 */

#include "stx.h"
#include "mem.h"
#include "boot.h"
#include "../cmn/mem.h"

int qse_stx_init (qse_stx_t* stx, qse_mmgr_t* mmgr, qse_size_t memcapa)
{
	QSE_MEMSET (stx, 0, QSE_SIZEOF(*stx));
	stx->mmgr = mmgr;

	/* initialize object memory subsystem */
	if (qse_stx_initmem (stx, memcapa) <= -1) return -1;

	return 0;
}

void qse_stx_fini (qse_stx_t* stx)
{
	qse_stx_finimem (stx);
}

qse_stx_t* qse_stx_open (
	qse_mmgr_t* mmgr, qse_size_t xtnsize, qse_size_t memcapa)
{
	qse_stx_t* stx;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();
		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");
		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	stx = (qse_stx_t*) QSE_MMGR_ALLOC (
		mmgr, QSE_SIZEOF(qse_stx_t) + xtnsize
	);
	if (stx == QSE_NULL) return QSE_NULL;

	if (qse_stx_init (stx, mmgr, memcapa) <= -1)
	{
		QSE_MMGR_FREE (stx->mmgr, stx);
		return QSE_NULL;
	}

	return stx;
}

void qse_stx_close (qse_stx_t* stx)
{
	qse_stx_fini (stx);
	QSE_MMGR_FREE (stx->mmgr, stx);
}

