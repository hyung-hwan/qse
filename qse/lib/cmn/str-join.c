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
#include <stdarg.h>

/* ----------------------------------- */

#undef char_t
#undef strjoin
#undef strjoinv
#undef strxjoin
#undef strxjoinv
#undef strcpy
#undef strxcpy

#define char_t qse_mchar_t
#define strjoin qse_mbsjoin
#define strjoinv qse_mbsjoinv
#define strxjoin qse_mbsxjoin
#define strxjoinv qse_mbsxjoinv
#define strcpy qse_mbscpy
#define strxcpy qse_mbsxcpy
#include "str-join.h"

/* ----------------------------------- */

#undef char_t
#undef strjoin
#undef strjoinv
#undef strxjoin
#undef strxjoinv
#undef strcpy
#undef strxcpy

#define char_t qse_wchar_t
#define strjoin qse_wcsjoin
#define strjoinv qse_wcsjoinv
#define strxjoin qse_wcsxjoin
#define strxjoinv qse_wcsxjoinv
#define strcpy qse_wcscpy
#define strxcpy qse_wcsxcpy
#include "str-join.h"

#undef char_t
#undef strjoin
#undef strjoinv
#undef strxjoin
#undef strxjoinv
#undef strcpy
#undef strxcpy

#define char_t qse_char_t
#define strjoin qse_strjoin
#define strjoinv qse_strjoinv
#define strxjoin qse_strxjoin
#define strxjoinv qse_strxjoinv
#define strcpy qse_strcpy
#define strxcpy qse_strxcpy
#include "str-join.h"
