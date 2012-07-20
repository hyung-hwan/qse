/*
 * $Id: rio.c 480 2011-05-25 14:00:19Z hyunghwan.chung $
 *
    Copyright 2006-2012 Chung, Hyung-Hwan.
    This file is part of QSE.

    QSE is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as 
    published by the Free Software Foundation, either version 3 of 
    the License, or (at your option) any later version.

    QSE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public 
    License along with QSE. If not, see <http://www.gnu.org/licenses/>.
 */

#include "awk.h"

enum io_mask_t
{
	IO_MASK_READ  = 0x0100,
	IO_MASK_WRITE = 0x0200,
	IO_MASK_RDWR  = 0x0400,
	IO_MASK_CLEAR = 0x00FF
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
	IO_MASK_READ,
	IO_MASK_RDWR,
	IO_MASK_READ,
	IO_MASK_READ
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
	IO_MASK_WRITE,
	IO_MASK_RDWR,
	IO_MASK_WRITE,
	IO_MASK_WRITE,
	IO_MASK_WRITE
};

static int find_rio_in (
	qse_awk_rtx_t* run, int in_type, const qse_char_t* name,
	qse_awk_rio_arg_t** rio, qse_awk_rio_fun_t* fun)
{
	qse_awk_rio_arg_t* p = run->rio.chain;
	qse_awk_rio_fun_t handler;
	int io_type, io_mode, io_mask;

	QSE_ASSERT (in_type >= 0 && in_type <= QSE_COUNTOF(in_type_map));
	QSE_ASSERT (in_type >= 0 && in_type <= QSE_COUNTOF(in_mode_map));
	QSE_ASSERT (in_type >= 0 && in_type <= QSE_COUNTOF(in_mask_map));

	/* translate the in_type into the relevant io type and mode */
	io_type = in_type_map[in_type];
	io_mode = in_mode_map[in_type];
	io_mask = in_mask_map[in_type];

	/* get the I/O handler provided by a user */
	handler = run->rio.handler[io_type];
	if (handler == QSE_NULL)
	{
		/* no I/O handler provided */
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
		qse_ssize_t x;

		/* if the name doesn't exist in the chain, create an entry
		 * to the chain */
		p = (qse_awk_rio_arg_t*) QSE_AWK_ALLOC (
			run->awk, QSE_SIZEOF(qse_awk_rio_arg_t));
		if (p == QSE_NULL)
		{
			qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM, QSE_NULL);
			return -1;
		}

		QSE_MEMSET (p, 0, QSE_SIZEOF(*p));

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
		/*
		p->handle = QSE_NULL;
		p->next = QSE_NULL;
		p->rwcstate = 0;

		p->in.buf[0] = QSE_T('\0');
		p->in.pos = 0;
		p->in.len = 0;
		p->in.eof = 0;
		p->in.eos = 0;
		*/

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
		 * of the input. the user I/O handler can return 0 for the
		 * open request if it doesn't have any files to open. One
		 * advantage of doing this would be that you can skip the
		 * entire pattern-block matching and execution. */
		if (x == 0) p->in.eos = 1;
	}

	*rio = p;
	*fun = handler;

	return 0;
}

static QSE_INLINE int resolve_rs (
	qse_awk_rtx_t* run, qse_awk_val_t* rs, qse_xstr_t* rrs)
{
	int ret = 0;

	switch (rs->type)
	{
		case QSE_AWK_VAL_NIL:
			rrs->ptr = QSE_NULL;
			rrs->len = 0;
			break;

		case QSE_AWK_VAL_STR:
			rrs->ptr = ((qse_awk_val_str_t*)rs)->val.ptr;
			rrs->len = ((qse_awk_val_str_t*)rs)->val.len;
			break;

		default:
			rrs->ptr = qse_awk_rtx_valtocpldup (run, rs, &rrs->len);
			if (rrs->ptr == QSE_NULL) ret = -1;
			break;
	}

	return ret;
}

