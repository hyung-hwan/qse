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

#ifndef _QSE_RAD_RADMSG_H_
#define _QSE_RAD_RADMSG_H_

#include <qse/types.h>
#include <qse/macros.h>
#include <qse/cmn/ipad.h>

#define QSE_RAD_PACKET_MAX 65535

/* radius code */
enum qse_rad_code_t
{
	QSE_RAD_ACCESS_REQUEST      = 1,
	QSE_RAD_ACCESS_ACCEPT       = 2,
	QSE_RAD_ACCESS_REJECT       = 3,
	QSE_RAD_ACCOUNTING_REQUEST  = 4,
	QSE_RAD_ACCOUNTING_RESPONSE = 5,
	QSE_RAD_ACCESS_CHALLENGE    = 6,
	QSE_RAD_DISCONNECT_REQUEST  = 40,
	QSE_RAD_DISCONNECT_ACK      = 41,
	QSE_RAD_DISCONNECT_NAK      = 42,
	QSE_RAD_COA_REQUEST         = 43,
	QSE_RAD_COA_ACK             = 44,
	QSE_RAD_COA_NAK             = 45,

	QSE_RAD_ACCOUNTING_ERROR    = 255 /* this is not a real radius code */
};
typedef enum qse_rad_code_t qse_rad_code_t;

#define QSE_RAD_MAX_AUTHENTICATOR_LEN 16
#define QSE_RAD_MAX_ATTR_VALUE_LEN  (QSE_TYPE_MAX(qse_uint8_t) - QSE_SIZEOF(qse_rad_attr_hdr_t))
#define QSE_RAD_MAX_VSATTR_VALUE_LEN (QSE_TYPE_MAX(qse_uint8_t) - QSE_SIZEOF(qse_rad_attr_hdr_t) - QSE_SIZEOF(qse_rad_vsattr_hdr_t))


typedef struct qse_rad_hdr_t qse_rad_hdr_t;
typedef struct qse_rad_attr_hdr_t qse_rad_attr_hdr_t;
typedef struct qse_rad_vsattr_hdr_t qse_rad_vsattr_hdr_t;
typedef struct qse_rad_attr_int_t qse_rad_attr_int_t;

#include <qse/pack1.h>
struct qse_rad_hdr_t 
{
	qse_uint8_t   code; /* qse_rad_code_t */
	qse_uint8_t   id;
	qse_uint16_t  length;
	qse_uint8_t   authenticator[QSE_RAD_MAX_AUTHENTICATOR_LEN]; /* authenticator */
};

struct qse_rad_attr_hdr_t
{
	qse_uint8_t id; /* qse_rad_attr_id_t */
	qse_uint8_t length;
};

struct qse_rad_vsattr_hdr_t
{
	qse_uint8_t  id;
	qse_uint8_t  length;
	qse_uint32_t vendor; /* in network-byte order */
};

struct qse_rad_attr_int_t
{
	qse_rad_attr_hdr_t hdr;
	qse_uint32_t val;
};
#include <qse/unpack.h>


typedef int (*qse_rad_attr_walker_t) (
	const qse_rad_hdr_t*      hdr, 
	qse_uint32_t           vendor, /* in host-byte order */
	const qse_rad_attr_hdr_t* attr, 
	void*                 ctx
);

enum qse_rad_attr_id_t
{
	QSE_RAD_ATTR_USER_NAME             = 1,  /* string */
	QSE_RAD_ATTR_USER_PASSWORD         = 2,  /* string encrypted */
	QSE_RAD_ATTR_NAS_IP_ADDRESS        = 4,  /* ipaddr */
	QSE_RAD_ATTR_NAS_PORT              = 5,  /* integer */
	QSE_RAD_ATTR_SERVICE_TYPE          = 6,  /* integer */
	QSE_RAD_ATTR_FRAMED_IP_ADDRESS     = 8,  /* ipaddr */
	QSE_RAD_ATTR_REPLY_MESSAGE         = 18, /* string */
	QSE_RAD_ATTR_CLASS                 = 25, /* octets */
	QSE_RAD_ATTR_VENDOR_SPECIFIC       = 26, /* octets */
	QSE_RAD_ATTR_SESSION_TIMEOUT       = 27, /* integer */
	QSE_RAD_ATTR_IDLE_TIMEOUT          = 28, /* integer */
	QSE_RAD_ATTR_TERMINATION_ACTION    = 29, /* integer. 0:default, 1:radius-request */
	QSE_RAD_ATTR_CALLING_STATION_ID    = 31, /* string */
	QSE_RAD_ATTR_NAS_IDENTIFIER        = 32, /* string */
	QSE_RAD_ATTR_ACCT_STATUS_TYPE      = 40, /* integer */
	QSE_RAD_ATTR_ACCT_INPUT_OCTETS     = 42, /* integer */
	QSE_RAD_ATTR_ACCT_OUTPUT_OCTETS    = 43, /* integer */
	QSE_RAD_ATTR_ACCT_SESSION_ID       = 44, /* string */
	QSE_RAD_ATTR_ACCT_SESSION_TIME     = 46, /* integer */
	QSE_RAD_ATTR_ACCT_TERMINATE_CAUSE  = 49, /* integer */
	QSE_RAD_ATTR_ACCT_INPUT_GIGAWORDS  = 52, /* integer */
	QSE_RAD_ATTR_ACCT_OUTPUT_GIGAWORDS = 53, /* integer */
	QSE_RAD_ATTR_EVENT_TIMESTAMP       = 55, /* integer */
	QSE_RAD_ATTR_NAS_PORT_TYPE         = 61, /* integer */
	QSE_RAD_ATTR_ACCT_INTERIM_INTERVAL = 85, /* integer */
	QSE_RAD_ATTR_NAS_PORT_ID           = 87, /* string */
	QSE_RAD_ATTR_FRAMED_IPV6_PREFIX    = 97  /* ipv6prefix */
};

