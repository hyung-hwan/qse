/*
 * $Id: extio.c,v 1.24 2006-08-02 11:26:10 bacon Exp $
 */

#include <xp/awk/awk_i.h>

#ifndef XP_AWK_STAND_ALONE
#include <xp/bas/assert.h>
#include <xp/bas/string.h>
#include <xp/bas/memory.h>
#endif

enum
{
	__MASK_READ  = 0x0100,
	__MASK_WRITE = 0x0200,
	__MASK_RDWR  = 0x0400,

	__MASK_CLEAR   = 0x00FF
};

static int __in_type_map[] =
{
	/* the order should match the order of the 
	 * XP_AWK_IN_XXX values in tree.h */
	XP_AWK_EXTIO_PIPE,
	XP_AWK_EXTIO_COPROC,
	XP_AWK_EXTIO_FILE,
	XP_AWK_EXTIO_CONSOLE
};

static int __in_mode_map[] =
{
	/* the order should match the order of the 
	 * XP_AWK_IN_XXX values in tree.h */
	XP_AWK_IO_PIPE_READ,
	0,
	XP_AWK_IO_FILE_READ,
	XP_AWK_IO_CONSOLE_READ
};

static int __in_mask_map[] =
{
	__MASK_READ,
	__MASK_RDWR,
	__MASK_READ,
	__MASK_READ
};

static int __out_type_map[] =
{
	/* the order should match the order of the 
	 * XP_AWK_OUT_XXX values in tree.h */
	XP_AWK_EXTIO_PIPE,
	XP_AWK_EXTIO_COPROC,
	XP_AWK_EXTIO_FILE,
	XP_AWK_EXTIO_FILE,
	XP_AWK_EXTIO_CONSOLE
};

static int __out_mode_map[] =
{
	/* the order should match the order of the 
	 * XP_AWK_OUT_XXX values in tree.h */
	XP_AWK_IO_PIPE_WRITE,
	0,
	XP_AWK_IO_FILE_WRITE,
	XP_AWK_IO_FILE_APPEND,
	XP_AWK_IO_CONSOLE_WRITE
};

static int __out_mask_map[] =
{
	__MASK_WRITE,
	__MASK_RDWR,
	__MASK_WRITE,
	__MASK_WRITE,
	__MASK_WRITE
};

int xp_awk_readextio (
	xp_awk_run_t* run, int in_type,
	const xp_char_t* name, xp_str_t* buf, int* errnum)
{
	xp_awk_extio_t* p = run->extio;
	xp_awk_io_t handler;
	int extio_type, extio_mode, extio_mask, n;

	xp_assert (in_type >= 0 && in_type <= xp_countof(__in_type_map));
	xp_assert (in_type >= 0 && in_type <= xp_countof(__in_mode_map));
	xp_assert (in_type >= 0 && in_type <= xp_countof(__in_mask_map));

	/* translate the in_type into the relevant extio type and mode */
	extio_type = __in_type_map[in_type];
	extio_mode = __in_mode_map[in_type];
	extio_mask = __in_mask_map[in_type];

	handler = run->awk->extio[extio_type];
	if (handler == XP_NULL)
	{
		/* no io handler provided */
		*errnum = XP_AWK_EIOIMPL; /* TODO: change the error code */
		return -1;
	}

	while (p != XP_NULL)
	{
		if (p->type == (extio_type | extio_mask) &&
		    xp_strcmp(p->name,name) == 0) break;
		p = p->next;
	}

	if (p == XP_NULL)
	{
		p = (xp_awk_extio_t*) xp_malloc (xp_sizeof(xp_awk_extio_t));
		if (p == XP_NULL)
		{
			*errnum = XP_AWK_ENOMEM;
			return -1;
		}

		p->name = xp_strdup (name);
		if (p->name == XP_NULL)
		{
			xp_free (p);
			*errnum = XP_AWK_ENOMEM;
			return -1;
		}

		p->type = (extio_type | extio_mask);
		p->mode = extio_mode;
		p->handle = XP_NULL;

		p->in.buf[0] = XP_C('\0');
		p->in.pos = 0;
		p->in.len = 0;
		p->in.eof = xp_false;

		p->next = XP_NULL;

		n = handler (XP_AWK_IO_OPEN, p, XP_NULL, 0);
		if (n == -1)
		{
			xp_free (p->name);
			xp_free (p);
				
			/* TODO: use meaningful error code */
			xp_awk_setglobal (run, 
				XP_AWK_GLOBAL_ERRNO, xp_awk_val_one);

			*errnum = XP_AWK_ENOERR;
			return -1;
		}

		/* chain it */
		p->next = run->extio;
		run->extio = p;

		/* n == 0 indicates that it has reached the end of input. 
		 * the user io handler can return 0 for the open request
		 * if it doesn't have any files to open. One advantage 
		 * of doing this would be that you can skip the entire
		 * pattern-block matching and exeuction. */
		if (n == 0) return 0;
	}

	/* read a line */
	xp_str_clear (buf);

	while (1)
	{
		xp_char_t c;

		if (p->in.pos >= p->in.len)
		{
			xp_ssize_t n;

			if (p->in.eof)
			{
				if (XP_STR_LEN(buf) == 0) return 0;
				break;
			}

			n = handler (XP_AWK_IO_READ, p, 
				p->in.buf, xp_countof(p->in.buf));
			if (n == -1) 
			{
				/* handler error. getline should return -1 */
				/* TODO: use meaningful error code */
				xp_awk_setglobal (run, 
					XP_AWK_GLOBAL_ERRNO, xp_awk_val_one);

				*errnum = XP_AWK_ENOERR;
				return -1;
			}

			if (n == 0) 
			{
				p->in.eof = xp_true;
				if (XP_STR_LEN(buf) == 0) return 0;
				break;
			}

			p->in.len = n;
			p->in.pos = 0;
		}

		c = p->in.buf[p->in.pos++];
		if (c == XP_C('\n')) break; 

		if (xp_str_ccat (buf, c) == (xp_size_t)-1)
		{
			*errnum = XP_AWK_ENOMEM;
			return -1;
		}
	}

	return 1;
}

