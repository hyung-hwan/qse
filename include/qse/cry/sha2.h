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



/*
 * FILE:	sha2.h
 * AUTHOR:	Aaron D. Gifford - http://www.aarongifford.com/
 * 
 * Copyright (c) 2000-2001, Aaron D. Gifford
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTOR(S) ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTOR(S) BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: sha2.h,v 1.1 2001/11/08 00:02:01 adg Exp adg $
 */

#ifndef _QSE_CRY_SHA2_H_
#define _QSE_CRY_SHA2_H_

#include <qse/types.h>
#include <qse/macros.h>

#define QSE_SHA256_BLOCK_LEN   (64)
#define QSE_SHA256_DIGEST_LEN  (32)
#define QSE_SHA384_BLOCK_LEN   (128)
#define QSE_SHA384_DIGEST_LEN  (48)
#define QSE_SHA512_BLOCK_LEN   (128)
#define QSE_SHA512_DIGEST_LEN  (64)

struct qse_sha256_t 
{
	qse_uint32_t  state[8];
	qse_uint64_t  bitcount;
	qse_uint8_t   buffer[QSE_SHA256_BLOCK_LEN];
};
typedef struct qse_sha256_t qse_sha256_t;

struct qse_sha512_t 
{
	qse_uint64_t state[8];
	qse_uint64_t bitcount[2];
	qse_uint8_t  buffer[QSE_SHA512_BLOCK_LEN];
};

typedef struct qse_sha512_t qse_sha512_t;

typedef qse_sha512_t qse_sha384_t;

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT void qse_sha256_initialize (
	qse_sha256_t* ctx
);

QSE_EXPORT void qse_sha256_update (
	qse_sha256_t*      ctx,
	const qse_uint8_t* data,
	qse_size_t         len
);

#define qse_sha256_updatex qse_sha256_update

QSE_EXPORT qse_size_t qse_sha256_digest (
	qse_sha256_t* ctx,
	qse_uint8_t*  digest,
	qse_size_t    size
);

QSE_EXPORT void qse_sha384_initialize (
	qse_sha384_t* ctx
);

QSE_EXPORT void qse_sha384_update (
	qse_sha384_t*      ctx,
	const qse_uint8_t* data,
	qse_size_t         len
);

#define qse_sha384_updatex qse_sha384_update

QSE_EXPORT qse_size_t qse_sha384_digest (
	qse_sha384_t* ctx,
	qse_uint8_t*  digest,
	qse_size_t    size
);


QSE_EXPORT void qse_sha512_initialize (
	qse_sha512_t* ctx
);

QSE_EXPORT void qse_sha512_update (
	qse_sha512_t*      ctx,
	const qse_uint8_t* data,
	qse_size_t         len
);

#define qse_sha512_updatex qse_sha512_update

QSE_EXPORT qse_size_t qse_sha512_digest (
	qse_sha512_t* ctx,
	qse_uint8_t*  digest,
	qse_size_t    size
);

#if defined(__cplusplus)
}
#endif

#endif
