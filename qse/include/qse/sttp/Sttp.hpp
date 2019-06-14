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
#include <qse/cmn/Transmittable.hpp>
#include <qse/cmn/time.h>
#include <qse/sttp/SttpCmd.hpp>

QSE_BEGIN_NAMESPACE(QSE)

class Sttp: public Mmged, public Uncopyable
{
public:
	enum ErrorNumber
	{
		E_NOERR = 0,
		E_MEMORY,
		E_RECEIVE,
		E_SEND,
		E_UTF8_CONV,
		E_CMDNAME,
		E_CMDPROC,      // user cmd proc returned -1
		E_UNKNOWNCMD,   // unknown command received
		E_TOOLONGCMD,
		E_SEMICOLON,
		E_TOOMANYARGS,
		E_WRONGARG,
		E_WRONGCHAR
	};

	Sttp (Transmittable* s = QSE_NULL, Mmgr* mmgr = QSE_NULL) QSE_CPP_NOEXCEPT;
	~Sttp () QSE_CPP_NOEXCEPT;

	void reset () QSE_CPP_NOEXCEPT;

	qse_size_t getMaxRawCmdLen () const QSE_CPP_NOEXCEPT
	{
		return this->max_raw_cmd_len;
	}
	void setMaxRawCmdLen (qse_size_t v) QSE_CPP_NOEXCEPT
	{
		this->max_raw_cmd_len = v;
	}

	qse_size_t setMaxArgCount () const QSE_CPP_NOEXCEPT
	{
		return this->max_arg_count;
	}
	void setMaxArgCount (qse_size_t v) QSE_CPP_NOEXCEPT
	{
		this->max_arg_count = v;
	}

	bool getOptSendNewline () const QSE_CPP_NOEXCEPT
	{
		return this->opt_send_newline;
	}
	void setOptSendNewline (bool opt) QSE_CPP_NOEXCEPT
	{
		this->opt_send_newline = opt;
	}

	int getErrorNumber() const QSE_CPP_NOEXCEPT
	{
		return this->p_errcode;
	}

	// The receiveCmd() function reads a complete command and stores
	// it to the command object pointed to by \a cmd. 
	//
	// Upon failure, if the error code is #ERR_RECEIVE, you can check
	// the error code of the medium set to find more about the error.
	// See the following pseudo code.
	//
	// \code
	// Socket sck;
	// Sttp sttp(sck);
	// if (sttp->receiveCmd(&cmd) <= -1 && 
	//     sttp->getErrorNumber() == Sttp::E_RECEIVE &&
	//     sck->getErrorNumber() == Socket::E_EAGAIN) { ... }
	// \endcode
	//
	// \return 1 if a command is received. 0 if end of input is detected
	//         -1 if an error has occurred.
	int receiveCmd (SttpCmd* cmd) QSE_CPP_NOEXCEPT;

	int sendCmd  (const SttpCmd& cmd) QSE_CPP_NOEXCEPT;
	int sendCmd  (const qse_char_t* name, qse_size_t nargs, ...) QSE_CPP_NOEXCEPT;
	int sendCmdL (const qse_char_t* name, qse_size_t nargs, ...) QSE_CPP_NOEXCEPT;
	int sendCmdL (const qse_char_t* name, qse_size_t nmlen, qse_size_t nargs, ...) QSE_CPP_NOEXCEPT;

	const qse_char_t* getErrorStr () const QSE_CPP_NOEXCEPT;

	void setErrorNumber (int code) QSE_CPP_NOEXCEPT
	{
		this->p_errcode = code;
	}

protected:
	enum 
	{
		MAX_RAW_CMD_LEN  = 1024 * 1000,
		MAX_ARG_COUNT    = 20,
		MAX_INBUF_LEN    = 1024,
		MAX_OUTBUF_LEN   = 1024

	};

	enum TokenType 
	{
		T_EOF         = 1,
		T_STRING      = 2,
		T_IDENT       = 3,
		T_SEMICOLON   = 4,
		T_COMMA       = 5
	};

	Transmittable* p_medium;
	int p_errcode; /* ErrorNumber */

	qse_mchar_t inbuf [MAX_INBUF_LEN];
	qse_mchar_t outbuf[MAX_OUTBUF_LEN];
	qse_size_t  inbuf_len;
	qse_size_t  outbuf_len;
	qse_size_t  sttp_curp;
	qse_cint_t  sttp_curc;

	qse_size_t max_raw_cmd_len;
	qse_size_t max_arg_count;
	qse_size_t raw_cmd_len;
	bool opt_send_newline;

	TokenType token_type;
	QSE::String token_value;

	int get_char () QSE_CPP_NOEXCEPT;
	int get_token () QSE_CPP_NOEXCEPT;
	int get_ident () QSE_CPP_NOEXCEPT;
	int get_string (qse_char_t end) QSE_CPP_NOEXCEPT;
	qse_cint_t translate_escaped_char (qse_cint_t c) QSE_CPP_NOEXCEPT;
	bool is_ident_char (qse_cint_t c) QSE_CPP_NOEXCEPT;

	int put_mchar (qse_mchar_t ch) QSE_CPP_NOEXCEPT;
	int put_wchar (qse_wchar_t ch) QSE_CPP_NOEXCEPT;

	int put_char (qse_char_t ch) QSE_CPP_NOEXCEPT
	{
	#if defined(QSE_CHAR_IS_MCHAR)
		return this->put_mchar(ch);
	#else
		return this->put_wchar(ch);
	#endif
	}

	int flush_outbuf () QSE_CPP_NOEXCEPT;
};


#if 0
class SttpStdHandler
{
	int operator() (const SttpCmd& cmd)
	{
	}

};

template <class HANDLER>
class SttpX
{
	int exec()
	{
		if (this->receiveCmd(&cmd) <= -1)
		{
		}

		if (this->handler(&cmd) <= -1)
		{
		}
	}

protected:
	HANDLER handler;
};
#endif

QSE_END_NAMESPACE(QSE)

#endif
