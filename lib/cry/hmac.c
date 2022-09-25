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
 this file is based on and heavily modified of 
 https://github.com/Yubico/yubikey-personalization/blob/master/hmac.c

Copyright (c) 2006-2013 Yubico AB
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <qse/cry/hmac.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/str.h>
#include "../cmn/mem-prv.h"

static inline qse_size_t sha_block_size (qse_hmac_sha_type_t sha_type)
{
	static qse_size_t block_size[] =
	{
		QSE_MD5_BLOCK_LEN,
		QSE_SHA1_BLOCK_LEN,
		QSE_SHA256_BLOCK_LEN,
		QSE_SHA384_BLOCK_LEN,
		QSE_SHA512_BLOCK_LEN
	};
	return block_size[sha_type];
}

static inline qse_size_t sha_digest_size (qse_hmac_sha_type_t sha_type)
{
	static qse_size_t digest_size[] =
	{
		QSE_MD5_DIGEST_LEN,
		QSE_SHA1_DIGEST_LEN,
		QSE_SHA256_DIGEST_LEN,
		QSE_SHA384_DIGEST_LEN,
		QSE_SHA512_DIGEST_LEN
	};
	return digest_size[sha_type];
}

static inline void sha_initialize (qse_hmac_sha_t* ctx, qse_hmac_sha_type_t sha_type)
{
	switch (sha_type)
	{
		case QSE_HMAC_MD5:
			qse_md5_initialize (&ctx->md5);
			break;
		case QSE_HMAC_SHA1:
			qse_sha1_initialize (&ctx->sha1);
			break;
		case QSE_HMAC_SHA256:
			qse_sha256_initialize (&ctx->sha256);
			break;
		case QSE_HMAC_SHA384:
			qse_sha384_initialize (&ctx->sha384);
			break;
		case QSE_HMAC_SHA512:
			qse_sha512_initialize (&ctx->sha512);
			break;
	}
}
static inline void sha_updatex (qse_hmac_sha_t* ctx, qse_hmac_sha_type_t sha_type, const qse_uint8_t* data, qse_size_t len)
{
	switch (sha_type)
	{
		case QSE_HMAC_MD5:
			qse_md5_updatex (&ctx->md5, data, len);
			break;
		case QSE_HMAC_SHA1:
			qse_sha1_updatex (&ctx->sha1, data, len);
			break;
		case QSE_HMAC_SHA256:
			qse_sha256_updatex (&ctx->sha256, data, len);
			break;
		case QSE_HMAC_SHA384:
			qse_sha384_updatex (&ctx->sha384, data, len);
			break;
		case QSE_HMAC_SHA512:
			qse_sha512_updatex (&ctx->sha512, data, len);
			break;
	}
}

static inline qse_size_t sha_digest (qse_hmac_sha_t* ctx, qse_hmac_sha_type_t sha_type, qse_uint8_t* digest, qse_size_t size)
{
	switch (sha_type)
	{
		case QSE_HMAC_MD5:
			return qse_md5_digest(&ctx->md5, digest, size);

		case QSE_HMAC_SHA1:
			return qse_sha1_digest(&ctx->sha1, digest, size);

		case QSE_HMAC_SHA256:
			return qse_sha256_digest(&ctx->sha256, digest, size);

		case QSE_HMAC_SHA384:
			return qse_sha384_digest(&ctx->sha384, digest, size);

		case QSE_HMAC_SHA512:
			return qse_sha512_digest(&ctx->sha512, digest, size);

	}

	/* this should not happen */
	return 0;
}


/******************** See RFC 4634 for details ******************/
/*
 *  Description:
 *      This file implements the HMAC algorithm (Keyed-Hashing for
 *      Message Authentication, RFC2104), expressed in terms of the
 *      various SHA algorithms.
 */

