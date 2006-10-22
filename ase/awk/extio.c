/*
 * $Id: extio.c,v 1.55 2006-10-22 11:34:53 bacon Exp $
 */

#include <sse/awk/awk_i.h>

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
	 * SSE_AWK_IN_XXX values in tree.h */
	SSE_AWK_EXTIO_PIPE,
	SSE_AWK_EXTIO_COPROC,
	SSE_AWK_EXTIO_FILE,
	SSE_AWK_EXTIO_CONSOLE
};

static int __in_mode_map[] =
{
	/* the order should match the order of the 
	 * SSE_AWK_IN_XXX values in tree.h */
	SSE_AWK_IO_PIPE_READ,
	0,
	SSE_AWK_IO_FILE_READ,
	SSE_AWK_IO_CONSOLE_READ
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
	 * SSE_AWK_OUT_XXX values in tree.h */
	SSE_AWK_EXTIO_PIPE,
	SSE_AWK_EXTIO_COPROC,
	SSE_AWK_EXTIO_FILE,
	SSE_AWK_EXTIO_FILE,
	SSE_AWK_EXTIO_CONSOLE
};

static int __out_mode_map[] =
{
	/* the order should match the order of the 
	 * SSE_AWK_OUT_XXX values in tree.h */
	SSE_AWK_IO_PIPE_WRITE,
	0,
	SSE_AWK_IO_FILE_WRITE,
	SSE_AWK_IO_FILE_APPEND,
	SSE_AWK_IO_CONSOLE_WRITE
};

static int __out_mask_map[] =
{
	__MASK_WRITE,
	__MASK_RDWR,
	__MASK_WRITE,
	__MASK_WRITE,
	__MASK_WRITE
};

