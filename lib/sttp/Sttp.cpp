/*
 * $Id$
 *
    Copyright (c) 2006-2019 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <qse/sttp/Sttp.hpp>

#include <qse/cmn/chr.h>
#include <qse/cmn/utf8.h>
#include "../cmn/mem-prv.h"

QSE_BEGIN_NAMESPACE(QSE)

Sttp::Sttp (Mmgr* mmgr) QSE_CPP_NOEXCEPT: Mmged(mmgr)
{
	this->rd_rd_state_top.state = STATE_START;
	this->rd_rd_state_top.next = QSE_NULL;
	this->rd_state_stack = &this->rd_rd_state_top;
	this->rd_lo_len = 0;

	this->wr_buf_len = 0;
	this->wr_arg_count = 0;
}

Sttp::~Sttp ()
{
	this->pop_all_read_states ();
}

void Sttp::reset ()
{
	this->pop_all_read_states ();
	this->rd_lo_len = 0;

	this->wr_buf_len = 0;
	this->wr_arg_count = 0;
}

int Sttp::feed (const qse_uint8_t* data, qse_size_t len, qse_size_t* rem)
{
	qse_size_t xlen, pos = 0, alen = len;

	if (this->rd_lo_len > 0)
	{
		int n;
		while (1)
		{
			this->rd_lo[this->rd_lo_len++] = data[pos++];
			alen--;

			n = this->feed_chunk(this->rd_lo, this->rd_lo_len, &xlen);
			if (n <= -1) return -1;
			if (n == 0 && xlen == 0)
			{
				/* not complete - incomplete sequence */
				if (alen > 0) continue; /* but still has more data given */
				goto done; /* still not resolved the incomplete sequence */
			}

			break;
		}
		QSE_ASSERT (xlen == this->rd_lo_len);
		this->rd_lo_len = 0;
	}

	while (alen > 0)
	{
		int n = this->feed_chunk(&data[pos], alen, &xlen);
		if (n <= -1) return -1;

		pos += xlen;
		alen -= xlen;

		if (rem) 
		{
			/*
			n=-1 error  
			n=0 need more data                 rem > 0 -> incomplete sequence             rem == 0 incomplete command
			n=1 completed at least a command   rem > 0 -> more command data at the back.  rem == 0. no more command
			*/
			break;
		}

		if (n == 0)
		{
			while (alen > 0)
			{
				/* the unprocessed data is due to an incomplete sequence */
				this->rd_lo[this->rd_lo_len++] = data[pos++];
				alen--;
			}
			break;
		}
	}

done:
	if (rem) *rem = alen;
	return 0;
}

int Sttp::feed_chunk (const qse_uint8_t* data, qse_size_t len, qse_size_t* xlen)
{
	const qse_uint8_t* ptr, * end, * optr;
	bool ever_completed = false;

	ptr = data;
	end = ptr + len;

//printf ("FEED => len=%d  [", (int)len);
//for (int i = 0; i < len; i++) printf ("%u ", data[i]);
//printf ("]\n");
	while (ptr < end)
	{
		qse_char_t c;

		optr = ptr;
	#if defined(QSE_CHAR_IS_MCHAR)
		c = *ptr++;
	#else
		qse_size_t bcslen = end - ptr;
		qse_wchar_t uc;
		qse_size_t n = qse_utf8touc((const qse_mchar_t*)ptr, bcslen, &uc);
		if (n == 0) 
		{
			/* invalid sequence */
			this->setErrorFmt (E_EINVAL, QSE_T("invalid utf8 sequence starting with 0x%lx"), (unsigned long int)*ptr);
			return -1;
		}
		else if (n > bcslen) 
		{
			/* incomplete sequence */
			break;
		}

		ptr += n;
		c = uc;
	#endif

		if (this->rd_state_stack->state == STATE_START && this->is_space_char(c)) continue;
		if (ever_completed) 
		{
			ptr = optr;
			break;
		}

		if (this->handle_char(c) <= -1)  return -1; /* error */
		if (this->rd_state_stack->state == STATE_START) ever_completed = true; // don't break here to consume some space after the semicolon
	}

	*xlen = ptr - data;
	return ever_completed? 1: 0;
}

