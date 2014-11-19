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

#undef T
#undef char_t
#undef cstr_t
#undef strfcpy
#undef strfncpy
#undef strxfcpy
#undef strxfncpy

#define T(x) QSE_MT(x)
#define char_t qse_mchar_t
#define cstr_t qse_mcstr_t
#define strfcpy qse_mbsfcpy
#define strfncpy qse_mbsfncpy
#define strxfcpy qse_mbsxfcpy
#define strxfncpy qse_mbsxfncpy
#include "str-fcpy.h"

/* ----------------------------------- */

#undef T
#undef char_t
#undef cstr_t
#undef strfcpy
#undef strfncpy
#undef strxfcpy
#undef strxfncpy

#define T(x) QSE_WT(x)
#define char_t qse_wchar_t
#define cstr_t qse_wcstr_t
#define strfcpy qse_wcsfcpy
#define strfncpy qse_wcsfncpy
#define strxfcpy qse_wcsxfcpy
#define strxfncpy qse_wcsxfncpy
#include "str-fcpy.h"