enum qse_rad_attr_acct_status_type_t
{
	QSE_RAD_ATTR_ACCT_STATUS_TYPE_START  = 1, /* accounting start */
	QSE_RAD_ATTR_ACCT_STATUS_TYPE_STOP   = 2, /* accounting stop */
	QSE_RAD_ATTR_ACCT_STATUS_TYPE_UPDATE = 3, /* interim update */
	QSE_RAD_ATTR_ACCT_STATUS_TYPE_ON     = 7, /* accounting on */
	QSE_RAD_ATTR_ACCT_STATUS_TYPE_OFF    = 8, /* accounting off */
	QSE_RAD_ATTR_ACCT_STATUS_TYPE_FAILED = 15
};

enum qse_rad_attr_acct_terminate_cause_t
{
	QSE_RAD_ATTR_ACCT_TERMINATE_CAUSE_USER_REQUEST        = 1,
	QSE_RAD_ATTR_ACCT_TERMINATE_CAUSE_LOST_CARRIER        = 2,
	QSE_RAD_ATTR_ACCT_TERMINATE_CAUSE_LOST_SERVICE        = 3,
	QSE_RAD_ATTR_ACCT_TERMINATE_CAUSE_IDLE_TIMEOUT        = 4,
	QSE_RAD_ATTR_ACCT_TERMINATE_CAUSE_SESSION_TIMEOUT     = 5,
	QSE_RAD_ATTR_ACCT_TERMINATE_CAUSE_ADMIN_RESET         = 6,
	QSE_RAD_ATTR_ACCT_TERMINATE_CAUSE_ADMIN_REBOOT        = 7,
	QSE_RAD_ATTR_ACCT_TERMINATE_CAUSE_PORT_ERROR          = 8,
	QSE_RAD_ATTR_ACCT_TERMINATE_CAUSE_NAS_ERROR           = 9,
	QSE_RAD_ATTR_ACCT_TERMINATE_CAUSE_NAS_REQUEST         = 10,
	QSE_RAD_ATTR_ACCT_TERMINATE_CAUSE_NAS_REBOOT          = 11,
	QSE_RAD_ATTR_ACCT_TERMINATE_CAUSE_PORT_UNNEEDED       = 12,
	QSE_RAD_ATTR_ACCT_TERMINATE_CAUSE_PORT_PREEMPTED      = 13,
	QSE_RAD_ATTR_ACCT_TERMINATE_CAUSE_PORT_SUSPENDED      = 14,
	QSE_RAD_ATTR_ACCT_TERMINATE_CAUSE_SERVICE_UNAVAILABLE = 15,
	QSE_RAD_ATTR_ACCT_TERMINATE_CAUSE_CALLBACK            = 16,
	QSE_RAD_ATTR_ACCT_TERMINATE_CAUSE_USER_ERROR          = 17,
	QSE_RAD_ATTR_ACCT_TERMINATE_CAUSE_HOST_REQUEST        = 18
};

enum qse_rad_attr_nas_port_type_t
{
	QSE_RAD_ATTR_NAS_PORT_TYPE_ASYNC        = 0,
	QSE_RAD_ATTR_NAS_PORT_TYPE_SYNC         = 1,
	QSE_RAD_ATTR_NAS_PORT_TYPE_ISDN         = 2,
	QSE_RAD_ATTR_NAS_PORT_TYPE_ISDN_V120    = 3,
	QSE_RAD_ATTR_NAS_PORT_TYPE_ISDN_V110    = 4,
	QSE_RAD_ATTR_NAS_PORT_TYPE_VIRTUAL      = 5
	/* TODO: more types */
};


