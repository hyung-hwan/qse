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

/*
 * dict.c	Routines to read the dictionary file.
 *
 * Version:	$Id$
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * Copyright 2000,2006  The FreeRADIUS server project
 */

#include <qse/rad/raddic.h>
#include <qse/rad/radmsg.h>
#include <qse/cmn/htl.h>
#include <qse/cmn/str.h>
#include <qse/cmn/chr.h>
#include <qse/cmn/path.h>
#include "../cmn/mem-prv.h"
#include <qse/si/sio.h>
#include <stdarg.h>

typedef struct const_fixup_t const_fixup_t;

struct const_fixup_t 
{
	const_fixup_t*        next;
	qse_raddic_const_t*   dval;
	qse_size_t            line;
	qse_char_t*           fn;
	qse_char_t            attrstr[1];
};

struct qse_raddic_t
{
	qse_mmgr_t*         mmgr;
	qse_raddic_errnum_t errnum;
	qse_char_t          errmsg[256];
	qse_char_t          errmsg2[256];

	struct 
	{
		int trait;
	} opt;

	qse_htl_t vendors_byname;
	qse_htl_t vendors_byvalue;
	qse_htl_t attrs_byname;
	qse_htl_t attrs_byvalue;
	qse_htl_t consts_byvalue;
	qse_htl_t consts_byname;

	qse_raddic_attr_t* last_attr;
	qse_raddic_attr_t* base_attrs[256];
	const_fixup_t* const_fixup;
};

typedef struct name_id_t name_id_t;
struct name_id_t
{
	const qse_char_t* name;
	int               id;
	int               length;
};

static const name_id_t type_table[] = 
{
	{ QSE_T("string"),            QSE_RADDIC_ATTR_TYPE_STRING,            -1 },
	{ QSE_T("octets"),            QSE_RADDIC_ATTR_TYPE_OCTETS,            -1 },

	{ QSE_T("ipaddr"),            QSE_RADDIC_ATTR_TYPE_IPV4_ADDR,          4 },
	{ QSE_T("ipv4prefix"),        QSE_RADDIC_ATTR_TYPE_IPV4_PREFIX,       -1 },
	{ QSE_T("ipv6addr"),          QSE_RADDIC_ATTR_TYPE_IPV6_ADDR,         16 },
	{ QSE_T("ipv6prefix"),        QSE_RADDIC_ATTR_TYPE_IPV6_PREFIX,       -1 },
	{ QSE_T("ifid"),              QSE_RADDIC_ATTR_TYPE_IFID,               8 },
	{ QSE_T("combo-ip"),          QSE_RADDIC_ATTR_TYPE_COMBO_IP_ADDR,     -1 },
	{ QSE_T("combo-prefix"),      QSE_RADDIC_ATTR_TYPE_COMBO_IP_PREFIX,   -1 },
	{ QSE_T("ether"),             QSE_RADDIC_ATTR_TYPE_ETHERNET,           6 },

	{ QSE_T("bool"),              QSE_RADDIC_ATTR_TYPE_BOOL,               1 },

	{ QSE_T("uint8"),             QSE_RADDIC_ATTR_TYPE_UINT8,              1 },
	{ QSE_T("uint16"),            QSE_RADDIC_ATTR_TYPE_UINT16,             2 },
	{ QSE_T("uint32"),            QSE_RADDIC_ATTR_TYPE_UINT32,             4 },
	{ QSE_T("uint64"),            QSE_RADDIC_ATTR_TYPE_UINT64,             8 },

	{ QSE_T("int8"),              QSE_RADDIC_ATTR_TYPE_INT8,               1 },
	{ QSE_T("int16"),             QSE_RADDIC_ATTR_TYPE_INT16,              2 },
	{ QSE_T("int32"),             QSE_RADDIC_ATTR_TYPE_INT32,              4 },
	{ QSE_T("int64"),             QSE_RADDIC_ATTR_TYPE_INT64,              8 },

	{ QSE_T("float32"),           QSE_RADDIC_ATTR_TYPE_FLOAT32,            4 },
	{ QSE_T("float64"),           QSE_RADDIC_ATTR_TYPE_FLOAT64,            8 },

	{ QSE_T("timeval"),           QSE_RADDIC_ATTR_TYPE_TIMEVAL,           -1 },
	{ QSE_T("date"),              QSE_RADDIC_ATTR_TYPE_DATE,               4 },
	{ QSE_T("date_milliseconds"), QSE_RADDIC_ATTR_TYPE_DATE_MILLISECONDS, -1 },
	{ QSE_T("date_microseconds"), QSE_RADDIC_ATTR_TYPE_DATE_MICROSECONDS, -1 },
	{ QSE_T("date_nanoseconds"),  QSE_RADDIC_ATTR_TYPE_DATE_NANOSECONDS,  -1 },

	{ QSE_T("abinary"),           QSE_RADDIC_ATTR_TYPE_ABINARY,           -1 },

	{ QSE_T("size"),              QSE_RADDIC_ATTR_TYPE_SIZE,               8 },

	{ QSE_T("tlv"),               QSE_RADDIC_ATTR_TYPE_TLV,               -1 },
	{ QSE_T("struct"),            QSE_RADDIC_ATTR_TYPE_STRUCT,            -1 },

	{ QSE_T("extended"),          QSE_RADDIC_ATTR_TYPE_EXTENDED,           0 },
	{ QSE_T("long-extended"),     QSE_RADDIC_ATTR_TYPE_LONG_EXTENDED,      0 },

	{ QSE_T("vsa"),               QSE_RADDIC_ATTR_TYPE_VSA,               -1 },
	{ QSE_T("evs"),               QSE_RADDIC_ATTR_TYPE_EVS,               -1 },
	{ QSE_T("vendor"),            QSE_RADDIC_ATTR_TYPE_VENDOR,            -1 },

	/*
	 *	Alternative names
	 */
	{ QSE_T("cidr"),              QSE_RADDIC_ATTR_TYPE_IPV4_PREFIX,       -1 },
	{ QSE_T("byte"),              QSE_RADDIC_ATTR_TYPE_UINT8,              1 },
	{ QSE_T("short"),             QSE_RADDIC_ATTR_TYPE_UINT16,             2 },
	{ QSE_T("integer"),           QSE_RADDIC_ATTR_TYPE_UINT32,             4 },
	{ QSE_T("integer64"),         QSE_RADDIC_ATTR_TYPE_UINT64,             8 },
	{ QSE_T("decimal"),           QSE_RADDIC_ATTR_TYPE_FLOAT64,            8 },
	{ QSE_T("signed"),            QSE_RADDIC_ATTR_TYPE_INT32,              4 }
};

/* -------------------------------------------------------------------------- */
static int str_to_type (qse_raddic_t* dic, const qse_char_t* name, int* length)
{
	int i;

	for (i = 0; i < QSE_COUNTOF(type_table); i++)
	{
		if (qse_strcmp(name, type_table[i].name) == 0) 
		{
			if (length) *length = type_table[i].length;
			return  type_table[i].id;
		}
	}

	dic->errnum = QSE_RADDIC_EINVAL;
	return -1;
}
/* -------------------------------------------------------------------------- */

static qse_uint32_t dict_vendor_name_hash (qse_htl_t* htl, const void *data)
{
	return qse_strcasehash32(((const qse_raddic_vendor_t*)data)->name);
}

static int dict_vendor_name_cmp (qse_htl_t* htl, const void* one, const void* two)
{
	const qse_raddic_vendor_t* a = one;
	const qse_raddic_vendor_t* b = two;
	return qse_strcasecmp(a->name, b->name);
}

static void dict_vendor_name_free (qse_htl_t* htl, void* data)
{
	QSE_MMGR_FREE (htl->mmgr, data);
}

static qse_uint32_t dict_vendor_name_hetero_hash (qse_htl_t* htl, const void* one)
{
	return qse_strcasehash32((const qse_char_t*)one);
}

static int dict_vendor_name_hetero_cmp (qse_htl_t* htl, const void* one, const void* two)
{
	const qse_raddic_vendor_t* b = two;
	return qse_strcasecmp((const qse_char_t*)one, b->name);
}

