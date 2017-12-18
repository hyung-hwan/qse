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

#include <qse/rad/radmsg.h>
#include "../cmn/mem-prv.h"
#include "../cmn/syscall.h"
#include <qse/cmn/hton.h>
#include <qse/cmn/str.h>
#include <qse/cmn/mbwc.h>
#include <qse/cmn/time.h>
#include <qse/cmn/alg.h>
#include <qse/cry/md5.h>


void qse_rad_initialize (qse_rad_hdr_t* hdr, qse_rad_code_t code, qse_uint8_t id)
{
	QSE_MEMSET (hdr, 0, sizeof(*hdr));
	hdr->code = code;
	hdr->id = id;
	hdr->length = qse_hton16(sizeof(*hdr));
}

static QSE_INLINE void xor (void* p, void* q, int length)
{
	int i;
	qse_uint8_t* pp = (qse_uint8_t*)p;
	qse_uint8_t* qq = (qse_uint8_t*)q;
	for (i = 0; i < length; i++) *(pp++) ^= *(qq++);
}

static void fill_authenticator_randomly (void* authenticator, int length)
{
	qse_uint8_t* v = (qse_uint8_t*)authenticator;
	int total = 0;

#if defined(__linux)
	int fd;

	fd = QSE_OPEN("/dev/urandom", O_RDONLY, 0); /* Linux: get *real* random numbers */
	if (fd >= 0) 
	{
		while (total < length) 
		{
			int bytes = QSE_READ (fd, &v[total], length - total);
			if (bytes <= 0) break;
			total += bytes;
		}
		QSE_CLOSE(fd);
	}
#endif

	if (total < length) 
	{
		qse_ntime_t now;
		qse_uint32_t seed;

		qse_gettime (&now);
		seed = QSE_GETPID() + now.sec + now.nsec;

		while (total < length) 
		{
			seed = qse_randxs32(seed);
			v[total] = seed % QSE_TYPE_MAX(qse_uint8_t);
			total++;
		}
	}
}

static qse_rad_attr_hdr_t* find_attribute (qse_rad_attr_hdr_t* attr, int* len, qse_uint8_t attrid)
{
	int rem = *len;

	while (rem >= QSE_SIZEOF(*attr))
	{
		/* sanity checks */
		if (rem < attr->length) return NULL;
		if (attr->length < QSE_SIZEOF(*attr)) 
		{
			/* attribute length cannot be less than the header size.
			 * the packet could be corrupted... */
			return NULL; 
		}

		rem -= attr->length;
		if (attr->id == attrid) 
		{
			*len = rem;
			return attr;
		}

		attr = (qse_rad_attr_hdr_t*) ((char*) attr + attr->length);
	}

	return NULL;
}

qse_rad_attr_hdr_t* qse_rad_find_attribute (qse_rad_hdr_t* hdr, qse_uint8_t attrid, int index)
{
	qse_rad_attr_hdr_t *attr = (qse_rad_attr_hdr_t*)(hdr+1);
	int len = qse_ntoh16(hdr->length) - QSE_SIZEOF(*hdr);
	attr = find_attribute (attr, &len, attrid);
	while (attr)
	{
		if (index <= 0) return attr;
		index--;
		attr = find_attribute ((qse_rad_attr_hdr_t*)((char*)attr+attr->length), &len, attrid);
	}

	return NULL;
}

qse_rad_attr_hdr_t* qse_rad_find_vendor_specific_attribute (qse_rad_hdr_t* hdr, qse_uint32_t vendor, qse_uint8_t attrid, int index)
{
	qse_rad_attr_hdr_t *attr = (qse_rad_attr_hdr_t*)(hdr+1);
	int len = qse_ntoh16(hdr->length) - QSE_SIZEOF(*hdr);

	attr = find_attribute (attr, &len, QSE_RAD_ATTR_VENDOR_SPECIFIC);
	while (attr)
	{
		qse_rad_vsattr_hdr_t* vsattr;

		if (attr->length >= QSE_SIZEOF(*vsattr)) /* sanity check */
		{
			vsattr = (qse_rad_vsattr_hdr_t*)attr;

			if (qse_ntoh32(vsattr->vendor) == vendor)
			{
				qse_rad_attr_hdr_t* subattr;
				int sublen = vsattr->length - QSE_SIZEOF(*vsattr);

				if (sublen >= QSE_SIZEOF(*subattr)) /* sanity check */
				{
					subattr = (qse_rad_attr_hdr_t*)(vsattr + 1);
					if (subattr->id == attrid && subattr->length == sublen) 
					{
						if (index <= 0) return subattr;
						index--;
					}
				}
			}
		}

		attr = find_attribute ((qse_rad_attr_hdr_t*)((char*)attr+attr->length), &len, QSE_RAD_ATTR_VENDOR_SPECIFIC);
	}

	return NULL;
}

