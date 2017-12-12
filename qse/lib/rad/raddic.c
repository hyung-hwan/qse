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
#include <qse/cmn/htl.h>
#include <qse/cmn/str.h>
#include "../cmn/mem-prv.h"
#include <qse/si/sio.h>

struct qse_raddic_t
{
	qse_mmgr_t* mmgr;

	qse_htl_t vendors_byname;
	qse_htl_t vendors_byvalue;
	qse_htl_t attrs_byname;
	qse_htl_t attrs_byvalue;
	qse_htl_t consts_byvalue;
	qse_htl_t consts_byname;

	qse_raddic_attr_t* last_attr;
	qse_raddic_attr_t* base_attrs[256];
};


typedef struct name_id_t name_id_t;
struct name_id_t
{
	const char      *name;
	int             id;
};

static const name_id_t type_table[] = 
{
	{ "integer",    QSE_RADDIC_ATTR_TYPE_INTEGER },
	{ "string",     QSE_RADDIC_ATTR_TYPE_STRING },
	{ "ipaddr",     QSE_RADDIC_ATTR_TYPE_IPADDR },
	{ "date",       QSE_RADDIC_ATTR_TYPE_DATE },
	{ "abinary",    QSE_RADDIC_ATTR_TYPE_ABINARY },
	{ "octets",     QSE_RADDIC_ATTR_TYPE_OCTETS },
	{ "ifid",       QSE_RADDIC_ATTR_TYPE_IFID },
	{ "ipv6addr",   QSE_RADDIC_ATTR_TYPE_IPV6ADDR },
	{ "ipv6prefix", QSE_RADDIC_ATTR_TYPE_IPV6PREFIX },
	{ "byte",       QSE_RADDIC_ATTR_TYPE_BYTE },
	{ "short",      QSE_RADDIC_ATTR_TYPE_SHORT },
	{ "ether",      QSE_RADDIC_ATTR_TYPE_ETHERNET },
	{ "combo-ip",   QSE_RADDIC_ATTR_TYPE_COMBO_IP },
	{ "tlv",        QSE_RADDIC_ATTR_TYPE_TLV },
	{ "signed",     QSE_RADDIC_ATTR_TYPE_SIGNED },
	{ QSE_NULL,     0 }
};

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
	return a->attr - b->attr;
}

/* -------------------------------------------------------------------------- */

struct const_hsd_t
{
	const qse_char_t* name;
	int attr;
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
	int x;

	x = a->attr - b->attr;
	if (x != 0) return x;

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
	int x;

	x = hsd->attr - b->attr;
	if (x != 0) return x;

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
	const qse_raddic_const_t *a = one;
	const qse_raddic_const_t *b = two;
	int x;

	x = a->attr - b->attr;
	if (x != 0) return x;

	return a->value - b->value;
}

/* -------------------------------------------------------------------------- */

int qse_raddic_init (qse_raddic_t* dic, qse_mmgr_t* mmgr)
{
	QSE_MEMSET (dic, 0, QSE_SIZEOF(*dic));
	dic->mmgr = mmgr;

	qse_htl_init (&dic->vendors_byname, mmgr, 1);
	qse_htl_init (&dic->vendors_byvalue, mmgr, 1);
	qse_htl_init (&dic->attrs_byname, mmgr, 1);
	qse_htl_init (&dic->attrs_byvalue, mmgr, 1);
	qse_htl_init (&dic->consts_byname, mmgr, 1);
	qse_htl_init (&dic->consts_byvalue, mmgr, 1);

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
}

