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

#ifndef _QSE_RAD_RADDIC_H_
#define _QSE_RAD_RADDIC_H_

#include <qse/types.h>
#include <qse/macros.h>

enum qse_raddic_opt_t
{
	QSE_RADDIC_TRAIT
};
typedef enum qse_raddic_opt_t qse_raddic_opt_t;

enum qse_raddic_trait_t
{
	QSE_RADDIC_ALLOW_CONST_WITHOUT_ATTR = (1 << 0),
	QSE_RADDIC_ALLOW_DUPLICATE_CONST    = (1 << 1)
};
typedef enum qse_raddic_trait_t qse_raddic_trait_t;

enum qse_raddic_errnum_t
{
	QSE_RADDIC_ENOERR,
	QSE_RADDIC_EOTHER,
	QSE_RADDIC_ENOIMPL,
	QSE_RADDIC_ESYSERR,
	QSE_RADDIC_EINTERN,
	QSE_RADDIC_ENOMEM,
	QSE_RADDIC_EINVAL,
	QSE_RADDIC_ENOENT,
	QSE_RADDIC_EEXIST,
	QSE_RADDIC_ESYNERR
};
typedef enum qse_raddic_errnum_t qse_raddic_errnum_t;


#if 0
#define QSE_RADDIC_ATTR_TYPE_STRING                  0
#define QSE_RADDIC_ATTR_TYPE_INTEGER                 1
#define QSE_RADDIC_ATTR_TYPE_IPADDR                  2
#define QSE_RADDIC_ATTR_TYPE_DATE                    3
#define QSE_RADDIC_ATTR_TYPE_ABINARY                 4
#define QSE_RADDIC_ATTR_TYPE_OCTETS                  5
#define QSE_RADDIC_ATTR_TYPE_IFID                    6
#define QSE_RADDIC_ATTR_TYPE_IPV6ADDR                7
#define QSE_RADDIC_ATTR_TYPE_IPV6PREFIX              8
#define QSE_RADDIC_ATTR_TYPE_BYTE                    9
#define QSE_RADDIC_ATTR_TYPE_SHORT                   10
#define QSE_RADDIC_ATTR_TYPE_ETHERNET                11
#define QSE_RADDIC_ATTR_TYPE_SIGNED                  12
#define QSE_RADDIC_ATTR_TYPE_COMBO_IP                13
#define QSE_RADDIC_ATTR_TYPE_TLV                     14
#endif

enum qse_raddic_attr_type_t
{
	QSE_RADDIC_ATTR_TYPE_INVALID = 0,

	QSE_RADDIC_ATTR_TYPE_STRING,
	QSE_RADDIC_ATTR_TYPE_OCTETS,

	QSE_RADDIC_ATTR_TYPE_IPV4_ADDR,
	QSE_RADDIC_ATTR_TYPE_IPV4_PREFIX,
	QSE_RADDIC_ATTR_TYPE_IPV6_ADDR,
	QSE_RADDIC_ATTR_TYPE_IPV6_PREFIX,
	QSE_RADDIC_ATTR_TYPE_IFID,
	QSE_RADDIC_ATTR_TYPE_COMBO_IP_ADDR,
	QSE_RADDIC_ATTR_TYPE_COMBO_IP_PREFIX,
	QSE_RADDIC_ATTR_TYPE_ETHERNET,

	QSE_RADDIC_ATTR_TYPE_BOOL,

	QSE_RADDIC_ATTR_TYPE_UINT8,
	QSE_RADDIC_ATTR_TYPE_UINT16,
	QSE_RADDIC_ATTR_TYPE_UINT32,
	QSE_RADDIC_ATTR_TYPE_UINT64,

	QSE_RADDIC_ATTR_TYPE_INT8,
	QSE_RADDIC_ATTR_TYPE_INT16,
	QSE_RADDIC_ATTR_TYPE_INT32,
	QSE_RADDIC_ATTR_TYPE_INT64,

	QSE_RADDIC_ATTR_TYPE_FLOAT32,
	QSE_RADDIC_ATTR_TYPE_FLOAT64,

	QSE_RADDIC_ATTR_TYPE_TIMEVAL,
	QSE_RADDIC_ATTR_TYPE_DATE,
	QSE_RADDIC_ATTR_TYPE_DATE_MILLISECONDS,
	QSE_RADDIC_ATTR_TYPE_DATE_MICROSECONDS,
	QSE_RADDIC_ATTR_TYPE_DATE_NANOSECONDS,

	QSE_RADDIC_ATTR_TYPE_ABINARY,

	QSE_RADDIC_ATTR_TYPE_SIZE,

	QSE_RADDIC_ATTR_TYPE_TLV,
	QSE_RADDIC_ATTR_TYPE_STRUCT,

	QSE_RADDIC_ATTR_TYPE_EXTENDED,
	QSE_RADDIC_ATTR_TYPE_LONG_EXTENDED,

	QSE_RADDIC_ATTR_TYPE_VSA,
	QSE_RADDIC_ATTR_TYPE_EVS,
	QSE_RADDIC_ATTR_TYPE_VENDOR
};
typedef enum qse_raddic_attr_type_t qse_raddic_attr_type_t;

struct qse_raddic_attr_flags_t 
{
	unsigned int            addport : 1;  /* add NAS-Port to IP address */
	unsigned int            has_tag : 1;  /* tagged attribute */
	unsigned int            do_xlat : 1;  /* strvalue is dynamic */
	unsigned int            unknown_attr : 1; /* not in dictionary */
	unsigned int            array : 1; /* pack multiples into 1 attr */
	unsigned int            has_value : 1; /* has a value */
	unsigned int            has_value_alias : 1; /* has a value alias */
	unsigned int            has_tlv : 1; /* has sub attributes */
	unsigned int            is_tlv : 1; /* is a sub attribute */
	unsigned int            encoded : 1; /* has been put into packet */
	qse_int8_t              tag;          /* tag for tunneled attributes */
	qse_uint8_t             encrypt;      /* encryption method */
};
typedef struct qse_raddic_attr_flags_t qse_raddic_attr_flags_t;