int Sttp::handle_char (qse_char_t c)
{
	int x;

start_over:
	switch (this->rd_state_stack->state)
	{
		case STATE_START:
			x = this->handle_start_char(c);
			break;

		case STATE_IN_NAME:
			x = this->handle_name_char(c);
			break;

		case STATE_IN_PARAM_LIST:
			x = this->handle_param_list_char(c);
			break;

		case STATE_IN_PARAM_WORD:
			x = this->handle_param_word_char(c);
			break;

		case STATE_IN_PARAM_STRING:
			x = this->handle_param_string_char(c);
			break;

		default:
			this->setErrorNumber (E_EINTERN);
			x = -1;
			break;
	}

	if (x <= -1) return -1;
	if (x == 0) goto start_over;

	return x;
}

#define PUSH_READ_STATE(x) if (this->push_read_state(x) <= -1) return -1;

int Sttp::handle_start_char (qse_char_t c)
{
	if (this->is_ident_char(c))
	{
		this->token.append (c);
		PUSH_READ_STATE (STATE_IN_NAME);
		return 1;
	}
#if 0
	else if (c == ';')
	{
		// empty command
	}
#endif
	else
	{
		this->setErrorFmt (E_EINVAL, QSE_T("invalid start character 0x%lx[%jc]"), (unsigned long int)c, c);
		return -1;
	}
}

int Sttp::handle_name_char (qse_char_t c)
{
	if (this->is_ident_char(c))
	{
		this->token.append (c);
	}
	else if (this->is_space_char(c))
	{
		this->command.setName (this->token);
		this->clear_token ();
		this->pop_read_state ();
		PUSH_READ_STATE (STATE_IN_PARAM_LIST);
	}
	else if (c == ';')
	{
		this->command.setName (this->token);
		this->clear_token ();
		this->pop_read_state ();

		int x = this->handle_command(this->command);
		this->command.clear ();
		if (x <= -1) return -1;
	}
	else
	{
		this->setErrorFmt (E_EINVAL, QSE_T("invalid character 0x%lx[%jc] in the command name"), (unsigned long int)c, c);
		return -1;
	}

	return 1;
}

int Sttp::handle_param_list_char (qse_char_t c)
{
	if (c == ';')
	{
		if (this->rd_state_stack->u.ipl.got_value || this->command.getArgCount() == 0)
		{
			if (this->rd_state_stack->u.ipl.got_value) this->command.addArg (this->token);
			this->clear_token();
			this->rd_state_stack->u.ipl.got_value = false;
			this->pop_read_state (); // back to the START state.

			int x = this->handle_command(this->command);
			this->command.clear ();
			if (x <= -1) return -1;
		}
		else
		{
			this->setErrorFmt (E_EINVAL, QSE_T("no parameter after a comma"));
			return -1;
		}
	}
	else if (c == ',')
	{
		if (this->rd_state_stack->u.ipl.got_value)
		{
			this->command.addArg (this->token);
			this->clear_token();
			this->rd_state_stack->u.ipl.got_value = false;
		}
		else
		{
			this->setErrorFmt (E_EINVAL, QSE_T("redundant comma"));
			return -1;
		}
	}
	else if (this->is_space_char(c))
	{
		// do nothing;
	}
	else
	{
		if (this->rd_state_stack->u.ipl.got_value)
		{
			// comma required.
			this->setErrorFmt (E_EINVAL, QSE_T("comma required"));
			return -1;
		}

		if (c == '\"' || c == '\'')
		{
			this->rd_state_stack->u.ipl.got_value = true;
			PUSH_READ_STATE (STATE_IN_PARAM_STRING);
			this->rd_state_stack->u.ps.qc = c;
			this->clear_token ();
			return 1;
		}
		else if (this->is_ident_char(c))
		{
			this->rd_state_stack->u.ipl.got_value = true;
			PUSH_READ_STATE (STATE_IN_PARAM_WORD);
			this->clear_token ();
			this->add_char_to_token (c);
			return 1;
		}
		else
		{
			this->setErrorFmt (E_EINVAL, QSE_T("invalid character 0x%lx[%jc]"), (unsigned long int)c, c);
			return -1;
		}
	}

	return 1;
}

int Sttp::handle_param_word_char (qse_char_t c)
{
	if (this->is_ident_char(c))
	{
		this->token.append (c);
		return 1;
	}

	this->pop_read_state ();
	return 0;  /* let handle_char() to handle this comma again */
}

static QSE_INLINE qse_char_t unescape (qse_char_t c)
{
	switch (c)
	{
		case 'a': return '\a';
		case 'b': return '\b';
		case 'f': return '\f';
		case 'n': return '\n';
		case 'r': return '\r';
		case 't': return '\t';
		case 'v': return '\v';
		default: return c;
	}
}