static qse_uint32_t dict_vendor_value_hash (qse_htl_t* htl, const void* data)
{
	const qse_raddic_vendor_t* v = (const qse_raddic_vendor_t*)data;
	return qse_genhash32(&v->vendorpec, QSE_SIZEOF(v->vendorpec));
}

static int dict_vendor_value_cmp (qse_htl_t* htl, const void* one, const void* two)
{
	const qse_raddic_vendor_t *a = one;
	const qse_raddic_vendor_t *b = two;
	return a->vendorpec - b->vendorpec;
}

/* -------------------------------------------------------------------------- */

static qse_uint32_t dict_attr_name_hash (qse_htl_t* htl, const void *data)
{
	return qse_strcasehash32(((const qse_raddic_attr_t*)data)->name);
}

static int dict_attr_name_cmp (qse_htl_t* htl, const void* one, const void* two)
{
	const qse_raddic_attr_t* a = one;
	const qse_raddic_attr_t* b = two;
	return qse_strcasecmp(a->name, b->name);
}

static void dict_attr_name_free (qse_htl_t* htl, void* data)
{
	QSE_MMGR_FREE (htl->mmgr, data);
}

static qse_uint32_t dict_attr_name_hetero_hash (qse_htl_t* htl, const void* one)
{
	return qse_strcasehash32((const qse_char_t*)one);
}

static int dict_attr_name_hetero_cmp (qse_htl_t* htl, const void* one, const void* two)
{
	const qse_raddic_attr_t* b = two;
	return qse_strcasecmp((const qse_char_t*)one, b->name);
}

static qse_uint32_t dict_attr_value_hash (qse_htl_t* htl, const void* data)
{
	const qse_raddic_attr_t* v = (const qse_raddic_attr_t*)data;
	qse_uint32_t hv;

	hv = qse_genhash32(&v->vendor, QSE_SIZEOF(v->vendor));
	return qse_genhash32_update(&v->attr, QSE_SIZEOF(v->attr), hv);
}

static int dict_attr_value_cmp (qse_htl_t* htl, const void* one, const void* two)
{
	const qse_raddic_attr_t *a = one;
	const qse_raddic_attr_t *b = two;

	if (a->vendor < b->vendor) return -1;
	if (a->vendor > b->vendor) return 1;

	if (a->attr < b->attr) return -1;
	if (a->attr > b->attr) return 1;
	return 0;
}

/* -------------------------------------------------------------------------- */

struct const_hsd_t
{
	const qse_char_t* name;
	qse_uint32_t attr;
};
typedef struct const_hsd_t const_hsd_t;

static qse_uint32_t dict_const_name_hash (qse_htl_t* htl, const void* data)
{
	qse_uint32_t hash;
	const qse_raddic_const_t* dval = data;

	hash = qse_strcasehash32(dval->name);
	return qse_genhash32_update (&dval->attr, QSE_SIZEOF(dval->attr), hash);
}

static int dict_const_name_cmp (qse_htl_t* htl, const void* one, const void* two)
{
	const qse_raddic_const_t* a = one;
	const qse_raddic_const_t* b = two;

	if (a->attr < b->attr) return -1;
	if (a->attr > b->attr) return 1;

	return qse_strcasecmp(a->name, b->name);
}

static void dict_const_name_free (qse_htl_t* htl, void* data)
{
	QSE_MMGR_FREE (htl->mmgr, data);
}

static qse_uint32_t dict_const_name_hetero_hash (qse_htl_t* htl, const void* one)
{
	qse_uint32_t hash;
	const const_hsd_t* hsd = (const const_hsd_t*)one;

	hash = qse_strcasehash32(hsd->name);
	return qse_genhash32_update (&hsd->attr, QSE_SIZEOF(hsd->attr), hash);
}

static int dict_const_name_hetero_cmp (qse_htl_t* htl, const void* one, const void* two)
{
	const const_hsd_t* hsd = (const const_hsd_t*)one;
	const qse_raddic_const_t* b = (const qse_raddic_const_t*)two;

	if (hsd->attr < b->attr) return -1;
	if (hsd->attr > b->attr) return 1;

	return qse_strcasecmp(hsd->name, b->name);
}

static qse_uint32_t dict_const_value_hash (qse_htl_t* htl, const void* data)
{
	qse_uint32_t hash;
	const qse_raddic_const_t *dval = data;

	hash = qse_genhash32(&dval->attr, QSE_SIZEOF(dval->attr));
	return qse_genhash32_update(&dval->value, QSE_SIZEOF(dval->value), hash);
}

static int dict_const_value_cmp (qse_htl_t* htl, const void* one, const void* two)
{
	const qse_raddic_const_t* a = one;
	const qse_raddic_const_t* b = two;

	if (a->attr < b->attr) return -1;
	if (a->attr > b->attr) return 1;

	if (a->value < b->value) return -1;
	if (a->value > b->value) return 1;

	return 0;
}

/* -------------------------------------------------------------------------- */

int qse_raddic_init (qse_raddic_t* dic, qse_mmgr_t* mmgr)
{
	int count = 0;

	QSE_MEMSET (dic, 0, QSE_SIZEOF(*dic));
	dic->mmgr = mmgr;
	qse_raddic_seterrnum (dic, QSE_RADDIC_ENOERR);

	if (qse_htl_init(&dic->vendors_byname, mmgr, 1) <= -1 || (++count == 0) ||
	    qse_htl_init(&dic->vendors_byvalue, mmgr, 1) <= -1 || (++count == 0) ||
	    qse_htl_init(&dic->attrs_byname, mmgr, 1) <= -1 ||  (++count == 0) ||
	    qse_htl_init(&dic->attrs_byvalue, mmgr, 1) <= -1 ||(++count == 0) ||
	    qse_htl_init(&dic->consts_byname, mmgr, 1) <= -1 || (++count == 0) ||
	    qse_htl_init(&dic->consts_byvalue, mmgr, 1) <= -1 || (++count == 0))
	{
		goto oops;
	}

	qse_htl_sethasher (&dic->vendors_byname, dict_vendor_name_hash);
	qse_htl_setcomper (&dic->vendors_byname, dict_vendor_name_cmp);
	qse_htl_setfreeer (&dic->vendors_byname, dict_vendor_name_free); 

	qse_htl_sethasher (&dic->vendors_byvalue, dict_vendor_value_hash);
	qse_htl_setcomper (&dic->vendors_byvalue, dict_vendor_value_cmp);
	/* no freeer for dic->vendors_byvalue */

	qse_htl_sethasher (&dic->attrs_byname, dict_attr_name_hash);
	qse_htl_setcomper (&dic->attrs_byname, dict_attr_name_cmp);
	qse_htl_setfreeer (&dic->attrs_byname, dict_attr_name_free); 

	qse_htl_sethasher (&dic->attrs_byvalue, dict_attr_value_hash);
	qse_htl_setcomper (&dic->attrs_byvalue, dict_attr_value_cmp);
	/* no freeer for dic->attrs_byvalue */

	qse_htl_sethasher (&dic->consts_byname, dict_const_name_hash);
	qse_htl_setcomper (&dic->consts_byname, dict_const_name_cmp);
	qse_htl_setfreeer (&dic->consts_byname, dict_const_name_free); 

	qse_htl_sethasher (&dic->consts_byvalue, dict_const_value_hash);
	qse_htl_setcomper (&dic->consts_byvalue, dict_const_value_cmp);
	/* no freeer for dic->consts_byvalue */

	return 0;

oops:
	if (count >= 6) qse_htl_fini (&dic->consts_byvalue);
	if (count >= 5) qse_htl_fini (&dic->consts_byname);
	if (count >= 4) qse_htl_fini (&dic->attrs_byvalue);
	if (count >= 3) qse_htl_fini (&dic->attrs_byname);
	if (count >= 2) qse_htl_fini (&dic->vendors_byvalue);
	if (count >= 1) qse_htl_fini (&dic->vendors_byname);

	return -1;
}

