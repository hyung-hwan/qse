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

#include <qse/cmn/str.h>

/* ----------------------------------- */

#undef char_t
#undef cstr_t
#undef T
#undef NOBUF
#undef strlen
#undef scan_dollar
#undef expand_dollar
#undef subst_t
#undef strxsubst
#undef strxnsubst

#define char_t qse_mchar_t
#define cstr_t qse_mcstr_t
#define T(x) QSE_MT(x)
#define NOBUF QSE_MBSSUBST_NOBUF
#define strlen qse_mbslen
#define scan_dollar mbs_scan_dollar
#define expand_dollar mbs_expand_dollar
#define subst_t qse_mbssubst_t
#define strxsubst qse_mbsxsubst
#define strxnsubst qse_mbsxnsubst
#include "str-subst.h"

/* ----------------------------------- */

#undef char_t
#undef cstr_t
#undef T
#undef NOBUF
#undef strlen
#undef scan_dollar
#undef expand_dollar
#undef subst_t
#undef strxsubst
#undef strxnsubst

#define char_t qse_wchar_t
#define cstr_t qse_wcstr_t
#define T(x) QSE_WT(x)
#define NOBUF QSE_WCSSUBST_NOBUF
#define strlen qse_wcslen
#define scan_dollar wcs_scan_dollar
#define expand_dollar wcs_expand_dollar
#define subst_t qse_wcssubst_t
#define strxsubst qse_wcsxsubst
#define strxnsubst qse_wcsxnsubst
#include "str-subst.h"

