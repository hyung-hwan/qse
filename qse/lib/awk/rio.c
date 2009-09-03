/*
 * $Id: rio.c 277 2009-09-02 12:55:55Z hyunghwan.chung $
 *
   Copyright 2006-2009 Chung, Hyung-Hwan.

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
	QSE_AWK_RIO_PIPE,
	QSE_AWK_RIO_PIPE,
	QSE_AWK_RIO_FILE,
	QSE_AWK_RIO_CONSOLE
};

static int in_mode_map[] =
{
	/* the order should match the order of the 
	 * QSE_AWK_IN_XXX values in tree.h */
	QSE_AWK_RIO_PIPE_READ,
	QSE_AWK_RIO_PIPE_RW,
	QSE_AWK_RIO_FILE_READ,
	QSE_AWK_RIO_CONSOLE_READ
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
	QSE_AWK_RIO_PIPE,
	QSE_AWK_RIO_PIPE,
	QSE_AWK_RIO_FILE,
	QSE_AWK_RIO_FILE,
	QSE_AWK_RIO_CONSOLE
};

static int out_mode_map[] =
{
	/* the order should match the order of the 
	 * QSE_AWK_OUT_XXX values in tree.h */
	QSE_AWK_RIO_PIPE_WRITE,
	QSE_AWK_RIO_PIPE_RW,
	QSE_AWK_RIO_FILE_WRITE,
	QSE_AWK_RIO_FILE_APPEND,
	QSE_AWK_RIO_CONSOLE_WRITE
};

static int out_mask_map[] =
{
	MASK_WRITE,
	MASK_RDWR,
	MASK_WRITE,
	MASK_WRITE,
	MASK_WRITE
};