void qse_raddic_fini (qse_raddic_t* dic)
{
	qse_htl_fini (&dic->vendors_byname);
	qse_htl_fini (&dic->vendors_byvalue);
	qse_htl_fini (&dic->attrs_byname);
	qse_htl_fini (&dic->attrs_byvalue);
	qse_htl_fini (&dic->consts_byvalue);
	qse_htl_fini (&dic->consts_byname);
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

/* -------------------------------------------------------------------------- */
qse_raddic_vendor_t* qse_raddic_findvendorbyname (qse_raddic_t* dic, const qse_char_t* name)
{
	qse_htl_node_t* np;
	np = qse_htl_heterosearch (&dic->vendors_byname, name, dict_vendor_name_hetero_hash, dict_vendor_name_hetero_cmp);
	if (!np) return QSE_NULL;
	return (qse_raddic_vendor_t*)np->data;
}

/*
 *	Return the vendor struct based on the PEC.
 */
qse_raddic_vendor_t* qse_raddic_findvendorbyvalue (qse_raddic_t* dic, int vendorpec)
{
	qse_htl_node_t* np;
	qse_raddic_vendor_t dv;

	dv.vendorpec = vendorpec;
	np = qse_htl_search (&dic->vendors_byvalue, &dv);
	if (!np) return QSE_NULL;
	return (qse_raddic_vendor_t*)np->data;
}

qse_raddic_vendor_t* qse_raddic_addvendor (qse_raddic_t* dic, const qse_char_t* name, int vendorpec)
{
	qse_size_t length;
	qse_raddic_vendor_t* dv, * old_dv;
	qse_htl_node_t* np;

	if (vendorpec <= 0 || vendorpec > 65535) return QSE_NULL;

	length = qse_strlen(name);

	/* no +1 for the terminating null because dv->name is char[1] */
	dv = QSE_MMGR_ALLOC(dic->mmgr, QSE_SIZEOF(*dv) + (length * QSE_SIZEOF(*name)));
	if (dv == QSE_NULL) return QSE_NULL;

	qse_strcpy(dv->name, name);
	dv->vendorpec = vendorpec;
	dv->type = dv->length = 1; /* defaults */
	dv->nextv = QSE_NULL;

	/* return an existing item or insert a new item */
	np = qse_htl_ensert(&dic->vendors_byname, dv);
	if (!np || np->data != dv)
	{
		/* insertion failure or existing item found */
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

int qse_raddic_deletevendorbyvalue (qse_raddic_t* dic, int vendorpec)
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
	if (!np) return QSE_NULL;
	return (qse_raddic_attr_t*)np->data;
}

qse_raddic_attr_t* qse_raddic_findattrbyvalue (qse_raddic_t* dic, int attr)
{
	qse_htl_node_t* np;
	qse_raddic_attr_t dv;

	/* simple cache lookup for basic attributes */
	if (attr >= 0 && attr <= 255) return dic->base_attrs[attr];

	dv.attr = attr;
	dv.vendor = QSE_RADDIC_ATTR_VENDOR(attr);
	np = qse_htl_search (&dic->attrs_byvalue, &dv);
	if (!np) return QSE_NULL;
	return (qse_raddic_attr_t*)np->data;
}

qse_raddic_attr_t* qse_raddic_addattr (qse_raddic_t* dic, const qse_char_t* name, int vendor, int type, int value, const qse_raddic_attr_flags_t* flags)
{
	qse_size_t length;
	qse_raddic_attr_t* dv, * old_dv;
	qse_htl_node_t* np;

	if (vendor < 0 || vendor > 65535) return QSE_NULL; /* 0 is allowed to mean no vendor */
	if (value < 0 || value > 255) return QSE_NULL;

	length = qse_strlen(name);

	if (vendor > 0)
	{
		/* TODO: validation ... */
	}

	/* no +1 for the terminating null because dv->name is char[1] */
	dv = QSE_MMGR_ALLOC(dic->mmgr, QSE_SIZEOF(*dv) + (length * QSE_SIZEOF(*name)));
	if (dv == QSE_NULL) return QSE_NULL;

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
			qse_htl_delete (&dic->attrs_byname, dv);
			return QSE_NULL;
		}
	}

	if (vendor == 0) dic->base_attrs[value] = dv; /* cache a base attribute */
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

int qse_raddic_deleteattrbyvalue (qse_raddic_t* dic, int attr)
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

qse_raddic_const_t* qse_raddic_findconstbyname (qse_raddic_t* dic, int attr, const qse_char_t* name)
{
	qse_htl_node_t* np;
	const_hsd_t hsd;

	hsd.name = name;
	hsd.attr = attr;

	np = qse_htl_heterosearch (&dic->consts_byname, &hsd.name, dict_const_name_hetero_hash, dict_const_name_hetero_cmp);
	if (!np) return QSE_NULL;
	return (qse_raddic_const_t*)np->data;
}

qse_raddic_const_t* qse_raddic_findconstbyvalue (qse_raddic_t* dic, int attr, int value)
{
	qse_htl_node_t* np;
	qse_raddic_const_t dval;

	dval.attr = attr;
	dval.value = value;
	np = qse_htl_search (&dic->consts_byvalue, &dval);
	if (!np) return QSE_NULL;
	return (qse_raddic_const_t*)np->data;
}

qse_raddic_const_t* qse_raddic_addconst (qse_raddic_t* dic, const qse_char_t* name, const qse_char_t* attrstr, int value)
{
	qse_size_t length;
	qse_raddic_const_t* dval, * old_dval;
	qse_raddic_attr_t* dattr;
	qse_htl_node_t* np;

	length = qse_strlen(name);

	/* no +1 for the terminating null because dval->name is char[1] */
	dval = QSE_MMGR_ALLOC(dic->mmgr, QSE_SIZEOF(*dval) + (length * QSE_SIZEOF(*name)));
	if (dval == QSE_NULL) return QSE_NULL;

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
		if (dattr->flags.has_value_alias) 
		{
			/* cannot add a VALUE for an attribute having a VALUE_ALIAS */
			return QSE_NULL;
		}

		dval->attr = dattr->attr;

#if 0
		/*
		 * Enforce valid values
		 * Don't worry about fixups...
		 */
		switch (dattr->type) 
		{
			case QSE_RADDIC_ATTR_TYPE_BYTE:
				if (value < 0 || value > 255) goto wrong_value; 
				break;

			case QSE_RADDIC_ATTR_TYPE_SHORT:
				if (value < 0 || value > 65535)  goto wrong_value;
				break;

				/*
				 *	Allow octets for now, because
				 *	of dictionary.cablelabs
				 */
			case QSE_RADDIC_ATTR_TYPE_OCTETS:
			case QSE_RADDIC_ATTR_TYPE_INTEGER:
				break;

			default: /* cannot define VALUE for other types */
			wrong_value:
				QSE_MMGR_FREE (dic->mmgr, dval);
				return QSE_NULL;
		}
#endif
		dattr->flags.has_value = 1;
	} 
	else
	{
		QSE_MMGR_FREE (dic->mmgr, dval);
		return QSE_NULL;
	}

	/* return an existing item or insert a new item */
	np = qse_htl_ensert(&dic->consts_byname, dval);
	if (!np || np->data != dval)
	{
		/* insertion failure or existing item found */
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
			qse_htl_delete (&dic->consts_byname, dval);
			return QSE_NULL;
		}
	}

	return dval;
}

int qse_raddic_deleteconstbyname (qse_raddic_t* dic, int attr, const qse_char_t* name)
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
		QSE_ASSERT (dv->value == dv2->value);
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

int qse_raddic_deleteconstbyvalue (qse_raddic_t* dic, int attr, int value)
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

static int str2argv (qse_char_t *str, qse_char_t* argv[], int max_argc)
{
	int nflds, i;
	qse_char_t* ptr;

	nflds = qse_strspl (str, QSE_T(""), QSE_T('\0'), QSE_T('\0'), QSE_T('\0'));
	if (nflds <= 0)
	{
		return -1;
	}

	ptr = str;
	for (i = 0; i < nflds; i++)
	{
		argv[i] = ptr;
		while (*ptr != QSE_T('\0')) ptr++;
		ptr++;
	}

	return nflds;
}

static int sscanf_i(const qse_char_t *str, int *pvalue)
{
#if 0
	int rcode = 0;
	int base = 10;
	const char *tab = "0123456789";

	if ((str[0] == '0') && ((str[1] == 'x') || (str[1] == 'X'))) 
	{
		tab = "0123456789abcdef";
		base = 16;

		str += 2;
	}

	while (*str) 
	{
		const char *c;

		c = memchr(tab, tolower((int) *str), base);
		if (!c) return 0;

		rcode *= base;
		rcode += (c - tab);
		str++;
	}

	*pvalue = rcode;
	return 1;
#else
	qse_long_t v;
	const qse_char_t* end;
	QSE_STRTONUM (v, str, &end, 0);
	if (*end != '\0') return 0;
	*pvalue = v;
	return 1;
#endif
}




