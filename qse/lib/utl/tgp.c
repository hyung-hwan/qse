/*
 * $Id$
 */

#include <qse/utl/tgp.h>
#include "../cmn/mem.h"
#include "tgp.h"

QSE_IMPLEMENT_COMMON_FUNCTIONS (tgp)

qse_tgp_t* qse_tgp_open (qse_mmgr_t* mmgr, qse_size_t xtn)
{
	qse_tgp_t* tgp;

	if (mmgr == QSE_NULL) 
	{
		mmgr = QSE_MMGR_GETDFL();

		QSE_ASSERTX (mmgr != QSE_NULL,
			"Set the memory manager with QSE_MMGR_SETDFL()");

		if (mmgr == QSE_NULL) return QSE_NULL;
	}

	tgp = (qse_tgp_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_tgp_t) + xtn);
	if (tgp == QSE_NULL) return QSE_NULL;

	if (qse_tgp_init (tgp, mmgr) == QSE_NULL)
	{
		QSE_MMGR_FREE (tgp->mmgr, tgp);
		return QSE_NULL;
	}

	return tgp;
}

void qse_tgp_close (qse_tgp_t* tgp)
{
	qse_tgp_fini (tgp);
	QSE_MMGR_FREE (tgp->mmgr, tgp);
}

qse_tgp_t* qse_tgp_init (qse_tgp_t* tgp, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (tgp, 0, sizeof(*tgp));
	tgp->mmgr = mmgr;

	return tgp;
}

void qse_tgp_fini (qse_tgp_t* tgp)
{
}

int qse_tgp_geterrnum (qse_tgp_t* tgp)
{
	return tgp->errnum;
}

static int getc (qse_tgp_t* tgp, qse_char_t* c)
{
	if (tgp->ib.pos >= tgp->ib.len) 
	{
		qse_ssize_t n;

		n = tgp->ih.func (QSE_TGP_IO_READ, tgp->ih.arg, tgp->ib.ptr, QSE_COUNTOF(tgp->ib.ptr));
		if (n < 0) return -1;
		else if (n == 0) 
		{
			*c = QSE_CHAR_EOF;
			return 0;
		}
		else
		{
			tgp->ib.pos = 0;
			tgp->ib.len = n;	
		}
	}

	*c = tgp->ib.ptr[tgp->ib.pos++];
	return 1;
}

static int putc (qse_tgp_t* tgp, qse_char_t c)
{
	if (tgp->ob.len >= QSE_COUNTOF(tgp->ob.ptr))
	{
		qse_ssize_t n;

		/* TODO: submit on a newline as well */
		n = tgp->oh.func (QSE_TGP_IO_WRITE, tgp->oh.arg, tgp->ob.ptr, QSE_COUNTOF(tgp->ob.ptr));
		if (n < 0) return -1;
		else if (n == 0) return 0;
	}

	tgp->ob.ptr[tgp->ob.len++] = c;
	return 1;
}

static int runc (qse_tgp_t* tgp, qse_char_t c)
{
	if (tgp->rb.len >= QSE_COUNTOF(tgp->rb.ptr))
	{
		qse_ssize_t n;

		n = tgp->rh.func (QSE_TGP_IO_WRITE, tgp->rh.arg, tgp->rb.ptr, tgp->rb.len);
		if (n < 0) return -1;
		else if (n == 0) return 0;

		tgp->rh.func (QSE_TGP_IO_READ, tgp->rh.arg, tgp->rb.ptr, tgp->rb.len);
	}

	tgp->rb.ptr[tgp->rb.len++] = c;
	return 1;
}

int qse_tgp_run (qse_tgp_t* tgp)
{
	qse_bool_t in_tag = QSE_FALSE;
	qse_char_t c;
	int n;

	tgp->ib.pos = 0;
	tgp->ib.len = 0;
	tgp->ob.len = 0;
	tgp->rb.len = 0;

	n = tgp->ih.func (QSE_TGP_IO_OPEN, tgp->ih.arg, QSE_NULL, 0);
	if (n == -1)
	{
		/* error */
		return -1;
	}
	if (n == 0)
	{
		/* reached end of input upon opening the file... */
		tgp->ih.func (QSE_TGP_IO_CLOSE, tgp->ih.arg, QSE_NULL, 0);
		return 0;
	}

	n = tgp->oh.func (QSE_TGP_IO_OPEN, tgp->oh.arg, QSE_NULL, 0);
	if (n == -1)
	{
		tgp->ih.func (QSE_TGP_IO_CLOSE, tgp->ih.arg, QSE_NULL, 0);
		return -1;
	}
	if (n == 0)
	{
		/* reached end of input upon opening the file... */
		tgp->oh.func (QSE_TGP_IO_CLOSE, tgp->oh.arg, QSE_NULL, 0);
		tgp->ih.func (QSE_TGP_IO_CLOSE, tgp->ih.arg, QSE_NULL, 0);
		return 0;
	}
	
	while (1)
	{
		n = getc (tgp, &c);
		if (n == -1) return -1;
		if (n == 0) break;

		if (c == QSE_T('<')) 
		{
			n = getc (tgp, &c);
			if (n == -1) return -1;
			if (n == 0) 
			{
				putc (tgp, QSE_T('<'));
				break;
			}

			if (c == QSE_T('?'))
			{
				if (in_tag)
				{
					/* ERROR - netsted tag */
					return -1;
				}
				else in_tag = QSE_TRUE;
			}
			else 
			{
				if (putc (tgp, QSE_T('<')) <= 0) return -1;
				if (putc (tgp, c) <= 0) return -1;
			}
		}
		else if (c == QSE_T('?'))
		{
			n = getc (tgp, &c);
			if (n == -1) return -1;
			if (n == 0) 
			{
				if (putc (tgp, QSE_T('<')) <= 0) return -1;
				break;
			}

			if (c == QSE_T('>'))
			{
				if (in_tag) in_tag = QSE_FALSE;
				else
				{
					/* ERROR - unpaired tag close */
					return -1;
				}
			}
			else
			{
				if (putc (tgp, QSE_T('?')) <= 0) return -1;
				if (putc (tgp, c) <= 0) return -1;
			}
		}
		else if (in_tag)
		{
			runc (tgp, c);
		}
		else 
		{
			if (putc (tgp, c) <= 0) return -1;
		}
	}
	
	tgp->oh.func (QSE_TGP_IO_CLOSE, tgp->oh.arg, QSE_NULL, 0);
	tgp->ih.func (QSE_TGP_IO_CLOSE, tgp->ih.arg, QSE_NULL, 0);
	return 0;
}

void qse_tgp_attachin (qse_tgp_t* tgp, qse_tgp_io_t io, void* arg)
{
	tgp->ih.func = io;
	tgp->ih.arg = arg;
}

void qse_tgp_detachin (qse_tgp_t* tgp)
{
	tgp->ih.func = QSE_NULL;
	tgp->ih.arg = QSE_NULL;
}

void qse_tgp_attachout (qse_tgp_t* tgp, qse_tgp_io_t io, void* arg)
{
	tgp->oh.func = io;
	tgp->oh.arg = arg;
}

void qse_tgp_detachout (qse_tgp_t* tgp)
{
	tgp->oh.func = QSE_NULL;
	tgp->oh.arg = QSE_NULL;
}

void qse_tgp_attachexec (qse_tgp_t* tgp, qse_tgp_io_t io, void* arg)
{
	tgp->rh.func = io;
	tgp->rh.arg = arg;
}

void qse_tgp_detachexec (qse_tgp_t* tgp)
{
	tgp->rh.func = QSE_NULL;
	tgp->rh.arg = QSE_NULL;
}
