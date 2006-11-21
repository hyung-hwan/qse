/*
 * $Id: extio.c,v 1.61 2006-11-21 15:06:14 bacon Exp $
 */

#include <ase/awk/awk_i.h>

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
	 * ASE_AWK_IN_XXX values in tree.h */
	ASE_AWK_EXTIO_PIPE,
	ASE_AWK_EXTIO_COPROC,
	ASE_AWK_EXTIO_FILE,
	ASE_AWK_EXTIO_CONSOLE
};

static int __in_mode_map[] =
{
	/* the order should match the order of the 
	 * ASE_AWK_IN_XXX values in tree.h */
	ASE_AWK_EXTIO_PIPE_READ,
	0,
	ASE_AWK_EXTIO_FILE_READ,
	ASE_AWK_EXTIO_CONSOLE_READ
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
	 * ASE_AWK_OUT_XXX values in tree.h */
	ASE_AWK_EXTIO_PIPE,
	ASE_AWK_EXTIO_COPROC,
	ASE_AWK_EXTIO_FILE,
	ASE_AWK_EXTIO_FILE,
	ASE_AWK_EXTIO_CONSOLE
};

static int __out_mode_map[] =
{
	/* the order should match the order of the 
	 * ASE_AWK_OUT_XXX values in tree.h */
	ASE_AWK_EXTIO_PIPE_WRITE,
	0,
	ASE_AWK_EXTIO_FILE_WRITE,
	ASE_AWK_EXTIO_FILE_APPEND,
	ASE_AWK_EXTIO_CONSOLE_WRITE
};

static int __out_mask_map[] =
{
	__MASK_WRITE,
	__MASK_RDWR,
	__MASK_WRITE,
	__MASK_WRITE,
	__MASK_WRITE
};

