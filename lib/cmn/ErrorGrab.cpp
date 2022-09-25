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

#define MSG_ENOERR  "no error"
#define MSG_EOTHER  "other error"
#define MSG_ENOIMPL "not implemented"
#define MSG_ESYSERR "subsystem error"
#define MSG_EINTERN "internal error"

#define MSG_ENOMEM  "insufficient memory"
#define MSG_ENARGS  "wrong number of arguments"
#define MSG_EINVAL "invalid parameter or data"
#define MSG_EACCES "access denied"
#define MSG_EPERM "operation not allowed"

#define MSG_ENOENT "no such data"
#define MSG_EEXIST "data exists"
#define MSG_ENOTDIR "not directory"
#define MSG_EINTR "interrupted"
#define MSG_EPIPE "pipe error"

#define MSG_EINPROG "in progress"
#define MSG_EAGAIN  "resource temporarily unavailable"
#define MSG_EEXCEPT "exception"

static const qse_mchar_t* _merrstr[] =
{
	MSG_ENOERR,
	MSG_EOTHER,
	MSG_ENOIMPL,
	MSG_ESYSERR,
	MSG_EINTERN,

	MSG_ENOMEM,
	MSG_ENARGS,
	MSG_EINVAL,
	MSG_EACCES,
	MSG_EPERM,

	MSG_ENOENT,
	MSG_EEXIST,
	MSG_ENOTDIR,
	MSG_EINTR,
	MSG_EPIPE,

	MSG_EINPROG,
	MSG_EAGAIN,
	MSG_EEXCEPT
};

static const qse_wchar_t* _werrstr[] =
{
	QSE_WT(MSG_ENOERR),
	QSE_WT(MSG_EOTHER),
	QSE_WT(MSG_ENOIMPL),
	QSE_WT(MSG_ESYSERR),
	QSE_WT(MSG_EINTERN),

	QSE_WT(MSG_ENOMEM),
	QSE_WT(MSG_ENARGS),
	QSE_WT(MSG_EINVAL),
	QSE_WT(MSG_EACCES),
	QSE_WT(MSG_EPERM),

	QSE_WT(MSG_ENOENT),
	QSE_WT(MSG_EEXIST),
	QSE_WT(MSG_ENOTDIR),
	QSE_WT(MSG_EINTR),
	QSE_WT(MSG_EPIPE),

	QSE_WT(MSG_EINPROG),
	QSE_WT(MSG_EAGAIN),
	QSE_WT(MSG_EEXCEPT)
};

const qse_wchar_t* TypesErrorNumberToWcstr::operator() (Types::ErrorNumber errnum)
{
	return errnum >= QSE_COUNTOF(_werrstr)? QSE_WT("unknown error"): _werrstr[errnum];
}

const qse_mchar_t* TypesErrorNumberToMbstr::operator() (Types::ErrorNumber errnum)
{
	return errnum >= QSE_COUNTOF(_merrstr)? QSE_MT("unknown error"): _merrstr[errnum];
}

QSE_END_NAMESPACE(QSE)