int qse_rad_walk_attributes (const qse_rad_hdr_t* hdr, qse_rad_attr_walker_t walker, void* ctx)
{
	int totlen, rem;
	qse_rad_attr_hdr_t *attr;

	totlen = qse_ntoh16(hdr->length);
	if (totlen < QSE_SIZEOF(*hdr)) return -1;

	rem = totlen - QSE_SIZEOF(*hdr);
	attr = (qse_rad_attr_hdr_t*)(hdr + 1);
	while (rem >= QSE_SIZEOF(*attr))
	{
		/* sanity checks */
		if (rem < attr->length) return -1;
		if (attr->length < QSE_SIZEOF(*attr)) 
		{
			/* attribute length cannot be less than the header size.
			 * the packet could be corrupted... */
			return -1;
		}

		rem -= attr->length;

		if (attr->id == QSE_RAD_ATTR_VENDOR_SPECIFIC)
		{
			qse_rad_vsattr_hdr_t* vsattr;
			qse_rad_attr_hdr_t* subattr;
			int sublen;

			if (attr->length < QSE_SIZEOF(*vsattr)) return -1;
			vsattr = (qse_rad_vsattr_hdr_t*)attr;

			sublen = vsattr->length - QSE_SIZEOF(*vsattr);
			if (sublen < QSE_SIZEOF(*subattr)) return -1;
			subattr = (qse_rad_attr_hdr_t*)(vsattr + 1);
			if (subattr->length != sublen) return -1;

			/* if this vendor happens to be 0, walker can't tell
			 * if it is vendor specific or not because 0 is passed in
			 * for non-VSAs. but i don't care. in reality, 
			 * 0 is reserved in IANA enterpirse number assignments.
			 * (http://www.iana.org/assignments/enterprise-numbers) */
			if (walker (hdr, qse_ntoh32(vsattr->vendor), subattr, ctx) <= -1) return -1;
		}
		else
		{
			if (walker (hdr, 0, attr, ctx) <= -1) return -1;
		}

		attr = (qse_rad_attr_hdr_t*) ((char*) attr + attr->length);
	}

	return 0;
}


int qse_rad_insert_attribute (
	qse_rad_hdr_t* auth, int max,
	qse_uint8_t id, const void* ptr, qse_uint8_t len)
{
	qse_rad_attr_hdr_t* attr;
	int auth_len = qse_ntoh16(auth->length);
	int new_auth_len;

	/*if (len > QSE_RAD_MAX_ATTR_VALUE_LEN) return -1;*/
	if (len > QSE_RAD_MAX_ATTR_VALUE_LEN) len = QSE_RAD_MAX_ATTR_VALUE_LEN;
	new_auth_len = auth_len + len + QSE_SIZEOF(*attr);

	if (new_auth_len > max) return -1;

	attr = (qse_rad_attr_hdr_t*) ((char*)auth + auth_len);
	attr->id = id;
	attr->length = new_auth_len - auth_len;
	QSE_MEMCPY (attr + 1, ptr, len);
	auth->length = qse_hton16(new_auth_len);

	return 0;
}

