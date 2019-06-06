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

#ifndef _QSE_CRY_KSEED_H_
#define _QSE_CRY_KSEED_H_

#include <qse/types.h>
#include <qse/macros.h>

#define QSE_KSEED_NUM_S_BOXES 4
#define QSE_KSEED_NUM_ENTRIES 256

#define QSE_KSEED_MIN_KEY_LEN 16 /* bytes - 128 bits */
#define QSE_KSEED_MAX_KEY_LEN 16 /* bytes - 128 bits */
#define QSE_KSEED_BLOCK_SIZE  16 /* bytes */

struct qse_kseed_t
{
	qse_uint32_t KD[32];
};
typedef struct qse_kseed_t qse_kseed_t;

typedef qse_uint8_t (qse_kseed_block_t)[QSE_KSEED_BLOCK_SIZE];

#ifdef __cplusplus
extern "C" {
#endif


QSE_EXPORT void qse_kseed_initialize (
	qse_kseed_t*    ks,
	const void*     key,
	qse_size_t      len
);

QSE_EXPORT void qse_kseed_encrypt_block (
	qse_kseed_t*       ks,
	qse_kseed_block_t* blk
);

QSE_EXPORT void qse_kseed_decrypt_block (
	qse_kseed_t*       ks,
	qse_kseed_block_t* blk
);

/*
QSE_EXPORT void qse_kseed_encrypt (
	qse_kseed_t*    ks,
	void*           data,
	qse_size_t      len
);

QSE_EXPORT void qse_kseed_decrypt (
	qse_kseed_t*    ks,
	void*           data,
	qse_size_t      len
);
*/

#ifdef __cplusplus
}
#endif

#endif
