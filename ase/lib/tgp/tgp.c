/*
 * $Id$
 */

#include <ase/tgp/tgp.h>
#include <ase/cmn/mem.h>

struct ase_tgp_t
{
	ase_mmgr_t mmgr;
	void* assoc_data;
	int errnum;

	ase_tgp_io_t* ih;
	ase_tgp_io_t* oh;
	ase_tgp_io_t* rh;

	void* ih_arg;
	void* oh_arg;
	void* rh_arg;

	struct
	{
		ase_size_t pos;
		ase_size_t len;
		ase_char_t ptr[512];
	} ib;

	struct
	{
		ase_size_t len;
		ase_char_t ptr[512];
	} ob;

	struct
	{
		ase_size_t len;
		ase_char_t ptr[512];
	} rb;

	int (*read) (ase_tgp_t* tgp, ase_char_t* buf, int len);
	int (*write) (ase_tgp_t* tgp, const ase_char_t* buf, int len);
	int (*run) (ase_tgp_t* tgp, const ase_char_t* buf, int len);
};

ase_tgp_t* ase_tgp_open (ase_mmgr_t* mmgr)
{
	ase_tgp_t* tgp;

	/*
	if (mmgr == ASE_NULL) mmgr = ASE_GETMMGR();
	if (mmgr == ASE_NULL) 
	{
		ASE_ASSERTX (mmgr != ASE_NULL, 
			"Provide the memory manager or set the global memory manager with ASE_SETMMGR()");
		return ASE_NULL;
	}
	*/

	tgp = ASE_MALLOC (mmgr, ASE_SIZEOF(*tgp));
	if (tgp == ASE_NULL) return ASE_NULL;

	ase_memset (tgp, 0, ASE_SIZEOF(*tgp));
	ase_memcpy (&tgp->mmgr, mmgr, ASE_SIZEOF(*mmgr));

	return tgp;
}

void ase_tgp_close (ase_tgp_t* tgp)
{
	ASE_FREE (&tgp->mmgr, tgp);
}

void ase_tgp_setassocdata (ase_tgp_t* tgp, void* data)
{
	tgp->assoc_data = data;
}

void* ase_tgp_getassocdata (ase_tgp_t* tgp)
{
	return tgp->assoc_data;
}

int ase_tgp_geterrnum (ase_tgp_t* tgp)
{
	return tgp->errnum;
}