void qse_raddic_fini (qse_raddic_t* dic)
{
	qse_raddic_clear (dic);

	qse_htl_fini (&dic->consts_byvalue);
	qse_htl_fini (&dic->consts_byname);
	qse_htl_fini (&dic->attrs_byvalue);
	qse_htl_fini (&dic->attrs_byname);
	qse_htl_fini (&dic->vendors_byvalue);
	qse_htl_fini (&dic->vendors_byname);
}

qse_raddic_t* qse_raddic_open (qse_mmgr_t* mmgr, qse_size_t xtnsize)
{
	qse_raddic_t* dic;

	dic = (qse_raddic_t*) QSE_MMGR_ALLOC (mmgr, QSE_SIZEOF(qse_raddic_t) + xtnsize);
	if (dic)
	{
		if (qse_raddic_init (dic, mmgr) <= -1)
		{
			QSE_MMGR_FREE (mmgr, dic);
			return QSE_NULL;
		}
		else QSE_MEMSET (QSE_XTN(dic), 0, xtnsize);
	}

	return dic;
}

void qse_raddic_close (qse_raddic_t* dic)
{
	qse_raddic_fini (dic);
	QSE_MMGR_FREE (dic->mmgr, dic);
}

int qse_raddic_getopt (qse_raddic_t* dic, qse_raddic_opt_t id, void* value)
{
	switch (id)
	{
		case QSE_RADDIC_TRAIT:
			*(int*)value = dic->opt.trait;
			return 0;
	}

	qse_raddic_seterrnum (dic, QSE_RADDIC_EINVAL);
	return -1;
}

int qse_raddic_setopt (qse_raddic_t* dic, qse_raddic_opt_t id, const void* value)
{
	switch (id)
	{
		case QSE_RADDIC_TRAIT:
			dic->opt.trait = *(const int*)value;
			return 0;
	}

	qse_raddic_seterrnum (dic, QSE_RADDIC_EINVAL);
	return -1;
}


static const qse_char_t* errnum_to_str (qse_raddic_errnum_t errnum)
{
	static const qse_char_t* errstr[] =
	{
		QSE_T("no error"),
		QSE_T("other error"),
		QSE_T("not implemented"),
		QSE_T("subsystem error"),
		QSE_T("internal error that should never have happened"),
		QSE_T("insufficient memory"),
		QSE_T("invalid parameter or data"),
		QSE_T("no data found"),
		QSE_T("existing data found"),
		QSE_T("syntax error")
	};

	return errnum < QSE_COUNTOF(errstr)? errstr[errnum]: QSE_T("unknown error");
}

qse_raddic_errnum_t qse_raddic_geterrnum (qse_raddic_t* dic)
{
	return dic->errnum;
}

const qse_char_t* qse_raddic_geterrmsg (qse_raddic_t* dic)
{
	return dic->errmsg;
}

void qse_raddic_seterrnum (qse_raddic_t* dic, qse_raddic_errnum_t errnum)
{
	dic->errnum = errnum;
	qse_strxcpy (dic->errmsg, QSE_COUNTOF(dic->errmsg), errnum_to_str(errnum));
}

void qse_raddic_seterrfmt (qse_raddic_t* dic, qse_raddic_errnum_t errnum, const qse_char_t* fmt, ...)
{
	va_list ap;
	dic->errnum = errnum;
	va_start (ap, fmt);
	qse_strxvfmt (dic->errmsg, QSE_COUNTOF(dic->errmsg), fmt, ap);
	va_end (ap);
}

/* -------------------------------------------------------------------------- */

void qse_raddic_clear (qse_raddic_t* dic)
{
	int i;

	while (dic->const_fixup)
	{
		const_fixup_t* fixup = dic->const_fixup;
		dic->const_fixup = dic->const_fixup->next;
		QSE_MMGR_FREE (dic->mmgr, fixup->dval);
		QSE_MMGR_FREE (dic->mmgr, fixup);
	}

	dic->last_attr = QSE_NULL;
	for (i = 0; i < QSE_COUNTOF(dic->base_attrs); i++) dic->base_attrs[i] = QSE_NULL;

	qse_htl_clear (&dic->vendors_byname);
	qse_htl_clear (&dic->vendors_byvalue);
	qse_htl_clear (&dic->attrs_byname);
	qse_htl_clear (&dic->attrs_byvalue);
	qse_htl_clear (&dic->consts_byvalue);
	qse_htl_clear (&dic->consts_byname);
}

/* -------------------------------------------------------------------------- */
qse_raddic_vendor_t* qse_raddic_findvendorbyname (qse_raddic_t* dic, const qse_char_t* name)
{
	qse_htl_node_t* np;
	np = qse_htl_heterosearch (&dic->vendors_byname, name, dict_vendor_name_hetero_hash, dict_vendor_name_hetero_cmp);
	if (!np) 
	{
		qse_raddic_seterrfmt (dic, QSE_RADDIC_ENOENT, QSE_T("cannot find a vendor named %s"), name);
		return QSE_NULL;
	}
	return (qse_raddic_vendor_t*)np->data;
}

/*
 *	Return the vendor struct based on the PEC.
 */
qse_raddic_vendor_t* qse_raddic_findvendorbyvalue (qse_raddic_t* dic, unsigned int vendorpec)
{
	qse_htl_node_t* np;
	qse_raddic_vendor_t dv;

	dv.vendorpec = vendorpec;
	np = qse_htl_search (&dic->vendors_byvalue, &dv);
	if (!np) 
	{
		qse_raddic_seterrfmt (dic, QSE_RADDIC_ENOENT, QSE_T("cannot find a vendor of value %u"), vendorpec);
		return QSE_NULL;
	}
	return (qse_raddic_vendor_t*)np->data;
}

qse_raddic_vendor_t* qse_raddic_addvendor (qse_raddic_t* dic, const qse_char_t* name, unsigned int vendorpec)
{
	qse_size_t length;
	qse_raddic_vendor_t* dv, * old_dv;
	qse_htl_node_t* np;

	if (vendorpec <= 0 || vendorpec > 65535) 
	{
		qse_raddic_seterrfmt (dic, QSE_RADDIC_EINVAL, QSE_T("vendor value %u out of accepted range"), vendorpec);
		return QSE_NULL;
	}

	length = qse_strlen(name);

	/* no +1 for the terminating null because dv->name is char[1] */
	dv = QSE_MMGR_ALLOC(dic->mmgr, QSE_SIZEOF(*dv) + (length * QSE_SIZEOF(*name)));
	if (dv == QSE_NULL) 
	{
		qse_raddic_seterrnum (dic, QSE_RADDIC_ENOMEM);
		return QSE_NULL;
	}

	qse_strcpy(dv->name, name);
	dv->vendorpec = vendorpec;
	dv->type = dv->length = 1; /* defaults */
	dv->nextv = QSE_NULL;

	/* return an existing item or insert a new item */
	np = qse_htl_ensert(&dic->vendors_byname, dv);
	if (!np || np->data != dv)
	{
		/* insertion failure or existing item found */
		if (!np) qse_raddic_seterrnum (dic, QSE_RADDIC_ENOMEM);
		else qse_raddic_seterrfmt (dic, QSE_RADDIC_EEXIST, QSE_T("existing vendor %s"), name);

		QSE_MMGR_FREE (dic->mmgr, dv);
		return QSE_NULL;
	}

	/* attempt to update the lookup table by value */
	np = qse_htl_upyank(&dic->vendors_byvalue, dv, (void**)&old_dv);
	if (np)
	{
		/* updated the existing item successfully. 
		 * link the old item to the current item */
		QSE_ASSERT (np->data == dv);
		QSE_ASSERT (dv->vendorpec == old_dv->vendorpec);
		dv->nextv = old_dv;
	}
	else
	{
		/* update failure, this entry must be new. try insertion */
		if (!qse_htl_insert (&dic->vendors_byvalue, dv))
		{
			qse_raddic_seterrnum (dic, QSE_RADDIC_ENOMEM);
			qse_htl_delete (&dic->vendors_byname, dv);
			return QSE_NULL;
		}
	}

	return dv;
}

