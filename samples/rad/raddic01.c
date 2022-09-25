#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/rad/raddic.h>
#include <qse/si/sio.h>
#include <string.h>
#include <qse/cmn/test.h>

#if 0
#include <qse/rad/radmsg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) goto oops; \
	} while (0)

static int test1 ()
{
	qse_raddic_t* dic;
	qse_raddic_vendor_t* vendor, * v;
	int i;

	dic = qse_raddic_open (QSE_MMGR_GETDFL(), 0);
	QSE_TESASSERT1 (dic != QSE_NULL, QSE_T("unable to create a radius dictionary"));

	vendor = qse_raddic_addvendor (dic, QSE_T("abiyo.net"), 12365);
	QSE_TESASSERT1 (vendor != QSE_NULL, QSE_T("unable to add the first vendor"));
	QSE_TESASSERT1 (vendor->vendorpec == 12365, QSE_T("the vendor value is not 12365"));
	QSE_TESASSERT1 (qse_strcasecmp(vendor->name, QSE_T("abiyo.net")) == 0, QSE_T("the vendor name is not abiyo.net"));

	vendor = qse_raddic_addvendor (dic, QSE_T("abiyo.net"), 99999);
	QSE_TESASSERT1 (vendor == QSE_NULL, QSE_T("a duplicate name must not be allowed"));

	vendor = qse_raddic_addvendor (dic, QSE_T("abiyo-aliased.net"), 12365);
	QSE_TESASSERT1 (vendor != QSE_NULL, QSE_T("unable to add a duplicate id"));

	vendor = qse_raddic_findvendorbyname (dic, QSE_T("Abiyo.Net"));
	QSE_TESASSERT1 (vendor != QSE_NULL && vendor->vendorpec == 12365, QSE_T("unable to find a vendor named Abiyo.Net"));

	vendor = qse_raddic_findvendorbyvalue (dic, 12365);
	QSE_TESASSERT1 (vendor != QSE_NULL && vendor->vendorpec == 12365, QSE_T("unable to find a vendor of value 12365"));
	QSE_TESASSERT1 (qse_strcasecmp(vendor->name, QSE_T("abiyo-aliased.net")) == 0, QSE_T("unable to find a vendor of value 12365"));

	vendor = qse_raddic_findvendorbyname (dic, QSE_T("Abiyo-aliased.Net"));
	QSE_TESASSERT1 (vendor != QSE_NULL && vendor->vendorpec == 12365, QSE_T("unable to find a vendor named Abiyo-aliased.Net"));
	QSE_TESASSERT1 (qse_strcasecmp(vendor->name, QSE_T("abiyo-aliased.net")) == 0, QSE_T("unable to find a vendor of value 12365"));

#define COUNT1 600
#define COUNT2 700

	for (i = 1; i < COUNT1; i++)
	{
		qse_char_t tmp[64];
		qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test-%d"), i);
		vendor = qse_raddic_addvendor (dic, tmp, i);
		QSE_TESASSERT1 (vendor != QSE_NULL, QSE_T("unable to add a vendor"));
		QSE_TESASSERT1 (vendor->vendorpec == i, QSE_T("wrong vendor value"));
		QSE_TESASSERT1 (qse_strcasecmp(vendor->name, tmp) == 0, QSE_T("wrong vendor name"));
	}

	for (i = 1; i < COUNT1; i++)
	{
		qse_char_t tmp[64];
		qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test-%d"), i);
		vendor = qse_raddic_findvendorbyname (dic, tmp);
		QSE_TESASSERT1 (vendor != QSE_NULL, QSE_T("unable to find a vendor"));
		QSE_TESASSERT1 (vendor->vendorpec == i, QSE_T("wrong vendor value"));
		QSE_TESASSERT1 (qse_strcasecmp(vendor->name, tmp) == 0, QSE_T("wrong vendor name"));
	}

	for (i = 1; i < COUNT1; i++)
	{
		qse_char_t tmp[64];
		qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test-%d"), i);
		vendor = qse_raddic_findvendorbyvalue (dic, i);
		QSE_TESASSERT1 (vendor != QSE_NULL, QSE_T("unable to find a vendor"));
		QSE_TESASSERT1 (vendor->vendorpec == i, QSE_T("wrong vendor value"));
		QSE_TESASSERT1 (qse_strcasecmp(vendor->name, tmp) == 0, QSE_T("wrong vendor name"));
	}

	for (i = COUNT1; i < COUNT2; i++)
	{
		qse_char_t tmp[64];
		qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test-%d"), i);
		vendor = qse_raddic_addvendor (dic, tmp, COUNT1); 
		// insert different items with the same value
		QSE_TESASSERT1 (vendor != QSE_NULL, QSE_T("unable to add a vendor"));
		QSE_TESASSERT1 (vendor->vendorpec == COUNT1, QSE_T("wrong vendor value"));
		QSE_TESASSERT1 (qse_strcasecmp(vendor->name, tmp) == 0, QSE_T("wrong vendor name"));

		v = qse_raddic_findvendorbyvalue (dic, COUNT1);
		QSE_TESASSERT1 (vendor == v, QSE_T("unable to find a last added vendor by value"));
	}

	for (i = COUNT1; i < COUNT2 - 1; i++)
	{
		qse_char_t tmp[64];
		int n;

		qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test-%d"), i);

		n = qse_raddic_deletevendorbyname (dic, tmp); 
		QSE_TESASSERT1 (n == 0, QSE_T("unable to delete a vendor"));

		v = qse_raddic_findvendorbyname (dic, tmp);
		QSE_TESASSERT1 (v == QSE_NULL, QSE_T("vendor found errorenously"));

		if (i == COUNT2 - 1)
		{
			v = qse_raddic_findvendorbyvalue (dic, COUNT1);
			QSE_TESASSERT1 (v == QSE_NULL, QSE_T("vendor of COUNT1 found errorenously"));
		}
		else
		{
			qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test-%d"), i + 1);
			v = qse_raddic_findvendorbyname (dic, tmp);
			QSE_TESASSERT1 (v != QSE_NULL && v->vendorpec == COUNT1 && qse_strcasecmp(tmp, v->name) == 0, QSE_T("unable to find an expected vendor"));

			v = qse_raddic_findvendorbyvalue (dic, COUNT1);
			QSE_TESASSERT1 (v != QSE_NULL && v->vendorpec == COUNT1, QSE_T("unable to find the vendor of COUNT1"));
		}
	}

	for (i = 1; i < COUNT1; i++)
	{
		qse_char_t tmp[64];
		int n;

		qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test-%d"), i);
		n = qse_raddic_deletevendorbyname (dic, tmp);
		QSE_TESASSERT1 (n == 0, QSE_T("unable to delete a vendor"));
	}

	for (i = 1; i < COUNT1; i++)
	{
		qse_char_t tmp[64];
		qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test-%d"), i);
		v = qse_raddic_addvendor (dic, tmp, i);
		QSE_TESASSERT1 (v != QSE_NULL && v->vendorpec == i, QSE_T("unable to add a vendor"));

		qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("testx-%d"), i);
		v = qse_raddic_addvendor (dic, tmp, i);
		QSE_TESASSERT1 (v != QSE_NULL && v->vendorpec == i, QSE_T("unable to add a vendor"));

		qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("testy-%d"), i);
		v = qse_raddic_addvendor (dic, tmp, i);
		QSE_TESASSERT1 (v != QSE_NULL && v->vendorpec == i, QSE_T("unable to add a vendor"));
	}

	for (i = 1; i < COUNT1; i++)
	{
		int n;
		n = qse_raddic_deletevendorbyvalue (dic, i);
		QSE_TESASSERT1 (n == 0, QSE_T("unable to delete a vendor by value"));

		n = qse_raddic_deletevendorbyvalue (dic, i);
		QSE_TESASSERT1 (n == 0, QSE_T("unable to delete a vendor by value"));

		n = qse_raddic_deletevendorbyvalue (dic, i);
		QSE_TESASSERT1 (n == 0, QSE_T("unable to delete a vendor by value"));

		n = qse_raddic_deletevendorbyvalue (dic, i);
		QSE_TESASSERT1 (n <= -1, QSE_T("erroreneously successful vendor deletion by value"));
	}

	qse_raddic_close (dic);
	return 0;

