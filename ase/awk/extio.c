/*
 * $Id: extio.c,v 1.38 2006-08-30 07:15:14 bacon Exp $
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

	__MASK_CLEAR = 0x00FF
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

static int __writeextio (
	xp_awk_run_t* run, int out_type, 
	const xp_char_t* name, xp_awk_val_t* v, xp_bool_t nl);

int xp_awk_readextio (
	xp_awk_run_t* run, int in_type,
	const xp_char_t* name, xp_str_t* buf)
{
	xp_awk_extio_t* p = run->extio.chain;
	xp_awk_io_t handler;
	int extio_type, extio_mode, extio_mask, n, ret;
	xp_awk_val_t* rs;
	xp_char_t* rs_ptr;
	xp_size_t rs_len;
	xp_size_t line_len = 0;

	xp_assert (in_type >= 0 && in_type <= xp_countof(__in_type_map));
	xp_assert (in_type >= 0 && in_type <= xp_countof(__in_mode_map));
	xp_assert (in_type >= 0 && in_type <= xp_countof(__in_mask_map));

	/* translate the in_type into the relevant extio type and mode */
	extio_type = __in_type_map[in_type];
	extio_mode = __in_mode_map[in_type];
	extio_mask = __in_mask_map[in_type];

	handler = run->extio.handler[extio_type];
	if (handler == XP_NULL)
	{
		/* no io handler provided */
		run->errnum = XP_AWK_EIOIMPL; /* TODO: change the error code */
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
			run->errnum = XP_AWK_ENOMEM;
			return -1;
		}

		p->name = xp_strdup (name);
		if (p->name == XP_NULL)
		{
			xp_free (p);
			run->errnum = XP_AWK_ENOMEM;
			return -1;
		}

		p->type = (extio_type | extio_mask);
		p->mode = extio_mode;
		p->handle = XP_NULL;

		p->in.buf[0] = XP_T('\0');
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
			if (xp_awk_setglobal (
				run, XP_AWK_GLOBAL_ERRNO, 
				xp_awk_val_one) == -1) return -1;

			run->errnum = XP_AWK_EIOHANDLER;
			return -1;
		}

		/* chain it */
		p->next = run->extio.chain;
		run->extio.chain = p;

		/* n == 0 indicates that it has reached the end of input. 
		 * the user io handler can return 0 for the open request
		 * if it doesn't have any files to open. One advantage 
		 * of doing this would be that you can skip the entire
		 * pattern-block matching and exeuction. */
		if (n == 0) return 0;
	}

	/* ready to read a line */
	xp_str_clear (buf);

	/* get the record separator */
	rs = xp_awk_getglobal (run, XP_AWK_GLOBAL_RS);
	xp_awk_refupval (rs);

	if (rs->type == XP_AWK_VAL_NIL)
	{
		rs_ptr = XP_NULL;
		rs_len = 0;
	}
	else if (rs->type == XP_AWK_VAL_STR)
	{
		rs_ptr = ((xp_awk_val_str_t*)rs)->buf;
		rs_len = ((xp_awk_val_str_t*)rs)->len;
	}
	else 
	{
		rs_ptr = xp_awk_valtostr (
			rs, &run->errnum, xp_true, XP_NULL, &rs_len);
		if (rs_ptr == XP_NULL)
		{
			xp_awk_refdownval (run, rs);
			return -1;
		}
	}

	ret = 1;

	/* call the io handler */
	while (1)
	{
		xp_char_t c;

		if (p->in.pos >= p->in.len)
		{
			xp_ssize_t n;

			if (p->in.eof)
			{
				if (XP_STR_LEN(buf) == 0) ret = 0;
				break;
			}

			n = handler (XP_AWK_IO_READ, p, 
				p->in.buf, xp_countof(p->in.buf));
			if (n == -1) 
			{
				/* handler error. getline should return -1 */
				/* TODO: use meaningful error code */
				if (xp_awk_setglobal (
					run, XP_AWK_GLOBAL_ERRNO, 
					xp_awk_val_one) == -1) 
				{
					ret = -1;
				}
				else
				{
					run->errnum = XP_AWK_EIOHANDLER;
					ret = -1;
				}
				break;
			}

			if (n == 0) 
			{
				p->in.eof = xp_true;
				if (XP_STR_LEN(buf) == 0) ret = 0;
				break;
			}

			p->in.len = n;
			p->in.pos = 0;
		}

		c = p->in.buf[p->in.pos++];

		if (rs_ptr == XP_NULL)
		{
			/* separate by a new line */
			/* TODO: handle different line terminator like \r\n */
			if (c == XP_T('\n')) break;
		}
		else if (rs_len == 0)
		{
			/* separate by a blank line */
			/* TODO: handle different line terminator like \r\n */
			if (line_len == 0 && c == XP_T('\n'))
			{
				if (XP_STR_LEN(buf) <= 0) 
				{
					/* if the record is empty when a blank 
					 * line is encountered, the line 
					 * terminator should not be added to 
					 * the record */
					continue;
				}

				/* when a blank line is encountered,
				 * it needs to snip off the line 
				 * terminator of the previous line */
				/* TODO: handle different line terminator like \r\n */
				XP_STR_LEN(buf) -= 1;
				break;
			}
		}
		else if (rs_len == 1)
		{
			if (c == rs_ptr[0]) break;
		}
		else
		{
			xp_char_t* match_ptr;
			xp_size_t match_len;

			xp_assert (run->extio.rs_rex != NULL);

			/* TODO: safematchrex */
			n = xp_awk_matchrex (run->extio.rs_rex, 
				XP_STR_BUF(buf), XP_STR_LEN(buf), 
				&match_ptr, &match_len, &run->errnum);
			if (n == -1)
			{
				ret = -1;
				break;
			}

			if (n == 1)
			{
				/* the match should be found at the end of
				 * the current buffer */
				xp_assert (
					XP_STR_BUF(buf) + XP_STR_LEN(buf) ==
					match_ptr + match_len);

				XP_STR_LEN(buf) -= match_len;
				break;
			}
		}

		if (xp_str_ccat (buf, c) == (xp_size_t)-1)
		{
			run->errnum = XP_AWK_ENOMEM;
			ret = -1;
			break;
		}

		/* TODO: handle different line terminator like \r\n */
		if (c == XP_T('\n')) line_len = 0;
		else line_len = line_len + 1;
	}

	if (rs_ptr != XP_NULL && rs->type != XP_AWK_VAL_STR) xp_free (rs_ptr);
	xp_awk_refdownval (run, rs);

	/* increment NR */
	if (ret != -1)
	{
		xp_awk_val_t* nr;
		xp_long_t lv;
		xp_real_t rv;

		nr = xp_awk_getglobal (run, XP_AWK_GLOBAL_NR);
		xp_awk_refupval (nr);

		n = xp_awk_valtonum (nr, &lv, &rv);
		xp_awk_refdownval (run, nr);

		if (n == -1)
		{
			run->errnum = XP_AWK_EVALTYPE;
			ret = -1;
		}
		else
		{
			if (n == 1) lv = (xp_long_t)rv;

			nr = xp_awk_makeintval (run, lv + 1);
			if (nr == XP_NULL) 
			{
				run->errnum = XP_AWK_ENOMEM;
				ret = -1;
			}
			else 
			{
				if (xp_awk_setglobal (
					run, XP_AWK_GLOBAL_NR, nr) == -1) ret = -1;
			}
		}
	}

	return ret;
}