int qse_raddic_deletevendorbyname (qse_raddic_t* dic, const qse_char_t* name)
{
	qse_raddic_vendor_t* dv, * dv2;

	dv = qse_raddic_findvendorbyname(dic, name);
	if (!dv) return -1;

	dv2 = qse_raddic_findvendorbyvalue(dic, dv->vendorpec);
	QSE_ASSERT (dv2 != QSE_NULL);

	if (dv != dv2)
	{
		qse_raddic_vendor_t* x, * y;

		QSE_ASSERT (qse_strcasecmp(dv->name, dv2->name) != 0);
		QSE_ASSERT (dv->vendorpec == dv2->vendorpec);

		/* when the vendor of the given name is not the first one
		 * referenced by value, i need to unlink the vendor from the
		 * vendor chains with the same ID */
		x = dv2;
		y = QSE_NULL; 
		while (x)
		{
			if (x == dv) 
			{
				if (y) y->nextv = x->nextv;
				break;
			}
			y = x;
			x = x->nextv;
		}
	}
	else
	{
		/* this is the only vendor with the vendor ID. i can 
		 * safely remove it from the lookup table by value */
		qse_htl_delete (&dic->vendors_byvalue, dv);
	}

	/* delete the vendor from the lookup table by name */
	qse_htl_delete (&dic->vendors_byname, dv);
	return 0;
}

int qse_raddic_deletevendorbyvalue (qse_raddic_t* dic, unsigned int vendorpec)
{
	qse_raddic_vendor_t* dv;

	dv = qse_raddic_findvendorbyvalue(dic, vendorpec);
	if (!dv) return -1;

	if (dv->nextv)
	{
		qse_htl_update (&dic->vendors_byvalue, dv->nextv);
	}
	else
	{
		/* this is the only vendor with the vendor ID. i can 
		 * safely remove it from the lookup table by value */
		qse_htl_delete (&dic->vendors_byvalue, dv);
	}

	/* delete the vendor from the lookup table by name */
	qse_htl_delete (&dic->vendors_byname, dv);
	return 0;
}

/* -------------------------------------------------------------------------- */

qse_raddic_attr_t* qse_raddic_findattrbyname (qse_raddic_t* dic, const qse_char_t* name)
{
	qse_htl_node_t* np;
	np = qse_htl_heterosearch (&dic->attrs_byname, name, dict_attr_name_hetero_hash, dict_attr_name_hetero_cmp);
	if (!np) 
	{
		qse_raddic_seterrfmt (dic, QSE_RADDIC_ENOENT, QSE_T("cannot find an attribute named %s"), name);
		return QSE_NULL;
	}
	return (qse_raddic_attr_t*)np->data;
}

qse_raddic_attr_t* qse_raddic_findattrbyvalue (qse_raddic_t* dic, qse_uint32_t attr)
{
	qse_htl_node_t* np;
	qse_raddic_attr_t dv;

	/* simple cache lookup for basic attributes */
	if (attr >= 0 && attr <= 255) return dic->base_attrs[attr];

	dv.attr = attr;
	dv.vendor = QSE_RADDIC_ATTR_VENDOR(attr);
	np = qse_htl_search (&dic->attrs_byvalue, &dv);
	if (!np) 
	{
		qse_raddic_seterrfmt (dic, QSE_RADDIC_ENOENT, QSE_T("cannot find an attribute of value %d and of vendor %d"), QSE_RADDIC_ATTR_VALUE(attr), QSE_RADDIC_ATTR_VENDOR(attr));
		return QSE_NULL;
	}
	return (qse_raddic_attr_t*)np->data;
}

qse_raddic_attr_t* qse_raddic_addattr (qse_raddic_t* dic, const qse_char_t* name, unsigned int vendor, qse_raddic_attr_type_t type, unsigned int value, const qse_raddic_attr_flags_t* flags)
{
	qse_size_t length;
	qse_raddic_attr_t* dv, * old_dv;
	qse_htl_node_t* np;

	if (vendor < 0 || vendor > 65535u) 
	{
		qse_raddic_seterrfmt (dic, QSE_RADDIC_EINVAL, QSE_T("vendor %u out of accepted range"), vendor);
		return QSE_NULL; /* 0 is allowed to mean no vendor */
	}
	if (value < 0 || value > 65535u)  
	{
		/* the upper bound is not 255 because there are vendors defining values in 16-bit format */
		qse_raddic_seterrfmt (dic, QSE_RADDIC_EINVAL, QSE_T("attribute value %u out of accepted range"), value);
		return QSE_NULL;
	}

	length = qse_strlen(name);

	if (vendor > 0)
	{
		/* TODO: validation ... */
	}

	/* no +1 for the terminating null because dv->name is char[1] */
	dv = QSE_MMGR_ALLOC(dic->mmgr, QSE_SIZEOF(*dv) + (length * QSE_SIZEOF(*name)));
	if (dv == QSE_NULL) 
	{
		qse_raddic_seterrnum (dic, QSE_RADDIC_ENOMEM);
		return QSE_NULL;
	}

	qse_strcpy(dv->name, name);
	dv->attr = QSE_RADDIC_ATTR_MAKE(vendor, value);
	dv->vendor = vendor;
	dv->type = type;
	dv->flags = *flags;
	dv->nexta = QSE_NULL;

	/* return an existing item or insert a new item */
	np = qse_htl_ensert(&dic->attrs_byname, dv);
	if (!np || np->data != dv)
	{
		/* insertion failure or existing item found */
		if (!np) qse_raddic_seterrnum (dic, QSE_RADDIC_ENOMEM);
		else qse_raddic_seterrfmt (dic, QSE_RADDIC_EEXIST, QSE_T("existing attribute %s"), name);

		QSE_MMGR_FREE (dic->mmgr, dv);
		return QSE_NULL;
	}

	/* attempt to update the lookup table by value */
	np = qse_htl_upyank(&dic->attrs_byvalue, dv, (void**)&old_dv);
	if (np)
	{
		/* updated the existing item successfully. 
		 * link the old item to the current item */
		QSE_ASSERT (np->data == dv);
		QSE_ASSERT (dv->attr == old_dv->attr);
		dv->nexta = old_dv;
	}
	else
	{
		/* update failure, this entry must be new. try insertion */
		if (!qse_htl_insert (&dic->attrs_byvalue, dv))
		{
			qse_raddic_seterrnum (dic, QSE_RADDIC_ENOMEM);
			qse_htl_delete (&dic->attrs_byname, dv);
			return QSE_NULL;
		}
	}

	if (vendor == 0 && value <= 255) dic->base_attrs[value] = dv; /* cache a base attribute */
	return dv;
}

int qse_raddic_deleteattrbyname (qse_raddic_t* dic, const qse_char_t* name)
{
	qse_raddic_attr_t* dv, * dv2;

	dv = qse_raddic_findattrbyname(dic, name);
	if (!dv) return -1;

	dv2 = qse_raddic_findattrbyvalue(dic, dv->attr);
	QSE_ASSERT (dv2 != QSE_NULL);

	if (dv != dv2)
	{
		qse_raddic_attr_t* x, * y;

		QSE_ASSERT (qse_strcasecmp(dv->name, dv2->name) != 0);
		QSE_ASSERT (dv->attr == dv2->attr);
		QSE_ASSERT (dv->vendor == dv2->vendor);

		/* when the attribute of the given name is not the first one
		 * referenced by value, i need to unlink the attr from the
		 * attr chains with the same ID */
		x = dv2;
		y = QSE_NULL; 
		while (x)
		{
			if (x == dv) 
			{
				if (y) y->nexta = x->nexta;
				break;
			}
			y = x;
			x = x->nexta;
		}
		/* no need to update cache as the deleted item was not the first one formerly */
	}
	else
	{
		/* this is the only attr with the attr ID. i can 
		 * safely remove it from the lookup table by value */
		if (dv == dic->last_attr) dic->last_attr = QSE_NULL;
		if (dv->vendor == 0)
		{
			/* update the cache first */
			dic->base_attrs[dv->vendor] = QSE_NULL;
		}
		qse_htl_delete (&dic->attrs_byvalue, dv);
	}

	/* delete the attr from the lookup table by name */
	qse_htl_delete (&dic->attrs_byname, dv);
	return 0;
}