int xp_awk_writeextio (
	xp_awk_run_t* run, int out_type, 
	const xp_char_t* name, xp_awk_val_t* v, int* errnum)
{
	xp_awk_extio_t* p = run->extio;
	xp_awk_io_t handler;
	xp_str_t buf;
	int extio_type, extio_mode, extio_mask, n;

	xp_assert (out_type >= 0 && out_type <= xp_countof(__out_type_map));
	xp_assert (out_type >= 0 && out_type <= xp_countof(__out_mode_map));
	xp_assert (out_type >= 0 && out_type <= xp_countof(__out_mask_map));

	/* translate the out_type into the relevant extio type and mode */
	extio_type = __out_type_map[out_type];
	extio_mode = __out_mode_map[out_type];
	extio_mask = __out_mask_map[out_type];

	handler = run->awk->extio[extio_type];
	if (handler == XP_NULL)
	{
		/* no io handler provided */
		*errnum = XP_AWK_EIOIMPL; /* TODO: change the error code */
		return -1;
	}

	if (v->type != XP_AWK_VAL_STR)
	{
		/* TODO: optimize the buffer management. 
		 *       each xp_awk_run_t may have a buffer for this.  */
		if (xp_str_open (&buf, 256) == XP_NULL)
		{
			*errnum = XP_AWK_ENOMEM;
			return -1;
		}

		/* convert the value to string representation first */
		if (xp_awk_valtostr(v, errnum, &buf, XP_NULL) == XP_NULL)
		{
			xp_str_close (&buf);
			return -1;
		}
	}

	/* look for the corresponding extio for name */
	while (p != XP_NULL)
	{
		/* the file "1.tmp", in the following code snippets, 
		 * would be opened by the first print statement, but not by
		 * the second print statement. this is because
		 * both XP_AWK_OUT_FILE and XP_AWK_OUT_FILE_APPEND are
		 * translated to XP_AWK_EXTIO_FILE and it is used to
		 * keep track of file handles..
		 *
		 *    print "1111" >> "1.tmp"
		 *    print "1111" > "1.tmp"
		 */
		if (p->type == (extio_type | extio_mask) && 
		    xp_strcmp (p->name, name) == 0) break;
		p = p->next;
	}

	/* if there is not corresponding extio for name, create one */
	if (p == XP_NULL)
	{
		p = (xp_awk_extio_t*) xp_malloc (xp_sizeof(xp_awk_extio_t));
		if (p == XP_NULL)
		{
			if (v->type != XP_AWK_VAL_STR) xp_str_close (&buf);
			*errnum = XP_AWK_ENOMEM;
			return -1;
		}

		p->name = xp_strdup (name);
		if (p->name == XP_NULL)
		{
			xp_free (p);
			if (v->type != XP_AWK_VAL_STR) xp_str_close (&buf);
			*errnum = XP_AWK_ENOMEM;
			return -1;
		}

		p->type = (extio_type | extio_mask);
		p->mode = extio_mode;
		p->handle = XP_NULL;
		p->next = XP_NULL;

		n = handler (XP_AWK_IO_OPEN, p, XP_NULL, 0);
		if (n == -1)
		{
			xp_free (p->name);
			xp_free (p);
			if (v->type != XP_AWK_VAL_STR) xp_str_close (&buf);
				
			/* TODO: use meaningful error code */
			xp_awk_setglobal (run, 
				XP_AWK_GLOBAL_ERRNO, xp_awk_val_one);

			*errnum = XP_AWK_ENOERR;
			return -1;
		}

		/* chain it */
		p->next = run->extio;
		run->extio = p;

		/* read the comment in xp_awk_readextio */
		if (n == 0) return 0;
	}

/* TODO: */
/* TODO: */
/* TODO: if write handler returns less than the request, loop */
/* TODO: */
/* TODO: */
	if (v->type != XP_AWK_VAL_STR)
	{
		n = handler (XP_AWK_IO_WRITE, p,
			XP_STR_BUF(&buf), XP_STR_LEN(&buf));
	}
	else
	{
		n = handler (XP_AWK_IO_WRITE, p,
			((xp_awk_val_str_t*)v)->buf, 
			((xp_awk_val_str_t*)v)->len);
	}
	if (n == -1) 
	{
		if (v->type != XP_AWK_VAL_STR) xp_str_close (&buf);

		/* TODO: use meaningful error code */
		xp_awk_setglobal (run, 
			XP_AWK_GLOBAL_ERRNO, xp_awk_val_one);
		*errnum = XP_AWK_ENOERR;
		return -1;
	}

	if (n == 0)
	{
		if (v->type != XP_AWK_VAL_STR) xp_str_close (&buf);
		return 0;
	}

	if (v->type != XP_AWK_VAL_STR) xp_str_close (&buf);
	return 1;
}