int xp_awk_writeextio (
	xp_awk_run_t* run, int out_type, 
	const xp_char_t* name, xp_awk_val_t* v)
{
	return __writeextio (run, out_type, name, v, xp_false);
}

int xp_awk_writeextio_nl (
	xp_awk_run_t* run, int out_type, 
	const xp_char_t* name, xp_awk_val_t* v)
{
	return __writeextio (run, out_type, name, v, xp_true);
}

static int __writeextio (
	xp_awk_run_t* run, int out_type, 
	const xp_char_t* name, xp_awk_val_t* v, xp_bool_t nl)
{
	xp_awk_extio_t* p = run->extio.chain;
	xp_awk_io_t handler;
	xp_char_t* str;
	xp_size_t len;
	int extio_type, extio_mode, extio_mask, n;

	xp_assert (out_type >= 0 && out_type <= xp_countof(__out_type_map));
	xp_assert (out_type >= 0 && out_type <= xp_countof(__out_mode_map));
	xp_assert (out_type >= 0 && out_type <= xp_countof(__out_mask_map));

	/* translate the out_type into the relevant extio type and mode */
	extio_type = __out_type_map[out_type];
	extio_mode = __out_mode_map[out_type];
	extio_mask = __out_mask_map[out_type];

	handler = run->extio.handler[extio_type];
	if (handler == XP_NULL)
	{
		/* no io handler provided */
		run->errnum = XP_AWK_EIOIMPL; /* TODO: change the error code */
		return -1;
	}

	if (v->type == XP_AWK_VAL_STR)
	{
		str = ((xp_awk_val_str_t*)v)->buf;
		len = ((xp_awk_val_str_t*)v)->len;
	}
	else
	{
		/* convert the value to string representation first */

		/* TOOD: consider using a shared buffer when calling
		 *       xp_awk_valtostr. maybe run->shared_buf.extio */
		str = xp_awk_valtostr (
			v, &run->errnum, xp_true, NULL, &len);
		if (str == XP_NULL) return -1;
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
			if (v->type != XP_AWK_VAL_STR) xp_free (str);
			run->errnum = XP_AWK_ENOMEM;
			return -1;
		}

		p->name = xp_strdup (name);
		if (p->name == XP_NULL)
		{
			xp_free (p);
			if (v->type != XP_AWK_VAL_STR) xp_free (str);
			run->errnum = XP_AWK_ENOMEM;
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
			if (v->type != XP_AWK_VAL_STR) xp_free (str);
				
			/* TODO: use meaningful error code */
			if (xp_awk_setglobal (
				run, XP_AWK_GLOBAL_ERRNO, 
				xp_awk_val_one) == -1) return -1;

			run->errnum = XP_AWK_EIOHANDLER;
			return -1;
		}

		/* chain it */
		p->next = run->extio.chain;
		run->extio.chain = p;

		/* read the comment in xp_awk_readextio */
		if (n == 0) return 0;
	}