static QSE_INLINE int match_long_rs (
	qse_awk_rtx_t* run, qse_str_t* buf, qse_awk_rio_arg_t* p)
{
	qse_cstr_t match;
	qse_awk_errnum_t errnum;
	int ret;

	QSE_ASSERT (run->gbl.rs != QSE_NULL);

	ret = QSE_AWK_MATCHREX (
		run->awk, run->gbl.rs,
		((run->gbl.ignorecase)? QSE_REX_IGNORECASE: 0),
		QSE_STR_CSTR(buf), QSE_STR_CSTR(buf),
		&match, &errnum);
	if (ret <= -1)
	{
		qse_awk_rtx_seterrnum (run, errnum, QSE_NULL);
	}
	else if (ret >= 1)
	{
		if (p->in.eof)
		{
			/* when EOF is reached, the record buffer
			 * is not added with a new character. It's
			 * just called again with the same record buffer
			 * as the previous call to this function.
			 * A match in this case must end at the end of
			 * the current record buffer */
			QSE_ASSERT (
				QSE_STR_PTR(buf) + QSE_STR_LEN(buf) == 
				match.ptr + match.len
			);

			/* drop the RS part. no extra character after RS to drop
			 * because we're at EOF and the EOF condition didn't
			 * add a new character to the buffer before the call
			 * to this function.
			 */
			QSE_STR_LEN(buf) -= match.len;
		}
		else
		{
			/* If the match is found before the end of the current buffer,
			 * I see it as the longest match. A match ending at the end
			 * of the buffer is not indeterministic as we don't have the
			 * full input yet.
			 */
			const qse_char_t* be = QSE_STR_PTR(buf) + QSE_STR_LEN(buf);
			const qse_char_t* me = match.ptr + match.len;

			if (me < be)
			{
				/* the match ends before the ending boundary.
				 * it must be the longest match. drop the RS part
				 * and the characters after RS. */
				QSE_STR_LEN(buf) -= match.len + (be - me);
				p->in.pos -= (be - me);
			}
			else
			{
				/* the match is at the ending boundary. switch to no match */
				ret = 0;
			}
		}
	}

	return ret;
}

