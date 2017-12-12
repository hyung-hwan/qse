#include <qse/cmn/mem.h>
#include <qse/cmn/str.h>
#include <qse/rad/raddic.h>
#include <qse/si/sio.h>

#define R(f) \
	do { \
		qse_printf (QSE_T("== %s ==\n"), QSE_T(#f)); \
		if (f() == -1) return -1; \
	} while (0)

#define FAIL(msg) qse_printf(QSE_T("FAILURE in %hs line %d - %hs\n"), __func__, __LINE__, msg)
#define _assert(test,msg) do { if (!(test)) { FAIL(msg); return -1; } } while(0)
#define _verify(test) do { int r=test(); tests_run++; if(r) return r; } while(0)

static int test1 ()
{
	qse_raddic_t* dic;
	qse_raddic_vendor_t* vendor, * v;
	qse_raddic_attr_t* attr, * a;
	int i;

	dic = qse_raddic_open (QSE_MMGR_GETDFL(), 0);
	_assert (dic != QSE_NULL, "unable to create a radius dictionary");

	vendor = qse_raddic_addvendor (dic, QSE_T("abiyo.net"), 12365);
	_assert (vendor != QSE_NULL, "unable to add the first vendor");
	_assert (vendor->vendorpec == 12365, "the vendor value is not 12365");
	_assert (qse_strcasecmp(vendor->name, QSE_T("abiyo.net")) == 0, "the vendor name is not abiyo.net");

	vendor = qse_raddic_addvendor (dic, QSE_T("abiyo.net"), 99999);
	_assert (vendor == QSE_NULL, "a duplicate name must not be allowed");

	vendor = qse_raddic_addvendor (dic, QSE_T("abiyo-aliased.net"), 12365);
	_assert (vendor != QSE_NULL, "unable to add a duplicate id");

	vendor = qse_raddic_findvendorbyname (dic, QSE_T("Abiyo.Net"));
	_assert (vendor != QSE_NULL && vendor->vendorpec == 12365, "unable to find a vendor named Abiyo.Net");

	vendor = qse_raddic_findvendorbyvalue (dic, 12365);
	_assert (vendor != QSE_NULL && vendor->vendorpec == 12365, "unable to find a vendor of value 12365");
	_assert (qse_strcasecmp(vendor->name, QSE_T("abiyo-aliased.net")) == 0, "unable to find a vendor of value 12365");

	vendor = qse_raddic_findvendorbyname (dic, QSE_T("Abiyo-aliased.Net"));
	_assert (vendor != QSE_NULL && vendor->vendorpec == 12365, "unable to find a vendor named Abiyo-aliased.Net");
	_assert (qse_strcasecmp(vendor->name, QSE_T("abiyo-aliased.net")) == 0, "unable to find a vendor of value 12365");

#define COUNT1 600
#define COUNT2 700

	for (i = 1; i < COUNT1; i++)
	{
		qse_char_t tmp[64];
		qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test%d"), i);
		vendor = qse_raddic_addvendor (dic, tmp, i);
		_assert (vendor != QSE_NULL, "unable to add a vendor");
		_assert (vendor->vendorpec == i, "wrong vendor value");
		_assert (qse_strcasecmp(vendor->name, tmp) == 0, "wrong vendor name");
	}

	for (i = 1; i < COUNT1; i++)
	{
		qse_char_t tmp[64];
		qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test%d"), i);
		vendor = qse_raddic_findvendorbyname (dic, tmp);
		_assert (vendor != QSE_NULL, "unable to find a vendor");
		_assert (vendor->vendorpec == i, "wrong vendor value");
		_assert (qse_strcasecmp(vendor->name, tmp) == 0, "wrong vendor name");
	}

	for (i = 1; i < COUNT1; i++)
	{
		qse_char_t tmp[64];
		qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test%d"), i);
		vendor = qse_raddic_findvendorbyvalue (dic, i);
		_assert (vendor != QSE_NULL, "unable to find a vendor");
		_assert (vendor->vendorpec == i, "wrong vendor value");
		_assert (qse_strcasecmp(vendor->name, tmp) == 0, "wrong vendor name");
	}

	for (i = COUNT1; i < COUNT2; i++)
	{
		qse_char_t tmp[64];
		qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test%d"), i);
		vendor = qse_raddic_addvendor (dic, tmp, COUNT1); 
		// insert different items with the same value
		_assert (vendor != QSE_NULL, "unable to add a vendor");
		_assert (vendor->vendorpec == COUNT1, "wrong vendor value");
		_assert (qse_strcasecmp(vendor->name, tmp) == 0, "wrong vendor name");

		v = qse_raddic_findvendorbyvalue (dic, COUNT1);
		_assert (vendor == v, "unable to find a last added vendor by value");
	}

	for (i = COUNT1; i < COUNT2 - 1; i++)
	{
		qse_char_t tmp[64];
		int n;

		qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test%d"), i);

		n = qse_raddic_deletevendorbyname (dic, tmp); 
		_assert (n == 0, "unable to delete a vendor");

		v = qse_raddic_findvendorbyname (dic, tmp);
		_assert (v == QSE_NULL, "vendor found errorenously");

		if (i == COUNT2 - 1)
		{
			v = qse_raddic_findvendorbyvalue (dic, COUNT1);
			_assert (v == QSE_NULL, "vendor of COUNT1 found errorenously");
		}
		else
		{
			qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test%d"), i + 1);
			v = qse_raddic_findvendorbyname (dic, tmp);
			_assert (v != QSE_NULL && v->vendorpec == COUNT1 && qse_strcasecmp(tmp, v->name) == 0, "unable to find an expected vendor");

			v = qse_raddic_findvendorbyvalue (dic, COUNT1);
			_assert (v != QSE_NULL && v->vendorpec == COUNT1, "unable to find the vendor of COUNT1");
		}
	}

	for (i = 1; i < COUNT1; i++)
	{
		qse_char_t tmp[64];
		int n;

		qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test%d"), i);
		n = qse_raddic_deletevendorbyname (dic, tmp);
		_assert (n == 0, "unable to delete a vendor");
	}

	for (i = 1; i < COUNT1; i++)
	{
		qse_char_t tmp[64];
		qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test%d"), i);
		v = qse_raddic_addvendor (dic, tmp, i);
		_assert (v != QSE_NULL && v->vendorpec == i, "unable to add a vendor");

		qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("testx%d"), i);
		v = qse_raddic_addvendor (dic, tmp, i);
		_assert (v != QSE_NULL && v->vendorpec == i, "unable to add a vendor");

		qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("testy%d"), i);
		v = qse_raddic_addvendor (dic, tmp, i);
		_assert (v != QSE_NULL && v->vendorpec == i, "unable to add a vendor");
	}

	for (i = 1; i < COUNT1; i++)
	{
		int n;
		n = qse_raddic_deletevendorbyvalue (dic, i);
		_assert (n == 0, "unable to delete a vendor by value");

		n = qse_raddic_deletevendorbyvalue (dic, i);
		_assert (n == 0, "unable to delete a vendor by value");

		n = qse_raddic_deletevendorbyvalue (dic, i);
		_assert (n == 0, "unable to delete a vendor by value");

		n = qse_raddic_deletevendorbyvalue (dic, i);
		_assert (n <= -1, "erroreneously successful vendor deletion by value");
	}

	qse_raddic_close (dic);
	return 0;
}