int qse_raddic_deleteattrbyvalue (qse_raddic_t* dic, qse_uint32_t attr)
{
	qse_raddic_attr_t* dv;

	dv = qse_raddic_findattrbyvalue(dic, attr);
	if (!dv) return -1;

	QSE_ASSERT (QSE_RADDIC_ATTR_VENDOR(attr) == dv->vendor);

	if (QSE_RADDIC_ATTR_VENDOR(attr) == 0) 
	{
		/* update the cache */
		if (dv == dic->last_attr) dic->last_attr = QSE_NULL;
		dic->base_attrs[QSE_RADDIC_ATTR_VALUE(attr)] = dv->nexta;
	}

	if (dv->nexta)
	{
		qse_htl_update (&dic->attrs_byvalue, dv->nexta);
	}
	else
	{
		/* this is the only attr with the attr ID. i can 
		 * safely remove it from the lookup table by value */
		qse_htl_delete (&dic->attrs_byvalue, dv);
	}

	/* delete the attr from the lookup table by name */
	qse_htl_delete (&dic->attrs_byname, dv);
	return 0;
}

/* -------------------------------------------------------------------------- */

qse_raddic_const_t* qse_raddic_findconstbyname (qse_raddic_t* dic, qse_uint32_t attr, const qse_char_t* name)
{
	qse_htl_node_t* np;
	const_hsd_t hsd;

	hsd.name = name;
	hsd.attr = attr;

	np = qse_htl_heterosearch (&dic->consts_byname, &hsd.name, dict_const_name_hetero_hash, dict_const_name_hetero_cmp);
	if (!np) 
	{
		qse_raddic_seterrfmt (dic, QSE_RADDIC_ENOENT, QSE_T("cannot find a constant named %s"), name);
		return QSE_NULL;
	}
	return (qse_raddic_const_t*)np->data;
}

qse_raddic_const_t* qse_raddic_findconstbyvalue (qse_raddic_t* dic, qse_uint32_t attr, qse_uintmax_t value)
{
	qse_htl_node_t* np;
	qse_raddic_const_t dval;

	dval.attr = attr;
	dval.value = value;
	np = qse_htl_search (&dic->consts_byvalue, &dval);
	if (!np)
	{
		qse_raddic_seterrfmt (dic, QSE_RADDIC_ENOENT, QSE_T("cannot find a constant valued %d"), value);
		return QSE_NULL;
	}
	return (qse_raddic_const_t*)np->data;
}

static qse_raddic_const_t* __add_const (qse_raddic_t* dic, qse_raddic_const_t* dval)
{
	qse_htl_node_t* np;
	qse_raddic_const_t* old_dval;

	/* return an existing item or insert a new item */
	np = qse_htl_ensert(&dic->consts_byname, dval);
	if (!np || np->data != dval)
	{
		/* insertion failure or existing item found */
		if (!np) 
		{
			qse_raddic_seterrnum (dic, QSE_RADDIC_ENOMEM);
		}
		else 
		{
			if ((dic->opt.trait & QSE_RADDIC_ALLOW_DUPLICATE_CONST) && 
			    ((qse_raddic_const_t*)np->data)->value == dval->value) 
			{
				QSE_MMGR_FREE (dic->mmgr, dval);
				return np->data;
			}
			qse_raddic_seterrfmt (dic, QSE_RADDIC_EEXIST, QSE_T("existing constant %s"), dval->name);
		}

		QSE_MMGR_FREE (dic->mmgr, dval);
		return QSE_NULL;
	}

	/* attempt to update the lookup table by value */
	np = qse_htl_upyank(&dic->consts_byvalue, dval, (void**)&old_dval);
	if (np)
	{
		/* updated the existing item successfully. 
		 * link the old item to the current item */
		QSE_ASSERT (np->data == dval);
		QSE_ASSERT (dval->value == old_dval->value);
		dval->nextc = old_dval;
	}
	else
	{
		/* update failure, this entry must be new. try insertion */
		if (!qse_htl_insert (&dic->consts_byvalue, dval))
		{
			qse_raddic_seterrnum (dic, QSE_RADDIC_ENOMEM);
			qse_htl_delete (&dic->consts_byname, dval);
			return QSE_NULL;
		}
	}

	return dval;
}

static qse_raddic_const_t* add_const (qse_raddic_t* dic, const qse_char_t* name, const qse_char_t* attrstr, qse_uintmax_t value, const qse_char_t* fn, qse_size_t line)
{
	qse_size_t length;
	qse_raddic_const_t* dval;
	qse_raddic_attr_t* dattr;

	length = qse_strlen(name);

	/* no +1 for the terminating null because dval->name is char[1] */
	dval = QSE_MMGR_ALLOC(dic->mmgr, QSE_SIZEOF(*dval) + (length * QSE_SIZEOF(*name)));
	if (dval == QSE_NULL) 
	{
		qse_raddic_seterrnum (dic, QSE_RADDIC_ENOMEM);
		return QSE_NULL;
	}

	qse_strcpy(dval->name, name);
	dval->value = value;
	dval->nextc = QSE_NULL;

	/*
	 *	Most VALUEs are bunched together by ATTRIBUTE.  We can
	 *	save a lot of lookups on dictionary initialization by
	 *	caching the last attribute.
	 */
	if (dic->last_attr && qse_strcasecmp(attrstr, dic->last_attr->name) == 0)
	{
		dattr = dic->last_attr;
	}
	else
	{
		dattr = qse_raddic_findattrbyname(dic, attrstr);
		dic->last_attr = dattr;
	}

	/*
	 * Remember which attribute is associated with this value, if possible.
	 */
	if (dattr) 
	{
#if 0
		if (dattr->type != value->type)
		{
			qse_raddic_seterrfmt (dic, QSE_RADDIC_EINVAL, QSE_T("conflicts between attribute type(%d) and constant value type(%d)"), (int)dattr->type, (int)value->type);
			return QSE_NULL;
		}
#endif

		dval->attr = dattr->attr;

		switch (dattr->type) 
		{
			case QSE_RADDIC_ATTR_TYPE_UINT8:
				if (value < QSE_TYPE_MIN(qse_uint8_t) || value > QSE_TYPE_MAX(qse_uint8_t)) goto wrong_value; 
				break;

			case QSE_RADDIC_ATTR_TYPE_UINT16:
				if (value < QSE_TYPE_MIN(qse_uint16_t) || value > QSE_TYPE_MAX(qse_uint16_t))  goto wrong_value;
				break;

			case QSE_RADDIC_ATTR_TYPE_UINT32:
				if (value < QSE_TYPE_MIN(qse_uint32_t) || value > QSE_TYPE_MAX(qse_uint32_t))  goto wrong_value;
				break;

			case QSE_RADDIC_ATTR_TYPE_UINT64:
				if (value < QSE_TYPE_MIN(qse_uint64_t) || value > QSE_TYPE_MAX(qse_uint64_t))  goto wrong_value;
				break;

			case QSE_RADDIC_ATTR_TYPE_INT8:
				if (value < QSE_TYPE_MIN(qse_int8_t) || value > QSE_TYPE_MAX(qse_int8_t)) goto wrong_value; 
				break;

			case QSE_RADDIC_ATTR_TYPE_INT16:
				if (value < QSE_TYPE_MIN(qse_int16_t) || value > QSE_TYPE_MAX(qse_int16_t))  goto wrong_value;
				break;

			case QSE_RADDIC_ATTR_TYPE_INT32:
				if (value < QSE_TYPE_MIN(qse_int32_t) || value > QSE_TYPE_MAX(qse_int32_t))  goto wrong_value;
				break;

			case QSE_RADDIC_ATTR_TYPE_INT64:
				if (value < QSE_TYPE_MIN(qse_int64_t) || value > QSE_TYPE_MAX(qse_int64_t))  goto wrong_value;
				break;

				/*
				 *	Allow octets for now, because
				 *	of dictionary.cablelabs
				 */
			case QSE_RADDIC_ATTR_TYPE_OCTETS:
				break;

			default: /* cannot define VALUE for other types */
			wrong_value:
				qse_raddic_seterrfmt (dic, QSE_RADDIC_EINVAL, QSE_T("value %jd for a constant %s not allowed for an attribute %s of type %d"), value, name, attrstr, (int)dattr->type);
				QSE_MMGR_FREE (dic->mmgr, dval);
				return QSE_NULL;
		}

		dattr->flags.has_value = 1;
	} 
	else
	{
		if (fn && (dic->opt.trait & QSE_RADDIC_ALLOW_CONST_WITHOUT_ATTR))
		{
			const_fixup_t* fixup;
			qse_size_t attrstrlen, fnlen;

			attrstrlen = qse_strlen(attrstr);
			fnlen = qse_strlen(fn);

			/* TODO: don't copy fn again and again */
			fixup = QSE_MMGR_ALLOC(dic->mmgr, QSE_SIZEOF(*fixup) + ((attrstrlen + fnlen + 1) * QSE_SIZEOF(*attrstr)));
			if (!fixup)
			{
				qse_raddic_seterrnum (dic, QSE_RADDIC_ENOMEM);
				QSE_MMGR_FREE (dic->mmgr, dval);
				return QSE_NULL;
			}

			QSE_MEMSET (fixup, 0, QSE_SIZEOF(*fixup));
			qse_strcpy (fixup->attrstr, attrstr);
			fixup->dval = dval;
			fixup->next = dic->const_fixup;
			fixup->line = line;
			fixup->fn = fixup->attrstr + attrstrlen + 1;
			qse_strcpy (fixup->fn, fn); /* TODO: don't copy fn again and again */

			dic->const_fixup = fixup;

			return dval; /* this is not complete */
		}
		else
		{
			qse_raddic_seterrfmt (dic, QSE_RADDIC_EINVAL, QSE_T("attribute %s not found for a constant"), attrstr, name);
			QSE_MMGR_FREE (dic->mmgr, dval);
			return QSE_NULL;
		}
	}

	return __add_const(dic, dval);
}

