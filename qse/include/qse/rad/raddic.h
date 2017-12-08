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

#define QSE_RAD_ATTR_TYPE_STRING                  0
#define QSE_RAD_ATTR_TYPE_INTEGER                 1
#define QSE_RAD_ATTR_TYPE_IPADDR                  2
#define QSE_RAD_ATTR_TYPE_DATE                    3
#define QSE_RAD_ATTR_TYPE_ABINARY                 4
#define QSE_RAD_ATTR_TYPE_OCTETS                  5
#define QSE_RAD_ATTR_TYPE_IFID                    6
#define QSE_RAD_ATTR_TYPE_IPV6ADDR                7
#define QSE_RAD_ATTR_TYPE_IPV6PREFIX              8
#define QSE_RAD_ATTR_TYPE_BYTE                    9
#define QSE_RAD_ATTR_TYPE_SHORT                   10
#define QSE_RAD_ATTR_TYPE_ETHERNET                11
#define QSE_RAD_ATTR_TYPE_SIGNED                  12
#define QSE_RAD_ATTR_TYPE_COMBO_IP                13
#define QSE_RAD_ATTR_TYPE_TLV                     14

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

struct qse_raddic_attr_t 
{
	int                     attr;
	int                     type;
	int                     vendor;
	qse_raddic_attr_flags_t flags;
	qse_char_t              name[1];
};
typedef struct qse_raddic_attr_t qse_raddic_attr_t;


struct qse_raddic_value_t
{
	int                     attr;
	int                     value;
	qse_char_t              name[1];
};
typedef struct qse_raddic_value_t qse_raddic_value_t;

struct qse_raddic_vendor_t
{
	int         vendorpec;
	int         type;
	int         length;
	int         flags;
	qse_char_t  name[1];
};
typedef struct qse_raddic_vendor_t qse_raddic_vendor_t;

typedef struct qse_raddic_t qse_raddic_t;

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
	int               value
);

#if defined(__cplusplus)
}
#endif


#endif