/* TODO: */
/* TODO: */
/* TODO: if write handler returns less than the request, loop */
/* TODO: */
/* TODO: */
	if (len > 0)
	{
		n = handler (XP_AWK_IO_WRITE, p, str, len);

		if (n == -1) 
		{
			if (v->type != XP_AWK_VAL_STR) xp_free (str);

			/* TODO: use meaningful error code */
			if (xp_awk_setglobal (
				run, XP_AWK_GLOBAL_ERRNO, 
				xp_awk_val_one) == -1) return -1;

			run->errnum = XP_AWK_EIOHANDLER;
			return -1;
		}

		if (n == 0)
		{
			if (v->type != XP_AWK_VAL_STR) xp_free (str);
			return 0;
		}
	}

	if (v->type != XP_AWK_VAL_STR) xp_free (str);

	if (nl)
	{
		/* TODO: use proper NEWLINE separator */
		n = handler (XP_AWK_IO_WRITE, p, XP_T("\n"), 1);
		if (n == -1) 
		{
			/* TODO: use meaningful error code */
			if (xp_awk_setglobal (
				run, XP_AWK_GLOBAL_ERRNO, 
				xp_awk_val_one) == -1) return -1;

			run->errnum = XP_AWK_EIOHANDLER;
			return -1;
		}

		if (n == 0) return 0;
	}

	return 1;
}