int qse_awk_rtx_readio (
	qse_awk_rtx_t* run, int in_type,
	const qse_char_t* name, qse_str_t* buf)
{
	qse_awk_rio_arg_t* p = run->rio.chain;
	qse_awk_rio_fun_t handler;
	int io_type, io_mode, io_mask, ret, n;
	qse_ssize_t x;
	qse_awk_val_t* rs;
	qse_char_t* rs_ptr;
	qse_size_t rs_len;
	qse_size_t line_len = 0;
	qse_char_t c = QSE_T('\0'), pc;

	QSE_ASSERT (in_type >= 0 && in_type <= QSE_COUNTOF(in_type_map));
	QSE_ASSERT (in_type >= 0 && in_type <= QSE_COUNTOF(in_mode_map));
	QSE_ASSERT (in_type >= 0 && in_type <= QSE_COUNTOF(in_mask_map));

	/* translate the in_type into the relevant io type and mode */
	io_type = in_type_map[in_type];
	io_mode = in_mode_map[in_type];
	io_mask = in_mask_map[in_type];

	/* get the io handler provided by a user */
	handler = run->rio.handler[io_type];
	if (handler == QSE_NULL)
	{
		/* no io handler provided */
		qse_awk_rtx_seterrnum (run, QSE_AWK_EIOUSER, QSE_NULL);
		return -1;
	}

	/* search the chain for exiting an existing io name */
	while (p != QSE_NULL)
	{
		if (p->type == (io_type | io_mask) &&
		    qse_strcmp (p->name,name) == 0) break;
		p = p->next;
	}

	if (p == QSE_NULL)
	{
		/* if the name doesn't exist in the chain, create an entry
		 * to the chain */
		p = (qse_awk_rio_arg_t*) QSE_AWK_ALLOC (
			run->awk, QSE_SIZEOF(qse_awk_rio_arg_t));
		if (p == QSE_NULL)
		{
			qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM, QSE_NULL);
			return -1;
		}

		p->name = QSE_AWK_STRDUP (run->awk, name);
		if (p->name == QSE_NULL)
		{
			QSE_AWK_FREE (run->awk, p);
			qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM, QSE_NULL);
			return -1;
		}

		p->type = (io_type | io_mask);
		p->mode = io_mode;
		p->rwcmode = QSE_AWK_RIO_CLOSE_FULL;
		p->handle = QSE_NULL;
		p->next = QSE_NULL;
		p->rwcstate = 0;

		p->in.buf[0] = QSE_T('\0');
		p->in.pos = 0;
		p->in.len = 0;
		p->in.eof = 0;
		p->in.eos = 0;

		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOERR, QSE_NULL);

		/* request to open the stream */
		x = handler (run, QSE_AWK_RIO_OPEN, p, QSE_NULL, 0);
		if (x <= -1)
		{
			QSE_AWK_FREE (run->awk, p->name);
			QSE_AWK_FREE (run->awk, p);

			if (run->errinf.num == QSE_AWK_ENOERR)
			{
				/* if the error number has not been 
				 * set by the user handler */
				qse_awk_rtx_seterrnum (run, QSE_AWK_EIOIMPL, QSE_NULL);
			}

			return -1;
		}

		/* chain it */
		p->next = run->rio.chain;
		run->rio.chain = p;

		/* usually, x == 0 indicates that it has reached the end 
		 * of the input. the user io handler can return 0 for the 
		 * open request if it doesn't have any files to open. One 
		 * advantage of doing this would be that you can skip the 
		 * entire pattern-block matching and exeuction. */
		if (x == 0) 
		{
			p->in.eos = 1;
			return 0;
		}
	}

	if (p->in.eos)
	{
		/* no more streams. */
		return 0;
	}

	/* ready to read a line. clear the line buffer */
	qse_str_clear (buf);

	/* get the record separator */
	rs = qse_awk_rtx_getgbl (run, QSE_AWK_GBL_RS);
	qse_awk_rtx_refupval (run, rs);

	if (rs->type == QSE_AWK_VAL_NIL)
	{
		rs_ptr = QSE_NULL;
		rs_len = 0;
	}
	else if (rs->type == QSE_AWK_VAL_STR)
	{
		rs_ptr = ((qse_awk_val_str_t*)rs)->ptr;
		rs_len = ((qse_awk_val_str_t*)rs)->len;
	}
	else 
	{
		rs_ptr = qse_awk_rtx_valtocpldup (run, rs, &rs_len);
		if (rs_ptr == QSE_NULL)
		{
			qse_awk_rtx_refdownval (run, rs);
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

			qse_awk_rtx_seterrnum (run, QSE_AWK_ENOERR, QSE_NULL);

			n = handler (run, QSE_AWK_RIO_READ,
				p, p->in.buf, QSE_COUNTOF(p->in.buf));
			if (n <= -1) 
			{
				if (run->errinf.num == QSE_AWK_ENOERR)
				{
					/* if the error number has not been 
				 	 * set by the user handler */
					qse_awk_rtx_seterrnum (run, QSE_AWK_EIOIMPL, QSE_NULL);
				}

				ret = -1;
				break;
			}

			if (n == 0) 
			{
				p->in.eof = 1;

				if (QSE_STR_LEN(buf) == 0) ret = 0;
				else if (rs_len >= 2)
				{
					/* when RS is multiple characters, it needs to check
					 * for the match at the end of the input stream as
					 * the buffer has been appened with the last character
					 * after the previous matchrex has failed */

					qse_cstr_t match;
					qse_awk_errnum_t errnum;

					QSE_ASSERT (run->gbl.rs != QSE_NULL);

					n = QSE_AWK_MATCHREX (
						run->awk, run->gbl.rs, 
						((run->gbl.ignorecase)? QSE_REX_MATCH_IGNORECASE: 0),
						QSE_STR_PTR(buf), QSE_STR_LEN(buf), 
						QSE_STR_PTR(buf), QSE_STR_LEN(buf), 
						&match, &errnum);
					if (n <= -1)
					{
						qse_awk_rtx_seterrnum (run, errnum, QSE_NULL);
						ret = -1;
						break;
					}

					if (n >= 1)
					{
						/* the match should be found at the end of
						 * the current buffer */
						QSE_ASSERT (
							QSE_STR_PTR(buf) + QSE_STR_LEN(buf) ==
							match.ptr + match.len);

						QSE_STR_LEN(buf) -= match.len;
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
			qse_cstr_t match;
			qse_awk_errnum_t errnum;

/* TODO: minimize the number of regular expressoin match here...
 *       currently matchrex is called for each character added to buf.
 *       this is a very bad way of doing the job.
 */
			QSE_ASSERT (run->gbl.rs != QSE_NULL);

			n = QSE_AWK_MATCHREX (
				run->awk, run->gbl.rs, 
				((run->gbl.ignorecase)? QSE_REX_MATCH_IGNORECASE: 0),
				QSE_STR_PTR(buf), QSE_STR_LEN(buf), 
				QSE_STR_PTR(buf), QSE_STR_LEN(buf), 
				&match, &errnum);
			if (n <= -1)
			{
				qse_awk_rtx_seterrnum (run, errnum, QSE_NULL);
				ret = -1;
				p->in.pos--; /* unread the character in c */
				break;
			}

			if (n >= 1)
			{
				/* the match should be found at the end of
				 * the current buffer */
				QSE_ASSERT (
					QSE_STR_PTR(buf) + QSE_STR_LEN(buf) ==
					match.ptr + match.len);

				QSE_STR_LEN(buf) -= match.len;
				p->in.pos--; /* unread the character in c */
				break;
			}
		}

		if (qse_str_ccat (buf, c) == (qse_size_t)-1)
		{
			qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM, QSE_NULL);
			ret = -1;
			break;
		}

		/* TODO: handle different line terminator like \r\n */
		if (c == QSE_T('\n')) line_len = 0;
		else line_len = line_len + 1;
	}

	if (rs_ptr != QSE_NULL && 
	    rs->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, rs_ptr);
	qse_awk_rtx_refdownval (run, rs);

	return ret;
}

int qse_awk_rtx_writeio_val (
	qse_awk_rtx_t* run, int out_type, 
	const qse_char_t* name, qse_awk_val_t* v)
{
	qse_char_t* str;
	qse_size_t len;
	int n;

	if (v->type == QSE_AWK_VAL_STR)
	{
		str = ((qse_awk_val_str_t*)v)->ptr;
		len = ((qse_awk_val_str_t*)v)->len;
	}
	else
	{
		qse_awk_rtx_valtostr_out_t out;

		out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP |
		           QSE_AWK_RTX_VALTOSTR_PRINT;
		if (qse_awk_rtx_valtostr (run, v, &out) == QSE_NULL) return -1;

		str = out.u.cpldup.ptr;
		len = out.u.cpldup.len;
	}

	n = qse_awk_rtx_writeio_str (run, out_type, name, str, len);

	if (v->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, str);
	return n;
}

int qse_awk_rtx_writeio_str (
	qse_awk_rtx_t* run, int out_type, 
	const qse_char_t* name, qse_char_t* str, qse_size_t len)
{
	qse_awk_rio_arg_t* p = run->rio.chain;
	qse_awk_rio_fun_t handler;
	int io_type, io_mode, io_mask; 
	qse_ssize_t n;

	QSE_ASSERT (out_type >= 0 && out_type <= QSE_COUNTOF(out_type_map));
	QSE_ASSERT (out_type >= 0 && out_type <= QSE_COUNTOF(out_mode_map));
	QSE_ASSERT (out_type >= 0 && out_type <= QSE_COUNTOF(out_mask_map));

	/* translate the out_type into the relevant io type and mode */
	io_type = out_type_map[out_type];
	io_mode = out_mode_map[out_type];
	io_mask = out_mask_map[out_type];

	handler = run->rio.handler[io_type];
	if (handler == QSE_NULL)
	{
		/* no io handler provided */
		qse_awk_rtx_seterrnum (run, QSE_AWK_EIOUSER, QSE_NULL);
		return -1;
	}

	/* look for the corresponding rio for name */
	while (p != QSE_NULL)
	{
		/* the file "1.tmp", in the following code snippets, 
		 * would be opened by the first print statement, but not by
		 * the second print statement. this is because
		 * both QSE_AWK_OUT_FILE and QSE_AWK_OUT_APFILE are
		 * translated to QSE_AWK_RIO_FILE and it is used to
		 * keep track of file handles..
		 *
		 *    print "1111" >> "1.tmp"
		 *    print "1111" > "1.tmp"
		 */
		if (p->type == (io_type | io_mask) && 
		    qse_strcmp (p->name, name) == 0) break;
		p = p->next;
	}

	/* if there is not corresponding rio for name, create one */
	if (p == QSE_NULL)
	{
		p = (qse_awk_rio_arg_t*) QSE_AWK_ALLOC (
			run->awk, QSE_SIZEOF(qse_awk_rio_arg_t));
		if (p == QSE_NULL)
		{
			qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM, QSE_NULL);
			return -1;
		}

		p->name = QSE_AWK_STRDUP (run->awk, name);
		if (p->name == QSE_NULL)
		{
			QSE_AWK_FREE (run->awk, p);
			qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM, QSE_NULL);
			return -1;
		}

		p->type = (io_type | io_mask);
		p->mode = io_mode;
		p->rwcmode = QSE_AWK_RIO_CLOSE_FULL;
		p->handle = QSE_NULL;
		p->next = QSE_NULL;
		p->rwcstate = 0;

		p->out.eof = 0;
		p->out.eos = 0;

		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOERR, QSE_NULL);
		n = handler (run, QSE_AWK_RIO_OPEN, p, QSE_NULL, 0);
		if (n <= -1)
		{
			QSE_AWK_FREE (run->awk, p->name);
			QSE_AWK_FREE (run->awk, p);

			if (run->errinf.num == QSE_AWK_ENOERR)
				qse_awk_rtx_seterrnum (run, QSE_AWK_EIOIMPL, QSE_NULL);

			return -1;
		}

		/* chain it */
		p->next = run->rio.chain;
		run->rio.chain = p;

		/* usually, n == 0 indicates that it has reached the end 
		 * of the input. the user io handler can return 0 for the 
		 * open request if it doesn't have any files to open. One 
		 * advantage of doing this would be that you can skip the 
		 * entire pattern-block matching and exeuction. */
		if (n == 0) 
		{
			p->out.eos = 1;
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
		qse_awk_rtx_seterrnum (run, QSE_AWK_ENOERR, QSE_NULL);
		n = handler (run, QSE_AWK_RIO_WRITE, p, str, len);
		if (n <= -1) 
		{
			if (run->errinf.num == QSE_AWK_ENOERR)
				qse_awk_rtx_seterrnum (run, QSE_AWK_EIOIMPL, QSE_NULL);

			return -1;
		}

		if (n == 0) 
		{
			p->out.eof = 1;
			return 0;
		}

		len -= n;
		str += n;
	}

	return 1;
}

