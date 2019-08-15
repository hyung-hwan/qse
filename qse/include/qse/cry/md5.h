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

#ifndef _QSE_CRY_MD5_H_
#define _QSE_CRY_MD5_H_

#include <qse/types.h>
#include <qse/macros.h>

#define QSE_MD5_DIGEST_LEN (16)
#define QSE_MD5_BLOCK_LEN  (64)

struct qse_md5_t
{
	qse_uint32_t  count[2];
	qse_uint32_t  state[4];
	qse_uint8_t   buffer[QSE_MD5_BLOCK_LEN];
};
typedef struct qse_md5_t qse_md5_t;

#ifdef __cplusplus
extern "C" {
#endif

QSE_EXPORT void qse_md5_initialize (
	qse_md5_t* md5
);

QSE_EXPORT void qse_md5_update (
	qse_md5_t*   md5,
	const void*  data,
	qse_uint32_t len
);

QSE_EXPORT void qse_md5_updatex (
	qse_md5_t*   md5,
	const void*  data,
	qse_size_t   len
);

QSE_EXPORT qse_size_t qse_md5_digest (
	qse_md5_t* md5,
	void*      digest,
	qse_size_t size
);

#ifdef __cplusplus
}
#endif

#endif