int ase_awk_readextio (
	ase_awk_run_t* run, int in_type,
	const ase_char_t* name, ase_awk_str_t* buf)
{
	ase_awk_extio_t* p = run->extio.chain;
	ase_awk_io_t handler;
	int extio_type, extio_mode, extio_mask, n, ret;
	ase_awk_val_t* rs;
	ase_char_t* rs_ptr;
	ase_size_t rs_len;
	ase_size_t line_len = 0;

	ASE_AWK_ASSERT (run->awk,
		in_type >= 0 && in_type <= ase_countof(__in_type_map));
	ASE_AWK_ASSERT (run->awk,
		in_type >= 0 && in_type <= ase_countof(__in_mode_map));
	ASE_AWK_ASSERT (run->awk,
		in_type >= 0 && in_type <= ase_countof(__in_mask_map));

	/* translate the in_type into the relevant extio type and mode */
	extio_type = __in_type_map[in_type];
	extio_mode = __in_mode_map[in_type];
	extio_mask = __in_mask_map[in_type];

	handler = run->extio.handler[extio_type];
	if (handler == ASE_NULL)
	{
		/* no io handler provided */
		run->errnum = ASE_AWK_EIOIMPL; /* TODO: change the error code */
		return -1;
	}

	while (p != ASE_NULL)
	{
		if (p->type == (extio_type | extio_mask) &&
		    ase_awk_strcmp (p->name,name) == 0) break;
		p = p->next;
	}

	if (p == ASE_NULL)
	{
		p = (ase_awk_extio_t*) ASE_AWK_MALLOC (
			run->awk, ase_sizeof(ase_awk_extio_t));
		if (p == ASE_NULL)
		{
			run->errnum = ASE_AWK_ENOMEM;
			return -1;
		}

		p->name = ase_awk_strdup (run->awk, name);
		if (p->name == ASE_NULL)
		{
			ASE_AWK_FREE (run->awk, p);
			run->errnum = ASE_AWK_ENOMEM;
			return -1;
		}

		p->run = run;
		p->type = (extio_type | extio_mask);
		p->mode = extio_mode;
		p->handle = ASE_NULL;
		p->next = ASE_NULL;
		p->custom_data = run->extio.custom_data;

		p->in.buf[0] = ASE_T('\0');
		p->in.pos = 0;
		p->in.len = 0;
		p->in.eof = ase_false;
		p->in.eos = ase_false;

		n = handler (ASE_AWK_IO_OPEN, p, ASE_NULL, 0);
		if (n == -1)
		{
			ASE_AWK_FREE (run->awk, p->name);
			ASE_AWK_FREE (run->awk, p);
				
			/* TODO: use meaningful error code */
			if (ase_awk_setglobal (
				run, ASE_AWK_GLOBAL_ERRNO, 
				ase_awk_val_one) == -1) return -1;

			run->errnum = ASE_AWK_EIOHANDLER;
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
		if (n == 0) 
		{
			p->in.eos = ase_true;
			return 0;
		}
	}

	if (p->in.eos) 
	{
		/* no more streams. */
		return 0;
	}

	/* ready to read a line */
	ase_awk_str_clear (buf);

	/* get the record separator */
	rs = ase_awk_getglobal (run, ASE_AWK_GLOBAL_RS);
	ase_awk_refupval (run, rs);

	if (rs->type == ASE_AWK_VAL_NIL)
	{
		rs_ptr = ASE_NULL;
		rs_len = 0;
	}
	else if (rs->type == ASE_AWK_VAL_STR)
	{
		rs_ptr = ((ase_awk_val_str_t*)rs)->buf;
		rs_len = ((ase_awk_val_str_t*)rs)->len;
	}
	else 
	{
		rs_ptr = ase_awk_valtostr (
			run, rs, ASE_AWK_VALTOSTR_CLEAR, ASE_NULL, &rs_len);
		if (rs_ptr == ASE_NULL)
		{
			ase_awk_refdownval (run, rs);
			return -1;
		}
	}

	ret = 1;

	/* call the io handler */
	while (1)
	{
		ase_char_t c;

		if (p->in.pos >= p->in.len)
		{
			ase_ssize_t n;

			if (p->in.eof)
			{
				if (ASE_AWK_STR_LEN(buf) == 0) ret = 0;
				break;
			}

			n = handler (ASE_AWK_IO_READ, p, 
				p->in.buf, ase_countof(p->in.buf));
			if (n == -1) 
			{
				/* handler error. getline should return -1 */
				/* TODO: use meaningful error code */
				if (ase_awk_setglobal (
					run, ASE_AWK_GLOBAL_ERRNO, 
					ase_awk_val_one) == -1) 
				{
					ret = -1;
				}
				else
				{
					run->errnum = ASE_AWK_EIOHANDLER;
					ret = -1;
				}
				break;
			}

			if (n == 0) 
			{
				p->in.eof = ase_true;
				if (ASE_AWK_STR_LEN(buf) == 0) ret = 0;
				break;
			}

			p->in.len = n;
			p->in.pos = 0;
		}

		c = p->in.buf[p->in.pos++];

		if (rs_ptr == ASE_NULL)
		{
			/* separate by a new line */
			/* TODO: handle different line terminator like \r\n */
			if (c == ASE_T('\n')) break;
		}
		else if (rs_len == 0)
		{
			/* separate by a blank line */
			/* TODO: handle different line terminator like \r\n */
			if (line_len == 0 && c == ASE_T('\n'))
			{
				if (ASE_AWK_STR_LEN(buf) <= 0) 
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
				ASE_AWK_STR_LEN(buf) -= 1;
				break;
			}
		}
		else if (rs_len == 1)
		{
			if (c == rs_ptr[0]) break;
		}
		else
		{
			const ase_char_t* match_ptr;
			ase_size_t match_len;

			ASE_AWK_ASSERT (run->awk, run->global.rs != ASE_NULL);

			/* TODO: safematchrex */
			n = ase_awk_matchrex (
				run->awk, run->global.rs, 
				((run->global.ignorecase)? ASE_AWK_REX_IGNORECASE: 0),
				ASE_AWK_STR_BUF(buf), ASE_AWK_STR_LEN(buf), 
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
				ASE_AWK_ASSERT (run->awk,
					ASE_AWK_STR_BUF(buf) + ASE_AWK_STR_LEN(buf) ==
					match_ptr + match_len);

				ASE_AWK_STR_LEN(buf) -= match_len;
				break;
			}
		}

		if (ase_awk_str_ccat (buf, c) == (ase_size_t)-1)
		{
			run->errnum = ASE_AWK_ENOMEM;
			ret = -1;
			break;
		}

		/* TODO: handle different line terminator like \r\n */
		if (c == ASE_T('\n')) line_len = 0;
		else line_len = line_len + 1;
	}

	if (rs_ptr != ASE_NULL && 
	    rs->type != ASE_AWK_VAL_STR) ASE_AWK_FREE (run->awk, rs_ptr);
	ase_awk_refdownval (run, rs);

	/* increment NR */
	if (ret != -1 && ret != 0)
	{
		ase_awk_val_t* nr;
		ase_long_t lv;
		ase_real_t rv;

		nr = ase_awk_getglobal (run, ASE_AWK_GLOBAL_NR);
		ase_awk_refupval (run, nr);
		n = ase_awk_valtonum (run, nr, &lv, &rv);
		ase_awk_refdownval (run, nr);

		if (n == -1) ret = -1;
		else
		{
			if (n == 1) lv = (ase_long_t)rv;

			nr = ase_awk_makeintval (run, lv + 1);
			if (nr == ASE_NULL) 
			{
				run->errnum = ASE_AWK_ENOMEM;
				ret = -1;
			}
			else 
			{
				if (ase_awk_setglobal (
					run, ASE_AWK_GLOBAL_NR, nr) == -1) ret = -1;
			}
		}
	}

	return ret;
}

int ase_awk_writeextio_val (
	ase_awk_run_t* run, int out_type, 
	const ase_char_t* name, ase_awk_val_t* v)
{
	ase_char_t* str;
	ase_size_t len;
	int n;

	if (v->type == ASE_AWK_VAL_STR)
	{
		str = ((ase_awk_val_str_t*)v)->buf;
		len = ((ase_awk_val_str_t*)v)->len;
	}
	else
	{
		str = ase_awk_valtostr (
			run, v, 
			ASE_AWK_VALTOSTR_CLEAR | ASE_AWK_VALTOSTR_PRINT, 
			ASE_NULL, &len);
		if (str == ASE_NULL) return -1;
	}

	n = ase_awk_writeextio_str (run, out_type, name, str, len);

	if (v->type != ASE_AWK_VAL_STR) ASE_AWK_FREE (run->awk, str);
	return n;
}

int ase_awk_writeextio_str (
	ase_awk_run_t* run, int out_type, 
	const ase_char_t* name, ase_char_t* str, ase_size_t len)
{
	ase_awk_extio_t* p = run->extio.chain;
	ase_awk_io_t handler;
	int extio_type, extio_mode, extio_mask, n;

	ASE_AWK_ASSERT (run->awk, 
		out_type >= 0 && out_type <= ase_countof(__out_type_map));
	ASE_AWK_ASSERT (run->awk,
		out_type >= 0 && out_type <= ase_countof(__out_mode_map));
	ASE_AWK_ASSERT (run->awk,
		out_type >= 0 && out_type <= ase_countof(__out_mask_map));

	/* translate the out_type into the relevant extio type and mode */
	extio_type = __out_type_map[out_type];
	extio_mode = __out_mode_map[out_type];
	extio_mask = __out_mask_map[out_type];

	handler = run->extio.handler[extio_type];
	if (handler == ASE_NULL)
	{
		/* no io handler provided */
		run->errnum = ASE_AWK_EIOIMPL; /* TODO: change the error code */
		return -1;
	}

	/* look for the corresponding extio for name */
	while (p != ASE_NULL)
	{
		/* the file "1.tmp", in the following code snippets, 
		 * would be opened by the first print statement, but not by
		 * the second print statement. this is because
		 * both ASE_AWK_OUT_FILE and ASE_AWK_OUT_FILE_APPEND are
		 * translated to ASE_AWK_EXTIO_FILE and it is used to
		 * keep track of file handles..
		 *
		 *    print "1111" >> "1.tmp"
		 *    print "1111" > "1.tmp"
		 */
		if (p->type == (extio_type | extio_mask) && 
		    ase_awk_strcmp (p->name, name) == 0) break;
		p = p->next;
	}

	/* if there is not corresponding extio for name, create one */
	if (p == ASE_NULL)
	{
		p = (ase_awk_extio_t*) ASE_AWK_MALLOC (
			run->awk, ase_sizeof(ase_awk_extio_t));
		if (p == ASE_NULL)
		{
			run->errnum = ASE_AWK_ENOMEM;
			return -1;
		}

		p->name = ase_awk_strdup (run->awk, name);
		if (p->name == ASE_NULL)
		{
			ASE_AWK_FREE (run->awk, p);
			run->errnum = ASE_AWK_ENOMEM;
			return -1;
		}

		p->run = run;
		p->type = (extio_type | extio_mask);
		p->mode = extio_mode;
		p->handle = ASE_NULL;
		p->next = ASE_NULL;
		p->custom_data = run->extio.custom_data;

		p->out.eof = ase_false;
		p->out.eos = ase_false;

		n = handler (ASE_AWK_IO_OPEN, p, ASE_NULL, 0);
		if (n == -1)
		{
			ASE_AWK_FREE (run->awk, p->name);
			ASE_AWK_FREE (run->awk, p);
				
			/* TODO: use meaningful error code */
			if (ase_awk_setglobal (
				run, ASE_AWK_GLOBAL_ERRNO, 
				ase_awk_val_one) == -1) return -1;

			run->errnum = ASE_AWK_EIOHANDLER;
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
		if (n == 0) 
		{
			p->out.eos = ase_true;
			return 0;
		}
	}

	if (p->out.eos) 
	{
		/* no more streams */
		return 0;
	}

	if (p->out.eof) 
	{
		/* it has reached the end of the stream but this function
		 * has been recalled */
		return 0;
	}

	while (len > 0)
	{
		n = handler (ASE_AWK_IO_WRITE, p, str, len);

		if (n == -1) 
		{
			/* TODO: use meaningful error code */
			if (ase_awk_setglobal (
				run, ASE_AWK_GLOBAL_ERRNO, 
				ase_awk_val_one) == -1) return -1;

			run->errnum = ASE_AWK_EIOHANDLER;
			return -1;
		}

		if (n == 0) 
		{
			p->out.eof = ase_true;
			return 0;
		}

		len -= n;
		str += n;
	}

	return 1;
}

int ase_awk_flushextio (
	ase_awk_run_t* run, int out_type, const ase_char_t* name)
{
	ase_awk_extio_t* p = run->extio.chain;
	ase_awk_io_t handler;
	int extio_type, /*extio_mode,*/ extio_mask, n;
	ase_bool_t ok = ase_false;

	ASE_AWK_ASSERT (run->awk,
		out_type >= 0 && out_type <= ase_countof(__out_type_map));
	ASE_AWK_ASSERT (run->awk,
		out_type >= 0 && out_type <= ase_countof(__out_mode_map));
	ASE_AWK_ASSERT (run->awk,
		out_type >= 0 && out_type <= ase_countof(__out_mask_map));

	/* translate the out_type into the relevant extio type and mode */
	extio_type = __out_type_map[out_type];
	/*extio_mode = __out_mode_map[out_type];*/
	extio_mask = __out_mask_map[out_type];

	handler = run->extio.handler[extio_type];
	if (handler == ASE_NULL)
	{
		/* no io handler provided */
		run->errnum = ASE_AWK_EIOIMPL; /* TODO: change the error code */
		return -1;
	}

	/* look for the corresponding extio for name */
	while (p != ASE_NULL)
	{
		if (p->type == (extio_type | extio_mask) && 
		    (name == ASE_NULL || ase_awk_strcmp (p->name, name) == 0)) 
		{
			n = handler (ASE_AWK_IO_FLUSH, p, ASE_NULL, 0);

			if (n == -1) 
			{
				/* TODO: use meaningful error code */
				if (ase_awk_setglobal (
					run, ASE_AWK_GLOBAL_ERRNO, 
					ase_awk_val_one) == -1) return -1;

				run->errnum = ASE_AWK_EIOHANDLER;
				return -1;
			}

			ok = ase_true;
		}

		p = p->next;
	}

	if (ok) return 0;

	/* there is no corresponding extio for name */
	/* TODO: use meaningful error code. but is this needed? */
	if (ase_awk_setglobal (
		run, ASE_AWK_GLOBAL_ERRNO, ase_awk_val_one) == -1) return -1;

	run->errnum = ASE_AWK_ENOSUCHIO;
	return -1;
}

int ase_awk_nextextio_read (
	ase_awk_run_t* run, int in_type, const ase_char_t* name)
{
	ase_awk_extio_t* p = run->extio.chain;
	ase_awk_io_t handler;
	int extio_type, /*extio_mode,*/ extio_mask, n;

	ASE_AWK_ASSERT (run->awk,
		in_type >= 0 && in_type <= ase_countof(__in_type_map));
	ASE_AWK_ASSERT (run->awk,
		in_type >= 0 && in_type <= ase_countof(__in_mode_map));
	ASE_AWK_ASSERT (run->awk,
		in_type >= 0 && in_type <= ase_countof(__in_mask_map));

	/* translate the in_type into the relevant extio type and mode */
	extio_type = __in_type_map[in_type];
	/*extio_mode = __in_mode_map[in_type];*/
	extio_mask = __in_mask_map[in_type];

	handler = run->extio.handler[extio_type];
	if (handler == ASE_NULL)
	{
		/* no io handler provided */
		run->errnum = ASE_AWK_EIOIMPL; /* TODO: change the error code */
		return -1;
	}

	while (p != ASE_NULL)
	{
		if (p->type == (extio_type | extio_mask) &&
		    ase_awk_strcmp (p->name,name) == 0) break;
		p = p->next;
	}

	if (p == ASE_NULL)
	{
		/* something is totally wrong */
		run->errnum = ASE_AWK_EINTERNAL;
		return -1;
	}

	if (p->in.eos) 
	{
		/* no more streams. */
		return 0;
	}

	n = handler (ASE_AWK_IO_NEXT, p, ASE_NULL, 0);
	if (n == -1)
	{
		/* TODO: is this errnum correct? */
		run->errnum = ASE_AWK_EIOHANDLER;
		return -1;
	}

	if (n == 0) 
	{
		/* the next stream cannot be opened. 
		 * set the eos flags so that the next call to nextextio_read
		 * will return 0 without executing the handler */
		p->in.eos = ase_true;
	}
	else 
	{
		/* as the next stream has been opened successfully,
		 * the eof flag should be cleared if set */
		p->in.eof = ase_false;
	}

	return n;
}

int ase_awk_closeextio_read (
	ase_awk_run_t* run, int in_type, const ase_char_t* name)
{
	ase_awk_extio_t* p = run->extio.chain, * px = ASE_NULL;
	ase_awk_io_t handler;
	int extio_type, /*extio_mode,*/ extio_mask;

	ASE_AWK_ASSERT (run->awk,
		in_type >= 0 && in_type <= ase_countof(__in_type_map));
	ASE_AWK_ASSERT (run->awk,
		in_type >= 0 && in_type <= ase_countof(__in_mode_map));
	ASE_AWK_ASSERT (run->awk,
		in_type >= 0 && in_type <= ase_countof(__in_mask_map));

	/* translate the in_type into the relevant extio type and mode */
	extio_type = __in_type_map[in_type];
	/*extio_mode = __in_mode_map[in_type];*/
	extio_mask = __in_mask_map[in_type];

	handler = run->extio.handler[extio_type];
	if (handler == ASE_NULL)
	{
		/* no io handler provided */
		run->errnum = ASE_AWK_EIOIMPL; /* TODO: change the error code */
		return -1;
	}

	while (p != ASE_NULL)
	{
		if (p->type == (extio_type | extio_mask) &&
		    ase_awk_strcmp (p->name, name) == 0) 
		{
			ase_awk_io_t handler;
		       
			handler = run->extio.handler[p->type & __MASK_CLEAR];
			if (handler != ASE_NULL)
			{
				if (handler (ASE_AWK_IO_CLOSE, p, ASE_NULL, 0) == -1)
				{
					/* this is not a run-time error.*/
					/* TODO: set ERRNO */
					run->errnum = ASE_AWK_EIOHANDLER;
					return -1;
				}
			}

			if (px != ASE_NULL) px->next = p->next;
			else run->extio.chain = p->next;

			ASE_AWK_FREE (run->awk, p->name);
			ASE_AWK_FREE (run->awk, p);
			return 0;
		}

		px = p;
		p = p->next;
	}

	/* this is not a run-time error */
	run->errnum = ASE_AWK_EIOHANDLER;
	return -1;
}

int ase_awk_closeextio_write (
	ase_awk_run_t* run, int out_type, const ase_char_t* name)
{
	ase_awk_extio_t* p = run->extio.chain, * px = ASE_NULL;
	ase_awk_io_t handler;
	int extio_type, /*extio_mode,*/ extio_mask;

	ASE_AWK_ASSERT (run->awk,
		out_type >= 0 && out_type <= ase_countof(__out_type_map));
	ASE_AWK_ASSERT (run->awk,
		out_type >= 0 && out_type <= ase_countof(__out_mode_map));
	ASE_AWK_ASSERT (run->awk,
		out_type >= 0 && out_type <= ase_countof(__out_mask_map));

	/* translate the out_type into the relevant extio type and mode */
	extio_type = __out_type_map[out_type];
	/*extio_mode = __out_mode_map[out_type];*/
	extio_mask = __out_mask_map[out_type];

	handler = run->extio.handler[extio_type];
	if (handler == ASE_NULL)
	{
		/* no io handler provided */
		run->errnum = ASE_AWK_EIOIMPL; /* TODO: change the error code */
		return -1;
	}

	while (p != ASE_NULL)
	{
		if (p->type == (extio_type | extio_mask) &&
		    ase_awk_strcmp (p->name, name) == 0) 
		{
			ase_awk_io_t handler;
		       
			handler = run->extio.handler[p->type & __MASK_CLEAR];
			if (handler != ASE_NULL)
			{
				if (handler (ASE_AWK_IO_CLOSE, p, ASE_NULL, 0) == -1)
				{
					/* this is not a run-time error.*/
					/* TODO: set ERRNO */
					run->errnum = ASE_AWK_EIOHANDLER;
					return -1;
				}
			}

			if (px != ASE_NULL) px->next = p->next;
			else run->extio.chain = p->next;

			ASE_AWK_FREE (run->awk, p->name);
			ASE_AWK_FREE (run->awk, p);
			return 0;
		}

		px = p;
		p = p->next;
	}

	/* this is not a run-time error */
	/* TODO: set ERRNO */
	run->errnum = ASE_AWK_EIOHANDLER;
	return -1;
}

int ase_awk_closeextio (ase_awk_run_t* run, const ase_char_t* name)
{
	ase_awk_extio_t* p = run->extio.chain, * px = ASE_NULL;

	while (p != ASE_NULL)
	{
		 /* it handles the first that matches the given name
		  * regardless of the extio type */
		if (ase_awk_strcmp (p->name, name) == 0) 
		{
			ase_awk_io_t handler;
		       
			handler = run->extio.handler[p->type & __MASK_CLEAR];
			if (handler != ASE_NULL)
			{
				if (handler (ASE_AWK_IO_CLOSE, p, ASE_NULL, 0) == -1)
				{
					/* this is not a run-time error.*/
					/* TODO: set ERRNO */
					run->errnum = ASE_AWK_EIOHANDLER;
					return -1;
				}
			}

			if (px != ASE_NULL) px->next = p->next;
			else run->extio.chain = p->next;

			ASE_AWK_FREE (run->awk, p->name);
			ASE_AWK_FREE (run->awk, p);
			return 0;
		}

		px = p;
		p = p->next;
	}

	/* this is not a run-time error */
	/* TODO: set ERRNO */
	run->errnum = ASE_AWK_EIOHANDLER;
	return -1;
}

void ase_awk_clearextio (ase_awk_run_t* run)
{
	ase_awk_extio_t* next;
	ase_awk_io_t handler;
	int n;

	while (run->extio.chain != ASE_NULL)
	{
		handler = run->extio.handler[
			run->extio.chain->type & __MASK_CLEAR];
		next = run->extio.chain->next;

		if (handler != ASE_NULL)
		{
			n = handler (ASE_AWK_IO_CLOSE, run->extio.chain, ASE_NULL, 0);
			if (n == -1)
			{
				/* TODO: 
				 * some warning actions need to be taken */
			}
		}

		ASE_AWK_FREE (run->awk, run->extio.chain->name);
		ASE_AWK_FREE (run->awk, run->extio.chain);

		run->extio.chain = next;
	}
}