static int getc (ase_tgp_t* tgp, ase_char_t* c)
{
	if (tgp->ib.pos >= tgp->ib.len) 
	{
		ase_ssize_t n;

		n = tgp->ih (tgp->ih.arg, ASE_TGP_IO_READ, tgp->ib.ptr, ASE_COUNTOF(tgp->ib.ptr));
		if (n < 0) return -1;
		else if (n == 0) 
		{
			*c = ASE_CHAR_EOF;
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

static int putc (ase_tgp_t* tgp, ase_char_t c)
{
	if (tgp->ob.len >= ASE_COUNTOF(tgp->ob.ptr))
	{
		ase_ssize_t n;

		/* TODO: submit on a newline as well */
		n = tgp->oh (tgp->oh.arg, ASE_TGP_IO_WRITE, tgp->ob.ptr, ASE_COUNTOF(tgp->ob.ptr));
		if (n < 0) return -1;
		else if (n == 0) return 0;
	}

	tgp->ob.ptr[tgp->ob.len++] = c;
	return 1;
}

static int runc (ase_tgp_t* tgp, ase_char_t c)
{
	if (tgp->rb.len >= ASE_COUNTOF(tgp->rb.ptr))
	{
		ase_ssize_t n;

		n = tgp->rh (tgp->rh.arg, ASE_TGP_IO_PUT, tgp->rb.ptr, tgp->rb.len);
		if (n < 0) return -1;
		else if (n == 0) return 0;

		tgp->rh (tgp->rh.arg, ASE_TGP_IO_GET, tgp->rb.ptr, tgp->rb.len);
	}

	tgp->rb.ptr[tgp->rb.len++] = c;
	return 1;
}

int ase_tgp_run (ase_tgp_t* tgp)
{
	ase_bool_t in_tag = ASE_FALSE;
	ase_char_t c;
	int n;

	tgp->ib.pos = 0;
	tgp->ib.len = 0;
	tgp->ob.len = 0;
	tgp->rb.len = 0;

	n = tgp->ih.func (ASE_TGP_IO_OPEN, tgp->ih.arg, ASE_NULL, 0);
	if (n == -1)
	{
		/* error */
		return -1;
	}
	if (n == 0)
	{
		/* reached end of input upon opening the file... */
		tgp->ih.func (ASE_TGP_IO_CLOSE, tgp->ih.arg, ASE_NULL, 0);
		return 0;
	}

	n = tgp->oh.func (ASE_TGP_IO_OPEN, tgp->oh.arg, ASE_NULL, 0);
	if (n == -1)
	{
		tgp->ih.func (ASE_TGP_IO_CLOSE, tgp->ih.arg, ASE_NULL, 0);
		return -1;
	}
	if (n == 0)
	{
		/* reached end of input upon opening the file... */
		tgp->oh.func (ASE_TGP_IO_CLOSE, tgp->oh.arg, ASE_NULL, 0);
		tgp->ih.func (ASE_TGP_IO_CLOSE, tgp->ih.arg, ASE_NULL, 0);
		return 0;
	}
	
	while (1)
	{
		n = getc (tgp, &c);
		if (n == -1) return -1;
		if (n == 0) break;

		if (c == ASE_T('<')) 
		{
			n = getc (tgp, &c);
			if (n == -1) return -1;
			if (n == 0) 
			{
				putc (tgp, ASE_T('<'));
				break;
			}

			if (c == ASE_T('?'))
			{
				if (in_tag)
				{
					/* ERROR - netsted tag */
					return -1;
				}
				else in_tag = ASE_TRUE;
			}
			else 
			{
				if (putc (tgp, ASE_T('<')) <= 0) return -1;
				if (putc (tgp, c) <= 0) return -1;
			}
		}
		else if (c == ASE_T('?'))
		{
			n = getc (tgp, &c);
			if (n == -1) return -1;
			if (n == 0) 
			{
				if (putc (tgp, ASE_T('<')) <= 0) return -1;
				break;
			}

			if (c == ASE_T('>'))
			{
				if (in_tag) in_tag = ASE_FALSE;
				else
				{
					/* ERROR - unpaired tag close */
					return -1;
				}
			}
			else
			{
				if (putc (tgp, ASE_T('?')) <= 0) return -1;
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
	
	tgp->oh.func (ASE_TGP_IO_CLOSE, tgp->oh.arg, ASE_NULL, 0);
	tgp->ih.func (ASE_TGP_IO_CLOSE, tgp->ih.arg, ASE_NULL, 0);
	return 0;
}

void ase_tgp_attachin (ase_tgp_t* tgp, ase_tgp_io_t* io, void* arg)
{
	tgp->ih = io;
	tgp->ih_arg = arg;
}

void ase_tgp_detachin (ase_tgp_t* tgp)
{
	tgp->ih = ASE_NULL;
	tgp->ih_arg = ASE_NULL;
}

void ase_tgp_attachout (ase_tgp_t* tgp, ase_tgp_io_t* io)
{
	tgp->oh = io;
	tgp->oh_arg = arg;
}

void ase_tgp_detachout (ase_tgp_t* tgp)
{
	tgp->oh = ASE_NULL;
	tgp->oh_arg = ASE_NULL;
}

void ase_tgp_attachin (ase_tgp_t* tgp, ase_tgp_io_t* io)
{
	tgp->rh = io;
	tgp->rh_arg = arg;
}

void ase_tgp_detachin (ase_tgp_t* tgp)
{
	tgp->rh = ASE_NULL;
	tgp->rh_arg = ASE_NULL;
}