int qse_awk_rtx_flushio (
	qse_awk_rtx_t* run, int out_type, const qse_char_t* name)
{
	qse_awk_rio_arg_t* p = run->rio.chain;
	qse_awk_rio_fun_t handler;
	int io_type, /*io_mode,*/ io_mask;
	qse_ssize_t n;
	int ok = 0;

	QSE_ASSERT (out_type >= 0 && out_type <= QSE_COUNTOF(out_type_map));
	QSE_ASSERT (out_type >= 0 && out_type <= QSE_COUNTOF(out_mode_map));
	QSE_ASSERT (out_type >= 0 && out_type <= QSE_COUNTOF(out_mask_map));

	/* translate the out_type into the relevant io type and mode */
	io_type = out_type_map[out_type];
	/*io_mode = out_mode_map[out_type];*/
	io_mask = out_mask_map[out_type];

	handler = run->rio.handler[io_type];
	if (handler == QSE_NULL)
	{
		/* no io handler provided */
		qse_awk_rtx_seterrnum (run, QSE_AWK_EIOUSER, QSE_NULL);
		return -1;
	}

	/* look for the corresponding rio for name */
	while (p != QSE_NULL)
	{
		if (p->type == (io_type | io_mask) && 
		    (name == QSE_NULL || qse_strcmp(p->name,name) == 0)) 
		{
			qse_awk_rtx_seterrnum (run, QSE_AWK_ENOERR, QSE_NULL);
			n = handler (run, QSE_AWK_RIO_FLUSH, p, QSE_NULL, 0);

			if (n <= -1) 
			{
				if (run->errinf.num == QSE_AWK_ENOERR)
					qse_awk_rtx_seterrnum (run, QSE_AWK_EIOIMPL, QSE_NULL);
				return -1;
			}

			ok = 1;
		}

		p = p->next;
	}

	if (ok) return 0;

	/* there is no corresponding rio for name */
	qse_awk_rtx_seterrnum (run, QSE_AWK_EIONMNF, QSE_NULL);
	return -1;
}

