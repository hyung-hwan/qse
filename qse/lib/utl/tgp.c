/*
 * $Id$
 *
   Copyright 2006-2008 Chung, Hyung-Hwan.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */

#include <qse/utl/tgp.h>
#include "../cmn/mem.h"

struct qse_tgp_t
{
	qse_mmgr_t mmgr;
	void* assoc_data;
	int errnum;

	struct
	{
		qse_tgp_io_t func;
		void* arg;
	} ih;

	struct 
	{
		qse_tgp_io_t func;
		void* arg;
	} oh;

	struct 
	{
		qse_tgp_io_t func;
		void* arg;
	} rh;

	struct
	{
		qse_size_t pos;
		qse_size_t len;
		qse_char_t ptr[512];
	} ib;

	struct
	{
		qse_size_t len;
		qse_char_t ptr[512];
	} ob;

	struct
	{
		qse_size_t len;
		qse_char_t ptr[512];
	} rb;

	int (*read) (qse_tgp_t* tgp, qse_char_t* buf, int len);
	int (*write) (qse_tgp_t* tgp, const qse_char_t* buf, int len);
	int (*run) (qse_tgp_t* tgp, const qse_char_t* buf, int len);
};

qse_tgp_t* qse_tgp_open (qse_mmgr_t* mmgr)
{
	qse_tgp_t* tgp;

	/*
	if (mmgr == QSE_NULL) mmgr = QSE_GETMMGR();
	if (mmgr == QSE_NULL) 
	{
		QSE_ASSERTX (mmgr != QSE_NULL, 
			"Provide the memory manager or set the global memory manager with QSE_SETMMGR()");
		return QSE_NULL;
	}
	*/

	tgp = QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(*tgp));
	if (tgp == QSE_NULL) return QSE_NULL;

	QSE_MEMSET (tgp, 0, QSE_SIZEOF(*tgp));
	QSE_MEMCPY (&tgp->mmgr, mmgr, QSE_SIZEOF(*mmgr));

	return tgp;
}

void qse_tgp_close (qse_tgp_t* tgp)
{
	QSE_MMGR_FREE (&tgp->mmgr, tgp);
}

void qse_tgp_setassocdata (qse_tgp_t* tgp, void* data)
{
	tgp->assoc_data = data;
}

void* qse_tgp_getassocdata (qse_tgp_t* tgp)
{
	return tgp->assoc_data;
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
