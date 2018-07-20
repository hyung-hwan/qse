/*
 * $Id$
 *
    Copyright (c) 2006-2014 Chung, Hyung-Hwan. All rights reserved.

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

#define GET_CHAR()        if (this->get_char() <= -1) return -1;
#define GET_TOKEN()       if (this->get_token() <= -1) return -1;
#define PUT_CHAR(x)       if (this->put_char(x) <= -1) return -1;

QSE_BEGIN_NAMESPACE(QSE)

Sttp::Sttp (Transmittable* s, Mmgr* mmgr) QSE_CPP_NOEXCEPT: Mmged(mmgr), p_medium(s)
{
	this->reset ();
}

Sttp::~Sttp () QSE_CPP_NOEXCEPT
{
}

void Sttp::reset () QSE_CPP_NOEXCEPT
{
	this->inbuf_len       = 0;
	this->outbuf_len      = 0;
	this->sttp_curp       = 0;
	this->sttp_curc       = QSE_T('\0');

	this->max_raw_cmd_len  = MAX_RAW_CMD_LEN;
	this->max_arg_count    = MAX_ARG_COUNT;
	this->raw_cmd_len      = 0;

	this->opt_send_newline = true;
}

int Sttp::receiveCmd (SttpCmd* cmd) QSE_CPP_NOEXCEPT
{
	QSE_ASSERT (p_medium != QSE_NULL);

	this->p_errcode = E_NOERR;
	this->raw_cmd_len = 0;

	GET_CHAR ();
	GET_TOKEN();

	/*
	if (this->token_type == T_SEMICOLON) { 
		// null command
		cmd->clear ();
		return 0;
	}
	*/

	if (this->token_type == T_EOF) return 0; // no more command. end of input

	if (this->token_type != T_IDENT) 
	{
		this->p_errcode = E_CMDNAME;
		return -1;
	}

	cmd->clear ();
	try { cmd->setName (token_value.getBuffer(), token_value.getSize()); } 
	catch (...) 
	{ 
		this->p_errcode= E_MEMORY;
		return -1;
	}

	while (1)
	{
		GET_TOKEN();
		switch (this->token_type)
		{
			case T_STRING:
				try { cmd->addArg (token_value.getBuffer(), token_value.getSize()); }
				catch (...)
				{
					this->p_errcode = E_MEMORY;
					return -1;
				}
				break;

			case T_IDENT:
				// you don't have to quote a string owing to this.
				try { cmd->addArg (token_value.getBuffer(), token_value.getSize()); }
				catch (...)
				{
					this->p_errcode = E_MEMORY;
					return -1;
				}
				break;

			default:
				if (cmd->getArgCount() == 0) break;
				this->p_errcode = E_WRONGARG;
				return -1;
		}

		GET_TOKEN();
		if (this->token_type != T_COMMA) break;
		if (cmd->getArgCount() >= this->max_arg_count) 
		{
			this->p_errcode = E_TOOMANYARGS;
			return -1;
		}
	}

	if (this->token_type != T_SEMICOLON) 
	{
		this->p_errcode = E_SEMICOLON;
		return -1;
	}

	return 1; // got a command
}

int Sttp::sendCmd (const SttpCmd& cmd) QSE_CPP_NOEXCEPT
{
	this->p_errcode = E_NOERR;

	const qse_char_t* p = (const qse_char_t*)cmd.name;
	if (*p == QSE_T('\0')) return 0; // don't send a null command

	while (*p != QSE_T('\0')) PUT_CHAR(*p++); 

	qse_size_t nargs = cmd.getArgCount();
	if (nargs > 0) 
	{
		PUT_CHAR (QSE_T(' '));

		for (qse_size_t i = 0; i < nargs; i++) 
		{
			const qse_char_t* arg = cmd.getArgAt(i);
			qse_size_t arg_len = cmd.getArgLenAt(i);

			PUT_CHAR (QSE_T('\"'));
			for (qse_size_t j = 0; j < arg_len; j++) 
			{
				// Don't have to send a backslash when encryption is on
				// because only 16 characters from 'A' TO 'P' are used
				if (arg[j] == QSE_T('\\') || 
				    arg[j] == QSE_T('\"')) PUT_CHAR ('\\');
				PUT_CHAR (arg[j]);
			}
			PUT_CHAR (QSE_T('\"'));

			if (i < nargs - 1) PUT_CHAR (QSE_T(','));
		}
	}

	PUT_CHAR (QSE_T(';'));
	if (this->opt_send_newline) 
	{
		PUT_CHAR (QSE_T('\r'));
		PUT_CHAR (QSE_T('\n'));
	}
	return this->flush_outbuf();
}


