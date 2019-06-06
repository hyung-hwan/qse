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

#ifndef _QSE_CMN_TEST_H_
#define _QSE_CMN_TEST_H_

#include <qse/si/sio.h>

#define QSE_TESASSERT_FAIL1(msg1) qse_printf(QSE_T("FAILURE in %hs[%d] - %s\n"), __func__, (int)__LINE__, msg1)
#define QSE_TESASSERT_FAIL2(msg1,msg2) qse_printf(QSE_T("FAILURE in %hs[%d] - %s - %s\n"), __func__, (int)__LINE__, msg1, msg2)

#define QSE_TESASSERT1(test,msg1) do { if (!(test)) { QSE_TESASSERT_FAIL1(msg1); goto oops; } } while(0)
#define QSE_TESASSERT2(test,msg1,msg2) do { if (!(test)) { QSE_TESASSERT_FAIL2(msg1,msg2); goto oops; } } while(0)

#endif