static int process_value(qse_raddic_t* dic, const qse_char_t* fn, qse_size_t line, qse_char_t* argv[], int argc)
{
	/* Process the VALUE command */

	int	value;

	if (argc != 3) 
	{
		/*fr_strerror_printf("dict_init: %s[%d]: invalid VALUE line", fn, line);*/
		return -1;
	}

	/* For Compatibility, skip "Server-Config" */
	if (qse_strcasecmp(argv[0], QSE_T("Server-Config")) == 0) return 0;

	/* Validate all entries */
	if (!sscanf_i(argv[2], &value)) 
	{
		//fr_strerror_printf("dict_init: %s[%d]: invalid value", fn, line);
		return -1;
	}

	if (qse_raddic_addconst(dic, argv[1], argv[0], value) < 0) 
	{
		//fr_strerror_printf("dict_init: %s[%d]: %s", fn, line, fr_strerror());
		return -1;
	}

	return 0;
}


/*
 *	Process the ATTRIBUTE command
 */
static int process_attribute (
	const char* fn, const qse_size_t line,
	const int block_vendor, qse_raddic_attr_t *block_tlv,
	char **argv, int argc)
{
	int		vendor = 0;
	int		value;
	int		type;
	ATTR_FLAGS	flags;

	if ((argc < 3) || (argc > 4)) {
		fr_strerror_printf("dict_init: %s[%d]: invalid ATTRIBUTE line",
			fn, line);
		return -1;
	}

	/*
	 *	Validate all entries
	 */
	if (!sscanf_i(argv[1], &value)) 
	{
		fr_strerror_printf("dict_init: %s[%d]: invalid value", fn, line);
		return -1;
	}

	/*
	 *	find the type of the attribute.
	 */
	type = fr_str2int(type_table, argv[2], -1);
	if (type < 0) {
		fr_strerror_printf("dict_init: %s[%d]: invalid type \"%s\"",
			fn, line, argv[2]);
		return -1;
	}

	/*
	 *	Only look up the vendor if the string
	 *	is non-empty.
	 */
	QSE_MEMSET (&flags, 0, QSE_SIZEOF(flags));
	if (argc == 4) 
	{
		char *key, *next, *last;

		key = argv[3];
		do {
			next = strchr(key, ',');
			if (next) *(next++) = '\0';

			if (qse_mbscmp(key, "has_tag") == 0 ||
			    qse_mbscmp(key, "has_tag=1") == 0) {
				/* Boolean flag, means this is a
				   tagged attribute */
				flags.has_tag = 1;
				
			} else if (strncmp(key, "encrypt=", 8) == 0) {
				/* Encryption method, defaults to 0 (none).
				   Currently valid is just type 2,
				   Tunnel-Password style, which can only
				   be applied to strings. */
				flags.encrypt = strtol(key + 8, &last, 0);
				if (*last) {
					fr_strerror_printf( "dict_init: %s[%d] invalid option %s",
						    fn, line, key);
					return -1;
				}
				
			} else if (strncmp(key, "array", 8) == 0) {
				flags.array = 1;
				
				switch (type) {
					case QSE_RADDIC_ATTR_TYPE_IPADDR:
					case QSE_RADDIC_ATTR_TYPE_BYTE:
					case QSE_RADDIC_ATTR_TYPE_SHORT:
					case QSE_RADDIC_ATTR_TYPE_INTEGER:
					case QSE_RADDIC_ATTR_TYPE_DATE:
						break;

					default:
						fr_strerror_printf( "dict_init: %s[%d] Only IP addresses can have the \"array\" flag set.",
							    fn, line);
						return -1;
				}
				
			} else {
				fr_strerror_printf( "dict_init: %s[%d]: unknown option \"%s\"",
					    fn, line, key);
				return -1;
			}

			key = next;
			if (key && !*key) break;
		} while (key);
	}

	if (block_vendor) vendor = block_vendor;

	/*
	 *	Special checks for tags, they make our life much more
	 *	difficult.
	 */
	if (flags.has_tag) {
		/*
		 *	Only string, octets, and integer can be tagged.
		 */
		switch (type) {
		case QSE_RADDIC_ATTR_TYPE_STRING:
		case QSE_RADDIC_ATTR_TYPE_INTEGER:
			break;

		default:
			fr_strerror_printf("dict_init: %s[%d]: Attributes of type %s cannot be tagged.",
				   fn, line,
				   fr_int2str(type_table, type, "?Unknown?"));
			return -1;

		}
	}

	if (type == QSE_RADDIC_ATTR_TYPE_TLV) {
		flags.has_tlv = 1;
	}

	if (block_tlv) {
		/*
		 *	TLV's can be only one octet.
		 */
		if ((value <= 0) || (value > 255)) {
			fr_strerror_printf( "dict_init: %s[%d]: sub-tlv's cannot have value > 255",
				    fn, line);
			return -1;
		}

		if (flags.encrypt != FLAG_ENCRYPT_NONE) {
			fr_strerror_printf( "dict_init: %s[%d]: sub-tlv's cannot be encrypted",
				    fn, line);
			return -1;
		}

		/*
		 *	
		 */
		value <<= 8;
		value |= (block_tlv->attr & 0xffff);
		flags.is_tlv = 1;
	}

	/*
	 *	Add it in.
	 */
	if (dict_addattr(argv[0], vendor, type, value, flags) < 0) {
		fr_strerror_printf("dict_init: %s[%d]: %s",
			   fn, line, fr_strerror());
		return -1;
	}

	return 0;
}


/*
 *	Process the VALUE command
 */