int qse_rad_insert_vendor_specific_attribute (
	qse_rad_hdr_t* auth, int max,
	qse_uint32_t vendor, qse_uint8_t attrid, const void* ptr, qse_uint8_t len)
{
	qse_rad_vsattr_hdr_t* attr;
	qse_rad_attr_hdr_t* subattr;
	int auth_len = qse_ntoh16(auth->length);
	int new_auth_len;


	/*if (len > QSE_RAD_MAX_VSATTR_VALUE_LEN) return -1;*/
	if (len > QSE_RAD_MAX_VSATTR_VALUE_LEN) len = QSE_RAD_MAX_VSATTR_VALUE_LEN;
	new_auth_len = auth_len + len + QSE_SIZEOF(*attr) + QSE_SIZEOF(*subattr);

	if (new_auth_len > max) return -1;

	attr = (qse_rad_vsattr_hdr_t*) ((char*)auth + auth_len);
	attr->id = QSE_RAD_ATTR_VENDOR_SPECIFIC;
	attr->length = new_auth_len - auth_len;
	attr->vendor = qse_hton32 (vendor);

	subattr = (qse_rad_attr_hdr_t*)(attr + 1);
	subattr->id = attrid;
	subattr->length = len + QSE_SIZEOF(*subattr);
	QSE_MEMCPY (subattr + 1, ptr, len);

	auth->length = qse_hton16(new_auth_len);
	return 0;
}

static int delete_attribute (qse_rad_hdr_t* auth, qse_rad_attr_hdr_t* attr)
{
	qse_uint16_t auth_len;
	qse_uint16_t tmp_len;

	auth_len = qse_ntoh16(auth->length);
	tmp_len = ((qse_uint8_t*)attr - (qse_uint8_t*)auth) + attr->length;
	if (tmp_len > auth_len) return -1; /* can this happen? */

	QSE_MEMCPY (attr, (qse_uint8_t*)attr + attr->length, auth_len - tmp_len);

	auth_len -= attr->length;
	auth->length = qse_hton16(auth_len);
	return 0;
}

int qse_rad_delete_attribute (qse_rad_hdr_t* auth, qse_uint8_t attrid)
{
	qse_rad_attr_hdr_t* attr;

	attr = qse_rad_find_attribute (auth, attrid, 0);
	if (attr == NULL) return 0; /* not found */
	return (delete_attribute (auth, attr) <= -1)? -1: 1;
}

int qse_rad_delete_vendor_specific_attribute (
	qse_rad_hdr_t* auth, qse_uint32_t vendor, qse_uint8_t attrid)
{
	qse_rad_attr_hdr_t* attr; 
	qse_rad_vsattr_hdr_t* vsattr;

	attr = qse_rad_find_vendor_specific_attribute (auth, vendor, attrid, 0);
	if (attr == NULL) return 0; /* not found */

	vsattr = (qse_rad_vsattr_hdr_t*)((qse_uint8_t*)attr - QSE_SIZEOF(qse_rad_vsattr_hdr_t));
	return (delete_attribute (auth, (qse_rad_attr_hdr_t*)vsattr) <= -1)? -1: 1;
}

int qse_rad_insert_string_attribute (
	qse_rad_hdr_t* auth, int max, qse_uint32_t vendor, 
	qse_uint8_t id, const qse_mchar_t* value)
{
	return (vendor == 0)?
		qse_rad_insert_attribute (auth, max, id, value, qse_mbslen(value)):
		qse_rad_insert_vendor_specific_attribute (auth, max, vendor, id, value, qse_mbslen(value));
}

int qse_rad_insert_wide_string_attribute (
	qse_rad_hdr_t* auth, int max, qse_uint32_t vendor, 
	qse_uint8_t id, const qse_wchar_t* value)
{
	int n;
	qse_mchar_t* val;
	qse_size_t mbslen;

	val = qse_wcstombsdup (value, &mbslen, QSE_MMGR_GETDFL());
	n = (vendor == 0)?
		qse_rad_insert_attribute (auth, max, id, val, mbslen):
		qse_rad_insert_vendor_specific_attribute (auth, max, vendor, id, val, mbslen);
	QSE_MMGR_FREE (QSE_MMGR_GETDFL(), val);

	return n;
}

int qse_rad_insert_string_attribute_with_length (
	qse_rad_hdr_t* auth, int max, qse_uint32_t vendor, 
	qse_uint8_t id, const qse_mchar_t* value, qse_uint8_t length)
{
	return (vendor == 0)?
		qse_rad_insert_attribute (auth, max, id, value, length):
		qse_rad_insert_vendor_specific_attribute (auth, max, vendor, id, value, length);
}

