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


#ifndef _QSE_CRY_SHA1_H_
#define _QSE_CRY_SHA1_H_

#define QSE_SHA1_DIGEST_LEN 20

#include <qse/types.h>
#include <qse/macros.h>

struct qse_sha1_t 
{
	qse_uint32_t state[5];
	qse_uint32_t count[2];
	qse_uint8_t  buffer[64];
};
typedef struct qse_sha1_t qse_sha1_t;

#ifdef __cplusplus
extern "C" {
#endif

QSE_EXPORT void qse_sha1_initialize (
	qse_sha1_t* ctx
);

QSE_EXPORT void qse_sha1_update (
	qse_sha1_t*  sha1, 
	const void*  data,
	qse_uint32_t len
);

QSE_EXPORT void qse_sha1_updatex (
	qse_sha1_t* sha1, 
	const void* data,
	qse_size_t  len
);

QSE_EXPORT qse_size_t qse_sha1_finalize (
	qse_sha1_t* sha1,
	void*       digest,
	qse_size_t  size
);

#ifdef __cplusplus
}
#endif

#endif /* SHA1_H */