oops:
	if (dic) qse_raddic_close (dic);
	return -1;
}


static int test2 ()
{
	qse_raddic_t* dic;
	qse_raddic_attr_t* attr;
	qse_raddic_attr_flags_t f;
	qse_raddic_const_t* con;
	int i, j;

	dic = qse_raddic_open (QSE_MMGR_GETDFL(), 0);
	QSE_TESASSERT1 (dic != QSE_NULL, QSE_T("unable to create a radius dictionary"));

	memset (&f, 0, QSE_SIZEOF(f));

	for (j = 0; j < 100; j++)
	{
		for (i = 0; i <= 255; i++)
		{
			qse_char_t tmp[64];
			qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test-%d-%d"), j, i);
			attr = qse_raddic_addattr (dic, tmp, j, QSE_RADDIC_ATTR_TYPE_UINT32, i, &f);
			QSE_TESASSERT1 (attr != QSE_NULL, QSE_T("unable to add an attribute"));
			QSE_TESASSERT1 (attr->attr == QSE_RADDIC_ATTR_MAKE(j, i), QSE_T("wrong attr value"));
			QSE_TESASSERT1 (qse_strcasecmp(attr->name, tmp) == 0, QSE_T("wrong attr name"));
		}
	}

	for (j = 0; j < 100; j++)
	{
		for (i = 0; i <= 255; i++)
		{
			qse_char_t tmp[64];
			qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test-%d-%d"), j, i);
			attr = qse_raddic_findattrbyname (dic, tmp);
			QSE_TESASSERT1 (attr != QSE_NULL, QSE_T("unable to find an attribute"));
			QSE_TESASSERT1 (attr->attr == QSE_RADDIC_ATTR_MAKE(j, i), QSE_T("wrong attr value"));
			QSE_TESASSERT1 (qse_strcasecmp(attr->name, tmp) == 0, QSE_T("wrong attr name"));

			attr = qse_raddic_findattrbyvalue (dic, QSE_RADDIC_ATTR_MAKE(j, i));
			QSE_TESASSERT1 (attr != QSE_NULL, QSE_T("unable to find an attribute"));
			QSE_TESASSERT1 (attr->attr == QSE_RADDIC_ATTR_MAKE(j, i), QSE_T("wrong attr value"));
			QSE_TESASSERT1 (qse_strcasecmp(attr->name, tmp) == 0, QSE_T("wrong attr name"));
		}
	}

	for (j = 0; j < 100; j++)
	{
		for (i = 0; i <= 255; i++)
		{
			qse_char_t tmp[64];
			qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("testx-%d-%d"), j, i);
			attr = qse_raddic_addattr (dic, tmp, j, QSE_RADDIC_ATTR_TYPE_UINT32, i, &f);
			QSE_TESASSERT1 (attr != QSE_NULL, QSE_T("unable to add an attribute"));
			QSE_TESASSERT1 (attr->attr == QSE_RADDIC_ATTR_MAKE(j, i), QSE_T("wrong attr value"));
			QSE_TESASSERT1 (qse_strcasecmp(attr->name, tmp) == 0, QSE_T("wrong attr name"));

			qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("testy-%d-%d"), j, i);
			attr = qse_raddic_addattr (dic, tmp, j, QSE_RADDIC_ATTR_TYPE_UINT32, i, &f);
			QSE_TESASSERT1 (attr != QSE_NULL, QSE_T("unable to add an attribute"));
			QSE_TESASSERT1 (attr->attr == QSE_RADDIC_ATTR_MAKE(j, i), QSE_T("wrong attr value"));
			QSE_TESASSERT1 (qse_strcasecmp(attr->name, tmp) == 0, QSE_T("wrong attr name"));
		}
	}

	for (j = 0; j < 100; j++)
	{
		for (i = 0; i <= 255; i++)
		{
			qse_char_t tmp[64], tmpx[64], tmpy[64];
			qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test-%d-%d"), j, i);
			qse_strxfmt(tmpx, QSE_COUNTOF(tmpx), QSE_T("testx-%d-%d"), j, i);
			qse_strxfmt(tmpy, QSE_COUNTOF(tmpy), QSE_T("testy-%d-%d"), j, i);

			attr = qse_raddic_findattrbyname (dic, tmp);
			QSE_TESASSERT1 (attr != QSE_NULL, QSE_T("unable to add an attribute"));
			QSE_TESASSERT1 (attr->attr == QSE_RADDIC_ATTR_MAKE(j, i), QSE_T("wrong attr value"));
			QSE_TESASSERT1 (qse_strcasecmp(attr->name, tmp) == 0, QSE_T("wrong attr name"));

			attr = qse_raddic_findattrbyname (dic, tmpx);
			QSE_TESASSERT1 (attr != QSE_NULL, QSE_T("unable to add an attribute"));
			QSE_TESASSERT1 (attr->attr == QSE_RADDIC_ATTR_MAKE(j, i), QSE_T("wrong attr value"));
			QSE_TESASSERT1 (qse_strcasecmp(attr->name, tmpx) == 0, QSE_T("wrong attr name"));

			attr = qse_raddic_findattrbyname (dic, tmpy);
			QSE_TESASSERT1 (attr != QSE_NULL, QSE_T("unable to add an attribute"));
			QSE_TESASSERT1 (attr->attr == QSE_RADDIC_ATTR_MAKE(j, i), QSE_T("wrong attr value"));
			QSE_TESASSERT1 (qse_strcasecmp(attr->name, tmpy) == 0, QSE_T("wrong attr name"));

			attr = qse_raddic_findattrbyvalue (dic, QSE_RADDIC_ATTR_MAKE(j, i));
			QSE_TESASSERT1 (attr != QSE_NULL, QSE_T("unable to add an attribute"));
			QSE_TESASSERT1 (attr->attr == QSE_RADDIC_ATTR_MAKE(j, i), QSE_T("wrong attr value"));
			QSE_TESASSERT1 (qse_strcasecmp(attr->name, tmpy) == 0, QSE_T("wrong attr name"));

			QSE_TESASSERT1 (attr->nexta != QSE_NULL, QSE_T("unable to find an old attribute"));
			QSE_TESASSERT1 (attr->nexta->attr == QSE_RADDIC_ATTR_MAKE(j, i), QSE_T("wrong attr value"));
			QSE_TESASSERT1 (qse_strcasecmp(attr->nexta->name, tmpx) == 0, QSE_T("wrong attr name"));

			QSE_TESASSERT1 (attr->nexta->nexta != QSE_NULL, QSE_T("unable to find an old attribute"));
			QSE_TESASSERT1 (attr->nexta->nexta->attr == QSE_RADDIC_ATTR_MAKE(j, i), QSE_T("wrong attr value"));
			QSE_TESASSERT1 (qse_strcasecmp(attr->nexta->nexta->name, tmp) == 0, QSE_T("wrong attr name"));

			QSE_TESASSERT1 (attr->nexta->nexta->nexta == QSE_NULL, QSE_T("wrong attribute chian"));
		}
	}

	for (j = 0; j < 100; j++)
	{
		for (i = 0; i <= 255; i++)
		{
			qse_char_t tmp[64];
			int n;

			qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("testx-%d-%d"), j, i);
			n = qse_raddic_deleteattrbyname (dic, tmp);
			QSE_TESASSERT1 (n == 0, QSE_T("erroreneous attribute deletion failure by name"));
		}
	}

	for (j = 0; j < 100; j++)
	{
		for (i = 0; i <= 255; i++)
		{
			qse_char_t tmp[64], tmpx[64], tmpy[64];
			qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test-%d-%d"), j, i);
			qse_strxfmt(tmpx, QSE_COUNTOF(tmpx), QSE_T("testx-%d-%d"), j, i);
			qse_strxfmt(tmpy, QSE_COUNTOF(tmpy), QSE_T("testy-%d-%d"), j, i);

			attr = qse_raddic_findattrbyname (dic, tmp);
			QSE_TESASSERT1 (attr != QSE_NULL, QSE_T("unable to add an attribute"));
			QSE_TESASSERT1 (attr->attr == QSE_RADDIC_ATTR_MAKE(j, i), QSE_T("wrong attr value"));
			QSE_TESASSERT1 (qse_strcasecmp(attr->name, tmp) == 0, QSE_T("wrong attr name"));

			attr = qse_raddic_findattrbyname (dic, tmpx);
			QSE_TESASSERT1 (attr == QSE_NULL, QSE_T("erroneous search success"));

			attr = qse_raddic_findattrbyname (dic, tmpy);
			QSE_TESASSERT1 (attr != QSE_NULL, QSE_T("unable to add an attribute"));
			QSE_TESASSERT1 (attr->attr == QSE_RADDIC_ATTR_MAKE(j, i), QSE_T("wrong attr value"));
			QSE_TESASSERT1 (qse_strcasecmp(attr->name, tmpy) == 0, QSE_T("wrong attr name"));

			attr = qse_raddic_findattrbyvalue (dic, QSE_RADDIC_ATTR_MAKE(j, i));
			QSE_TESASSERT1 (attr != QSE_NULL, QSE_T("unable to add an attribute"));
			QSE_TESASSERT1 (attr->attr == QSE_RADDIC_ATTR_MAKE(j, i), QSE_T("wrong attr value"));
			QSE_TESASSERT1 (qse_strcasecmp(attr->name, tmpy) == 0, QSE_T("wrong attr name"));

			QSE_TESASSERT1 (attr->nexta != QSE_NULL, QSE_T("unable to find an old attribute"));
			QSE_TESASSERT1 (attr->nexta->attr == QSE_RADDIC_ATTR_MAKE(j, i), QSE_T("wrong attr value"));
			QSE_TESASSERT1 (qse_strcasecmp(attr->nexta->name, tmp) == 0, QSE_T("wrong attr name"));

			QSE_TESASSERT1 (attr->nexta->nexta == QSE_NULL, QSE_T("wrong attribute chian"));
		}
	}


	{
		int n;
		n = qse_raddic_deleteattrbyvalue (dic, QSE_RADDIC_ATTR_MAKE(0, 0));
		QSE_TESASSERT1 (n == 0, QSE_T("erroneous deletion failure by value"));

		n = qse_raddic_deleteattrbyvalue (dic, QSE_RADDIC_ATTR_MAKE(0, 0));
		QSE_TESASSERT1 (n == 0, QSE_T("erroneous deletion failure by value"));

		n = qse_raddic_deleteattrbyvalue (dic, QSE_RADDIC_ATTR_MAKE(0, 0));
		QSE_TESASSERT1 (n <= -1, QSE_T("erroneous deletion success by value"));
	}


	for (j = 1; j < 100; j++)
	{
		for (i = 1; i <= 255; i++)
		{
			qse_char_t constr[64], attrstr[64];

			qse_strxfmt(attrstr, QSE_COUNTOF(attrstr), QSE_T("test-%d-%d"), j, i);
			qse_strxfmt(constr, QSE_COUNTOF(constr), QSE_T("const-%d-%d"), j, i);
			con = qse_raddic_addconst (dic, constr, attrstr, 10);
			QSE_TESASSERT2 (con != QSE_NULL, QSE_T("unable to add an constant"), qse_raddic_geterrmsg(dic));
			QSE_TESASSERT1 (con->value == 10, QSE_T("wrong constant value"));
			QSE_TESASSERT1 (qse_strcasecmp(con->name, constr) == 0, QSE_T("wrong constant name"));
		}
	}

	for (j = 1; j < 100; j++)
	{
		for (i = 1; i <= 255; i++)
		{
			qse_char_t constr[64];

			qse_strxfmt(constr, QSE_COUNTOF(constr), QSE_T("const-%d-%d"), j, i);

			con = qse_raddic_findconstbyname (dic, QSE_RADDIC_ATTR_MAKE(j, i), constr);
			QSE_TESASSERT1 (con != QSE_NULL, QSE_T("unable to find an constant"));
			QSE_TESASSERT1 (con->attr == QSE_RADDIC_ATTR_MAKE(j, i), QSE_T("wrong constant value"));
			QSE_TESASSERT1 (con->value == 10, QSE_T("wrong constant value"));

			con = qse_raddic_findconstbyvalue (dic, QSE_RADDIC_ATTR_MAKE(j, i), 10);
			QSE_TESASSERT1 (con != QSE_NULL, QSE_T("unable to find an constant"));
			QSE_TESASSERT1 (con->value == 10, QSE_T("wrong constant value"));
			QSE_TESASSERT1 (con->attr == QSE_RADDIC_ATTR_MAKE(j, i), QSE_T("wrong constant value"));
		}
	}

	{
		int n;

		n = qse_raddic_deleteconstbyname (dic, QSE_RADDIC_ATTR_MAKE(1,1), QSE_T("const-1-1"));
		QSE_TESASSERT1 (n == 0, QSE_T("erroneous constant deletion failure"));

		n = qse_raddic_deleteconstbyname (dic, QSE_RADDIC_ATTR_MAKE(1,1), QSE_T("const-1-1"));
		QSE_TESASSERT1 (n <= -1, QSE_T("erroneous constant deletion success"));

		n = qse_raddic_deleteconstbyvalue (dic, QSE_RADDIC_ATTR_MAKE(2,2), 20);
		QSE_TESASSERT1 (n <= -1, QSE_T("erroneous constant deletion success"));

		n = qse_raddic_deleteconstbyvalue (dic, QSE_RADDIC_ATTR_MAKE(2,2), 10);
		QSE_TESASSERT1 (n == 0, QSE_T("erroneous constant deletion success"));

		n = qse_raddic_deleteconstbyvalue (dic, QSE_RADDIC_ATTR_MAKE(2,2), 10);
		QSE_TESASSERT1 (n <= -1, QSE_T("erroneous constant deletion success"));
	}

	qse_raddic_close (dic);
	return 0;