int xp_awk_nextextio_read (
	xp_awk_run_t* run, int in_type, const xp_char_t* name, int* errnum)
{
	xp_awk_extio_t* p = run->extio;
	xp_awk_io_t handler;
	int extio_type, extio_mode, extio_mask, n;

	xp_assert (in_type >= 0 && in_type <= xp_countof(__in_type_map));
	xp_assert (in_type >= 0 && in_type <= xp_countof(__in_mode_map));
	xp_assert (in_type >= 0 && in_type <= xp_countof(__in_mask_map));

	/* translate the in_type into the relevant extio type and mode */
	extio_type = __in_type_map[in_type];
	extio_mode = __in_mode_map[in_type];
	extio_mask = __in_mask_map[in_type];

	handler = run->awk->extio[extio_type];
	if (handler == XP_NULL)
	{
		/* no io handler provided */
		*errnum = XP_AWK_EIOIMPL; /* TODO: change the error code */
		return -1;
	}

	while (p != XP_NULL)
	{
		if (p->type == (extio_type | extio_mask) &&
		    xp_strcmp(p->name,name) == 0) break;
		p = p->next;
	}

	if (p == XP_NULL)
	{
		/* something is totally wrong */
		*errnum = XP_AWK_EINTERNAL;
		return -1;
	}

	n = handler (XP_AWK_IO_NEXT, p, XP_NULL, 0);
	if (n == -1)
	{
		/* TODO: is this errnum correct? */
		*errnum = XP_AWK_ENOERR;
		return -1;
	}

	return n;
}