int qse_awk_rtx_nextio_read (
	qse_awk_rtx_t* run, int in_type, const qse_char_t* name)
{
	qse_awk_rio_arg_t* p = run->rio.chain;
	qse_awk_rio_fun_t handler;
	int io_type, /*io_mode,*/ io_mask; 
	qse_ssize_t n;

	QSE_ASSERT (in_type >= 0 && in_type <= QSE_COUNTOF(in_type_map));
	QSE_ASSERT (in_type >= 0 && in_type <= QSE_COUNTOF(in_mode_map));
	QSE_ASSERT (in_type >= 0 && in_type <= QSE_COUNTOF(in_mask_map));

	/* translate the in_type into the relevant io type and mode */
	io_type = in_type_map[in_type];
	/*io_mode = in_mode_map[in_type];*/
	io_mask = in_mask_map[in_type];

	handler = run->rio.handler[io_type];
	if (handler == QSE_NULL)
	{
		/* no io handler provided */
		qse_awk_rtx_seterrnum (run, QSE_AWK_EIOUSER, QSE_NULL);
		return -1;
	}

	while (p != QSE_NULL)
	{
		if (p->type == (io_type | io_mask) &&
		    qse_strcmp (p->name,name) == 0) break;
		p = p->next;
	}

	if (p == QSE_NULL)
	{
		/* something is totally wrong */
		QSE_ASSERT (
			!"should never happen - cannot find the relevant rio entry");
		qse_awk_rtx_seterrnum (run, QSE_AWK_EINTERN, QSE_NULL);
		return -1;
	}

	if (p->in.eos) 
	{
		/* no more streams. */
		return 0;
	}

	qse_awk_rtx_seterrnum (run, QSE_AWK_ENOERR, QSE_NULL);
	n = handler (run, QSE_AWK_RIO_NEXT, p, QSE_NULL, 0);
	if (n <= -1)
	{
		if (run->errinf.num == QSE_AWK_ENOERR)
			qse_awk_rtx_seterrnum (run, QSE_AWK_EIOIMPL, QSE_NULL);
		return -1;
	}

	if (n == 0) 
	{
		/* the next stream cannot be opened. 
		 * set the eos flags so that the next call to nextio_read
		 * will return 0 without executing the handler */
		p->in.eos = 1;
		return 0;
	}
	else 
	{
		/* as the next stream has been opened successfully,
		 * the eof flag should be cleared if set */
		p->in.eof = 0;

		/* also the previous input buffer must be reset */
		p->in.pos = 0;
		p->in.len = 0;

		return 1;
	}
}

