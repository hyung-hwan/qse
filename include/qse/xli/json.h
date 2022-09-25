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

#ifndef _QSE_XLI_JSON_H_
#define _QSE_XLI_JSON_H_

#include <qse/types.h>
#include <qse/macros.h>

/** 
 * The qse_json_t type defines a simple json parser.
 */
typedef struct qse_json_t qse_json_t;

#define QSE_JSON_HDR \
	qse_size_t _instsize; \
	qse_mmgr_t* _mmgr; \
	qse_cmgr_t* _cmgr

typedef struct qse_json_alt_t qse_json_alt_t;
struct qse_json_alt_t
{
	/* ensure that qse_json_t matches the beginning part of qse_json_t */
	QSE_JSON_HDR;
};

enum qse_json_errnum_t
{
	QSE_JSON_ENOERR = 0,
	QSE_JSON_EOTHER,
	QSE_JSON_ENOIMPL,
	QSE_JSON_ESYSERR,
	QSE_JSON_EINTERN,

	QSE_JSON_ENOMEM,
	QSE_JSON_EINVAL,
	QSE_JSON_EFINIS,
	QSE_JSON_EECERR
};
typedef enum qse_json_errnum_t qse_json_errnum_t;

enum qse_json_option_t
{
	QSE_JSON_TRAIT
};
typedef enum qse_json_option_t qse_json_option_t;

enum qse_json_trait_t
{
	/* no trait defined at this moment. XXXX is just a placeholder */
	QSE_JSON_XXXX  = (1 << 0)
};
typedef enum qse_json_trait_t qse_json_trait_t;

/* ========================================================================= */

enum qse_json_state_t
{
	QSE_JSON_STATE_START,
	QSE_JSON_STATE_IN_ARRAY,
	QSE_JSON_STATE_IN_DIC,

	QSE_JSON_STATE_IN_WORD_VALUE,
	QSE_JSON_STATE_IN_NUMERIC_VALUE,
	QSE_JSON_STATE_IN_STRING_VALUE,
	QSE_JSON_STATE_IN_CHARACTER_VALUE
};
typedef enum qse_json_state_t qse_json_state_t;


/* ========================================================================= */
enum qse_json_inst_t
{
	QSE_JSON_INST_START_ARRAY,
	QSE_JSON_INST_END_ARRAY,
	QSE_JSON_INST_START_DIC,
	QSE_JSON_INST_END_DIC,

	QSE_JSON_INST_KEY,

	QSE_JSON_INST_CHARACTER, /* there is no such element as character in real JSON */
	QSE_JSON_INST_STRING,
	QSE_JSON_INST_NUMBER,
	QSE_JSON_INST_NIL,
	QSE_JSON_INST_TRUE,
	QSE_JSON_INST_FALSE,
};
typedef enum qse_json_inst_t qse_json_inst_t;

typedef int (*qse_json_instcb_t) (
	qse_json_t*           json,
	qse_json_inst_t       inst,
	const qse_cstr_t*     str
);

struct qse_json_prim_t
{
	qse_json_instcb_t        instcb;
};
typedef struct qse_json_prim_t qse_json_prim_t;

/* ========================================================================= */

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT qse_json_t* qse_json_open (
	qse_mmgr_t*          mmgr,
	qse_size_t           xtnsize,
	qse_json_prim_t*     prim,
	qse_json_errnum_t*   errnum
);

QSE_EXPORT void qse_json_close (
	qse_json_t* json
);

QSE_EXPORT void qse_json_reset (
	qse_json_t* json
);

QSE_EXPORT int qse_json_feed (
	qse_json_t*    json,
	const void*    ptr,
	qse_size_t     len,
	qse_size_t*    xlen
);

QSE_EXPORT qse_json_state_t qse_json_getstate (
	qse_json_t* json
);

QSE_EXPORT int qse_json_setoption (
	qse_json_t*         json,
	qse_json_option_t   id,
	const void*         value
);

QSE_EXPORT int qse_json_getoption (
	qse_json_t*         json,
	qse_json_option_t   id,
	void*               value
);


#if defined(QSE_HAVE_INLINE)
static QSE_INLINE void* qse_json_getxtn (qse_json_t* json) { return (void*)((qse_uint8_t*)json + ((qse_json_alt_t*)json)->_instsize); }
static QSE_INLINE qse_mmgr_t* qse_json_getmmgr (qse_json_t* json) { return ((qse_json_alt_t*)json)->_mmgr; }
static QSE_INLINE qse_cmgr_t* qse_json_getcmgr (qse_json_t* json) { return ((qse_json_alt_t*)json)->_cmgr; }
static QSE_INLINE void qse_json_setcmgr (qse_json_t* json, qse_cmgr_t* cmgr) { ((qse_json_alt_t*)json)->_cmgr = cmgr; }
#else
#       define qse_json_getxtn(json) ((void*)((qse_uint8_t*)json + ((qse_json_alt_t*)json)->_instsize))
#	define qse_json_getmmgr(json) (((qse_json_alt_t*)(json))->_mmgr)
#	define qse_json_getcmgr(json) (((qse_json_alt_t*)(json))->_cmgr)
#	define qse_json_setcmgr(json,_cmgr) (((qse_json_alt_t*)(json))->_cmgr = (_cmgr))
#endif /* QSE_HAVE_INLINE */


QSE_EXPORT qse_json_errnum_t qse_json_geterrnum (
	qse_json_t* json
);

QSE_EXPORT const qse_char_t* qse_json_geterrmsg (
	qse_json_t* json
);

QSE_EXPORT const qse_char_t* qse_json_backuperrmsg (
	qse_json_t* json
);


QSE_EXPORT void qse_json_seterrnum (
	qse_json_t*   json,
	qse_json_errnum_t  errnum
);

QSE_EXPORT void qse_json_seterrfmt (
	qse_json_t*        json,
	qse_json_errnum_t  errnum,
	const qse_char_t*  fmt,
	...
);

QSE_EXPORT void* qse_json_allocmem (
	qse_json_t* json,
	qse_size_t     size
);

QSE_EXPORT void* qse_json_callocmem (
	qse_json_t* json,
	qse_size_t     size
);

QSE_EXPORT void* qse_json_reallocmem (
	qse_json_t* json,
	void*         ptr,
	qse_size_t     size
);

QSE_EXPORT void qse_json_freemem (
	qse_json_t* json,
	void*         ptr
);

#if defined(__cplusplus)
}
#endif

#endif