static int process_value(qse_raddic_t* dic, const qse_char_t* fn, const qse_size_t line, qse_char_t** argv, int argc)
{
	int value;

	if (argc != 3) 
	{
		//fr_strerror_printf("dict_init: %s[%d]: invalid VALUE line", fn, line);
		return -1;
	}

	/*
	 *	For Compatibility, skip "Server-Config"
	 */
	if (qse_strcasecmp(argv[0], QSE_T("Server-Config")) == 0) return 0;

	/*
	 *	Validate all entries
	 */
	if (!sscanf_i(argv[2], &value)) 
	{
		//fr_strerror_printf("dict_init: %s[%d]: invalid value", fn, line);
		return -1;
	}

	if (qse_raddic_addconst (dic, argv[1], argv[0], value) <= -1) 
	{
		//fr_strerror_printf("dict_init: %s[%d]: %s", fn, line, fr_strerror());
		return -1;
	}

	return 0;
}


#if 0
/*
 *	Process the VALUE-ALIAS command
 *
 *	This allows VALUE mappings to be shared among multiple
 *	attributes.
 */
static int process_value_alias(qse_raddic_t* dic, const qse_char_t* fn, const qse_size_t line, qse_char_t** argv, int argc)
{
	qse_raddic_attr_t* my_da, * da;
	qse_raddic_const_t* dval;

	if (argc != 2) 
	{
		//fr_strerror_printf("dict_init: %s[%d]: invalid VALUE-ALIAS line", fn, line);
		return -1;
	}

	my_da = qse_raddic_findattrbyname(argv[0]);
	if (!my_da) 
	{
		//fr_strerror_printf("dict_init: %s[%d]: ATTRIBUTE \"%s\" does not exist",
		//	   fn, line, argv[1]);
		return -1;
	}

	if (my_da->flags.has_value) 
	{
		//fr_strerror_printf("dict_init: %s[%d]: Cannot add VALUE-ALIAS to ATTRIBUTE \"%s\" with pre-existing VALUE",
		//	   fn, line, argv[0]);
		return -1;
	}

	if (my_da->flags.has_value_alias) 
	{
		//fr_strerror_printf("dict_init: %s[%d]: Cannot add VALUE-ALIAS to ATTRIBUTE \"%s\" with pre-existing VALUE-ALIAS",
		//	   fn, line, argv[0]);
		return -1;
	}

	da = qse_raddic_findattrbyname(argv[1]);
	if (!da) 
	{
		//fr_strerror_printf("dict_init: %s[%d]: Cannot find ATTRIBUTE \"%s\" for alias", fn, line, argv[1]);
		return -1;
	}

	if (!da->flags.has_value) 
	{
		//fr_strerror_printf("dict_init: %s[%d]: VALUE-ALIAS cannot refer to ATTRIBUTE %s: It has no values", fn, line, argv[1]);
		return -1;
	}

	if (da->flags.has_value_alias) 
	{
		//fr_strerror_printf("dict_init: %s[%d]: Cannot add VALUE-ALIAS to ATTRIBUTE \"%s\" which itself has a VALUE-ALIAS", fn, line, argv[1]);
		return -1;
	}

	if (my_da->type != da->type) 
	{
		fr_strerror_printf("dict_init: %s[%d]: Cannot add VALUE-ALIAS between attributes of differing type",
			   fn, line);
		return -1;
	}

	if ((dval = fr_pool_alloc(QSE_SIZEOF(*dval))) == QSE_NULL) {
		fr_strerror_printf("dict_addvalue: out of memory");
		return -1;
	}

	dval->name[0] = '\0';	/* empty name */
	dval->attr = my_da->attr;
	dval->value = da->attr;

	if (!fr_hash_table_insert(values_byname, dval)) {
		fr_strerror_printf("dict_init: %s[%d]: Error create alias",
			   fn, line);
		fr_pool_free(dval);
		return -1;
	}

	return 0;
}

#endif

/*
 *	Process the VENDOR command
 */
static int process_vendor (qse_raddic_t* dic, const qse_char_t* fn, const qse_size_t line, qse_char_t** argv,  int argc)
{
	int	value;
	int	continuation = 0;
	const qse_char_t* format = QSE_NULL;

	if ((argc < 2) || (argc > 3)) 
	{
		//fr_strerror_printf( "dict_init: %s[%d] invalid VENDOR entry", fn, line);
		return -1;
	}

	/*
	 *	 Validate all entries
	 */
	if (!isdigit((int) argv[1][0])) {
		fr_strerror_printf("dict_init: %s[%d]: invalid value",
			fn, line);
		return -1;
	}
	value = atoi(argv[1]);

	/* Create a new VENDOR entry for the list */
	if (dict_addvendor(argv[0], value) < 0) {
		fr_strerror_printf("dict_init: %s[%d]: %s",
			   fn, line, fr_strerror());
		return -1;
	}

	/*
	 *	Look for a format statement
	 */
	if (argc == 3) {
		format = argv[2];

	} else if (value == VENDORPEC_USR) { /* catch dictionary screw-ups */
		format = "format=4,0";

	} else if (value == VENDORPEC_LUCENT) {
		format = "format=2,1";

	} else if (value == VENDORPEC_STARENT) {
		format = "format=2,2";

	} /* else no fixups to do */

	if (format) {
		int type, length;
		const char *p;
		qse_raddic_vendor_t *dv;

		if (strncasecmp(format, "format=", 7) != 0) {
			fr_strerror_printf("dict_init: %s[%d]: Invalid format for VENDOR.  Expected \"format=\", got \"%s\"",
				   fn, line, format);
			return -1;
		}

		p = format + 7;
		if ((qse_mbslen(p) < 3) ||
		    !isdigit((int) p[0]) ||
		    (p[1] != ',') ||
		    !isdigit((int) p[2]) ||
		    (p[3] && (p[3] != ','))) {
			fr_strerror_printf("dict_init: %s[%d]: Invalid format for VENDOR.  Expected text like \"1,1\", got \"%s\"",
				   fn, line, p);
			return -1;
		}

		type = (int) (p[0] - '0');
		length = (int) (p[2] - '0');

		if (p[3] == ',') {
			if ((p[4] != 'c') ||
			    (p[5] != '\0')) {
				fr_strerror_printf("dict_init: %s[%d]: Invalid format for VENDOR.  Expected text like \"1,1\", got \"%s\"",
					   fn, line, p);
				return -1;
			}
			continuation = 1;
		}

		dv = dict_vendorbyvalue(value);
		if (!dv) {
			fr_strerror_printf("dict_init: %s[%d]: Failed adding format for VENDOR",
				   fn, line);
			return -1;
		}

		if ((type != 1) && (type != 2) && (type != 4)) {
			fr_strerror_printf("dict_init: %s[%d]: invalid type value %d for VENDOR",
				   fn, line, type);
			return -1;
		}

		if ((length != 0) && (length != 1) && (length != 2)) {
			fr_strerror_printf("dict_init: %s[%d]: invalid length value %d for VENDOR",
				   fn, line, length);
			return -1;
		}

		dv->type = type;
		dv->length = length;
		dv->flags = continuation;
	}

	return 0;
}