int qse_awk_rtx_nextio_write (
	qse_awk_rtx_t* run, int out_type, const qse_char_t* name)
{
	qse_awk_rio_arg_t* p = run->rio.chain;
	qse_awk_rio_fun_t handler;
	int io_type, /*io_mode,*/ io_mask; 
	qse_ssize_t n;

	QSE_ASSERT (out_type >= 0 && out_type <= QSE_COUNTOF(out_type_map));
	QSE_ASSERT (out_type >= 0 && out_type <= QSE_COUNTOF(out_mode_map));
	QSE_ASSERT (out_type >= 0 && out_type <= QSE_COUNTOF(out_mask_map));

	/* translate the out_type into the relevant io type and mode */
	io_type = out_type_map[out_type];
	/*io_mode = out_mode_map[out_type];*/
	io_mask = out_mask_map[out_type];

	handler = run->rio.handler[io_type];
	if (handler == QSE_NULL)
	{
		/* no io handler provided */
		qse_awk_rtx_seterrnum (run, QSE_AWK_EIOUSER, QSE_NULL);
		return -1;
	}

	while (p != QSE_NULL)
	{
		if (p->type == (io_type | io_mask) &&
		    qse_strcmp (p->name,name) == 0) break;
		p = p->next;
	}

	if (p == QSE_NULL)
	{
		/* something is totally wrong */
		QSE_ASSERT (!"should never happen - cannot find the relevant rio entry");

		qse_awk_rtx_seterrnum (run, QSE_AWK_EINTERN, QSE_NULL);
		return -1;
	}

	if (p->out.eos) 
	{
		/* no more streams. */
		return 0;
	}

	qse_awk_rtx_seterrnum (run, QSE_AWK_ENOERR, QSE_NULL);
	n = handler (run, QSE_AWK_RIO_NEXT, p, QSE_NULL, 0);
	if (n <= -1)
	{
		if (run->errinf.num == QSE_AWK_ENOERR)
			qse_awk_rtx_seterrnum (run, QSE_AWK_EIOIMPL, QSE_NULL);
		return -1;
	}

	if (n == 0) 
	{
		/* the next stream cannot be opened. 
		 * set the eos flags so that the next call to nextio_write
		 * will return 0 without executing the handler */
		p->out.eos = 1;
		return 0;
	}
	else 
	{
		/* as the next stream has been opened successfully,
		 * the eof flag should be cleared if set */
		p->out.eof = 0;
		return 1;
	}
}

