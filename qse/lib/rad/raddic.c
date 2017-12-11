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
	qse_htl_t values_byvalue;
	qse_htl_t values_byname;

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
	if (a->vendor > b->vendor) return -1;
	return a->attr - b->attr;
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
	qse_htl_init (&dic->values_byname, mmgr, 1);
	qse_htl_init (&dic->values_byvalue, mmgr, 1);

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

	return 0;
}

void qse_raddic_fini (qse_raddic_t* dic)
{
	qse_htl_fini (&dic->vendors_byname);
	qse_htl_fini (&dic->vendors_byvalue);
	qse_htl_fini (&dic->attrs_byname);
	qse_htl_fini (&dic->attrs_byvalue);
	qse_htl_fini (&dic->values_byvalue);
	qse_htl_fini (&dic->values_byname);
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

	if (vendor == 0) dic->base_attrs[value] = dv; /* cache base attributes */
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

		/* when the attr of the given name is not the first one
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

int qse_raddic_deleteattrbyvalue (qse_raddic_t* dic, int attr)
{
	qse_raddic_attr_t* dv;

	dv = qse_raddic_findattrbyvalue(dic, attr);
	if (!dv) return -1;

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

#if 0 // XXX
/*
 *	Get an attribute by its numerical value.
 */
qse_raddic_attr_t *dict_attrbyvalue(qse_raddic_t* dic, int attr)
{
	qse_raddic_attr_t dattr;

#if 0
	if ((attr > 0) && (attr < 256)) return dict_base_attrs[attr];
#endif

	dattr.attr = attr;
	dattr.vendor = VENDOR(attr) /*& 0x7fff*/;

	return fr_hash_table_finddata(dic->attributes_byvalue, &dattr);
}

/*
 *	Get an attribute by its name.
 */
qse_raddic_attr_t *dict_attrbyname(const char *name)
{
	qse_raddic_attr_t *da;
	uint32_t buffer[(QSE_SIZEOF(*da) + qse_raddic_attr_t_MAX_NAME_LEN + 3)/4];

	if (!name) return QSE_NULL;

	da = (qse_raddic_attr_t *) buffer;
	strlcpy(da->name, name, qse_raddic_attr_t_MAX_NAME_LEN + 1);

	return fr_hash_table_finddata(attributes_byname, da);
}

/*
 *	Associate a value with an attribute and return it.
 */
qse_raddic_value_t *dict_valbyattr(qse_raddic_t* dic, int attr, int value)
{
	qse_raddic_value_t dval, *dv;

	/*
	 *	First, look up aliases.
	 */
	dval.attr = attr;
	dval.name[0] = '\0';

	/*
	 *	Look up the attribute alias target, and use
	 *	the correct attribute number if found.
	 */
	dv = fr_hash_table_finddata(values_byname, &dval);
	if (dv) dval.attr = dv->value;

	dval.value = value;

	return fr_hash_table_finddata(values_byvalue, &dval);
}

/*
 *	Get a value by its name, keyed off of an attribute.
 */
qse_raddic_value_t *dict_valbyname(qse_raddic_t* dic, int attr, const char *name)
{
	qse_raddic_value_t *my_dv, *dv;
	uint32_t buffer[(QSE_SIZEOF(*my_dv) + qse_raddic_value_t_MAX_NAME_LEN + 3)/4];

	if (!name) return QSE_NULL;

	my_dv = (qse_raddic_value_t *) buffer;
	my_dv->attr = attr;
	my_dv->name[0] = '\0';

	/*
	 *	Look up the attribute alias target, and use
	 *	the correct attribute number if found.
	 */
	dv = fr_hash_table_finddata(values_byname, my_dv);
	if (dv) my_dv->attr = dv->value;

	strlcpy(my_dv->name, name, qse_raddic_value_t_MAX_NAME_LEN + 1);

	return fr_hash_table_finddata(values_byname, my_dv);
}

/*
 *	Get the vendor PEC based on the vendor name
 *
 *	This is efficient only for small numbers of vendors.
 */

/*
 *	Add a value for an attribute to the dictionary.
 */
int dict_addvalue(qse_raddic_t* dic, const qse_char_t* namestr, const qse_char_t* attrstr, int value)
{
	qse_size_t     length;
	qse_raddic_attr_t*     dattr;
	qse_raddic_value_t*    dval;

	if (!*namestr) 
	{
		//fr_strerror_printf("dict_addvalue: empty names are not permitted");
		return -1;
	}

	length = qse_strlen(namestr);

	/* no +1 to length when allocating the space because dval has space for one character */
	if ((dval = QSE_MMGR_ALLOC (dic->mmgr,  QSE_SIZEOF(*dval) + (length * QSE_SIZEOF(*namestr)))) == QSE_NULL) 
	{
		//fr_strerror_printf("dict_addvalue: out of memory");
		return -1;
	}
	QSE_MEMSET(dval, 0, QSE_SIZEOF(*dval));

	qse_strcpy(dval->name, namestr);
	dval->value = value;

	/*
	 *	Most VALUEs are bunched together by ATTRIBUTE.  We can
	 *	save a lot of lookups on dictionary initialization by
	 *	caching the last attribute.
	 */
	if (dic->last_attr && (qse_strcasecmp(attrstr, dic->last_attr->name) == 0))
	{
		dattr = dic->last_attr;
	}
	else
	{
		dattr = dict_attrbyname(attrstr);
		dic->last_attr = dattr;
	}

	/*
	 *	Remember which attribute is associated with this
	 *	value, if possible.
	 */
	if (dattr) 
	{
		if (dattr->flags.has_value_alias) 
		{
			//fr_strerror_printf("dict_addvalue: Cannot add VALUE for ATTRIBUTE \"%s\": It already has a VALUE-ALIAS", attrstr);
			return -1;
		}

		dval->attr = dattr->attr;

		/*
		 *	Enforce valid values
		 *
		 *	Don't worry about fixups...
		 */
		switch (dattr->type) 
		{
			case QSE_RADDIC_ATTR_TYPE_BYTE:
				if (value > 255) 
				{
					fr_pool_free(dval);
					fr_strerror_printf("dict_addvalue: ATTRIBUTEs of type 'byte' cannot have VALUEs larger than 255");
					return -1;
				}
				break;
			case QSE_RADDIC_ATTR_TYPE_SHORT:
				if (value > 65535) 
				{
					fr_pool_free(dval);
					fr_strerror_printf("dict_addvalue: ATTRIBUTEs of type 'short' cannot have VALUEs larger than 65535");
					return -1;
				}
				break;

				/*
				 *	Allow octets for now, because
				 *	of dictionary.cablelabs
				 */
			case QSE_RADDIC_ATTR_TYPE_OCTETS:
			case QSE_RADDIC_ATTR_TYPE_INTEGER:
				break;

			default:
				fr_pool_free(dval);
				fr_strerror_printf("dict_addvalue: VALUEs cannot be defined for attributes of type '%s'",
					   fr_int2str(type_table, dattr->type, "?Unknown?"));
				return -1;
		}

		dattr->flags.has_value = 1;
	} 
	else 
	{
		value_fixup_t *fixup;

		fixup = (value_fixup_t *) malloc(QSE_SIZEOF(*fixup));
		if (!fixup) {
			fr_pool_free(dval);
			fr_strerror_printf("dict_addvalue: out of memory");
			return -1;
		}
		QSE_MEMSET(fixup, 0, QSE_SIZEOF(*fixup));

		strlcpy(fixup->attrstr, attrstr, QSE_SIZEOF(fixup->attrstr));
		fixup->dval = dval;

		/*
		 *	Insert to the head of the list.
		 */
		fixup->next = value_fixup;
		value_fixup = fixup;

		return 0;
	}

	/*
	 *	Add the value into the dictionary.
	 */
	if (!fr_hash_table_insert(values_byname, dval)) 
	{
		if (dattr) 
		{
			qse_raddic_value_t *old;

			/*
			 *	Suppress duplicates with the same
			 *	name and value.  There are lots in
			 *	dictionary.ascend.
			 */
			old = dict_valbyname(dattr->attr, namestr);
			if (old && (old->value == dval->value)) 
			{
				fr_pool_free(dval);
				return 0;
			}
		}

		fr_pool_free(dval);
		fr_strerror_printf("dict_addvalue: Duplicate value name %s for attribute %s", namestr, attrstr);
		return -1;
	}

	/*
	 *	There are multiple VALUE's, keyed by attribute, so we
	 *	take care of that here.
	 */
	if (!fr_hash_table_replace(values_byvalue, dval)) 
	{
		fr_strerror_printf("dict_addvalue: Failed inserting value %s", namestr);
		return -1;
	}

	return 0;
}



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




static int process_value(const qse_char_t* fn, qse_size_t line, qse_char_t* argv[], int argc)
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

	if (dict_addvalue(argv[1], argv[0], value) < 0) 
	{
		//fr_strerror_printf("dict_init: %s[%d]: %s", fn, line, fr_strerror());
		return -1;
	}

	return 0;
}