int sse_awk_readextio (
	sse_awk_run_t* run, int in_type,
	const sse_char_t* name, sse_awk_str_t* buf)
{
	sse_awk_extio_t* p = run->extio.chain;
	sse_awk_io_t handler;
	int extio_type, extio_mode, extio_mask, n, ret;
	sse_awk_val_t* rs;
	sse_char_t* rs_ptr;
	sse_size_t rs_len;
	sse_size_t line_len = 0;

	sse_awk_assert (run->awk,
		in_type >= 0 && in_type <= sse_countof(__in_type_map));
	sse_awk_assert (run->awk,
		in_type >= 0 && in_type <= sse_countof(__in_mode_map));
	sse_awk_assert (run->awk,
		in_type >= 0 && in_type <= sse_countof(__in_mask_map));

	/* translate the in_type into the relevant extio type and mode */
	extio_type = __in_type_map[in_type];
	extio_mode = __in_mode_map[in_type];
	extio_mask = __in_mask_map[in_type];

	handler = run->extio.handler[extio_type];
	if (handler == SSE_NULL)
	{
		/* no io handler provided */
		run->errnum = SSE_AWK_EIOIMPL; /* TODO: change the error code */
		return -1;
	}

	while (p != SSE_NULL)
	{
		if (p->type == (extio_type | extio_mask) &&
		    sse_awk_strcmp (p->name,name) == 0) break;
		p = p->next;
	}

	if (p == SSE_NULL)
	{
		p = (sse_awk_extio_t*) SSE_AWK_MALLOC (
			run->awk, sse_sizeof(sse_awk_extio_t));
		if (p == SSE_NULL)
		{
			run->errnum = SSE_AWK_ENOMEM;
			return -1;
		}

		p->name = sse_awk_strdup (run->awk, name);
		if (p->name == SSE_NULL)
		{
			SSE_AWK_FREE (run->awk, p);
			run->errnum = SSE_AWK_ENOMEM;
			return -1;
		}

		p->run = run;
		p->type = (extio_type | extio_mask);
		p->mode = extio_mode;
		p->handle = SSE_NULL;
		p->next = SSE_NULL;
		p->custom_data = run->extio.custom_data;

		p->in.buf[0] = SSE_T('\0');
		p->in.pos = 0;
		p->in.len = 0;
		p->in.eof = sse_false;

		n = handler (SSE_AWK_IO_OPEN, p, SSE_NULL, 0);
		if (n == -1)
		{
			SSE_AWK_FREE (run->awk, p->name);
			SSE_AWK_FREE (run->awk, p);
				
			/* TODO: use meaningful error code */
			if (sse_awk_setglobal (
				run, SSE_AWK_GLOBAL_ERRNO, 
				sse_awk_val_one) == -1) return -1;

			run->errnum = SSE_AWK_EIOHANDLER;
			return -1;
		}

		/* chain it */
		p->next = run->extio.chain;
		run->extio.chain = p;

		/* usually, n == 0 indicates that it has reached the end 
		 * of the input. the user io handler can return 0 for the 
		 * open request if it doesn't have any files to open. One 
		 * advantage of doing this would be that you can skip the 
		 * entire pattern-block matching and exeuction. */
		if (n == 0) return 0;
	}

	/* ready to read a line */
	sse_awk_str_clear (buf);

	/* get the record separator */
	rs = sse_awk_getglobal (run, SSE_AWK_GLOBAL_RS);
	sse_awk_refupval (rs);

	if (rs->type == SSE_AWK_VAL_NIL)
	{
		rs_ptr = SSE_NULL;
		rs_len = 0;
	}
	else if (rs->type == SSE_AWK_VAL_STR)
	{
		rs_ptr = ((sse_awk_val_str_t*)rs)->buf;
		rs_len = ((sse_awk_val_str_t*)rs)->len;
	}
	else 
	{
		rs_ptr = sse_awk_valtostr (
			run, rs, SSE_AWK_VALTOSTR_CLEAR, SSE_NULL, &rs_len);
		if (rs_ptr == SSE_NULL)
		{
			sse_awk_refdownval (run, rs);
			return -1;
		}
	}

	ret = 1;

	/* call the io handler */
	while (1)
	{
		sse_char_t c;

		if (p->in.pos >= p->in.len)
		{
			sse_ssize_t n;

			if (p->in.eof)
			{
				if (SSE_AWK_STR_LEN(buf) == 0) ret = 0;
				break;
			}

			n = handler (SSE_AWK_IO_READ, p, 
				p->in.buf, sse_countof(p->in.buf));
			if (n == -1) 
			{
				/* handler error. getline should return -1 */
				/* TODO: use meaningful error code */
				if (sse_awk_setglobal (
					run, SSE_AWK_GLOBAL_ERRNO, 
					sse_awk_val_one) == -1) 
				{
					ret = -1;
				}
				else
				{
					run->errnum = SSE_AWK_EIOHANDLER;
					ret = -1;
				}
				break;
			}

			if (n == 0) 
			{
				p->in.eof = sse_true;
				if (SSE_AWK_STR_LEN(buf) == 0) ret = 0;
				break;
			}

			p->in.len = n;
			p->in.pos = 0;
		}

		c = p->in.buf[p->in.pos++];

		if (rs_ptr == SSE_NULL)
		{
			/* separate by a new line */
			/* TODO: handle different line terminator like \r\n */
			if (c == SSE_T('\n')) break;
		}
		else if (rs_len == 0)
		{
			/* separate by a blank line */
			/* TODO: handle different line terminator like \r\n */
			if (line_len == 0 && c == SSE_T('\n'))
			{
				if (SSE_AWK_STR_LEN(buf) <= 0) 
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
				SSE_AWK_STR_LEN(buf) -= 1;
				break;
			}
		}
		else if (rs_len == 1)
		{
			if (c == rs_ptr[0]) break;
		}
		else
		{
			const sse_char_t* match_ptr;
			sse_size_t match_len;

			sse_awk_assert (run->awk, run->global.rs != SSE_NULL);

			/* TODO: safematchrex */
			n = sse_awk_matchrex (
				run->awk, run->global.rs, 
				((run->global.ignorecase)? SSE_AWK_REX_IGNORECASE: 0),
				SSE_AWK_STR_BUF(buf), SSE_AWK_STR_LEN(buf), 
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
				sse_awk_assert (run->awk,
					SSE_AWK_STR_BUF(buf) + SSE_AWK_STR_LEN(buf) ==
					match_ptr + match_len);

				SSE_AWK_STR_LEN(buf) -= match_len;
				break;
			}
		}

		if (sse_awk_str_ccat (buf, c) == (sse_size_t)-1)
		{
			run->errnum = SSE_AWK_ENOMEM;
			ret = -1;
			break;
		}

		/* TODO: handle different line terminator like \r\n */
		if (c == SSE_T('\n')) line_len = 0;
		else line_len = line_len + 1;
	}

	if (rs_ptr != SSE_NULL && rs->type != SSE_AWK_VAL_STR) SSE_AWK_FREE (run->awk, rs_ptr);
	sse_awk_refdownval (run, rs);

	/* increment NR */
	if (ret != -1)
	{
		sse_awk_val_t* nr;
		sse_long_t lv;
		sse_real_t rv;

		nr = sse_awk_getglobal (run, SSE_AWK_GLOBAL_NR);
		sse_awk_refupval (nr);

		n = sse_awk_valtonum (run, nr, &lv, &rv);
		sse_awk_refdownval (run, nr);

		if (n == -1) ret = -1;
		else
		{
			if (n == 1) lv = (sse_long_t)rv;

			nr = sse_awk_makeintval (run, lv + 1);
			if (nr == SSE_NULL) 
			{
				run->errnum = SSE_AWK_ENOMEM;
				ret = -1;
			}
			else 
			{
				if (sse_awk_setglobal (
					run, SSE_AWK_GLOBAL_NR, nr) == -1) ret = -1;
			}
		}
	}

	return ret;
}