int Sttp::handle_param_string_char (qse_char_t c)
{
	int ret = 1;

	if (this->rd_state_stack->u.ps.escaped == 3)
	{
		if (c >= '0' && c <= '7')
		{
			this->rd_state_stack->u.ps.acc = this->rd_state_stack->u.ps.acc * 8 + c - '0';
			this->rd_state_stack->u.ps.digit_count++;
			if (this->rd_state_stack->u.ps.digit_count >= this->rd_state_stack->u.ps.escaped) goto add_sv_acc;
		}
		else
		{
			ret = 0;
			goto add_sv_acc;
		}
	}
	else if (this->rd_state_stack->u.ps.escaped >= 2)
	{
		if (c >= '0' && c <= '9')
		{
			this->rd_state_stack->u.ps.acc = this->rd_state_stack->u.ps.acc * 16 + c - '0';
			this->rd_state_stack->u.ps.digit_count++;
			if (this->rd_state_stack->u.ps.digit_count >= this->rd_state_stack->u.ps.escaped) goto add_sv_acc;
		}
		else if (c >= 'a' && c <= 'f')
		{
			this->rd_state_stack->u.ps.acc = this->rd_state_stack->u.ps.acc * 16 + c - 'a' + 10;
			this->rd_state_stack->u.ps.digit_count++;
			if (this->rd_state_stack->u.ps.digit_count >= this->rd_state_stack->u.ps.escaped) goto add_sv_acc;
		}
		else if (c >= 'A' && c <= 'F')
		{
			this->rd_state_stack->u.ps.acc = this->rd_state_stack->u.ps.acc * 16 + c - 'A' + 10;
			this->rd_state_stack->u.ps.digit_count++;
			if (this->rd_state_stack->u.ps.digit_count >= this->rd_state_stack->u.ps.escaped) goto add_sv_acc;
		}
		else
		{
			ret = 0;
		add_sv_acc:
		#if defined(QSE_CHAR_IS_MCHAR)
			/* convert the character to utf8 */
			qse_mchar_t bcsbuf[QSE_MBLEN_MAX];
			qse_size_t n;

			n = qse_uctoutf8(this->rd_state_stack->u.ps.acc, bcsbuf, QSE_COUNTOF(bcsbuf));
			if (n == 0 || n > QSE_COUNTOF(bcsbuf))
			{
				// illegal character or buffer to small 
				this->setErrorFmt (E_EINVAL, QSE_T("unable to convert 0x%lx to utf8"), this->rd_state_stack->u.ps.acc);
				return -1;
			}

			this->add_chars_to_token(bcsbuf, n);
		#else
			this->add_char_to_token(this->rd_state_stack->u.ps.acc);
		#endif
			this->rd_state_stack->u.ps.escaped = 0;
		}
	}
	else if (this->rd_state_stack->u.ps.escaped == 1)
	{
		if (c >= '0' && c <= '8') 
		{
			this->rd_state_stack->u.ps.escaped = 3;
			this->rd_state_stack->u.ps.digit_count = 0;
			this->rd_state_stack->u.ps.acc = c - '0';
		}
		else if (c == 'x')
		{
			this->rd_state_stack->u.ps.escaped = 2;
			this->rd_state_stack->u.ps.digit_count = 0;
			this->rd_state_stack->u.ps.acc = 0;
		}
		else if (c == 'u')
		{
			this->rd_state_stack->u.ps.escaped = 4;
			this->rd_state_stack->u.ps.digit_count = 0;
			this->rd_state_stack->u.ps.acc = 0;
		}
		else if (c == 'U')
		{
			this->rd_state_stack->u.ps.escaped = 8;
			this->rd_state_stack->u.ps.digit_count = 0;
			this->rd_state_stack->u.ps.acc = 0;
		}
		else
		{
			this->rd_state_stack->u.ps.escaped = 0;
			this->add_char_to_token(unescape(c));
		}
	}
	else if (c == '\\')
	{
		this->rd_state_stack->u.ps.escaped = 1;
	}
	else if (c == this->rd_state_stack->u.ps.qc)
	{
		this->pop_read_state ();
	}
	else
	{
		this->add_char_to_token(c);
	}

	return ret;
}

int Sttp::push_read_state (rd_state_t state)
{
	rd_state_node_t* ss;

	ss = (rd_state_node_t*)this->getMmgr()->callocate(QSE_SIZEOF(*ss), false);
	if (!ss) 
	{
		this->setErrorNumber (E_ENOMEM);
		return -1;
	}

	ss->state = state;
	ss->next = this->rd_state_stack;

	this->rd_state_stack = ss;
	return 0;
}