int qse_rad_insert_wide_string_attribute_with_length (
	qse_rad_hdr_t* auth, int max, qse_uint32_t vendor, 
	qse_uint8_t id, const qse_wchar_t* value, qse_uint8_t length)
{
	int n;
	qse_mchar_t* val;
	qse_size_t mbslen;

	val = qse_wcsntombsdup (value, length, &mbslen, QSE_MMGR_GETDFL());
	n = (vendor == 0)?
		qse_rad_insert_attribute (auth, max, id, val, mbslen):
		qse_rad_insert_vendor_specific_attribute (auth, max, vendor, id, val, mbslen);
	QSE_MMGR_FREE (QSE_MMGR_GETDFL(), val);

	return n;
}

int qse_rad_insert_uint32_attribute (
	qse_rad_hdr_t* auth, int max, qse_uint32_t vendor, qse_uint8_t id, qse_uint32_t value)
{
	qse_uint32_t val = qse_hton32(value);
	return (vendor == 0)?
		qse_rad_insert_attribute (auth, max, id, &val, QSE_SIZEOF(val)):
		qse_rad_insert_vendor_specific_attribute (auth, max, vendor, id, &val, QSE_SIZEOF(val));
}

int qse_rad_insert_ipv6prefix_attribute (
	qse_rad_hdr_t* auth, int  max, qse_uint32_t vendor, qse_uint8_t id,
	qse_uint8_t prefix_bits, const qse_ip6ad_t* value)
{
	struct ipv6prefix_t
	{
		qse_uint8_t reserved;
		qse_uint8_t bits;
		qse_ip6ad_t value;
	} __attribute__((__packed__));

	struct ipv6prefix_t  ipv6prefix;	
	qse_uint8_t i, j;

	if (prefix_bits > 128) prefix_bits = 128;

	QSE_MEMSET (&ipv6prefix, 0, sizeof(ipv6prefix));
	ipv6prefix.bits = prefix_bits;

	for (i = 0, j = 0; i < prefix_bits; i += 8, j++)
	{
		qse_uint8_t bits = prefix_bits - i;
		if (bits >= 8)
		{
			ipv6prefix.value.value[j] = value->value[j];
		}
		else
		{
			/*
				1 -> 10000000
				2 -> 11000000
				3 -> 11100000
				4 -> 11110000
				5 -> 11111000
				6 -> 11111100
				7 -> 11111110
			*/
			ipv6prefix.value.value[j] = value->value[j] & (0xFF << (8 - bits));
		}
	}
	
	return (vendor == 0)?
		qse_rad_insert_attribute (auth, max, id, &ipv6prefix, j + 2):
		qse_rad_insert_vendor_specific_attribute (auth, max, vendor, id, &ipv6prefix, j + 2);
}

int qse_rad_insert_giga_attribute (
	qse_rad_hdr_t* auth, int max, qse_uint32_t  vendor, int low_id, int high_id, qse_uint64_t value)
{
	qse_uint32_t low;
	low = value & QSE_TYPE_MAX(qse_uint32_t);
	low = qse_hton32(low);

	if (vendor == 0)
	{
		if (qse_rad_insert_attribute (auth, max, low_id, &low, QSE_SIZEOF(low)) <= -1) return -1;

		if (value > QSE_TYPE_MAX(qse_uint32_t))
		{
			qse_uint32_t high;
			high = value >> (QSE_SIZEOF(qse_uint32_t) * 8);
			high = qse_hton32(high);
			if (qse_rad_insert_attribute (auth, max, high_id, &high, QSE_SIZEOF(high)) <= -1) return -1;
		}
	}
	else
	{
		if (qse_rad_insert_vendor_specific_attribute (auth, max, vendor, low_id, &low, QSE_SIZEOF(low)) <= -1) return -1;

		if (value > QSE_TYPE_MAX(qse_uint32_t))
		{
			qse_uint32_t high;
			high = value >> (QSE_SIZEOF(qse_uint32_t) * 8);
			high = qse_hton32(high);
			if (qse_rad_insert_vendor_specific_attribute (auth, max, vendor, high_id, &high, QSE_SIZEOF(high)) <= -1) return -1;
		}
	}

	return 0;
}


