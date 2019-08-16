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
 
#ifndef _QSE_CRY_HMAC_H_
#define _QSE_CRY_HMAC_H_

#include <qse/cry/md5.h>
#include <qse/cry/sha1.h>
#include <qse/cry/sha2.h>

#define QSE_HMAC_MAX_DIGEST_LEN QSE_SHA512_DIGEST_LEN
#define QSE_HMAC_MAX_BLOCK_LEN  QSE_SHA512_BLOCK_LEN

enum qse_hmac_sha_type_t
{
	QSE_HMAC_MD5,
	QSE_HMAC_SHA1,
	QSE_HMAC_SHA256,
	QSE_HMAC_SHA384,
	QSE_HMAC_SHA512
};
typedef enum qse_hmac_sha_type_t qse_hmac_sha_type_t;

union qse_hmac_sha_t
{
	qse_md5_t    md5;
	qse_sha1_t   sha1;
	qse_sha256_t sha256;
	qse_sha384_t sha384;
	qse_sha512_t sha512;
};
typedef union qse_hmac_sha_t qse_hmac_sha_t;

struct qse_hmac_t
{
	qse_hmac_sha_type_t sha_type;
	qse_size_t digest_size;
	qse_size_t block_size;
	qse_hmac_sha_t sha;
	qse_uint8_t k_opad[QSE_HMAC_MAX_BLOCK_LEN];
};
typedef struct qse_hmac_t qse_hmac_t;

#if defined(__cplusplus)
extern "C" {
#endif

void qse_hmac_initialize (
	qse_hmac_t*         ctx,
	qse_hmac_sha_type_t sha_type,
	const qse_uint8_t*  key,
	qse_size_t          key_len
);

void qse_hmac_update (
	qse_hmac_t*        ctx,
	const qse_uint8_t* data,
	qse_size_t         len
);

#define qse_hmac_upatex qse_hmac_upate

qse_size_t qse_hmac_digest (
	qse_hmac_t*  ctx,
	qse_uint8_t* digest,
	qse_size_t   size
);



qse_size_t qse_get_hmac_digest_size (
	qse_hmac_sha_type_t sha_type
);

qse_size_t qse_get_hmac_block_size (
	qse_hmac_sha_type_t sha_type
);



/* given an array of pointer and length pairs, it creates a string 
 * prefixed with hmac followed by the pair values encoded in hexdecimal
 * digits seperated by a dash 
 */
qse_mchar_t* qse_encode_hmacstr (
	qse_hmac_sha_type_t sha_type,
	const qse_uint8_t*  keyptr,
	qse_size_t          keylen,
	qse_xptl_t*         data,
	qse_size_t          count,
	qse_mmgr_t*         mmgr
);

qse_xptl_t* qse_decode_hmacstr (
	qse_hmac_sha_type_t sha_type,
	const qse_uint8_t*  keyptr,
	qse_size_t          keylen,
	const qse_mchar_t*  hmacstr,
	qse_size_t*         count,
	qse_mmgr_t*         mmgr
);


#if defined(__cplusplus)
}
#endif

#endif
