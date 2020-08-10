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


#ifndef _QSE_STTP_STTP_CLASS_
#define _QSE_STTP_STTP_CLASS_

#include <qse/Uncopyable.hpp>
#include <qse/cmn/Mmged.hpp>
#include <qse/cmn/ErrorGrab.hpp>
#include <qse/sttp/SttpCmd.hpp>
#include <stdarg.h>

QSE_BEGIN_NAMESPACE(QSE)

class QSE_EXPORT Sttp: public Mmged, public Uncopyable, public Types, public ErrorGrab64
{
public:
	Sttp (Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT;
	virtual ~Sttp ();

	void reset ();
	int feed (const qse_uint8_t* data, qse_size_t len, qse_size_t* rem);

	int beginWrite (const qse_mchar_t* cmd);
	int beginWrite (const qse_wchar_t* cmd);
	int writeWordArg (const qse_mchar_t* arg);
	int writeWordArg (const qse_wchar_t* arg);
	int writeStringArg (const qse_mchar_t* arg);
	int writeStringArg (const qse_mchar_t* arg, qse_size_t len);
	int writeStringArg (const qse_wchar_t* arg);
	int writeStringArg (const qse_wchar_t* arg, qse_size_t len);
	int endWrite ();

	int sendCmd (const qse_mchar_t* name, qse_size_t nargs, ...);
	int sendCmd (const qse_wchar_t* name, qse_size_t nargs, ...);
	int sendCmdL (const qse_mchar_t* name, qse_size_t nargs, ...);
	int sendCmdL (const qse_wchar_t* name, qse_size_t nargs, ...);

	int sendCmdV (const qse_mchar_t* name, qse_size_t nargs, va_list ap);
	int sendCmdV (const qse_wchar_t* name, qse_size_t nargs, va_list ap);
	// TODO: sendCmdLV

	// ------------------------------------------------------------------

	virtual int handle_command (const SttpCmd& cmd)
	{
		// it's the subclssases' responsibility to implement this
		return 0;
	}

	virtual int write_bytes (const qse_uint8_t* data, qse_size_t len)
	{
		// it's the subclssases' responsibility to implement this
		return 0;
	}

	// ------------------------------------------------------------------

private:
	enum rd_state_t
	{
		STATE_START,
		STATE_IN_NAME,
		STATE_IN_PARAM_LIST,
		STATE_IN_PARAM_WORD,
		STATE_IN_PARAM_STRING,
	};

	struct rd_state_node_t
	{
		rd_state_t state;
		union
		{
			struct
			{
				bool got_value;
			} ipl; /* in parameter list */
			struct
			{
				int escaped;
				int digit_count;
				qse_wchar_t acc;
				qse_char_t qc;
			} ps; /* parameter string */
		} u;
		rd_state_node_t* next;
	};

	rd_state_node_t rd_rd_state_top;
	rd_state_node_t* rd_state_stack;
	SttpCmd command;
	QSE::String token;
	qse_uint8_t rd_lo[QSE_MBLEN_MAX];
	qse_size_t rd_lo_len;

	qse_uint8_t wr_buf[4096];
	qse_size_t wr_buf_len;
	int wr_arg_count;

	int feed_chunk (const qse_uint8_t* data, qse_size_t len, qse_size_t* xlen);
	int handle_char (qse_char_t c);
	int handle_start_char (qse_char_t c);
	int handle_name_char (qse_char_t c);
	int handle_param_list_char (qse_char_t c);
	int handle_param_word_char (qse_char_t c);
	int handle_param_string_char (qse_char_t c);

	bool is_space_char (qse_char_t c)
	{
		return QSE_ISSPACE(c);
	}

	bool is_ident_char (qse_char_t c)
	{
		return QSE_ISALNUM(c) || c == QSE_T('_') || c == QSE_T('-') || c == QSE_T('.') || c == QSE_T('*') || c == QSE_T('@');
	}

	void add_char_to_token (qse_char_t c)
	{
		this->token.append (c);
	}

	void add_chars_to_token (const qse_char_t* ptr, qse_size_t len)
	{
		this->token.append (ptr, len);
	}

	void clear_token ()
	{
		this->token.clear ();
	}

	int push_read_state (rd_state_t state);
	void pop_read_state ();
	void pop_all_read_states ();


	int write_char (qse_wchar_t c);
	int write_char (qse_mchar_t c);
};

QSE_END_NAMESPACE(QSE)

#endif