qse_raddic_const_t* qse_raddic_addconst (qse_raddic_t* dic, const qse_char_t* name, const qse_char_t* attrstr, qse_uintmax_t value)
{
	return add_const (dic, name, attrstr, value, QSE_NULL, 0);
}

int qse_raddic_deleteconstbyname (qse_raddic_t* dic, qse_uint32_t attr, const qse_char_t* name)
{
	qse_raddic_const_t* dv, * dv2;

	dv = qse_raddic_findconstbyname(dic, attr, name);
	if (!dv) return -1;

	QSE_ASSERT (attr == dv->attr);
	dv2 = qse_raddic_findconstbyvalue(dic, attr, dv->value);
	QSE_ASSERT (dv2 != QSE_NULL);

	if (dv != dv2)
	{
		qse_raddic_const_t* x, * y;

		QSE_ASSERT (qse_strcasecmp(dv->name, dv2->name) != 0);
		QSE_ASSERT (dv->value ==dv2->value);
		QSE_ASSERT (dv->attr == dv2->attr);

		/* when the constibute of the given name is not the first one
		 * referenced by value, i need to unlink the const from the
		 * const chains with the same ID */
		x = dv2;
		y = QSE_NULL; 
		while (x)
		{
			if (x == dv) 
			{
				if (y) y->nextc = x->nextc;
				break;
			}
			y = x;
			x = x->nextc;
		}
		/* no need to update cache as the deleted item was not the first one formerly */
	}
	else
	{
		/* this is the only const with the const ID. i can 
		 * safely remove it from the lookup table by value */
		qse_htl_delete (&dic->consts_byvalue, dv);
	}

	/* delete the const from the lookup table by name */
	qse_htl_delete (&dic->consts_byname, dv);
	return 0;
}

int qse_raddic_deleteconstbyvalue (qse_raddic_t* dic, qse_uint32_t attr, qse_uintmax_t value)
{
	qse_raddic_const_t* dv;

	dv = qse_raddic_findconstbyvalue(dic, attr, value);
	if (!dv) return -1;

	if (dv->nextc)
	{
		qse_htl_update (&dic->consts_byvalue, dv->nextc);
	}
	else
	{
		/* this is the only const with the const ID. i can 
		 * safely remove it from the lookup table by value */
		qse_htl_delete (&dic->consts_byvalue, dv);
	}

	/* delete the const from the lookup table by name */
	qse_htl_delete (&dic->consts_byname, dv);
	return 0;
}

/* -------------------------------------------------------------------------- */

static int str2argv (qse_char_t* str, qse_char_t* argv[], int max_argc)
{
	int nflds, i;
	qse_char_t* ptr;

	nflds = qse_strspl (str, QSE_T(""), QSE_T('\0'), QSE_T('\0'), QSE_T('\0'));
	if (nflds <= 0) return -1;

	ptr = str;
	for (i = 0; i < nflds; i++)
	{
		argv[i] = ptr;
		while (*ptr != QSE_T('\0')) ptr++;
		ptr++;
	}

	return nflds;
}

static int sscanf_i (qse_raddic_t* dic, const qse_char_t* str, int* pvalue)
{
	qse_long_t v;
	const qse_char_t* end;

	QSE_STRTONUM (v, str, &end, 0);
	if (*end != '\0') 
	{
		qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("invalid number - %s"), str);
		return -1;
	}
	*pvalue = v;
	return 0;
}

static int sscanf_ui (qse_raddic_t* dic, const qse_char_t* str, qse_uintmax_t* pvalue)
{
	qse_uintmax_t v;
	const qse_char_t* end;

	if (!QSE_ISDIGIT(*str) && *str != QSE_T('+')) 
	{
		qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("invalid unsigned number - %s"), str);
		return -1;
	}

	QSE_STRTONUM (v, str, &end, 0);

	if (*end != '\0')
	{
		qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("invalid unsigned number - %s"), str);
		return -1;
	}

	*pvalue = v;
	return 0;
}

static int sscanf_ui32 (qse_raddic_t* dic, const qse_char_t* str, qse_uint32_t* pvalue, qse_uint32_t* pvalue2)
{
	qse_long_t v, v2;
	const qse_char_t* end;
	const qse_char_t* start2 = QSE_NULL;

	if (!QSE_ISDIGIT(*str) && *str != QSE_T('+')) 
	{
		qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("invalid unsigned number - %s"), str);
		return -1;
	}

	QSE_STRTONUM (v, str, &end, 0);
	if (pvalue2 && *end == '.')
	{
		start2 = end + 1;
		QSE_STRTONUM (v2, start2, &end, 0);
	}

	if (*end != '\0')
	{
		qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("invalid unsigned number - %s"), str);
		return -1;
	}

	*pvalue = v;
	if (start2) 
	{
		*pvalue2 = v2;
		return (int)(end - start2); /* the value must not be very long. so i cast it to 'int' */
	}

	return 0;
}

/*
 *	Process the ATTRIBUTE command
 */
