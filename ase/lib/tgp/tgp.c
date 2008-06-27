/*
 * $Id$
 */

#include <ase/tgp/tgp.h>
#include <ase/cmn/mem.h>

struct ase_tgp_t
{
	ase_mmgr_t mmgr;
	int errnum;

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

int ase_tgp_geterrnum (ase_tgp_t* tgp)
{
	return tgp->errnum;
}

static int getc (ase_tgp_t* tgp, ase_char_t* c)
{
	if (tgp->ib.pos >= tgp->ib.len) 
	{
		ase_ssize_t n;

		n = tgp->read (tgp, tgp->ib.ptr, ASE_COUNTOF(tgp->ib.ptr));
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

		n = tgp->write (tgp, tgp->ob.ptr, tgp->ob.len);
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

		n = tgp->run (tgp, tgp->rb.ptr, tgp->rb.len);
		if (n < 0) return -1;
		else if (n == 0) return 0;
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
	
	return 0;
}