void qse_hmac_initialize (qse_hmac_t* ctx, qse_hmac_sha_type_t sha_type, const qse_uint8_t* key, qse_size_t key_len)
{
	qse_size_t i, block_size, digest_size;

	/* inner padding - key XORd with ipad */
	qse_uint8_t k_ipad[QSE_HMAC_MAX_BLOCK_LEN];

	/* temporary buffer when keylen > block_size */
	qse_uint8_t tempkey[QSE_HMAC_MAX_DIGEST_LEN];

	block_size = ctx->block_size = sha_block_size(sha_type);
	digest_size = ctx->digest_size = sha_digest_size(sha_type);

	ctx->sha_type = sha_type;

	/*
	* If key is longer than the hash block_size,
	* reset it to key = HASH(key).
	*/
	if (key_len > block_size)
	{
		qse_hmac_sha_t tctx;

		sha_initialize (&tctx, sha_type);
		sha_updatex (&tctx, sha_type, key, key_len);
		sha_digest (&tctx, sha_type, tempkey, QSE_SIZEOF(tempkey));

		key = tempkey;
		key_len = digest_size;
	}

	/*
	* The HMAC transform looks like:
	*
	* SHA(K XOR opad, SHA(K XOR ipad, text))
	*
	* where K is an n byte key.
	* ipad is the byte 0x36 repeated block_size times
	* opad is the byte 0x5c repeated block_size times
	* and text is the data being protected.
	*/

	/* store key into the pads, XOR'd with ipad and opad values */
	for (i = 0; i < key_len; i++)
	{
		k_ipad[i] = key[i] ^ 0x36;
		ctx->k_opad[i] = key[i] ^ 0x5c;
	}
	/* remaining pad bytes are '\0' XOR'd with ipad and opad values */
	for (; i < block_size; i++)
	{
		k_ipad[i] = 0x36;
		ctx->k_opad[i] = 0x5c;
	}

	/* perform inner hash */
	sha_initialize (&ctx->sha, sha_type);
	sha_updatex (&ctx->sha, sha_type, k_ipad, block_size); 
}

void qse_hmac_update (qse_hmac_t * ctx, const qse_uint8_t* data, qse_size_t len)
{
	sha_updatex (&ctx->sha, ctx->sha_type, data, len);
}

qse_size_t qse_hmac_digest (qse_hmac_t* ctx, qse_uint8_t* digest, qse_size_t size)
{
	qse_uint8_t tmp[QSE_HMAC_MAX_DIGEST_LEN];
	qse_size_t tmpsz;

	tmpsz = sha_digest(&ctx->sha, ctx->sha_type, tmp, QSE_SIZEOF(tmp));
	QSE_ASSERT (tmpsz == ctx->digest_size);

	/* outer SHA */
	sha_initialize (&ctx->sha, ctx->sha_type);
	sha_updatex (&ctx->sha, ctx->sha_type, ctx->k_opad, ctx->block_size);
	sha_updatex (&ctx->sha, ctx->sha_type, tmp, tmpsz);
	return sha_digest(&ctx->sha, ctx->sha_type, digest, size);
}


/* ------------------------------------------------------------------------ */

qse_mchar_t* qse_encode_hmacmbs (qse_hmac_sha_type_t sha_type, const qse_uint8_t* keyptr, qse_size_t keylen, qse_xptl_t* data, qse_size_t count, qse_mmgr_t* mmgr)
{
	qse_size_t reqsize = 0, i , j;
	qse_mchar_t* buf, * ptr;
	qse_xptl_t* dptr;
	qse_hmac_t hmac;
	qse_uint8_t digest[QSE_HMAC_MAX_DIGEST_LEN];
	qse_size_t digest_len = sha_digest_size(sha_type);

	for (i = 0, dptr = data; i < count; i++, dptr++) reqsize += 1 + (dptr->len * 2);
	reqsize += digest_len * 2 + 1;

	buf = (qse_mchar_t*)QSE_MMGR_ALLOC(mmgr, reqsize * QSE_SIZEOF(*buf));
	if (!buf) return QSE_NULL;

	ptr = &buf[digest_len * 2 + 1];
	for (i = 0, dptr = data; i < count; i++, dptr++)
	{
		for (j = 0; j < dptr->len; j++) 
		{
			ptr += qse_mbsfmt(ptr, QSE_MT("%02x"), *((qse_uint8_t*)dptr->ptr + j));
		}
		if (i < count - 1) *ptr++ = QSE_MT('-');
	}
	*ptr = '\0';

	qse_hmac_initialize (&hmac, sha_type, keyptr, keylen);
	qse_hmac_update (&hmac, (const qse_uint8_t*)&buf[digest_len * 2 + 1], ptr - &buf[digest_len * 2 + 1]);
	qse_hmac_digest (&hmac, digest, digest_len);

	ptr = buf;
	for (i = 0; i < digest_len; i++) ptr += qse_mbsfmt(ptr, QSE_MT("%02x"), digest[i]);
	*ptr = '-';

	return buf;
}


