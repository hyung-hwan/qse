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


#ifndef _QSE_EXCEPTION_HPP_
#define _QSE_EXCEPTION_HPP_

/// \file
/// Provides the Exception class.

#include <qse/types.h>
#include <qse/macros.h>

/////////////////////////////////
QSE_BEGIN_NAMESPACE(QSE)
/////////////////////////////////

/// The Exception class implements the exception object.
class QSE_EXPORT Exception
{
public:
	Exception (
		const qse_char_t* name, const qse_char_t* msg, 
		const qse_char_t* file, qse_size_t line): 
		name(name), msg(msg)
#if !defined(QSE_NO_LOCATION_IN_EXCEPTION)
		, file(file), line(line) 
#endif
	{
	}

	const qse_char_t* name;
	const qse_char_t* msg;
#if !defined(QSE_NO_LOCATION_IN_EXCEPTION)
	const qse_char_t* file;
	qse_size_t        line;
#endif
};

#define QSE_THROW(ex_name) \
	throw ex_name(QSE_Q(ex_name),QSE_Q(ex_name), QSE_T(__FILE__), (qse_size_t)__LINE__)
#define QSE_THROW_WITH_MSG(ex_name,msg) \
	throw ex_name(QSE_Q(ex_name),msg, QSE_T(__FILE__), (qse_size_t)__LINE__)

#define QSE_EXCEPTION(ex_name) \
	class ex_name: public QSE::Exception \
	{ \
	public: \
		ex_name (const qse_char_t* name, const qse_char_t* msg, \
		         const qse_char_t* file, qse_size_t line): \
			QSE::Exception (name, msg, file, line) {} \
	}

#define QSE_EXCEPTION_NAME(exception_object) ((exception_object).name)
#define QSE_EXCEPTION_MSG(exception_object)  ((exception_object).msg)

#if !defined(QSE_NO_LOCATION_IN_EXCEPTION)
#	define QSE_EXCEPTION_FILE(exception_object) ((exception_object).file)
#	define QSE_EXCEPTION_LINE(exception_object) ((exception_object).line)
#else
#	define QSE_EXCEPTION_FILE(exception_object) (QSE_T(""))
#	define QSE_EXCEPTION_LINE(exception_object) (0)
#endif

/////////////////////////////////
QSE_END_NAMESPACE(QSE)
/////////////////////////////////

#endif