int xp_awk_flushextio (
	xp_awk_run_t* run, int out_type, const xp_char_t* name)
{
	xp_awk_extio_t* p = run->extio.chain;
	xp_awk_io_t handler;
	int extio_type, extio_mode, extio_mask, n;
	xp_bool_t ok = xp_false;

	xp_assert (out_type >= 0 && out_type <= xp_countof(__out_type_map));
	xp_assert (out_type >= 0 && out_type <= xp_countof(__out_mode_map));
	xp_assert (out_type >= 0 && out_type <= xp_countof(__out_mask_map));

	/* translate the out_type into the relevant extio type and mode */
	extio_type = __out_type_map[out_type];
	extio_mode = __out_mode_map[out_type];
	extio_mask = __out_mask_map[out_type];

	handler = run->extio.handler[extio_type];
	if (handler == XP_NULL)
	{
		/* no io handler provided */
		run->errnum = XP_AWK_EIOIMPL; /* TODO: change the error code */
		return -1;
	}

	/* look for the corresponding extio for name */
	while (p != XP_NULL)
	{
		if (p->type == (extio_type | extio_mask) && 
		    (name == XP_NULL || xp_strcmp (p->name, name) == 0)) 
		{
			n = handler (XP_AWK_IO_FLUSH, p, XP_NULL, 0);

			if (n == -1) 
			{
				/* TODO: use meaningful error code */
				if (xp_awk_setglobal (
					run, XP_AWK_GLOBAL_ERRNO, 
					xp_awk_val_one) == -1) return -1;

				run->errnum = XP_AWK_EIOHANDLER;
				return -1;
			}

			ok = xp_true;
		}

		p = p->next;
	}

	if (ok) return 0;

	/* there is no corresponding extio for name */
	/* TODO: use meaningful error code. but is this needed? */
	if (xp_awk_setglobal (
		run, XP_AWK_GLOBAL_ERRNO, xp_awk_val_one) == -1) return -1;

	run->errnum = XP_AWK_ENOSUCHIO;
	return -1;
}

int xp_awk_nextextio_read (
	xp_awk_run_t* run, int in_type, const xp_char_t* name)
{
	xp_awk_extio_t* p = run->extio.chain;
	xp_awk_io_t handler;
	int extio_type, extio_mode, extio_mask, n;

	xp_assert (in_type >= 0 && in_type <= xp_countof(__in_type_map));
	xp_assert (in_type >= 0 && in_type <= xp_countof(__in_mode_map));
	xp_assert (in_type >= 0 && in_type <= xp_countof(__in_mask_map));

	/* translate the in_type into the relevant extio type and mode */
	extio_type = __in_type_map[in_type];
	extio_mode = __in_mode_map[in_type];
	extio_mask = __in_mask_map[in_type];

	handler = run->extio.handler[extio_type];
	if (handler == XP_NULL)
	{
		/* no io handler provided */
		run->errnum = XP_AWK_EIOIMPL; /* TODO: change the error code */
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
		run->errnum = XP_AWK_EINTERNAL;
		return -1;
	}

	n = handler (XP_AWK_IO_NEXT, p, XP_NULL, 0);
	if (n == -1)
	{
		/* TODO: is this errnum correct? */
		run->errnum = XP_AWK_EIOHANDLER;
		return -1;
	}

	return n;
}

int xp_awk_closeextio_read (
	xp_awk_run_t* run, int in_type, const xp_char_t* name)
{
	xp_awk_extio_t* p = run->extio.chain, * px = XP_NULL;
	xp_awk_io_t handler;
	int extio_type, extio_mode, extio_mask;

	xp_assert (in_type >= 0 && in_type <= xp_countof(__in_type_map));
	xp_assert (in_type >= 0 && in_type <= xp_countof(__in_mode_map));
	xp_assert (in_type >= 0 && in_type <= xp_countof(__in_mask_map));

	/* translate the in_type into the relevant extio type and mode */
	extio_type = __in_type_map[in_type];
	extio_mode = __in_mode_map[in_type];
	extio_mask = __in_mask_map[in_type];

	handler = run->extio.handler[extio_type];
	if (handler == XP_NULL)
	{
		/* no io handler provided */
		run->errnum = XP_AWK_EIOIMPL; /* TODO: change the error code */
		return -1;
	}

	while (p != XP_NULL)
	{
		if (p->type == (extio_type | extio_mask) &&
		    xp_strcmp (p->name, name) == 0) 
		{
			xp_awk_io_t handler;
		       
			handler = run->extio.handler[p->type & __MASK_CLEAR];
			if (handler != XP_NULL)
			{
				if (handler (XP_AWK_IO_CLOSE, p, XP_NULL, 0) == -1)
				{
					/* this is not a run-time error.*/
					/* TODO: set ERRNO */
					run->errnum = XP_AWK_EIOHANDLER;
					return -1;
				}
			}

			if (px != XP_NULL) px->next = p->next;
			else run->extio.chain = p->next;

			xp_free (p->name);
			xp_free (p);
			return 0;
		}

		px = p;
		p = p->next;
	}

	/* this is not a run-time error */
	run->errnum = XP_AWK_EIOHANDLER;
	return -1;
}