int xp_awk_closeextio_read (
	xp_awk_run_t* run, int in_type, const xp_char_t* name, int* errnum)
{
	xp_awk_extio_t* p = run->extio, * px = XP_NULL;
	xp_awk_io_t handler;
	int extio_type, extio_mode, extio_mask;

	xp_assert (in_type >= 0 && in_type <= xp_countof(__in_type_map));
	xp_assert (in_type >= 0 && in_type <= xp_countof(__in_mode_map));
	xp_assert (in_type >= 0 && in_type <= xp_countof(__in_mask_map));

	/* translate the in_type into the relevant extio type and mode */
	extio_type = __in_type_map[in_type];
	extio_mode = __in_mode_map[in_type];
	extio_mask = __in_mask_map[in_type];

	handler = run->awk->extio[extio_type];
	if (handler == XP_NULL)
	{
		/* no io handler provided */
		*errnum = XP_AWK_EIOIMPL; /* TODO: change the error code */
		return -1;
	}

	while (p != XP_NULL)
	{
		if (p->type == (extio_type | extio_mask) &&
		    xp_strcmp (p->name, name) == 0) 
		{
			xp_awk_io_t handler;
		       
			handler = run->awk->extio[p->type & __MASK_CLEAR];
			if (handler != XP_NULL)
			{
				if (handler (XP_AWK_IO_CLOSE, p, XP_NULL, 0) == -1)
				{
					/* this is not a run-time error.*/
					*errnum = XP_AWK_ENOERR;
					return -1;
				}
			}

			if (px != XP_NULL) px->next = p->next;
			else run->extio = p->next;

			xp_free (p->name);
			xp_free (p);
			return 0;
		}

		px = p;
		p = p->next;
	}

	/* this is not a run-time error */
	*errnum = XP_AWK_ENOERR;
	return -1;
}

int xp_awk_closeextio_write (
	xp_awk_run_t* run, int out_type, const xp_char_t* name, int* errnum)
{
	xp_awk_extio_t* p = run->extio, * px = XP_NULL;
	xp_awk_io_t handler;
	int extio_type, extio_mode, extio_mask;

	xp_assert (out_type >= 0 && out_type <= xp_countof(__out_type_map));
	xp_assert (out_type >= 0 && out_type <= xp_countof(__out_mode_map));
	xp_assert (out_type >= 0 && out_type <= xp_countof(__out_mask_map));

	/* translate the out_type into the relevant extio type and mode */
	extio_type = __out_type_map[out_type];
	extio_mode = __out_mode_map[out_type];
	extio_mask = __out_mask_map[out_type];

	handler = run->awk->extio[extio_type];
	if (handler == XP_NULL)
	{
		/* no io handler provided */
		*errnum = XP_AWK_EIOIMPL; /* TODO: change the error code */
		return -1;
	}

	while (p != XP_NULL)
	{
		if (p->type == (extio_type | extio_mask) &&
		    xp_strcmp (p->name, name) == 0) 
		{
			xp_awk_io_t handler;
		       
			handler = run->awk->extio[p->type & __MASK_CLEAR];
			if (handler != XP_NULL)
			{
				if (handler (XP_AWK_IO_CLOSE, p, XP_NULL, 0) == -1)
				{
					/* this is not a run-time error.*/
					*errnum = XP_AWK_ENOERR;
					return -1;
				}
			}

			if (px != XP_NULL) px->next = p->next;
			else run->extio = p->next;

			xp_free (p->name);
			xp_free (p);
			return 0;
		}

		px = p;
		p = p->next;
	}

	/* this is not a run-time error */
	*errnum = XP_AWK_ENOERR;
	return -1;
}

int xp_awk_closeextio (
	xp_awk_run_t* run, const xp_char_t* name, int* errnum)
{
	xp_awk_extio_t* p = run->extio, * px = XP_NULL;

	while (p != XP_NULL)
	{
		 /* it handles the first that matches the given name
		  * regardless of the extio type */
		if (xp_strcmp (p->name, name) == 0) 
		{
			xp_awk_io_t handler;
		       
			handler = run->awk->extio[p->type & __MASK_CLEAR];
			if (handler != XP_NULL)
			{
				if (handler (XP_AWK_IO_CLOSE, p, XP_NULL, 0) == -1)
				{
					/* this is not a run-time error.*/
					*errnum = XP_AWK_ENOERR;
					return -1;
				}
			}

			if (px != XP_NULL) px->next = p->next;
			else run->extio = p->next;

			xp_free (p->name);
			xp_free (p);
			return 0;
		}

		px = p;
		p = p->next;
	}

	/* this is not a run-time error */
	*errnum = XP_AWK_ENOERR;
	return -1;
}

void xp_awk_clearextio (xp_awk_run_t* run)
{
	xp_awk_extio_t* next;
	xp_awk_io_t handler;
	int n;

	while (run->extio != XP_NULL)
	{
		handler = run->awk->extio[run->extio->type & __MASK_CLEAR];
		next = run->extio->next;

		if (handler != XP_NULL)
		{
			n = handler (XP_AWK_IO_CLOSE, run->extio, XP_NULL, 0);
			if (n == -1)
			{
				/* TODO: 
				 * some warning actions need to be taken */
			}
		}

		xp_free (run->extio->name);
		xp_free (run->extio);

		run->extio = next;
	}
}