int sse_awk_writeextio_val (
	sse_awk_run_t* run, int out_type, 
	const sse_char_t* name, sse_awk_val_t* v)
{
	sse_char_t* str;
	sse_size_t len;
	int n;

	if (v->type == SSE_AWK_VAL_STR)
	{
		str = ((sse_awk_val_str_t*)v)->buf;
		len = ((sse_awk_val_str_t*)v)->len;
	}
	else
	{
		str = sse_awk_valtostr (
			run, v, 
			SSE_AWK_VALTOSTR_CLEAR | SSE_AWK_VALTOSTR_PRINT, 
			SSE_NULL, &len);
		if (str == SSE_NULL) return -1;
	}

	n = sse_awk_writeextio_str (run, out_type, name, str, len);

	if (v->type != SSE_AWK_VAL_STR) SSE_AWK_FREE (run->awk, str);
	return n;
}

int sse_awk_writeextio_str (
	sse_awk_run_t* run, int out_type, 
	const sse_char_t* name, sse_char_t* str, sse_size_t len)
{
	sse_awk_extio_t* p = run->extio.chain;
	sse_awk_io_t handler;
	int extio_type, extio_mode, extio_mask, n;

	sse_awk_assert (run->awk, 
		out_type >= 0 && out_type <= sse_countof(__out_type_map));
	sse_awk_assert (run->awk,
		out_type >= 0 && out_type <= sse_countof(__out_mode_map));
	sse_awk_assert (run->awk,
		out_type >= 0 && out_type <= sse_countof(__out_mask_map));

	/* translate the out_type into the relevant extio type and mode */
	extio_type = __out_type_map[out_type];
	extio_mode = __out_mode_map[out_type];
	extio_mask = __out_mask_map[out_type];

	handler = run->extio.handler[extio_type];
	if (handler == SSE_NULL)
	{
		/* no io handler provided */
		run->errnum = SSE_AWK_EIOIMPL; /* TODO: change the error code */
		return -1;
	}

	/* look for the corresponding extio for name */
	while (p != SSE_NULL)
	{
		/* the file "1.tmp", in the following code snippets, 
		 * would be opened by the first print statement, but not by
		 * the second print statement. this is because
		 * both SSE_AWK_OUT_FILE and SSE_AWK_OUT_FILE_APPEND are
		 * translated to SSE_AWK_EXTIO_FILE and it is used to
		 * keep track of file handles..
		 *
		 *    print "1111" >> "1.tmp"
		 *    print "1111" > "1.tmp"
		 */
		if (p->type == (extio_type | extio_mask) && 
		    sse_awk_strcmp (p->name, name) == 0) break;
		p = p->next;
	}

	/* if there is not corresponding extio for name, create one */
	if (p == SSE_NULL)
	{
		p = (sse_awk_extio_t*) SSE_AWK_MALLOC (
			run->awk, sse_sizeof(sse_awk_extio_t));
		if (p == SSE_NULL)
		{
			run->errnum = SSE_AWK_ENOMEM;
			return -1;
		}

		p->name = sse_awk_strdup (run->awk, name);
		if (p->name == SSE_NULL)
		{
			SSE_AWK_FREE (run->awk, p);
			run->errnum = SSE_AWK_ENOMEM;
			return -1;
		}

		p->run = run;
		p->type = (extio_type | extio_mask);
		p->mode = extio_mode;
		p->handle = SSE_NULL;
		p->next = SSE_NULL;
		p->custom_data = run->extio.custom_data;

		n = handler (SSE_AWK_IO_OPEN, p, SSE_NULL, 0);
		if (n == -1)
		{
			SSE_AWK_FREE (run->awk, p->name);
			SSE_AWK_FREE (run->awk, p);
				
			/* TODO: use meaningful error code */
			if (sse_awk_setglobal (
				run, SSE_AWK_GLOBAL_ERRNO, 
				sse_awk_val_one) == -1) return -1;

			run->errnum = SSE_AWK_EIOHANDLER;
			return -1;
		}

		/* chain it */
		p->next = run->extio.chain;
		run->extio.chain = p;

		/* usually, n == 0 indicates that it has reached the end 
		 * of the input. the user io handler can return 0 for the 
		 * open request if it doesn't have any files to open. One 
		 * advantage of doing this would be that you can skip the 
		 * entire pattern-block matching and exeuction. */
		if (n == 0) return 0;
	}

/* TODO: if write handler returns less than the request, loop */
	if (len > 0)
	{
		n = handler (SSE_AWK_IO_WRITE, p, str, len);

		if (n == -1) 
		{
			/* TODO: use meaningful error code */
			if (sse_awk_setglobal (
				run, SSE_AWK_GLOBAL_ERRNO, 
				sse_awk_val_one) == -1) return -1;

			run->errnum = SSE_AWK_EIOHANDLER;
			return -1;
		}

		if (n == 0) return 0;
	}

	return 1;
}


