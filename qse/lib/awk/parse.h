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

#ifndef _QSE_LIB_AWK_PARSE_H_
#define _QSE_LIB_AWK_PARSE_H_

/* these enums should match kwtab in parse.c */
enum qse_awk_kwid_t
{
	QSE_AWK_KWID_XABORT,
	QSE_AWK_KWID_XGLOBAL,
	QSE_AWK_KWID_XINCLUDE,
	QSE_AWK_KWID_XLOCAL,
	QSE_AWK_KWID_XRESET,
	QSE_AWK_KWID_BEGIN,
	QSE_AWK_KWID_END,
	QSE_AWK_KWID_BREAK,
	QSE_AWK_KWID_CONTINUE,
	QSE_AWK_KWID_DELETE,
	QSE_AWK_KWID_DO,
	QSE_AWK_KWID_ELSE,
	QSE_AWK_KWID_EXIT,
	QSE_AWK_KWID_FOR,
	QSE_AWK_KWID_FUNCTION,
	QSE_AWK_KWID_GETLINE,
	QSE_AWK_KWID_IF,
	QSE_AWK_KWID_IN,
	QSE_AWK_KWID_NEXT,
	QSE_AWK_KWID_NEXTFILE,
	QSE_AWK_KWID_NEXTOFILE,
	QSE_AWK_KWID_PRINT,
	QSE_AWK_KWID_PRINTF,
	QSE_AWK_KWID_RETURN,
	QSE_AWK_KWID_WHILE
};

typedef enum qse_awk_kwid_t qse_awk_kwid_t;

#if defined(__cplusplus)
extern "C" {
#endif

int qse_awk_putsrcstr (
	qse_awk_t*        awk,
	const qse_char_t* str
);

int qse_awk_putsrcstrn (
	qse_awk_t*        awk,
	const qse_char_t* str,
	qse_size_t        len
);

const qse_char_t* qse_awk_getgblname (
	qse_awk_t*  awk,
	qse_size_t  idx,
	qse_size_t* len
);

void qse_awk_getkwname (
	qse_awk_t*     awk,
	qse_awk_kwid_t id, 
	qse_cstr_t*    s
);

int qse_awk_initgbls (
	qse_awk_t* awk
);

void qse_awk_clearsionames (
	qse_awk_t* awk
);

#if defined(__cplusplus)
}
#endif

#endif