void Sttp::pop_read_state ()
{
	rd_state_node_t* ss;

	ss = this->rd_state_stack;
	QSE_ASSERT (ss != QSE_NULL && ss != &this->rd_rd_state_top);
	this->rd_state_stack = ss->next;

	// anything todo here?

	/* TODO: don't free this. move it to the free list? */
	this->getMmgr()->dispose(ss);
}

void Sttp::pop_all_read_states ()
{
	while (this->rd_state_stack != &this->rd_rd_state_top) this->pop_read_state ();
}


#define WRITE_CHAR(x) if (this->write_char(x) <= -1) return -1;

int Sttp::beginWrite (const qse_mchar_t* cmd)
{
	const qse_mchar_t* ptr = cmd;
	this->wr_arg_count = 0;
	while (*ptr != '\0') WRITE_CHAR(*ptr++);
	return 0;
}

int Sttp::beginWrite (const qse_wchar_t* cmd)
{
	const qse_wchar_t* ptr = cmd;
	this->wr_arg_count = 0;
	while (*ptr != '\0') WRITE_CHAR(*ptr++);
	return 0;
}

int Sttp::writeWordArg (const qse_mchar_t* arg)
{
	const qse_mchar_t* ptr = arg;
	if (this->wr_arg_count > 0)  WRITE_CHAR(',');
	WRITE_CHAR (' ');
	while (*ptr != '\0') WRITE_CHAR(*ptr++);
	this->wr_arg_count++;
	return 0;
}

int Sttp::writeWordArg (const qse_wchar_t* arg)
{
	const qse_wchar_t* ptr = arg;
	if (this->wr_arg_count > 0)  WRITE_CHAR(',');
	WRITE_CHAR (' ');
	while (*ptr != '\0') WRITE_CHAR(*ptr++);
	this->wr_arg_count++;
	return 0;
}

int Sttp::writeStringArg (const qse_mchar_t* arg)
{
	const qse_mchar_t* ptr = arg;
	if (this->wr_arg_count > 0)  WRITE_CHAR(',');

	WRITE_CHAR(' ');
	WRITE_CHAR('\"');

	while (*ptr != '\0') 
	{
		if (*ptr == '\"' || *ptr == '\\')  WRITE_CHAR('\\');
		WRITE_CHAR(*ptr++);
	}

	WRITE_CHAR('\"');
	this->wr_arg_count++;
	return 0;
}

int Sttp::writeStringArg (const qse_mchar_t* arg, qse_size_t len)
{
	const qse_mchar_t* ptr = arg;
	const qse_mchar_t* end = arg + len;

	if (this->wr_arg_count > 0)  WRITE_CHAR(',');

	WRITE_CHAR(' ');
	WRITE_CHAR('\"');

	while (ptr < end)
	{
		if (*ptr == '\"' || *ptr == '\\')  WRITE_CHAR('\\');
		WRITE_CHAR(*ptr++);
	}

	WRITE_CHAR('\"');
	this->wr_arg_count++;
	return 0;
}

int Sttp::writeStringArg (const qse_wchar_t* arg)
{
	const qse_wchar_t* ptr = arg;
	if (this->wr_arg_count > 0)  WRITE_CHAR(',');

	WRITE_CHAR(' ');
	WRITE_CHAR('\"');

	while (*ptr != '\0') 
	{
		if (*ptr == '\"' || *ptr == '\\')  WRITE_CHAR('\\');
		WRITE_CHAR(*ptr++);
	}

	WRITE_CHAR('\"');
	this->wr_arg_count++;
	return 0;
}

int Sttp::writeStringArg (const qse_wchar_t* arg, qse_size_t len)
{
	const qse_wchar_t* ptr = arg;
	const qse_wchar_t* end = arg + len;

	if (this->wr_arg_count > 0)  WRITE_CHAR(',');

	WRITE_CHAR(' ');
	WRITE_CHAR('\"');

	while (ptr < end)
	{
		if (*ptr == '\"' || *ptr == '\\')  WRITE_CHAR('\\');
		WRITE_CHAR(*ptr++);
	}

	WRITE_CHAR('\"');
	this->wr_arg_count++;
	return 0;
}


int Sttp::endWrite ()
{
	WRITE_CHAR(';');
	WRITE_CHAR('\n');
	if (this->wr_buf_len > 0)
	{
		if (this->write_bytes(this->wr_buf, this->wr_buf_len) <= -1) return -1;
		this->wr_buf_len = 0;
	}
	return 0;
}