int xp_awk_closeextio_write (
	xp_awk_run_t* run, int out_type, const xp_char_t* name)
{
	xp_awk_extio_t* p = run->extio.chain, * px = XP_NULL;
	xp_awk_io_t handler;
	int extio_type, extio_mode, extio_mask;

	xp_assert (out_type >= 0 && out_type <= xp_countof(__out_type_map));
	xp_assert (out_type >= 0 && out_type <= xp_countof(__out_mode_map));
	xp_assert (out_type >= 0 && out_type <= xp_countof(__out_mask_map));

	/* translate the out_type into the relevant extio type and mode */
	extio_type = __out_type_map[out_type];
	extio_mode = __out_mode_map[out_type];
	extio_mask = __out_mask_map[out_type];

	handler = run->extio.handler[extio_type];
	if (handler == XP_NULL)
	{
		/* no io handler provided */
		run->errnum = XP_AWK_EIOIMPL; /* TODO: change the error code */
		return -1;
	}

	while (p != XP_NULL)
	{
		if (p->type == (extio_type | extio_mask) &&
		    xp_strcmp (p->name, name) == 0) 
		{
			xp_awk_io_t handler;
		       
			handler = run->extio.handler[p->type & __MASK_CLEAR];
			if (handler != XP_NULL)
			{
				if (handler (XP_AWK_IO_CLOSE, p, XP_NULL, 0) == -1)
				{
					/* this is not a run-time error.*/
					/* TODO: set ERRNO */
					run->errnum = XP_AWK_EIOHANDLER;
					return -1;
				}
			}

			if (px != XP_NULL) px->next = p->next;
			else run->extio.chain = p->next;

			xp_free (p->name);
			xp_free (p);
			return 0;
		}

		px = p;
		p = p->next;
	}

	/* this is not a run-time error */
	/* TODO: set ERRNO */
	run->errnum = XP_AWK_EIOHANDLER;
	return -1;
}

int xp_awk_closeextio (xp_awk_run_t* run, const xp_char_t* name)
{
	xp_awk_extio_t* p = run->extio.chain, * px = XP_NULL;

	while (p != XP_NULL)
	{
		 /* it handles the first that matches the given name
		  * regardless of the extio type */
		if (xp_strcmp (p->name, name) == 0) 
		{
			xp_awk_io_t handler;
		       
			handler = run->extio.handler[p->type & __MASK_CLEAR];
			if (handler != XP_NULL)
			{
				if (handler (XP_AWK_IO_CLOSE, p, XP_NULL, 0) == -1)
				{
					/* this is not a run-time error.*/
					/* TODO: set ERRNO */
					run->errnum = XP_AWK_EIOHANDLER;
					return -1;
				}
			}

			if (px != XP_NULL) px->next = p->next;
			else run->extio.chain = p->next;

			xp_free (p->name);
			xp_free (p);
			return 0;
		}

		px = p;
		p = p->next;
	}

	/* this is not a run-time error */
	/* TODO: set ERRNO */
	run->errnum = XP_AWK_EIOHANDLER;
	return -1;
}

void xp_awk_clearextio (xp_awk_run_t* run)
{
	xp_awk_extio_t* next;
	xp_awk_io_t handler;
	int n;

	while (run->extio.chain != XP_NULL)
	{
		handler = run->extio.handler[run->extio.chain->type & __MASK_CLEAR];
		next = run->extio.chain->next;

		if (handler != XP_NULL)
		{
			n = handler (XP_AWK_IO_CLOSE, run->extio.chain, XP_NULL, 0);
			if (n == -1)
			{
				/* TODO: 
				 * some warning actions need to be taken */
			}
		}

		xp_free (run->extio.chain->name);
		xp_free (run->extio.chain);

		run->extio.chain = next;
	}
}