static int process_attribute (
	qse_raddic_t* dic, const qse_char_t* fn, const qse_size_t line,
	int block_vendor, qse_raddic_attr_t* block_tlv, qse_char_t** argv, int argc)
{
	unsigned int vendor = 0;
	qse_uintmax_t value;
	int type;
	qse_raddic_attr_flags_t flags;
	qse_char_t* p;
	int typelen = -1;

	if ((argc < 3) || (argc > 4)) 
	{
		qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd]: invalid ATTRIBUTE line"), fn, line);
		return -1;
	}

	QSE_MEMSET (&flags, 0, QSE_SIZEOF(flags));

	/*
	 *	Validate all entries
	 */
	if (sscanf_ui(dic, argv[1], &value) <= -1 || value > QSE_TYPE_MAX(qse_uint16_t)) 
	{
		qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd]: invalid attribute value  %s"), fn, line, argv[1]);
		return -1;
	}

	/*
	 *	find the type of the attribute.
	 */
	p = qse_strchr(argv[2], QSE_T('[')); /* for instance, octets[20] */
	if (p) 
	{
		qse_char_t* q;

		*p = QSE_T('\0');
		q = qse_strchr(p + 1, QSE_T(']'));
		if (!q || q[1] != QSE_T('\0')) 
		{
			*p = QSE_T('[');
			goto invalid_type;
		}

		*q = QSE_T('\0');
		if (sscanf_i(dic, p + 1, &typelen) <= -1 || typelen <= 0 || typelen > 253)
		{
			*p = QSE_T('[');
			*q = QSE_T(']');
			goto invalid_type;
		}

		flags.length = typelen;
	}

	type = str_to_type(dic, argv[2], &typelen);
	if (type < 0) 
	{
	invalid_type:
		qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd]: invalid attribute type \"%s\""), fn, line, argv[2]);
		return -1;
	}

	if (flags.length <= 0 && typelen >= 0) flags.length = typelen;

	/*
	 *	Only look up the vendor if the string
	 *	is non-empty.
	 */
	
	if (argc >= 4) 
	{
		qse_char_t* key, * next;

		key = argv[3];
		do 
		{
			next = qse_strchr(key, QSE_T(','));
			if (next) *(next++) = QSE_T('\0');

			if (qse_strcasecmp(key, QSE_T("has_tag")) == 0 ||
			    qse_strcasecmp(key, QSE_T("has_tag=1")) == 0) 
			{
				flags.has_tag = 1;
			}
			else if (qse_strzcasecmp(key, QSE_T("encrypt="), 8) == 0) 
			{
				/* Encryption method, defaults to 0 (none).
				   Currently valid is just type 2,
				   Tunnel-Password style, which can only
				   be applied to strings. */
				int ev;
				if (sscanf_i(dic, key + 8, &ev) <= -1 || ev < QSE_RADDIC_ATTR_FLAG_ENCRYPT_NONE || ev > QSE_RADDIC_ATTR_FLAG_ENCRYPT_OTHER)
				{
					qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR,  QSE_T("%s[%zd] invalid option %s"), fn, line, key);
					return -1;
				}

				flags.encrypt = ev;
			} 
			else if (qse_strcasecmp(key, QSE_T("array")) == 0) 
			{
				flags.array = 1;
			}
			else if (qse_strcasecmp(key, QSE_T("concat")) == 0) 
			{
				flags.concat = 1;
			}
			else if (qse_strcasecmp(key, QSE_T("internal")) == 0) 
			{
				flags.internal = 1;
			}
			else 
			{
				qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR,  QSE_T("%s[%zd]: unknown option \"%s\""), fn, line, key);
				return -1;
			}

			key = next;
			if (key && !*key) break;
		}
		while (key);
	}

	if (block_vendor) vendor = block_vendor;

	/*
	 *	Special checks for tags, they make our life much more
	 *	difficult.
	 */
	if (flags.has_tag) 
	{
		/*
		 *	Only string, octets, and integer can be tagged.
		 */
		switch (type) 
		{
			case QSE_RADDIC_ATTR_TYPE_STRING:
			case QSE_RADDIC_ATTR_TYPE_UINT32:
				break;

			default:
				qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd]: attribute of this type cannot be tagged"), fn, line);
				return -1;
		}
	}

	if (type == QSE_RADDIC_ATTR_TYPE_TLV) 
	{
		flags.has_tlv = 1;
	}

// TODO: what is tlv???
	if (block_tlv) 
	{
		/*
		 *	TLV's can be only one octet.
		 */
		if ((value <= 0) || (value > 255)) 
		{
			qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR,  QSE_T("%s[%zd]: sub-tlv's cannot have value > 255"), fn, line);
			return -1;
		}

#if 0
		if (flags.encrypt != QSE_RADDIC_ATTR_FLAG_ENCRYPT_NONE) 
		{
			qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR,  QSE_T("%s[%zd]: sub-tlv's cannot be encrypted"), fn, line);
			return -1;
		}
#endif

		value <<= 8;
		value |= (block_tlv->attr & 0xffff);
		flags.is_tlv = 1;
	}

	/*
	 *	Add it in.
	 */
	if (qse_raddic_addattr(dic, argv[0], vendor, type, value, &flags) == QSE_NULL) 
	{
		qse_strcpy (dic->errmsg2, dic->errmsg);
		qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd]: cannot add attribute - %s"), fn, line, dic->errmsg2);
		return -1;
	}

	return 0;
}

static int process_constant(qse_raddic_t* dic, const qse_char_t* fn, const qse_size_t line, qse_char_t** argv, int argc)
{
	qse_uintmax_t value;
	int n;

	if (argc != 3) 
	{
		qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd]: invalid VALUE line"), fn, line);
		return -1;
	}

	/*
	 *	For Compatibility, skip "Server-Config"
	 */
	if (qse_strcasecmp(argv[0], QSE_T("Server-Config")) == 0) return 0;

	/*
	 *	Validate all entries
	 */

	if ((n = sscanf_ui(dic, argv[2], &value)) <= -1)
	{
		qse_strcpy (dic->errmsg2, dic->errmsg);
		qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd]: invalid constant value - %s"), fn, line, dic->errmsg2);
		return -1;
	}

	if (add_const(dic, argv[1], argv[0], value, fn, line) == QSE_NULL) 
	{
		qse_strcpy (dic->errmsg2, dic->errmsg);
		qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd]: cannot add a constant \"%s\" - %s"), fn, line, argv[1], dic->errmsg2);
		return -1;
	}

	return 0;
}

/*
 *	Process the VENDOR command
 */
static int process_vendor (qse_raddic_t* dic, const qse_char_t* fn, const qse_size_t line, qse_char_t** argv,  int argc)
{
	int value;
	int continuation = 0;
	const qse_char_t* format = QSE_NULL;

	if ((argc < 2) || (argc > 3)) 
	{
		qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR,  QSE_T("%s[%zd] invalid VENDOR entry"), fn, line);
		return -1;
	}

	/*
	 *	 Validate all entries
	 */
	if (!QSE_ISDIGIT(argv[1][0])) 
	{
		qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd]: invalid value"), fn, line);
		return -1;
	}
	value = qse_strtoi(argv[1], 0);

	/* Create a new VENDOR entry for the list */
	if (qse_raddic_addvendor(dic, argv[0], value) == QSE_NULL) 
	{
		qse_strcpy (dic->errmsg2, dic->errmsg);
		qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd]: cannot add a vendor - %s"), fn, line, dic->errmsg2);
		return -1;
	}

	/*
	 *	Look for a format statement
	 */
	if (argc == 3) 
	{
		format = argv[2];
	}
#if 0
	else if (value == VENDORPEC_USR) 
	{ /* catch dictionary screw-ups */
		format = QSE_T("format=4,0");
	}
	else if (value == VENDORPEC_LUCENT) 
	{
		format = QSE_T("format=2,1");
	}
	else if (value == VENDORPEC_STARENT) 
	{
		format = QSE_T("format=2,2");
	} /* else no fixups to do */
#endif

	if (format) 
	{
		int type, length;
		const qse_char_t* p;
		qse_raddic_vendor_t *dv;

		if (qse_strzcasecmp(format, QSE_T("format="), 7) != 0) 
		{
			qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd]: Invalid format for VENDOR.  Expected \"format=\", got \"%s\""), fn, line, format);
			return -1;
		}

		p = format + 7;
		if (qse_strlen(p) < 3 || !QSE_ISDIGIT(p[0]) || p[1] != QSE_T(',') || !QSE_ISDIGIT(p[2]) || (p[3] && p[3] != QSE_T(','))) 
		{
			qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd]: Invalid format for VENDOR.  Expected text like \"1,1\", got \"%s\""), fn, line, p);
			return -1;
		}

		type = p[0] - QSE_T('0');
		length = p[2] - QSE_T('0');

		if (p[3] == QSE_T(',')) 
		{
			if (p[4] != QSE_T('c') || p[5] != QSE_T('\0')) 
			{
				qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd]: Invalid format for VENDOR.  Expected text like \"1,1\", got \"%s\""), fn, line, p);
				return -1;
			}
			continuation = 1;
		}

		dv = qse_raddic_findvendorbyvalue(dic, value);
		if (!dv) 
		{
			qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd]: Failed adding format for VENDOR"), fn, line);
			return -1;
		}

		if ((type != 1) && (type != 2) && (type != 4)) 
		{
			qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd]: invalid type value %d for VENDOR"), fn, line, type);
			return -1;
		}

		if ((length != 0) && (length != 1) && (length != 2)) 
		{
			qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd]: invalid length value %d for VENDOR"), fn, line, length);
			return -1;
		}

		dv->type = type;
		dv->length = length;
		dv->flags = continuation;
	}

	return 0;
}