int qse_awk_rtx_closio_read (
	qse_awk_rtx_t* run, int in_type, const qse_char_t* name)
{
	qse_awk_rio_arg_t* p = run->rio.chain, * px = QSE_NULL;
	qse_awk_rio_fun_t handler;
	int io_type, /*io_mode,*/ io_mask;

	QSE_ASSERT (in_type >= 0 && in_type <= QSE_COUNTOF(in_type_map));
	QSE_ASSERT (in_type >= 0 && in_type <= QSE_COUNTOF(in_mode_map));
	QSE_ASSERT (in_type >= 0 && in_type <= QSE_COUNTOF(in_mask_map));

	/* translate the in_type into the relevant io type and mode */
	io_type = in_type_map[in_type];
	/*io_mode = in_mode_map[in_type];*/
	io_mask = in_mask_map[in_type];

	handler = run->rio.handler[io_type];
	if (handler == QSE_NULL)
	{
		/* no io handler provided */
		qse_awk_rtx_seterrnum (run, QSE_AWK_EIOUSER, QSE_NULL);
		return -1;
	}

	while (p != QSE_NULL)
	{
		if (p->type == (io_type | io_mask) &&
		    qse_strcmp (p->name, name) == 0) 
		{
			qse_awk_rio_fun_t handler;
		       
			handler = run->rio.handler[p->type & MASK_CLEAR];
			if (handler != QSE_NULL)
			{
				if (handler (run, QSE_AWK_RIO_CLOSE, p, QSE_NULL, 0) <= -1)
				{
					/* this is not a run-time error.*/
					qse_awk_rtx_seterrnum (run, QSE_AWK_EIOIMPL, QSE_NULL);
					return -1;
				}
			}

			if (px != QSE_NULL) px->next = p->next;
			else run->rio.chain = p->next;

			QSE_AWK_FREE (run->awk, p->name);
			QSE_AWK_FREE (run->awk, p);
			return 0;
		}

		px = p;
		p = p->next;
	}

	/* the name given is not found */
	qse_awk_rtx_seterrnum (run, QSE_AWK_EIONMNF, QSE_NULL);
	return -1;
}

int qse_awk_rtx_closio_write (
	qse_awk_rtx_t* run, int out_type, const qse_char_t* name)
{
	qse_awk_rio_arg_t* p = run->rio.chain, * px = QSE_NULL;
	qse_awk_rio_fun_t handler;
	int io_type, /*io_mode,*/ io_mask;

	QSE_ASSERT (out_type >= 0 && out_type <= QSE_COUNTOF(out_type_map));
	QSE_ASSERT (out_type >= 0 && out_type <= QSE_COUNTOF(out_mode_map));
	QSE_ASSERT (out_type >= 0 && out_type <= QSE_COUNTOF(out_mask_map));

	/* translate the out_type into the relevant io type and mode */
	io_type = out_type_map[out_type];
	/*io_mode = out_mode_map[out_type];*/
	io_mask = out_mask_map[out_type];

	handler = run->rio.handler[io_type];
	if (handler == QSE_NULL)
	{
		/* no io handler provided */
		qse_awk_rtx_seterrnum (run, QSE_AWK_EIOUSER, QSE_NULL);
		return -1;
	}

	while (p != QSE_NULL)
	{
		if (p->type == (io_type | io_mask) &&
		    qse_strcmp (p->name, name) == 0) 
		{
			qse_awk_rio_fun_t handler;
		       
			handler = run->rio.handler[p->type & MASK_CLEAR];
			if (handler != QSE_NULL)
			{
				qse_awk_rtx_seterrnum (run, QSE_AWK_ENOERR, QSE_NULL);
				if (handler (run, QSE_AWK_RIO_CLOSE, p, QSE_NULL, 0) <= -1)
				{
					if (run->errinf.num == QSE_AWK_ENOERR)
						qse_awk_rtx_seterrnum (run, QSE_AWK_EIOIMPL, QSE_NULL);
					return -1;
				}
			}

			if (px != QSE_NULL) px->next = p->next;
			else run->rio.chain = p->next;

			QSE_AWK_FREE (run->awk, p->name);
			QSE_AWK_FREE (run->awk, p);
			return 0;
		}

		px = p;
		p = p->next;
	}

	qse_awk_rtx_seterrnum (run, QSE_AWK_EIONMNF, QSE_NULL);
	return -1;
}

