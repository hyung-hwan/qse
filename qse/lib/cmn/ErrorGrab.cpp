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

#include <qse/cmn/ErrorGrab.hpp>

QSE_BEGIN_NAMESPACE(QSE)

static const qse_char_t* _errstr[] =
{
	QSE_T("no error"),
	QSE_T("other error"),
	QSE_T("not implemented"),
	QSE_T("subsystem error"),
	QSE_T("internal error"),

	QSE_T("insufficient memory"),
	QSE_T("wrong number of arguments"),
	QSE_T("invalid parameter or data"),
	QSE_T("access denied"),
	QSE_T("operation not allowed"),

	QSE_T("data not found"),
	QSE_T("existing data"),
	QSE_T("not directory"),
	QSE_T("interrupted"),
	QSE_T("pipe error"),

	QSE_T("in progress"),
	QSE_T("resource unavailable"),
	QSE_T("exception")
};

const qse_char_t* TypesErrorNumberToStr::operator() (Types::ErrorNumber errnum)
{
	return errnum >= QSE_COUNTOF(_errstr)? QSE_T("unknown error"): _errstr[errnum];
}

QSE_END_NAMESPACE(QSE)