oops:
	if (dic) qse_raddic_close (dic);
	return -1;
}

static int test3 ()
{
	qse_raddic_t* dic;
	qse_raddic_vendor_t* v;
	qse_raddic_const_t* c;
	int n, trait;

	dic = qse_raddic_open (QSE_MMGR_GETDFL(), 0);
	QSE_TESASSERT1 (dic != QSE_NULL, QSE_T("unable to create a radius dictionary"));

	trait = QSE_RADDIC_ALLOW_CONST_WITHOUT_ATTR | QSE_RADDIC_ALLOW_DUPLICATE_CONST;
	n = qse_raddic_setopt (dic, QSE_RADDIC_TRAIT, &trait);
	QSE_TESASSERT1 (n == 0, QSE_T("cannot set trait"));

	n = qse_raddic_load (dic, QSE_T("fr/dictionary"));
	QSE_TESASSERT2 (n == 0, QSE_T("unabled to load dictionary"), qse_raddic_geterrmsg(dic));

	v = qse_raddic_findvendorbyname (dic, QSE_T("cisco"));
	QSE_TESASSERT1 (v && v->vendorpec == 9, "wrong vendor value");

	c = qse_raddic_findconstbyname (dic, QSE_RADDIC_ATTR_MAKE(0,49), QSE_T("Reauthentication-Failure"));
	QSE_TESASSERT1 (c && c->value == 20, QSE_T("wrong constant value"));

	qse_raddic_close (dic);
	return 0;

oops:
	if (dic) qse_raddic_close (dic);
	return -1;
}

#if 0
static int test10()
{
	struct sockaddr_in sin;
	int s;
	char buf[10000];

	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) return -1;

	memset (&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr("192.168.1.2");
	sin.sin_port = htons(1812);

	qse_rad_initialize (buf, QSE_RAD_ACCESS_REQUEST, 99);
	qse_rad_insert_extended_vendor_specific_attribute (buf, sizeof(buf), 243, 12345, 5, "xxxxxx", 6);

	sendto (s, buf, ntohs(((qse_rad_hdr_t*)buf)->length), 0, &sin, sizeof(sin));

	close (s);
	return 0;
}
#endif

int main ()
{
	qse_open_stdsios (); 

	R (test1);
	R (test2);
	R (test3);

#if 0
	R (test10);
#endif

oops:
	qse_close_stdsios ();
	return 0;
}