int Sttp::sendCmd (const qse_char_t* name, qse_size_t nargs = 0, ...) QSE_CPP_NOEXCEPT
{
	this->p_errcode = E_NOERR;

	const qse_char_t* p = name;
	if (*p == QSE_T('\0')) return 0; // don't send a null command

	while (*p != QSE_T('\0')) PUT_CHAR (*p++);

	if (nargs > 0) 
	{
		va_list ap;
		va_start (ap, nargs);

		PUT_CHAR (QSE_T(' '));
		for (qse_size_t i = 1; i <= nargs; i++) 
		{
			p = va_arg (ap, qse_char_t*);

			PUT_CHAR (QSE_T('\"'));
			while (*p) 
			{
				if (*p == QSE_T('\\') || *p == QSE_T('\"')) PUT_CHAR (QSE_T('\\'));
				PUT_CHAR (*p++); 
			}
			PUT_CHAR (QSE_T('\"'));
	
			if (i < nargs) PUT_CHAR (QSE_T(','));
		}
		va_end (ap);
	}

	PUT_CHAR (QSE_T(';'));
	if (this->opt_send_newline) 
	{
		PUT_CHAR (QSE_T('\r'));
		PUT_CHAR (QSE_T('\n'));
	}
	return this->flush_outbuf();
}

int Sttp::sendCmdL (const qse_char_t* name, qse_size_t nargs = 0, ...) QSE_CPP_NOEXCEPT
{
	this->p_errcode = E_NOERR;

	const qse_char_t* p = name;
	if (*p == QSE_T('\0')) return 0; // don't send a null command

	while (*p != QSE_T('\0')) PUT_CHAR (*p++);

	if (nargs > 0) 
	{
		va_list ap;
		va_start (ap, nargs);

		PUT_CHAR (QSE_T(' '));
		for (qse_size_t i = 1; i <= nargs; i++) 
		{
			p = va_arg (ap, qse_char_t*);
			qse_size_t len = va_arg (ap, qse_size_t);

			PUT_CHAR (QSE_T('\"'));
			while (len > 0) 
			{
				if (*p == QSE_T('\\') || *p == QSE_T('\"')) PUT_CHAR (QSE_T('\\'));
				PUT_CHAR (*p++); 
				len--;
			}
			PUT_CHAR (QSE_T('\"'));

			if (i < nargs) PUT_CHAR (QSE_T(','));
		}
		va_end (ap);
	}

	PUT_CHAR (QSE_T(';'));
	if (this->opt_send_newline) 
	{
		PUT_CHAR (QSE_T('\r'));
		PUT_CHAR (QSE_T('\n'));
	}
	return this->flush_outbuf();
}

int Sttp::sendCmdL (const qse_char_t* name, qse_size_t nmlen, qse_size_t nargs = 0, ...) QSE_CPP_NOEXCEPT
{
	this->p_errcode = E_NOERR;

	qse_char_t* p = (qse_char_t*)name;
	if (*p == QSE_T('\0')) return 0; // don't send a null command

	//while (*p != QSE_T('\0')) PUT_CHAR (*p++);
	while (nmlen > 0) 
	{
		PUT_CHAR (*p++);
		nmlen--;
	}

	if (nargs > 0) 
	{
		va_list ap;

		va_start (ap, nargs);

		PUT_CHAR (QSE_T(' '));
		for (qse_size_t i = 1; i <= nargs; i++) 
		{
			p = va_arg (ap, qse_char_t*);
			qse_size_t len = va_arg (ap, qse_size_t);

			PUT_CHAR (QSE_T('\"'));
			while (len > 0) 
			{
				if (*p == QSE_T('\\') || *p == QSE_T('\"')) PUT_CHAR (QSE_T('\\'));
				PUT_CHAR (*p++); 
				len--;
			}
			PUT_CHAR (QSE_T('\"'));

			if (i < nargs) PUT_CHAR (QSE_T(','));
		}

		va_end (ap);
	}

	PUT_CHAR (QSE_T(';'));
	if (this->opt_send_newline) 
	{
		PUT_CHAR (QSE_T('\r'));
		PUT_CHAR (QSE_T('\n'));
	}
	return this->flush_outbuf();
}


