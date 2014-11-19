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

#ifndef _QSE_LIB_AWK_FNC_H_
#define _QSE_LIB_AWK_FNC_H_

struct qse_awk_fnc_t
{
	struct
	{
		qse_char_t* ptr;
		qse_size_t  len;
	} name;

	int dfl0; /* if set, ($0) is assumed if () is missing. 
	           * this ia mainly for the weird length() function */

	qse_awk_fnc_spec_t spec;
	const qse_char_t* owner; /* set this to a module name if a built-in function is located in a module */

	qse_awk_mod_t* mod; /* set by the engine to a valid pointer if it's associated to a module */
};

#if defined(__cplusplus)
extern "C" {
#endif

qse_awk_fnc_t* qse_awk_findfnc (qse_awk_t* awk, const qse_cstr_t* name);

/* EXPORT is required for linking on windows as they are referenced by mod-str.c */
QSE_EXPORT int qse_awk_fnc_index   (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
QSE_EXPORT int qse_awk_fnc_rindex  (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
QSE_EXPORT int qse_awk_fnc_length  (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
QSE_EXPORT int qse_awk_fnc_substr  (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
QSE_EXPORT int qse_awk_fnc_split   (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
QSE_EXPORT int qse_awk_fnc_match   (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
QSE_EXPORT int qse_awk_fnc_gsub    (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
QSE_EXPORT int qse_awk_fnc_sub     (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
QSE_EXPORT int qse_awk_fnc_tolower (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
QSE_EXPORT int qse_awk_fnc_toupper (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);
QSE_EXPORT int qse_awk_fnc_sprintf (qse_awk_rtx_t* rtx, const qse_awk_fnc_info_t* fi);

#if defined(__cplusplus)
}
#endif

#endif