static int load_file (qse_raddic_t* dic, const qse_char_t *dir, const qse_char_t *fn, const qse_char_t *src_file, int src_line)
{
	qse_sio_t* sio = QSE_NULL;
	qse_char_t dirtmp[256]; /* TODO: longer path */
	qse_char_t buf[256];
	qse_char_t* p;
	qse_size_t line = 0;
	int vendor;
	int block_vendor;

	qse_char_t* argv[16]; /* TODO: what is the best size? */
	int argc;
	qse_raddic_attr_t* da, * block_tlv = QSE_NULL;

#if 0
	if (qse_strlen(fn) >= QSE_SIZEOF(dirtmp) / 2 ||
	    qse_strlen(dir) >= QSE_SIZEOF(dirtmp) / 2) 
	{
		fr_strerror_printf("dict_init: filename name too long");
		return -1;
	}

	/*
	 *	First see if fn is relative to dir. If so, create
	 *	new filename. If not, remember the absolute dir.
	 */
	if ((p = qse_strrchr(fn, FR_DIR_SEP)) != QSE_NULL) 
	{
		qse_strcpy(dirtmp, fn);
		dirtmp[p - fn] = 0;
		dir = dirtmp;
	}
	else if (dir && dir[0] && qse_strcmp(dir, ".") != 0) 
	{
		snprintf(dirtmp, QSE_SIZEOF(dirtmp), "%s/%s", dir, fn);
		fn = dirtmp;
	}
#endif

	sio = qse_sio_open (dic->mmgr, 0, fn, QSE_SIO_READ);
	if (!sio)
	{
#if 0
		if (!src_file) {
			fr_strerror_printf("dict_init: Couldn't open dictionary \"%s\": %s",
				   fn, strerror(errno));
		} else {
			fr_strerror_printf("dict_init: %s[%d]: Couldn't open dictionary \"%s\": %s",
				   src_file, src_line, fn, strerror(errno));
		}
		}
#endif
		return -1;
	}

#if 0
	stat(fn, &statbuf); /* fopen() guarantees this will succeed */
	if (!S_ISREG(statbuf.st_mode)) {
		fclose(fp);
		fr_strerror_printf("dict_init: Dictionary \"%s\" is not a regular file",
			   fn);
		return -1;
	}

	dict_stat_add(fn, &statbuf);

	/*
	 *	Seed the random pool with data.
	 */
	fr_rand_seed(&statbuf, QSE_SIZEOF(statbuf));
#endif

	block_vendor = 0;

	while (qse_sio_getstr (sio, buf, QSE_COUNTOF(buf)) >= 0) 
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
			//fr_strerror_printf( "dict_init: %s[%d] invalid entry", fn, line);
			goto oops;
		}

		/*
		 *	Process VALUE lines.
		 */
		if (qse_strcasecmp(argv[0], QSE_T("VALUE")) == 0) 
		{
			if (process_value(dic, fn, line, argv + 1, argc - 1) == -1) goto oops;
			continue;
		}

		/*
		 *	Perhaps this is an attribute.
		 */
		if (qse_strcasecmp(argv[0], QSE_T("ATTRIBUTE")) == 0) 
		{
			if (process_attribute(dic, fn, line, block_vendor, block_tlv, argv + 1, argc - 1) == -1) goto oops;
			continue;
		}

		/*
		 *	See if we need to import another dictionary.
		 */
		if (qse_strcasecmp(argv[0], QSE_T("$INCLUDE")) == 0) 
		{
			if (load_file(dic, dir, argv[1], fn, line) < 0) goto oops;
			continue;
		} /* $INCLUDE */

#if 0
		if (qse_strcasecmp(argv[0], QSE_T("VALUE-ALIAS")) == 0) 
		{
			if (process_value_alias(dic, fn, line, argv + 1, argc - 1) == -1) goto oops;
			continue;
		}
#endif

		/*
		 *	Process VENDOR lines.
		 */
		if (qse_strcasecmp(argv[0], QSE_T("VENDOR")) == 0) 
		{
			if (process_vendor(dic, fn, line, argv + 1, argc - 1) == -1)  goto oops;
			continue;
		}

		if (qse_strcasecmp(argv[0], QSE_T("BEGIN-TLV")) == 0) 
		{
			if (argc != 2) 
			{
				//fr_strerror_printf("dict_init: %s[%d] invalid BEGIN-TLV entry", fn, line);
				goto oops;
			}

			da = qse_findattrbyname (dic, argv[1]);
			if (!da) 
			{
				//fr_strerror_printf("dict_init: %s[%d]: unknown attribute %s", fn, line, argv[1]);
				goto oops;
			}

			if (da->type != QSE_RADDIC_ATTR_TYPE_TLV) 
			{
				//fr_strerror_printf("dict_init: %s[%d]: attribute %s is not of type tlv", fn, line, argv[1]);
				goto oops;
			}

			block_tlv = da;
			continue;
		} /* BEGIN-TLV */

		if (qse_strcasecmp(argv[0], "END-TLV") == 0) 
		{
			if (argc != 2) 
			{
				//fr_strerror_printf("dict_init: %s[%d] invalid END-TLV entry", fn, line);
				goto oops;
			}

			da = qse_raddic_findattrbyname(dic, argv[1]);
			if (!da) 
			{
				//fr_strerror_printf("dict_init: %s[%d]: unknown attribute %s", fn, line, argv[1]);
				goto oops;
			}

			if (da != block_tlv) 
			{
				//fr_strerror_printf("dict_init: %s[%d]: END-TLV %s does not match any previous BEGIN-TLV", fn, line, argv[1]);
				goto oops;
			}
			block_tlv = QSE_NULL;
			continue;
		} /* END-VENDOR */

		if (qse_strcasecmp(argv[0], "BEGIN-VENDOR") == 0) 
		{
			if (argc != 2) 
			{
				//fr_strerror_printf("dict_init: %s[%d] invalid BEGIN-VENDOR entry", fn, line);
				goto oops;
			}

			vendor = qse_raddic_findvendorbyname (dic, argv[1]);
			if (!vendor) 
			{
				//fr_strerror_printf("dict_init: %s[%d]: unknown vendor %s", fn, line, argv[1]);
				goto oops;
			}
			block_vendor = vendor;
			continue;
		} /* BEGIN-VENDOR */

		if (qse_strcasecmp(argv[0], QSE_T("END-VENDOR")) == 0) 
		{
			if (argc != 2) {
				//fr_strerror_printf("dict_init: %s[%d] invalid END-VENDOR entry", fn, line);
				goto oops;
			}

			vendor = qse_raddic_findvendorbyname(argv[1]);
			if (!vendor) 
			{
				//fr_strerror_printf("dict_init: %s[%d]: unknown vendor %s", fn, line, argv[1]);
				goto oops;
			}

			if (vendor != block_vendor)
			{
				//fr_strerror_printf(
				//	"dict_init: %s[%d]: END-VENDOR %s does not match any previous BEGIN-VENDOR",
				//	fn, line, argv[1]);
				goto oops;
			}
			block_vendor = 0;
			continue;
		} /* END-VENDOR */

		/*
		 *	Any other string: We don't recognize it.
		 */
		//fr_strerror_printf("dict_init: %s[%d] invalid keyword \"%s\"", fn, line, argv[0]);
		goto oops;
		
	}

	qse_sio_close (sio);
	return 0;


