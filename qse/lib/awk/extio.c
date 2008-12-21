/*
 * $Id: extio.c 466 2008-12-09 09:50:40Z baconevi $
 *
 * {License}
 */

#include "awk.h"

enum
{
	MASK_READ  = 0x0100,
	MASK_WRITE = 0x0200,
	MASK_RDWR  = 0x0400,

	MASK_CLEAR = 0x00FF
};

static int in_type_map[] =
{
	/* the order should match the order of the 
	 * QSE_AWK_IN_XXX values in tree.h */
	QSE_AWK_EXTIO_PIPE,
	QSE_AWK_EXTIO_COPROC,
	QSE_AWK_EXTIO_FILE,
	QSE_AWK_EXTIO_CONSOLE
};

static int in_mode_map[] =
{
	/* the order should match the order of the 
	 * QSE_AWK_IN_XXX values in tree.h */
	QSE_AWK_EXTIO_PIPE_READ,
	0,
	QSE_AWK_EXTIO_FILE_READ,
	QSE_AWK_EXTIO_CONSOLE_READ
};

static int in_mask_map[] =
{
	MASK_READ,
	MASK_RDWR,
	MASK_READ,
	MASK_READ
};

static int out_type_map[] =
{
	/* the order should match the order of the 
	 * QSE_AWK_OUT_XXX values in tree.h */
	QSE_AWK_EXTIO_PIPE,
	QSE_AWK_EXTIO_COPROC,
	QSE_AWK_EXTIO_FILE,
	QSE_AWK_EXTIO_FILE,
	QSE_AWK_EXTIO_CONSOLE
};

static int out_mode_map[] =
{
	/* the order should match the order of the 
	 * QSE_AWK_OUT_XXX values in tree.h */
	QSE_AWK_EXTIO_PIPE_WRITE,
	0,
	QSE_AWK_EXTIO_FILE_WRITE,
	QSE_AWK_EXTIO_FILE_APPEND,
	QSE_AWK_EXTIO_CONSOLE_WRITE
};

static int out_mask_map[] =
{
	MASK_WRITE,
	MASK_RDWR,
	MASK_WRITE,
	MASK_WRITE,
	MASK_WRITE
};