qse_xptl_t* qse_decode_hmacmbs (qse_hmac_sha_type_t sha_type, const qse_uint8_t* keyptr, qse_size_t keylen, const qse_mchar_t* hmacstr, qse_size_t* count, qse_mmgr_t* mmgr)
{
	qse_uint8_t digest[QSE_HMAC_MAX_DIGEST_LEN];
	qse_uint8_t orgdig[QSE_HMAC_MAX_DIGEST_LEN];
	qse_size_t digest_len = sha_digest_size(sha_type);
	const qse_mchar_t* ptr, * segptr, * segstart;
	qse_size_t seglen, reqlen = 0, segcount = 0, i;
	qse_uint8_t* data, * uptr;
	qse_xptl_t* xptl;
	qse_hmac_t hmac;

	for (ptr = hmacstr, i = 0; *ptr != QSE_MT('\0') && *ptr != QSE_MT('-'); ptr += 2, i++)
	{
		if (!QSE_ISMXDIGIT(ptr[0]) || !QSE_ISMXDIGIT(ptr[1])) return QSE_NULL;
		if (i >= digest_len) return QSE_NULL; /* digest too long */
		orgdig[i] = QSE_MXDIGITTONUM(ptr[0]) * 16 + QSE_MXDIGITTONUM(ptr[1]);
	}

	if (*ptr == QSE_MT('\0')) return QSE_NULL; /* no dash found after digest */
	if (i != digest_len) return QSE_NULL; /* digest too short */
	ptr++;
	qse_hmac_initialize (&hmac, sha_type, keyptr, keylen);
	qse_hmac_update (&hmac, (const qse_uint8_t*)ptr, qse_mbslen(ptr));
	qse_hmac_digest (&hmac, digest, digest_len);

	if (QSE_MEMCMP(digest, orgdig, digest_len) != 0) return QSE_NULL; /* wrong hmac */

	segstart = ptr;
	while (1)
	{
		segptr = ptr;

		for (ptr = segptr; *ptr != QSE_MT('\0') && *ptr != QSE_MT('-'); ptr++) ;
		seglen = ptr - segptr;

		if (seglen & 1) return QSE_NULL; /* odd-length segment */

		reqlen += (seglen >> 1) + QSE_SIZEOF(*xptl);
		segcount++;

		if (*ptr == QSE_MT('\0')) break;
		ptr++;
	}

	data = (qse_uint8_t*)QSE_MMGR_ALLOC(mmgr, reqlen);
	if (!data) return QSE_NULL;

	ptr = segstart;
	xptl = (qse_xptl_t*)data;
	uptr = data + (segcount * QSE_SIZEOF(*xptl)); 
	while (1)
	{
		segptr = ptr;

		xptl->ptr = uptr;
		for (ptr = segptr, i = 0; *ptr != QSE_MT('\0') && *ptr != QSE_MT('-'); ptr += 2, i++)
		{
			*uptr++ = QSE_MXDIGITTONUM(ptr[0]) * 16 + QSE_MXDIGITTONUM(ptr[1]);
		}
		xptl->len = i;
		xptl++;

		if (*ptr == QSE_MT('\0')) break;
		ptr++;
	}

	*count = segcount;
	return (qse_xptl_t*)data;
}

/* ------------------------------------------------------------------------ */


