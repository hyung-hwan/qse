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

#ifndef _QSE_LIB_AWK_STD_H
#define _QSE_LIB_AWK_STD_H_

#include "awk.h"

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT qse_awk_flt_t qse_awk_stdmathpow (qse_awk_t* awk, qse_awk_flt_t x, qse_awk_flt_t y);
QSE_EXPORT qse_awk_flt_t qse_awk_stdmathmod (qse_awk_t* awk, qse_awk_flt_t x, qse_awk_flt_t y);

QSE_EXPORT int qse_awk_stdmodstartup (qse_awk_t* awk);
QSE_EXPORT void qse_awk_stdmodshutdown (qse_awk_t* awk);

QSE_EXPORT void* qse_awk_stdmodopen (qse_awk_t* awk, const qse_awk_mod_spec_t* spec);
QSE_EXPORT void qse_awk_stdmodclose (qse_awk_t* awk, void* handle);
QSE_EXPORT void* qse_awk_stdmodsym (qse_awk_t* awk, void* handle, const qse_char_t* name);

#if defined(__cplusplus)
}
#endif


#endif