static int load_file (qse_raddic_t* dic, const qse_char_t* fn, const qse_char_t* src_file, qse_size_t src_line)
{
	qse_sio_t* sio = QSE_NULL;
	qse_char_t buf[256]; /* TODO: is this a good size? */
	qse_char_t* p;
	qse_size_t line = 0;
	qse_raddic_vendor_t* vendor;
	qse_raddic_vendor_t* block_vendor;

	qse_char_t* argv[16]; /* TODO: what is the best size? */
	int argc;
	qse_raddic_attr_t* da, * block_tlv = QSE_NULL;
	qse_char_t* fname = (qse_char_t*)fn;

	if (!qse_isabspath(fn) && src_file)
	{
		const qse_char_t* b = qse_basename(src_file);
		if (b != src_file)
		{
			fname = qse_substbasenamedup (src_file, fn, dic->mmgr);
			if (!fname)
			{
				qse_raddic_seterrfmt (dic, QSE_RADDIC_ENOMEM, QSE_T("%s[%zd]: out of memory before including %s"), fn);
				return -1;
			}
			qse_canonpath (fname, fname, 0);
		}
	}

	sio = qse_sio_open (dic->mmgr, 0, fname, QSE_SIO_READ);
	if (!sio)
	{
		if (src_file)
			qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd]: cannot open %s"), src_file, src_line, fname);
		else
			qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("cannot open \"%s\""), fname);
		goto oops;
	}

	block_vendor = 0;

	while (qse_sio_getstr (sio, buf, QSE_COUNTOF(buf)) > 0) 
	{
		line++;

		qse_strpac (buf);
		if (buf[0] == QSE_T('\0') || buf[0] == QSE_T('#')) continue;

		/*
		 *  Comment characters should NOT be appearing anywhere but
		 *  as start of a comment;
		 */
		p = qse_strchr (buf, QSE_T('#'));
		if (p) *p = QSE_T('\0');

		argc = str2argv(buf, argv, QSE_COUNTOF(argv));
		if (argc == 0) continue;

		if (argc == 1) 
		{
			qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR,  QSE_T("%s[%zd] invalid entry \"%s\""), fname, line, argv[0]);
			goto oops;
		}

		/*
		 *	Process VALUE lines.
		 */
		if (qse_strcasecmp(argv[0], QSE_T("VALUE")) == 0) 
		{
			if (process_constant(dic, fname, line, argv + 1, argc - 1) == -1) goto oops;
			continue;
		}

		/*
		 *	Perhaps this is an attribute.
		 */
		if (qse_strcasecmp(argv[0], QSE_T("ATTRIBUTE")) == 0) 
		{
			if (process_attribute(dic, fname, line, (block_vendor? block_vendor->vendorpec: 0), block_tlv, argv + 1, argc - 1) == -1) goto oops;
			continue;
		}

		/*
		 *	See if we need to import another dictionary.
		 */
		if (qse_strcasecmp(argv[0], QSE_T("$INCLUDE")) == 0) 
		{
			if (load_file(dic, argv[1], fname, line) < 0) goto oops;
			continue;
		} /* $INCLUDE */

		/*
		 *	Process VENDOR lines.
		 */
		if (qse_strcasecmp(argv[0], QSE_T("VENDOR")) == 0) 
		{
			if (process_vendor(dic, fname, line, argv + 1, argc - 1) == -1)  goto oops;
			continue;
		}

		if (qse_strcasecmp(argv[0], QSE_T("BEGIN-TLV")) == 0) 
		{
			if (argc != 2) 
			{
				qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd] invalid BEGIN-TLV entry"), fname, line);
				goto oops;
			}

			da = qse_raddic_findattrbyname (dic, argv[1]);
			if (!da) 
			{
				qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd]: unknown attribute %s"), fname, line, argv[1]);
				goto oops;
			}

			if (da->type != QSE_RADDIC_ATTR_TYPE_TLV) 
			{
				qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd]: attribute %s is not of type tlv"), fname, line, argv[1]);
				goto oops;
			}

			block_tlv = da;
			continue;
		} /* BEGIN-TLV */

		if (qse_strcasecmp(argv[0], QSE_T("END-TLV")) == 0) 
		{
			if (argc != 2) 
			{
				qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd] invalid END-TLV entry"), fname, line);
				goto oops;
			}

			da = qse_raddic_findattrbyname(dic, argv[1]);
			if (!da) 
			{
				qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd]: unknown attribute %s"), fname, line, argv[1]);
				goto oops;
			}

			if (da != block_tlv) 
			{
				qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd]: END-TLV %s does not match any previous BEGIN-TLV"), fname, line, argv[1]);
				goto oops;
			}
			block_tlv = QSE_NULL;
			continue;
		} /* END-VENDOR */

		if (qse_strcasecmp(argv[0], QSE_T("BEGIN-VENDOR")) == 0) 
		{
			if (argc != 2) 
			{
				qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd] invalid BEGIN-VENDOR entry"), fname, line);
				goto oops;
			}

			vendor = qse_raddic_findvendorbyname (dic, argv[1]);
			if (!vendor) 
			{
				qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd]: unknown vendor %s"), fname, line, argv[1]);
				goto oops;
			}
			block_vendor = vendor;
			continue;
		} /* BEGIN-VENDOR */

		if (qse_strcasecmp(argv[0], QSE_T("END-VENDOR")) == 0) 
		{
			if (argc != 2) 
			{
				qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd] invalid END-VENDOR entry"), fname, line);
				goto oops;
			}

			vendor = qse_raddic_findvendorbyname(dic, argv[1]);
			if (!vendor) 
			{
				qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd]: unknown vendor \"%s\""), fname, line, argv[1]);
				goto oops;
			}

			if (vendor != block_vendor)
			{
				qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR,
					QSE_T("%s[%zd]: END-VENDOR %s does not match any previous BEGIN-VENDOR"), fname, line, argv[1]);
				goto oops;
			}
			block_vendor = 0;
			continue;
		} /* END-VENDOR */

		/*
		 *	Any other string: We don't recognize it.
		 */
		qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd] invalid keyword \"%s\""), fname, line, argv[0]);
		goto oops;
	}

	qse_sio_close (sio);
	if (fname != fn) QSE_MMGR_FREE (dic->mmgr, fname);
	return 0;

oops:
	if (sio) qse_sio_close (sio);
	if (fname != fn) QSE_MMGR_FREE (dic->mmgr, fname);
	return -1;
}

int qse_raddic_load (qse_raddic_t* dic, const qse_char_t* file)
{
	int n;

	n = load_file (dic, file, QSE_NULL, 0);

	while (dic->const_fixup)
	{
		qse_raddic_attr_t* attr;
		const_fixup_t* fixup = dic->const_fixup;
		dic->const_fixup = dic->const_fixup->next;

		if (n >= 0)
		{
			attr = qse_raddic_findattrbyname (dic, fixup->attrstr);
			if (attr)
			{
				fixup->dval->attr = attr->attr;
				attr->flags.has_value = 1;
				if (__add_const(dic, fixup->dval) != QSE_NULL) goto fixed;
			}
			else
			{
				qse_raddic_seterrfmt (dic, QSE_RADDIC_ESYNERR, QSE_T("%s[%zd]: constant \"%s\" defined for an unknown attribute \"%s\""), fixup->fn, fixup->line, fixup->dval->name, fixup->attrstr);
				n = -1;
			}
		}

		QSE_MMGR_FREE (dic->mmgr, fixup->dval);
	fixed:
		QSE_MMGR_FREE (dic->mmgr, fixup);
	}

	return n;
}