oops:
	if (sio) qse_sio_close (sio);
	return -1;
}

static int qse_raddic_load (qse_raddic_t* dic, const qse_char_t* file)
{
	return load_file (dic, QSE_NULL, file, QSE_NULL, 0);
}


#if 0
/*
 *	For faster HUP's, we cache the stat information for
 *	files we've $INCLUDEd
 */
typedef struct dict_stat_t {
	struct dict_stat_t* next;
	char*               name;
	time_t		     mtime;
} dict_stat_t;

static char *stat_root_dir = QSE_NULL;
static char *stat_root_file = QSE_NULL;

static dict_stat_t *stat_head = QSE_NULL;
static dict_stat_t *stat_tail = QSE_NULL;

typedef struct value_fixup_t {
	char		attrstr[qse_raddic_attr_t_MAX_NAME_LEN];
	qse_raddic_value_t	*dval;
	struct value_fixup_t *next;
} value_fixup_t;


/*
 *	So VALUEs in the dictionary can have forward references.
 */
static value_fixup_t *value_fixup = QSE_NULL;

static const FR_NAME_NUMBER type_table[] = {
	{ "integer",	QSE_RADDIC_ATTR_TYPE_INTEGER },
	{ "string",	QSE_RADDIC_ATTR_TYPE_STRING },
	{ "ipaddr",	QSE_RADDIC_ATTR_TYPE_IPADDR },
	{ "date",	QSE_RADDIC_ATTR_TYPE_DATE },
	{ "abinary",	QSE_RADDIC_ATTR_TYPE_ABINARY },
	{ "octets",	QSE_RADDIC_ATTR_TYPE_OCTETS },
	{ "ifid",	QSE_RADDIC_ATTR_TYPE_IFID },
	{ "ipv6addr",	QSE_RADDIC_ATTR_TYPE_IPV6ADDR },
	{ "ipv6prefix", QSE_RADDIC_ATTR_TYPE_IPV6PREFIX },
	{ "byte",	QSE_RADDIC_ATTR_TYPE_BYTE },
	{ "short",	QSE_RADDIC_ATTR_TYPE_SHORT },
	{ "ether",	QSE_RADDIC_ATTR_TYPE_ETHERNET },
	{ "combo-ip",	QSE_RADDIC_ATTR_TYPE_COMBO_IP },
	{ "tlv",	QSE_RADDIC_ATTR_TYPE_TLV },
	{ "signed",	QSE_RADDIC_ATTR_TYPE_SIGNED },
	{ QSE_NULL, 0 }
};



static int sscanf_i(const char *str, int *pvalue)
{
	int rcode = 0;
	int base = 10;
	const char *tab = "0123456789";

	if ((str[0] == '0') &&
	    ((str[1] == 'x') || (str[1] == 'X'))) {
		tab = "0123456789abcdef";
		base = 16;

		str += 2;
	}

	while (*str) {
		const char *c;

		c = memchr(tab, tolower((int) *str), base);
		if (!c) return 0;

		rcode *= base;
		rcode += (c - tab);
		str++;
	}

	*pvalue = rcode;
	return 1;
}


/*
 *	String split routine.  Splits an input string IN PLACE
 *	into pieces, based on spaces.
 */
static int str2argv(char *str, char **argv, int max_argc)
{
	int argc = 0;

	while (*str) {
		if (argc >= max_argc) return argc;

		/*
		 *	Chop out comments early.
		 */
		if (*str == '#') {
			*str = '\0';
			break;
		}

		while ((*str == ' ') ||
		       (*str == '\t') ||
		       (*str == '\r') ||
		       (*str == '\n')) *(str++) = '\0';

		if (!*str) return argc;

		argv[argc] = str;
		argc++;

		while (*str &&
		       (*str != ' ') &&
		       (*str != '\t') &&
		       (*str != '\r') &&
		       (*str != '\n')) str++;
	}

	return argc;
}

#define MAX_ARGV (16)

/*
 *	Initialize the dictionary.
 */