typedef struct qse_raddic_attr_t qse_raddic_attr_t;
struct qse_raddic_attr_t 
{
	qse_uint32_t            attr;
	int                     type;
	int                     vendor;
	qse_raddic_attr_flags_t flags;
	qse_raddic_attr_t*      nexta;
	qse_char_t              name[1];
};

typedef struct qse_raddic_const_t qse_raddic_const_t;
struct qse_raddic_const_t
{
	qse_uint32_t        attr;     /* vendor + attribute-value */
	int                 value;
	qse_raddic_const_t* nextc;
	qse_char_t          name[1];
};

typedef struct qse_raddic_vendor_t qse_raddic_vendor_t;
struct qse_raddic_vendor_t
{
	int                  vendorpec;
	int                  type;
	int                  length;
	int                  flags;
	qse_raddic_vendor_t* nextv; /* next vendor with same details except the name */
	qse_char_t           name[1];
};

typedef struct qse_raddic_t qse_raddic_t;

#define QSE_RADDIC_ATTR_MAKE(vendor,value) ((((vendor) & 0xFFFFu) << 16) | (value))
#define QSE_RADDIC_ATTR_VENDOR(attr)       (((attr) >> 16) & 0xFFFFu)
#define QSE_RADDIC_ATTR_VALUE(attr)        ((attr) & 0xFFFFu)

#if defined(__cplusplus)
extern "C" {
#endif

QSE_EXPORT qse_raddic_t* qse_raddic_open (
	qse_mmgr_t*   mmgr,
	qse_size_t    xtnsize
);

QSE_EXPORT void qse_raddic_close (
	qse_raddic_t* dic
);

QSE_EXPORT int qse_raddic_getopt (
	qse_raddic_t*    raddic,
	qse_raddic_opt_t id,
	void*            value
);

QSE_EXPORT int qse_raddic_setopt (
	qse_raddic_t*    raddic,
	qse_raddic_opt_t id,
	const void*     value
);

QSE_EXPORT qse_raddic_errnum_t qse_raddic_geterrnum (
	qse_raddic_t* dic
);

QSE_EXPORT const qse_char_t* qse_raddic_geterrmsg (
	qse_raddic_t* dic
);

QSE_EXPORT void qse_raddic_seterrnum (
	qse_raddic_t*       dic,
	qse_raddic_errnum_t errnum
);

QSE_EXPORT void qse_raddic_seterrfmt (
	qse_raddic_t*        dic,
	qse_raddic_errnum_t  errnum,
	const qse_char_t*    fmt,
	...
);

QSE_EXPORT void qse_raddic_clear (
	qse_raddic_t* dic
);

QSE_EXPORT int qse_raddic_load (
	qse_raddic_t*     dic,
	const qse_char_t* path
);

QSE_EXPORT qse_raddic_vendor_t* qse_raddic_findvendorbyname (
	qse_raddic_t*     dic,
	const qse_char_t* name
);

QSE_EXPORT qse_raddic_vendor_t* qse_raddic_findvendorbyvalue (
	qse_raddic_t*     dic,
	int               vendorpec
);

QSE_EXPORT qse_raddic_vendor_t* qse_raddic_addvendor (
	qse_raddic_t*     dic,
	const qse_char_t* name,
	int               vendorpec
);

QSE_EXPORT int qse_raddic_deletevendorbyname (
	qse_raddic_t*     dic,
	const qse_char_t* name
);

QSE_EXPORT int qse_raddic_deletevendorbyvalue (
	qse_raddic_t*     dic,
	int               vendorpec
);




QSE_EXPORT qse_raddic_attr_t* qse_raddic_findattrbyname (
	qse_raddic_t*     dic,
	const qse_char_t* name
);

QSE_EXPORT qse_raddic_attr_t* qse_raddic_findattrbyvalue (
	qse_raddic_t*     dic,
	qse_uint32_t      attr
);

QSE_EXPORT qse_raddic_attr_t* qse_raddic_addattr (
	qse_raddic_t*                  dic,
	const qse_char_t*              name,
	int                            vendor,
	int                            type,
	int                            value,
	const qse_raddic_attr_flags_t* flags
);

QSE_EXPORT int qse_raddic_deleteattrbyname (
	qse_raddic_t*     dic,
	const qse_char_t* name
);

QSE_EXPORT int qse_raddic_deleteattrbyvalue (
	qse_raddic_t*     dic,
	int               attr
);


QSE_EXPORT qse_raddic_const_t* qse_raddic_findconstbyname (
	qse_raddic_t*     dic,
	qse_uint32_t      attr,
	const qse_char_t* name
);

QSE_EXPORT qse_raddic_const_t* qse_raddic_findconstbyvalue (
	qse_raddic_t*     dic,
	qse_uint32_t      attr,
	int               value
);

QSE_EXPORT qse_raddic_const_t* qse_raddic_addconst (
	qse_raddic_t*     dic,
	const qse_char_t* name,
	const qse_char_t* attrstr,
	int               value
);

QSE_EXPORT int qse_raddic_deleteconstbyname (
	qse_raddic_t*     dic,
	qse_uint32_t      attr,
	const qse_char_t* name
);

QSE_EXPORT int qse_raddic_deleteconstbyvalue (
	qse_raddic_t*     dic,
	qse_uint32_t      attr,
	int               value
);


#if defined(__cplusplus)
}
#endif


#endif