int qse_rad_insert_extended_vendor_specific_attribute (
	qse_rad_hdr_t* auth, int max, qse_uint8_t base, qse_uint32_t vendor,
	qse_uint8_t attrid, const void* ptr, qse_uint8_t len)
{
	/* RFC6929 */
	qse_rad_extvsattr_hdr_t* attr;
	int auth_len = qse_ntoh16(auth->length);
	int new_auth_len;

	if (base < 241 && base > 244) return -1;
/* TODO: for 245 and 246, switch to long-extended format */

	/*if (len > QSE_RAD_MAX_EXTVSATTR_VALUE_LEN) return -1;*/
	if (len > QSE_RAD_MAX_EXTVSATTR_VALUE_LEN) len = QSE_RAD_MAX_EXTVSATTR_VALUE_LEN;
	new_auth_len = auth_len + len + QSE_SIZEOF(*attr);

	if (new_auth_len > max) return -1;

	attr = (qse_rad_extvsattr_hdr_t*) ((char*)auth + auth_len);
	attr->id = base;
	attr->length = new_auth_len - auth_len;
	attr->xid = QSE_RAD_ATTR_VENDOR_SPECIFIC;
	attr->vendor = qse_hton32(vendor);
	attr->evsid = attrid;

	/* no special header for the evs-value */
	QSE_MEMCPY (attr + 1, ptr, len);

	auth->length = qse_hton16(new_auth_len);
	return 0;
}



#define PASS_BLKSIZE QSE_RAD_MAX_AUTHENTICATOR_LEN
#define ALIGN(x,factor) ((((x) + (factor) - 1) / (factor)) * (factor))

int qse_rad_set_user_password (qse_rad_hdr_t* auth, int max, const qse_mchar_t* password, const qse_mchar_t* secret)
{
	qse_md5_t md5;

	qse_uint8_t hashed[QSE_RAD_MAX_ATTR_VALUE_LEN]; /* can't be longer than this */
	qse_uint8_t tmp[PASS_BLKSIZE];

	int i, pwlen, padlen;

	QSE_ASSERT (QSE_SIZEOF(tmp) >= QSE_MD5_DIGEST_LEN);

	pwlen = qse_mbslen(password);

	/* calculate padlen to be the multiples of 16.
	 * 0 is forced to 16. */
	padlen = (pwlen <= 0)? PASS_BLKSIZE: ALIGN(pwlen,PASS_BLKSIZE);

	/* keep the padded length limited within the maximum attribute length */
	if (padlen > QSE_RAD_MAX_ATTR_VALUE_LEN)
	{
		padlen = QSE_RAD_MAX_ATTR_VALUE_LEN;
		padlen = ALIGN(padlen,PASS_BLKSIZE);
		if (padlen > QSE_RAD_MAX_ATTR_VALUE_LEN) padlen -= PASS_BLKSIZE;

		/* also limit the original length */
		if (pwlen > padlen) pwlen = padlen;
	}

	QSE_MEMSET (hashed, 0, padlen);
	QSE_MEMCPY (hashed, password, pwlen);

	/*
	 * c1 = p1 XOR MD5(secret + authenticator)
	 * c2 = p2 XOR MD5(secret + c1)
	 * ...
	 * cn = pn XOR MD5(secret + cn-1)
	 */
	qse_md5_initialize (&md5);
	qse_md5_update (&md5, secret, qse_mbslen(secret));
	qse_md5_update (&md5, auth->authenticator, QSE_SIZEOF(auth->authenticator));
	qse_md5_digest (&md5, tmp, QSE_SIZEOF(tmp));

	xor (&hashed[0], tmp, QSE_SIZEOF(tmp));

	for (i = 1; i < (padlen >> 4); i++) 
	{
		qse_md5_initialize (&md5);
		qse_md5_update (&md5, secret, qse_mbslen(secret));
		qse_md5_update (&md5, &hashed[(i-1) * PASS_BLKSIZE], PASS_BLKSIZE);
		qse_md5_digest (&md5, tmp, QSE_SIZEOF(tmp));
		xor (&hashed[i * PASS_BLKSIZE], tmp, QSE_SIZEOF(tmp));
	}

	/* ok if not found or deleted. but not ok if an error occurred */
	if (qse_rad_delete_attribute (auth, QSE_RAD_ATTR_USER_PASSWORD) <= -1) goto oops; 
	if (qse_rad_insert_attribute (auth, max, QSE_RAD_ATTR_USER_PASSWORD, hashed, padlen) <= -1) goto oops;

	return 0;

oops:
	return -1;
}