static int my_dict_init(const char *dir, const char *fn,
			const char *src_file, int src_line)
{
	FILE	*fp;
	char 	dirtmp[256];
	char	buf[256];
	char	*p;
	int	line = 0;
	int	vendor;
	int	block_vendor;
	struct stat statbuf;
	char	*argv[MAX_ARGV];
	int	argc;
	qse_raddic_attr_t *da, *block_tlv = QSE_NULL;

	if (qse_mbslen(fn) >= QSE_SIZEOF(dirtmp) / 2 ||
	    qse_mbslen(dir) >= QSE_SIZEOF(dirtmp) / 2) {
		fr_strerror_printf("dict_init: filename name too long");
		return -1;
	}

	/*
	 *	First see if fn is relative to dir. If so, create
	 *	new filename. If not, remember the absolute dir.
	 */
	if ((p = strrchr(fn, FR_DIR_SEP)) != QSE_NULL) {
		qse_mbscpy(dirtmp, fn);
		dirtmp[p - fn] = 0;
		dir = dirtmp;
	} else if (dir && dir[0] && qse_mbscmp(dir, ".") != 0) {
		snprintf(dirtmp, QSE_SIZEOF(dirtmp), "%s/%s", dir, fn);
		fn = dirtmp;
	}

	if ((fp = fopen(fn, "r")) == QSE_NULL) {
		if (!src_file) {
			fr_strerror_printf("dict_init: Couldn't open dictionary \"%s\": %s",
				   fn, strerror(errno));
		} else {
			fr_strerror_printf("dict_init: %s[%d]: Couldn't open dictionary \"%s\": %s",
				   src_file, src_line, fn, strerror(errno));
		}
		return -1;
	}

	stat(fn, &statbuf); /* fopen() guarantees this will succeed */
	if (!S_ISREG(statbuf.st_mode)) {
		fclose(fp);
		fr_strerror_printf("dict_init: Dictionary \"%s\" is not a regular file",
			   fn);
		return -1;
	}

	/*
	 *	Globally writable dictionaries means that users can control
	 *	the server configuration with little difficulty.
	 */
#ifdef S_IWOTH
	if ((statbuf.st_mode & S_IWOTH) != 0) {
		fclose(fp);
		fr_strerror_printf("dict_init: Dictionary \"%s\" is globally writable.  Refusing to start due to insecure configuration.",
			   fn);
		return -1;
	}
#endif

	dict_stat_add(fn, &statbuf);

	/*
	 *	Seed the random pool with data.
	 */
	fr_rand_seed(&statbuf, QSE_SIZEOF(statbuf));

	block_vendor = 0;

	while (fgets(buf, QSE_SIZEOF(buf), fp) != QSE_NULL) {
		line++;
		if (buf[0] == '#' || buf[0] == 0 ||
		    buf[0] == '\n' || buf[0] == '\r')
			continue;

		/*
		 *  Comment characters should NOT be appearing anywhere but
		 *  as start of a comment;
		 */
		p = strchr(buf, '#');
		if (p) *p = '\0';

		argc = str2argv(buf, argv, MAX_ARGV);
		if (argc == 0) continue;

		if (argc == 1) {
			fr_strerror_printf( "dict_init: %s[%d] invalid entry",
				    fn, line);
			fclose(fp);
			return -1;
		}

		/*
		 *	Process VALUE lines.
		 */
		if (qse_mbscasecmp(argv[0], "VALUE") == 0) {
			if (process_value(fn, line,
					  argv + 1, argc - 1) == -1) {
				fclose(fp);
				return -1;
			}
			continue;
		}

		/*
		 *	Perhaps this is an attribute.
		 */
		if (qse_mbscasecmp(argv[0], "ATTRIBUTE") == 0) {
			if (process_attribute(fn, line, block_vendor,
					      block_tlv,
					      argv + 1, argc - 1) == -1) {
				fclose(fp);
				return -1;
			}
			continue;
		}

		/*
		 *	See if we need to import another dictionary.
		 */
		if (qse_mbscasecmp(argv[0], "$INCLUDE") == 0) {
			if (my_dict_init(dir, argv[1], fn, line) < 0) {
				fclose(fp);
				return -1;
			}
			continue;
		} /* $INCLUDE */

		if (qse_mbscasecmp(argv[0], "VALUE-ALIAS") == 0) {
			if (process_value_alias(fn, line,
						argv + 1, argc - 1) == -1) {
				fclose(fp);
				return -1;
			}
			continue;
		}

		/*
		 *	Process VENDOR lines.
		 */
		if (qse_mbscasecmp(argv[0], "VENDOR") == 0) {
			if (process_vendor(fn, line,
					   argv + 1, argc - 1) == -1) {
				fclose(fp);
				return -1;
			}
			continue;
		}

		if (qse_mbscasecmp(argv[0], "BEGIN-TLV") == 0) {
			if (argc != 2) {
				fr_strerror_printf(
				"dict_init: %s[%d] invalid BEGIN-TLV entry",
					fn, line);
				fclose(fp);
				return -1;
			}

			da = dict_attrbyname(argv[1]);
			if (!da) {
				fr_strerror_printf(
					"dict_init: %s[%d]: unknown attribute %s",
					fn, line, argv[1]);
				fclose(fp);
				return -1;
			}

			if (da->type != QSE_RADDIC_ATTR_TYPE_TLV) {
				fr_strerror_printf(
					"dict_init: %s[%d]: attribute %s is not of type tlv",
					fn, line, argv[1]);
				fclose(fp);
				return -1;
			}

			block_tlv = da;
			continue;
		} /* BEGIN-TLV */

		if (qse_mbscasecmp(argv[0], "END-TLV") == 0) {
			if (argc != 2) {
				fr_strerror_printf(
				"dict_init: %s[%d] invalid END-TLV entry",
					fn, line);
				fclose(fp);
				return -1;
			}

			da = dict_attrbyname(argv[1]);
			if (!da) {
				fr_strerror_printf(
					"dict_init: %s[%d]: unknown attribute %s",
					fn, line, argv[1]);
				fclose(fp);
				return -1;
			}

			if (da != block_tlv) {
				fr_strerror_printf(
					"dict_init: %s[%d]: END-TLV %s does not match any previous BEGIN-TLV",
					fn, line, argv[1]);
				fclose(fp);
				return -1;
			}
			block_tlv = QSE_NULL;
			continue;
		} /* END-VENDOR */

		if (qse_mbscasecmp(argv[0], "BEGIN-VENDOR") == 0) {
			if (argc != 2) {
				fr_strerror_printf(
				"dict_init: %s[%d] invalid BEGIN-VENDOR entry",
					fn, line);
				fclose(fp);
				return -1;
			}

			vendor = dict_vendorbyname(argv[1]);
			if (!vendor) {
				fr_strerror_printf(
					"dict_init: %s[%d]: unknown vendor %s",
					fn, line, argv[1]);
				fclose(fp);
				return -1;
			}
			block_vendor = vendor;
			continue;
		} /* BEGIN-VENDOR */

		if (qse_mbscasecmp(argv[0], "END-VENDOR") == 0) {
			if (argc != 2) {
				fr_strerror_printf(
				"dict_init: %s[%d] invalid END-VENDOR entry",
					fn, line);
				fclose(fp);
				return -1;
			}

			vendor = dict_vendorbyname(argv[1]);
			if (!vendor) {
				fr_strerror_printf(
					"dict_init: %s[%d]: unknown vendor %s",
					fn, line, argv[1]);
				fclose(fp);
				return -1;
			}

			if (vendor != block_vendor) {
				fr_strerror_printf(
					"dict_init: %s[%d]: END-VENDOR %s does not match any previous BEGIN-VENDOR",
					fn, line, argv[1]);
				fclose(fp);
				return -1;
			}
			block_vendor = 0;
			continue;
		} /* END-VENDOR */

		/*
		 *	Any other string: We don't recognize it.
		 */
		fr_strerror_printf("dict_init: %s[%d] invalid keyword \"%s\"",
			   fn, line, argv[0]);
		fclose(fp);
		return -1;
	}
	fclose(fp);
	return 0;
}