int sse_awk_flushextio (
	sse_awk_run_t* run, int out_type, const sse_char_t* name)
{
	sse_awk_extio_t* p = run->extio.chain;
	sse_awk_io_t handler;
	int extio_type, extio_mode, extio_mask, n;
	sse_bool_t ok = sse_false;

	sse_awk_assert (run->awk,
		out_type >= 0 && out_type <= sse_countof(__out_type_map));
	sse_awk_assert (run->awk,
		out_type >= 0 && out_type <= sse_countof(__out_mode_map));
	sse_awk_assert (run->awk,
		out_type >= 0 && out_type <= sse_countof(__out_mask_map));

	/* translate the out_type into the relevant extio type and mode */
	extio_type = __out_type_map[out_type];
	extio_mode = __out_mode_map[out_type];
	extio_mask = __out_mask_map[out_type];

	handler = run->extio.handler[extio_type];
	if (handler == SSE_NULL)
	{
		/* no io handler provided */
		run->errnum = SSE_AWK_EIOIMPL; /* TODO: change the error code */
		return -1;
	}

	/* look for the corresponding extio for name */
	while (p != SSE_NULL)
	{
		if (p->type == (extio_type | extio_mask) && 
		    (name == SSE_NULL || sse_awk_strcmp (p->name, name) == 0)) 
		{
			n = handler (SSE_AWK_IO_FLUSH, p, SSE_NULL, 0);

			if (n == -1) 
			{
				/* TODO: use meaningful error code */
				if (sse_awk_setglobal (
					run, SSE_AWK_GLOBAL_ERRNO, 
					sse_awk_val_one) == -1) return -1;

				run->errnum = SSE_AWK_EIOHANDLER;
				return -1;
			}

			ok = sse_true;
		}

		p = p->next;
	}

	if (ok) return 0;

	/* there is no corresponding extio for name */
	/* TODO: use meaningful error code. but is this needed? */
	if (sse_awk_setglobal (
		run, SSE_AWK_GLOBAL_ERRNO, sse_awk_val_one) == -1) return -1;

	run->errnum = SSE_AWK_ENOSUCHIO;
	return -1;
}

