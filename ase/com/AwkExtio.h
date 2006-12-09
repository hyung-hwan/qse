// AwkExtio.h : Declaration of the CAwkExtio

#ifndef __AWKEXTIO_H_
#define __AWKEXTIO_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CAwkExtio
class ATL_NO_VTABLE CAwkExtio : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAwkExtio, &CLSID_AwkExtio>,
	public IDispatchImpl<IAwkExtio, &IID_IAwkExtio, &LIBID_ASELib>
{
public:
	CComBSTR name;
	int type;
	int mode;

	CAwkExtio ();
	~CAwkExtio ();

DECLARE_REGISTRY_RESOURCEID(IDR_AWKEXTIO)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAwkExtio)
	COM_INTERFACE_ENTRY(IAwkExtio)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IAwkExtio
public:
	STDMETHOD(get_Mode)(/*[out, retval]*/ int *pVal);
	STDMETHOD(put_Mode)(/*[in]*/ int newVal);
	STDMETHOD(get_Type)(/*[out, retval]*/ int *pVal);
	STDMETHOD(put_Type)(/*[in]*/ int newVal);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Name)(/*[in]*/ BSTR newVal);
};

#endif //__AWKEXTIO_H_