int qse_awk_readextio (
	qse_awk_run_t* run, int in_type,
	const qse_char_t* name, qse_str_t* buf)
{
	qse_awk_extio_t* p = run->extio.chain;
	qse_awk_io_t handler;
	int extio_type, extio_mode, extio_mask, ret, n;
	qse_ssize_t x;
	qse_awk_val_t* rs;
	qse_char_t* rs_ptr;
	qse_size_t rs_len;
	qse_size_t line_len = 0;
	qse_char_t c = QSE_T('\0'), pc;

	QSE_ASSERT (in_type >= 0 && in_type <= QSE_COUNTOF(in_type_map));
	QSE_ASSERT (in_type >= 0 && in_type <= QSE_COUNTOF(in_mode_map));
	QSE_ASSERT (in_type >= 0 && in_type <= QSE_COUNTOF(in_mask_map));

	/* translate the in_type into the relevant extio type and mode */
	extio_type = in_type_map[in_type];
	extio_mode = in_mode_map[in_type];
	extio_mask = in_mask_map[in_type];

	handler = run->extio.handler[extio_type];
	if (handler == QSE_NULL)
	{
		/* no io handler provided */
		qse_awk_setrunerrnum (run, QSE_AWK_EIOUSER);
		return -1;
	}

	while (p != QSE_NULL)
	{
		if (p->type == (extio_type | extio_mask) &&
		    qse_strcmp (p->name,name) == 0) break;
		p = p->next;
	}

	if (p == QSE_NULL)
	{
		p = (qse_awk_extio_t*) QSE_AWK_ALLOC (
			run->awk, QSE_SIZEOF(qse_awk_extio_t));
		if (p == QSE_NULL)
		{
			qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);
			return -1;
		}

		p->name = QSE_AWK_STRDUP (run->awk, name);
		if (p->name == QSE_NULL)
		{
			QSE_AWK_FREE (run->awk, p);
			qse_awk_setrunerrnum (run, QSE_AWK_ENOMEM);
			return -1;
		}

		p->run = run;
		p->type = (extio_type | extio_mask);
		p->mode = extio_mode;
		p->handle = QSE_NULL;
		p->next = QSE_NULL;
		p->data = run->extio.data;

		p->in.buf[0] = QSE_T('\0');
		p->in.pos = 0;
		p->in.len = 0;
		p->in.eof = QSE_FALSE;
		p->in.eos = QSE_FALSE;

		qse_awk_setrunerrnum (run, QSE_AWK_ENOERR);

		x = handler (QSE_AWK_IO_OPEN, p, QSE_NULL, 0);
		if (x <= -1)
		{
			QSE_AWK_FREE (run->awk, p->name);
			QSE_AWK_FREE (run->awk, p);

			if (run->errnum == QSE_AWK_ENOERR)
			{
				/* if the error number has not been 
				 * set by the user handler */
				qse_awk_setrunerrnum (run, QSE_AWK_EIOIMPL);
			}

			return -1;
		}

		/* chain it */
		p->next = run->extio.chain;
		run->extio.chain = p;

		/* usually, x == 0 indicates that it has reached the end 
		 * of the input. the user io handler can return 0 for the 
		 * open request if it doesn't have any files to open. One 
		 * advantage of doing this would be that you can skip the 
		 * entire pattern-block matching and exeuction. */
		if (x == 0) 
		{
			p->in.eos = QSE_TRUE;
			return 0;
		}
	}

	if (p->in.eos) 
	{
		/* no more streams. */
		return 0;
	}

	/* ready to read a line */
	qse_str_clear (buf);

	/* get the record separator */
	rs = qse_awk_getglobal (run, QSE_AWK_GLOBAL_RS);
	qse_awk_refupval (run, rs);

	if (rs->type == QSE_AWK_VAL_NIL)
	{
		rs_ptr = QSE_NULL;
		rs_len = 0;
	}
	else if (rs->type == QSE_AWK_VAL_STR)
	{
		rs_ptr = ((qse_awk_val_str_t*)rs)->buf;
		rs_len = ((qse_awk_val_str_t*)rs)->len;
	}
	else 
	{
		rs_ptr = qse_awk_valtostr (
			run, rs, QSE_AWK_VALTOSTR_CLEAR, QSE_NULL, &rs_len);
		if (rs_ptr == QSE_NULL)
		{
			qse_awk_refdownval (run, rs);
			return -1;
		}
	}

	ret = 1;

	/* call the io handler */
	while (1)
	{
		if (p->in.pos >= p->in.len)
		{
			qse_ssize_t n;

			if (p->in.eof)
			{
				if (QSE_STR_LEN(buf) == 0) ret = 0;
				break;
			}

			qse_awk_setrunerrnum (run, QSE_AWK_ENOERR);

			n = handler (QSE_AWK_IO_READ, 
				p, p->in.buf, QSE_COUNTOF(p->in.buf));
			if (n <= -1) 
			{
				if (run->errnum == QSE_AWK_ENOERR)
				{
					/* if the error number has not been 
				 	 * set by the user handler */
					qse_awk_setrunerrnum (run, QSE_AWK_EIOIMPL);
				}

				ret = -1;
				break;
			}

			if (n == 0) 
			{
				p->in.eof = QSE_TRUE;

				if (QSE_STR_LEN(buf) == 0) ret = 0;
				else if (rs_len >= 2)
				{
					/* when RS is multiple characters, it needs to check
					 * for the match at the end of the input stream as
					 * the buffer has been appened with the last character
					 * after the previous matchrex has failed */

					const qse_char_t* match_ptr;
					qse_size_t match_len;

					QSE_ASSERT (run->global.rs != QSE_NULL);

					n = QSE_AWK_MATCHREX (
						run->awk, run->global.rs, 
						((run->global.ignorecase)? QSE_REX_IGNORECASE: 0),
						QSE_STR_PTR(buf), QSE_STR_LEN(buf), 
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
						QSE_ASSERT (
							QSE_STR_PTR(buf) + QSE_STR_LEN(buf) ==
							match_ptr + match_len);

						QSE_STR_LEN(buf) -= match_len;
						break;
					}
				}

				break;
			}

			p->in.len = n;
			p->in.pos = 0;
		}

		pc = c;
		c = p->in.buf[p->in.pos++];

		if (rs_ptr == QSE_NULL)
		{
			/* separate by a new line */
			if (c == QSE_T('\n')) 
			{
				if (pc == QSE_T('\r') && 
				    QSE_STR_LEN(buf) > 0) 
				{
					QSE_STR_LEN(buf) -= 1;
				}
				break;
			}
		}
		else if (rs_len == 0)
		{
			/* separate by a blank line */
			if (c == QSE_T('\n'))
			{
				if (pc == QSE_T('\r') && 
				    QSE_STR_LEN(buf) > 0) 
				{
					QSE_STR_LEN(buf) -= 1;
				}
			}

			if (line_len == 0 && c == QSE_T('\n'))
			{
				if (QSE_STR_LEN(buf) <= 0) 
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
				QSE_STR_LEN(buf) -= 1;
				break;
			}
		}
		else if (rs_len == 1)
		{
			if (c == rs_ptr[0]) break;
		}
		else
		{
			const qse_char_t* match_ptr;
			qse_size_t match_len;

			QSE_ASSERT (run->global.rs != QSE_NULL);

			n = QSE_AWK_MATCHREX (
				run->awk, run->global.rs, 
				((run->global.ignorecase)? QSE_REX_IGNORECASE: 0),
				QSE_STR_PTR(buf), QSE_STR_LEN(buf), 
				&match_ptr, &match_len, &run->errnum);
			if (n == -1)
			{
				ret = -1;
				p->in.pos--; /* unread the character in c */
				break;
			}

			if (n == 1)
			{
				/* the match should be found at the end of
				 * the current buffer */
				QSE_ASSERT (
					QSE_STR_PTR(buf) + QSE_STR_LEN(buf) ==
					match_ptr + match_len);

				QSE_STR_LEN(buf) -= match_len;
				p->in.pos--; /* unread the character in c */
				break;
			}
		}

		if (qse_str_ccat (buf, c) == (qse_size_t)-1)
		{
			qse_awk_setrunerror (
				run, QSE_AWK_ENOMEM, 0, QSE_NULL, 0);
			ret = -1;
			break;
		}

		/* TODO: handle different line terminator like \r\n */
		if (c == QSE_T('\n')) line_len = 0;
		else line_len = line_len + 1;
	}

	if (rs_ptr != QSE_NULL && 
	    rs->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, rs_ptr);
	qse_awk_refdownval (run, rs);

	return ret;
}