void qse_rad_fill_authenticator (qse_rad_hdr_t* auth)
{
	fill_authenticator_randomly (auth->authenticator, QSE_SIZEOF(auth->authenticator));
}

void qse_rad_copy_authenticator (qse_rad_hdr_t* dst, const qse_rad_hdr_t* src)
{
	QSE_MEMCPY (dst->authenticator, src->authenticator, QSE_SIZEOF(dst->authenticator));
}

int qse_rad_set_authenticator (qse_rad_hdr_t* req, const qse_mchar_t* secret)
{
	qse_md5_t md5;

	/* this assumes that req->authentcator at this point
	 * is filled with zeros. so make sure that it contains zeros
	 * before you call this function */

	qse_md5_initialize (&md5);
	qse_md5_update (&md5, req, qse_ntoh16(req->length));
	if (*secret) qse_md5_update (&md5, secret, qse_mbslen(secret));
	qse_md5_digest (&md5, req->authenticator, QSE_SIZEOF(req->authenticator));

	return 0;
}

int qse_rad_verify_request (qse_rad_hdr_t* req, const qse_mchar_t* secret)
{
	qse_md5_t md5;
	qse_uint8_t orgauth[QSE_RAD_MAX_AUTHENTICATOR_LEN];
	int ret;

	QSE_MEMCPY (orgauth, req->authenticator, QSE_SIZEOF(req->authenticator));
	QSE_MEMSET (req->authenticator, 0, QSE_SIZEOF(req->authenticator));

	qse_md5_initialize (&md5);
	qse_md5_update (&md5, req, qse_ntoh16(req->length));
	if (*secret) qse_md5_update (&md5, secret, qse_mbslen(secret));
	qse_md5_digest (&md5, req->authenticator, QSE_SIZEOF(req->authenticator));

	ret = (QSE_MEMCMP (req->authenticator, orgauth, QSE_SIZEOF(req->authenticator)) == 0)? 1: 0;
	QSE_MEMCPY (req->authenticator, orgauth, QSE_SIZEOF(req->authenticator));

	return ret;
}

int qse_rad_verify_response (qse_rad_hdr_t* res, const qse_rad_hdr_t* req, const qse_mchar_t* secret)
{
	qse_md5_t md5;

	qse_uint8_t calculated[QSE_RAD_MAX_AUTHENTICATOR_LEN];
	qse_uint8_t reply[QSE_RAD_MAX_AUTHENTICATOR_LEN];

	QSE_ASSERT (QSE_SIZEOF(req->authenticator) == QSE_RAD_MAX_AUTHENTICATOR_LEN);
	QSE_ASSERT (QSE_SIZEOF(res->authenticator) == QSE_RAD_MAX_AUTHENTICATOR_LEN);

	/*
	 * We could dispense with the QSE_MEMCPY, and do MD5's of the packet
	 * + authenticator piece by piece. This is easier understand, 
	 * and maybe faster.
	 */
	QSE_MEMCPY(reply, res->authenticator, QSE_SIZEOF(res->authenticator)); /* save the reply */
	QSE_MEMCPY(res->authenticator, req->authenticator, QSE_SIZEOF(req->authenticator)); /* sent authenticator */

	/* MD5(response packet header + authenticator + response packet data + secret) */
	qse_md5_initialize (&md5);
	qse_md5_update (&md5, res, qse_ntoh16(res->length));

	/* 
	 * This next bit is necessary because of a bug in the original Livingston
	 * RADIUS server. The authentication authenticator is *supposed* to be 
	 * MD5'd with the old password (as the secret) for password changes.
	 * However, the old password isn't used. The "authentication" authenticator
	 * for the server reply packet is simply the MD5 of the reply packet.
	 * Odd, the code is 99% there, but the old password is never copied
	 * to the secret!
	 */
	if (*secret) qse_md5_update (&md5, secret, qse_mbslen(secret));
	qse_md5_digest (&md5, calculated, QSE_SIZEOF(calculated));

	/* Did he use the same random authenticator + shared secret? */
	return (QSE_MEMCMP(calculated, reply, QSE_SIZEOF(reply)) != 0)? 0: 1;
}