#ifdef __cplusplus
extern "C" {
#endif

QSE_EXPORT void qse_rad_initialize (
	qse_rad_hdr_t*  hdr,
	qse_rad_code_t  code,
	qse_uint8_t     id
);

QSE_EXPORT qse_rad_attr_hdr_t* qse_rad_find_attribute (
	qse_rad_hdr_t*  hdr,
	qse_uint8_t     attrid,
	int             index
);

QSE_EXPORT qse_rad_attr_hdr_t* qse_rad_find_vendor_specific_attribute (
	qse_rad_hdr_t*  hdr,
	qse_uint32_t    vendor,
	qse_uint8_t     id,
	int             index
);

QSE_EXPORT int qse_rad_walk_attributes (
	const qse_rad_hdr_t*  hdr,
	qse_rad_attr_walker_t walker, 
	void*                 ctx
);

QSE_EXPORT int qse_rad_insert_attribute (
	qse_rad_hdr_t*  auth,
	int             max,
	qse_uint8_t     id,
	const void*     ptr,
	qse_uint8_t     len
);

QSE_EXPORT int qse_rad_insert_vendor_specific_attribute (
	qse_rad_hdr_t*  auth,
	int             max,
	qse_uint32_t    vendor,
	qse_uint8_t     attrid, 
	const void*     ptr,
	qse_uint8_t     len
);

QSE_EXPORT int qse_rad_delete_attribute (
	qse_rad_hdr_t* hdr,
	qse_uint8_t attrid
);

QSE_EXPORT int qse_rad_delete_vendor_specific_attribute (
	qse_rad_hdr_t*  hdr,
	qse_uint32_t    vendor,
	qse_uint8_t     attrid
);

QSE_EXPORT int qse_rad_insert_string_attribute (
	qse_rad_hdr_t*     auth, 
	int                max, 
	qse_uint32_t       vendor, /* in host-byte order */
	qse_uint8_t        id, 
	const qse_mchar_t* value
);

QSE_EXPORT int qse_rad_insert_wide_string_attribute (
	qse_rad_hdr_t*     auth, 
	int                max, 
	qse_uint32_t       vendor, /* in host-byte order */
	qse_uint8_t        id, 
	const qse_wchar_t* value
);

QSE_EXPORT int qse_rad_insert_string_attribute_with_length (
	qse_rad_hdr_t*         auth, 
	int                max, 
	qse_uint32_t       vendor, /* in host-byte order */
	qse_uint8_t        id, 
	const qse_mchar_t* value,
	qse_uint8_t        length
);

QSE_EXPORT int qse_rad_insert_wide_string_attribute_with_length (
	qse_rad_hdr_t*         auth, 
	int                max, 
	qse_uint32_t       vendor, /* in host-byte order */
	qse_uint8_t        id, 
	const qse_wchar_t* value,
	qse_uint8_t        length
);

QSE_EXPORT int qse_rad_insert_integer_attribute (
	qse_rad_hdr_t*       auth, 
	int              max,
	qse_uint32_t     vendor, /* in host-byte order */
	qse_uint8_t      id,
	qse_uint32_t     value /* in host-byte order */
);

QSE_EXPORT int qse_rad_insert_ipv6prefix_attribute (
	qse_rad_hdr_t*          auth, 
	int                 max,
	qse_uint32_t        vendor, /* in host-byte order */
	qse_uint8_t         id,
	qse_uint8_t         prefix_bits,
	const qse_ip6ad_t*  value
);

QSE_EXPORT int qse_rad_insert_giga_attribute (
	qse_rad_hdr_t*    auth,
	int               max,
	qse_uint32_t      vendor,
	int               low_id,
	int               high_id,
	qse_uint64_t      value
);

QSE_EXPORT int qse_rad_set_user_password (
	qse_rad_hdr_t*      auth,
	int                 max,
	const qse_mchar_t*  password,
	const qse_mchar_t*  secret
);

QSE_EXPORT void qse_rad_fill_authenticator (
	qse_rad_hdr_t*        auth
);

QSE_EXPORT void qse_rad_copy_authenticator (
	qse_rad_hdr_t*        dst,
	const qse_rad_hdr_t*  src
);

QSE_EXPORT int qse_rad_set_authenticator (
	qse_rad_hdr_t*       req, 
	const qse_mchar_t*   secret
);

/* 
 * verify an accounting request.
 * the authenticator of an access request is filled randomly.
 * so this function doesn't apply
 */
QSE_EXPORT int qse_rad_verify_request (
	qse_rad_hdr_t*      req,
	const qse_mchar_t*  secret
);

QSE_EXPORT int qse_rad_verify_response (
	qse_rad_hdr_t*         res,
	const qse_rad_hdr_t*   req,
	const qse_mchar_t*     secret
);


#ifdef __cplusplus
}
#endif

#endif
