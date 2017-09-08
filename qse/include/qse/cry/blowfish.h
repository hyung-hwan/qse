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

#ifndef _QSE_CRY_BLOWFISH_H_
#define _QSE_CRY_BLOWFISH_H_

#include <qse/types.h>
#include <qse/macros.h>

#define QSE_BLOWFISH_NUM_SUBKEYS 18
#define QSE_BLOWFISH_NUM_S_BOXES 4
#define QSE_BLOWFISH_NUM_ENTRIES 256

#define QSE_BLOWFISH_MIN_KEY_LEN 4  /* bytes - 32 bits */
#define QSE_BLOWFISH_MAX_KEY_LEN 56 /* bytes - 448 bits */
#define QSE_BLOWFISH_BLOCK_SIZE  8  /* bytes */

struct qse_blowfish_t
{
	qse_uint32_t PA[QSE_BLOWFISH_NUM_SUBKEYS];
	qse_uint32_t SB[QSE_BLOWFISH_NUM_S_BOXES][QSE_BLOWFISH_NUM_ENTRIES];
};
typedef struct qse_blowfish_t qse_blowfish_t;


typedef qse_uint8_t (qse_blowfish_block_t)[QSE_BLOWFISH_BLOCK_SIZE];

#ifdef __cplusplus
extern "C" {
#endif


QSE_EXPORT void qse_blowfish_initialize (
	qse_blowfish_t* bf,
	const void*     keyptr,
	qse_size_t      keylen
);

QSE_EXPORT void qse_blowfish_encrypt_block (
	qse_blowfish_t*       bf,
	qse_blowfish_block_t* blk
);

QSE_EXPORT void qse_blowfish_decrypt_block (
	qse_blowfish_t*       bf,
	qse_blowfish_block_t* blk
);

/*
QSE_EXPORT void qse_blowfish_encrypt (
	qse_blowfish_t* bf,
	void*           data,
	qse_size_t      len
);

QSE_EXPORT void qse_blowfish_decrypt (
	qse_blowfish_t* bf,
	void*           data,
	qse_size_t      len
);
*/

#ifdef __cplusplus
}
#endif

#endif