/*
 *	Empty callback for hash table initialization.
 */
static int null_callback(void *ctx, void *data)
{
	ctx = ctx;		/* -Wunused */
	data = data;		/* -Wunused */

	return 0;
}


/*
 *	Initialize the directory, then fix the attr member of
 *	all attributes.
 */
int dict_init(const char *dir, const char *fn)
{
	/*
	 *	Check if we need to change anything.  If not, don't do
	 *	anything.
	 */
	if (dict_stat_check(dir, fn)) {
		return 0;
	}

	/*
	 *	Free the dictionaries, and the stat cache.
	 */
	dict_free();
	stat_root_dir = strdup(dir);
	stat_root_file = strdup(fn);

	/*
	 *	Create the table of vendor by name.   There MAY NOT
	 *	be multiple vendors of the same name.
	 *
	 *	Each vendor is malloc'd, so the free function is free.
	 */
	vendors_byname = fr_hash_table_create(dict_vendor_name_hash,
						dict_vendor_name_cmp,
						fr_pool_free);
	if (!vendors_byname) {
		return -1;
	}

	/*
	 *	Create the table of vendors by value.  There MAY
	 *	be vendors of the same value.  If there are, we
	 *	pick the latest one.
	 */
	vendors_byvalue = fr_hash_table_create(dict_vendor_value_hash,
						 dict_vendor_value_cmp,
						 fr_pool_free);
	if (!vendors_byvalue) {
		return -1;
	}

	/*
	 *	Create the table of attributes by name.   There MAY NOT
	 *	be multiple attributes of the same name.
	 *
	 *	Each attribute is malloc'd, so the free function is free.
	 */
	attributes_byname = fr_hash_table_create(dict_attr_name_hash,
						   dict_attr_name_cmp,
						   fr_pool_free);
	if (!attributes_byname) {
		return -1;
	}

	/*
	 *	Create the table of attributes by value.  There MAY
	 *	be attributes of the same value.  If there are, we
	 *	pick the latest one.
	 */
	attributes_byvalue = fr_hash_table_create(dict_attr_value_hash,
						    dict_attr_value_cmp,
						    fr_pool_free);
	if (!attributes_byvalue) {
		return -1;
	}

	values_byname = fr_hash_table_create(dict_value_name_hash,
					       dict_value_name_cmp,
					       fr_pool_free);
	if (!values_byname) {
		return -1;
	}

	values_byvalue = fr_hash_table_create(dict_value_value_hash,
						dict_value_value_cmp,
						fr_pool_free);
	if (!values_byvalue) {
		return -1;
	}

	value_fixup = QSE_NULL;	/* just to be safe. */

	if (my_dict_init(dir, fn, QSE_NULL, 0) < 0)
		return -1;

	if (value_fixup) {
		qse_raddic_attr_t *a;
		value_fixup_t *this, *next;

		for (this = value_fixup; this != QSE_NULL; this = next) {
			next = this->next;

			a = dict_attrbyname(this->attrstr);
			if (!a) {
				fr_strerror_printf(
					"dict_init: No ATTRIBUTE \"%s\" defined for VALUE \"%s\"",
					this->attrstr, this->dval->name);
				return -1; /* leak, but they should die... */
			}

			this->dval->attr = a->attr;

			/*
			 *	Add the value into the dictionary.
			 */
			if (!fr_hash_table_replace(values_byname,
						     this->dval)) {
				fr_strerror_printf("dict_addvalue: Duplicate value name %s for attribute %s", this->dval->name, a->name);
				return -1;
			}

			/*
			 *	Allow them to use the old name, but
			 *	prefer the new name when printing
			 *	values.
			 */
			if (!fr_hash_table_finddata(values_byvalue, this->dval)) {
				fr_hash_table_replace(values_byvalue,
							this->dval);
			}
			free(this);

			/*
			 *	Just so we don't lose track of things.
			 */
			value_fixup = next;
		}
	}

	/*
	 *	Walk over all of the hash tables to ensure they're
	 *	initialized.  We do this because the threads may perform
	 *	lookups, and we don't want multi-threaded re-ordering
	 *	of the table entries.  That would be bad.
	 */
	fr_hash_table_walk(vendors_byname, null_callback, QSE_NULL);
	fr_hash_table_walk(vendors_byvalue, null_callback, QSE_NULL);

	fr_hash_table_walk(attributes_byname, null_callback, QSE_NULL);
	fr_hash_table_walk(attributes_byvalue, null_callback, QSE_NULL);

	fr_hash_table_walk(values_byvalue, null_callback, QSE_NULL);
	fr_hash_table_walk(values_byname, null_callback, QSE_NULL);

	return 0;
}


#endif