int sse_awk_nextextio_read (
	sse_awk_run_t* run, int in_type, const sse_char_t* name)
{
	sse_awk_extio_t* p = run->extio.chain;
	sse_awk_io_t handler;
	int extio_type, extio_mode, extio_mask, n;

	sse_awk_assert (run->awk,
		in_type >= 0 && in_type <= sse_countof(__in_type_map));
	sse_awk_assert (run->awk,
		in_type >= 0 && in_type <= sse_countof(__in_mode_map));
	sse_awk_assert (run->awk,
		in_type >= 0 && in_type <= sse_countof(__in_mask_map));

	/* translate the in_type into the relevant extio type and mode */
	extio_type = __in_type_map[in_type];
	extio_mode = __in_mode_map[in_type];
	extio_mask = __in_mask_map[in_type];

	handler = run->extio.handler[extio_type];
	if (handler == SSE_NULL)
	{
		/* no io handler provided */
		run->errnum = SSE_AWK_EIOIMPL; /* TODO: change the error code */
		return -1;
	}

	while (p != SSE_NULL)
	{
		if (p->type == (extio_type | extio_mask) &&
		    sse_awk_strcmp (p->name,name) == 0) break;
		p = p->next;
	}

	if (p == SSE_NULL)
	{
		/* something is totally wrong */
		run->errnum = SSE_AWK_EINTERNAL;
		return -1;
	}

	n = handler (SSE_AWK_IO_NEXT, p, SSE_NULL, 0);
	if (n == -1)
	{
		/* TODO: is this errnum correct? */
		run->errnum = SSE_AWK_EIOHANDLER;
		return -1;
	}

	return n;
}

int sse_awk_closeextio_read (
	sse_awk_run_t* run, int in_type, const sse_char_t* name)
{
	sse_awk_extio_t* p = run->extio.chain, * px = SSE_NULL;
	sse_awk_io_t handler;
	int extio_type, extio_mode, extio_mask;

	sse_awk_assert (run->awk,
		in_type >= 0 && in_type <= sse_countof(__in_type_map));
	sse_awk_assert (run->awk,
		in_type >= 0 && in_type <= sse_countof(__in_mode_map));
	sse_awk_assert (run->awk,
		in_type >= 0 && in_type <= sse_countof(__in_mask_map));

	/* translate the in_type into the relevant extio type and mode */
	extio_type = __in_type_map[in_type];
	extio_mode = __in_mode_map[in_type];
	extio_mask = __in_mask_map[in_type];

	handler = run->extio.handler[extio_type];
	if (handler == SSE_NULL)
	{
		/* no io handler provided */
		run->errnum = SSE_AWK_EIOIMPL; /* TODO: change the error code */
		return -1;
	}

	while (p != SSE_NULL)
	{
		if (p->type == (extio_type | extio_mask) &&
		    sse_awk_strcmp (p->name, name) == 0) 
		{
			sse_awk_io_t handler;
		       
			handler = run->extio.handler[p->type & __MASK_CLEAR];
			if (handler != SSE_NULL)
			{
				if (handler (SSE_AWK_IO_CLOSE, p, SSE_NULL, 0) == -1)
				{
					/* this is not a run-time error.*/
					/* TODO: set ERRNO */
					run->errnum = SSE_AWK_EIOHANDLER;
					return -1;
				}
			}

			if (px != SSE_NULL) px->next = p->next;
			else run->extio.chain = p->next;

			SSE_AWK_FREE (run->awk, p->name);
			SSE_AWK_FREE (run->awk, p);
			return 0;
		}

		px = p;
		p = p->next;
	}

	/* this is not a run-time error */
	run->errnum = SSE_AWK_EIOHANDLER;
	return -1;
}

