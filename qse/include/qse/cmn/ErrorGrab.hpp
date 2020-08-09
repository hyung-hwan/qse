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

#ifndef _QSE_CMN_ERRORGRAB_CLASS_
#define _QSE_CMN_ERRORGRAB_CLASS_

#include <stdarg.h>
#include <qse/Types.hpp>
#include <qse/cmn/str.h>

QSE_BEGIN_NAMESPACE(QSE)

template <typename ERRNUM, typename ERRNUMTOSTR, int MSGSZ>
class QSE_EXPORT ErrorGrab
{
public:
	ErrorGrab(): _errnum((ERRNUM)0)
	{
		this->_errmsg[0] = QSE_T('\0');
		this->_errmsg_backup[0] = QSE_T('\0');
	}

	ERRNUM getErrorNumber () const QSE_CPP_NOEXCEPT { return this->_errnum; }

	const qse_char_t* getErrorMsg () const QSE_CPP_NOEXCEPT { return this->_errmsg; }
	const qse_char_t* backupErrorMsg () QSE_CPP_NOEXCEPT  
	{
		qse_strcpy (this->_errmsg_backup, this->_errmsg);
		return this->_errmsg_backup;
	}

	void setErrorFmtv (ERRNUM errnum, const qse_char_t* fmt, va_list ap) QSE_CPP_NOEXCEPT
	{
		this->_errnum = errnum;
		qse_strxvfmt (this->_errmsg, QSE_COUNTOF(this->_errmsg), fmt, ap);
	}

	void setErrorFmt (ERRNUM errnum, const qse_char_t* fmt, ...) QSE_CPP_NOEXCEPT
	{
		va_list ap;
		va_start (ap, fmt);
		this->setErrorFmtv (errnum, fmt, ap);
		va_end (ap);
	}

	void setErrorNumber (ERRNUM errnum)
	{
		this->_errnum = errnum;
		qse_strxcpy (this->_errmsg, QSE_COUNTOF(this->_errmsg), this->_errtostr(errnum));
	}

private:
	ERRNUM _errnum;
	ERRNUMTOSTR _errtostr;
	qse_char_t _errmsg_backup[MSGSZ];
	qse_char_t _errmsg[MSGSZ];
};

// the stock functor class to convert the Types::ErrorNumber to a string
struct TypesErrorNumberToWcstr
{
	const qse_wchar_t* operator() (Types::ErrorNumber errnum);
};

struct TypesErrorNumberToMbstr
{
	const qse_mchar_t* operator() (Types::ErrorNumber errnum);
};

#if defined(QSE_CHAR_IS_MCHAR)
typedef TypesErrorNumberToMbstr TypesErrorNumberToStr;
#else
typedef TypesErrorNumberToWcstr TypesErrorNumberToStr;
#endif

typedef ErrorGrab<Types::ErrorNumber, TypesErrorNumberToStr, 64> ErrorGrab64;
typedef ErrorGrab<Types::ErrorNumber, TypesErrorNumberToStr, 128> ErrorGrab128;
typedef ErrorGrab<Types::ErrorNumber, TypesErrorNumberToStr, 256> ErrorGrab256;

QSE_END_NAMESPACE(QSE)

#endif