#include <qse/utl/stdio.h>
int qse_awk_writeextio_val (
	qse_awk_run_t* run, int out_type, 
	const qse_char_t* name, qse_awk_val_t* v)
{
	qse_char_t* str;
	qse_size_t len;
	int n;

	if (v->type == QSE_AWK_VAL_STR)
	{
		str = ((qse_awk_val_str_t*)v)->buf;
		len = ((qse_awk_val_str_t*)v)->len;
	}
	else
	{
		str = qse_awk_valtostr (
			run, v, 
			QSE_AWK_VALTOSTR_CLEAR | QSE_AWK_VALTOSTR_PRINT, 
			QSE_NULL, &len);
		if (str == QSE_NULL) return -1;
	}

	n = qse_awk_writeextio_str (run, out_type, name, str, len);

	if (v->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, str);
	return n;
}

int qse_awk_writeextio_str (
	qse_awk_run_t* run, int out_type, 
	const qse_char_t* name, qse_char_t* str, qse_size_t len)
{
	qse_awk_extio_t* p = run->extio.chain;
	qse_awk_io_t handler;
	int extio_type, extio_mode, extio_mask; 
	qse_ssize_t n;

	QSE_ASSERT (out_type >= 0 && out_type <= QSE_COUNTOF(out_type_map));
	QSE_ASSERT (out_type >= 0 && out_type <= QSE_COUNTOF(out_mode_map));
	QSE_ASSERT (out_type >= 0 && out_type <= QSE_COUNTOF(out_mask_map));

	/* translate the out_type into the relevant extio type and mode */
	extio_type = out_type_map[out_type];
	extio_mode = out_mode_map[out_type];
	extio_mask = out_mask_map[out_type];

	handler = run->extio.handler[extio_type];
	if (handler == QSE_NULL)
	{
		/* no io handler provided */
		qse_awk_setrunerrnum (run, QSE_AWK_EIOUSER);
		return -1;
	}

	/* look for the corresponding extio for name */
	while (p != QSE_NULL)
	{
		/* the file "1.tmp", in the following code snippets, 
		 * would be opened by the first print statement, but not by
		 * the second print statement. this is because
		 * both QSE_AWK_OUT_FILE and QSE_AWK_OUT_FILE_APPEND are
		 * translated to QSE_AWK_EXTIO_FILE and it is used to
		 * keep track of file handles..
		 *
		 *    print "1111" >> "1.tmp"
		 *    print "1111" > "1.tmp"
		 */
		if (p->type == (extio_type | extio_mask) && 
		    qse_strcmp (p->name, name) == 0) break;
		p = p->next;
	}

	/* if there is not corresponding extio for name, create one */
	if (p == QSE_NULL)
	{
		p = (qse_awk_extio_t*) QSE_AWK_ALLOC (
			run->awk, QSE_SIZEOF(qse_awk_extio_t));
		if (p == QSE_NULL)
		{
			qse_awk_setrunerror (
				run, QSE_AWK_ENOMEM, 0, QSE_NULL, 0);
			return -1;
		}

		p->name = QSE_AWK_STRDUP (run->awk, name);
		if (p->name == QSE_NULL)
		{
			QSE_AWK_FREE (run->awk, p);
			qse_awk_setrunerror (
				run, QSE_AWK_ENOMEM, 0, QSE_NULL, 0);
			return -1;
		}

		p->run = run;
		p->type = (extio_type | extio_mask);
		p->mode = extio_mode;
		p->handle = QSE_NULL;
		p->next = QSE_NULL;
		p->data = run->extio.data;

		p->out.eof = QSE_FALSE;
		p->out.eos = QSE_FALSE;

		qse_awk_setrunerrnum (run, QSE_AWK_ENOERR);
		n = handler (QSE_AWK_IO_OPEN, p, QSE_NULL, 0);
		if (n <= -1)
		{
			QSE_AWK_FREE (run->awk, p->name);
			QSE_AWK_FREE (run->awk, p);

			if (run->errnum == QSE_AWK_ENOERR)
				qse_awk_setrunerrnum (run, QSE_AWK_EIOIMPL);

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
			p->out.eos = QSE_TRUE;
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
		qse_awk_setrunerrnum (run, QSE_AWK_ENOERR);
		n = handler (QSE_AWK_IO_WRITE, p, str, len);
		if (n <= -1) 
		{
			if (run->errnum == QSE_AWK_ENOERR)
				qse_awk_setrunerrnum (run, QSE_AWK_EIOIMPL);

			return -1;
		}

		if (n == 0) 
		{
			p->out.eof = QSE_TRUE;
			return 0;
		}

		len -= n;
		str += n;
	}

	return 1;
}

int qse_awk_flushextio (
	qse_awk_run_t* run, int out_type, const qse_char_t* name)
{
	qse_awk_extio_t* p = run->extio.chain;
	qse_awk_io_t handler;
	int extio_type, /*extio_mode,*/ extio_mask;
	qse_ssize_t n;
	qse_bool_t ok = QSE_FALSE;

	QSE_ASSERT (out_type >= 0 && out_type <= QSE_COUNTOF(out_type_map));
	QSE_ASSERT (out_type >= 0 && out_type <= QSE_COUNTOF(out_mode_map));
	QSE_ASSERT (out_type >= 0 && out_type <= QSE_COUNTOF(out_mask_map));

	/* translate the out_type into the relevant extio type and mode */
	extio_type = out_type_map[out_type];
	/*extio_mode = out_mode_map[out_type];*/
	extio_mask = out_mask_map[out_type];

	handler = run->extio.handler[extio_type];
	if (handler == QSE_NULL)
	{
		/* no io handler provided */
		qse_awk_setrunerrnum (run, QSE_AWK_EIOUSER);
		return -1;
	}

	/* look for the corresponding extio for name */
	while (p != QSE_NULL)
	{
		if (p->type == (extio_type | extio_mask) && 
		    (name == QSE_NULL || qse_strcmp(p->name,name) == 0)) 
		{
			qse_awk_setrunerrnum (run, QSE_AWK_ENOERR);
			n = handler (QSE_AWK_IO_FLUSH, p, QSE_NULL, 0);

			if (n <= -1) 
			{
				if (run->errnum == QSE_AWK_ENOERR)
					qse_awk_setrunerrnum (run, QSE_AWK_EIOIMPL);
				return -1;
			}

			ok = QSE_TRUE;
		}

		p = p->next;
	}

	if (ok) return 0;

	/* there is no corresponding extio for name */
	qse_awk_setrunerrnum (run, QSE_AWK_EIONONE);
	return -1;
}

int qse_awk_nextextio_read (
	qse_awk_run_t* run, int in_type, const qse_char_t* name)
{
	qse_awk_extio_t* p = run->extio.chain;
	qse_awk_io_t handler;
	int extio_type, /*extio_mode,*/ extio_mask; 
	qse_ssize_t n;

	QSE_ASSERT (in_type >= 0 && in_type <= QSE_COUNTOF(in_type_map));
	QSE_ASSERT (in_type >= 0 && in_type <= QSE_COUNTOF(in_mode_map));
	QSE_ASSERT (in_type >= 0 && in_type <= QSE_COUNTOF(in_mask_map));

	/* translate the in_type into the relevant extio type and mode */
	extio_type = in_type_map[in_type];
	/*extio_mode = in_mode_map[in_type];*/
	extio_mask = in_mask_map[in_type];

	handler = run->extio.handler[extio_type];
	if (handler == QSE_NULL)
	{
		/* no io handler provided */
		qse_awk_setrunerrnum (run, QSE_AWK_EIOUSER);
		return -1;
	}

	while (p != QSE_NULL)
	{
		if (p->type == (extio_type | extio_mask) &&
		    qse_strcmp (p->name,name) == 0) break;
		p = p->next;
	}

	if (p == QSE_NULL)
	{
		/* something is totally wrong */
		QSE_ASSERT (
			!"should never happen - cannot find the relevant extio entry");
		qse_awk_setrunerror (run, QSE_AWK_EINTERN, 0, QSE_NULL, 0);
		return -1;
	}

	if (p->in.eos) 
	{
		/* no more streams. */
		return 0;
	}

	qse_awk_setrunerrnum (run, QSE_AWK_ENOERR);
	n = handler (QSE_AWK_IO_NEXT, p, QSE_NULL, 0);
	if (n <= -1)
	{
		if (run->errnum == QSE_AWK_ENOERR)
			qse_awk_setrunerrnum (run, QSE_AWK_EIOIMPL);
		return -1;
	}

	if (n == 0) 
	{
		/* the next stream cannot be opened. 
		 * set the eos flags so that the next call to nextextio_read
		 * will return 0 without executing the handler */
		p->in.eos = QSE_TRUE;
		return 0;
	}
	else 
	{
		/* as the next stream has been opened successfully,
		 * the eof flag should be cleared if set */
		p->in.eof = QSE_FALSE;

		/* also the previous input buffer must be reset */
		p->in.pos = 0;
		p->in.len = 0;

		return 1;
	}
}

int qse_awk_nextextio_write (
	qse_awk_run_t* run, int out_type, const qse_char_t* name)
{
	qse_awk_extio_t* p = run->extio.chain;
	qse_awk_io_t handler;
	int extio_type, /*extio_mode,*/ extio_mask; 
	qse_ssize_t n;

	QSE_ASSERT (out_type >= 0 && out_type <= QSE_COUNTOF(out_type_map));
	QSE_ASSERT (out_type >= 0 && out_type <= QSE_COUNTOF(out_mode_map));
	QSE_ASSERT (out_type >= 0 && out_type <= QSE_COUNTOF(out_mask_map));

	/* translate the out_type into the relevant extio type and mode */
	extio_type = out_type_map[out_type];
	/*extio_mode = out_mode_map[out_type];*/
	extio_mask = out_mask_map[out_type];

	handler = run->extio.handler[extio_type];
	if (handler == QSE_NULL)
	{
		/* no io handler provided */
		qse_awk_setrunerrnum (run, QSE_AWK_EIOUSER);
		return -1;
	}

	while (p != QSE_NULL)
	{
		if (p->type == (extio_type | extio_mask) &&
		    qse_strcmp (p->name,name) == 0) break;
		p = p->next;
	}

	if (p == QSE_NULL)
	{
		/* something is totally wrong */
		QSE_ASSERT (!"should never happen - cannot find the relevant extio entry");

		qse_awk_setrunerror (run, QSE_AWK_EINTERN, 0, QSE_NULL, 0);
		return -1;
	}

	if (p->out.eos) 
	{
		/* no more streams. */
		return 0;
	}

	qse_awk_setrunerrnum (run, QSE_AWK_ENOERR);
	n = handler (QSE_AWK_IO_NEXT, p, QSE_NULL, 0);
	if (n <= -1)
	{
		if (run->errnum == QSE_AWK_ENOERR)
			qse_awk_setrunerrnum (run, QSE_AWK_EIOIMPL);
		return -1;
	}

	if (n == 0) 
	{
		/* the next stream cannot be opened. 
		 * set the eos flags so that the next call to nextextio_write
		 * will return 0 without executing the handler */
		p->out.eos = QSE_TRUE;
		return 0;
	}
	else 
	{
		/* as the next stream has been opened successfully,
		 * the eof flag should be cleared if set */
		p->out.eof = QSE_FALSE;
		return 1;
	}
}

int qse_awk_closeextio_read (
	qse_awk_run_t* run, int in_type, const qse_char_t* name)
{
	qse_awk_extio_t* p = run->extio.chain, * px = QSE_NULL;
	qse_awk_io_t handler;
	int extio_type, /*extio_mode,*/ extio_mask;

	QSE_ASSERT (in_type >= 0 && in_type <= QSE_COUNTOF(in_type_map));
	QSE_ASSERT (in_type >= 0 && in_type <= QSE_COUNTOF(in_mode_map));
	QSE_ASSERT (in_type >= 0 && in_type <= QSE_COUNTOF(in_mask_map));

	/* translate the in_type into the relevant extio type and mode */
	extio_type = in_type_map[in_type];
	/*extio_mode = in_mode_map[in_type];*/
	extio_mask = in_mask_map[in_type];

	handler = run->extio.handler[extio_type];
	if (handler == QSE_NULL)
	{
		/* no io handler provided */
		qse_awk_setrunerrnum (run, QSE_AWK_EIOUSER);
		return -1;
	}

	while (p != QSE_NULL)
	{
		if (p->type == (extio_type | extio_mask) &&
		    qse_strcmp (p->name, name) == 0) 
		{
			qse_awk_io_t handler;
		       
			handler = run->extio.handler[p->type & MASK_CLEAR];
			if (handler != QSE_NULL)
			{
				if (handler (QSE_AWK_IO_CLOSE, p, QSE_NULL, 0) <= -1)
				{
					/* this is not a run-time error.*/
					qse_awk_setrunerror (run, QSE_AWK_EIOIMPL, 0, QSE_NULL, 0);
					return -1;
				}
			}

			if (px != QSE_NULL) px->next = p->next;
			else run->extio.chain = p->next;

			QSE_AWK_FREE (run->awk, p->name);
			QSE_AWK_FREE (run->awk, p);
			return 0;
		}

		px = p;
		p = p->next;
	}

	/* the name given is not found */
	qse_awk_setrunerrnum (run, QSE_AWK_EIONONE);
	return -1;
}

int qse_awk_closeextio_write (
	qse_awk_run_t* run, int out_type, const qse_char_t* name)
{
	qse_awk_extio_t* p = run->extio.chain, * px = QSE_NULL;
	qse_awk_io_t handler;
	int extio_type, /*extio_mode,*/ extio_mask;

	QSE_ASSERT (out_type >= 0 && out_type <= QSE_COUNTOF(out_type_map));
	QSE_ASSERT (out_type >= 0 && out_type <= QSE_COUNTOF(out_mode_map));
	QSE_ASSERT (out_type >= 0 && out_type <= QSE_COUNTOF(out_mask_map));

	/* translate the out_type into the relevant extio type and mode */
	extio_type = out_type_map[out_type];
	/*extio_mode = out_mode_map[out_type];*/
	extio_mask = out_mask_map[out_type];

	handler = run->extio.handler[extio_type];
	if (handler == QSE_NULL)
	{
		/* no io handler provided */
		qse_awk_setrunerror (run, QSE_AWK_EIOUSER, 0, QSE_NULL, 0);
		return -1;
	}

	while (p != QSE_NULL)
	{
		if (p->type == (extio_type | extio_mask) &&
		    qse_strcmp (p->name, name) == 0) 
		{
			qse_awk_io_t handler;
		       
			handler = run->extio.handler[p->type & MASK_CLEAR];
			if (handler != QSE_NULL)
			{
				qse_awk_setrunerrnum (run, QSE_AWK_ENOERR);
				if (handler (QSE_AWK_IO_CLOSE, p, QSE_NULL, 0) <= -1)
				{
					if (run->errnum == QSE_AWK_ENOERR)
						qse_awk_setrunerrnum (run, QSE_AWK_EIOIMPL);
					return -1;
				}
			}

			if (px != QSE_NULL) px->next = p->next;
			else run->extio.chain = p->next;

			QSE_AWK_FREE (run->awk, p->name);
			QSE_AWK_FREE (run->awk, p);
			return 0;
		}

		px = p;
		p = p->next;
	}

	qse_awk_setrunerrnum (run, QSE_AWK_EIONONE);
	return -1;
}

int qse_awk_closeextio (qse_awk_run_t* run, const qse_char_t* name)
{
	qse_awk_extio_t* p = run->extio.chain, * px = QSE_NULL;

	while (p != QSE_NULL)
	{
		 /* it handles the first that matches the given name
		  * regardless of the extio type */
		if (qse_strcmp (p->name, name) == 0) 
		{
			qse_awk_io_t handler;
		       
			handler = run->extio.handler[p->type & MASK_CLEAR];
			if (handler != QSE_NULL)
			{
				qse_awk_setrunerrnum (run, QSE_AWK_ENOERR);
				if (handler (QSE_AWK_IO_CLOSE, p, QSE_NULL, 0) <= -1)
				{
					/* this is not a run-time error.*/
					if (run->errnum == QSE_AWK_ENOERR)
						qse_awk_setrunerrnum (run, QSE_AWK_EIOIMPL);
					return -1;
				}
			}

			if (px != QSE_NULL) px->next = p->next;
			else run->extio.chain = p->next;

			QSE_AWK_FREE (run->awk, p->name);
			QSE_AWK_FREE (run->awk, p);

			return 0;
		}

		px = p;
		p = p->next;
	}

	qse_awk_setrunerrnum (run, QSE_AWK_EIONONE);
	return -1;
}

void qse_awk_clearextio (qse_awk_run_t* run)
{
	qse_awk_extio_t* next;
	qse_awk_io_t handler;
	qse_ssize_t n;

	while (run->extio.chain != QSE_NULL)
	{
		handler = run->extio.handler[
			run->extio.chain->type & MASK_CLEAR];
		next = run->extio.chain->next;

		if (handler != QSE_NULL)
		{
			qse_awk_setrunerrnum (run, QSE_AWK_ENOERR);
			n = handler (QSE_AWK_IO_CLOSE, run->extio.chain, QSE_NULL, 0);
			if (n <= -1)
			{
				if (run->errnum == QSE_AWK_ENOERR)
					qse_awk_setrunerrnum (run, QSE_AWK_EIOIMPL);
				/* TODO: some warnings need to be shown??? */
			}
		}

		QSE_AWK_FREE (run->awk, run->extio.chain->name);
		QSE_AWK_FREE (run->awk, run->extio.chain);

		run->extio.chain = next;
	}
}