int Sttp::get_char () QSE_CPP_NOEXCEPT
{
	qse_size_t remain = 0;

	if (this->sttp_curp == this->inbuf_len) 
	{
		qse_ssize_t n; 

		if (this->sttp_curc == QSE_CHAR_EOF)
		{
			/* called again after EOF is received. */
			this->p_errcode = E_RECEIVE;
			return -1;
		}

#if defined(QSE_CHAR_IS_WCHAR)
	get_char_utf8:
#endif
		n = this->p_medium->receive(&inbuf[remain], QSE_COUNTOF(inbuf) - remain);
		if (n <= -1) 
		{
			this->p_errcode = E_RECEIVE;
			return -1;
		}
		if (n == 0)
		{
			// no more input
// TODO: if EOF has been read already, raise an error

			this->sttp_curc = QSE_CHAR_EOF;
			return 0;
		}

		this->sttp_curp = 0;
		this->inbuf_len = (qse_size_t)n + remain;
	} 

#if defined(QSE_CHAR_IS_WCHAR)

	remain = this->inbuf_len - this->sttp_curp;

	qse_size_t seqlen = qse_utf8len(&this->inbuf[this->sttp_curp], remain);
	if (seqlen == 0) 
	{
		// invalid sequence
		this->sttp_curp++; // skip one byte
		this->p_errcode = E_UTF8_CONV;
		return -1;
	}

	if (remain < seqlen) 
	{
		// incomplete sequence. must read further...
		qse_memcpy (this->inbuf, &this->inbuf[this->sttp_curp], remain);
		this->sttp_curp = 0;
		this->inbuf_len = remain;
		goto get_char_utf8;
	}

	qse_wchar_t wch;
	qse_size_t n = qse_utf8touc(&this->inbuf[this->sttp_curp], seqlen, &wch);
	if (n == 0) 
	{
		// this part is not likely to be reached for qse_utf8len() above.
		// but keep it for completeness
		this->sttp_curp++; // still must skip a character
		this->p_errcode = E_UTF8_CONV;
		return -1;
	}

	this->sttp_curc = wch;
	this->sttp_curp += n;
#else
	this->sttp_curc = this->inbuf[this->sttp_curp++];
#endif

	/*
	if (sttp_curc == QSE_T('\0')) {
		this->p_errcode = E_WRONGCHAR;
		return -1;
	}
	*/

	if (raw_cmd_len >= max_raw_cmd_len) 
	{
		this->p_errcode = E_TOOLONGCMD;
		return -1;
	}
	raw_cmd_len++;

	return 0;
}

int Sttp::get_token () QSE_CPP_NOEXCEPT
{
	while (QSE_ISSPACE(this->sttp_curc)) GET_CHAR (); // skip spaces...

	if (is_ident_char(this->sttp_curc)) return get_ident ();
	else if (this->sttp_curc == QSE_T('\"') || this->sttp_curc == QSE_T('\'')) 
	{
		return get_string (sttp_curc);
	}
	else if (this->sttp_curc == QSE_T(';')) 
	{
		this->token_type = T_SEMICOLON;
		this->token_value = QSE_T(';');
		// do not read the next character to terminate a command
		// get_char ();
	}
	else if (this->sttp_curc == QSE_T(',')) 
	{
		this->token_type  = T_COMMA;
		this->token_value = QSE_T(',');
		GET_CHAR ();
	}
	else 
	{
		this->p_errcode = E_WRONGCHAR;
		return -1;
	}

	return 0;
}

int Sttp::get_ident () QSE_CPP_NOEXCEPT
{
	this->token_type  = T_IDENT;
	this->token_value = QSE_T("");

	while (is_ident_char(this->sttp_curc)) 
	{
		this->token_value.append (this->sttp_curc);
		GET_CHAR ();
	}

	return 0;
}