static int load_file (qse_raddic_t* dic, const qse_char_t *dir, const qse_char_t *fn, const qse_char_t *src_file, int src_line)
{
	qse_sio_t* sio = QSE_NULL;
	qse_char_t dirtmp[256]; /* TODO: longer path */
	char buf[256];
	char* p;
	qse_size_t line = 0;
	int vendor;
	int block_vendor;
#if 0
	struct stat statbuf;
#endif
	qse_mchar_t* argv[16]; /* TODO: what is the best size? */
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

	while (qse_sio_getmbs (sio, buf, QSE_COUNTOF(buf)) >= 0) 
	{
		line++;

		qse_strpac (buf);
		if (buf[0] == QSE_MT('\0') || buf[0] == QSE_MT('#')) continue;

		/*
		 *  Comment characters should NOT be appearing anywhere but
		 *  as start of a comment;
		 */
		p = qse_strchr (buf, QSE_MT('#'));
		if (p) *p = '\0';

		argc = str2argv(buf, argv, QSE_COUNTOF(argv));
		if (argc == 0) continue;

		if (argc == 1) 
		{
			fr_strerror_printf( "dict_init: %s[%d] invalid entry", fn, line);
			goto oops;
		}

		/*
		 *	Process VALUE lines.
		 */
		if (qse_strcasecmp(argv[0], QSE_T("VALUE")) == 0) 
		{
			if (process_value(fn, line, argv + 1, argc - 1) == -1) goto oops;
			continue;
		}

		/*
		 *	Perhaps this is an attribute.
		 */
		if (qse_strcasecmp(argv[0], QSE_T("ATTRIBUTE")) == 0) 
		{
			if (process_attribute(fn, line, block_vendor, block_tlv, argv + 1, argc - 1) == -1) goto oops;
			continue;
		}

		/*
		 *	See if we need to import another dictionary.
		 */
		if (qse_strcasecmp(argv[0], QSE_T("$INCLUDE")) == 0) 
		{
			if (load_file(dir, argv[1], fn, line) < 0) goto oops;
			continue;
		} /* $INCLUDE */

		if (qse_strcasecmp(argv[0], QSE_T("VALUE-ALIAS")) == 0) 
		{
			if (process_value_alias(fn, line, argv + 1, argc - 1) == -1) goto oops;
			continue;
		}

		/*
		 *	Process VENDOR lines.
		 */
		if (qse_strcasecmp(argv[0], QSE_T("VENDOR")) == 0) 
		{
			if (process_vendor(fn, line, argv + 1, argc - 1) == -1)  goto oops;
			continue;
		}

		if (qse_strcasecmp(argv[0], QSE_T("BEGIN-TLV")) == 0) 
		{
			if (argc != 2) 
			{
				fr_strerror_printf("dict_init: %s[%d] invalid BEGIN-TLV entry", fn, line);
				goto oops;
			}

			da = dict_attrbyname(argv[1]);
			if (!da) 
			{
				fr_strerror_printf("dict_init: %s[%d]: unknown attribute %s", fn, line, argv[1]);
				goto oops;
			}

			if (da->type != QSE_RADDIC_ATTR_TYPE_TLV) 
			{
				fr_strerror_printf("dict_init: %s[%d]: attribute %s is not of type tlv", fn, line, argv[1]);
				goto oops;
			}

			block_tlv = da;
			continue;
		} /* BEGIN-TLV */

		if (qse_strcasecmp(argv[0], "END-TLV") == 0) 
		{
			if (argc != 2) 
			{
				fr_strerror_printf("dict_init: %s[%d] invalid END-TLV entry", fn, line);
				goto oops;
			}

			da = dict_attrbyname(argv[1]);
			if (!da) 
			{
				fr_strerror_printf("dict_init: %s[%d]: unknown attribute %s", fn, line, argv[1]);
				goto oops;
			}

			if (da != block_tlv) 
			{
				fr_strerror_printf("dict_init: %s[%d]: END-TLV %s does not match any previous BEGIN-TLV", fn, line, argv[1]);
				goto oops;
			}
			block_tlv = QSE_NULL;
			continue;
		} /* END-VENDOR */

		if (qse_strcasecmp(argv[0], "BEGIN-VENDOR") == 0) {
			if (argc != 2) {
				fr_strerror_printf("dict_init: %s[%d] invalid BEGIN-VENDOR entry", fn, line);
				goto oops;
			}

			vendor = dict_vendorbyname(argv[1]);
			if (!vendor) {
				fr_strerror_printf("dict_init: %s[%d]: unknown vendor %s", fn, line, argv[1]);
				goto oops;
			}
			block_vendor = vendor;
			continue;
		} /* BEGIN-VENDOR */

		if (qse_strcasecmp(argv[0], QSE_T("END-VENDOR")) == 0) 
		{
			if (argc != 2) {
				fr_strerror_printf("dict_init: %s[%d] invalid END-VENDOR entry", fn, line);
				goto oops;
			}

			vendor = dict_vendorbyname(argv[1]);
			if (!vendor) 
			{
				fr_strerror_printf("dict_init: %s[%d]: unknown vendor %s", fn, line, argv[1]);
				goto oops;
			}

			if (vendor != block_vendor) {
				fr_strerror_printf(
					"dict_init: %s[%d]: END-VENDOR %s does not match any previous BEGIN-VENDOR",
					fn, line, argv[1]);
				goto oops;
			}
			block_vendor = 0;
			continue;
		} /* END-VENDOR */

		/*
		 *	Any other string: We don't recognize it.
		 */
		fr_strerror_printf("dict_init: %s[%d] invalid keyword \"%s\"", fn, line, argv[0]);
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
#define qse_raddic_value_t_MAX_NAME_LEN (128)
#define qse_raddic_vendor_t_MAX_NAME_LEN (128)
#define qse_raddic_attr_t_MAX_NAME_LEN (128)

static fr_hash_table_t *vendors_byname = QSE_NULL;
static fr_hash_table_t *vendors_byvalue = QSE_NULL;

static fr_hash_table_t *attributes_byname = QSE_NULL;
static fr_hash_table_t *attributes_byvalue = QSE_NULL;

static fr_hash_table_t *values_byvalue = QSE_NULL;
static fr_hash_table_t *values_byname = QSE_NULL;

static qse_raddic_attr_t *dict_base_attrs[256];

/*
 *	For faster HUP's, we cache the stat information for
 *	files we've $INCLUDEd
 */
typedef struct dict_stat_t {
	struct dict_stat_t *next;
	char	   	   *name;
	time_t		   mtime;
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


/*
 *	Create the hash of the name.
 *
 *	We copy the hash function here because it's substantially faster.
 */
#define FNV_MAGIC_INIT (0x811c9dc5)
#define FNV_MAGIC_PRIME (0x01000193)

static uint32_t dict_hashname(const char *name)
{
	uint32_t hash = FNV_MAGIC_INIT;
	const char *p;

	for (p = name; *p != '\0'; p++) {
		int c = *(const unsigned char *) p;
		if (isalpha(c)) c = tolower(c);

		hash *= FNV_MAGIC_PRIME;
		hash ^= (uint32_t ) (c & 0xff);
	}

	return hash;
}


/*
 *	Hash callback functions.
 */
static uint32_t dict_attr_name_hash(const void *data)
{
	return dict_hashname(((const qse_raddic_attr_t *)data)->name);
}

static int dict_attr_name_cmp(const void *one, const void *two)
{
	const qse_raddic_attr_t *a = one;
	const qse_raddic_attr_t *b = two;

	return qse_mbscasecmp(a->name, b->name);
}

static uint32_t dict_attr_value_hash(const void *data)
{
	uint32_t hash;
	const qse_raddic_attr_t *attr = data;

	hash = fr_hash(&attr->vendor, QSE_SIZEOF(attr->vendor));
	return fr_hash_update(&attr->attr, QSE_SIZEOF(attr->attr), hash);
}

static int dict_attr_value_cmp(const void *one, const void *two)
{
	const qse_raddic_attr_t *a = one;
	const qse_raddic_attr_t *b = two;

	if (a->vendor < b->vendor) return -1;
	if (a->vendor > b->vendor) return +1;

	return a->attr - b->attr;
}


static uint32_t dict_value_name_hash(const void *data)
{
	uint32_t hash;
	const qse_raddic_value_t *dval = data;

	hash = dict_hashname(dval->name);
	return fr_hash_update(&dval->attr, QSE_SIZEOF(dval->attr), hash);
}

static int dict_value_name_cmp(const void *one, const void *two)
{
	int rcode;
	const qse_raddic_value_t *a = one;
	const qse_raddic_value_t *b = two;

	rcode = a->attr - b->attr;
	if (rcode != 0) return rcode;

	return qse_mbscasecmp(a->name, b->name);
}

static uint32_t dict_value_value_hash(const void *data)
{
	uint32_t hash;
	const qse_raddic_value_t *dval = data;

	hash = fr_hash(&dval->attr, QSE_SIZEOF(dval->attr));
	return fr_hash_update(&dval->value, QSE_SIZEOF(dval->value), hash);
}

static int dict_value_value_cmp(const void *one, const void *two)
{
	int rcode;
	const qse_raddic_value_t *a = one;
	const qse_raddic_value_t *b = two;

	rcode = a->attr - b->attr;
	if (rcode != 0) return rcode;

	return a->value - b->value;
}


/*
 *	Free the list of stat buffers
 */
static void dict_stat_free(void)
{
	dict_stat_t *this, *next;

	free(stat_root_dir);
	stat_root_dir = QSE_NULL;
	free(stat_root_file);
	stat_root_file = QSE_NULL;

	if (!stat_head) {
		stat_tail = QSE_NULL;
		return;
	}

	for (this = stat_head; this != QSE_NULL; this = next) {
		next = this->next;
		free(this->name);
		free(this);
	}

	stat_head = stat_tail = QSE_NULL;
}


/*
 *	Add an entry to the list of stat buffers.
 */
static void dict_stat_add(const char *name, const struct stat *stat_buf)
{
	dict_stat_t *this;

	this = malloc(QSE_SIZEOF(*this));
	if (!this) return;
	QSE_MEMSET(this, 0, QSE_SIZEOF(*this));

	this->name = strdup(name);
	this->mtime = stat_buf->st_mtime;

	if (!stat_head) {
		stat_head = stat_tail = this;
	} else {
		stat_tail->next = this;
		stat_tail = this;
	}
}


/*
 *	See if any dictionaries have changed.  If not, don't
 *	do anything.
 */
static int dict_stat_check(const char *root_dir, const char *root_file)
{
	struct stat buf;
	dict_stat_t *this;

	if (!stat_root_dir) return 0;
	if (!stat_root_file) return 0;

	if (qse_mbscmp(root_dir, stat_root_dir) != 0) return 0;
	if (qse_mbscmp(root_file, stat_root_file) != 0) return 0;

	if (!stat_head) return 0; /* changed, reload */

	for (this = stat_head; this != QSE_NULL; this = this->next) {
		if (stat(this->name, &buf) < 0) return 0;

		if (buf.st_mtime != this->mtime) return 0;
	}

	return 1;
}

typedef struct fr_pool_t {
	void	*page_end;
	void	*free_ptr;
	struct fr_pool_t *page_free;
	struct fr_pool_t *page_next;
} fr_pool_t;

#define FR_POOL_SIZE (32768)
#define FR_ALLOC_ALIGN (8)

static fr_pool_t *dict_pool = QSE_NULL;

static fr_pool_t *fr_pool_create(void)
{
	fr_pool_t *fp = malloc(FR_POOL_SIZE);

	if (!fp) return QSE_NULL;

	QSE_MEMSET(fp, 0, FR_POOL_SIZE);

	fp->page_end = ((uint8_t *) fp) + FR_POOL_SIZE;
	fp->free_ptr = ((uint8_t *) fp) + QSE_SIZEOF(*fp);
	fp->page_free = fp;
	fp->page_next = QSE_NULL;
	return fp;
}

static void fr_pool_delete(fr_pool_t **pfp)
{
	fr_pool_t *fp, *next;

	if (!pfp || !*pfp) return;

	for (fp = *pfp; fp != QSE_NULL; fp = next) {
		next = fp->page_next;
		free(fp);
	}
}


static void *fr_pool_alloc(size_t size)
{
	void *ptr;

	if (size == 0) return QSE_NULL;

	if (size > 256) return QSE_NULL; /* shouldn't happen */

	if (!dict_pool) {
		dict_pool = fr_pool_create();
		if (!dict_pool) return QSE_NULL;
	}

	if ((size & (FR_ALLOC_ALIGN - 1)) != 0) {
		size += FR_ALLOC_ALIGN - (size & (FR_ALLOC_ALIGN - 1));
	}

	if ((((uint8_t *) dict_pool->page_free->free_ptr) + size) > (uint8_t *) dict_pool->page_free->page_end) {
		dict_pool->page_free->page_next = fr_pool_create();
		if (!dict_pool->page_free->page_next) return QSE_NULL;
		dict_pool->page_free = dict_pool->page_free->page_next;
	}

	ptr = dict_pool->page_free->free_ptr;
	dict_pool->page_free->free_ptr = ((uint8_t *) dict_pool->page_free->free_ptr) + size;

	return ptr;
}

/*
 *	Free the dictionary_attributes and dictionary_values lists.
 */
void dict_free(void)
{
	/*
	 *	Free the tables
	 */
	fr_hash_table_free(vendors_byname);
	fr_hash_table_free(vendors_byvalue);
	vendors_byname = QSE_NULL;
	vendors_byvalue = QSE_NULL;

	fr_hash_table_free(attributes_byname);
	fr_hash_table_free(attributes_byvalue);
	attributes_byname = QSE_NULL;
	attributes_byvalue = QSE_NULL;

	fr_hash_table_free(values_byname);
	fr_hash_table_free(values_byvalue);
	values_byname = QSE_NULL;
	values_byvalue = QSE_NULL;

	QSE_MEMSET(dict_base_attrs, 0, QSE_SIZEOF(dict_base_attrs));

	fr_pool_delete(&dict_pool);

	dict_stat_free();
}



/*
 *	Add an attribute to the dictionary.
 */
int dict_addattr(const char *name, int vendor, int type, int value,
		 ATTR_FLAGS flags)
{
	size_t namelen;
	static int      max_attr = 0;
	qse_raddic_attr_t	*attr;

	namelen = qse_mbslen(name);
	if (namelen >= qse_raddic_attr_t_MAX_NAME_LEN) {
		fr_strerror_printf("dict_addattr: attribute name too long");
		return -1;
	}

	/*
	 *	If the value is '-1', that means use a pre-existing
	 *	one (if it already exists).  If one does NOT already exist,
	 *	then create a new attribute, with a non-conflicting value,
	 *	and use that.
	 */
	if (value == -1) {
		if (dict_attrbyname(name)) {
			return 0; /* exists, don't add it again */
		}

		value = ++max_attr;

	} else if (vendor == 0) {
		/*
		 *  Update 'max_attr'
		 */
		if (value > max_attr) {
			max_attr = value;
		}
	}

	if (value < 0) {
		fr_strerror_printf("dict_addattr: ATTRIBUTE has invalid number (less than zero)");
		return -1;
	}

	if (value >= 65536) {
		fr_strerror_printf("dict_addattr: ATTRIBUTE has invalid number (larger than 65535).");
		return -1;
	}

	if (vendor) {
		qse_raddic_vendor_t *dv;
		static qse_raddic_vendor_t *last_vendor = QSE_NULL;

		if (flags.is_tlv && (flags.encrypt != FLAG_ENCRYPT_NONE)) {
			fr_strerror_printf("Sub-TLV's cannot be encrypted");
			return -1;
		}

		if (flags.has_tlv && (flags.encrypt != FLAG_ENCRYPT_NONE)) {
			fr_strerror_printf("TLV's cannot be encrypted");
			return -1;
		}

		if (flags.is_tlv && flags.has_tag) {
			fr_strerror_printf("Sub-TLV's cannot have a tag");
			return -1;
		}

		if (flags.has_tlv && flags.has_tag) {
			fr_strerror_printf("TLV's cannot have a tag");
			return -1;
		}

		/*
		 *	Most ATTRIBUTEs are bunched together by
		 *	VENDOR.  We can save a lot of lookups on
		 *	dictionary initialization by caching the last
		 *	vendor.
		 */
		if (last_vendor && (vendor == last_vendor->vendorpec)) {
			dv = last_vendor;
		} else {
			dv = dict_vendorbyvalue(vendor);
			last_vendor = dv;
		}

		/*
		 *	If the vendor isn't defined, die.
		 */
		if (!dv) {
			fr_strerror_printf("dict_addattr: Unknown vendor");
			return -1;
		}

		/*
		 *	FIXME: Switch over dv->type, and limit things
		 *	properly.
		 */
		if ((dv->type == 1) && (value >= 256) && !flags.is_tlv) {
			fr_strerror_printf("dict_addattr: ATTRIBUTE has invalid number (larger than 255).");
			return -1;
		} /* else 256..65535 are allowed */
	}

	/*
	 *	Create a new attribute for the list
	 */
	if ((attr = fr_pool_alloc(QSE_SIZEOF(*attr) + namelen)) == QSE_NULL) {
		fr_strerror_printf("dict_addattr: out of memory");
		return -1;
	}

	memcpy(attr->name, name, namelen);
	attr->name[namelen] = '\0';
	attr->attr = value;
	attr->attr |= (vendor << 16); /* FIXME: hack */
	attr->vendor = vendor;
	attr->type = type;
	attr->flags = flags;
	attr->vendor = vendor;

	/*
	 *	Insert the attribute, only if it's not a duplicate.
	 */
	if (!fr_hash_table_insert(attributes_byname, attr)) {
		qse_raddic_attr_t	*a;

		/*
		 *	If the attribute has identical number, then
		 *	ignore the duplicate.
		 */
		a = fr_hash_table_finddata(attributes_byname, attr);
		if (a && (qse_mbscasecmp(a->name, attr->name) == 0)) {
			if (a->attr != attr->attr) {
				fr_strerror_printf("dict_addattr: Duplicate attribute name %s", name);
				fr_pool_free(attr);
				return -1;
			}

			/*
			 *	Same name, same vendor, same attr,
			 *	maybe the flags and/or type is
			 *	different.  Let the new value
			 *	over-ride the old one.
			 */
		}


		fr_hash_table_delete(attributes_byvalue, a);

		if (!fr_hash_table_replace(attributes_byname, attr)) {
			fr_strerror_printf("dict_addattr: Internal error storing attribute %s", name);
			fr_pool_free(attr);
			return -1;
		}
	}

	/*
	 *	Insert the SAME pointer (not free'd when this entry is
	 *	deleted), into another table.
	 *
	 *	We want this behaviour because we want OLD names for
	 *	the attributes to be read from the configuration
	 *	files, but when we're printing them, (and looking up
	 *	by value) we want to use the NEW name.
	 */
	if (!fr_hash_table_replace(attributes_byvalue, attr)) {
		fr_strerror_printf("dict_addattr: Failed inserting attribute name %s", name);
		return -1;
	}

	if (!vendor && (value > 0) && (value < 256)) {
	 	 dict_base_attrs[value] = attr;
	}

	return 0;
}



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
static int process_value(const char* fn, const qse_size_t line, char **argv,
			 int argc)
{
	int	value;

	if (argc != 3) {
		fr_strerror_printf("dict_init: %s[%d]: invalid VALUE line",
			fn, line);
		return -1;
	}
	/*
	 *	For Compatibility, skip "Server-Config"
	 */
	if (qse_mbscasecmp(argv[0], "Server-Config") == 0) return 0;

	/*
	 *	Validate all entries
	 */
	if (!sscanf_i(argv[2], &value)) {
		fr_strerror_printf("dict_init: %s[%d]: invalid value", fn, line);
		return -1;
	}

	if (dict_addvalue(argv[1], argv[0], value) < 0) {
		fr_strerror_printf("dict_init: %s[%d]: %s", fn, line, fr_strerror());
		return -1;
	}

	return 0;
}


/*
 *	Process the VALUE-ALIAS command
 *
 *	This allows VALUE mappings to be shared among multiple
 *	attributes.
 */
static int process_value_alias(const char* fn, const qse_size_t line, char **argv,
			       int argc)
{
	qse_raddic_attr_t *my_da, *da;
	qse_raddic_value_t *dval;

	if (argc != 2) {
		fr_strerror_printf("dict_init: %s[%d]: invalid VALUE-ALIAS line", fn, line);
		return -1;
	}

	my_da = dict_attrbyname(argv[0]);
	if (!my_da) {
		fr_strerror_printf("dict_init: %s[%d]: ATTRIBUTE \"%s\" does not exist",
			   fn, line, argv[1]);
		return -1;
	}

	if (my_da->flags.has_value) {
		fr_strerror_printf("dict_init: %s[%d]: Cannot add VALUE-ALIAS to ATTRIBUTE \"%s\" with pre-existing VALUE",
			   fn, line, argv[0]);
		return -1;
	}

	if (my_da->flags.has_value_alias) {
		fr_strerror_printf("dict_init: %s[%d]: Cannot add VALUE-ALIAS to ATTRIBUTE \"%s\" with pre-existing VALUE-ALIAS",
			   fn, line, argv[0]);
		return -1;
	}

	da = dict_attrbyname(argv[1]);
	if (!da) {
		fr_strerror_printf("dict_init: %s[%d]: Cannot find ATTRIBUTE \"%s\" for alias",
			   fn, line, argv[1]);
		return -1;
	}

	if (!da->flags.has_value) {
		fr_strerror_printf("dict_init: %s[%d]: VALUE-ALIAS cannot refer to ATTRIBUTE %s: It has no values",
			   fn, line, argv[1]);
		return -1;
	}

	if (da->flags.has_value_alias) {
		fr_strerror_printf("dict_init: %s[%d]: Cannot add VALUE-ALIAS to ATTRIBUTE \"%s\" which itself has a VALUE-ALIAS",
			   fn, line, argv[1]);
		return -1;
	}

	if (my_da->type != da->type) {
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


/*
 *	Process the VENDOR command
 */
static int process_vendor(const char* fn, const qse_size_t line, char **argv,
			  int argc)
{
	int	value;
	int	continuation = 0;
	const	char *format = QSE_NULL;

	if ((argc < 2) || (argc > 3)) {
		fr_strerror_printf( "dict_init: %s[%d] invalid VENDOR entry",
			    fn, line);
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


#endif //XXX