static int test2 ()
{
	qse_raddic_t* dic;
	qse_raddic_attr_t* attr, * v;
	qse_raddic_attr_flags_t f;
	int i, j;

	dic = qse_raddic_open (QSE_MMGR_GETDFL(), 0);
	_assert (dic != QSE_NULL, "unable to create a radius dictionary");

	for (j = 0; j < 100; j++)
	{
		for (i = 0; i <= 255; i++)
		{
			qse_char_t tmp[64];
			qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test%d-%d"), j, i);
			attr = qse_raddic_addattr (dic, tmp, j, QSE_RADDIC_ATTR_TYPE_STRING, i, &f);
			_assert (attr != QSE_NULL, "unable to add an attribute");
			_assert (attr->attr == QSE_RADDIC_ATTR_MAKE(j, i), "wrong attr value");
			_assert (qse_strcasecmp(attr->name, tmp) == 0, "wrong attr name");
		}
	}

	for (j = 0; j < 100; j++)
	{
		for (i = 0; i <= 255; i++)
		{
			qse_char_t tmp[64];
			qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test%d-%d"), j, i);
			attr = qse_raddic_findattrbyname (dic, tmp);
			_assert (attr != QSE_NULL, "unable to add an attribute");
			_assert (attr->attr == QSE_RADDIC_ATTR_MAKE(j, i), "wrong attr value");
			_assert (qse_strcasecmp(attr->name, tmp) == 0, "wrong attr name");

			attr = qse_raddic_findattrbyvalue (dic, QSE_RADDIC_ATTR_MAKE(j, i));
			_assert (attr != QSE_NULL, "unable to add an attribute");
			_assert (attr->attr == QSE_RADDIC_ATTR_MAKE(j, i), "wrong attr value");
			_assert (qse_strcasecmp(attr->name, tmp) == 0, "wrong attr name");
		}
	}

	for (j = 0; j < 100; j++)
	{
		for (i = 0; i <= 255; i++)
		{
			qse_char_t tmp[64];
			qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("testx%d-%d"), j, i);
			attr = qse_raddic_addattr (dic, tmp, j, QSE_RADDIC_ATTR_TYPE_STRING, i, &f);
			_assert (attr != QSE_NULL, "unable to add an attribute");
			_assert (attr->attr == QSE_RADDIC_ATTR_MAKE(j, i), "wrong attr value");
			_assert (qse_strcasecmp(attr->name, tmp) == 0, "wrong attr name");

			qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("testy%d-%d"), j, i);
			attr = qse_raddic_addattr (dic, tmp, j, QSE_RADDIC_ATTR_TYPE_STRING, i, &f);
			_assert (attr != QSE_NULL, "unable to add an attribute");
			_assert (attr->attr == QSE_RADDIC_ATTR_MAKE(j, i), "wrong attr value");
			_assert (qse_strcasecmp(attr->name, tmp) == 0, "wrong attr name");
		}
	}

	for (j = 0; j < 100; j++)
	{
		for (i = 0; i <= 255; i++)
		{
			qse_char_t tmp[64], tmpx[64], tmpy[64];
			qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test%d-%d"), j, i);
			qse_strxfmt(tmpx, QSE_COUNTOF(tmpx), QSE_T("testx%d-%d"), j, i);
			qse_strxfmt(tmpy, QSE_COUNTOF(tmpy), QSE_T("testy%d-%d"), j, i);

			attr = qse_raddic_findattrbyname (dic, tmp);
			_assert (attr != QSE_NULL, "unable to add an attribute");
			_assert (attr->attr == QSE_RADDIC_ATTR_MAKE(j, i), "wrong attr value");
			_assert (qse_strcasecmp(attr->name, tmp) == 0, "wrong attr name");

			attr = qse_raddic_findattrbyname (dic, tmpx);
			_assert (attr != QSE_NULL, "unable to add an attribute");
			_assert (attr->attr == QSE_RADDIC_ATTR_MAKE(j, i), "wrong attr value");
			_assert (qse_strcasecmp(attr->name, tmpx) == 0, "wrong attr name");

			attr = qse_raddic_findattrbyname (dic, tmpy);
			_assert (attr != QSE_NULL, "unable to add an attribute");
			_assert (attr->attr == QSE_RADDIC_ATTR_MAKE(j, i), "wrong attr value");
			_assert (qse_strcasecmp(attr->name, tmpy) == 0, "wrong attr name");

			attr = qse_raddic_findattrbyvalue (dic, QSE_RADDIC_ATTR_MAKE(j, i));
			_assert (attr != QSE_NULL, "unable to add an attribute");
			_assert (attr->attr == QSE_RADDIC_ATTR_MAKE(j, i), "wrong attr value");
			_assert (qse_strcasecmp(attr->name, tmpy) == 0, "wrong attr name");

			_assert (attr->nexta != QSE_NULL, "unable to find an old attribute");
			_assert (attr->nexta->attr == QSE_RADDIC_ATTR_MAKE(j, i), "wrong attr value");
			_assert (qse_strcasecmp(attr->nexta->name, tmpx) == 0, "wrong attr name");

			_assert (attr->nexta->nexta != QSE_NULL, "unable to find an old attribute");
			_assert (attr->nexta->nexta->attr == QSE_RADDIC_ATTR_MAKE(j, i), "wrong attr value");
			_assert (qse_strcasecmp(attr->nexta->nexta->name, tmp) == 0, "wrong attr name");

			_assert (attr->nexta->nexta->nexta == QSE_NULL, "wrong attribute chian");
		}
	}

	for (j = 0; j < 100; j++)
	{
		for (i = 0; i <= 255; i++)
		{
			qse_char_t tmp[64];
			int n;

			qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("testx%d-%d"), j, i);
			n = qse_raddic_deleteattrbyname (dic, tmp);
			_assert (n == 0, "erroreneous attribute deletion failure by name");
		}
	}

	for (j = 0; j < 100; j++)
	{
		for (i = 0; i <= 255; i++)
		{
			qse_char_t tmp[64], tmpx[64], tmpy[64];
			qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test%d-%d"), j, i);
			qse_strxfmt(tmpy, QSE_COUNTOF(tmpy), QSE_T("testx%d-%d"), j, i);
			qse_strxfmt(tmpy, QSE_COUNTOF(tmpy), QSE_T("testy%d-%d"), j, i);

			attr = qse_raddic_findattrbyname (dic, tmp);
			_assert (attr != QSE_NULL, "unable to add an attribute");
			_assert (attr->attr == QSE_RADDIC_ATTR_MAKE(j, i), "wrong attr value");
			_assert (qse_strcasecmp(attr->name, tmp) == 0, "wrong attr name");

			attr = qse_raddic_findattrbyname (dic, tmpx);
			_assert (attr == QSE_NULL, "errorneous search success");

			attr = qse_raddic_findattrbyname (dic, tmpy);
			_assert (attr != QSE_NULL, "unable to add an attribute");
			_assert (attr->attr == QSE_RADDIC_ATTR_MAKE(j, i), "wrong attr value");
			_assert (qse_strcasecmp(attr->name, tmpy) == 0, "wrong attr name");


			attr = qse_raddic_findattrbyvalue (dic, QSE_RADDIC_ATTR_MAKE(j, i));
			_assert (attr != QSE_NULL, "unable to add an attribute");
			_assert (attr->attr == QSE_RADDIC_ATTR_MAKE(j, i), "wrong attr value");
			_assert (qse_strcasecmp(attr->name, tmpy) == 0, "wrong attr name");

			_assert (attr->nexta != QSE_NULL, "unable to find an old attribute");
			_assert (attr->nexta->attr == QSE_RADDIC_ATTR_MAKE(j, i), "wrong attr value");
			_assert (qse_strcasecmp(attr->nexta->name, tmp) == 0, "wrong attr name");

			_assert (attr->nexta->nexta == QSE_NULL, "wrong attribute chian");
		}
	}


	{
		int n;
		n = qse_raddic_deleteattrbyvalue (dic, QSE_RADDIC_ATTR_MAKE(0, 0));
		_assert (n == 0, "errorneous deletion failure by value");

		n = qse_raddic_deleteattrbyvalue (dic, QSE_RADDIC_ATTR_MAKE(0, 0));
		_assert (n == 0, "errorneous deletion failure by value");

		n = qse_raddic_deleteattrbyvalue (dic, QSE_RADDIC_ATTR_MAKE(0, 0));
		_assert (n <= -1, "errorneous deletion success by value");
	}

	qse_raddic_close (dic);
	return 0;
}

int main ()
{
	qse_open_stdsios (); 
	R (test1);
	R (test2);
	qse_close_stdsios ();
	return 0;
}