int qse_awk_rtx_readio (
	qse_awk_rtx_t* run, int in_type,
	const qse_char_t* name, qse_str_t* buf)
{
	qse_awk_rio_arg_t* p;
	qse_awk_rio_fun_t handler;
	int ret;

	qse_awk_val_t* rs;
	qse_xstr_t rrs;

	qse_size_t line_len = 0;
	qse_char_t c = QSE_T('\0'), pc;

	if (find_rio_in (run, in_type, name, &p, &handler) <= -1) return -1;
	if (p->in.eos) return 0; /* no more streams left */

	/* ready to read a record(typically a line). clear the buffer. */
	qse_str_clear (buf);

	/* get the record separator */
	rs = qse_awk_rtx_getgbl (run, QSE_AWK_GBL_RS);
	qse_awk_rtx_refupval (run, rs);

	if (resolve_rs (run, rs, &rrs) <= -1)
	{
		qse_awk_rtx_refdownval (run, rs);
		return -1;
	}

	ret = 1;

	/* call the I/O handler */
	while (1)
	{
		if (p->in.pos >= p->in.len)
		{
			qse_ssize_t x;

			/* no more data in the read buffer.
			 * let the I/O handler read more */

			if (p->in.eof)
			{
				/* it has reached EOF at the previous call. */
				if (QSE_STR_LEN(buf) == 0)
				{
					/* we return EOF if the record buffer is empty */
					ret = 0;
				}
				break;
			}

			qse_awk_rtx_seterrnum (run, QSE_AWK_ENOERR, QSE_NULL);

			x = handler (run, QSE_AWK_RIO_READ,
				p, p->in.buf, QSE_COUNTOF(p->in.buf));
			if (x <= -1)
			{
				if (run->errinf.num == QSE_AWK_ENOERR)
				{
					/* if the error number has not been 
				 	 * set by the user handler, we set
				 	 * it here to QSE_AWK_EIOIMPL. */
					qse_awk_rtx_seterrnum (run, QSE_AWK_EIOIMPL, QSE_NULL);
				}

				ret = -1;
				break;
			}

			if (x == 0)
			{
				/* EOF reached */
				p->in.eof = 1;

				if (QSE_STR_LEN(buf) == 0)
				{
					/* We can return EOF now if the record buffer
					 * is empty */
					ret = 0;
				}
				else if (rrs.ptr != QSE_NULL && rrs.len == 0)
				{
					/* TODO: handle different line terminator */
					/* drop the line terminator from the record
					 * if RS is a blank line and EOF is reached. */
					if (QSE_STR_LASTCHAR(buf) == QSE_T('\n'))
					{
						QSE_STR_LEN(buf) -= 1;
						if (run->awk->option & QSE_AWK_CRLF)
						{
							/* drop preceding CR */
							if (QSE_STR_LEN(buf) > 0 &&
							    QSE_STR_LASTCHAR(buf) == QSE_T('\r'))
								QSE_STR_LEN(buf) -= 1;
						}
					}
				}
				else if (rrs.len >= 2)
				{
					/* When RS is multiple characters, it should 
					 * check for the match at the end of the 
					 * input stream also because the previous 
					 * match could fail as it didn't end at the
					 * desired position to be the longest match.
					 * At EOF, the match at the end is considered 
					 * the longest as there are no more characters
					 * left */
					int n = match_long_rs (run, buf, p);
					if (n != 0)
					{
						if (n <= -1) ret = -1;
						break;
					}
				}

				break;
			}

			p->in.len = x;
			p->in.pos = 0;
		}

		if (rrs.ptr == QSE_NULL)
		{
			qse_size_t start_pos = p->in.pos;
			qse_size_t end_pos, tmp;

			do
			{
				pc = c;
				c = p->in.buf[p->in.pos++];
				end_pos = p->in.pos;

				/* TODO: handle different line terminator */
				/* separate by a new line */
				if (c == QSE_T('\n')) 
				{
					end_pos--;
					if (pc == QSE_T('\r'))
					{
						if (end_pos > start_pos)
						{
							/* CR is the part of the read buffer.
							 * decrementing the end_pos variable can
							 * simply drop it */
							end_pos--;
						}
						else
						{
							/* CR must have come from the previous
							 * read. drop CR that must be found  at 
							 * the end of the record buffer. */
							QSE_ASSERT (end_pos == start_pos);
							QSE_ASSERT (QSE_STR_LEN(buf) > 0);
							QSE_ASSERT (QSE_STR_LASTCHAR(buf) == QSE_T('\r'));
							QSE_STR_LEN(buf)--;
						}
					}
					break;
				}
			}
			while (p->in.pos < p->in.len);

			tmp = qse_str_ncat (
				buf,
				&p->in.buf[start_pos],
				end_pos - start_pos
			);
			if (tmp == (qse_size_t)-1)
			{
				qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM, QSE_NULL);
				ret = -1;
				break;
			}

			if (end_pos < p->in.len) break; /* RS found */
		}
		else if (rrs.len == 0)
		{
			int done = 0;

			do
			{
				pc = c;
				c = p->in.buf[p->in.pos++];

				/* TODO: handle different line terminator */
				/* separate by a blank line */
				if (c == QSE_T('\n'))
				{
					if (pc == QSE_T('\r') &&
					    QSE_STR_LEN(buf) > 0)
					{
						/* shrink the line length and the record
						 * by dropping of CR before NL */
						QSE_ASSERT (line_len > 0);
						line_len--;

						/* we don't drop CR from the record buffer 
						 * if we're in CRLF mode. POINT-X */	
						if (!(run->awk->option & QSE_AWK_CRLF))
							QSE_STR_LEN(buf) -= 1;
					}

					if (line_len == 0)
					{
						/* we got a blank line */

						if (run->awk->option & QSE_AWK_CRLF)
						{
							if (QSE_STR_LEN(buf) > 0 && 
							    QSE_STR_LASTCHAR(buf) == QSE_T('\r'))
							{
								/* drop CR not dropped in POINT-X above */
								QSE_STR_LEN(buf) -= 1;
							}

							if (QSE_STR_LEN(buf) <= 0)
							{
								/* if the record is empty when a blank
								 * line is encountered, the line
								 * terminator should not be added to
								 * the record */
								continue;
							}

							/* drop NL */
							QSE_STR_LEN(buf) -= 1;

							/* drop preceding CR */
							if (QSE_STR_LEN(buf) > 0 &&
							    QSE_STR_LASTCHAR(buf) == QSE_T('\r')) 
								QSE_STR_LEN(buf) -= 1;
						}
						else
						{
							if (QSE_STR_LEN(buf) <= 0)
							{
								/* if the record is empty when a blank
								 * line is encountered, the line
								 * terminator should not be added to
								 * the record */
								continue;
							}

							/* drop NL of the previous line */
							QSE_STR_LEN(buf) -= 1; /* simply drop NL */
						}

						done = 1;
						break;
					}

					line_len = 0;
				}
				else line_len++;

				if (qse_str_ccat (buf, c) == (qse_size_t)-1)
				{
					qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM, QSE_NULL);
					ret = -1;
					done = 1;
					break;
				}
			}
			while (p->in.pos < p->in.len);

			if (done) break;
		}
		else if (rrs.len == 1)
		{
			qse_size_t start_pos = p->in.pos;
			qse_size_t end_pos, tmp;

			do
			{
				c = p->in.buf[p->in.pos++];
				end_pos = p->in.pos;
				if (c == rrs.ptr[0])
				{
					end_pos--;
					break;
				}
			}
			while (p->in.pos < p->in.len);

			tmp = qse_str_ncat (
				buf,
				&p->in.buf[start_pos],
				end_pos - start_pos
			);
			if (tmp == (qse_size_t)-1)
			{
				qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM, QSE_NULL);
				ret = -1;
				break;
			}

			if (end_pos < p->in.len) break; /* RS found */
		}
		else
		{
			qse_size_t tmp;
			int n;

			/* if RS is composed of multiple characters,
			 * I perform the matching after having added the
			 * current character 'c' to the record buffer 'buf'
			 * to find the longest match. If a match found ends
			 * one character before this character just added
			 * to the buffer, it is the longest match.
			 */

			tmp = qse_str_ncat (
				buf,
				&p->in.buf[p->in.pos],
				p->in.len - p->in.pos
			);
			if (tmp == (qse_size_t)-1)
			{
				qse_awk_rtx_seterrnum (run, QSE_AWK_ENOMEM, QSE_NULL);
				ret = -1;
				break;
			}

			p->in.pos = p->in.len;

			n = match_long_rs (run, buf, p);
			if (n != 0)
			{
				if (n <= -1) ret = -1;
				break;
			}
		}
	}

	if (rrs.ptr != QSE_NULL &&
	    rs->type != QSE_AWK_VAL_STR) QSE_AWK_FREE (run->awk, rrs.ptr);
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
		str = ((qse_awk_val_str_t*)v)->val.ptr;
		len = ((qse_awk_val_str_t*)v)->val.len;
	}
	else
	{
		qse_awk_rtx_valtostr_out_t out;

		out.type = QSE_AWK_RTX_VALTOSTR_CPLDUP |
		           QSE_AWK_RTX_VALTOSTR_PRINT;
		if (qse_awk_rtx_valtostr (run, v, &out) <= -1) return -1;

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
		/* no I/O handler provided */
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
		
		QSE_MEMSET (p, 0, QSE_SIZEOF(*p));

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
		/*
		p->handle = QSE_NULL;
		p->next = QSE_NULL;
		p->rwcstate = 0;

		p->out.eof = 0;
		p->out.eos = 0;
		*/

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
		 * of the input. the user I/O handler can return 0 for the
		 * open request if it doesn't have any files to open. One 
		 * advantage of doing this would be that you can skip the 
		 * entire pattern-block matching and execution. */
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

	/* translate the out_type into the relevant I/O type and mode */
	io_type = out_type_map[out_type];
	/*io_mode = out_mode_map[out_type];*/
	io_mask = out_mask_map[out_type];

	handler = run->rio.handler[io_type];
	if (handler == QSE_NULL)
	{
		/* no I/O handler provided */
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

	/* translate the in_type into the relevant I/O type and mode */
	io_type = in_type_map[in_type];
	/*io_mode = in_mode_map[in_type];*/
	io_mask = in_mask_map[in_type];

	handler = run->rio.handler[io_type];
	if (handler == QSE_NULL)
	{
		/* no I/O handler provided */
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
		 * set the EOS flags so that the next call to nextio_read
		 * will return 0 without executing the handler */
		p->in.eos = 1;
		return 0;
	}
	else 
	{
		/* as the next stream has been opened successfully,
		 * the EOF flag should be cleared if set */
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

	/* translate the out_type into the relevant I/O type and mode */
	io_type = out_type_map[out_type];
	/*io_mode = out_mode_map[out_type];*/
	io_mask = out_mask_map[out_type];

	handler = run->rio.handler[io_type];
	if (handler == QSE_NULL)
	{
		/* no I/O handler provided */
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
		 * set the EOS flags so that the next call to nextio_write
		 * will return 0 without executing the handler */
		p->out.eos = 1;
		return 0;
	}
	else 
	{
		/* as the next stream has been opened successfully,
		 * the EOF flag should be cleared if set */
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

	/* translate the in_type into the relevant I/O type and mode */
	io_type = in_type_map[in_type];
	/*io_mode = in_mode_map[in_type];*/
	io_mask = in_mask_map[in_type];

	handler = run->rio.handler[io_type];
	if (handler == QSE_NULL)
	{
		/* no I/O handler provided */
		qse_awk_rtx_seterrnum (run, QSE_AWK_EIOUSER, QSE_NULL);
		return -1;
	}

	while (p != QSE_NULL)
	{
		if (p->type == (io_type | io_mask) &&
		    qse_strcmp (p->name, name) == 0) 
		{
			qse_awk_rio_fun_t handler;
		       
			handler = run->rio.handler[p->type & IO_MASK_CLEAR];
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
		       
			handler = run->rio.handler[p->type & IO_MASK_CLEAR];
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
					if (p->type & IO_MASK_RDWR) 
					{
						if (p->rwcstate != QSE_AWK_RIO_CLOSE_WRITE)
						{
							/* if the write end is not
							 * closed, let io handler close
							 * the read end only. */
							rwcmode = QSE_AWK_RIO_CLOSE_READ;
						}
					}
					else if (!(p->type & IO_MASK_READ)) goto skip;
				}
				else
				{
					QSE_ASSERT (opt[0] == QSE_T('w'));
					if (p->type & IO_MASK_RDWR)
					{
						if (p->rwcstate != QSE_AWK_RIO_CLOSE_READ)
						{
							/* if the read end is not 
							 * closed, let io handler close
							 * the write end only. */
							rwcmode = QSE_AWK_RIO_CLOSE_WRITE;
						}
					}
					else if (!(p->type & IO_MASK_WRITE)) goto skip;
				}
			}

			handler = rtx->rio.handler[p->type & IO_MASK_CLEAR];
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

			if (p->type & IO_MASK_RDWR) 
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
			run->rio.chain->type & IO_MASK_CLEAR];
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