int qse_awk_rtx_closeio (
	qse_awk_rtx_t* rtx, const qse_char_t* name, const qse_char_t* opt)
{
	qse_awk_rio_arg_t* p = rtx->rio.chain, * px = QSE_NULL;

	while (p != QSE_NULL)
	{
		 /* it handles the first that matches the given name
		  * regardless of the io type */
		if (qse_strcmp (p->name, name) == 0) 
		{
			qse_awk_rio_fun_t handler;
			qse_awk_rio_rwcmode_t rwcmode = QSE_AWK_RIO_CLOSE_FULL;

			if (opt != QSE_NULL)
			{
				if (opt[0] == QSE_T('r'))
				{
					if (p->type & MASK_RDWR) 
					{
						if (p->rwcstate != QSE_AWK_RIO_CLOSE_WRITE)
						{
							/* if the write end is not
							 * closed, let io handler close
							 * the read end only. */
							rwcmode = QSE_AWK_RIO_CLOSE_READ;
						}
					}
					else if (!(p->type & MASK_READ)) goto skip;
				}
				else
				{
					QSE_ASSERT (opt[0] == QSE_T('w'));
					if (p->type & MASK_RDWR)
					{
						if (p->rwcstate != QSE_AWK_RIO_CLOSE_READ)
						{
							/* if the read end is not 
							 * closed, let io handler close
							 * the write end only. */
							rwcmode = QSE_AWK_RIO_CLOSE_WRITE;
						}
					}
					else if (!(p->type & MASK_WRITE)) goto skip;
				}
			}

			handler = rtx->rio.handler[p->type & MASK_CLEAR];
			if (handler != QSE_NULL)
			{
				qse_awk_rtx_seterrnum (rtx, QSE_AWK_ENOERR, QSE_NULL);
				p->rwcmode = rwcmode;
				if (handler (rtx, QSE_AWK_RIO_CLOSE, p, QSE_NULL, 0) <= -1)
				{
					/* this is not a run-time error.*/
					if (rtx->errinf.num == QSE_AWK_ENOERR)
						qse_awk_rtx_seterrnum (rtx, QSE_AWK_EIOIMPL, QSE_NULL);
					return -1;
				}
			}

			if (p->type & MASK_RDWR) 
			{
				p->rwcmode = rwcmode;
				if (p->rwcstate == 0 && rwcmode != 0)
				{
					/* if either end has not been closed.
					 * return success without destroying 
					 * the internal node. rwcstate keeps 
					 * what has been successfully closed */
					p->rwcstate = rwcmode;
					return 0;
				}
			}

			if (px != QSE_NULL) px->next = p->next;
			else rtx->rio.chain = p->next;

			QSE_AWK_FREE (rtx->awk, p->name);
			QSE_AWK_FREE (rtx->awk, p);

			return 0;
		}

	skip:
		px = p;
		p = p->next;
	}

	qse_awk_rtx_seterrnum (rtx, QSE_AWK_EIONMNF, QSE_NULL);
	return -1;
}

void qse_awk_rtx_cleario (qse_awk_rtx_t* run)
{
	qse_awk_rio_arg_t* next;
	qse_awk_rio_fun_t handler;
	qse_ssize_t n;

	while (run->rio.chain != QSE_NULL)
	{
		handler = run->rio.handler[
			run->rio.chain->type & MASK_CLEAR];
		next = run->rio.chain->next;

		if (handler != QSE_NULL)
		{
			qse_awk_rtx_seterrnum (run, QSE_AWK_ENOERR, QSE_NULL);
			run->rio.chain->rwcmode = 0;
			n = handler (run, QSE_AWK_RIO_CLOSE, run->rio.chain, QSE_NULL, 0);
			if (n <= -1)
			{
				if (run->errinf.num == QSE_AWK_ENOERR)
					qse_awk_rtx_seterrnum (run, QSE_AWK_EIOIMPL, QSE_NULL);
				/* TODO: some warnings need to be shown??? */
			}
		}

		QSE_AWK_FREE (run->awk, run->rio.chain->name);
		QSE_AWK_FREE (run->awk, run->rio.chain);

		run->rio.chain = next;
	}
}