int Sttp::get_string (qse_char_t end) QSE_CPP_NOEXCEPT
{
	bool escaped = false;

	this->token_type  = T_STRING;
	this->token_value = QSE_T("");

	GET_CHAR ();
	while (1)
	{
		if (escaped == true) 
		{
			this->sttp_curc = this->translate_escaped_char(this->sttp_curc);
			escaped = false;
		}
		else 
		{
			if (this->sttp_curc == end) 
			{
				GET_CHAR ();
				break;
			}
			else if (this->sttp_curc == QSE_T('\\')) 
			{
				GET_CHAR ();
				escaped = true;
				continue;
			}
		}

		this->token_value.append (this->sttp_curc);
		GET_CHAR ();
	}

	return 0;
}

qse_cint_t Sttp::translate_escaped_char (qse_cint_t c) QSE_CPP_NOEXCEPT
{
	if (c == QSE_T('n')) c = QSE_T('\n');
	else if (c == QSE_T('t')) c = QSE_T('\t');
	else if (c == QSE_T('r')) c = QSE_T('\r');
	else if (c == QSE_T('v')) c = QSE_T('\v');
	else if (c == QSE_T('f')) c = QSE_T('\f');
	else if (c == QSE_T('a')) c = QSE_T('\a');
	else if (c == QSE_T('b')) c = QSE_T('\b');
	//else if (c == QSE_T('0')) c = QSE_T('\0');

	return c;
}

bool Sttp::is_ident_char (qse_cint_t c) QSE_CPP_NOEXCEPT
{
	return QSE_ISALNUM(c) || c == QSE_T('_') || c == QSE_T('.') || c == QSE_T('*') || c == QSE_T('@');
}

/////////////////////////////////////////////////////////////////////////

int Sttp::put_mchar (qse_mchar_t ch) QSE_CPP_NOEXCEPT
{
	this->outbuf[outbuf_len++] = ch;
	if (this->outbuf_len >= QSE_COUNTOF(outbuf)) return this->flush_outbuf();
	return 0;
}

int Sttp::put_wchar (qse_wchar_t ch) QSE_CPP_NOEXCEPT
{
	qse_mchar_t buf[QSE_UTF8LEN_MAX];
	qse_size_t len = qse_uctoutf8(ch, buf, QSE_COUNTOF(buf));
	if (len == 0 || len > QSE_COUNTOF(buf)) 
	{
		this->p_errcode = E_UTF8_CONV;
		return -1;
	}

	for (qse_size_t i = 0; i < len; i++) 
	{
		if (this->put_mchar(buf[i]) == -1) return -1;
	}
	return 0;
}


int Sttp::flush_outbuf () QSE_CPP_NOEXCEPT
{
	if (this->outbuf_len > 0) 
	{
		qse_size_t pos = 0;

		do
		{
			qse_ssize_t n = this->p_medium->send(&this->outbuf[pos], this->outbuf_len);
			if (n <= -1)
			{
				if (pos > 0) QSE_MEMCPY (&this->outbuf[0], &this->outbuf[pos], this->outbuf_len * QSE_SIZEOF(this->outbuf[0]));
				this->p_errcode = E_SEND;
				return -1;
			}

			this->outbuf_len -= n;
			pos += n;
		}
		while (this->outbuf_len > 0);
		return 1;
	}

	return 0;
}


const qse_char_t* Sttp::getErrorStr () const QSE_CPP_NOEXCEPT
{
	switch (this->p_errcode) 
	{
		case E_NOERR:
			return QSE_T("no error");
		case E_MEMORY:
			return QSE_T("memory exhausted");
		case E_RECEIVE:
			return QSE_T("failed receive over medium");
		case E_SEND:
			return QSE_T("failed send over medium");
		case E_UTF8_CONV:
			return QSE_T("utf8 conversion failure");
		case E_CMDNAME: 
			return QSE_T("command name expected");
		case E_CMDPROC: 
			return QSE_T("command procedure exit");
		case E_UNKNOWNCMD: 
			return QSE_T("unknown command");
		case E_TOOLONGCMD: 
			return QSE_T("command too long");
		case E_SEMICOLON: 
			return QSE_T("semicolon expected");
		case E_TOOMANYARGS: 
			return QSE_T("too many command arguments");
		case E_WRONGARG: 
			return QSE_T("wrong command argument");
		case E_WRONGCHAR: 
			return QSE_T("wrong character");
		default:
			return QSE_T("unknown error");
	}
}

QSE_END_NAMESPACE(QSE)
