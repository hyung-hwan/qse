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

#ifndef _QSE_CMN_UNI_H_
#define _QSE_CMN_UNI_H_


/** @file
 * This file provides functions, types, macros for unicode handling.
 */

#include <qse/types.h>
#include <qse/macros.h>


#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT int qse_isunitype (qse_wcint_t c, int type);
QSE_EXPORT int qse_isuniupper (qse_wcint_t c);
QSE_EXPORT int qse_isunilower (qse_wcint_t c);
QSE_EXPORT int qse_isunialpha (qse_wcint_t c);
QSE_EXPORT int qse_isunidigit (qse_wcint_t c);
QSE_EXPORT int qse_isunixdigit (qse_wcint_t c);
QSE_EXPORT int qse_isunialnum (qse_wcint_t c);
QSE_EXPORT int qse_isunispace (qse_wcint_t c);
QSE_EXPORT int qse_isuniprint (qse_wcint_t c);
QSE_EXPORT int qse_isunigraph (qse_wcint_t c);
QSE_EXPORT int qse_isunicntrl (qse_wcint_t c);
QSE_EXPORT int qse_isunipunct (qse_wcint_t c);
QSE_EXPORT qse_wcint_t qse_touniupper (qse_wcint_t c);
QSE_EXPORT qse_wcint_t qse_tounilower (qse_wcint_t c);

#if defined(__cplusplus)
}
#endif

#endif