int sse_awk_closeextio_write (
	sse_awk_run_t* run, int out_type, const sse_char_t* name)
{
	sse_awk_extio_t* p = run->extio.chain, * px = SSE_NULL;
	sse_awk_io_t handler;
	int extio_type, extio_mode, extio_mask;

	sse_awk_assert (run->awk,
		out_type >= 0 && out_type <= sse_countof(__out_type_map));
	sse_awk_assert (run->awk,
		out_type >= 0 && out_type <= sse_countof(__out_mode_map));
	sse_awk_assert (run->awk,
		out_type >= 0 && out_type <= sse_countof(__out_mask_map));

	/* translate the out_type into the relevant extio type and mode */
	extio_type = __out_type_map[out_type];
	extio_mode = __out_mode_map[out_type];
	extio_mask = __out_mask_map[out_type];

	handler = run->extio.handler[extio_type];
	if (handler == SSE_NULL)
	{
		/* no io handler provided */
		run->errnum = SSE_AWK_EIOIMPL; /* TODO: change the error code */
		return -1;
	}

	while (p != SSE_NULL)
	{
		if (p->type == (extio_type | extio_mask) &&
		    sse_awk_strcmp (p->name, name) == 0) 
		{
			sse_awk_io_t handler;
		       
			handler = run->extio.handler[p->type & __MASK_CLEAR];
			if (handler != SSE_NULL)
			{
				if (handler (SSE_AWK_IO_CLOSE, p, SSE_NULL, 0) == -1)
				{
					/* this is not a run-time error.*/
					/* TODO: set ERRNO */
					run->errnum = SSE_AWK_EIOHANDLER;
					return -1;
				}
			}

			if (px != SSE_NULL) px->next = p->next;
			else run->extio.chain = p->next;

			SSE_AWK_FREE (run->awk, p->name);
			SSE_AWK_FREE (run->awk, p);
			return 0;
		}

		px = p;
		p = p->next;
	}

	/* this is not a run-time error */
	/* TODO: set ERRNO */
	run->errnum = SSE_AWK_EIOHANDLER;
	return -1;
}

int sse_awk_closeextio (sse_awk_run_t* run, const sse_char_t* name)
{
	sse_awk_extio_t* p = run->extio.chain, * px = SSE_NULL;

	while (p != SSE_NULL)
	{
		 /* it handles the first that matches the given name
		  * regardless of the extio type */
		if (sse_awk_strcmp (p->name, name) == 0) 
		{
			sse_awk_io_t handler;
		       
			handler = run->extio.handler[p->type & __MASK_CLEAR];
			if (handler != SSE_NULL)
			{
				if (handler (SSE_AWK_IO_CLOSE, p, SSE_NULL, 0) == -1)
				{
					/* this is not a run-time error.*/
					/* TODO: set ERRNO */
					run->errnum = SSE_AWK_EIOHANDLER;
					return -1;
				}
			}

			if (px != SSE_NULL) px->next = p->next;
			else run->extio.chain = p->next;

			SSE_AWK_FREE (run->awk, p->name);
			SSE_AWK_FREE (run->awk, p);
			return 0;
		}

		px = p;
		p = p->next;
	}

	/* this is not a run-time error */
	/* TODO: set ERRNO */
	run->errnum = SSE_AWK_EIOHANDLER;
	return -1;
}

void sse_awk_clearextio (sse_awk_run_t* run)
{
	sse_awk_extio_t* next;
	sse_awk_io_t handler;
	int n;

	while (run->extio.chain != SSE_NULL)
	{
		handler = run->extio.handler[
			run->extio.chain->type & __MASK_CLEAR];
		next = run->extio.chain->next;

		if (handler != SSE_NULL)
		{
			n = handler (SSE_AWK_IO_CLOSE, run->extio.chain, SSE_NULL, 0);
			if (n == -1)
			{
				/* TODO: 
				 * some warning actions need to be taken */
			}
		}

		SSE_AWK_FREE (run->awk, run->extio.chain->name);
		SSE_AWK_FREE (run->awk, run->extio.chain);

		run->extio.chain = next;
	}
}