qse_wchar_t* qse_encode_hmacwcs (qse_hmac_sha_type_t sha_type, const qse_uint8_t* keyptr, qse_size_t keylen, qse_xptl_t* data, qse_size_t count, qse_mmgr_t* mmgr)
{
	qse_size_t reqsize = 0, i , j;
	qse_wchar_t* buf, * ptr;
	qse_xptl_t* dptr;
	qse_hmac_t hmac;
	qse_uint8_t digest[QSE_HMAC_MAX_DIGEST_LEN];
	qse_size_t digest_len = sha_digest_size(sha_type);

	for (i = 0, dptr = data; i < count; i++, dptr++) reqsize += 1 + (dptr->len * 2);
	reqsize += digest_len * 2 + 1;

	buf = (qse_wchar_t*)QSE_MMGR_ALLOC(mmgr, reqsize * QSE_SIZEOF(*buf));
	if (!buf) return QSE_NULL;

	ptr = &buf[digest_len * 2 + 1];
	for (i = 0, dptr = data; i < count; i++, dptr++)
	{
		for (j = 0; j < dptr->len; j++) 
		{
			ptr += qse_wcsfmt(ptr, QSE_WT("%02x"), *((qse_uint8_t*)dptr->ptr + j));
		}
		if (i < count - 1) *ptr++ = QSE_WT('-');
	}
	*ptr = '\0';

	qse_hmac_initialize (&hmac, sha_type, keyptr, keylen);
	qse_hmac_update (&hmac, (const qse_uint8_t*)&buf[digest_len * 2 + 1], ptr - &buf[digest_len * 2 + 1]);
	qse_hmac_digest (&hmac, digest, digest_len);

	ptr = buf;
	for (i = 0; i < digest_len; i++) ptr += qse_wcsfmt(ptr, QSE_WT("%02x"), digest[i]);
	*ptr = '-';

	return buf;
}


qse_xptl_t* qse_decode_hmacwcs (qse_hmac_sha_type_t sha_type, const qse_uint8_t* keyptr, qse_size_t keylen, const qse_wchar_t* hmacstr, qse_size_t* count, qse_mmgr_t* mmgr)
{
	qse_uint8_t digest[QSE_HMAC_MAX_DIGEST_LEN];
	qse_uint8_t orgdig[QSE_HMAC_MAX_DIGEST_LEN];
	qse_size_t digest_len = sha_digest_size(sha_type);
	const qse_wchar_t* ptr, * segptr, * segstart;
	qse_size_t seglen, reqlen = 0, segcount = 0, i;
	qse_uint8_t* data, * uptr;
	qse_xptl_t* xptl;
	qse_hmac_t hmac;

	for (ptr = hmacstr, i = 0; *ptr != QSE_WT('\0') && *ptr != QSE_WT('-'); ptr += 2, i++)
	{
		if (!QSE_ISWXDIGIT(ptr[0]) || !QSE_ISWXDIGIT(ptr[1])) return QSE_NULL;
		if (i >= digest_len) return QSE_NULL; /* digest too long */
		orgdig[i] = QSE_WXDIGITTONUM(ptr[0]) * 16 + QSE_WXDIGITTONUM(ptr[1]);
	}

	if (*ptr == QSE_WT('\0')) return QSE_NULL; /* no dash found after digest */
	if (i != digest_len) return QSE_NULL; /* digest too short */
	ptr++;
	qse_hmac_initialize (&hmac, sha_type, keyptr, keylen);
	qse_hmac_update (&hmac, (const qse_uint8_t*)ptr, qse_wcslen(ptr));
	qse_hmac_digest (&hmac, digest, digest_len);

	if (QSE_MEMCMP(digest, orgdig, digest_len) != 0) return QSE_NULL; /* wrong hmac */

	segstart = ptr;
	while (1)
	{
		segptr = ptr;

		for (ptr = segptr; *ptr != QSE_WT('\0') && *ptr != QSE_WT('-'); ptr++) ;
		seglen = ptr - segptr;

		if (seglen & 1) return QSE_NULL; /* odd-length segment */

		reqlen += (seglen >> 1) + QSE_SIZEOF(*xptl);
		segcount++;

		if (*ptr == QSE_WT('\0')) break;
		ptr++;
	}

	data = (qse_uint8_t*)QSE_MMGR_ALLOC(mmgr, reqlen);
	if (!data) return QSE_NULL;

	ptr = segstart;
	xptl = (qse_xptl_t*)data;
	uptr = data + (segcount * QSE_SIZEOF(*xptl)); 
	while (1)
	{
		segptr = ptr;

		xptl->ptr = uptr;
		for (ptr = segptr, i = 0; *ptr != QSE_WT('\0') && *ptr != QSE_WT('-'); ptr += 2, i++)
		{
			*uptr++ = QSE_WXDIGITTONUM(ptr[0]) * 16 + QSE_WXDIGITTONUM(ptr[1]);
		}
		xptl->len = i;
		xptl++;

		if (*ptr == QSE_WT('\0')) break;
		ptr++;
	}

	*count = segcount;
	return (qse_xptl_t*)data;
}