int Sttp::write_char (qse_mchar_t c)
{
	if (this->wr_buf_len >= QSE_COUNTOF(this->wr_buf))
	{
		if (this->write_bytes (this->wr_buf, this->wr_buf_len) <= -1) return -1;
		this->wr_buf_len = 0;
	}

	this->wr_buf[this->wr_buf_len++] = c;
	return 0;
}

int Sttp::write_char (qse_wchar_t c)
{
	qse_mchar_t bcsbuf[QSE_MBLEN_MAX];
	qse_size_t n;

	n = qse_uctoutf8(c, bcsbuf, QSE_COUNTOF(bcsbuf));
	if (n == 0 || n > QSE_COUNTOF(bcsbuf))
	{
		this->setErrorFmt (E_EINVAL, QSE_T("unable to convert 0x%lx to utf8"), (unsigned long int)c);
		return -1;
	}

	for (qse_size_t i = 0; i < n; i++) 
	{
		if (this->write_char(bcsbuf[i]) <= -1) return -1;
	}
	return 0;
}


int Sttp::sendCmd (const qse_mchar_t* name, qse_size_t nargs, ...)
{
	int n;
	va_list ap;
	va_start (ap, nargs);
	n = this->sendCmdV(name, nargs, ap);
	va_end (ap);
	return n;
}

int Sttp::sendCmd (const qse_wchar_t* name, qse_size_t nargs, ...)
{
	int n;
	va_list ap;
	va_start (ap, nargs);
	n = this->sendCmdV(name, nargs, ap);
	va_end (ap);
	return n;
}

int Sttp::sendCmdL (const qse_mchar_t* name, qse_size_t nargs, ...)
{
	int n;
	va_list ap;
	va_start (ap, nargs);
	n = this->sendCmdLV(name, nargs, ap);
	va_end (ap);
	return n;
}

int Sttp::sendCmdL (const qse_wchar_t* name, qse_size_t nargs, ...)
{
	int n;
	va_list ap;
	va_start (ap, nargs);
	n = this->sendCmdLV(name, nargs, ap);
	va_end (ap);
	return n;
}

int Sttp::sendCmdV (const qse_mchar_t* name, qse_size_t nargs, va_list ap)
{
	if (name[0] == '\0') return 0; // don't send a null command
	if (this->beginWrite(name) <= -1) return -1;

	if (nargs > 0)
	{
		for (qse_size_t i = 1; i <= nargs; i++)
		{
			qse_mchar_t* p = va_arg(ap, qse_mchar_t*);
			if (this->writeStringArg(p) <= -1) return -1;
		}
	}

	if (this->endWrite() <= -1) return -1;
	return 0;
}

int Sttp::sendCmdV (const qse_wchar_t* name, qse_size_t nargs, va_list ap)
{
	if (name[0] == '\0') return 0; // don't send a null command
	if (this->beginWrite(name) <= -1) return -1;

	if (nargs > 0)
	{
		for (qse_size_t i = 1; i <= nargs; i++)
		{
			qse_wchar_t* p = va_arg(ap, qse_wchar_t*);
			if (this->writeStringArg(p) <= -1) return -1;
		}
	}

	if (this->endWrite() <= -1) return -1;
	return 0;
}

int Sttp::sendCmdLV (const qse_mchar_t* name, qse_size_t nargs, va_list ap)
{
	if (name[0] == '\0') return 0; // don't send a null command
	if (this->beginWrite(name) <= -1) return -1;

	if (nargs > 0)
	{
		for (qse_size_t i = 1; i <= nargs; i++)
		{
			qse_mchar_t* p = va_arg(ap, qse_mchar_t*);
			qse_size_t l = va_arg(ap, qse_size_t);
			if (this->writeStringArg(p, l) <= -1) return -1;
		}
	}

	if (this->endWrite() <= -1) return -1;
	return 0;
}

int Sttp::sendCmdLV (const qse_wchar_t* name, qse_size_t nargs, va_list ap)
{
	if (name[0] == '\0') return 0; // don't send a null command
	if (this->beginWrite(name) <= -1) return -1;

	if (nargs > 0)
	{
		for (qse_size_t i = 1; i <= nargs; i++)
		{
			qse_wchar_t* p = va_arg(ap, qse_wchar_t*);
			qse_size_t l = va_arg(ap, qse_size_t);
			if (this->writeStringArg(p, l) <= -1) return -1;
		}
	}

	if (this->endWrite() <= -1) return -1;
	return 0;
}


QSE_END_NAMESPACE(QSE)
