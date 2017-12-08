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
	qse_raddic_vendor_t* vendor;
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
	_assert (vendor != QSE_NULL && vendor->vendorpec == 12365, "unabled to find a vendor named Abiyo.Net");

	vendor = qse_raddic_findvendorbyvalue (dic, 12365);
	_assert (vendor != QSE_NULL && vendor->vendorpec == 12365, "unabled to find a vendor of value 12365");
	_assert (qse_strcasecmp(vendor->name, QSE_T("abiyo-aliased.net")) == 0, "unabled to find a vendor of value 12365");

	vendor = qse_raddic_findvendorbyname (dic, QSE_T("Abiyo-aliased.Net"));
	_assert (vendor != QSE_NULL && vendor->vendorpec == 12365, "unabled to find a vendor named Abiyo-aliased.Net");
	_assert (qse_strcasecmp(vendor->name, QSE_T("abiyo-aliased.net")) == 0, "unabled to find a vendor of value 12365");

#define COUNT 65535

	for (i = 0; i < COUNT; i++)
	{
		qse_char_t tmp[64];
		qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test%d"), i);
		vendor = qse_raddic_addvendor (dic, tmp, i);
		_assert (vendor != QSE_NULL, "unable to add a vendor");
		_assert (vendor->vendorpec == i, "wrong vendor value");
		_assert (qse_strcasecmp(vendor->name, tmp) == 0, "wrong vendor name");
	}

	for (i = 0; i < COUNT; i++)
	{
		qse_char_t tmp[64];
		qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test%d"), i);
		vendor = qse_raddic_findvendorbyname (dic, tmp);
		_assert (vendor != QSE_NULL, "unable to find a vendor");
		_assert (vendor->vendorpec == i, "wrong vendor value");
		_assert (qse_strcasecmp(vendor->name, tmp) == 0, "wrong vendor name");
	}

	for (i = 0; i < COUNT; i++)
	{
		qse_char_t tmp[64];
		qse_strxfmt(tmp, QSE_COUNTOF(tmp), QSE_T("test%d"), i);
		vendor = qse_raddic_findvendorbyvalue (dic, i);
		_assert (vendor != QSE_NULL, "unable to find a vendor");
		_assert (vendor->vendorpec == i, "wrong vendor value");
		_assert (qse_strcasecmp(vendor->name, tmp) == 0, "wrong vendor name");
	}

	qse_raddic_close (dic);
	return 0;
}

int main ()
{
	qse_open_stdsios (); 
	R (test1);
	qse_close_stdsios ();
	return 0;
}

